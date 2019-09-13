#!/bin/bash

cd $(dirname "${BASH_SOURCE[0]}")/..
ROOT="`pwd`"

URL=https://www.libsdl.org/release/SDL2-2.0.10.tar.gz
FILE=SDL2-2.0.10.tar.gz
DIR=SDL2-2.0.10

if [ ! -d "$DIR" ]; then
	wget "$URL" || exit 1
	tar -xf "$FILE" || exit 1
fi

cd "$DIR" || exit 1

mkdir -p build && cd build || exit 1
cmake \
	-DCMAKE_INSTALL_PREFIX="$ROOT/sdl2-prefix" \
	-DSDL_ATOMIC=OFF \
	-DSDL_AUDIO=OFF \
	-DSDL_CPUINFO=OFF \
	-DSDL_EVENTS=ON \
	-DSDL_FILE=OFF \
	-DSDL_FILESYSTEM=OFF \
	-DSDL_HAPTIC=ON \
	-DSDL_JOYSTICK=ON \
	-DSDL_LOADSO=OFF \
	-DSDL_RENDER=OFF \
	-DSDL_SHARED=ON \
	-DSDL_STATIC=OFF \
	-DSDL_TEST=OFF \
	-DSDL_THREADS=ON \
	-DSDL_TIMERS=OFF \
	-DSDL_VIDEO=OFF \
	.. || exit 1
# SDL_THREADS is not needed, but it doesn't compile without

make -j4 || exit 1
make install || exit 1

