#include "mikroc_compat.h"
#include "pic_init.h"

// Configuration words of the mikroC project (ATU-10.cfg):
// CONFIG1..5 = 0x2904, 0x3E21, 0x3F1F, 0x3003, 0x0003
#pragma config FEXTOSC = OFF, RSTOSC = HFINT32, CLKOUTEN = OFF, CSWEN = ON, FCMEN = ON
#pragma config MCLRE = ON, PWRTE = ON, LPBOREN = OFF, BOREN = OFF, BORV = LO
#pragma config ZCD = OFF, PPS1WAY = ON, STVREN = ON, DEBUG = OFF
#pragma config WDTCPS = WDTCPS_31, WDTE = OFF, WDTCWS = WDTCWS_7, WDTCCS = SC
#pragma config WRT = OFF, SCANE = available, LVP = ON
#pragma config CP = OFF, CPD = OFF
// pic_init unit for Micro C PRO
// 2020

void pic_init (void) {
// ports initialisation
  ANSELA = 0;         // all as digital
  ANSELB = 0;         // all as digital
  ANSB0_bit = 1;      // analog input
  ANSB1_bit = 1;      // analog input
  ANSB2_bit = 1;      // analog input
  ANSELC = 0;         // all as digital
  ANSELE = 0;         // all as digital
  ANSELD = 0;         // all as digital

  C1ON_bit = 0;      // Disable comparators
  C2ON_bit = 0;

  PORTA = 0;
  PORTB = 0;
  PORTC = 0;
  PORTD = 0;
  PORTE = 0;
  LATA = 0b00000000;
  LATB = 0b00000000;
  LATC = 0b00010000;
  LATD = 0b00000110;
  LATE = 0b00000000;
  TRISA = 0b00000000;
  TRISB = 0b00100111;
  TRISC = 0b00000000;
  TRISD = 0b00000000;
  TRISE = 0b00000000;

  // open drains
  ODCA2_bit = 1;
  ODCA3_bit = 1;
  ODCD1_bit = 1;
  ODCD2_bit = 1;
  
  // Timer0 settings
  T0CS0_bit = 0; // Fosc/4
  T0CS1_bit = 1;
  T0CS2_bit = 0;
  T016BIT_bit = 1;
  TMR0L = 0xC0;   // 8_000 cycles to OF
  TMR0H = 0xE0;
  TMR0IF_bit = 0;
  T0EN_bit = 1;
  TMR0IE_bit = 1;
  
  // Modules disable
  PMD0 = 0b00011110; //
  PMD1 = 0b11111110;
  PMD2 = 0b01000111;
  PMD3 = 0b01111111;
  PMD4 = 0b1110111;
  PMD5 = 0b11011111;
  //interrupt setting
  GIE_bit = 1;
  Delay_ms (100);
  return;
}
