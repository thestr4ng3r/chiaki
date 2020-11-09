#!/bin/bash

docker run \
   	-v $HOME/.local/share/Chiaki:/home/ps4/.local/share/Chiaki \
   	-v $HOME/.config/Chiaki:/home/ps4/.config/Chiaki \
	-v /tmp/.X11-unix:/tmp/.X11-unix \
	--net host \
	-e DISPLAY=$DISPLAY \
	--device /dev/snd \
	--name chiaki \
	--rm \
	thestr4ng3r/chiaki

