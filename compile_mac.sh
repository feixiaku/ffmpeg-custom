#!/bin/bash

make clean

./configure --enable-shared --disable-static --prefix=output/mac
