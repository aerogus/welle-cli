#!/usr/bin/env bash

##
# named pipe player simulator
#
# $1 named pipe absolute path to read
##

NAMED_PIPE="$1"

OUTPUT_DEST="${NAMED_PIPE}.output"
#OUTPUT_DEST="/dev/null"

if [[ ! -p "${NAMED_PIPE}" ]]; then
    echo "${NAMED_PIPE} is not a named pipe"
    exit 1
fi

tail -f "${NAMED_PIPE}" >> "${OUTPUT_DEST}"
