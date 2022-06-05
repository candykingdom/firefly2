# Firefly 2

## Working on this library

To do development work on this library, first clone this repo somewhere. Then, set up symbolic links in the Arduino libraries directory for the `generic` and `arduino` libraries. The names are important (they must the names used in the board definition).

```bash
# set me
FIREFLY2_DIR=$PWD

pushd ~/Arduino/libraries
ln -s $FIREFLY2_DIR/src/generic firefly_generic
ln -s $FIREFLY2_DIR/src/arduino firefly_arduino
```
