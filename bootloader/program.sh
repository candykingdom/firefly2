cat - | nc localhost 4444 <<EOF
reset halt
flash write_image erase bootloader-rfboard-v3.5.0-21-gfb6fecf.bin
reset run
exit

EOF
