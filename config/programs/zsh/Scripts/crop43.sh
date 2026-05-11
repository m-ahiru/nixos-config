#!/usr/bin/env bash

input="$1"
output="${input%.*}_4by3.mp4"

ffmpeg -i "$input" -vf "crop=ih*4/3:ih" -c:v libx264 -crf 18 -preset slow -c:a copy "$output"
