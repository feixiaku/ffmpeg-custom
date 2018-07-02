#!/bin/bash

make clean

./configure \
    --enable-debug=3 \
    --disable-optimizations \
    --disable-asm \
    --disable-stripping \
    --enable-shared \
    --disable-static \
    --prefix=output/mac
