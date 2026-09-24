#include "swr.h"

void swr_calc(float F, float R){
   volatile float gamma;
   //
   F /= 1000;  // to Volts
   R /= 1000;  // to Volts
   F = Cal_a * F * F + Cal_b * F;
   R = Cal_a * R * R + Cal_b * R;
   PWR = (int)(F * 10 + 0.5);         // 0 - 150 (0 - 15.0 Watts)
   //
   if(PWR<min_for_start)  SWR = 0;      // < 1W
   else if(R >= F) SWR = 999;
   else {
      gamma = sqrt_n(R / F);
      if((1.0-gamma) == 0) gamma = 0.001;
      gamma = (1.0 + gamma) / (1.0 - gamma);
      if(gamma<1.0)
         gamma = 1.0;
      if(gamma>9.985) SWR = 999;
      else SWR = (int)(gamma * 100 + 0.5);
   }
   //
   return;
}
//
float sqrt_n(float x){   // Thanks, Newton !
   // Starting at x/2 with a fixed number of steps did not converge for
   // small x (SWR 1.01 was shown as 1.03), so start at (1 + x) / 2, which
   // is close for 0 <= x <= 1, and iterate until the value settles.
   char i;
   float a, b;
   if(x <= 0) return 0;
   a = (1 + x) / 2;
   for(i = 0; i < 20; i++) {
      b = (a + x / a) / 2;
      if(a - b < a * 0.0001) return b;   // Newton approaches from above
      a = b;
   }
   return a;
}
