#!/bin/bash

set -xveo pipefail

arg1=$1
CHIAKI_ENABLE_SWITCH_LINUX="ON"
build="./build"
if [ "$arg1" != "linux" ]; then
  CHIAKI_ENABLE_SWITCH_LINUX="OFF"
  source /opt/devkitpro/switchvars.sh
  toolchain=/opt/devkitpro/switch.cmake

  export CC=${TOOL_PREFIX}gcc
  export CXX=${TOOL_PREFIX}g++
  build="./build_switch"
fi

SCRIPTDIR=$(dirname "$0")
BASEDIR=$(realpath "${SCRIPTDIR}/../../")

build_chiaki (){
  pushd "${BASEDIR}"
    #rm -rf ./build

    cmake -B "${build}" -DCMAKE_TOOLCHAIN_FILE=${toolchain} \
      -DCHIAKI_ENABLE_TESTS=OFF \
      -DCHIAKI_ENABLE_CLI=OFF \
      -DCHIAKI_ENABLE_GUI=OFF \
      -DCHIAKI_ENABLE_ANDROID=OFF \
      -DCHIAKI_ENABLE_SWITCH=ON \
      -DCHIAKI_ENABLE_SWITCH_LINUX="${CHIAKI_ENABLE_SWITCH_LINUX}" \
      -DCHIAKI_LIB_ENABLE_MBEDTLS=ON

    pushd "${BASEDIR}/${build}"
      make -j8
    popd
  popd
}

build_chiaki

