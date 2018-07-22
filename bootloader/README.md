## Bootloader installation

Use an FT232H breakout (from Adafruit) to install the bootloader. Connect pin `D0` to `CLK`, `D1` to `DIO`, and put a 470ohm resistor between pins `D1` and `D2`.

Now, install openocd:

```
sudo apt install openocd
```

Figure out where the cfg file for our proc is and start the openocd server.
```
SAMD_CFG=`dpkg -L openocd | grep at91samdXX.cfg`
sudo openocd -f ft232h_swd.cfg -f $SAMD_CFG
```

Download the bootloader, connect to openocd and write the bootloader image
```
wget https://github.com/mattairtech/ArduinoCore-samd/raw/master/bootloaders/zero/binaries/sam_ba_Generic_D11C14A_SAMD11C14A.bin
telnet localhost 4444
reset halt
flash write_image erase sam_ba_Generic_D11C14A_SAMD11C14A.bin
reset run
```

## Programming using the Arduino IDE

Install the MattairTech Arduino board definitions by adding this URL to the board manager URLs:

```
https://www.mattairtech.com/software/arduino/package_MattairTech_index.json
```

Full documentation on [their Github page](https://github.com/mattairtech/ArduinoCore-samd).

Select the `Generic D11C14A` board. You may need to change the USB config to `USB_DISABLED` (but note that if you don't include CDC, the board may not auto-reset when you program it). Choose theboard in the `Port` menu - it should show up as an ACM device (e.g. `/dev/ACM0`).

The bootloader is a bit flaky. When programming, if you get an error during programming, or it says the serial port is busy, wait a few seconds, then try again. Same thing for the serial monitor - it seems to take 15-20s after resetting for it to work.

## Programming flakiness

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
