EESchema Schematic File Version 2
LIBS:rfboard-rescue
LIBS:power
LIBS:device
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
LIBS:homebrew
LIBS:SparkFun-Aesthetics
LIBS:SparkFun-Batteries
LIBS:SparkFun-Boards
LIBS:SparkFun-Capacitors
LIBS:SparkFun-Clocks
LIBS:SparkFun-Coils
LIBS:SparkFun-Connectors
LIBS:SparkFun-DiscreteSemi
LIBS:SparkFun-Displays
LIBS:SparkFun-Electromechanical
LIBS:SparkFun-Fuses
LIBS:SparkFun-GPS
LIBS:SparkFun-Hardware
LIBS:SparkFun-IC-Amplifiers
LIBS:SparkFun-IC-Comms
LIBS:SparkFun-IC-Conversion
LIBS:SparkFun-IC-Logic
LIBS:SparkFun-IC-Memory
LIBS:SparkFun-IC-Microcontroller
LIBS:SparkFun-IC-Power
LIBS:SparkFun-IC-Special-Function
LIBS:SparkFun-Jumpers
LIBS:SparkFun-LED
LIBS:SparkFun-PowerSymbols
LIBS:SparkFun-Resistors
LIBS:SparkFun-RF
LIBS:SparkFun-Sensors
LIBS:SparkFun-Switches
LIBS:10118194-0001LF
LIBS:MIC5504-3.3YM5-T5
LIBS:homebrew_button
LIBS:rf_remote-cache
EELAYER 25 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
Text Notes 7350 7500 0    60   ~ 0
Firefly V2
Text Notes 8150 7650 0    60   ~ 0
2018-01-08
Text Notes 10550 7650 0    60   ~ 0
1	
$Comp
L Conn_01x04 P1
U 1 1 5A5423D1
P 3750 7350
F 0 "P1" H 3750 7600 50  0000 C CNN
F 1 "CONN_01X04" V 3850 7350 50  0000 C CNN
F 2 "SF_Connectors:1X04" H 3750 7350 50  0001 C CNN
F 3 "" H 3750 7350 50  0000 C CNN
	1    3750 7350
	1    0    0    -1  
$EndComp
Text Notes 3150 6750 0    60   ~ 0
ARM\nSerial Wire Debug
$Comp
L GND #PWR01
U 1 1 5A542459
P 3550 7550
F 0 "#PWR01" H 3550 7300 50  0001 C CNN
F 1 "GND" H 3550 7400 50  0000 C CNN
F 2 "" H 3550 7550 50  0000 C CNN
F 3 "" H 3550 7550 50  0000 C CNN
	1    3550 7550
	1    0    0    -1  
$EndComp
Text GLabel 3400 7350 1    60   Input ~ 0
ARM_SWDIO
Text GLabel 3250 7450 1    60   Input ~ 0
ARM_SWCLK
$Comp
L GND #PWR02
U 1 1 5A542675
P 5400 1900
F 0 "#PWR02" H 5400 1650 50  0001 C CNN
F 1 "GND" H 5400 1750 50  0000 C CNN
F 2 "" H 5400 1900 50  0000 C CNN
F 3 "" H 5400 1900 50  0000 C CNN
	1    5400 1900
	0    -1   -1   0   
$EndComp
$Comp
L GND #PWR03
U 1 1 5A542695
P 5400 2100
F 0 "#PWR03" H 5400 1850 50  0001 C CNN
F 1 "GND" H 5400 1950 50  0000 C CNN
F 2 "" H 5400 2100 50  0000 C CNN
F 3 "" H 5400 2100 50  0000 C CNN
	1    5400 2100
	1    0    0    -1  
$EndComp
$Comp
L Conn_01x01 P2
U 1 1 5A542715
P 5000 7400
F 0 "P2" H 5000 7500 50  0000 C CNN
F 1 "CONN_01X01" V 5100 7400 50  0000 C CNN
F 2 "SF_Connectors:1X01" H 5000 7400 50  0001 C CNN
F 3 "" H 5000 7400 50  0000 C CNN
	1    5000 7400
	1    0    0    -1  
$EndComp
Text Notes 4750 6750 0    60   ~ 0
Antenna
Text GLabel 4800 7400 1    60   Input ~ 0
ANTENNA
Text GLabel 5600 2000 2    60   Input ~ 0
ANTENNA
Text GLabel 5400 1300 2    60   Input ~ 0
RADIO_DIO0
Text GLabel 4400 1500 0    60   Input ~ 0
RADIO_MISO
Text GLabel 4400 1600 0    60   Input ~ 0
RADIO_MOSI
Text GLabel 4400 1700 0    60   Input ~ 0
RADIO_SCK
Text GLabel 4400 1800 0    60   Input ~ 0
RADIO_NSS
$Comp
L +3V3 #PWR04
U 1 1 5A54A758
P 4400 1300
F 0 "#PWR04" H 4400 1150 50  0001 C CNN
F 1 "+3V3" H 4400 1440 50  0000 C CNN
F 2 "" H 4400 1300 50  0000 C CNN
F 3 "" H 4400 1300 50  0000 C CNN
	1    4400 1300
	1    0    0    -1  
$EndComp
$Comp
L RFM69CW-915MHZ U2
U 1 1 5A83E74A
P 4900 1700
F 0 "U2" H 4850 2500 45  0000 L BNN
F 1 "RFM69CW-915MHZ" H 4500 1100 45  0000 L BNN
F 2 "homebrew:RFM69CW-XXXS2_" H 4900 2300 20  0001 C CNN
F 3 "" H 4900 1700 60  0001 C CNN
F 4 "IC-11996" H 4900 2350 60  0000 C CNN "Field4"
	1    4900 1700
	1    0    0    -1  
$EndComp
NoConn ~ 5400 1400
NoConn ~ 5400 1500
NoConn ~ 5400 1600
NoConn ~ 5400 1700
NoConn ~ 4400 2100
$Comp
L SAMD21E18 U1
U 1 1 5A8519D5
P 1750 2150
F 0 "U1" H 1650 700 60  0000 C CNN
F 1 "SAMD21E18" H 1650 3400 60  0000 C CNN
F 2 "SF_Silicon-Standard:TQFP32-08" H 1750 2150 60  0001 C CNN
F 3 "" H 1750 2150 60  0001 C CNN
	1    1750 2150
	1    0    0    -1  
$EndComp
$Comp
L +3V3 #PWR05
U 1 1 5A851BC3
P 1100 1000
F 0 "#PWR05" H 1100 850 50  0001 C CNN
F 1 "+3V3" H 1100 1140 50  0000 C CNN
F 2 "" H 1100 1000 50  0001 C CNN
F 3 "" H 1100 1000 50  0001 C CNN
	1    1100 1000
	1    0    0    -1  
$EndComp
Wire Wire Line
	3400 7350 3550 7350
Wire Wire Line
	3550 7450 3250 7450
Wire Wire Line
	5600 2000 5400 2000
Wire Wire Line
	1100 1100 1100 1000
$Comp
L GND #PWR06
U 1 1 5A851C44
P 1100 1800
F 0 "#PWR06" H 1100 1550 50  0001 C CNN
F 1 "GND" H 1100 1650 50  0000 C CNN
F 2 "" H 1100 1800 50  0001 C CNN
F 3 "" H 1100 1800 50  0001 C CNN
	1    1100 1800
	1    0    0    -1  
$EndComp
Wire Wire Line
	1100 1800 1100 1700
Text GLabel 2200 1800 2    60   Input ~ 0
RADIO_MOSI
Text GLabel 2200 1900 2    60   Input ~ 0
RADIO_SCK
Text GLabel 2200 2000 2    60   Input ~ 0
RADIO_MISO
Text GLabel 2200 2100 2    60   Input ~ 0
RADIO_NSS
Text GLabel 2200 3200 2    60   Input ~ 0
SIMPLE_LED
$Comp
L LED D1
U 1 1 5A8531E4
P 7450 1900
F 0 "D1" H 7450 2000 50  0000 C CNN
F 1 "LED" H 7450 1800 50  0000 C CNN
F 2 "LEDs:LED_0603" H 7450 1900 50  0001 C CNN
F 3 "" H 7450 1900 50  0001 C CNN
	1    7450 1900
	-1   0    0    1   
$EndComp
Text GLabel 7000 1900 0    60   Input ~ 0
SIMPLE_LED
$Comp
L GND #PWR07
U 1 1 5A853737
P 7600 1900
F 0 "#PWR07" H 7600 1650 50  0001 C CNN
F 1 "GND" H 7600 1750 50  0000 C CNN
F 2 "" H 7600 1900 50  0000 C CNN
F 3 "" H 7600 1900 50  0000 C CNN
	1    7600 1900
	1    0    0    -1  
$EndComp
$Comp
L R R1
U 1 1 5A8538CF
P 7150 1900
F 0 "R1" V 7230 1900 50  0000 C CNN
F 1 "1k" V 7150 1900 50  0000 C CNN
F 2 "Resistors_SMD:R_0603_HandSoldering" V 7080 1900 50  0001 C CNN
F 3 "" H 7150 1900 50  0001 C CNN
	1    7150 1900
	0    1    1    0   
$EndComp
Text GLabel 2200 3300 2    60   Input ~ 0
RADIO_DIO0
Text GLabel 2200 3500 2    60   Input ~ 0
ARM_SWDIO
Text GLabel 2200 3400 2    60   Input ~ 0
ARM_SWCLK
NoConn ~ 1100 1500
NoConn ~ 1100 1300
Text Notes 1250 4400 0    60   ~ 0
You can probably ditch the WS2812 that's on there, and of course the\n headers. A wire antenna should work - we can experiment with coiling it\n inside the enclosure (the Moteino folks use the same radio and have a\n variant with a PCB trace antenna - you could go for that too. RF design is a\n little trickier, but I have a friend I can ask about doing the calculations for\n that if you'd like).\n
Text Notes 1250 5200 0    60   ~ 0
processor, radio, and a couple of tactile switches.\n\nUsing this as footprint for battery holder. Almost the same as the one recommended by polycase.\nhttp://www.keyelco.com/product.cfm/product_id/798\nhttps://www.polycase.com/batt-hdr-fb?file=2d
Text GLabel 2200 2700 2    60   Input ~ 0
SW1
Text GLabel 9900 2100 2    60   Input ~ 0
SW1
Text GLabel 2200 2600 2    60   Input ~ 0
SW2
Text GLabel 2200 2500 2    60   Input ~ 0
SW3
Text GLabel 2200 2400 2    60   Input ~ 0
SW4
Text GLabel 9900 3150 2    60   Input ~ 0
SW2
Text GLabel 9900 4050 2    60   Input ~ 0
SW3
Text GLabel 9900 5000 2    60   Input ~ 0
SW4
$Comp
L GND #PWR08
U 1 1 5ADD2C56
P 9500 2100
F 0 "#PWR08" H 9500 1850 50  0001 C CNN
F 1 "GND" H 9500 1950 50  0000 C CNN
F 2 "" H 9500 2100 50  0001 C CNN
F 3 "" H 9500 2100 50  0001 C CNN
	1    9500 2100
	0    1    1    0   
$EndComp
$Comp
L GND #PWR09
U 1 1 5ADD4016
P 9500 3150
F 0 "#PWR09" H 9500 2900 50  0001 C CNN
F 1 "GND" H 9500 3000 50  0000 C CNN
F 2 "" H 9500 3150 50  0001 C CNN
F 3 "" H 9500 3150 50  0001 C CNN
	1    9500 3150
	0    1    1    0   
$EndComp
$Comp
L GND #PWR010
U 1 1 5ADD418C
P 9500 4050
F 0 "#PWR010" H 9500 3800 50  0001 C CNN
F 1 "GND" H 9500 3900 50  0000 C CNN
F 2 "" H 9500 4050 50  0001 C CNN
F 3 "" H 9500 4050 50  0001 C CNN
	1    9500 4050
	0    1    1    0   
$EndComp
$Comp
L GND #PWR011
U 1 1 5ADD4235
P 9500 5000
F 0 "#PWR011" H 9500 4750 50  0001 C CNN
F 1 "GND" H 9500 4850 50  0000 C CNN
F 2 "" H 9500 5000 50  0001 C CNN
F 3 "" H 9500 5000 50  0001 C CNN
	1    9500 5000
	0    1    1    0   
$EndComp
$Comp
L Battery_Cell BT1
U 1 1 5ADD440D
P 850 6000
F 0 "BT1" H 950 6100 50  0000 L CNN
F 1 "3V battery" H 950 6000 50  0000 L CNN
F 2 "Battery_Holders:Keystone_3034_1x20mm-CoinCell" V 850 6060 50  0001 C CNN
F 3 "" V 850 6060 50  0001 C CNN
	1    850  6000
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR012
U 1 1 5ADD460D
P 850 6100
F 0 "#PWR012" H 850 5850 50  0001 C CNN
F 1 "GND" H 850 5950 50  0000 C CNN
F 2 "" H 850 6100 50  0001 C CNN
F 3 "" H 850 6100 50  0001 C CNN
	1    850  6100
	1    0    0    -1  
$EndComp
$Comp
L +3V3 #PWR013
U 1 1 5B0A37CE
P 850 5800
F 0 "#PWR013" H 850 5650 50  0001 C CNN
F 1 "+3V3" H 850 5940 50  0000 C CNN
F 2 "" H 850 5800 50  0001 C CNN
F 3 "" H 850 5800 50  0001 C CNN
	1    850  5800
	1    0    0    -1  
$EndComp
$Comp
L 1.0UF-0603-16V-10% C1
U 1 1 5B25ACEC
P 1600 6000
F 0 "C1" H 1660 6115 45  0000 L BNN
F 1 "1.0UF-0603-16V-10%" H 1300 5450 45  0000 L BNN
F 2 "Capacitors_SMD:C_0603" H 1600 6250 20  0001 C CNN
F 3 "" H 1600 6000 50  0001 C CNN
F 4 "CAP-00868" H 1550 5550 60  0000 C CNN "Field4"
	1    1600 6000
	1    0    0    -1  
$EndComp
$Comp
L 10UF-0603-6.3V-20% C2
U 1 1 5B25AEA2
P 2350 6000
F 0 "C2" H 2410 6115 45  0000 L BNN
F 1 "10UF-0603-6.3V-20%" H 2200 5450 45  0000 L BNN
F 2 "Capacitors_SMD:C_0603" H 2350 6250 20  0001 C CNN
F 3 "" H 2350 6000 50  0001 C CNN
F 4 "CAP-11015" H 2450 5600 60  0000 C CNN "Field4"
	1    2350 6000
	1    0    0    -1  
$EndComp
$Comp
L +3V3 #PWR014
U 1 1 5B25B2B9
P 1600 5800
F 0 "#PWR014" H 1600 5650 50  0001 C CNN
F 1 "+3V3" H 1600 5940 50  0000 C CNN
F 2 "" H 1600 5800 50  0001 C CNN
F 3 "" H 1600 5800 50  0001 C CNN
	1    1600 5800
	1    0    0    -1  
$EndComp
$Comp
L +3V3 #PWR015
U 1 1 5B25B308
P 2350 5800
F 0 "#PWR015" H 2350 5650 50  0001 C CNN
F 1 "+3V3" H 2350 5940 50  0000 C CNN
F 2 "" H 2350 5800 50  0001 C CNN
F 3 "" H 2350 5800 50  0001 C CNN
	1    2350 5800
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR016
U 1 1 5B25B346
P 1600 6100
F 0 "#PWR016" H 1600 5850 50  0001 C CNN
F 1 "GND" H 1600 5950 50  0000 C CNN
F 2 "" H 1600 6100 50  0001 C CNN
F 3 "" H 1600 6100 50  0001 C CNN
	1    1600 6100
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR017
U 1 1 5B25B384
P 2350 6100
F 0 "#PWR017" H 2350 5850 50  0001 C CNN
F 1 "GND" H 2350 5950 50  0000 C CNN
F 2 "" H 2350 6100 50  0001 C CNN
F 3 "" H 2350 6100 50  0001 C CNN
	1    2350 6100
	1    0    0    -1  
$EndComp
$Comp
L 10118194-0001LF J1
U 1 1 5B515E1C
P 5350 5900
F 0 "J1" H 5060 6120 50  0000 L BNN
F 1 "10118194-0001LF" H 5049 5599 50  0000 L BNN
F 2 "10118194-0001LF:FRAMATOME_10118194-0001LF" H 5350 5900 50  0001 L BNN
F 3 "None" H 5350 5900 50  0001 L BNN
F 4 "Amphenol" H 5350 5900 50  0001 L BNN "Field4"
F 5 "Warning" H 5350 5900 50  0001 L BNN "Field5"
F 6 "Micro Usb, 2.0 Type b, Rcpt, Smt" H 5350 5900 50  0001 L BNN "Field6"
F 7 "10118194-0001LF" H 5350 5900 50  0001 L BNN "Field7"
F 8 "0.26 USD" H 5350 5900 50  0001 L BNN "Field8"
	1    5350 5900
	1    0    0    -1  
$EndComp
Text GLabel 4850 5800 0    60   Input ~ 0
USB_D+
Text GLabel 4850 5900 0    60   Input ~ 0
USB_D-
Text GLabel 2200 3100 2    60   Input ~ 0
USB_D+
Text GLabel 2200 3000 2    60   Input ~ 0
USB_D-
$Comp
L GND #PWR018
U 1 1 5B516002
P 5850 6000
F 0 "#PWR018" H 5850 5750 50  0001 C CNN
F 1 "GND" H 5850 5850 50  0000 C CNN
F 2 "" H 5850 6000 50  0001 C CNN
F 3 "" H 5850 6000 50  0001 C CNN
	1    5850 6000
	1    0    0    -1  
$EndComp
$Comp
L MOMENTARY-SWITCH-6x6x4.5mm S2
U 1 1 5B5176C0
P 9700 3150
F 0 "S2" H 9500 3250 45  0000 L BNN
F 1 "MOMENTARY-SWITCH-6x6x4.5mm" H 9200 2900 45  0000 L BNN
F 2 "homebrew_button:6x6x4.3mm button" H 9700 3350 20  0001 C CNN
F 3 "" H 9700 3150 50  0001 C CNN
F 4 "SWCH-PTS645" H 9700 3400 60  0000 C CNN "Field4"
	1    9700 3150
	1    0    0    -1  
$EndComp
$Comp
L MOMENTARY-SWITCH-6x6x4.5mm S3
U 1 1 5B51772C
P 9700 4050
F 0 "S3" H 9500 4150 45  0000 L BNN
F 1 "MOMENTARY-SWITCH-6x6x4.5mm" H 9200 3800 45  0000 L BNN
F 2 "homebrew_button:6x6x4.3mm button" H 9700 4250 20  0001 C CNN
F 3 "" H 9700 4050 50  0001 C CNN
F 4 "SWCH-PTS645" H 9700 4300 60  0000 C CNN "Field4"
	1    9700 4050
	1    0    0    -1  
$EndComp
$Comp
L MOMENTARY-SWITCH-6x6x4.5mm S4
U 1 1 5B517785
P 9700 5000
F 0 "S4" H 9500 5100 45  0000 L BNN
F 1 "MOMENTARY-SWITCH-6x6x4.5mm" H 9200 4750 45  0000 L BNN
F 2 "homebrew_button:6x6x4.3mm button" H 9700 5200 20  0001 C CNN
F 3 "" H 9700 5000 50  0001 C CNN
F 4 "SWCH-PTS645" H 9700 5250 60  0000 C CNN "Field4"
	1    9700 5000
	1    0    0    -1  
$EndComp
NoConn ~ 4850 6000
NoConn ~ 5850 5800
$Comp
L MOMENTARY-SWITCH-6x6x4.5mm S1
U 1 1 5B518565
P 9700 2100
F 0 "S1" H 9500 2200 45  0000 L BNN
F 1 "MOMENTARY-SWITCH-6x6x4.5mm" H 9450 1900 45  0000 L BNN
F 2 "homebrew_button:6x6x4.3mm button" H 9700 2300 20  0001 C CNN
F 3 "" H 9700 2100 50  0001 C CNN
F 4 "SWCH-PTS645" H 9700 2350 60  0000 C CNN "Field4"
	1    9700 2100
	1    0    0    -1  
$EndComp
Wire Wire Line
	9500 2200 9500 2100
Wire Wire Line
	9900 2200 9900 2100
Wire Wire Line
	9500 3250 9500 3150
Wire Wire Line
	9900 3250 9900 3150
Wire Wire Line
	9500 4150 9500 4050
Wire Wire Line
	9900 4150 9900 4050
Wire Wire Line
	9500 5100 9500 5000
Wire Wire Line
	9900 5100 9900 5000
$EndSCHEMATC
