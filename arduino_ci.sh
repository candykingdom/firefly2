#!/usr/bin/env bash

set -euo pipefail

cd software/arduino

# Link the existing code into the fake Arduino libraries dir, so that it's used.
mkdir -p Arduino/libraries
ln -s $PWD/../generic Arduino/libraries/firefly_generic || true
ln -s $PWD Arduino/libraries/firefly_arduino || true

CLI_ARCHIVE="arduino-cli_0.5.0_Linux_64bit.tar.gz"
if [ ! -f "$CLI_ARCHIVE" ]; then
  wget https://github.com/arduino/arduino-cli/releases/download/0.5.0/$CLI_ARCHIVE
  if [[ $(sha256sum ./$CLI_ARCHIVE) -ne "f3ec1c5ccb73f6fb5074ac1682d419d6ff4d9f6e32aba7bf664f33d5c6afeb63" ]]; then
    echo "Invalid sha for the arduino-cli archive!"
    exit 1
  fi
fi

tar xf ./arduino-cli_0.5.0_Linux_64bit.tar.gz
CLI="./arduino-cli --config-file arduino-cli.yaml"
$CLI core update-index --additional-urls "https://candykingdom.github.io/firefly-v2-board/package_candykingdom_index.json"
$CLI core install "Candy Kingdom Firefly:samd" --additional-urls "https://candykingdom.github.io/firefly-v2-board/package_candykingdom_index.json"
$CLI lib install "Adafruit seesaw Library"
$CLI compile --fqbn "Candy Kingdom Firefly:samd:rfboard" node/node.ino
$CLI compile --fqbn "Candy Kingdom Firefly:samd:rfboard" trellis/trellis.ino
