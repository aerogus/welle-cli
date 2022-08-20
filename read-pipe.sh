#!/usr/bin/env bash

##
# Simulateur de lecture d'un tube nommé
#
# $1 chemin du tube nommé à lire
##

NAMED_PIPE="$1"

OUTPUT_DEST="${NAMED_PIPE}.output"
#OUTPUT_DEST="/dev/null"

if [[ ! -p "${NAMED_PIPE}" ]]; then
    echo "${NAMED_PIPE} n'est pas un tube nommé"
    exit 1
fi

tail -f "${NAMED_PIPE}" >> "${OUTPUT_DEST}"
