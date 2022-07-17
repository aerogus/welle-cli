#!/usr/bin/env bash

# compile

declare -r ABS_PATH="$( cd "$(dirname "$0")" || return; pwd -P )"

BUILD_DIR="$ABS_PATH/build"

if [[ -d "$BUILD_DIR" ]] ; then
  rm -Rf "$BUILD_DIR"
fi
mkdir "$BUILD_DIR"

cd "$BUILD_DIR" && \
cmake .. -DRTLSDR=1 -DBUILD_WELLE_IO=OFF -DBUILD_WELLE_CLI=ON \
&& make \
&& sudo make install
