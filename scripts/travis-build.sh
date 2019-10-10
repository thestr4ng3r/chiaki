#!/bin/bash

mkdir build && cd build || exit 1
cmake \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH \
	-DCHIAKI_ENABLE_TESTS=ON \
	-DCHIAKI_ENABLE_CLI=OFF \
	-DCHIAKI_GUI_ENABLE_QT_GAMEPAD=OFF \
	-DCHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER=ON \
	$CMAKE_EXTRA_ARGS \
	.. || exit 1
make -j4 || exit 1
test/chiaki-unit || exit 1