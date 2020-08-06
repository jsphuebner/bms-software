# Intro
This repo contains the firmware for the BMS project. The cell-module-firmware, surprise, contains the actual firmware. cell-module-calibration contains a special program for calibrating the cell module

# Fuses
The most important fuses are SELFPRGEN and EESAVE. They make sure the device can flash its own firmware and that the EEPROM content is preserved from chip erase.
- efuse 0xFE
- hfuse 0xD7
- lfuse 0x62

# Calibration
All components used on the cell module are off-the-shelv cheap and available. On the downside this means that the resistors and voltage references deviate from their design values. To compensate for this, the cell module must be calibrated to be able to deliver accurate voltage measurements. More precisly we need to calibrate:

- The internal clock
- The internal temperature sensor
- The internal 20x gain stage
- The 4 cell voltage channels in single ended mode
- The 4 cell voltage channels in differential mode

To be able to do the calibration, you need
- A 2kHz clock signal
- a known room temperature
- A stable voltage between about 80 - 140mV
- 4 stable, known voltages in series, e.g. 4, 8, 12, 16V. You can just use your actual battery if you have a precise multimeter to calibrate against

The provided firmware works with a sequencer, as some pins are multiplexed. This means at first the clock signal is put on pin PA6, then the 100mV reference. The sequencer is controlled by PA5. If you don't have one you'll need to hack the software to first do the clock calibration, then the voltage calibration. Also make sure to pull PA5 to GND of your "100mV" reference with the like of a 470 Ohm resistor.

Furthermore the expected values are hard-coded into the calibration firmware. You need to change the static const uint16_t expected[] to reflect your 4 cell voltages, static const uint8_t refTemp = 22; to reflect your room temperature and static const int32_t reference = 26373; to reflect your "100mV" reference.

# Communication
With all calibration values safely stored in the EEPROM you are ready to flash the application firmware. Operation is confirmed by a running light that tells you the module is waiting for an address.

It is now time to wire up the communication link. It is a 1-wire daisy chain archicture, that makes use of the fact, that adjacent modules are on a similar potential. The input of the first module is the master module. The input of the 2nd module connects to the output of the first module, the input of the n-th module connects to the output of the (n-1)-th module. The output of the n-th module loops back to the master module.
