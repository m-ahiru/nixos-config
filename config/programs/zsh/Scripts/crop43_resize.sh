#!/usr/bin/env bash
# crop to 4:3 + compress to target MB in one encode

if [ -z "$1" ]; then
    echo "Usage: $0 filename.mp4 [target_mb]  (default 25)"
    exit 1
fi

file=$1
target_mb=${2:-25}
target_size=$((target_mb * 1000 * 1000 * 8))

length=$(ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 "$file")
length_round_up=$(echo "$length" | cut -d '.' -f 1)
length_round_up=$((length_round_up + 1))

audio_bitrate=$((128 * 1000))
total_bitrate=$((target_size / length_round_up))
video_bitrate=$((total_bitrate - audio_bitrate))
bufsize=$((target_size / 20))

filedrive=$(dirname "$file")
filename=$(basename "$file")
fileextension="${filename##*.}"
filename_noext="${filename%.*}"
file_output="${filedrive}/${filename_noext}_4by3-out.${fileextension}"

ffmpeg -i "$file" -vf "crop=ih*4/3:ih" \
    -b:v "$video_bitrate" -maxrate:v "$video_bitrate" -bufsize:v "$bufsize" \
    -b:a "$audio_bitrate" "$file_output"

echo "Output file: $file_output"
