#!/bin/bash

cd $(dirname "${BASH_SOURCE[0]}")/..
ROOT="`pwd`"

TAG=n4.2

git clone https://git.ffmpeg.org/ffmpeg.git --depth 1 -b $TAG && cd ffmpeg || exit 1

./configure --disable-all --enable-avcodec --enable-decoder=h264 --prefix="$ROOT/ffmpeg-prefix" || exit 1
make -j4 || exit 1
make install || exit 1