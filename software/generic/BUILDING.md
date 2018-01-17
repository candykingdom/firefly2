# Building

- First, install cmake:
  ```
  sudo apt install cmake
  ```

- Build and install GTest from https://github.com/google/googletest

- Build **firefly2** using CMake:
  ```
  mkdir build && cd build
  cmake ..
  make
  ```

- Run the tests:
  `make && make test` or `make && ./generictest`
