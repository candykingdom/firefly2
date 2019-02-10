## Bootloader installation

Use an FT232H breakout (from Adafruit) to install the bootloader. Connect pin `D0` to `CLK`, `D1` to `DIO`, and put a 470ohm resistor between pins `D1` and `D2`.

Now, install openocd:

```
sudo apt install openocd
```

Figure out where the cfg file for our proc is and start the openocd server.
```
sudo openocd -f ft232h_swd.cfg -f $(dpkg -L openocd | grep at91samdXX.cfg)
```

Connect to openocd and write the bootloader image
```
telnet localhost 4444
reset halt
flash write_image erase bootloader-firefly_v2-b84c61f-dirty.bin
reset run
```

## Programming flakiness

*Note:* this may no longer be necessary - the Arduino IDE version 1.8.8 uninstalls modem manager when it is installed.

See the comment [here](https://learn.adafruit.com/adafruit-feather-m0-basic-proto/using-with-arduino-ide#ubuntu-and-linux-issue-fix) for why programming can be flaky. To fix, copy the included udev rule file to the udev directory and reload udev. You'll also need to unplug and replug the device.

```sh
sudo cp 99-candy-kingdom.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
sudo udevadm trigger
```

You'll also need to be in the `dialout` group to have permission to open the serial port:

```sh
sudo useradd $USER dialout
```
