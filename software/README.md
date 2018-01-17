# Firefly2 Software

## Structure

This is the tentative software structure:

```
software                   contains Arduino-specific code
└── generic                contains only vanilla C++ code
    ├── test               contains tests for the generic code
    │   └── CMakeLists.txt CMake file for tests
    └── CMakeLists.txt     CMake build file. Used only for tests, not for Arduino code.
```

*generic* will contain the radio code. This code will not use any Arduino
libraries. It will contain a thin abstraction over the RF69CW radio module that
we're using. The tests for this will live in *test*.

We will only use CMake for testing. It will build the contents of *generic*
so that it can be used by the tests. However, the Arduino code will not use
CMake. It will simply `#include` the needed files in *generic*. This means
that we will be able to use both the Arduino IDE and Makefiles to build and
deploy the Arduino code.
