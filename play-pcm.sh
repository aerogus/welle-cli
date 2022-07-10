#!/usr/bin/env bash

##
# Joue un stream PCM
##

cat "$1" | ffplay -f s16le -ar 48k -ac 2 -
