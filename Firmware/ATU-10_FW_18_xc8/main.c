#include "mikroc_compat.h"
// David Fainitski, N7DDC
// 2020
// XC8 port and improvements (FW 1.7, 1.8): DL8UG, 2026, assisted by Claude Code

#include "pic_init.h"
#include "main.h"
#include "oled_control.h"
#include "Soft_I2C.h"
#include "swr.h"
#include "tune.h"


// global variables
char txt[8], txt_2[8];
volatile unsigned long Tick = 0; // ms system tick
int Voltage, Voltage_old = 0;
volatile char btn_1_cnt = 0, btn_2_cnt = 0;
unsigned long volt_cnt = 0, watch_cnt = 0, btn_cnt = 0, refresh_cnt = 0;
volatile unsigned long off_cnt = 10, disp_cnt=10;
int PWR, SWR, SWR_ind = 0, SWR_fixed_old = 100, PWR_fixed_old = 9999, rldl;
char ind = 0, cap = 0, SW = 0;
__bit Overflow, gre;
volatile __bit B_short, B_long, B_xlong, E_short, E_long;
// set by the interrupt when disp_cnt / off_cnt run out: main cannot read
// the 32 bit counters atomically, a bit it can
volatile __bit Disp_expired, Off_expired;
// Bypass: short press toggles, the tuned setting is kept to switch back
__bit Bypass;
char byp_ind, byp_cap, byp_SW;
int byp_swr_old;

// depending on Cells
unsigned long Disp_time, Off_time;
int Rel_Del, min_for_start, max_for_start, Auto_delta, Peak_cnt;
__bit Auto;
float Cal_a, Cal_b;
// Cells (BCD coded settings)
const char Cells[10] = {
   0x05,   // 1) time to display off in minutes, 0 = always on
   0x30,   // 2) time to power off in minutes, 0 = always on
   0x07,   // 3) relay delay time in ms
   0x10,   // 4) min power to start tuning in tenths of a Watt, must not be 0
   0x15,   // 5) max power to start tuning in Watts
   0x13,   // 6) auto tuning starts when the SWR is above 1.2 and has changed since the
           //    last tune by more than (value - 10) tenths: 13 = change of more than 0.3
   0x01,   // 7) auto mode: 1 = on, 0 = off
   0x04,   // 8) calibration coefficient for 1 W (4 for BAT41 diodes)
   0x14,   // 9) calibration coefficient for 10 W (14 for BAT41 diodes)
   0x60    // 10) peak detector time for power measurement in tens of ms
};

#define FW_VER "1.8"

static void oled_labels(void);

static unsigned long tick(void){   // Tick read atomically
   unsigned long t;
   GIE_bit = 0;
   t = Tick;
   GIE_bit = 1;
   return t;
}
//
static void keep_awake(void){   // restart display and power off timers
   GIE_bit = 0;
   disp_cnt = Disp_time;
   off_cnt = Off_time;
   Disp_expired = 0;
   Off_expired = 0;
   GIE_bit = 1;
   return;
}
//
// interrupt processing
void __interrupt() interupt(void) {
   //
   if(TMR0IF_bit) {   // Timer0   every 1ms
      TMR0IF_bit = 0;
      Tick++;
      if(disp_cnt!=0 && --disp_cnt==0) Disp_expired = 1;
      if(off_cnt!=0 && --off_cnt==0) Off_expired = 1;
      TMR0L = 0xC0;   // 8_000 cycles to OF
      TMR0H = 0xE0;
      //
      if(Tick>=btn_cnt){  // every 10ms
         btn_cnt += 10;
         //
         if(GetButton | Start){
            disp_cnt = Disp_time;
            off_cnt = Off_time;
            Disp_expired = 0;
            Off_expired = 0;
         }
         //
         if(GetButton){  //
            if(btn_1_cnt<250) btn_1_cnt++;
            if(btn_1_cnt==25) B_long = 1;  // long pressing detected
            if(btn_1_cnt==250 & OLED_PWD) B_xlong = 1;  // Xtra long pressing detected
         }
         else if(btn_1_cnt>2 & btn_1_cnt<25){
            B_short = 1;               // short pressing detected
            btn_1_cnt = 0;
         }
         else
            btn_1_cnt = 0;
         //  External interface
         if(Start){
            if(btn_2_cnt<25) btn_2_cnt++;
            if(btn_2_cnt==20 & Key_in) E_long = 1;
         }
         else if(btn_2_cnt>1 & btn_2_cnt<10){
            E_short = 1;
            btn_2_cnt = 0;
         }
         else
            btn_2_cnt = 0;
      }
   }
   return;
}


void main(void) {
   pic_init();
   cells_reading();
   Red = 1;
   Key_out = 1;
   gre = 1;
   oled_start();
   //if(Debug) check_reset_flags();
   ADC_Init();
   Overflow = 0;
   //
   keep_awake();
   //
   //Relay_set(0, 0, 0);
   //
   while(1) {
      if(tick()>=volt_cnt){   // every 3 second
         volt_cnt += 3000;
         Voltage_show();
      }
      //
      if(tick()>=watch_cnt){   // every 300 ms    unless power off
         watch_cnt += 300;
         watch_swr();
         if(oled_fault) oled_refresh();   // display did not answer
      }
      //
      if(tick()>=refresh_cnt){   // every 30 s
         oled_refresh();
      }
      //
      if(Disp_time!=0 && Disp_expired){  // Display off
         //Disp = 0;
         OLED_PWD = 0;
      }
      //
      if(Off_time!=0 && Off_expired){    // Go to power off
         power_off();
      }
      //
      if(B_short){
        if(OLED_PWD) Btn_short();
         else oled_start();
      }
      if(B_long){
         if(OLED_PWD) Btn_long();
         else oled_start();
      }
      if(B_xlong){
         if(OLED_PWD) Btn_xlong();
         else oled_start();
      }
      // External interface
      if(E_short){
         if(OLED_PWD==0) oled_start();
         Ext_short();
      }
      if(E_long){
         if(OLED_PWD==0) { Ext_long(); oled_start(); }
         else Btn_long();
      }
    
  } // while(1)
} // main
//
void oled_start(){
   OLED_PWD = 1;
   //Disp = 1;
   Delay_ms(200);
   Soft_I2C_Init();
   Delay_ms(10);
   oled_init();
   //
   if(gre){
      Greating();
      gre = 0;
      oled_clear();
   }
   oled_labels();
   Voltage_old = 9999;
   SWR_fixed_old = 100;
   PWR_fixed_old = 9999;
   SWR_ind = 0;
   draw_swr(SWR_ind);
   volt_cnt = tick() + 1;
   watch_cnt = tick();
   refresh_cnt = tick() + 30000;
   B_short = 0; B_long = 0; B_xlong = 0, E_short = 0; E_long = 0;
   keep_awake();
   return;
}
//
static void oled_labels(void){   // fixed parts of the screen
   oled_wr_str(0, 0, "PWR ", 4);
   oled_wr_str(0, 96, "W", 1);
   oled_bat();
   oled_wr_str(0, 42, "=", 1);
   // like swr_label(4), but one call level less (hardware stack)
   oled_wr_str(2, 0, Bypass ? "BYP " : "SWR ", 4);
   oled_wr_str(2, 42, "=", 1);
   return;
}
//
// RF and relay pulses can garble the I2C transfers: free the bus, send the
// settings again and redraw everything without clearing (no flicker)
void oled_refresh(void){
   refresh_cnt = tick() + 30000;
   if(!OLED_PWD) return;
   oled_fault = 0;
   Soft_I2C_Init();
   oled_config(0);
   oled_labels();
   draw_swr(SWR_ind);
   PWR_fixed_old = 9999;         // power is redrawn by watch_swr
   if(Voltage_old!=9999) oled_voltage(Voltage_old);
   return;
}
//
void watch_swr(void){
   int delta = Auto_delta - 100;
   int PWR_fixed, SWR_fixed, c;
   //
   Delay_ms(50);
   // peak detector
   PWR_fixed = 0;
   SWR_fixed = 0;
   for(c=0; c<Peak_cnt; c++){
      get_pwr();
      if(PWR>PWR_fixed) {PWR_fixed = PWR; SWR_fixed = SWR;}
      Delay_ms(1);
   }
   //
   if(PWR_fixed>0){   // Turn on the display
      if(OLED_PWD) keep_awake();
      else oled_start();
   };
   //
   if(PWR_fixed!=PWR_fixed_old){
      if(Overflow)
         oled_wr_str(0, 42, ">", 1);
      else
         oled_wr_str(0, 42, "=", 1);
      PWR_fixed_old = PWR_fixed;
      draw_power(PWR_fixed);
   }
    //
   if(SWR_fixed>99 && SWR_fixed!=SWR_ind){
      SWR_ind = SWR_fixed;
      if(PWR_fixed<min_for_start){
         SWR_fixed = 0;
         //SWR_ind = 0;    // Last meassured SWR on display ! not a bug
         draw_swr(SWR_ind);
         return;
      }
      else
         draw_swr(SWR_ind);
   }
   //
   if(Overflow){
      for(c=3; c!=0; c--){
         oled_wr_str(2, 6, "OVERLOAD ", 9);
         Delay_ms(500);
         oled_wr_str(2, 0, "         ", 9);
         Delay_ms(500);
      }
      swr_label(9);
      draw_swr(SWR_fixed);
      Delay_ms(500);
      Overflow = 0;
   }
   //

   else if(Auto && !Bypass && PWR_fixed>=min_for_start && PWR_fixed<max_for_start && SWR_fixed>120) {
       if(  (SWR_fixed-SWR_fixed_old)>delta || (SWR_fixed_old-SWR_fixed)>delta || SWR_fixed>(999-delta) ) {
           Btn_long();
           return;
       }
   }
   //
   return;
}
//
void draw_swr(unsigned int s){
   if(s==0)
      oled_wr_str(2, 60, "0.00", 4);
   else {
      IntToStr(s, txt_2);
      txt[0] = txt_2[3];
      txt[1] = '.';
      txt[2] = txt_2[4];
      txt[3] = txt_2[5];
      //
      oled_wr_str(2, 60, txt, 4);
   }
   return;
}
//
void draw_power(unsigned int p){
   //
   if(p==0){
      oled_wr_str(0, 60, "0.0", 3);
      return;
   }
   else if(p<10){  // <1 W
      IntToStr(p, txt_2);
      txt[0] = '0';
      txt[1] = '.';
      txt[2] = txt_2[5];
   }
   else if(p<100){ // <10W
      IntToStr(p, txt_2);
      txt[0] = txt_2[4];
      txt[1] = '.';
      txt[2] = txt_2[5];
   }
   else{  // >10W
      p += 5;
      IntToStr(p, txt_2);
      txt[0] = ' ';
      txt[1] = txt_2[3];
      txt[2] = txt_2[4];
   }
   oled_wr_str(0, 60, txt, 3);
   return;
}
//
void Voltage_show(){       //  4.2 - 3.4  4200 - 3400
   get_batt();
   if(Voltage != Voltage_old) { 
      Voltage_old = Voltage; 
      oled_voltage(Voltage); 
      if(Voltage<=3800) rldl = Rel_Del + 1;
      else rldl = Rel_Del;
   }
   oled_config(0);   // repair settings garbled by RF, not visible
   //
   if(Voltage>3700){
      Green = 0;
      Red = 1;
      Delay_ms(30);
      Green = 1;
      Red = 1;
   }
   else if(Voltage>3590){
      Green = 0;
      Red = 0;
      Delay_ms(30);
      Green = 1;
      Red = 1;
   }
   else { // <3.7V
      Red = 0;
      Green = 1;
      Delay_ms(30);
      Red = 1;
      Green = 1;
   }
   if(Voltage<3400){
      oled_clear();
      oled_wr_str(1, 0, "  LOW BATT ", 11);
      Delay_ms(2000);
      OLED_PWD = 0;
      power_off();
   }
   return;
}
//
void Btn_xlong(){
   oled_clear();
   oled_wr_str(1, 0, " POWER OFF ", 11);
   Delay_ms(2000);
   power_off();
   return;
}
//
void Btn_long(){
   Green = 0;
   oled_wr_str(2, 0, "TUNE     ", 9);
   Key_out = 0;
   Bypass = 0;
   tune();
   SWR_ind = SWR;
   SWR_fixed_old = SWR;
   oled_refresh();   // after RF on the lines
   Key_out = 1;
   Green = 1;
   B_long = 0;
   E_long = 0;
   btn_1_cnt = 0;
   volt_cnt = tick();
   watch_cnt = tick();
   return;
}
//
void Ext_long(){
   Green = 0;
   OLED_PWD = 1;
   Key_out = 0;   //
   Bypass = 0;
   get_swr();     //
   if(SWR>99){
      tune();
   }
   Key_out = 1;   //
   SWR_ind = SWR;
   Green = 1;
   E_long = 0;
   return;
}
//
static void short_action(char toggle){
   Green = 0;
   if(Bypass && toggle) bypass_off();
   else bypass_on();
   Delay_ms(300);
   Green = 1;
   B_short = 0;
   E_short = 0;
   btn_1_cnt = 0;
   volt_cnt = tick();
   watch_cnt = tick();
   return;
}
//
void Btn_short(){   // button: toggle bypass
   short_action(1);
   return;
}
//
void Ext_short(){   // external interface (reset request): bypass on only
   short_action(0);
   return;
}
//
void bypass_on(void){
   if(!Bypass){      // keep the tuned setting to switch back to
      byp_ind = ind;
      byp_cap = cap;
      byp_SW = SW;
      byp_swr_old = SWR_fixed_old;
   }
   Bypass = 1;
   ind = 0;
   cap = 0;
   SW = 0;
   Relay_set(ind, cap, SW);
   oled_wr_str(2, 0, "BYPASS   ", 9);
   Delay_ms(600);
   swr_label(5);
   SWR_ind = 0;
   oled_wr_str(2, 60, "0.00", 4);
   return;
}
//
void bypass_off(void){
   Bypass = 0;
   ind = byp_ind;
   cap = byp_cap;
   SW = byp_SW;
   Relay_set(ind, cap, SW);
   SWR_fixed_old = byp_swr_old;   // no immediate auto tune
   swr_label(5);
   SWR_ind = 0;
   oled_wr_str(2, 60, "0.00", 4);
   return;
}
//
void swr_label(char len){   // label of the SWR line, BYP while in bypass
   if(Bypass) oled_wr_str(2, 0, "BYP      ", len);
   else oled_wr_str(2, 0, "SWR      ", len);
   oled_wr_str(2, 42, "=", 1);
   return;
}
//
void Greating(){
   Green = 0;
   oled_clear();
   oled_wr_str_s(1, 0, " DESIGNED BY N7DDC", 18);
   oled_wr_str_s(3, 0, " FW VERSION ", 12);
   oled_wr_str_s(3, 12*7, FW_VER, 3);
   Delay_ms(3000);
   while(GetButton) NOP();
   Green = 1;
   return;
}
//
void Relay_set(char L, char C, char I){
   L_010 = !BIT(L, 0);
   L_022 = !BIT(L, 1);
   L_045 = !BIT(L, 2);
   L_100 = !BIT(L, 3);
   L_220 = !BIT(L, 4);
   L_450 = !BIT(L, 5);
   L_1000 = !BIT(L, 6);
   //
   C_22 = !BIT(C, 0);
   C_47 = !BIT(C, 1);
   C_100 = !BIT(C, 2);
   C_220 = !BIT(C, 3);
   C_470 = !BIT(C, 4);
   C_1000 = !BIT(C, 5);
   C_2200 = !BIT(C, 6);
   //
   C_sw = I;
   //
   Rel_to_gnd = 1;
   VDelay_ms(rldl);
   Rel_to_gnd = 0;
   Delay_us(10);
   Rel_to_plus_N = 0;
   VDelay_ms(rldl);
   Rel_to_plus_N = 1;
   VDelay_ms(rldl);
   //
   L_010 = 0;
   L_022 = 0;
   L_045 = 0;
   L_100 = 0;
   L_220 = 0;
   L_450 = 0;
   L_1000 = 0;
   //
   C_22 = 0;
   C_47 = 0;
   C_100 = 0;
   C_220 = 0;
   C_470 = 0;
   C_1000 = 0;
   C_2200 = 0;
   //
   C_sw = 0;
   Delay_ms(2);
   return;
}
//
void power_off(void){
   char button_cnt;
   // Disable interrupts
   GIE_bit = 0;
   T0EN_bit = 0;
   TMR0IF_bit = 0;
   IOCIE_bit = 1;
   IOCBF5_bit = 0;
   IOCBN5_bit = 1;
   // Power saving
   OLED_PWD = 0;
   Red = 1;
   Green = 1;
   //
   button_cnt = 0;
   while(1){
      if(button_cnt==0){ Delay_ms(100); IOCBF5_bit = 0; SLEEP(); }
      NOP();
      Delay_ms(100);
      if(GetButton) button_cnt++;
      else button_cnt = 0;
      if(button_cnt>15) break;
   }
   // Enable interrupts
   IOCIE_bit = 0;
   IOCBN5_bit = 0;
   IOCBF5_bit = 0;
   T0EN_bit = 1;
   GIE_bit = 1;
   // Return to work
   gre = 1;
   oled_start();
   while(GetButton){NOP();}
   btn_1_cnt = 0;
   B_short = 0;
   B_long = 0;
   B_xlong = 0;
   GIE_bit = 0;
   btn_cnt = Tick;
   GIE_bit = 1;
   return;
}
//
void check_reset_flags(void){
   char i = 0;
   if(STKOVF_bit){oled_wr_str_s(0,  0, "Stack overflow",  14); i = 1;}
   if(STKUNF_bit){oled_wr_str_s(1,  0, "Stack underflow", 15); i = 1;}
   if(!nRWDT_bit){oled_wr_str_s(2,  0, "WDT overflow",    12); i = 1;}
   if(!nRMCLR_bit){oled_wr_str_s(3, 0, "MCLR reset  ",    12); i = 1;}
   if(!nBOR_bit){oled_wr_str_s(4,   0, "BOR reset  ",     12); i = 1;}
   if(i){
      Delay_ms(5000);
      oled_clear();
   }
   return;
}
//
int get_reverse(void){
   unsigned int v;
   volatile unsigned long d;
   ADC_Init_Advanced(_ADC_INTERNAL_VREFL | _ADC_INTERNAL_FVRH1);
   Delay_us(100);
   v = ADC_Get_Sample(REV_input);
   if(v==1023){
      ADC_Init_Advanced(_ADC_INTERNAL_VREFL | _ADC_INTERNAL_FVRH2);
      Delay_us(100);
      v = ADC_Get_Sample(REV_input) * 2;
   }
   if(v==2046){
      ADC_Init_Advanced(_ADC_INTERNAL_VREFL | _ADC_INTERNAL_VREFH);
      Delay_us(100);
      v = ADC_Get_Sample(REV_input);
      if(v==1023) Overflow = 1;
      get_batt();
      d = (long)v * (long)Voltage;
      d = d / 1024;
      v = (int)d;
   }
   return v;
}
//
int get_forward(void){
   unsigned int v;
   volatile unsigned long d;
   ADC_Init_Advanced(_ADC_INTERNAL_VREFL | _ADC_INTERNAL_FVRH1);
   Delay_us(100);
   v = ADC_Get_Sample(FWD_input);
   if(v==1023){
      ADC_Init_Advanced(_ADC_INTERNAL_VREFL | _ADC_INTERNAL_FVRH2);
      Delay_us(100);
      v = ADC_Get_Sample(FWD_input) * 2;
   }
   if(v==2046){
      ADC_Init_Advanced(_ADC_INTERNAL_VREFL | _ADC_INTERNAL_VREFH);
      Delay_us(100);
      v = ADC_Get_Sample(FWD_input);
      if(v==1023) Overflow = 1;
      get_batt();
      d = (long)v * (long)Voltage;
      d = d / 1024;
      v = (int)d;
   }
   return v;
}
//
static void pwr_wake(void){   // power detected: keep the display on
   if(PWR>0){
      if(OLED_PWD) keep_awake();
      else oled_start();
   }
   return;
}
//
void get_pwr(){
   swr_calc(get_forward(), get_reverse());
   pwr_wake();
   return;
}
//
void get_pwr_avg(char n){   // forward and reverse alternately, n pairs
   unsigned long F = 0, R = 0;
   char i;
   for(i=0; i<n; i++){
      F += get_forward();
      R += get_reverse();
   }
   swr_calc((float)F / n, (float)R / n);
   pwr_wake();
   return;
}
//
void get_batt(void){
   ADC_Init_Advanced(_ADC_INTERNAL_VREFL | _ADC_INTERNAL_FVRH1);
   Delay_us(100);
   Voltage = ADC_Get_Sample(Battery_input) * 11;
   return;
}
//

void cells_reading(void){
   char i;
   char Cells_2[10];
   for(i=0; i<10; i++){
      Cells_2[i] = Cells[i];
   }
   //
   Disp_time = Bcd2Dec(Cells_2[0]) ;
   Disp_time *= 60000;                        // minutes to ms
   Off_time = Bcd2Dec(Cells_2[1]);
   Off_time *= 60000;                         // minutes to ms
   Rel_Del =  Bcd2Dec(Cells_2[2]);            // Delay in ms
   min_for_start = Bcd2Dec(Cells_2[3]);       // Power with tens parts
   max_for_start = Bcd2Dec(Cells_2[4]) * 10;  // power in Watts
   Auto_delta = Bcd2Dec(Cells_2[5]) * 10;     // SWR with tens parts
   Auto = Bcd2Dec(Cells_2[6]) != 0;
   Cal_b = Bcd2Dec(Cells_2[7]) / 10.0;
   Cal_a = Bcd2Dec(Cells_2[8]) / 100.0 + 1.0;
   Peak_cnt = Bcd2Dec(Cells_2[9]) * 10 / 6;
   rldl = Rel_Del;
   return;
}

//
