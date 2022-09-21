#!/usr/bin/env bash

##
# DAB+ services multiplex recorder with welle-cli
#
# Active blocks in Paris: 5A, 6A, 6C, 6D, 8C, 9A, 9B, 11A, 11B
##

declare -r ABS_PATH="$( cd "$(dirname "$0")" || return; pwd -P )"
declare -r DEFAULT_CONF_FILE="$ABS_PATH/conf/default.ini"
declare -r LOG_PATH="$ABS_PATH/log"
declare -r WELLE_CLI_BIN="/usr/local/bin/welle-cli"
declare -r CUSTOM_CONF_FILE="$1"
declare -r AUTOSTART="$2"
declare -ri SIMU=0

echo "- Autostart: $AUTOSTART"
echo "- Simu:      $SIMU"

if [[ $CUSTOM_CONF_FILE ]]; then
    if [[ ! -f "$CUSTOM_CONF_FILE" ]]; then
        echo "- Custom configuration file $CUSTOM_CONF_FILE not found"
        exit 1
    else
        echo "- Config:    $CUSTOM_CONF_FILE"
        . "$CUSTOM_CONF_FILE"
    fi
elif [[ ! -f "$DEFAULT_CONF_FILE" ]]; then
    echo "- Default configuration file $DEFAULT_CONF_FILE not found"
    exit 1
else
    echo "- Config:    $DEFAULT_CONF_FILE"
    . "$DEFAULT_CONF_FILE"
fi

if [[ ! -d "$REC_DIR" ]]; then
    if mkdir -p "$REC_DIR" 2>/dev/null; then
        echo "- Create storage directory $REC_DIR"
    else
        echo "- Error in create storage directory$REC_DIR"
        exit 1
    fi
fi

echo "- Storage:   $REC_DIR"
echo "- Block:     $BLOCK"
echo "- Services:  $SERVICE_IDS"

# Internally, servicesId are lowercase
SERVICE_IDS=$(echo "$SERVICE_IDS" | tr '[:upper:]' '[:lower:]')
SERVICE_IDS_ARRAY=$(echo "$SERVICE_IDS" | tr "," "\n")
EXTENSIONS=(pcm ndjson)

# Cleanup old instances of read-pipe.sh
if [[ $SIMU -gt 0 ]]; then
    PREVIOUS_PIDS=$(ps | grep read-pipe.sh | grep -v grep | awk '{print $1}';)
    if [[ "$PREVIOUS_PIDS" ]]; then
        echo "Kill (SIGTERM) previous read-pipe.sh process : $PREVIOUS_PIDS"
        kill -15 "$PREVIOUS_PIDS"
    else
        echo "no previous read-pipe.sh instances to kill"
    fi
fi

# Create named pipes for the selected services to record
for SERVICE_ID in $SERVICE_IDS_ARRAY
do
    SERVICE_DIR="${REC_DIR}/${SERVICE_ID}"
    if [[ ! -d "$SERVICE_DIR" ]]; then
        echo "- Create directory $SERVICE_DIR"
        mkdir -p "$SERVICE_DIR"
    fi

    # cleanup .msc files (associated raw data)
    MSC_FILENAME="${FILE_PREFIX}.msc"
    if [[ -f "$MSC_FILENAME" ]]; then
        rm "$MSC_FILENAME"
        echo "Remove msc file $MSC_FILENAME"
    fi

    FILE_PREFIX="${SERVICE_DIR}/${SERVICE_ID}"
    for EXTENSION in "${EXTENSIONS[@]}"
    do
        # do not remove the named pipe everytime ...
        # impossible to arm otherwise
        FILENAME="${FILE_PREFIX}.${EXTENSION}"

        if [[ -f "$FILENAME" ]]; then
             rm "$FILENAME"
             echo "Delete regular file $FILENAME"
        fi

        if [[ ! -p "$FILENAME" ]]; then
            echo "- Create named pipe $FILENAME"
            mkfifo "$FILENAME"
        else
            echo "- Named pipe $FILENAME already exists"
        fi

        # background stream play simulation
        if [[ $SIMU -gt 0 ]]; then
            echo "Simulation of reading the named pipe $FILENAME"
            "${ABS_PATH}/read-pipe.sh" "${FILENAME}" &
        fi
    done
done

if [[ -z "$AUTOSTART" ]] && [[ $SIMU -eq 0 ]]; then
    echo "Did you arm all your recorders for selected services ? (y/N)"
    read -r CONFIRM
    if [[ $CONFIRM != "y" ]] && [[ $CONFIRM != "Y" ]]; then
        echo "Quit"
        exit 0
    fi
else
    echo "Automatic launch"
fi

# cleanup old log file for this block
if [[ -f "${LOG_PATH}/welle-cli-${BLOCK}.log" ]]; then
    echo "- remove logs for block $BLOCK"
    rm "${LOG_PATH}/welle-cli-${BLOCK}.log"
fi

echo "- welle-cli launch"
echo "---"
"$WELLE_CLI_BIN" -c "$BLOCK" -s "$SERVICE_IDS" -o "$REC_DIR" 2>&1
