#!/bin/bash

###############################################################################
# @file        bundle.sh
# @brief       Minifies and preprocesses HTML, CSS, and JS files for ESP32 web projects.
# @details
#   This script compresses (minifies) HTML, CSS, and JS files for use with ESP32
#   projects. It also performs preprocessing: if environment variables are referenced
#   in the files (e.g., {KEY}), they are replaced with values from platformio.ini.
#   The script is designed for use with PlatformIO-based ESP32 deployments.
#
# @author      iarghadip
# @date        2025-07-05
# @version     2.0
###############################################################################

OUT="$2/../data/$(basename "$1")"

###############################################################################
# @fn          check_size
# @brief       Prints the human-readable size of a file.
# @param[in]   $1  Path to the file whose size should be printed.
# @return      Prints file size in human-readable format (e.g., 4.5K).
###############################################################################
check_size() {
    ls -lh -- "$1" | awk '{print $5}'
}

###############################################################################
# @fn          sed_inplace
# @brief       Performs a cross-platform in-place sed operation.
# @param[in]   $@  Arguments for sed.
# @details
#   Uses the correct in-place syntax for BSD/macOS and GNU/Linux sed.
###############################################################################
sed_inplace() {
    if [[ "$(uname)" == "Darwin" ]]; then
        sed -i '' "$@"
    else
        sed -i "$@"
    fi
}

###############################################################################
# @fn          add_script_tag
# @brief       Adds or removes <script> tags around JS file content.
# @param[in]   $1  File path to process.
# @param[in]   $2  "true" to add tags, "false" to remove tags.
# @details
#   If $2 is true, wraps the file content in <script>...</script>.
#   If $2 is false, removes all <script> and </script> tags from the file.
###############################################################################
add_script_tag() {
    local file="$1"
    local add="$2"
    if [ "$add" = true ]; then
        add_script_tag "$file" false
        local tmp
        tmp=$(mktemp)
        { echo -n "<script>"; cat "$file"; echo -n "</script>"; } > "$tmp"
        mv "$tmp" "$file"
    else
        sed_inplace 's|<script>||g; s|</script>||g' "$file"
    fi
}

###############################################################################
# @fn          add_ini_variables
# @brief       Replaces {KEY} placeholders with values from platformio.ini.
# @param[in]   $1  File path to process.
# @param[in]   $2  Directory path to locate platformio.ini (typically $DIR).
# @details
#   Looks for -DKEY=VALUE definitions in platformio.ini and replaces all
#   {KEY} placeholders in the file with the corresponding VALUE.
###############################################################################
add_ini_variables() {
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
        sed_inplace "s|{$key}|$value|g" "$tmp.out"
    done < "$tmp.kv"
    mv "$tmp.out" "$file"
    rm -f "$tmp.kv"
}

###############################################################################
# @fn          compile_file
# @brief       Minifies and preprocesses a web file for ESP32 upload.
# @param[in]   $1  Source file path.
# @param[in]   $2  Directory path (typically $DIR).
# @details
#   Minifies HTML, CSS, or JS files using html-minifier. For JS files,
#   temporarily wraps in <script> tags for minification. After minification,
#   replaces template variables using add_ini_variables.
###############################################################################

if [[ "$1" == *.js ]]; then
    add_script_tag "$1" true
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
    add_script_tag "$1" false
    add_script_tag "$OUT" false
fi

add_ini_variables "$OUT" "$2"

echo "> Compiled: $(check_size "$1") → $(check_size "$OUT"): $(basename "$1")"
