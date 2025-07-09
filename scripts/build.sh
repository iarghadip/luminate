#!/bin/bash

###############################################################################
# build.sh - PlatformIO build and deployment helper script for ESP32 projects.
#
# Automates file compilation, filesystem image creation, firmware build, and
# upload tasks for PlatformIO-based ESP32 projects. Supports argument-based
# task selection and variable substitution from platformio.ini.
#
# Author: iarghadip
# Date:   2025-07-08
# Version: 2.0
###############################################################################

RED='\033[0;31m'
GRN='\033[0;32m'
YEL='\033[1;33m'
BLU='\033[0;34m'
BLD='\033[1m'
RST='\033[0m'

declare -A EXE
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

###############################################################################
# Prints an error message and exits.
# Arguments:
#   $1 - Error message (brief).
#   $2 - Additional suggestion or info.
###############################################################################
check_out() {
    echo
    echo -e "${RED}Error:${RST} $1" >&2
    echo "$2" >&2
    echo
    exit 1
}

###############################################################################
# Ensures platformio.ini exists; copies example if missing.
###############################################################################
check_ini() {
    local ini_path="$DIR/../platformio.ini"
    if [ ! -f "$ini_path" ]; then
        echo
        echo "> Created: platformio.ini"
        cp "$DIR/example.ini" "$ini_path"
    fi
}

###############################################################################
# Checks if a /dev/ttyUSB device is present.
# Returns:
#   0 if found, 1 otherwise.
###############################################################################
check_usb() {
    platformio device list | grep -q "/dev/ttyUSB"
}

###############################################################################
# Main entry point: Validates input and parses arguments.
###############################################################################

if [ ! -t 0 ]; then
    check_out "This script does not accept piped input." \
        "Try '$(basename "$0") --help' for usage information."
fi

if [ $# -eq 0 ]; then
    check_out "No arguments provided." \
        "Try '$(basename "$0") --help' for usage information."
fi

###############################################################################
# Parses and executes command-line arguments.
###############################################################################
for arg in "$@"; do
    case "$arg" in
        -fc|--filecompile)
            if [[ -z "${EXE[0]}" ]]; then
                EXE[0]=true
                check_ini
                echo
                for item in "$DIR/../interface"/*; do
                    "$DIR/bundle.sh" "$item" "$DIR" &
                done
                wait
            fi
            ;;
        -fs|--filesystem)
            if [[ -z "${EXE[1]}" ]]; then
                EXE[1]=true
                check_ini
                echo
                platformio run --target buildfs --environment esp32dev
                if check_usb; then
                    echo
                    platformio run --target uploadfs --environment esp32dev
                fi
            fi
            ;;
        -fw|--firmware)
            if [[ -z "${EXE[2]}" ]]; then
                EXE[2]=true
                check_ini
                echo
                platformio run --environment esp32dev
                if check_usb; then
                    echo
                    platformio run --target upload --environment esp32dev
                fi
            fi
            ;;
        -a|--all)
            if [[ -z "${EXE[3]}" ]]; then
                EXE[3]=true
                "$DIR/$(basename "$0")" -fc -fs -fw
                if check_usb; then
                    echo
                    platformio run --target uploadfs --environment esp32dev
                fi
            fi
            ;;
        -d|--debug)
            if [[ -z "${EXE[4]}" ]]; then
                EXE[4]=true
                if check_usb; then
                    echo
                    platformio device monitor
                    exit 0
                else
                    check_out "No devices detected." \
                        "Please connect a device and try again."
                fi
            fi
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
            check_out "Unknown option: $arg" \
                "Try '$(basename "$0") --help' for more information."
            ;;
    esac
done

echo
