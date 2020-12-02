##!/bin/bash

echo "Input path: "
read text_path

echo "Select voice: (Fred, Serena, Daniel)"
read voice

echo "Words per minte:"
read wpm

c=0

mkdir "output"
o_dir=output/"$voice"_"$wpm"
mkdir $o_dir

while read -r line
do
    c=$((c + 1))
    echo -e "$line"
    say -v "$voice" -o $o_dir/$c"-"$voice$wpm.wav --rate="$wpm" --file-format WAVE --data-format I16 "$line"
done < $text_path
