#!/usr/bin/env bash

##
# Stream a DAB+ ensemble in multicast on the network with welle-cli
#
# Active blocks in Paris: 5A, 6A, 6C, 6D, 8C, 9A, 9B, 11A, 11B
# Active blocks in Trouville: 5B, 10A, 10D
##

declare -r ABS_PATH="$( cd "$(dirname "$0")" || return; pwd -P )"
declare -r WELLE_CLI_BIN="/usr/local/bin/welle-cli"
declare -r BLOCK="$1"

if [[ ! $BLOCK ]]; then
    echo "Usage: ./stream.sh BLOCK"
    exit 1
fi

COMMON_FILE="$ABS_PATH/conf/common.ini"
BLOCK_FILE="$ABS_PATH/conf/$BLOCK.ini"

if [[ ! -f "$COMMON_FILE" ]]; then
    echo "- file $COMMON_FILE not found"
    exit 1
else
    echo "- loaded file: $COMMON_FILE"
    . "$COMMON_FILE"
fi

if [[ ! -f "$BLOCK_FILE" ]]; then
    echo "- file $BLOCK_FILE not found"
    exit 1
else
    echo "- loaded file: $BLOCK_FILE"
    . "$BLOCK_FILE"
fi

# join array elements to string
SERVICES_STR=$(IFS=, ; echo "${SERVICES[*]}")

echo "- block: $BLOCK"
echo "- services:multicast_ips: $SERVICES_STR"
echo "- rec dir: $REC_DIR"
echo "- cmd: $WELLE_CLI_BIN" -c "$BLOCK" -s "$SERVICES_STR" -o "$REC_DIR" 2>&1

"$WELLE_CLI_BIN" -c "$BLOCK" -s "$SERVICES_STR" -o "$REC_DIR" 2>&1
