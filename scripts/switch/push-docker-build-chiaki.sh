#!/bin/bash

cd "`dirname $(readlink -f ${0})`/../.."

docker run \
	-v "`pwd`:/build/chiaki" \
	-p 28771:28771 -ti \
	chiaki-switch \
	-c "/opt/devkitpro/tools/bin/nxlink -a $@ -s /build/chiaki/build_switch/switch/chiaki.nro"

