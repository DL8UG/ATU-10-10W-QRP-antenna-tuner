# ATU-10 FW 1.6 – XC8 port (Linux)

Port of the mikroC PRO for PIC firmware from `../ATU-10_FW_16` to the free Microchip XC8 compiler.
The original sources are left unchanged. Status: 2026-09-24, branch `xc8-port`. **Not yet tested on the device.**

Commits (each can be reverted individually):
1. `1ff0658` pure port, behaves like N7DDC 1.6
2. `4000158` tune/SWR code moved to `tune.c`/`swr.c` plus PC simulator, no change in behavior
3. `73901f0` bug fixes: `sqrt_n` accuracy, relay state after `coarse_tune`
4. `d11dd69` tune measurement averages 8 F/R pairs
5. `5c304a1` tune algorithm: RFL metric, repeated fine search, coarse search up to 64
6. `980364b` coarse search with tolerance, antenna test series in the simulator (`make simants`)
7. `c54e38e` bypass by short press
8. `d2afd48`, `0e3f518`, `252b25b` display robustness: I2C bus recovery, periodic display reconfiguration, interrupt races (see below)

## Building
```
make          # -> ATU-10_FW_16_xc8.hex
make clean
```
- Compiler: XC8 v4.00 (AUR `microchip-mplabxc8-bin`, installed under `/opt/microchip/xc8/v4.00`)
- Device Family Pack: XC8 v4 no longer ships it, so it lives under
  `~/.local/share/microchip/packs/PIC16F1xxxx_DFP/1.32.471`
  (source: `https://packs.download.microchip.com/Microchip.PIC16F1xxxx_DFP.1.32.471.atpack`, unpacked).
  The `Makefile` finds it automatically, otherwise use `make DFP=/path`.
- Result: 11,408 of 32,768 program words (pure port: 10,251, mikroC: 9,074). Hardware stack according to the XC8 call graph: `main` 14 levels, 15 with the interrupt (limit 16). That is tight, so check after every change: `grep "Estimated maximum stack depth" build/ATU-10.lst`.
- Listing and map with stack information: `build/ATU-10.lst` and `build/ATU-10.map`

## Flashing
Same as with the original: connect the tuner via USB-C, copy the hex onto the USB drive, run `sync`.
The PIC16F1454 on the board acts as the programmer; its parser is closed source.
`tools/normalize_hex.py` therefore writes the hex in mikroC format:
- CRLF line endings, upper case
- only record types 00, 04 and 01, no 04 record at the start
- at most 16 bytes per record

The script also checks the config words against `ATU-10.cfg`, but only the bits the chip actually uses.
`normalize_hex.py --check FILE` checks the format of a file. The original FW 1.5 hex passes this check.

Fallback: flash `../ATU-10_FW_15/ATU-10_FW_15.hex` the same way. There is no FW 1.6 hex in the repo; the groups.io group ATU100 may have one.

## Operation
- **Short press**: bypass on or off.
  - On: the relays go to L=0/C=0, the display briefly shows "BYPASS", then "BYP = x.xx". The tuned setting is remembered, auto mode is paused.
  - Off: the remembered setting is restored without an immediate auto-tune.
  - The original did a "RESET" to L=0/C=1 here (22 pF stayed in parallel), and auto mode then started again right away.
- **Long press**: tune. Ends an active bypass.
- **Very long press** (approx. 2.5 s): power off.
- **Short press while tuning**: aborts the tune and switches to bypass.
- **External interface** (Icom): "Reset" only switches to bypass and never back, "Tune" tunes as before.

## Settings (Cells)
The values are stored BCD-coded and commented in the `Cells[]` array in `main.c`. They are changed in the code, no longer in the hex.
The array no longer has a fixed address (in the original it was `absolute 0x7770`).

Explanations (traced from the code):
- **Cell 4, minimum power** (`0x10` = 1.0 W, unit 0.1 W): from this power on, the SWR is calculated and displayed (below it "0.00"), and auto-tune triggers. Tuning only starts at power *above* the value (`PWR > min_for_start`).
  - 0.5 W works with `0x05`, but the measurement becomes less accurate: at 0.5 W the reflected detector delivers only about 10 mV at SWR 1.2 and about 44 mV at SWR 1.5, with an ADC resolution of 1 mV.
  - Simulator with 3 mV noise: practically unchanged for the EFHW; for the random wire the rate of SWR ≤ 1.5 drops from 71 % to 68 %. The real diodes deviate more from the calibration formula at such small voltages. So it is better to tune with 2–5 W.
  - 0 is not allowed, otherwise any noise already counts as a carrier.
- **Cell 6, auto-tune threshold** (`0x13`): auto-tune starts when all of these conditions hold:
  - The SWR is above 1.2.
  - It has changed by more than (value − 10) tenths since the last tune, i.e. by more than 0.3 for `0x13`. Alternatively an SWR above 9.69 is enough.
  - The power is within the window of cells 4 and 5.
  - No bypass is active.
  
  So the value is not an SWR limit of 1.3. After a reset or display wake-up the reference value is reset. Tuning itself ends at SWR ≤ 1.2 (`TUNE_GOOD_SWR`).

## What was changed
- **`mikroc_compat.h/.c`**: replacement for the mikroC libraries
  - Register bits `*_bit` → `REGbits.X`: the XC8 header defines these names too, but only for assembler, hence `#undef` and redefine.
  - `Delay_ms/us`, `VDelay_ms`
  - `IntToStr` (6 characters right-aligned plus `\0`) and `Bcd2Dec`
  - `ADC_Init`, `ADC_Init_Advanced`, `ADC_Get_Sample` for the ADC² module: FRC clock, right-justified result, reference FVR 1.024 V / 2.048 V or Vdd
- **Syntax translated**:
  - `iv 0x0004` → `__interrupt()`
  - `bit` → `__bit`
  - `sbit … at` → `#define`
  - `x.Bn` → `BIT(x,n)`
  - `asm NOP/sleep` → `NOP()/SLEEP()`
  - variable-length array in `sqrt_n` → fixed size
  - `static oled_addr` → `static char`
  - `code` removed from the font
  - `const char*` for the string parameters
- **Semantics fixed**: in C, `~PORTB.B5` is always true, so it is now `!` (affects `GetButton` and `Start`).
- **Bit reshuffling in the display** (`oled_wr_str`): new helper function `dbl_nibble()`. Tested on the PC against the original logic; all 256 values match.
- **Battery bar**: the original shifts by −1 bit, which is undefined in C. The new helper function `bar()` returns 0 for 0.
- **`volatile`** for all variables changed by the interrupt: Tick, counters, button flags.
- **Config bits**: the raw values from `ATU-10.cfg` are implemented as named `#pragma config`. XC8 sets the unused bits to 1, which has no effect.

## Improvements over N7DDC (SWR measurement and tuning)
Code: `swr.c` (calculation from the detector voltages) and `tune.c` (search). Parameters in `tune.h`:
`TUNE_GOOD_SWR` 120, `TUNE_AVG` 8, `SHARP_PASSES` 4, `COARSE_MAX` 64, `COARSE_TOL` 25 %, `COARSE_TOL_MAX` 200.

- **Bug in `sqrt_n`**: the start value x/2 with 8 fixed steps did not converge for small Γ. SWR 1.01 was shown as 1.03, 1.02 as 1.04. It now starts at (1+x)/2 and iterates until convergence.
- **Bug in `coarse_tune`**: the function chose the best variant but left the relays in the state of the last one tried. The following steps therefore started from the wrong state.
- **Measurement during tuning**: `get_pwr_avg(8)` averages 8 alternately measured F/R pairs. Before: minimum of up to 5 individual SWR values, noisy and too optimistic. Without a carrier the wait loop is just as fast, so the timeout does not change. The display and the peak detection in `watch_swr` still use the single measurement.
- **RFL metric**: the search minimizes Pr/Pf·10000 instead of the SWR. The SWR is capped at 9.99; above that the search had no gradient and got stuck on high-impedance loads.
- **Fine search**: up to 4 alternating C/L passes, first with ~10 % steps, then with single steps, until nothing changes. Before, there was only one pass.
- **Coarse search**: it also tries the largest relay (64), before only up to 32.
- **Tolerance in the coarse search**: the search continues as long as a step is at most 25 % worse, capped at 200 RFL (2 % of Pr/Pf). This way it gets over small bumps into the valley. The original achieved that as a side effect of the `SWR/10` quantization. Without the cap the search runs away at high SWR.
- **Dropped**: remembering the best measured setting and jumping back to it at the end. It made no difference in the simulator: in the worse cases the search never measures the good setting in the first place.

### Simulator
`make simcompare [TABLE=1] [NOISE=3] [SEEDS="1 2 3"]` compares the frozen original algorithm (`tools/sim/orig`) with the current version.
`build/sim_new --case 28.5 30 80` logs a single case step by step.

The model (`tools/sim/sim.c`) covers:
- relay values 0.1–10 µH and 22–2200 pF, Q=100, 10 pF stray capacitance, C on either the transmitter or the load side
- bridge according to the cal formula, ADC range switching as in `get_forward`
- 5 W transmit power, optional Gaussian noise
- test cases: 9 bands (1.85–28.5 MHz) × 14 loads (12.5 Ω–2 kΩ, plus complex ones)

Result for the standard set (`make simcompare`, 126 cases):

| | original | new |
|---|---|---|
| mean SWR (capped at 10) | 4.23 | 1.89 |
| SWR ≤ 1.2 | 31.0 % | 45.2 % |
| SWR ≤ 1.5 | 51.6 % | 64.3 % |
| relay steps avg / max | 66 / 113 | 89 / 235 (~25 ms per step) |
| better / worse | – | 72 / 4 |
| with 3 mV noise, 10 seeds: better / worse | – | 739 / 38 |

Typical antennas (`make simants`), impedances estimated, as seen behind the transformer:
- Random wire with 9:1 unun: 4+j10 to 350−j50 Ω, 10 loads × 9 bands
- EFHW (40 m) with 49:1: 25–150 Ω ±j20…100, 8 loads × 4 bands

| | random wire old | random wire new | EFHW old | EFHW new |
|---|---|---|---|---|
| mean SWR | 1.58 | 1.41 | 1.23 | 1.17 |
| SWR ≤ 1.2 | 26 % | 48 % | 66 % | 75 % |
| SWR ≤ 1.5 | 58 % | 70 % | 84 % | 94 % |
| with 3 mV noise: SWR ≤ 1.5 | 46 % | 71 % | 83 % | 94 % |
| relay steps avg | 50 | 80 | 39 | 53 |
| better / worse (without noise) | – | 33 / 10 | – | 7 / 1 |

Some cases remain worse than with the original, mainly loads with a large reactive part, for example:
- Random wire 24.9 MHz / 80+j150: 2.60 instead of 1.04
- Random wire 14.2 MHz / 15−j40: 3.25 instead of 2.41

The greedy search by design cannot find minima that lie off its path, and every change to the search order shifts which cases get lucky. A grid search would fix that, but costs considerably more relay steps.

## Display robustness (freezing, strange characters)
The original 1.6 also shows garbled characters now and then, or the display freezes. This happens mainly while transmitting/tuning and after the display wakes up. Causes in the code:

1. **The display is only configured at power-on and wake-up.** RF and relay pulses can corrupt a bit on the software I2C (open drain, approx. 20 kHz). The SSD1306 then takes data as a command: addressing mode, scroll, start line, remap. The error persists until the display is re-initialized.
2. **No bus recovery**: if the controller holds SDA low, all further transfers go nowhere.
3. **Writing to the switched-off display** (`OLED_PWD = 0`), for example the battery indicator every 3 s. The controller can be half powered via SDA/SCL and then starts without a clean reset on wake-up.
4. **Interrupt races**: `Tick`, `disp_cnt` and `off_cnt` are 32 bits wide and were not read atomically in `main`. The display or the whole device could switch off wrongly because of this.

Fixes:
- `Soft_I2C_Init()` clocks a hanging slave free with up to 9 SCL pulses.
- `oled_config()` resends all settings every 3 s. Since no `0xAE` is sent, nothing flickers.
- `oled_refresh()` does bus recovery, resends the settings and redraws all fields without clearing. This happens after every tune, every 30 s and immediately when the display does not answer with ACK (`oled_fault`).
- All `oled_*` output does nothing while the display is off.
- The interrupt reports counter expiry via the bits `Disp_expired`/`Off_expired`. `keep_awake()` and `tick()` access them with interrupts disabled.

Software can only do so much against very strong RF coupling. If the error still occurs, a ferrite or decoupling on the display lines helps on the hardware side. In any case the display repairs itself after 30 s at the latest.

## Open points / risks
1. **Test on the device still pending**. Test the pure port (`1ff0658`) first, then the improvements:
   - Splash screen "FW VERSION 1.6"
   - Battery indicator
   - Short, long and very long button press
   - Tuning into a dummy load
   - Power and SWR compared to the original
   - Power-off and wake-up via button (IOC on RB5)
   - then with the improvements: SWR into a 50 Ω dummy load (should now show ~1.0x instead of ~1.03), tuning into 2–3 mismatches (e.g. 25/100/200 Ω), comparing the reached SWR and tune time with the pure port
   - adjust the parameters in `tune.h` if needed, running the simulator first
   - Display: tune several times with 5–10 W into a mismatch, wait for the display timeout 20× and wake it up (for testing set Cell 1 = 0x01). Briefly pull SDA to GND during operation: the display must recover by itself within 30 s at the latest.
2. **Simulator vs. reality**: the model is idealized (no relay stray inductance, no frequency dependence of the bridge, noise estimated). The trend should be right, absolute values not necessarily.
3. **A/D converter**: the behavior of the mikroC library is not documented and was rebuilt from the datasheet. If the readings differ, look here first (FVR, reference, acquisition time 20 µs).
4. **USB flashing**: if the programmer does not accept the hex, disassemble `../../PIC16F1454_FW.hex` with `gpdasm` and find out what its parser expects.
5. The `-Wsign-conversion` and `& vs ==` warnings come from the original code and mean the same as under mikroC. They are intentionally left untouched.
