# ATU-10 firmware 1.6, built with Microchip XC8
#   make        -> ATU-10_FW_16_xc8.hex (copy it onto the tuner's USB drive)
#   make clean

XC8    ?= $(firstword $(wildcard /opt/microchip/xc8/*/bin/xc8-cc) xc8-cc)
CPU     = 16LF18877
# Device Family Pack from https://packs.download.microchip.com (XC8 v3+ ships without it)
DFP    ?= $(lastword $(wildcard $(HOME)/.local/share/microchip/packs/PIC16F1xxxx_DFP/*))
TARGET  = ATU-10_FW_16_xc8
SRC     = main.c pic_init.c oled_control.c Soft_I2C.c mikroc_compat.c swr.c tune.c
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

.PHONY: all clean sim simcompare simants

# ---- PC simulator of the tuning algorithm (tools/sim), built with the host gcc
HOSTCC   = gcc
SIMFLAGS = -O2 -std=c99 -funsigned-char -D_DEFAULT_SOURCE
NOISE   ?= 3
SEEDS   ?= 1 2 3 4 5 6 7 8 9 10

build/sim_new: tools/sim/sim.c swr.c tune.c swr.h tune.h
	mkdir -p build
	$(HOSTCC) $(SIMFLAGS) -I. -o $@ tools/sim/sim.c swr.c tune.c -lm

# frozen copy of the original N7DDC algorithm for comparison
build/sim_orig: tools/sim/sim.c $(wildcard tools/sim/orig/*)
	mkdir -p build
	$(HOSTCC) $(SIMFLAGS) -Itools/sim/orig -o $@ tools/sim/sim.c tools/sim/orig/swr.c tools/sim/orig/tune.c -lm

sim: build/sim_new build/sim_orig

simcompare: sim
	build/sim_orig > build/orig.tsv
	build/sim_new > build/new.tsv
	for s in $(SEEDS); do build/sim_orig --noise $(NOISE) --seed $$s; done > build/orig_noise.tsv
	for s in $(SEEDS); do build/sim_new --noise $(NOISE) --seed $$s; done > build/new_noise.tsv
	@echo "== without noise"
	@python3 tools/sim/compare.py build/orig.tsv build/new.tsv $(if $(TABLE),--table)
	@echo "== with $(NOISE) mV noise, seeds $(SEEDS)"
	@python3 tools/sim/compare.py build/orig_noise.tsv build/new_noise.tsv $(if $(TABLE),--table)

# typical antennas: random wire with 9:1 unun, EFHW with 49:1
simants: sim
	tools/sim/antennas.sh build/sim_orig build/ant_orig
	tools/sim/antennas.sh build/sim_new build/ant_new
	tools/sim/antennas.sh build/sim_orig build/ant_orig_noise $(NOISE) "$(SEEDS)"
	tools/sim/antennas.sh build/sim_new build/ant_new_noise $(NOISE) "$(SEEDS)"
	@for a in rw ef; do \
	   echo "== $$a without noise"; python3 tools/sim/compare.py build/ant_orig_$$a.tsv build/ant_new_$$a.tsv $(if $(TABLE),--table); \
	   echo "== $$a with $(NOISE) mV noise"; python3 tools/sim/compare.py build/ant_orig_noise_$$a.tsv build/ant_new_noise_$$a.tsv; \
	done
