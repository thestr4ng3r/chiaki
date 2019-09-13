#!/bin/bash

cd $(dirname "${BASH_SOURCE[0]}")/..
ROOT="`pwd`"

URL=https://github.com/protocolbuffers/protobuf/releases/download/v3.9.1/protoc-3.9.1-linux-x86_64.zip

wget "$URL" -O protoc.zip || exit 1
unzip protoc.zip -d protoc || exit 1

