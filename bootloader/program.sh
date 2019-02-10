cat - | nc localhost 4444 <<EOF
reset halt
flash write_image erase bootloader-firefly_v2-v2.0.0-adafruit.5-40-g23047ef.bin
reset run
exit

EOF
