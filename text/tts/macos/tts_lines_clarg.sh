##!/bin/bash

# Input path
text_path=$1

# Voice (Fred, Serena, Daniel)
voice=$2

# Words per minte
wpm=$3

# Out Dir
o_dir=$4

c=0

mkdir $o_dir

while read -r line
do
    c=$((c + 1))
    echo -e "$line"
    #say -v "$voice" -o $o_dir/$c"-"$voice$wpm.wav --rate="$wpm" --file-format WAVE --data-format I16 "$line"
    say -v "$voice" -o $o_dir/$c.wav --rate="$wpm" --file-format WAVE --data-format I16@44100 --channels 2 "$line"
done < $text_path
