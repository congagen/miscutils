# imgstrobe


### Requirements:
FFMPEG (https://www.ffmpeg.org/)
Python3 + Pillow (pip install Pillow)

--------------------------------------------------------------------------------


### Usage:

```sh
# Prep input images:
python3 app.py

# Imgseq -> Video:
ffmpeg -framerate 25 -pattern_type glob -i 'input/prepped/*.jpg' output/video.mp4

# Loop video (x100):
ffmpeg -stream_loop 100 -i output/video.mp4 -c copy output/looped.mp4
```
