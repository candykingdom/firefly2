cat - | nc localhost 4444 <<EOF
reset halt
flash write_image erase bootloader-rfboard-v3.6.0-204-g32f5431.bin
reset run
exit

EOF
