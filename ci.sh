#!/usr/bin/env bash

set -euo pipefail

mkdir -p build
cd build
cmake .. -DBUILD_SIMULATOR=false
make
./smalltests
./largetests

