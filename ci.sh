#!/usr/bin/env bash

set -euo pipefail

mkdir -p build
cd build
cmake ..
make
./smalltests
./largetests

