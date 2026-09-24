// mikroC PRO for PIC compatibility layer for Microchip XC8
// Maps the mikroC register bit names and library calls used by the
// ATU-10 firmware onto XC8 equivalents for the PIC16LF18877.

#ifndef MIKROC_COMPAT_H
#define MIKROC_COMPAT_H

#include <xc.h>

#define _XTAL_FREQ 32000000

// Delays
#define Delay_ms(x)   __delay_ms(x)
#define Delay_us(x)   __delay_us(x)
void VDelay_ms(unsigned int ms);

// Single bit of a byte (replaces mikroC's var.Bn syntax)
#define BIT(v, n)     (((v) >> (n)) & 1)

// Register bits (mikroC name_bit -> XC8 REGbits.NAME). The device header
// defines these names too, but only for use in assembly, so replace them.
#undef  GIE_bit
#define GIE_bit       INTCONbits.GIE
#undef  TMR0IF_bit
#define TMR0IF_bit    PIR0bits.TMR0IF
#undef  TMR0IE_bit
#define TMR0IE_bit    PIE0bits.TMR0IE
#undef  IOCIE_bit
#define IOCIE_bit     PIE0bits.IOCIE
#undef  IOCBF5_bit
#define IOCBF5_bit    IOCBFbits.IOCBF5
#undef  IOCBN5_bit
#define IOCBN5_bit    IOCBNbits.IOCBN5
#undef  T0EN_bit
#define T0EN_bit      T0CON0bits.T0EN
#undef  T016BIT_bit
#define T016BIT_bit   T0CON0bits.T016BIT
#undef  T0CS0_bit
#define T0CS0_bit     T0CON1bits.T0CS0
#undef  T0CS1_bit
#define T0CS1_bit     T0CON1bits.T0CS1
#undef  T0CS2_bit
#define T0CS2_bit     T0CON1bits.T0CS2
#undef  ANSB0_bit
#define ANSB0_bit     ANSELBbits.ANSB0
#undef  ANSB1_bit
#define ANSB1_bit     ANSELBbits.ANSB1
#undef  ANSB2_bit
#define ANSB2_bit     ANSELBbits.ANSB2
#undef  C1ON_bit
#define C1ON_bit      CM1CON0bits.C1ON
#undef  C2ON_bit
#define C2ON_bit      CM2CON0bits.C2ON
#undef  ODCA2_bit
#define ODCA2_bit     ODCONAbits.ODCA2
#undef  ODCA3_bit
#define ODCA3_bit     ODCONAbits.ODCA3
#undef  ODCD1_bit
#define ODCD1_bit     ODCONDbits.ODCD1
#undef  ODCD2_bit
#define ODCD2_bit     ODCONDbits.ODCD2
#undef  STKOVF_bit
#define STKOVF_bit    PCON0bits.STKOVF
#undef  STKUNF_bit
#define STKUNF_bit    PCON0bits.STKUNF
#undef  nRWDT_bit
#define nRWDT_bit     PCON0bits.nRWDT
#undef  nRMCLR_bit
#define nRMCLR_bit    PCON0bits.nRMCLR
#undef  nBOR_bit
#define nBOR_bit      PCON0bits.nBOR
#undef  LATA2_bit
#define LATA2_bit     LATAbits.LATA2
#undef  LATA3_bit
#define LATA3_bit     LATAbits.LATA3
#undef  LATD1_bit
#define LATD1_bit     LATDbits.LATD1
#undef  LATD2_bit
#define LATD2_bit     LATDbits.LATD2

// ADC library (ADC2 module of the PIC16F18877)
#define _ADC_INTERNAL_VREFL  0x00   // Vref- = Vss
#define _ADC_INTERNAL_VREFH  0x10   // Vref+ = Vdd
#define _ADC_INTERNAL_FVRH1  0x20   // Vref+ = FVR 1.024 V
#define _ADC_INTERNAL_FVRH2  0x40   // Vref+ = FVR 2.048 V
void ADC_Init(void);
void ADC_Init_Advanced(char reference);
unsigned int ADC_Get_Sample(char channel);

// Conversions library
void IntToStr(int input, char *output);
char Bcd2Dec(char bcdnum);

#endif
