#!/bin/bash

RED='\033[0;31m'
GRN='\033[0;32m'
YEL='\033[1;33m'
BLU='\033[0;34m'
BLD='\033[1m'
RST='\033[0m'

BFS=false
BFW=false

ENV="platformio.ini"
PIO=~/.platformio/penv/bin/platformio
DIR=$(dirname "$(readlink -f "$0")")

if [ ! -t 0 ]; then
    echo
    echo -e "${RED}Error:${RST} This script does not accept piped input." >&2
    echo "Try '$(basename "$0") --help' for usage information." >&2
    echo
    exit 1
fi

function connected {
    $PIO device list | grep -q "/dev/ttyUSB"
}

function minify {
    html-minifier \
        --collapse-whitespace \
        --remove-comments \
        --remove-optional-tags \
        --remove-redundant-attributes \
        --remove-script-type-attributes \
        --remove-tag-whitespace \
        --use-short-doctype \
        --minify-css true \
        --minify-js true \
        "$1" -o "$DIR/../data/$(basename "$1")"
    echo "> Compressed: $(basename "$1")"
}

echo

if [ ! -f "$DIR/../$ENV" ]; then
    echo "> Created: $ENV"
    cp "$DIR/../$ENV.example" "$DIR/../$ENV"
fi

for item in "$DIR/../web"/*; do
    minify "$item" &
done

wait

for arg in "$@"; do
    case "$arg" in
        -fs|--filesystem)
            if [ "$BFS" = false ]; then
                BFS=true
                echo
                $PIO run --target buildfs --environment esp32dev
                if connected; then
                    echo
                    $PIO run --target uploadfs --environment esp32dev
                fi
            fi
            continue
            ;;
        -fw|--firmware)
            if [ "$BFW" = false ]; then
                BFW=true
                echo
                $PIO run --environment esp32dev
                if connected; then
                    echo
                    $PIO run --target upload --environment esp32dev
                fi
            fi
            continue
            ;;
        -h|--help)
            echo
            awk -v BLD="$BLD" -v BLU="$BLU" -v YEL="$YEL" -v RST="$RST" '
                /^Usage:/      {print BLD BLU $0 RST; next}
                /^Options:/    {print BLD BLU $0 RST; next}
                /^Description:/{print BLD BLU $0 RST; next}
                /^Examples:/   {print BLD BLU $0 RST; next}
                /^[[:space:]]+-[a-z]/ {sub(/^([[:space:]]+-[a-z, ]+)/, YEL "&" RST); print; next}
                {print}
            ' "$DIR/help.txt"
            echo
            exit 0
            ;;
        *)
            echo
            echo -e "${RED}Error:${RST} Unknown option: $arg" >&2
            echo "Try '$(basename "$0") --help' for more information." >&2
            echo
            exit 1
            ;;
    esac
done

echo
