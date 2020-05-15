FROM archlinux:latest

ENV DEVKITPRO=/opt/devkitpro
ENV DEVKITARM=/opt/devkitpro/devkitARM
ENV DEVKITPPC=/opt/devkitpro/devkitPPC
ENV PATH="${PATH}:${DEVKITARM}/bin/:${DEVKITPPC}/bin/"

ENV WORKDIR="/build"
WORKDIR "${WORKDIR}"

# Upgrade image
RUN pacman --noconfirm -Syu

# Install requirements for libtransistor 
RUN pacman --noconfirm -S \
    llvm \
    clang \
    lld \
    python \
    python-pip \
    python-virtualenv \
    squashfs-tools \
    base-devel \
    git \
    cmake \
    libx11 \
    vim

RUN pacman-key --init
# Install devkitpro 
# doc source :
# https://devkitpro.org/wiki/devkitPro_pacman

# First import the key which is used to validate the packages 
RUN pacman-key --recv-keys F7FD5492264BB9D0
RUN pacman-key --lsign F7FD5492264BB9D0

# Add the devkitPro repositories
ADD devkit_repo ./devkit_repo
RUN cat ./devkit_repo >> /etc/pacman.conf
# Install the keyring which adds more keys which may be used to verify the packages. 
RUN pacman --noconfirm -U https://downloads.devkitpro.org/devkitpro-keyring-r1.787e015-2-any.pkg.tar.xz
# Now resync the database and update installed packages.
RUN pacman -Sy

RUN pacman --noconfirm -Syu

#RUN pacman --noconfirm -S $(pacman -Slq dkp-libs)

RUN pacman --noconfirm -S \
    protobuf \
    python-protobuf \
    sfml \
    devkitARM \
    switch-pkg-config \
    devkitpro-pkgbuild-helpers \
    switch-dev \
    switch-zlib \
    switch-sdl2 \
    switch-freetype \
    switch-curl \
    switch-mesa \
    switch-glad \
    switch-glm \
    switch-libconfig \
    switch-sdl2_gfx \
    switch-sdl2_ttf \
    switch-sdl2_image \
    switch-libexpat \
    switch-bzip2 \
    switch-libopus \
    switch-ffmpeg \
	switch-mbedtls

# RUN pip3 install -U pip

VOLUME ${WORKDIR}
# nxlink server port
EXPOSE 28771
ENTRYPOINT ["/bin/bash"]
