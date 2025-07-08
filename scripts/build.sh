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

BDE=false
BAL=false
BFC=false
BFS=false
BFW=false

ENV="platformio.ini"
PIO="platformio"
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
                    "$DIR/bundle.sh" "$item" "$DIR" &
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
            exit 0
            ;;
        -a|--all)
            if [ "$BAL" = false ]; then
                BAL=true
                "$DIR/$(basename "$0")" -fc -fs -fw
                if check_usb; then
                    echo
                    "$PIO" run --target uploadfs --environment esp32dev
                fi
            fi
            ;;
        -d|--debug)
            if [ "$BDE" = false ]; then
                BDE=true
                if check_usb; then
                    echo
                    "$PIO" device monitor
                else
                    echo
                    echo -e "${RED}Error:${RST} No devices found." >&2
                    echo "Please connect a device and try again." >&2
                    echo
                    exit 1 
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
            echo
            echo -e "${RED}Error:${RST} Unknown option: $arg" >&2
            echo "Try '$(basename "$0") --help' for more information." >&2
            echo
            exit 1
            ;;
    esac
done

echo
