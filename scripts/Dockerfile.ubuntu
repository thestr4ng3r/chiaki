# Run chiaki in a container
# docker run -v $HOME/chiaki:/home/ps4/.local/share/Chiaki \
#   -v /tmp/.X11-unix:/tmp/.X11-unix \
#   --net host \
#   -e DISPLAY=$DISPLAY \
#   --device /dev/snd \
#   --name chiaki 
#   --rm \
#   strubbl_chiaki

FROM ubuntu:19.04 AS builder
LABEL maintainer="Strubbl <dockerfile-chiaki@linux4tw.de>"
ENV DEBIAN_FRONTEND noninteractive
WORKDIR /app
RUN apt-get update \
  && apt-get install -y \
  build-essential \
  cmake \
  git \
  protobuf-compiler \
  libavcodec-dev \
  libavutil-dev \
  libopus-dev \
  libqt5svg5-dev \
  libsdl2-dev \
  libssl-dev \
  python3 \
  python3-distutils \
  python3-protobuf \
  qt5-default \
  qtmultimedia5-dev \
  && git clone https://github.com/thestr4ng3r/chiaki.git /app \
  && git submodule update --init \
  && mkdir build \
  && cd build \
  && cmake .. \
  && make

FROM ubuntu:19.04
ENV DEBIAN_FRONTEND noninteractive
RUN apt-get update \
  && apt-get install -y \
  libavcodec58 \
  libavutil56 \
  libopus0 \
  libqt5multimedia5 \
  libqt5svg5 \
  libsdl2-2.0-0 \
  --no-install-recommends \
  && rm -rf /var/lib/apt/lists/* \
  && useradd -ms /bin/bash ps4
WORKDIR /home/ps4
USER ps4
COPY --from=builder /app/build/gui/chiaki .
VOLUME /home/ps4/.local/share/Chiaki
ENTRYPOINT ["/home/ps4/chiaki"]

