// Tuning algorithm: searches the relay setting with the lowest SWR.
// Free of hardware access so it also builds on a PC (tools/sim).

#ifndef TUNE_H
#define TUNE_H

#ifdef __XC8
#include "mikroc_compat.h"       // Delay_us
#else
#define __bit unsigned char
#define Delay_us(x)
#define CLRWDT()
#endif

// provided by main.c (or the simulator)
extern char ind, cap, SW;
extern int PWR, SWR, RFL, PWR_net, PWR_fixed_old, min_for_start, max_for_start;
extern volatile __bit B_short, B_xlong;
void Relay_set(char, char, char);
void get_pwr(void);
void get_pwr_avg(char n);          // n averaged F/R pairs
void draw_power(unsigned int);

#define TUNE_GOOD_SWR 120          // stop tuning at SWR 1.20 or better
#define TUNE_AVG 8                 // F/R pairs averaged per measurement while tuning
#define SHARP_PASSES 4             // max. passes of the fine search
#define COARSE_MAX 64              // largest relay tried by the coarse search (N7DDC: 32)
#define COARSE_TOL 25              // % a coarse step may be worse and the search still goes on,
#define COARSE_TOL_MAX 200         //   but at most this much RFL (2 % of Pr/Pf)
#define NOT_TRIED 32767
// Quick retune: when the relays hold the result of an earlier tune and the
// SWR is at most QUICK_MAX_SWR (x100), a fine search from there comes first.
// Its result is kept if it is at most QUICK_MARGIN worse than the earlier
// tune reached, otherwise the full search follows. QUICK_MAX_SWR 0 = off.
#ifndef QUICK_MAX_SWR
#define QUICK_MAX_SWR 500
#endif
#ifndef QUICK_MARGIN
#define QUICK_MARGIN 20
#endif
extern int tune_last;              // SWR the relays were tuned to, 0 = none
// after tune(): SWR 0 = cancelled, 999 = no match (true bypass)
#define TUNE_RESULT (SWR>0 && SWR<999 ? SWR : 0)
// A button press cancels the tuning: the search loops stop right away and
// the main loop handles the press afterwards (bypass or power off), the
// flags stay set until then. Calling Btn_short() from get_swr() used to
// switch the relays to bypass while the search went on and set them again,
// so the display showed BYP with tuned relays.
#define TUNE_ABORT (B_short || B_xlong)

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
