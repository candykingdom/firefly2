#!/usr/bin/env bash

set -euo pipefail

cd software/generic
mkdir build
cd build
cmake ..
make && ./generictest
