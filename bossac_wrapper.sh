#!/bin/bash
# This is a wrapper script for bossac that helps debug and fix issues

echo "=== BOSSAC WRAPPER ==="
echo "Arguments: $@"

# Extract port from arguments
PORT=""
for i in "$@"; do
  if [[ $i == /dev/* ]]; then
    PORT=$i
    break
  fi
done

if [ -z "$PORT" ]; then
  # No port found in arguments, try to find it
  PORT=$(ls /dev/cu.usbmodem* 2>/dev/null | head -n 1)
  if [ -z "$PORT" ]; then
    echo "Error: No USB modem port found. Make sure your board is connected."
    exit 1
  fi
  echo "Auto-detected port: $PORT"
fi

# Reset the board
echo "Resetting board with 1200 baud toggle on $PORT"
stty -f "$PORT" 1200
sleep 2

# Find the port again (might have changed)
NEW_PORT=$(ls /dev/cu.usbmodem* 2>/dev/null | head -n 1)
if [ -z "$NEW_PORT" ]; then
  echo "Error: No USB modem port found after reset."
  exit 1
fi

if [ "$PORT" != "$NEW_PORT" ]; then
  echo "Port changed after reset from $PORT to $NEW_PORT"
  PORT=$NEW_PORT
fi

# Get the firmware file
FIRMWARE=""
for i in "$@"; do
  if [[ $i == *.bin ]]; then
    FIRMWARE=$i
    break
  fi
done

if [ -z "$FIRMWARE" ]; then
  echo "Error: No firmware file found in arguments."
  exit 1
fi

echo "Using port: $PORT"
echo "Firmware: $FIRMWARE"
echo "Uploading..."

# Run bossac with corrected arguments
~/.local/bin/bossac -e -w -v -b -R -p "$PORT" "$FIRMWARE"
RESULT=$?

echo "Upload process finished with code $RESULT"
exit $RESULT