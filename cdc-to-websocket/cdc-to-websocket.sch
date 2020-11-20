EESchema Schematic File Version 4
EELAYER 30 0
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
$Comp
L power:PWR_FLAG #FLG0101
U 1 1 5FA306A6
P 9650 1750
F 0 "#FLG0101" H 9650 1825 50  0001 C CNN
F 1 "PWR_FLAG" H 9650 1923 50  0000 C CNN
F 2 "" H 9650 1750 50  0001 C CNN
F 3 "~" H 9650 1750 50  0001 C CNN
	1    9650 1750
	1    0    0    -1  
$EndComp
$Comp
L power:PWR_FLAG #FLG0102
U 1 1 5FA30D5B
P 10250 1750
F 0 "#FLG0102" H 10250 1825 50  0001 C CNN
F 1 "PWR_FLAG" H 10250 1923 50  0000 C CNN
F 2 "" H 10250 1750 50  0001 C CNN
F 3 "~" H 10250 1750 50  0001 C CNN
	1    10250 1750
	1    0    0    -1  
$EndComp
$Comp
L power:+5V #PWR0101
U 1 1 5FA3136A
P 9350 1750
F 0 "#PWR0101" H 9350 1600 50  0001 C CNN
F 1 "+5V" H 9365 1923 50  0000 C CNN
F 2 "" H 9350 1750 50  0001 C CNN
F 3 "" H 9350 1750 50  0001 C CNN
	1    9350 1750
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0102
U 1 1 5FA318E4
P 10250 1900
F 0 "#PWR0102" H 10250 1650 50  0001 C CNN
F 1 "GND" H 10255 1727 50  0000 C CNN
F 2 "" H 10250 1900 50  0001 C CNN
F 3 "" H 10250 1900 50  0001 C CNN
	1    10250 1900
	1    0    0    -1  
$EndComp
Wire Wire Line
	9350 1750 9350 1850
Wire Wire Line
	9350 1850 9650 1850
Wire Wire Line
	9650 1850 9650 1750
Wire Wire Line
	10250 1750 10250 1900
$Comp
L ESP32-DEVKITC-32D:ESP32-DEVKITC-32D U1
U 1 1 5FA66333
P 2100 2350
F 0 "U1" H 2100 3517 50  0000 C CNN
F 1 "ESP32-DEVKITC-32D" H 2100 3426 50  0000 C CNN
F 2 "ESP32-DEVKITC-32D:MODULE_ESP32-DEVKITC-32D" H 2100 2350 50  0001 L BNN
F 3 "" H 2100 2350 50  0001 L BNN
F 4 "4" H 2100 2350 50  0001 L BNN "PARTREV"
F 5 "Espressif Systems" H 2100 2350 50  0001 L BNN "MANUFACTURER"
	1    2100 2350
	1    0    0    -1  
$EndComp
Text GLabel 3100 2950 2    50   Input ~ 0
DEBUG_AP
Text GLabel 3100 2550 2    50   Input ~ 0
STM32RX_ESP32TX
Text GLabel 3100 2450 2    50   Input ~ 0
STM32TX_ESP32RX
Text GLabel 3100 2250 2    50   Input ~ 0
STM32CTS_ESP32RTS
Text GLabel 3100 2150 2    50   Input ~ 0
STM32RTS_ESP32CTS
$Comp
L power:GND #PWR0103
U 1 1 5FA79748
P 3100 1300
F 0 "#PWR0103" H 3100 1050 50  0001 C CNN
F 1 "GND" H 3105 1127 50  0000 C CNN
F 2 "" H 3100 1300 50  0001 C CNN
F 3 "" H 3100 1300 50  0001 C CNN
	1    3100 1300
	1    0    0    -1  
$EndComp
Wire Wire Line
	2900 1450 2900 1250
Wire Wire Line
	2900 1250 3100 1250
Wire Wire Line
	3100 1250 3100 1300
Wire Wire Line
	3100 2150 2900 2150
Wire Wire Line
	2900 2250 3100 2250
Wire Wire Line
	3100 2450 2900 2450
Wire Wire Line
	3100 2550 2900 2550
Wire Wire Line
	3100 2950 2900 2950
$Comp
L power:+5V #PWR0104
U 1 1 5FA82FA6
P 850 3250
F 0 "#PWR0104" H 850 3100 50  0001 C CNN
F 1 "+5V" H 865 3423 50  0000 C CNN
F 2 "" H 850 3250 50  0001 C CNN
F 3 "" H 850 3250 50  0001 C CNN
	1    850  3250
	1    0    0    -1  
$EndComp
Wire Wire Line
	1300 3250 850  3250
$Comp
L mylib:WeAct-MiniF4 U2
U 1 1 5FA948D1
P 5800 2350
F 0 "U2" H 5800 3615 50  0000 C CNN
F 1 "WeAct-MiniF4" H 5800 3524 50  0000 C CNN
F 2 "footprint:WeAct-MiniF4" H 5800 2350 50  0001 C CNN
F 3 "" H 5800 2350 50  0001 C CNN
	1    5800 2350
	1    0    0    -1  
$EndComp
Text GLabel 4850 2450 0    50   Input ~ 0
SWO
Wire Wire Line
	5000 2450 4850 2450
$Comp
L power:GND #PWR0105
U 1 1 5FA9E2CA
P 4800 3450
F 0 "#PWR0105" H 4800 3200 50  0001 C CNN
F 1 "GND" H 4805 3277 50  0000 C CNN
F 2 "" H 4800 3450 50  0001 C CNN
F 3 "" H 4800 3450 50  0001 C CNN
	1    4800 3450
	1    0    0    -1  
$EndComp
$Comp
L power:+5V #PWR0106
U 1 1 5FA9F846
P 4600 3050
F 0 "#PWR0106" H 4600 2900 50  0001 C CNN
F 1 "+5V" H 4615 3223 50  0000 C CNN
F 2 "" H 4600 3050 50  0001 C CNN
F 3 "" H 4600 3050 50  0001 C CNN
	1    4600 3050
	1    0    0    -1  
$EndComp
Wire Wire Line
	5000 3250 4800 3250
Wire Wire Line
	4800 3250 4800 3450
Wire Wire Line
	5000 3150 4600 3150
Wire Wire Line
	4600 3150 4600 3050
Text GLabel 6800 2850 2    50   Input ~ 0
STM32CTS_ESP32RTS
Text GLabel 6800 2750 2    50   Input ~ 0
STM32RTS_ESP32CTS
Text GLabel 6800 2650 2    50   Input ~ 0
STM32TX_ESP32RX
Text GLabel 6800 2550 2    50   Input ~ 0
STM32RX_ESP32TX
Wire Wire Line
	6800 2550 6600 2550
Wire Wire Line
	6600 2650 6800 2650
Wire Wire Line
	6800 2750 6600 2750
Wire Wire Line
	6800 2850 6600 2850
NoConn ~ 5000 1450
NoConn ~ 5000 1550
NoConn ~ 5000 1650
NoConn ~ 5000 1750
NoConn ~ 5000 1850
NoConn ~ 5000 1950
NoConn ~ 5000 2050
NoConn ~ 5000 2150
NoConn ~ 5000 2250
NoConn ~ 5000 2350
NoConn ~ 5000 2550
NoConn ~ 5000 2650
NoConn ~ 5000 2750
NoConn ~ 5000 2850
NoConn ~ 5000 2950
NoConn ~ 5000 3050
NoConn ~ 5000 3350
NoConn ~ 6600 3350
NoConn ~ 6600 3250
NoConn ~ 6600 3150
NoConn ~ 6600 3050
NoConn ~ 6600 2950
NoConn ~ 6600 2450
NoConn ~ 6600 2350
NoConn ~ 6600 2250
NoConn ~ 6600 2150
NoConn ~ 6600 2050
NoConn ~ 6600 1950
NoConn ~ 6600 1850
NoConn ~ 6600 1750
NoConn ~ 6600 1650
NoConn ~ 6600 1550
NoConn ~ 6600 1450
NoConn ~ 2900 3250
NoConn ~ 2900 3150
NoConn ~ 2900 3050
NoConn ~ 2900 2850
NoConn ~ 2900 2750
NoConn ~ 2900 2650
NoConn ~ 2900 2350
NoConn ~ 2900 2050
NoConn ~ 2900 1950
NoConn ~ 2900 1850
NoConn ~ 2900 1750
NoConn ~ 2900 1650
NoConn ~ 2900 1550
NoConn ~ 1300 1450
NoConn ~ 1300 1550
NoConn ~ 1300 1650
NoConn ~ 1300 1750
NoConn ~ 1300 1850
NoConn ~ 1300 1950
NoConn ~ 1300 2050
NoConn ~ 1300 2150
NoConn ~ 1300 2250
NoConn ~ 1300 3150
NoConn ~ 1300 3050
NoConn ~ 1300 2950
NoConn ~ 1300 2850
NoConn ~ 1300 2750
NoConn ~ 1300 2650
NoConn ~ 1300 2550
NoConn ~ 1300 2450
NoConn ~ 1300 2350
$Comp
L power:GND #PWR0107
U 1 1 5FACF143
P 3050 4350
F 0 "#PWR0107" H 3050 4100 50  0001 C CNN
F 1 "GND" H 3055 4177 50  0000 C CNN
F 2 "" H 3050 4350 50  0001 C CNN
F 3 "" H 3050 4350 50  0001 C CNN
	1    3050 4350
	1    0    0    -1  
$EndComp
$Comp
L mylib:SW_PUSH SW1
U 1 1 5FACF4E4
P 2750 4200
F 0 "SW1" H 2750 4455 50  0000 C CNN
F 1 "SW_PUSH" H 2750 4364 50  0000 C CNN
F 2 "Button_Switch_THT:SW_Tactile_Straight_KSA0Axx1LFTR" H 2750 4200 50  0001 C CNN
F 3 "" H 2750 4200 50  0000 C CNN
	1    2750 4200
	1    0    0    -1  
$EndComp
Text GLabel 2250 4200 0    50   Input ~ 0
DEBUG_AP
Wire Wire Line
	2250 4200 2450 4200
Wire Wire Line
	3050 4200 3050 4350
$Comp
L Connector:Conn_01x01_Female J1
U 1 1 5FAD0F93
P 4500 4050
F 0 "J1" H 4528 4076 50  0000 L CNN
F 1 "Conn_01x01_Female" H 4528 3985 50  0000 L CNN
F 2 "Connector_Wire:SolderWirePad_1x01_Drill0.8mm" H 4500 4050 50  0001 C CNN
F 3 "~" H 4500 4050 50  0001 C CNN
	1    4500 4050
	1    0    0    -1  
$EndComp
Text GLabel 4200 4050 0    50   Input ~ 0
SWO
Wire Wire Line
	4200 4050 4300 4050
$EndSCHEMATC