FFmpeg CUSTOM README
=============

FFmpeg-Custom is a tutorial for learning how to custom Demuxer/Codec/Filter.

## Demuxer

* `Dummy` as a simple demuxer.
        Usage:
        ffmpeg -video_size wxh -framerate 25 -pixel_format yuv420p -i input.dummy -c:v libx264 output.mp4
        ffmpeg -y -i input.avi output.dummy

## CODEC

* `Dummy` as a simple custom codec.

## Filter
* `Foobar` as a simple testbench filter.
        Usage:
        ffmpeg -i input.mp4 -vf foobar output.mp4

##Contact: 
E-mail: feixiaku@gmail.com
