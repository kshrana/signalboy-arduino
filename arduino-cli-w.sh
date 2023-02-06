#!/usr/bin/env bash
# This script builds and uploads this project to your Arduino device. (Drop this script into root of an Arduino Sketch)

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

query_port() {
  PORT="$(arduino-cli board list 2>/dev/null | grep $BOARD | awk '{print $1}')"

  if [[ -z "$PORT" ]]; then
    echo "Unable to find port for BOARD ($BOARD)."
    exit 1
  fi

  echo "$PORT" >"arduino-cli-w.tmp"
}

# Extract operand.
if [[ $# -lt 1 ]]; then
    print_help
    exit 1
fi
operand=$1; shift

if [[ -z "$PORT" ]]; then
  echo "PORT ($PORT) is empty or device is non-existant. Will search for a port for BOARD ($BOARD)."
  read -p "Press ENTER to continue..."
  
  query_port
fi

if [[ ! -e "$PORT" ]]; then
  is_port_existant=false

  for i in {1..2}; do
    sleep 2
    if [[ -e "$PORT" ]]; then
      is_port_existant=true
      break
    fi
  done

  if [[ "$is_port_existant" = false ]]; then
    echo "Device at PORT ($PORT) not-existant. Will search for a port for BOARD ($BOARD)."
    read -p "Press ENTER to continue..."
    
    query_port
  fi
fi

case "$operand" in
  run)
    arduino-cli compile
    echo "Compilation succeeded. Will upload to board at PORT: $PORT"
    arduino-cli upload -p "$PORT"

    echo "Upload complete."
    ;;

  monitor)
    log_file=$(mktemp)
    echo "Will save terminal-log file at (PORT=$PORT): $log_file"
    read -p "Press ENTER to continue..."

    while true; do
        arduino-cli monitor -p "$PORT" -q | tee -a "$log_file"
    done

    echo "Upload complete."
    ;;

  *)
    print_help
    ;;
esac
