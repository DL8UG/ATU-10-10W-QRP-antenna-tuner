#include "tune.h"

static char sharp_fine;   // fine search with single steps

// How much worse a coarse step may be and the search still goes on: a few
// percent let it cross small bumps on the way into the valley, the limit
// keeps it from drifting off at high SWR
static int coarse_tol(int rfl){
   long t = (long)rfl * COARSE_TOL / 100;
   return t > COARSE_TOL_MAX ? COARSE_TOL_MAX : (int)t;
}

void atu_reset(){
   ind = 0;
   cap = 1;
   SW = 0;
   Relay_set(ind, cap, SW);
   return;
}
//
void get_swr(){
   unsigned int pwr_cnt = 150, tuneoff_cnt = 300;
   unsigned int PWR_max = 0;
   PWR = 0;
   SWR = 0;
   PWR_max = 0;
   //
   // The upper limit protects the relays from a too strong transmitter, so
   // it checks the delivered power: the forward power alone goes up to the
   // ADC limit (~20 W) at bad settings on low bands even with 5 W
   while(PWR<min_for_start || PWR_net>max_for_start){   // waiting for good power
      CLRWDT();   // tuning may wait up to ~20 s for the carrier
      if(TUNE_ABORT){
         SWR = 0;
         RFL = 10000;
         break;
      }
      //
      // Quick single measurement to see whether a carrier is there. Only then
      // average several F/R pairs: the minimum of up to 5 single SWR values
      // used before was noisy and too optimistic. Without carrier the loop
      // takes as long as before, which keeps the timeout below unchanged.
      get_pwr();
      if(PWR>min_for_start & PWR_net<max_for_start)
         get_pwr_avg(TUNE_AVG);
      else {
         Delay_us(500);
         get_pwr();
      }
      //
      if(PWR>min_for_start & PWR_net<max_for_start)
         break;
      //
      if(pwr_cnt>0){
          pwr_cnt --;
          if(PWR>PWR_max)
              PWR_max = PWR;
      }
      else {
         if(PWR_max!=PWR_fixed_old) draw_power(PWR_max);
         PWR_fixed_old = PWR_max;
         PWR_max = 0;
         pwr_cnt = 50;
         if(tuneoff_cnt>0) tuneoff_cnt--;
         else { SWR = 0; break; }
      }
   }
   //  good power
   return;
}
//
void tune(void){
   int RFL_mem;
   char cap_mem, ind_mem;
   //
   get_swr();
   if(SWR<=TUNE_GOOD_SWR) return;
   subtune();
   get_swr();
   if(SWR<=TUNE_GOOD_SWR) return;
   RFL_mem = RFL;
   cap_mem = cap;
   ind_mem = ind;
   if(SW==1) SW = 0;
   else SW = 1;
   subtune();
   get_swr();
   if(RFL>RFL_mem){
      if(SW==1) SW = 0;
      else SW = 1;
      cap = cap_mem;
      ind = ind_mem;
      Relay_set(ind, cap, SW);
      get_swr();
   }
   if(SWR<=TUNE_GOOD_SWR) return;
   sharp_tune();
   get_swr();
   if(SWR==999){   // no match found: true bypass (N7DDC: atu_reset, C = 22 pF)
      ind = 0;
      cap = 0;
      SW = 0;
      Relay_set(ind, cap, SW);
   }
   return;
}
//
void subtune(void){
   cap = 0;
   ind = 0;
   Relay_set(ind, cap, SW);
   get_swr();
   if(SWR<=TUNE_GOOD_SWR) return;
   coarse_tune();
   get_swr();
   if(SWR<=TUNE_GOOD_SWR) return;
   sharp_tune();
   return;
}
//
void coarse_tune(void){
   int RFL_mem1 = NOT_TRIED, RFL_mem2 = NOT_TRIED, RFL_mem3 = NOT_TRIED;
   char ind_mem1, cap_mem1, ind_mem2, cap_mem2, ind_mem3, cap_mem3;
   coarse_cap();
   coarse_ind();
   get_swr();
   if(SWR<=TUNE_GOOD_SWR) return;
   RFL_mem1 = RFL;
   ind_mem1 = ind;
   cap_mem1 = cap;
   if(cap<=2 & ind<=2){
      cap = 0;
      ind = 0;
      Relay_set(ind, cap, SW);
      coarse_ind();
      coarse_cap();
      get_swr();
      if(SWR<=TUNE_GOOD_SWR) return;
      RFL_mem2 = RFL;
      ind_mem2 = ind;
      cap_mem2 = cap;
   }
   if(cap<=2 & ind<=2){
      cap = 0;
      ind = 0;
      Relay_set(ind, cap, SW);
      coarse_ind_cap();
      get_swr();
      if(SWR<=TUNE_GOOD_SWR) return;
      RFL_mem3 = RFL;
      ind_mem3 = ind;
      cap_mem3 = cap;
   }
   if(RFL_mem1<=RFL_mem2 & RFL_mem1<=RFL_mem3){
      cap = cap_mem1;
      ind = ind_mem1;
   }
   else if(RFL_mem2<=RFL_mem1 & RFL_mem2<=RFL_mem3){
      cap = cap_mem2;
      ind = ind_mem2;
   }
   else if(RFL_mem3<=RFL_mem1 & RFL_mem3<=RFL_mem2){
      cap = cap_mem3;
      ind = ind_mem3;
   }
   // the relays still hold the last variant tried, switch to the best one
   if(RFL_mem2!=NOT_TRIED || RFL_mem3!=NOT_TRIED)
      Relay_set(ind, cap, SW);
   return;
}
//
void coarse_ind_cap(void){
   int RFL_mem;
   char ind_mem;
   ind_mem = 0;
   get_swr();
   RFL_mem = RFL;
   for(ind=1; ind<=COARSE_MAX && !TUNE_ABORT; ind*=2){
      Relay_set(ind, ind, SW);
      get_swr();
      if(RFL <= RFL_mem + coarse_tol(RFL_mem)){
         ind_mem = ind;
         RFL_mem = RFL;
      }
      else
         break;
   }
   ind = ind_mem;
   cap = ind_mem;
   Relay_set(ind, cap, SW);
   return;
}
//
void coarse_cap(void){
   int RFL_mem;
   char cap_mem;
   cap_mem = 0;
   get_swr();
   RFL_mem = RFL;
   for(cap=1; cap<=COARSE_MAX && !TUNE_ABORT; cap*=2){
      Relay_set(ind, cap, SW);
      get_swr();
      if(RFL <= RFL_mem + coarse_tol(RFL_mem)){
         cap_mem = cap;
         RFL_mem = RFL;
      }
      else
         break;
   }
   cap = cap_mem;
   Relay_set(ind, cap, SW);
   return;
}
//
void coarse_ind(void){
   int RFL_mem;
   char ind_mem;
   ind_mem = 0;
   get_swr();
   RFL_mem = RFL;
   for(ind=1; ind<=COARSE_MAX && !TUNE_ABORT; ind*=2){
      Relay_set(ind, cap, SW);
      get_swr();
      if(RFL <= RFL_mem + coarse_tol(RFL_mem)){
         ind_mem = ind;
         RFL_mem = RFL;
      }
      else
         break;
   }
   ind = ind_mem;
   Relay_set(ind, cap, SW);
   return;
}
//
void sharp_tune(void){
   // L and C interact, so one pass over each often stops short of the
   // optimum: repeat until a pass changes nothing, first with steps of
   // about 10 %, then with single steps
   char pass, cap_start, ind_start;
   sharp_fine = 0;
   for(pass=0; pass<SHARP_PASSES && !TUNE_ABORT; pass++){
      cap_start = cap;
      ind_start = ind;
      if(cap>=ind){
         sharp_cap();
         sharp_ind();
      }
      else{
         sharp_ind();
         sharp_cap();
      }
      if(cap==cap_start && ind==ind_start){
         if(sharp_fine) break;
         sharp_fine = 1;   // no gain with steps of 10 %: go on with single steps
      }
   }
   sharp_fine = 0;
   return;
}
//
void sharp_cap(void){
   int RFL_mem;
   char step, cap_mem;
   cap_mem = cap;
   step = sharp_fine ? 1 : cap / 10;
   if(step==0) step = 1;
   get_swr();
   RFL_mem = RFL;
   cap += step;
   Relay_set(ind, cap, SW);
   get_swr();
   if(RFL<=RFL_mem){
      RFL_mem = RFL;
      cap_mem = cap;
      for(cap+=step; cap<=(127-step) && !TUNE_ABORT; cap+=step){
         Relay_set(ind, cap, SW);
         get_swr();
         if(RFL<=RFL_mem){
            cap_mem = cap;
            RFL_mem = RFL;
            step = sharp_fine ? 1 : cap / 10;
            if(step==0) step = 1;
         }
         else
            break;
      }
   }
   else{
      RFL_mem = RFL;
      for(cap-=step; cap>=step && !TUNE_ABORT; cap-=step){
         Relay_set(ind, cap, SW);
         get_swr();
         if(RFL<=RFL_mem){
            cap_mem = cap;
            RFL_mem = RFL;
            step = sharp_fine ? 1 : cap / 10;
            if(step==0) step = 1;
         }
         else
            break;
      }
   }
   cap = cap_mem;
   Relay_set(ind, cap, SW);
   return;
}
//
void sharp_ind(void){
   int RFL_mem;
   char step, ind_mem;
   ind_mem = ind;
   step = sharp_fine ? 1 : ind / 10;
   if(step==0) step = 1;
   get_swr();
   RFL_mem = RFL;
   ind += step;
   Relay_set(ind, cap, SW);
   get_swr();
   if(RFL<=RFL_mem){
      RFL_mem = RFL;
      ind_mem = ind;
      for(ind+=step; ind<=(127-step) && !TUNE_ABORT; ind+=step){
         Relay_set(ind, cap, SW);
         get_swr();
         if(RFL<=RFL_mem){
            ind_mem = ind;
            RFL_mem = RFL;
            step = sharp_fine ? 1 : ind / 10;
            if(step==0) step = 1;
         }
         else
            break;
      }
   }
   else{
      RFL_mem = RFL;
      for(ind-=step; ind>=step && !TUNE_ABORT; ind-=step){
         Relay_set(ind, cap, SW);
         get_swr();
         if(RFL<=RFL_mem){
            ind_mem = ind;
            RFL_mem = RFL;
            step = sharp_fine ? 1 : ind / 10;
            if(step==0) step = 1;
         }
         else
            break;
      }
   }
   ind = ind_mem;
   Relay_set(ind, cap, SW);
   return;
}
//