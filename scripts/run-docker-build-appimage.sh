#!/bin/bash

set -xe
cd "`dirname $(readlink -f ${0})`"

docker build -t chiaki-xenial . -f Dockerfile.xenial
cd ..
docker run --rm \
	-v "`pwd`:/build/chiaki" \
	-w "/build/chiaki" \
	--device /dev/fuse \
	--cap-add SYS_ADMIN \
	-t chiaki-xenial \
	/bin/bash -c "scripts/build-appimage.sh"

