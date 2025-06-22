#!/bin/bash

ENV_FILE="platformio.ini"
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