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
extern int PWR, SWR, PWR_fixed_old, min_for_start, max_for_start;
extern volatile __bit B_short, B_xlong;
void Relay_set(char, char, char);
void get_pwr(void);
void Btn_short(void);
void draw_power(unsigned int);

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
