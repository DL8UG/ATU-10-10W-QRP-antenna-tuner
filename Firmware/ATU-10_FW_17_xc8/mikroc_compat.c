// mikroC PRO for PIC library replacements for Microchip XC8

#include "mikroc_compat.h"

void VDelay_ms(unsigned int ms) {
   while(ms--) __delay_ms(1);
}

void ADC_Init(void) {
   ADC_Init_Advanced(_ADC_INTERNAL_VREFL | _ADC_INTERNAL_VREFH);
}

void ADC_Init_Advanced(char reference) {
   ADCON0bits.ADON = 0;
   if(reference & (_ADC_INTERNAL_FVRH1 | _ADC_INTERNAL_FVRH2)) {
      FVRCONbits.ADFVR = (reference & _ADC_INTERNAL_FVRH2) ? 0b10 : 0b01;
      FVRCONbits.FVREN = 1;
      while(!FVRCONbits.FVRRDY);
      ADREF = 0b00000011;          // Vref+ = FVR, Vref- = Vss
   }
   else
      ADREF = 0b00000000;          // Vref+ = Vdd, Vref- = Vss
   ADCON1 = 0;
   ADCON2 = 0;                     // basic (legacy) mode
   ADCON3 = 0;
   ADACQ = 0;                      // acquisition time is done in software
   ADPRE = 0;
   ADCON0 = 0b00010100;            // FRC clock, right justified
   ADCON0bits.ADON = 1;
}

unsigned int ADC_Get_Sample(char channel) {
   ADPCH = channel;
   __delay_us(20);                 // acquisition time
   ADCON0bits.ADGO = 1;
   while(ADCON0bits.ADGO);
   return ((unsigned int)ADRESH << 8) | ADRESL;
}

// Right aligned in 6 characters, padded with blanks, plus terminator
void IntToStr(int input, char *output) {
   char i;
   char negative = input < 0;
   unsigned int n = negative ? -(unsigned int)input : (unsigned int)input;
   for(i = 0; i < 6; i++) output[i] = ' ';
   output[6] = 0;
   i = 5;
   do {
      output[i--] = n % 10 + '0';
      n /= 10;
   } while(n != 0);
   if(negative) output[i] = '-';
}

char Bcd2Dec(char bcdnum) {
   return (bcdnum >> 4) * 10 + (bcdnum & 0x0F);
}
