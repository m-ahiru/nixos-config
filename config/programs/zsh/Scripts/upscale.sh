#!/usr/bin/env bash
set -euo pipefail

if [ $# -lt 1 ]; then
  echo "Usage: $0 <inputfile> [optional_outputfile]"
  exit 1
fi

IN="$1"
if [ ! -f "$IN" ]; then
  echo "Eingabedatei nicht gefunden: $IN"
  exit 1
fi

# Basename/Output bestimmen (robust für .mp4/.MP4 etc.)
filename="$(basename "$IN")"
base="${filename%.*}"
OUT="${2:-${base}_upscaled.mp4}"

# Prüfe verfügbare Encoder
has_encoder() {
  ffmpeg -hide_banner -encoders 2>/dev/null | grep -q "$1"
}

# Versuche GPU (VAAPI), sonst CPU
USE="x264"
if has_encoder "h264_vaapi"; then
  USE="h264_vaapi"
elif has_encoder "hevc_vaapi"; then
  USE="hevc_vaapi"
fi

echo "Upscaling \"$IN\" → \"$OUT\" ..."
case "$USE" in
  h264_vaapi)
    # VAAPI braucht nv12 + hwupload; scale via scale_vaapi ist am schnellsten
    # Hinweis: /dev/dri/renderD128 ist der Standard-Rendernode
    ffmpeg -y -hide_banner \
      -vaapi_device /dev/dri/renderD128 \
      -i "$IN" \
      -vf "format=nv12,hwupload,scale_vaapi=w=3840:h=2160" \
      -c:v h264_vaapi -qp 18 \
      -c:a copy \
      "$OUT"
    ;;
  hevc_vaapi)
    ffmpeg -y -hide_banner \
      -vaapi_device /dev/dri/renderD128 \
      -i "$IN" \
      -vf "format=nv12,hwupload,scale_vaapi=w=3840:h=2160" \
      -c:v hevc_vaapi -qp 22 \
      -c:a copy \
      "$OUT"
    ;;
  x264)
    # CPU-Fallback
    ffmpeg -y -hide_banner \
      -i "$IN" \
      -vf "scale=3840:2160:flags=lanczos" \
      -c:v libx264 -preset veryfast -crf 18 \
      -pix_fmt yuv420p \
      -c:a copy \
      "$OUT"
    ;;
esac

echo "Fertig: $OUT"

