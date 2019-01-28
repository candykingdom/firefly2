#!/bin/bash

# Formats all of the source files using clang-format.
clang-format -i -style=Google $(find . -name *.[ch]pp)
clang-format -i -style=Google $(find . -name *.ino)
