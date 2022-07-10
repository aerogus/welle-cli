#!/usr/bin/env bash

##
# Simulateur de lecture d'un tube nommé
#
# $1 chemin du tube nommé à lire
#
# Trouver un moyen de killer facilement ces processus
##

declare -r ABS_PATH="$( cd "$(dirname "$0")" || return; pwd -P )"

NAMED_PIPE="$1"

if [[ ! -p "${NAMED_PIPE}" ]]; then
    exit 1
fi

tail -f "${NAMED_PIPE}" >> "${NAMED_PIPE}.output"
