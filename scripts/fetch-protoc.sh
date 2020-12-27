#!/bin/bash

set -xe

cd $(dirname "${BASH_SOURCE[0]}")/..
cd "./$1"
ROOT="`pwd`"

URL=https://github.com/protocolbuffers/protobuf/releases/download/v3.9.1/protoc-3.9.1-linux-x86_64.zip

curl -L "$URL" -o protoc.zip
unzip protoc.zip -d protoc

