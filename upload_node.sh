#!/bin/bash
set -e

# Look for firmware binaries
if [ -f ".pio/build/node/firmware.bin" ]; then
  echo "Using node firmware build"
  FIRMWARE=".pio/build/node/firmware.bin"
elif [ -f ".pio/build/node-macos/firmware.bin" ]; then
  echo "Using node-macos firmware build"
  FIRMWARE=".pio/build/node-macos/firmware.bin"
else
  echo "Firmware not found. Please run 'Build' in VS Code first."
  echo "Then run this script again."
  exit 1
fi

# Find the upload port
PORT=$(ls /dev/cu.usbmodem* | head -n 1)
if [ -z "$PORT" ]; then
  echo "Error: No USB modem port found. Make sure your board is connected."
  exit 1
fi
echo "Found device at: $PORT"

# Reset the board by toggling DTR
echo "Forcing reset using 1200bps open/close on port $PORT"
stty -f $PORT 1200
sleep 2

# Find the upload port again (might change after reset)
PORT=$(ls /dev/cu.usbmodem* | head -n 1)
if [ -z "$PORT" ]; then
  echo "Error: No USB modem port found after reset. Make sure your board is connected."
  exit 1
fi
echo "Using port for upload: $PORT"

# Upload using bossac
echo "Uploading firmware..."
# Parameters based on help output
~/.local/bin/bossac -p "$PORT" -e -w -v -b -R "$FIRMWARE"

echo "Upload complete!"