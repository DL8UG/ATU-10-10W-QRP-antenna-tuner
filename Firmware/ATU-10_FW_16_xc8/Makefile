# ATU-10 firmware 1.6, built with Microchip XC8
#   make        -> ATU-10_FW_16_xc8.hex (copy it onto the tuner's USB drive)
#   make clean

XC8    ?= $(firstword $(wildcard /opt/microchip/xc8/*/bin/xc8-cc) xc8-cc)
CPU     = 16LF18877
# Device Family Pack from https://packs.download.microchip.com (XC8 v3+ ships without it)
DFP    ?= $(lastword $(wildcard $(HOME)/.local/share/microchip/packs/PIC16F1xxxx_DFP/*))
TARGET  = ATU-10_FW_16_xc8
SRC     = main.c pic_init.c oled_control.c Soft_I2C.c mikroc_compat.c
# must match the #pragma config values in pic_init.c
CONFIG  = 2904,3E21,3F1F,3003,0003

# C99 mode: float and double are 32 bit like in mikroC
CFLAGS  = -mcpu=$(CPU) -mdfp=$(DFP)/xc8 -std=c99 -O2

all: $(TARGET).hex

build/ATU-10.hex: $(SRC) $(wildcard *.h)
	mkdir -p build
	$(XC8) $(CFLAGS) -Wa,-a -Wl,-Map=build/ATU-10.map -o build/ATU-10.elf $(SRC)

$(TARGET).hex: build/ATU-10.hex tools/normalize_hex.py
	python3 tools/normalize_hex.py $< $@ --config $(CONFIG)

clean:
	rm -rf build $(TARGET).hex

.PHONY: all clean
