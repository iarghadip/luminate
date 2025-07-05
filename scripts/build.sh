#!/bin/bash

###############################################################################
# @file        build.sh
# @brief       PlatformIO build and deployment helper script for ESP32 projects.
# @details
#   Automates file compilation, filesystem image creation, firmware build, and
#   upload tasks for PlatformIO-based ESP32 projects. Supports argument-based
#   task selection and variable substitution from platformio.ini.
#
# @author      iarghadip
# @date        2025-07-05
# @version     2.0
###############################################################################

RED='\033[0;31m'
GRN='\033[0;32m'
YEL='\033[1;33m'
BLU='\033[0;34m'
BLD='\033[1m'
RST='\033[0m'

BAL=false
BFC=false
BFS=false
BFW=false

ENV="platformio.ini"
PIO="$HOME/.platformio/penv/bin/platformio"
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

###############################################################################
# @fn          main
# @brief       Main entry point: Validates input and parses arguments.
###############################################################################

if [ ! -t 0 ]; then
    echo
    echo -e "${RED}Error:${RST} This script does not accept piped input." >&2
    echo "Try '$(basename "$0") --help' for usage information." >&2
    echo
    exit 1
fi

if [ $# -eq 0 ]; then
    echo
    echo -e "${RED}Error:${RST} No arguments provided." >&2
    echo "Try '$(basename "$0") --help' for usage information." >&2
    echo
    exit 1
fi

###############################################################################
# @fn          check_ini
# @brief       Ensures platformio.ini exists; copies example if missing.
###############################################################################
check_ini() {
    local ini_path="$DIR/../$ENV"
    if [ ! -f "$ini_path" ]; then
        echo
        echo "> Created: $ENV"
        cp "$DIR/example.ini" "$ini_path"
    fi
}

###############################################################################
# @fn          check_usb
# @brief       Checks if a /dev/ttyUSB device is present.
# @retval      0 if found, 1 otherwise.
###############################################################################
check_usb() {
    "$PIO" device list | grep -q "/dev/ttyUSB"
}

###############################################################################
# @fn          check_size
# @brief       Prints the human-readable size of a file.
# @param[in]   $1  File path.
###############################################################################
check_size() {
    ls -lh -- "$1" | awk '{print $5}'
}

###############################################################################
# @fn          sed_inplace
# @brief       Cross-platform in-place sed.
# @param[in]   $@  sed arguments.
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
# @brief       Wraps/unwraps JS files in <script> tags for minification.
# @param[in]   $1  File path.
# @param[in]   $2  true to add, false to remove tags.
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
# @brief       Replaces {KEY} in file with -DKEY=VALUE from platformio.ini.
# @param[in]   $1  File path.
###############################################################################
add_ini_variables() {
    local file="$1"
    local tmp
    tmp=$(mktemp)
    grep '^[[:space:]]*-D' "$DIR/../platformio.ini" | while read -r line; do
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
# @brief       Minifies and processes a web file for upload.
# @param[in]   $1  Source file path.
###############################################################################
compile_file() {
    local src="$1"
    local output="$DIR/../data/$(basename "$src")"
    if [[ "$src" == *.js ]]; then
        add_script_tag "$src" true
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
        "$src" -o "$output"
    if [[ "$src" == *.js ]]; then
        add_script_tag "$src" false
        add_script_tag "$output" false
    fi
    add_ini_variables "$output"
    echo "> Compiled: $(check_size "$src") → $(check_size "$output"): $(basename "$src")"
}

###############################################################################
# @fn          Argument Parsing
# @brief       Parses and executes command-line arguments.
###############################################################################
for arg in "$@"; do
    case "$arg" in
        -fc|--filecompile)
            if [ "$BFC" = false ]; then
                BFC=true
                check_ini
                echo
                for item in "$DIR/../web"/*; do
                    compile_file "$item" &
                done
                wait
            fi
            ;;
        -fs|--filesystem)
            if [ "$BFS" = false ]; then
                BFS=true
                check_ini
                echo
                "$PIO" run --target buildfs --environment esp32dev
                if check_usb; then
                    echo
                    "$PIO" run --target uploadfs --environment esp32dev
                fi
            fi
            ;;
        -fw|--firmware)
            if [ "$BFW" = false ]; then
                BFW=true
                check_ini
                echo
                "$PIO" run --environment esp32dev
                if check_usb; then
                    echo
                    "$PIO" run --target upload --environment esp32dev
                fi
            fi
            ;;
        -a|--all)
            if [ "$BAL" = false ]; then
                BAL=true
                "$DIR/$(basename "$0")" -fc -fs -fw
            fi
            exit 0
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
