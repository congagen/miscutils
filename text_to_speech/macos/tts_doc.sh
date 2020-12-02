##!/bin/bash

echo "Input path (*.txt):"
read text_path

echo "Select voice: (Fred, Serena, Daniel)"
read voice

echo "Words per minte:"
read wpm

say -v "$voice" -f "$text_path" -o "$text_path"-"$voice"-"$wpm".wav --rate="$wpm" --file-format WAVE --data-format I16 --progress
