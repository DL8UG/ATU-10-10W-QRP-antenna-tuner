# ATU-10 FW 1.6 – XC8 port (Linux)

Port of the mikroC PRO for PIC firmware from `../ATU-10_FW_16` to the free Microchip XC8 compiler.
The original sources are left unchanged. Status: 2026-09-23. **Not yet tested on the device.**

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
- Result: 10,251 of 32,768 program words (mikroC: 9,074). Hardware stack: `main` needs 9 levels, the interrupt only a few (limit 16).
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

## Settings (Cells)
The values are stored BCD-coded and commented in the `Cells[]` array in `main.c`. They are changed in the code, no longer in the hex.
The array no longer has a fixed address (in the original it was `absolute 0x7770`).

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

## Open points / risks
1. **Test on the device still pending**:
   - Splash screen "FW VERSION 1.6"
   - Battery indicator
   - Short, long and very long button press
   - Tuning into a dummy load
   - Power and SWR compared to the original
   - Power-off and wake-up via button (IOC on RB5)
2. **A/D converter**: the behavior of the mikroC library is not documented and was rebuilt from the datasheet. If the readings differ, look here first (FVR, reference, acquisition time 20 µs).
3. **USB flashing**: if the programmer does not accept the hex, disassemble `../../PIC16F1454_FW.hex` with `gpdasm` and find out what its parser expects.
4. The `-Wsign-conversion` and `& vs ==` warnings come from the original code and mean the same as under mikroC. They are intentionally left untouched.
