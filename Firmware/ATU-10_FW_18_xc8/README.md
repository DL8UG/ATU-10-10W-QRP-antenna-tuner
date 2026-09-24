# ATU-10 FW 1.8 – XC8 port (Linux)

Port of the mikroC PRO for PIC firmware from `../ATU-10_FW_16` to the free Microchip XC8 compiler.
FW 1.7 = N7DDC's FW 1.6 plus this port and the improvements listed below; FW 1.8 adds quick retune, memory of the relay setting, watchdog/brown-out and a clean tune abort.
The original sources are left unchanged. Status: 2026-09-24. **FW 1.7 is tested on the device by DL8UG, FW 1.8 not yet.**

## Acknowledgements
Many thanks to David Fainitski, N7DDC, the original developer of the ATU-10, for his work on
the hardware and the firmware, and for publishing it.

Since development of this project appeared to have stalled, and I ran into some problems with
my own tuner, I took the liberty of picking up his FW 1.6, porting it to Linux (free XC8
compiler instead of mikroC) and improving it.

Programming was assisted by Claude Code (Anthropic).

— DL8UG

## Feedback wanted
Feedback from the community is very welcome, especially test reports from other tuners,
antennas and bands, and readings that differ from FW 1.6 (power, SWR, tuning result, display).
Please open an issue at https://github.com/DL8UG/ATU-10-10W-QRP-antenna-tuner/issues
or post in the ATU100 group at https://groups.io/g/ATU100.
Useful details: band, rig and power, antenna and transformer, SWR before/after, and the Cells
settings if they were changed.

Commits (each can be reverted individually):
1. `1ff0658` pure port, behaves like N7DDC 1.6
2. `4000158` tune/SWR code moved to `tune.c`/`swr.c` plus PC simulator, no change in behavior
3. `73901f0` bug fixes: `sqrt_n` accuracy, relay state after `coarse_tune`
4. `d11dd69` tune measurement averages 8 F/R pairs
5. `5c304a1` tune algorithm: RFL metric, repeated fine search, coarse search up to 64
6. `980364b` coarse search with tolerance, antenna test series in the simulator (`make simants`)
7. `c54e38e` bypass by short press
8. `d2afd48`, `0e3f518`, `252b25b` display robustness: I2C bus recovery, periodic display reconfiguration, interrupt races (see below)
9. `2940851` tuning on 80 m no longer aborts showing "~20 W": the upper power limit checks Pf − Pr instead of Pf, plus a simulator with source impedance (see below)
10. `a7f7b1e` version bumped to 1.7, folder and hex renamed to `ATU-10_FW_17_xc8`
11. FW 1.8 started in the new folder `ATU-10_FW_18_xc8`; FW 1.7 stays unchanged in `ATU-10_FW_17_xc8`

## Building
```
make          # -> ATU-10_FW_18_xc8.hex
make clean
```
- Compiler: XC8 v4.00 (AUR `microchip-mplabxc8-bin`, installed under `/opt/microchip/xc8/v4.00`)
- Device Family Pack: XC8 v4 no longer ships it, so it lives under
  `~/.local/share/microchip/packs/PIC16F1xxxx_DFP/1.32.471`
  (source: `https://packs.download.microchip.com/Microchip.PIC16F1xxxx_DFP.1.32.471.atpack`, unpacked).
  The `Makefile` finds it automatically, otherwise use `make DFP=/path`.
- Result: 11,408 of 32,768 program words (pure port: 10,251, mikroC: 9,074). Hardware stack according to the XC8 call graph: `main` 12 levels, 13 with the interrupt (limit 16; FW 1.7: 14/15). Check it after every change: `grep "Estimated maximum stack depth" build/ATU-10.lst`.
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

Fallback: flash `../ATU-10_FW_17_xc8/ATU-10_FW_17_xc8.hex` (FW 1.7, tested on the device) or `../ATU-10_FW_15/ATU-10_FW_15.hex` the same way. There is no FW 1.6 hex in the repo; the groups.io group ATU100 may have one.

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
- **Abort showing ~20 W (80 m, random wire)**: the wait loop in `get_swr` waits as long as the power is outside cells 4…5. The upper limit checked the forward power Pf. But a QRP PA is not a 50 Ω source: at badly mismatched intermediate settings (on 80 m with large L/C) the reflection is reflected again at the TRX, and Pf rises up to the ADC limit (~19–20 W depending on battery voltage), even though the TRX only delivers 5 W. The loop then hung for ~20 s, showed the peak power and aborted tuning via timeout (`SWR = 0`). This also happened with the original, but the coarse search up to 64 made it much more frequent. The upper limit now checks `PWR_net` = Pf − Pr, i.e. the power the transmitter actually delivers. That is what matters for protecting the relays. The lower limit (carrier present) and the display stay with Pf.
- **Cancelling a tune with a short press** (FW 1.8): the original switched the relays to bypass from inside the measurement loop, but the search then went on and set them again, so the display showed BYP with tuned relays. Now the measurement and all search loops stop as soon as the button flag is set (`TUNE_ABORT`), and the main loop switches to bypass afterwards. Together with a leaner `pwr_wake()` this also frees 2 levels of the hardware stack.
- **Dropped**: remembering the best measured setting and jumping back to it at the end. It made no difference in the simulator: in the worse cases the search never measures the good setting in the first place.

### Simulator
`make simcompare [TABLE=1] [NOISE=3] [SEEDS="1 2 3"]` compares the frozen original algorithm (`tools/sim/orig`) with the current version.
`build/sim_new --case 28.5 30 80` logs a single case step by step.
`build/sim_new --sweep 3.6 --rs 10` computes a grid over the whole impedance plane (R 2–5000 Ω, X ±3000 Ω, 500 loads). Column 6 counts how often `get_swr` had to wait for "good power" and displayed the power, column 7 the highest power displayed.
`--rs` models the TRX as a source with internal resistance (5 W into 50 Ω). Pf then rises above 5 W on mismatch, as with a real rig. Without `--rs`, Pf stays at a constant 5 W and all other results are unchanged.

Grid at 3.6 MHz, aborts due to "too much power" before / after the Pf−Pr change: Rs 10 Ω: 334 → 0 of 500, Rs 5 Ω: 428 → 0 (original N7DDC: 26 and 14). With Rs = 10 Ω the rate of SWR ≤ 1.5 rises from 5.4 % to 7.4 % (only 157 of the 500 loads in the grid are tunable at all, i.e. 31 %). For extreme loads with SWR > 50 the measurement saturates and the search is initially blind there, with and without source impedance.

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


