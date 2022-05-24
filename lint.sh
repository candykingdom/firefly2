#!/usr/bin/env bash

set -euo pipefail

if [ $# -ne 1 ]
  then
    echo "Usage: $0 check|format"
    exit 1
elif [ $1 = "check" ]; then
    find software -not -path software/generic/build -iname *.h -o -iname *.cpp | xargs clang-format --dry-run --Werror
elif [ $1 = "format" ]; then
    find software -not -path software/generic/build -iname *.h -o -iname *.cpp | xargs clang-format -i
else
    echo "Usage: $0 check|format"
    exit 1
fi
