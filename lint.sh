#!/usr/bin/env bash

set -euo pipefail

if [ $# -ne 1 ]; then
    echo "Usage: $0 check|format"
    exit 1
fi

FILES=$(find \
lib \
software \
-not -path software/generic/build \
-iname *.hpp \
-o -iname *.cpp \
-o -iname *.ino \
)

if [ $1 = "check" ]; then
    clang-format --dry-run --Werror $FILES
elif [ $1 = "format" ]; then
    clang-format -i $FILES
else
    echo "Usage: $0 check|format"
    exit 1
fi
