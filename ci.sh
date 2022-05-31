#!/usr/bin/env bash

set -euo pipefail

cd software/generic
mkdir build || true
cd build
cmake ..
make && ./generictest --gtest_filter=Perlin*
