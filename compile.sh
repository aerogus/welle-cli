#!/usr/bin/env bash
##
# welle-cli compilation script
##

declare -r ABS_PATH="$( cd "$(dirname "$0")" || return; pwd -P )"

BUILD_DIR="$ABS_PATH/build"

# cleanup previous compilation
if [[ -d "$BUILD_DIR" ]] ; then
  rm -Rf "$BUILD_DIR"
fi
mkdir "$BUILD_DIR"

cd "$BUILD_DIR" && \
cmake .. \
&& make \
&& sudo make install
