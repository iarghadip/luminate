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

if [ $# -eq 0 ]; then
    echo
    echo -e "${RED}Error:${RST} No arguments provided." >&2
    echo "Try '$(basename "$0") --help' for usage information." >&2
    echo
    exit 1
fi

function check_ini {
    if [ ! -f "$DIR/../$ENV" ]; then
        echo
        echo "> Created: $ENV"
        cp "$DIR/example.ini" "$DIR/../$ENV"
    fi
}

function check_usb {
    $PIO device list | grep -q "/dev/ttyUSB"
}

function sed_inplace {
    if [[ "$(uname)" == "Darwin" ]]; then
        sed -i '' "$@"
    else
        sed -i "$@"
    fi
}

function add_script_tag {
    if [ "$2" = true ]; then
        add_script_tag "$1" false
        tmp=$(mktemp)
        echo -n "<script>" > "$tmp"
        cat "$1" >> "$tmp"
        echo -n "</script>" >> "$tmp"
        mv "$tmp" "$1"
    else
        sed_inplace 's|<script>||g; s|</script>||g' "$1"
    fi
}

function add_ini_variables {
    local tmp=$(mktemp)
    grep '^[[:space:]]*-D' platformio.ini | while read -r line; do
        if [[ "$line" =~ -D([A-Za-z0-9_]+)=(.+) ]]; then
            key="${BASH_REMATCH[1]}"
            raw="${BASH_REMATCH[2]}"
            raw="${raw//\\\"/}"
            raw="${raw//\"/}"
            raw_escaped=$(printf '%s' "$raw" | sed -e 's/[\/&|]/\\&/g')
            printf "%s\t%s\n" "$key" "$raw_escaped"
        fi
    done > "$tmp.kv"
    cp "$1" "$tmp.out"
    while IFS=$'\t' read -r key value; do
        sed_inplace "s|{$key}|$value|g" "$tmp.out"
    done < "$tmp.kv"
    mv "$tmp.out" "$1"
    rm -f "$tmp.kv"
}

function compile_file {
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
        "$1" -o "$DIR/../data/$(basename "$1")"
    if [[ "$1" == *.js ]]; then
        add_script_tag "$1" false
        add_script_tag "$DIR/../data/$(basename "$1")" false
    fi
    add_ini_variables "$DIR/../data/$(basename "$1")"
    echo "> Compiled: $(basename "$1")"
}

for arg in "$@"; do
    case "$arg" in
        -fc|--filecompile)
            check_ini
            echo
            for item in "$DIR/../web"/*; do
                compile_file "$item" &
            done
            wait
            continue
            ;;
        -fs|--filesystem)
            if [ "$BFS" = false ]; then
                check_ini
                BFS=true
                echo
                $PIO run --target buildfs --environment esp32dev
                if check_usb; then
                    echo
                    $PIO run --target uploadfs --environment esp32dev
                fi
            fi
            continue
            ;;
        -fw|--firmware)
            if [ "$BFW" = false ]; then
                check_ini
                BFW=true
                echo
                $PIO run --environment esp32dev
                if check_usb; then
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