#!/usr/bin/env bash

##
# play a PCM stream
#
# Usage: ./play-pcm.sh audio.pcm
##

# ffplay arguments
FORMAT="s16le"
SAMPLING_RATE="48k"
CHANNELS="2"

if [[ ! -f $1 ]] && [[ ! -p $1 ]]; then
  "file or named pipe $1 not found"
  exit 1
fi

ffplay -f $FORMAT -ar $SAMPLING_RATE -ac $CHANNELS - < "$1"
