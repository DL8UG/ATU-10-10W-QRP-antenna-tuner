// PC simulator for the ATU-10 tuning algorithm.
//
// Links the firmware's tune.c and swr.c against a model of the L network
// and the SWR bridge, runs tune() for a set of loads and bands and prints
// one tab separated line per case:
//   freq_MHz  load  swr_reached  swr_best  relay_steps
// swr_reached is the true SWR of the relays as actually set, swr_best the
// best SWR of all 2 x 128 x 128 settings (brute force).
//
// Usage: sim [--noise mV] [--seed n] [--case MHz R X]
//   --case runs a single load and traces every relay step to stderr

#include <complex.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tune.h"
#include "swr.h"

// ---- globals the firmware code expects (defined in main.c on the PIC)
char ind, cap, SW;
int PWR, SWR, PWR_fixed_old, min_for_start = 10, max_for_start = 150;
volatile __bit B_short, B_xlong;
float Cal_a = 1.14f, Cal_b = 0.4f;            // default Cells 8 and 9

// ---- model
static const double L_uH[7] = {0.1, 0.22, 0.45, 1.0, 2.2, 4.5, 10.0};
static const double C_pF[7] = {22, 47, 100, 220, 470, 1000, 2200};
#define Q_L        100.0     // coil quality factor
#define C_STRAY    10.0      // pF, always in parallel to the capacitor bank
#define P_TX       5.0       // W forward power
#define VDD_MV     3900.0    // battery voltage for the ADC Vdd range

static double freq;          // Hz
static double complex Z_load;
static char r_ind, r_cap, r_sw;   // relays as actually switched
static long relay_steps;
static double noise_mv;
static int trace;

static double complex par(double complex a, double complex b) {
   return a * b / (a + b);
}

static double gamma_of(int l, int c, int sw) {
   double w = 2 * M_PI * freq, L = 0, C = C_STRAY;
   for(int i = 0; i < 7; i++) {
      if(l & (1 << i)) L += L_uH[i];
      if(c & (1 << i)) C += C_pF[i];
   }
   double complex zl = w * L * 1e-6 * (1.0 / Q_L + I);
   double complex zc = 1.0 / (I * w * C * 1e-12);
   double complex zin = sw ? par(zc, zl + Z_load) : zl + par(Z_load, zc);
   return cabs((zin - 50) / (zin + 50));
}

static double swr_of(double g) {
   return g >= 0.999 ? 999 : (1 + g) / (1 - g);
}

static double gauss(void) {
   double u = (rand() + 1.0) / (RAND_MAX + 2.0), v = (rand() + 1.0) / (RAND_MAX + 2.0);
   return sqrt(-2 * log(u)) * cos(2 * M_PI * v);
}

// detector voltage for power P, inverse of P = a*V^2 + b*V
static double det_mv(double p) {
   return (-Cal_b + sqrt(Cal_b * Cal_b + 4 * Cal_a * p)) / (2 * Cal_a) * 1000;
}

// ADC reading as get_forward()/get_reverse() return it, incl. range switching
static int adc_mv(double mv) {
   long v;
   mv += noise_mv * gauss();
   if(mv < 0) mv = 0;
   v = lround(mv);                                   // FVR 1.024 V, 1 mV/LSB
   if(v > 1023) v = 1023;
   if(v == 1023) {
      v = lround(mv / 2); if(v > 1023) v = 1023;     // FVR 2.048 V
      v *= 2;
   }
   if(v == 2046) {
      v = lround(mv * 1024 / VDD_MV); if(v > 1023) v = 1023;   // Vdd
      v = (long)(v * VDD_MV / 1024);
   }
   return (int)v;
}

// ---- hardware functions the firmware code calls
void Relay_set(char l, char c, char sw) {
   r_ind = l & 0x7F; r_cap = c & 0x7F; r_sw = sw;
   relay_steps++;
   if(trace)
      fprintf(stderr, "%4ld  SW=%d L=%3d C=%3d  SWR %.3f\n", relay_steps, r_sw, r_ind, r_cap,
              swr_of(gamma_of(r_ind, r_cap, r_sw)));
}

static void measure(int *f, int *r) {
   double g = gamma_of(r_ind, r_cap, r_sw);
   *f = adc_mv(det_mv(P_TX));
   *r = adc_mv(det_mv(P_TX * g * g));
}

void get_pwr(void) {
   int f, r;
   measure(&f, &r);
   swr_calc(f, r);
}

void get_pwr_avg(char n) {
   long fs = 0, rs = 0;
   for(int i = 0; i < n; i++) {
      int f, r;
      measure(&f, &r);
      fs += f; rs += r;
   }
   swr_calc((float)fs / n, (float)rs / n);
}

void Btn_short(void) {}
void draw_power(unsigned int p) { (void)p; }

// ---- test cases
static double bands[] = {1.85, 3.6, 7.1, 10.12, 14.2, 18.1, 21.2, 24.9, 28.5};
static double complex loads[] = {
   12.5, 25, 50, 100, 200, 450, 1000, 2000,
   10 - 100 * I, 30 + 80 * I, 300 + 300 * I, 2000 - 500 * I, 5 - 20 * I, 150 - 150 * I,
};

int main(int argc, char **argv) {
   unsigned seed = 1;
   size_t nbands = sizeof bands / sizeof *bands, nloads = sizeof loads / sizeof *loads;
   for(int i = 1; i < argc; i++) {
      if(!strcmp(argv[i], "--noise") && i + 1 < argc) noise_mv = atof(argv[++i]);
      else if(!strcmp(argv[i], "--seed") && i + 1 < argc) seed = atoi(argv[++i]);
      else if(!strcmp(argv[i], "--case") && i + 3 < argc) {
         bands[0] = atof(argv[i + 1]);
         loads[0] = atof(argv[i + 2]) + atof(argv[i + 3]) * I;
         nbands = nloads = 1;
         trace = 1;
         i += 3;
      }
      else { fprintf(stderr, "usage: %s [--noise mV] [--seed n] [--case MHz R X]\n", argv[0]); return 2; }
   }
   srand(seed);
   for(size_t b = 0; b < nbands; b++) {
      for(size_t l = 0; l < nloads; l++) {
         double best = 1;
         freq = bands[b] * 1e6;
         Z_load = loads[l];
         for(int sw = 0; sw < 2; sw++)
            for(int li = 0; li < 128; li++)
               for(int ci = 0; ci < 128; ci++) {
                  double g = gamma_of(li, ci, sw);
                  if(g < best) best = g;
               }
         atu_reset();
         relay_steps = 0;
         tune();
         printf("%.2f\t%g%+gj\t%.3f\t%.3f\t%ld\n", bands[b], creal(Z_load), cimag(Z_load),
                swr_of(gamma_of(r_ind, r_cap, r_sw)), swr_of(best), relay_steps);
      }
   }
   return 0;
}
