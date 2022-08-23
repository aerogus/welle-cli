#!/usr/bin/env bash

##
# captation DAB+ avec welle-cli
#
# canaux actifs à Paris: 5A, 6A, 6C, 6D, 8C, 9A, 9B, 11A, 11B
##

declare -r ABS_PATH="$( cd "$(dirname "$0")" || return; pwd -P )"
declare -r DEFAULT_CONF_FILE="$ABS_PATH/conf/default.ini"
declare -r WELLE_CLI_BIN="/usr/local/bin/welle-cli"
declare -r CUSTOM_CONF_FILE="$1"
declare -r AUTOSTART="$2"
declare -ri SIMU=0

echo "- Autostart: $AUTOSTART"
echo "- Simu:      $SIMU"

if [[ $CUSTOM_CONF_FILE ]]; then
    if [[ ! -f "$CUSTOM_CONF_FILE" ]]; then
        echo "- Fichier de configuration personnalisée $CUSTOM_CONF_FILE non trouvé"
        exit 1
    else
        echo "- Config:    $CUSTOM_CONF_FILE"
        . "$CUSTOM_CONF_FILE"
    fi
elif [[ ! -f "$DEFAULT_CONF_FILE" ]]; then
    echo "- Fichier de configuration par défaut $DEFAULT_CONF_FILE non trouvé"
    exit 1
else
    echo "- Config:    $DEFAULT_CONF_FILE"
    . "$DEFAULT_CONF_FILE"
fi

if [[ ! -d "$REC_DIR" ]]; then
    if mkdir -p "$REC_DIR" 2>/dev/null; then
        echo "- Création du répertoire de stockage $REC_DIR"
    else
        echo "- Erreur à la création du répertoire de stockage $REC_DIR"
        exit 1
    fi
fi

echo "- Stockage:  $REC_DIR"
echo "- Block:     $BLOCK"
echo "- Services:  $SERVICE_IDS"

# en interne, les servicesId sont en minuscules
SERVICE_IDS=$(echo "$SERVICE_IDS" | tr '[:upper:]' '[:lower:]')
SERVICE_IDS_ARRAY=$(echo "$SERVICE_IDS" | tr "," "\n")
EXTENSIONS=(pcm ndjson)

# Ménage d'anciennes éventuelles de read-pipe.sh
if [[ $SIMU -gt 0 ]]; then
    PREVIOUS_PIDS=$(ps | grep read-pipe.sh | grep -v grep | awk '{print $1}';)
    if [[ "$PREVIOUS_PIDS" ]]; then
        echo "Termine (SIGTERM) les processus read-pipe.sh suivants : $PREVIOUS_PIDS"
        kill -15 $PREVIOUS_PIDS
    else
        echo "aucun processus read-pipe.sh à terminer"
    fi
fi

# Création des tubes nommés pour les services à capter
for SERVICE_ID in $SERVICE_IDS_ARRAY
do
    SERVICE_DIR="${REC_DIR}/${SERVICE_ID}"
    if [[ ! -d "$SERVICE_DIR" ]]; then
        echo "- création répertoire $SERVICE_DIR"
        mkdir -p "$SERVICE_DIR"
    fi

    # ménage données .msc (données associées brutes avant décodage)
    MSC_FILENAME=="${FILE_PREFIX}.msc"
    if [[ -f "$MSC_FILENAME" ]]; then
        rm "$MSC_FILENAME"
        echo "Effacement du fichier .msc $MSC_FILENAME"
    fi

    FILE_PREFIX="${SERVICE_DIR}/${SERVICE_ID}"
    for EXTENSION in "${EXTENSIONS[@]}"
    do
        # ne pas effacer le tube nommé tout le temps ...
        # impossible d'armer à l'avance sinon
        FILENAME="${FILE_PREFIX}.${EXTENSION}"

        if [[ -f "$FILENAME" ]]; then
             rm "$FILENAME"
             echo "Effacement du fichier régulier $FILENAME"
        fi

        if [[ ! -p "$FILENAME" ]]; then
            echo "- Création du tube nommé $FILENAME"
            mkfifo "$FILENAME"
        else
            echo "- Tube nommé $FILENAME déjà existant"
        fi

        # simulation de lecture des flux (en background)
        if [[ $SIMU -gt 0 ]]; then
            echo "Simulation de lecture du tube nommé $FILENAME"
            "${ABS_PATH}/read-pipe.sh" "${FILENAME}" &
        fi
    done
done

if [[ -z "$AUTOSTART" ]] && [[ $SIMU -eq 0 ]]; then
    echo "Avez vous bien armé les captations pour tous les services demandés ? (o/N)"
    read -r CONFIRM
    if [[ $CONFIRM != "O" ]] && [[ $CONFIRM != "o" ]]; then
        echo "Arrêt"
        exit 0
    fi
else
    echo "Démarrage automatique"
fi

echo "- Lancement de welle-cli"
echo "---"
"$WELLE_CLI_BIN" -c "$BLOCK" -s "$SERVICE_IDS" -o "$REC_DIR" 2>&1

