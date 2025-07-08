#!/bin/bash

###############################################################################
# bundle.sh - Minifies and preprocesses HTML, CSS, and JS files for ESP32.
#
# Compresses (minifies) HTML, CSS, and JS files for ESP32 projects.
# Replaces {KEY} placeholders in files with values from platformio.ini.
# Intended for use with PlatformIO-based ESP32 deployments.
#
# Author: iarghadip
# Date:   2025-07-05
# Version: 2.0
###############################################################################

OUT="$2/../data/$(basename "$1")"

###############################################################################
# Prints the human-readable size of a file.
# Arguments:
#   $1 - Path to the file.
# Outputs:
#   File size (e.g., 4.5K) to stdout.
###############################################################################
check_len() {
    ls -lh -- "$1" | awk '{print $5}'
}

###############################################################################
# Performs a cross-platform in-place sed operation.
# Arguments:
#   All arguments are passed to sed.
# Notes:
#   Uses correct in-place syntax for BSD/macOS and GNU/Linux.
###############################################################################
check_sed() {
    if [[ "$(uname)" == "Darwin" ]]; then
        sed -i '' "$@"
    else
        sed -i "$@"
    fi
}

###############################################################################
# Adds or removes <script> tags around JS file content.
# Arguments:
#   $1 - File path.
#   $2 - "true" to add tags, "false" to remove tags.
# Behavior:
#   If $2 is true, wraps file content in <script>...</script>.
#   If $2 is false, removes all <script> and </script> tags.
###############################################################################
check_tag() {
    local file="$1"
    local add="$2"
    if [ "$add" = true ]; then
        check_tag "$file" false
        local tmp
        tmp=$(mktemp)
        { echo -n "<script>"; cat "$file"; echo -n "</script>"; } > "$tmp"
        mv "$tmp" "$file"
    else
        check_sed 's|<script>||g; s|</script>||g' "$file"
    fi
}

###############################################################################
# Replaces {KEY} placeholders with values from platformio.ini.
# Arguments:
#   $1 - File path.
#   $2 - Directory containing platformio.ini.
# Behavior:
#   Finds -DKEY=VALUE definitions in ini and replaces {KEY} in the file.
###############################################################################
check_ini() {
    local file="$1"
    local dir="$2"
    local tmp
    tmp=$(mktemp)
    grep '^[[:space:]]*-D' "$dir/../platformio.ini" | while read -r line; do
        if [[ "$line" =~ -D([A-Za-z0-9_]+)=(.+) ]]; then
            local key="${BASH_REMATCH[1]}"
            local raw="${BASH_REMATCH[2]}"
            raw="${raw//\\\"/}"
            raw="${raw//\"/}"
            local raw_escaped
            raw_escaped=$(printf '%s' "$raw" | sed -e 's/[\/&|]/\\&/g')
            printf "%s\t%s\n" "$key" "$raw_escaped"
        fi
    done > "$tmp.kv"
    cp "$file" "$tmp.out"
    while IFS=$'\t' read -r key value; do
        check_sed "s|{$key}|$value|g" "$tmp.out"
    done < "$tmp.kv"
    mv "$tmp.out" "$file"
    rm -f "$tmp.kv"
}

###############################################################################
# Minifies and preprocesses a web file for ESP32 upload.
# Arguments:
#   $1 - Source file path.
#   $2 - Directory path.
# Behavior:
#   Minifies HTML, CSS, or JS files using html-minifier.
#   For JS, temporarily wraps in <script> tags.
#   After minification, replaces template variables using check_ini.
###############################################################################

if [[ "$1" == *.js ]]; then
    check_tag "$1" true
fi

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
    "$1" -o "$OUT"

if [[ "$1" == *.js ]]; then
    check_tag "$1" false
    check_tag "$OUT" false
fi

check_ini "$OUT" "$2"

echo "> Compiled: $(check_len "$1") → $(check_len "$OUT"): $(basename "$1")"
