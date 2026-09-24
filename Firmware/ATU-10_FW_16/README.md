# ATU-10 FW 1.6 (N7DDC) – changing the Cells in the hex file

This applies to N7DDC's FW 1.6 hex only. From FW 1.7 (DL8UG, XC8 port) on, the Cells are set in
`Cells[]` in `main.c` and the firmware is rebuilt; the array has no fixed address in the hex any more.
The meaning of the cells is described in the [main README](../../README.md).

Original instructions by N7DDC (moved here from the main README):

Setting by Cells control implementation. There are 10 cells you can find opening a FW .hex file in Notepad++ program as Intel HEX file.
At the end by address EEE0 and EEF0 you can see them and change values. After changing any value you must to correct a checksumm for each
changed string. Copy the needed string into [Checsumm.html](../../Checksumm.html) and get correct checksumm. It should be green color before you can save changed .hex FW file.

[![](https://github.com/Dfinitski/ATU-10-10W-QRP-antenna-tuner/blob/main/Photos/Cells.jpg)](https://github.com/Dfinitski/ATU-10-10W-QRP-antenna-tuner/blob/main/Photos/Cells.jpg)
