#!/usr/bin/env sh
# This script builds and uploads this project to your Arduino device. (Drop into root of an Arduino Sketch)

# NOTE: Board is currently hardset. Arguably it would be nice when the board (port also if set)
# would be read from the project's `sketch.json`-file.
BOARD="arduino:samd:nano_33_iot"
PORT="$(cat arduino-cli-w.tmp 2>/dev/null)"

# Strict mode.
set -euo pipefail

print_help() {
    echo "usage: arduino-cli-w.sh <command>"
    echo ""
    echo "Available Commands:"
    echo "  run\t\tCompiles & Uploads program in working-directory to board."
    echo "  monitor\tOpens a communication port with a board."
}

# Operand
if [[ $# -lt 1 ]]; then
    print_help
    exit 1
fi
operand=$1; shift

if [[ -z "$PORT" || ! -e "$PORT" ]]; then
    PORT="$(arduino-cli board list 2>/dev/null | grep $BOARD | awk '{print $1}')"

    if [[ -z "$PORT" ]]; then
    echo 'Unable to find port.'
    exit 1
    fi

    echo "$PORT" > "arduino-cli-w.tmp"
fi

case "$operand" in
  run)
    arduino-cli compile
    arduino-cli upload -p "$PORT"

    echo "Upload complete."
    ;;

  monitor)
    log_file=$(mktemp)
    echo "Will save terminal-log file at: $log_file"
    read -p "Press enter to continue."

    while true; do
        arduino-cli monitor -p "$PORT" -q | tee -a "$log_file"
    done

    echo "Upload complete."
    ;;

  *)
    print_help
    ;;
esac
