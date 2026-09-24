#!/bin/bash
# Runs a simulator binary on typical antenna loads (as seen behind the unun):
#   rw: random wire with 9:1 unun, all bands
#   ef: EFHW (40 m) with 49:1 transformer, 40/20/15/10 m
# Usage: antennas.sh SIM_BINARY OUTPREFIX [NOISE_mV] [SEEDS]
# writes OUTPREFIX_rw.tsv and OUTPREFIX_ef.tsv
BIN=$1; P=$2; NOISE=${3:-0}; SEEDS=${4:-1}
RW="4 10 8 -25 15 -40 25 60 40 -120 80 150 150 -200 250 100 350 -50 12 5"   # R X pairs
EF="40 -20 55 30 70 -60 35 45 90 80 120 -100 25 -35 150 40"
suite() {   # loads, bands in $BANDS
   local loads=$1
   for s in $SEEDS; do
      set -- $loads
      while [ $# -gt 0 ]; do
         for f in $BANDS; do "$BIN" --case "$f" "$1" "$2" --noise "$NOISE" --seed "$s" 2>/dev/null; done
         shift 2
      done
   done
}
BANDS="1.85 3.6 7.1 10.12 14.2 18.1 21.2 24.9 28.5" suite "$RW" > "${P}_rw.tsv"
BANDS="7.1 14.2 21.2 28.5" suite "$EF" > "${P}_ef.tsv"
