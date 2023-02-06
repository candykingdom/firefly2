# Firefly2

## Programming

The `fancy-node` device can be programmed either using an STLink, or via USB port. To program via USB, you must first install [stm32loader] using pip (`pip install stm32loader`). You may also need to pass the port (typically `/dev/ttyUSB0`), e.g. `pio run -e fancy-node-usb -t upload --upload-port /dev/ttyUSB0`.
