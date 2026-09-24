# ATU-10  - The Tyny QRP Automatic Antenna Tuner

### Official conversation group - https://groups.io/g/ATU100
### Schematic and assembly instruction by VK3PE - http://carnut.info/ATU_N7DDC/ATU-10/ATU-10_by-vk3pe_build_info/ATU-10_vk3pe_V1.2_ALL_INFO_290921.pdf

## DL8UG fork: FW 1.7 and 1.8 (XC8 port)
This fork continues N7DDC's FW 1.6: ported from mikroC to the free Microchip XC8 compiler (builds on Linux) and improved.
Many thanks to David Fainitski, N7DDC, for the original hardware and firmware. Programming was assisted by Claude Code.

| Version | Firmware | Status |
|---|---|---|
| FW 1.8 | [Firmware/ATU-10_FW_18_xc8](Firmware/ATU-10_FW_18_xc8/README.md), `ATU-10_FW_18_xc8.zip` | **beta**: newest, not yet tested on the device |
| FW 1.7 | [Firmware/ATU-10_FW_17_xc8](Firmware/ATU-10_FW_17_xc8/README.md), `ATU-10_FW_17_xc8.zip`, [release v1.7](https://github.com/DL8UG/ATU-10-10W-QRP-antenna-tuner/releases/tag/v1.7) | **stable**: tested on the device, recommended |
| FW 1.6 and older | `Firmware/ATU-10_FW_16` … `ATU-10_FW_10` | original N7DDC firmware (mikroC) |

**Flashing** works as before: connect the tuner via USB-C and copy the hex onto its USB drive. Any older hex can be flashed back the same way.

**Operation from FW 1.7 on:**
- Short press: bypass on/off. Bypass is true bypass (L=0, C=0), and switching it off restores the last tuned setting. The original did a reset to L=0, C=1 instead.
- Long press: tune. A short press while tuning cancels it and switches to bypass.
- Very long press (approx. 2.5 s): power off.
- External interface (Icom): "Reset" switches to bypass only, "Tune" tunes as before.
- FW 1.8: the relay setting survives a reset or battery change. After an unusual restart the display shows the reason for 2 s (`LOW BATT`, `WDT RESET`, `STACK RST`).

**Settings (Cells) from FW 1.7 on** are set in `Cells[]` in `main.c` and the firmware is rebuilt, there is no hex editing any more. The meaning of the cells is the same, see *Cells description* below.

**Feedback** is very welcome, especially test reports with other antennas and bands: please open an [issue](https://github.com/DL8UG/ATU-10-10W-QRP-antenna-tuner/issues) or post in the [ATU100 group](https://groups.io/g/ATU100).

###### New in FW version 1.8 (DL8UG, beta)
1 - Quick retune: after a QSY a fine search from the current setting comes first, which needs about 60 % fewer relay steps.
2 - The relay setting and bypass state are kept in the EEPROM, so display and relays agree after a reset or battery change.
3 - Watchdog and brown-out reset, the reason of a restart is shown on the display.
4 - A short press while tuning now cancels cleanly (FW 1.6 showed bypass while the relays were tuned).
5 - A failed tune ends in true bypass instead of leaving 22 pF in parallel.

###### New in FW version 1.7 (DL8UG, stable)
1 - Port from mikroC to the free Microchip XC8 compiler, builds on Linux.
2 - Fixes: SWR display accuracy (1.01 was shown as 1.03), relay state after the coarse search.
3 - Better tuning: averaged measurement, Pr/Pf metric instead of the capped SWR, repeated fine search, coarse search up to relay 64 with tolerance.
4 - Tuning on 80 m with a QRP rig no longer aborts showing ~20 W (the power limit checks Pf − Pr).
5 - Short press toggles a true bypass.
6 - Display robustness: I2C bus recovery, periodic refresh of the display settings, fixed interrupt races.
7 - PC simulator for the tuning algorithm.

###### New in FW version 1.6
1 - New better meassurement formula for Power and SWR calculation, compensation and calibration.  
2 - Setting by Cells control implementation. In FW 1.6 the cells are changed in the hex file, see [Firmware/ATU-10_FW_16](Firmware/ATU-10_FW_16/README.md); from FW 1.7 on in `main.c`.

Cells description:
1) Time to display off in minutes, 5 mins by default, 0 to always on display
2) time to power off in minutes, 30 mins by default, 0 to always power on
3) relay's delay time, voltage applied to coiles in ms, 7 ms by default
4) min power to start tuning in ten's parts of Watt, 10 by default (1.0 W). This value can not be 0
5) max power to start tuning in watts, 15 by default
6) Delta SWR to auto start tuning in ten's parts SWR, 13 by default (SWR = 1.3)
   Note (DL8UG): in the code this is a change of the SWR by (value − 10) tenths since the last tune, i.e. 0.3 for 13, not a threshold of 1.3.
7) Auto mode 1 for activate or 0 to off. 1 by default
8) Calibration coefficient for 1W power, 4 by default for BAT41 diodes
9) calibration coefficient for 10W power, 14 by default for BAT41 diodes
10) Peak detector time for Power meassurement in tens ms, 60 (600ms)

If you are using 1N5711 diodes in the RF detector, you can calibrate the power meassurement.
Apply known 1W of power on 7MHz and change cell 8 for correct value (from FW 1.7 on in `main.c`).
Apply known 10W of power on 7MHw and change cell 9 for correct value.
Repeat it couple times to reach a good result.

###### New in FW version 1.5
1 - Tuning algorithm improvement   
2 - Minor changes 

###### New in FW version 1.4
1 - The button glitches solved   
2 - More stability in bus transfer to OLED   
3 - No continious data transferring to the display   

###### New in FW version 1.2  
1 - Display memory feature for last SWR  added  
2 - full automatic mode error solved.

###### New in FW version 1.1  
1 - the control method has been reworked, there is no more sleep mode. Now the tuner either shines on the display for 5 minutes after it is disturbed, or turns off after 30 minutes if it is not touched and the transmitter is not turned on. For these 30 minutes, the tuner constantly monitors the power supplied and instantly lights up the display when needed. The current consumption in this monitoring mode is 4 mA.
Long press on the button now turns the device on and off.  
2 - external control using the Icom protocol is implemented, works in both directions. That is, when the tuner button is pressed, the transceiver automatically generates a carrier for tuning and when changing from band to band, the transceiver initiates tuning by the tuner. When you press the button of external tuner control on the transceiver, the tuner is automatically run if ON or reset if OFF position.  

### Description
   The tuner is assembled in an affordable Chinese case 100x71x25 mm, the front and rear panels are made as PCB, in the same way as the main printed circuit board.
On the front panel there is a control button, a small 0.91" OLED 128 * 32 display and a USB Type C connector, used to charge the tuner's built-in battery and connect to a computer to change the firmware.

[![](https://github.com/Dfinitski/ATU-10-10W-QRP-antenna-tuner/blob/main/Photos/tuner_1.jpg)](https://github.com/Dfinitski/ATU-10-10W-QRP-antenna-tuner/blob/main/Photos/tuner_1.jpg)

   The rear panel contains RF BNC connectors, a ground clamp and an external control interface connector that can be connected to the transceiver for more convenient control of the tuner.

[![](https://github.com/Dfinitski/ATU-10-10W-QRP-antenna-tuner/blob/main/Photos/tuner_2.jpg)](https://github.com/Dfinitski/ATU-10-10W-QRP-antenna-tuner/blob/main/Photos/tuner_2.jpg)

   The control button has only 3 functions - a short press resets the tuner and sets all relays to their initial state, in which all reactive elements are disabled and do not affect the signal flow through the tuner, a long press causes the tuner to enter the tuning mode and a very long press for more than 5 seconds causes the firmware version to be displayed on the display.
   > Note (DL8UG): this describes the original firmware. From FW 1.7 on, the short press toggles a true bypass and the very long press powers the tuner off, see *Operation from FW 1.7 on* above.
   The display mainly shows the current transmitter power and SWR in the transmitter cable and sometimes briefly indicates the modes.
   
   The tuner is built on IM41 bistable relays, which means that a significant current is consumed by the tuner only for a short time during tuning; in rest mode, the tuner's relays retain their state for an arbitrarily long time, without consuming power.

   The built-in battery consists of two Li-Ion 14500 batteries connected in parallel. The best examples of these (SANYO) have a capacity of 800 mAh each, the worst ones are usually 400 mAh.
    The tuner has 3 power consumption modes - the operating mode, when the display indicates the parameters, lasts 5 minutes, after which the tuner falls asleep and the display turns off. You can wake him up with a short press on the button. After 30 minutes of sleep, the tuner turns off completely and you can turn it on by holding the button for a long time (more than 3 seconds).
    
   The current consumption in the operating mode is 12 mA, in the sleep mode 170 μA, in the off state 37 μA.
   
[![](https://github.com/Dfinitski/ATU-10-10W-QRP-antenna-tuner/blob/main/Photos/tuner_3.jpg)](https://github.com/Dfinitski/ATU-10-10W-QRP-antenna-tuner/blob/main/Photos/tuner_3.jpg)

   The main microprocessor of the tuner is PIC16F18877, another PIC16F1454 processor is used as an embedded programmer. It should be flashed once using the programmer with a special firmware, after which it will be possible to change the main firmware of the tuner as many times as necessary without resorting to special means. After connecting the tuner to the computer, a new logical disk with the appropriate name will appear in the explorer, in which you can find a link to the firmware repository. To flash the tuner, it is enough to copy the firmware file to this disk, the process takes a couple of seconds and the tuner is flashed at the same moment when the file copying is completed.
   
   There is also space for two dual-color LEDs with a common anode on the board, they can be installed if you do not need an OLED display.
The right LED indicates the battery charging process, the left LED indicates the operation. In operating mode, it blinks briefly in color every three seconds, depending on the battery charge level.

