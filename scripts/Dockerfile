# Run chiaki in a container
# xhost +"local:docker@"
# docker run -v $HOME/chiaki:/home/ps4/.local/share/Chiaki \
#   -v /tmp/.X11-unix:/tmp/.X11-unix \
#   --net host \
#   -e DISPLAY=$DISPLAY \
#   --device /dev/snd \
#   --name chiaki 
#   --rm \
#   strubbl_chiaki

FROM alpine:3 AS builder
LABEL maintainer="Strubbl <dockerfile-chiaki@linux4tw.de>"
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
  py3-protobuf@testing \
  qt5-qtbase \
  qt5-qtmultimedia-dev \
  qt5-qtsvg-dev \
  sdl2-dev \
  && git clone https://github.com/thestr4ng3r/chiaki.git /app \
  && git submodule update --init \
  && mkdir build \
  && cd build \
  && cmake .. \
  && make \
  && ./test/chiaki-unit \
  && rm -rf /var/cache/apk/*

FROM alpine:3
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

