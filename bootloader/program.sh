cat - | nc localhost 4444 <<EOF
reset halt
flash write_image erase bootloader-rf_remote-v2.0.0-adafruit.5-44-g2a8d054.bin
reset run
exit

EOF
