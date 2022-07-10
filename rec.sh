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

SERVICES=$(echo "$SERVICES" | tr '[:upper:]' '[:lower:]')
SERVICES_LIST=$(echo "$SERVICES" | tr "," "\n")
EXTENSIONS=(pcm txt)

# Création des tubes nommés pour les services à capter
for SERVICE in $SERVICES_LIST
do
    SERVICE_DIR="${REC_DIR}/${SERVICE}"
    if [[ ! -d "$SERVICE_DIR" ]]; then
        mkdir -p "$SERVICE_DIR"
    fi
    FILE_PREFIX="${SERVICE_DIR}/${SERVICE}"
    for EXTENSION in "${EXTENSIONS[@]}"
    do
        echo "rien"
        # ne pas effacer le tube nommé tout le temps ...
        # impossible d'armer à l'avance sinon
        #FILENAME="${FILE_PREFIX}.${EXTENSION}"
        #if [[ -f "$FILENAME" ]] || [[ -p "$FILENAME" ]]; then
        #    unlink "$FILENAME"
        #fi
        #echo "Création tube nommé $FILENAME"
        #mkfifo "$FILENAME"

        # simulation de lecture du flux
        #echo "Lecture du tube nommé $FILENAME"
        #"${ABS_PATH}/read-pipe.sh" "${FILENAME}" &
    done
done

"$WELLE_CLI_BIN" -c "$CHANNEL" -s "$SERVICES" -o "$REC_DIR" 2>&1

