#!/usr/bin/env bash

if [ -z "$1" ]; then
    echo "Usage:"
    echo
    echo "$0 filename.mp4"
    echo "or"
    echo "$0 filename.mp4 25"
    echo "  (25 = 25MB^)"
    exit 1
fi

target_mb=$2
if [ -z "$target_mb" ]; then
    # default target size is 25MB
    target_mb=25
fi

file=$1
target_size=$((target_mb * 1000 * 1000 * 8))

# Get the video duration using ffprobe
length=$(ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 "$file")
length_round_up=$(echo "$length" | cut -d '.' -f 1)
length_round_up=$((length_round_up + 1))

# 128k audio bitrate
audio_bitrate=$((128 * 1000))

# Calculate video bitrate
total_bitrate=$((target_size / length_round_up))
video_bitrate=$((total_bitrate - audio_bitrate))

# Buffer size calculation
bufsize=$((target_size / 20))

# Extract file details
filedrive=$(dirname "$file")
filename=$(basename "$file")
fileextension="${filename##*.}"
filename_noext="${filename%.*}"

# Output file name
file_output="${filedrive}/${filename_noext}-out.${fileextension}"

# Run ffmpeg to adjust the bitrate
ffmpeg -i "$file" -b:v "$video_bitrate" -maxrate:v "$video_bitrate" -bufsize:v "$bufsize" -b:a "$audio_bitrate" "$file_output"

echo "Output file: $file_output"
