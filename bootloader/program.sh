cat - | nc localhost 4444 <<EOF
reset halt
flash write_image erase /home/adam/hardware/firefly2/software/start/gcc/AtmelStart.bin
reset run
exit

EOF
