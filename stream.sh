#!/usr/bin/env bash

##
# DAB+ services multiplex stream with welle-cli
#
# Active blocks in Paris: 5A, 6A, 6C, 6D, 8C, 9A, 9B, 11A, 11B
##

declare -r ABS_PATH="$( cd "$(dirname "$0")" || return; pwd -P )"
declare -r LOG_PATH="$ABS_PATH/log"
declare -r WELLE_CLI_BIN="/usr/local/bin/welle-cli"
declare -r BLOCK="$1"

COMMON_FILE="$ABS_PATH/conf/common.ini"
BLOCK_FILE="$ABS_PATH/conf/$BLOCK.ini"

if [[ ! -f "$COMMON_FILE" ]]; then
    echo "- common file $COMMON_FILE not found"
    exit 1
else
    echo "- loaded common file: $COMMON_FILE"
    . "$COMMON_FILE"
fi

if [[ ! $BLOCK ]]; then
    echo "- block not set"
    exit 1
fi

if [[ ! -f "$BLOCK_FILE" ]]; then
    echo "- block file $BLOCK_FILE not found"
    exit 1
else
    echo "- loaded block file: $BLOCK_FILE"
    . "$BLOCK_FILE"
fi

echo "- block: $BLOCK"
echo "- services: $SERVICES"

# Internally, servicesId are lowercase
SERVICES=$(echo "$SERVICES" | tr '[:upper:]' '[:lower:]')
# join array elements to string
SERVICES_STR=$(IFS=, ; echo "${SERVICES[*]}")

# cleanup old log file for this block
if [[ -f "${LOG_PATH}/welle-cli-${BLOCK}.log" ]]; then
    echo "- remove logs for block $BLOCK"
    true > "${LOG_PATH}/welle-cli-${BLOCK}.log"
fi

echo "- welle-cli launch"
echo "---"
echo "$WELLE_CLI_BIN" -c "$BLOCK" -s "$SERVICES_STR" -o "$REC_DIR" 2>&1
"$WELLE_CLI_BIN" -c "$BLOCK" -s "$SERVICES_STR" -o "$REC_DIR" 2>&1
