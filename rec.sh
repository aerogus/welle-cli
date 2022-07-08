#!/usr/bin/env bash

##
# captation DAB+
#
# $1 canal du multiplex
# à Paris: 5A, 6A, 6C, 6D, 8C, 9A, 9B, 11A, 11B
##

declare -r ABS_PATH="$( cd "$(dirname "$0")" || return; pwd -P )"

. "$ABS_PATH/conf/captation.ini"

if [[ ! -d "$REC_DIR" ]]; then
  mkdir "$REC_DIR"
fi

welle-cli -c "$CHANNEL" -o "$REC_DIR"

