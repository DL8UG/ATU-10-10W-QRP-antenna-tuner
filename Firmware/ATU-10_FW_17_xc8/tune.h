// Tuning algorithm: searches the relay setting with the lowest SWR.
// Free of hardware access so it also builds on a PC (tools/sim).

#ifndef TUNE_H
#define TUNE_H

#ifdef __XC8
#include "mikroc_compat.h"       // Delay_us
#else
#define __bit unsigned char
#define Delay_us(x)
#endif

// provided by main.c (or the simulator)
extern char ind, cap, SW;
extern int PWR, SWR, RFL, PWR_net, PWR_fixed_old, min_for_start, max_for_start;
extern volatile __bit B_short, B_xlong;
void Relay_set(char, char, char);
void get_pwr(void);
void get_pwr_avg(char n);          // n averaged F/R pairs
void Btn_short(void);
void draw_power(unsigned int);

#define TUNE_GOOD_SWR 120          // stop tuning at SWR 1.20 or better
#define TUNE_AVG 8                 // F/R pairs averaged per measurement while tuning
#define SHARP_PASSES 4             // max. passes of the fine search
#define COARSE_MAX 64              // largest relay tried by the coarse search (N7DDC: 32)
#define COARSE_TOL 25              // % a coarse step may be worse and the search still goes on,
#define COARSE_TOL_MAX 200         //   but at most this much RFL (2 % of Pr/Pf)
#define NOT_TRIED 32767

void atu_reset(void);
void get_swr(void);
void tune(void);
void subtune(void);
void coarse_tune(void);
void coarse_ind_cap(void);
void coarse_cap(void);
void coarse_ind(void);
void sharp_tune(void);
void sharp_cap(void);
void sharp_ind(void);

#endif
