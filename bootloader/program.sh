cat - | nc localhost 4444 <<EOF
reset halt
flash write_image erase bootloader-rfboard-v3.6.0-48-g2ae51ff.bin
reset run
exit

EOF
