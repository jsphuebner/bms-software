# stm32-bms
This is the firmware for the 4-channel daisy-chained BMS discussed here:
https://openinverter.org/forum/viewtopic.php?f=13&t=60
It implements the communication with the 4-channel sense modules, has basic balancing support, implements Coloumb counting with either an analog or an ISA current sensor. It also calculates maximum charge and discharge currents depending on your settings. It has a basic algorithm for estimating the SoC of an LFP pack by measuring open circuit voltage after some hours of settling time.

# Compiling
You will need the arm-none-eabi toolchain: https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads
The only external depedency is libopencm3 which I forked. You can download and build this dependency by typing

`make get-deps`

You also need to compile cell-module-firmware as its binary is included with stm32-bms in order to do firmware upgrades of the cell modules
Now you can compile stm32-bms by typing

`make`

And upload it to your board using a JTAG/SWD adapter, the updater.py script or the esp8266 web interface
