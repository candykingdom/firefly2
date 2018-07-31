#flash write_image erase /home/adam/hardware/uf2-samdx1/build/firefly_v2/bootloader-firefly_v2-v2.0.0-adafruit.5-1-gb84c61f-dirty.bin
cat - | nc localhost 4444 <<EOF
reset halt
flash write_image erase /home/adam/hardware/uf2-samdx1/build/firefly_v2/bootloader-firefly_v2-b84c61f-dirty.bin
reset run
exit

EOF
