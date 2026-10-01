# ATU-10  - The Tyny QRP Automatic Antenna Tuner

### Official conversation group - https://groups.io/g/ATU100
### Schematic and assembly instruction by VK3PE - http://carnut.info/ATU_N7DDC/ATU-10/ATU-10_by-vk3pe_build_info/ATU-10_vk3pe_V1.2_ALL_INFO_290921.pdf

## DL8UG fork: FW 1.7 to 1.8.2 (XC8 port)
This fork continues N7DDC's FW 1.6. It is ported from mikroC to the free Microchip XC8 compiler, builds on Linux, and has better tuning and a more robust display.
Many thanks to David Fainitski, N7DDC, for the original hardware and firmware. Programming was assisted by Claude Code.

Contents: [Versions](#versions) · [Flashing](#flashing) · [Changes compared to FW 1.6](#changes-compared-to-n7ddc-fw-16) · [Operation](#operation) · [Settings (Cells)](#settings-cells) · [Coming in FW 1.9](#coming-in-fw-19) · [Feedback](#feedback) · [Version history](#version-history) · [Original description](#original-description-n7ddc)

### Versions
| Version | Firmware | Status |
|---|---|---|
| FW 1.8.2 | [Firmware/ATU-10_FW_182_xc8](Firmware/ATU-10_FW_182_xc8/README.md), `ATU-10_FW_182_xc8.zip`, [release v1.8.2](https://github.com/DL8UG/ATU-10-10W-QRP-antenna-tuner/releases/tag/v1.8.2) | **stable, recommended**: display reset after each transmission, display status check, no more display stripes. Tested on the device by DL8UG (4 h including idle) |
| FW 1.8.1 | [Firmware/ATU-10_FW_181_xc8](Firmware/ATU-10_FW_181_xc8/README.md), `ATU-10_FW_181_xc8.zip`, [release v1.8.1](https://github.com/DL8UG/ATU-10-10W-QRP-antenna-tuner/releases/tag/v1.8.1) | **stable**: automatic display reset against display lock-ups, previous version |
| FW 1.8 | [Firmware/ATU-10_FW_18_xc8](Firmware/ATU-10_FW_18_xc8/README.md), `ATU-10_FW_18_xc8.zip`, [release v1.8](https://github.com/DL8UG/ATU-10-10W-QRP-antenna-tuner/releases/tag/v1.8) | **stable**: quick retune, relay setting kept in EEPROM, watchdog/brown-out |
| FW 1.7 | [Firmware/ATU-10_FW_17_xc8](Firmware/ATU-10_FW_17_xc8/README.md), `ATU-10_FW_17_xc8.zip`, [release v1.7](https://github.com/DL8UG/ATU-10-10W-QRP-antenna-tuner/releases/tag/v1.7) | **stable**: first XC8 version, better tuning, true bypass |
| FW 1.6 and older | `Firmware/ATU-10_FW_16` … `ATU-10_FW_10` | original N7DDC firmware (mikroC) |

Each version has its own folder with the hex file, the sources and a detailed README (build, changes, simulator results).

### Flashing
Flashing works as before: connect the tuner via USB-C, copy the hex onto its USB drive and run `sync` (Linux) or eject the drive. Any older hex, including N7DDC's original firmware, can be flashed back the same way.

### Changes compared to N7DDC FW 1.6
The version in brackets is the one that introduced the change.

**Build**
- Ported from mikroC PRO to the free Microchip XC8 compiler, builds on Linux with `make` (1.7).
- The hex file is written in the mikroC format, so the tuner's built-in USB programmer accepts it as before (1.7).

**Measurement**
- SWR display accuracy fixed: 1.01 was shown as 1.03 (1.7).
- Forward and reflected power are averaged during tuning (1.7).
- Tuning on 80 m with a QRP rig no longer aborts showing ~20 W: the power limit checks the net power Pf − Pr (1.7).

**Tuning**
- Reflection metric Pr/Pf instead of the capped SWR value, so the search sees a difference between bad and very bad settings (1.7).
- Relay state after the coarse search fixed, repeated fine search, coarse search up to relay 64 with a tolerance (1.7).
- Quick retune: after a QSY a fine search from the current setting comes first, which needs about 60 % fewer relay steps (1.8).
- A failed tune ends in true bypass instead of leaving 22 pF in parallel (1.8).
- Result in the PC simulator with 126 standard loads: mean SWR after tuning 4.23 with FW 1.6, 1.89 with FW 1.7.

**Operation**
- Short press toggles a true bypass (L=0, C=0) and back to the last tuned setting (1.7).
- A short press while tuning cancels cleanly and switches to bypass (1.8). FW 1.6 showed bypass while the relays stayed tuned.
- External interface (Icom): "Reset" switches to bypass only (1.7).

**Reliability**
- Relay setting and bypass state are kept in the EEPROM, so display and relays agree after a reset or battery change (1.8).
- Watchdog (~8 s) and brown-out reset (2.7 V). The reason for an unusual restart is shown for 2 s: `LOW BATT`, `WDT RESET`, `STACK RST` (1.8).

**Display**
- I2C bus recovery, periodic refresh of the display settings against ghost images, interrupt races fixed (1.7).
- Automatic display reset (300 ms power off, then new initialization) every 10 min without RF and after repeated display faults (1.8.1), and 2 s after each transmission and tune (1.8.2).
- The display status is read every 3 s. A display that stops answering or reports itself switched off is reset at once (1.8.2).
- The analog display settings are sent only at initialization. This fixed the vertical stripes over the whole display that appeared since FW 1.7, even on an idle tuner (1.8.2).

**Settings**
- The Cells are set in `main.c` and the firmware is rebuilt (1.7 to 1.8.2). From FW 1.9 on they will be editable in the hex file again, see [Coming in FW 1.9](#coming-in-fw-19).

**Tools**
- PC simulator for the tuning algorithm with standard loads and typical antennas (random wire with 9:1, EFHW with 49:1), see the README in the firmware folder (1.7).

### Operation
- **Short press**: bypass on/off. Bypass is true bypass (L=0, C=0), and switching it off restores the last tuned setting. The original firmware did a reset to L=0, C=1 instead.
- **Long press**: tune. A short press while tuning cancels it and switches to bypass.
- **Very long press** (approx. 2.5 s): power off.
- **External interface (Icom)**: "Reset" switches to bypass only, "Tune" tunes as before.
- The relay setting survives a reset or battery change (from FW 1.8).
- **Display**: FW 1.8.2 resets the display shortly after each transmission. With digital modes it goes dark for about 0.6 s after each transmission. If the display still fails completely (stays dark or garbled), restart the tuner (power off and on).

### Settings (Cells)
Up to FW 1.8.2 the cells are set in `Cells[]` in `main.c` and the firmware is rebuilt. In FW 1.6 they were changed in the hex file, see [Firmware/ATU-10_FW_16](Firmware/ATU-10_FW_16/README.md). The meaning of the cells is the same in all versions:

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
Apply known 1W of power on 7MHz and change cell 8 for correct value.
Apply known 10W of power on 7MHw and change cell 9 for correct value.
Repeat it couple times to reach a good result.

### Coming in FW 1.9
- The Cells will be editable in the hex file again, like in N7DDC's FW 1.6. All values from FW 1.6 can then be adjusted without rebuilding the firmware.
- A new cell will switch off the automatic display resets, for tuners whose display runs stable without them.

### Feedback
Feedback is very welcome, especially test reports with other tuners, antennas and bands. Please post in the project thread of the ATU100 group: [My new ATU-10 firmware](https://groups.io/g/ATU100/topic/my_new_atu_10_firmware/121418227). You can also open an [issue](https://github.com/DL8UG/ATU-10-10W-QRP-antenna-tuner/issues).
Useful details: band, rig and power, antenna and transformer, SWR before and after tuning, and your Cells settings.

### Version history

###### New in FW version 1.8.2 (DL8UG, stable)
1 - Display reset 2 s after each transmission and tune: strong RF is what locks up the display, so stripes now last only until the end of the transmission instead of up to 10 min.  
2 - The display status is read every 3 s. If a display that answered before stops answering or reports itself switched off, it is reset at once.  
3 - The analog display settings (clock, charge pump, pre-charge, VCOMH) are no longer resent to the running display. This was the cause of the vertical stripes that appeared even without RF since FW 1.7.  

###### New in FW version 1.8.1 (DL8UG, stable)
1 - Automatic display reset (short power cycle) every 10 min without RF and after repeated display faults, against display lock-ups (vertical stripes) after a longer run time.  
2 - Display settings are resent with a NOP prefix, so a command garbled by RF cannot shift them.

###### New in FW version 1.8 (DL8UG, stable)
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
2 - Setting by Cells control implementation. In FW 1.6 the cells are changed in the hex file, see [Settings (Cells)](#settings-cells).

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

### Original description (N7DDC)
   The tuner is assembled in an affordable Chinese case 100x71x25 mm, the front and rear panels are made as PCB, in the same way as the main printed circuit board.
On the front panel there is a control button, a small 0.91" OLED 128 * 32 display and a USB Type C connector, used to charge the tuner's built-in battery and connect to a computer to change the firmware.

[![](https://github.com/Dfinitski/ATU-10-10W-QRP-antenna-tuner/blob/main/Photos/tuner_1.jpg)](https://github.com/Dfinitski/ATU-10-10W-QRP-antenna-tuner/blob/main/Photos/tuner_1.jpg)

   The rear panel contains RF BNC connectors, a ground clamp and an external control interface connector that can be connected to the transceiver for more convenient control of the tuner.

[![](https://github.com/Dfinitski/ATU-10-10W-QRP-antenna-tuner/blob/main/Photos/tuner_2.jpg)](https://github.com/Dfinitski/ATU-10-10W-QRP-antenna-tuner/blob/main/Photos/tuner_2.jpg)

   The control button has only 3 functions - a short press resets the tuner and sets all relays to their initial state, in which all reactive elements are disabled and do not affect the signal flow through the tuner, a long press causes the tuner to enter the tuning mode and a very long press for more than 5 seconds causes the firmware version to be displayed on the display.
   > Note (DL8UG): this describes the original firmware. From FW 1.7 on, the short press toggles a true bypass and the very long press powers the tuner off, see *Operation* above.
   The display mainly shows the current transmitter power and SWR in the transmitter cable and sometimes briefly indicates the modes.
   
   The tuner is built on IM41 bistable relays, which means that a significant current is consumed by the tuner only for a short time during tuning; in rest mode, the tuner's relays retain their state for an arbitrarily long time, without consuming power.

   The built-in battery consists of two Li-Ion 14500 batteries connected in parallel. The best examples of these (SANYO) have a capacity of 800 mAh each, the worst ones are usually 400 mAh.
    The tuner has 3 power consumption modes - the operating mode, when the display indicates the parameters, lasts 5 minutes, after which the tuner falls asleep and the display turns off. You can wake him up with a short press on the button. After 30 minutes of sleep, the tuner turns off completely and you can turn it on by holding the button for a long time (more than 3 seconds).
    
   The current consumption in the operating mode is 12 mA, in the sleep mode 170 μA, in the off state 37 μA.
   
[![](https://github.com/Dfinitski/ATU-10-10W-QRP-antenna-tuner/blob/main/Photos/tuner_3.jpg)](https://github.com/Dfinitski/ATU-10-10W-QRP-antenna-tuner/blob/main/Photos/tuner_3.jpg)

   The main microprocessor of the tuner is PIC16F18877, another PIC16F1454 processor is used as an embedded programmer. It should be flashed once using the programmer with a special firmware, after which it will be possible to change the main firmware of the tuner as many times as necessary without resorting to special means. After connecting the tuner to the computer, a new logical disk with the appropriate name will appear in the explorer, in which you can find a link to the firmware repository. To flash the tuner, it is enough to copy the firmware file to this disk, the process takes a couple of seconds and the tuner is flashed at the same moment when the file copying is completed.
   
   There is also space for two dual-color LEDs with a common anode on the board, they can be installed if you do not need an OLED display.
The right LED indicates the battery charging process, the left LED indicates the operation. In operating mode, it blinks briefly in color every three seconds, depending on the battery charge level.

