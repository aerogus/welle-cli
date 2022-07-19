#!/usr/bin/env bash

##
# Joue un stream PCM
#
# Usage: ./play-pcm.sh audio.pcm
##

FORMAT="s16le"
SAMPLING_RATE="48k"
CHANNELS="2"

if [[ ! -f $1 ]] && [[ ! -p $1 ]]; then
  "fichier ou tube nommé $1 non trouvé"
  exit 1
fi

cat "$1" | ffplay -f $FORMAT -ar $SAMPLING_RATE -ac $CHANNELS -
