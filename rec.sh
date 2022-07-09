#!/usr/bin/env bash

##
# captation DAB+
#
# canaux actifs à Paris: 5A, 6A, 6C, 6D, 8C, 9A, 9B, 11A, 11B
##

declare -r ABS_PATH="$( cd "$(dirname "$0")" || return; pwd -P )"
declare -r DEFAULT_CONF_FILE="$ABS_PATH/conf/captation.ini"
declare -r WELLE_CLI_BIN="/usr/local/bin/welle-cli"
declare -r CUSTOM_CONF_FILE="$1"

if [[ $CUSTOM_CONF_FILE ]]; then
    if [[ ! -f "$CUSTOM_CONF_FILE" ]]; then
        echo "Fichier de conf $CUSTOM_CONF_FILE non trouvé"
        exit 1
    else
        echo "Chargement de la conf personnalisée"
        . "$CUSTOM_CONF_FILE"
    fi
elif [[ ! -f "$DEFAULT_CONF_FILE" ]]; then
    echo "Fichier de conf $DEFAULT_CONF_FILE non trouvé"
    exit 1
else
    echo "Chargement de la conf par défaut"
    . "$DEFAULT_CONF_FILE"
fi

if [[ ! -d "$REC_DIR" ]]; then
    if mkdir -p "$REC_DIR" 2>/dev/null; then
        echo "Création du répertoire de stockage $REC_DIR"
    else
        echo "Erreur à la création du répertoire de stockage $REC_DIR"
        exit 1
    fi
fi

"$WELLE_CLI_BIN" -c "$CHANNEL" -s "$SERVICES" -o "$REC_DIR" 2> /dev/null

