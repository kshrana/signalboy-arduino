# This script builds and uploads this project to your Arduino device. (Drop into root of an Arduino Sketch)

# NOTE: Board is currently hardset. Arguably it would be nice when the board (port also if set)
# would be read from the project's `sketch.json`-file.
BOARD="arduino:samd:nano_33_iot"
PORT="$(cat run_cache.tmp 2>/dev/null)"

# Exit when any command fails.
set -e

if [[ -z "$PORT" || ! -e "$PORT" ]]; then
    PORT="$(arduino-cli board list 2>/dev/null | grep $BOARD | awk '{print $1}')"

    if [[ -z "$PORT" ]]; then
    echo 'Unable to find port.'
    exit
    fi

    echo "$PORT" > "run_cache.tmp"
fi

arduino-cli compile
arduino-cli upload -p "$PORT"

echo "Upload complete."
