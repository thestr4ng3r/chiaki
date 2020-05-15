#!/bin/bash

cd "`dirname $(readlink -f ${0})`/../.."

docker run \
	-v "`pwd`:/build/chiaki" \
	-t \
	chiaki-switch \
	-c "cd /build/chiaki && scripts/switch/build.sh"

