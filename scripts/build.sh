#!/bin/bash

ENV_FILE="platformio.ini"
PIO_HOME=~/.platformio/penv/bin/platformio
SCRIPT_DIR=$(dirname "$(readlink -f "$0")")

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
        "$1" -o "$SCRIPT_DIR/../data/$(basename "$1")"
    echo "> Compressed: $(basename $1)"
}

echo

if [ ! -f "$SCRIPT_DIR/../$ENV_FILE" ]; then
    echo "> Created: $ENV_FILE"
    cp "$SCRIPT_DIR/../$ENV_FILE.example" "$SCRIPT_DIR/../$ENV_FILE"
fi

for item in "$SCRIPT_DIR/../web"/*; do
    minify $item &
done

wait

echo

$PIO_HOME run --target buildfs --environment esp32dev

if $PIO_HOME device list | grep -q "/dev/cu.usb"; then
    $PIO_HOME run --target uploadfs --environment esp32dev
fi