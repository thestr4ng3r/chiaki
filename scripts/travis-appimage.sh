#!/bin/bash

cd build && make install DESTDIR=../appdir && cd .. || exit 1
wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage && chmod +x linuxdeploy-x86_64.AppImage || exit 1
wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage && chmod +x linuxdeploy-plugin-qt-x86_64.AppImage || exit 1
source /opt/qt512/bin/qt512-env.sh || exit 1

if [ -n "$SDL2_FROM_SRC" ]; then
	export LD_LIBRARY_PATH="$TRAVIS_BUILD_DIR/sdl2-prefix/lib:$LD_LIBRARY_PATH" || exit 1
fi

export EXTRA_QT_PLUGINS=opengl

./linuxdeploy-x86_64.AppImage --appdir=appdir -e appdir/usr/bin/chiaki -d appdir/usr/share/applications/chiaki.desktop --plugin qt --output appimage || exit 1
export DEPLOY_FILE="Chiaki-${CHIAKI_VERSION}-Linux-x86_64.AppImage" || exit 1
mv Chiaki-*-x86_64.AppImage "$DEPLOY_FILE" || exit 1