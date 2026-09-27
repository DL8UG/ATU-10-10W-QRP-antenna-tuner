// Power and SWR calculation from the detector voltages.
// Free of hardware access so it also builds on a PC (tools/sim).

#ifndef SWR_H
#define SWR_H

extern int PWR, SWR, min_for_start;
extern int RFL;           // reflected / forward power * 10000, not capped like SWR
extern int PWR_net;       // forward - reverse power, what the transmitter delivers
extern float Cal_a, Cal_b;

void swr_calc(float F, float R);   // forward and reverse detector voltage in mV
float sqrt_n(float);

#endif
