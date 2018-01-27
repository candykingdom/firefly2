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

## Hardware-specific notes:

When using the MattairTech Arduino core, the pin numbers aren't the physical
pin numbers. They are the PortA number (e.g. physical pin 1 is PA05, so is
Arduino pin 5).

## START code

start_config is a configuration file for [Atmel SMART](http://start.atmel.com). This is a code generation utility where you configure the peripherals, drivers, and middleware, and it generates initialization code. This code supports Makefiles (hurray!). Included are two exports from this tool - `start` and `start_nousb`. `start` has USB CDC included - it's quite large (the binary size is ~12kB). The `start_nousb` is the same, but with no USB support.
