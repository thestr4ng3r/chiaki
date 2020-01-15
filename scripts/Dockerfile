
FROM utensils/opengl:19.0.8 AS builder
LABEL maintainer="chiaki-docker@florianmaerkl.de"
WORKDIR /app
RUN echo "@testing http://dl-cdn.alpinelinux.org/alpine/edge/testing" >> /etc/apk/repositories \
	&& apk update \
	&& apk --no-cache add \
	build-base  \
	cmake \
	ffmpeg-dev \
	git \
	openssl \
	opus-dev \
	protobuf \
#	py3-protobuf@testing \ # Enable this when the OpenGL image updates alpine
	qt5-qtbase \
	qt5-qtmultimedia-dev \
	qt5-qtsvg-dev \
	sdl2-dev \
	&& pip3 install protobuf \
	&& git clone https://github.com/thestr4ng3r/chiaki.git /app \
	&& git submodule update --init \
	&& mkdir build \
	&& cd build \
	&& cmake .. \
	&& make \
	&& ./test/chiaki-unit \
	&& rm -rf /var/cache/apk/*

FROM utensils/opengl:19.0.8
RUN apk update \
	&& apk --no-cache add \
	ffmpeg \
	qt5-qtbase \
	qt5-qtmultimedia \
	qt5-qtsvg \
	sdl2 \
	&& rm -rf /var/cache/apk/* \
	&& addgroup ps4 && adduser -D -G ps4 ps4 \
	&& chown -R ps4: /home/ps4
WORKDIR /home/ps4
COPY --from=builder /app/build/gui/chiaki .
USER ps4
VOLUME /home/ps4/.local/share/Chiaki
CMD ["/home/ps4/chiaki"]

