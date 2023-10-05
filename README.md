PAO_EVCU
=====
This is a port of the GEVCU library(https://github.com/collin80/GEVCU6) to work with the feather board (paired with the canbus module).
The goal of this project is to provide a barebones implemntations to work with a gas and brake pedal to communicate with DMOC 645 without the need of the GEVCU hardware.
The advantage of using the feather board (M0 in the case) is that it provides built in BT support as well as the fact that it is very expandable as you can stack feather modules to add functionality

HARDWARE needed
feather MO board: https://www.adafruit.com/product/3403
featehr canbus shield: https://www.adafruit.com/product/5709

note: other variations of the feather board may be used but the pinouts can vary



ANDROID APP 
can be found here: https://github.com/vkorotchenko/pao_mobile/


FEATHER PINOUT
3V - 3.3V
REF - 3.3V
GND - GND
A0 - brake adc (input)
A1 - Throttle 1
A2 - Throttle 2
A3 - Enable
A4 - Reverse 
A5 - Main contactor
9 - Precharge
10 - CoolFan
11 - BrakeLight (output)
12  - Rev Light

//CAN bus pins ( CAN shield)
CanH - CanH
CanL - CanL
CanG - CanG