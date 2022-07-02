#!/usr/bin/env bash

set -euo pipefail

cd src/generic
mkdir build || true
cd build
cmake ..
make
./smalltests
./largetests
