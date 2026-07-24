/*
 * variant.c - Pin map table for dsPIC33CK256MP508
 *
 * Maps Arduino pin numbers to physical dsPIC33CK port/bit/ADC channel.
 * 80-pin device: 69 GPIO across PORTA-PORTE.
 *
 * ADC channel mapping (from device .PIC file):
 *   RA0=AN0, RA2=AN9, RA3=AN3, RA4=AN4
 *   RB0=AN5, RB1=AN6, RB2=AN1, RB3=AN8, RB7=AN2, RB8=AN10, RB9=AN11
 *   RC0=AN12, RC1=AN13, RC2=AN14, RC3=AN15, RC6=AN17, RC7=AN16
 *   RD10=AN18, RD11=AN19
 *   RE0=AN20, RE1=AN21, RE2=AN22, RE3=AN23
 */

#include <xc.h>
#include <stddef.h>
#include "pins_arduino.h"

#ifdef __cplusplus
extern "C" {
#endif

const pin_map_t g_pin_map[NUM_DIGITAL_PINS] = {
    /* ===== PORTA: D0-D4 (RA0-RA4) ===== */
    /* D0:  RA0 */  { &PORTA, &TRISA, &LATA, &ANSELA,  0,  0 },  /* AN0 */
    /* D1:  RA1 */  { &PORTA, &TRISA, &LATA, &ANSELA,  1, -1 },
    /* D2:  RA2 */  { &PORTA, &TRISA, &LATA, &ANSELA,  2,  9 },  /* AN9 */
    /* D3:  RA3 */  { &PORTA, &TRISA, &LATA, &ANSELA,  3,  3 },  /* AN3 */
    /* D4:  RA4 */  { &PORTA, &TRISA, &LATA, &ANSELA,  4,  4 },  /* AN4 */

    /* ===== PORTB: D5-D20 (RB0-RB15) ===== */
    /* D5:  RB0 */  { &PORTB, &TRISB, &LATB, &ANSELB,  0,  5 },  /* AN5 */
    /* D6:  RB1 */  { &PORTB, &TRISB, &LATB, &ANSELB,  1,  6 },  /* AN6 */
    /* D7:  RB2 */  { &PORTB, &TRISB, &LATB, &ANSELB,  2,  1 },  /* AN1 */
    /* D8:  RB3 */  { &PORTB, &TRISB, &LATB, &ANSELB,  3,  8 },  /* AN8 */
    /* D9:  RB4 */  { &PORTB, &TRISB, &LATB, &ANSELB,  4, -1 },
    /* D10: RB5 */  { &PORTB, &TRISB, &LATB, NULL,     5, -1 },
    /* D11: RB6 */  { &PORTB, &TRISB, &LATB, NULL,     6, -1 },
    /* D12: RB7 */  { &PORTB, &TRISB, &LATB, &ANSELB,  7,  2 },  /* AN2 */
    /* D13: RB8 */  { &PORTB, &TRISB, &LATB, &ANSELB,  8, 10 },  /* AN10 */
    /* D14: RB9 */  { &PORTB, &TRISB, &LATB, &ANSELB,  9, 11 },  /* AN11 */
    /* D15: RB10 */ { &PORTB, &TRISB, &LATB, NULL,    10, -1 },
    /* D16: RB11 */ { &PORTB, &TRISB, &LATB, NULL,    11, -1 },
    /* D17: RB12 */ { &PORTB, &TRISB, &LATB, NULL,    12, -1 },
    /* D18: RB13 */ { &PORTB, &TRISB, &LATB, NULL,    13, -1 },
    /* D19: RB14 */ { &PORTB, &TRISB, &LATB, NULL,    14, -1 },
    /* D20: RB15 */ { &PORTB, &TRISB, &LATB, NULL,    15, -1 },

    /* ===== PORTC: D21-D36 (RC0-RC15) ===== */
    /* D21: RC0 */  { &PORTC, &TRISC, &LATC, &ANSELC,  0, 12 },  /* AN12 */
    /* D22: RC1 */  { &PORTC, &TRISC, &LATC, &ANSELC,  1, 13 },  /* AN13 */
    /* D23: RC2 */  { &PORTC, &TRISC, &LATC, &ANSELC,  2, 14 },  /* AN14 */
    /* D24: RC3 */  { &PORTC, &TRISC, &LATC, &ANSELC,  3, 15 },  /* AN15 */
    /* D25: RC4 */  { &PORTC, &TRISC, &LATC, NULL,     4, -1 },
    /* D26: RC5 */  { &PORTC, &TRISC, &LATC, NULL,     5, -1 },
    /* D27: RC6 */  { &PORTC, &TRISC, &LATC, &ANSELC,  6, 17 },  /* AN17 */
    /* D28: RC7 */  { &PORTC, &TRISC, &LATC, &ANSELC,  7, 16 },  /* AN16 */
    /* D29: RC8 */  { &PORTC, &TRISC, &LATC, NULL,     8, -1 },
    /* D30: RC9 */  { &PORTC, &TRISC, &LATC, NULL,     9, -1 },
    /* D31: RC10 */ { &PORTC, &TRISC, &LATC, NULL,    10, -1 },
    /* D32: RC11 */ { &PORTC, &TRISC, &LATC, NULL,    11, -1 },
    /* D33: RC12 */ { &PORTC, &TRISC, &LATC, NULL,    12, -1 },
    /* D34: RC13 */ { &PORTC, &TRISC, &LATC, NULL,    13, -1 },
    /* D35: RC14 */ { &PORTC, &TRISC, &LATC, NULL,    14, -1 },
    /* D36: RC15 */ { &PORTC, &TRISC, &LATC, NULL,    15, -1 },

    /* ===== PORTD: D37-D52 (RD0-RD15) ===== */
    /* D37: RD0 */  { &PORTD, &TRISD, &LATD, NULL,     0, -1 },
    /* D38: RD1 */  { &PORTD, &TRISD, &LATD, NULL,     1, -1 },
    /* D39: RD2 */  { &PORTD, &TRISD, &LATD, NULL,     2, -1 },
    /* D40: RD3 */  { &PORTD, &TRISD, &LATD, NULL,     3, -1 },
    /* D41: RD4 */  { &PORTD, &TRISD, &LATD, NULL,     4, -1 },
    /* D42: RD5 */  { &PORTD, &TRISD, &LATD, NULL,     5, -1 },
    /* D43: RD6 */  { &PORTD, &TRISD, &LATD, NULL,     6, -1 },
    /* D44: RD7 */  { &PORTD, &TRISD, &LATD, NULL,     7, -1 },
    /* D45: RD8 */  { &PORTD, &TRISD, &LATD, NULL,     8, -1 },
    /* D46: RD9 */  { &PORTD, &TRISD, &LATD, NULL,     9, -1 },
    /* D47: RD10 */ { &PORTD, &TRISD, &LATD, &ANSELD, 10, 18 },  /* AN18 */
    /* D48: RD11 */ { &PORTD, &TRISD, &LATD, &ANSELD, 11, 19 },  /* AN19 */
    /* D49: RD12 */ { &PORTD, &TRISD, &LATD, NULL,    12, -1 },
    /* D50: RD13 */ { &PORTD, &TRISD, &LATD, &ANSELD, 13, -1 },
    /* D51: RD14 */ { &PORTD, &TRISD, &LATD, NULL,    14, -1 },
    /* D52: RD15 */ { &PORTD, &TRISD, &LATD, NULL,    15, -1 },

    /* ===== PORTE: D53-D68 (RE0-RE15) ===== */
    /* D53: RE0 */  { &PORTE, &TRISE, &LATE, &ANSELE,  0, 20 },  /* AN20 */
    /* D54: RE1 */  { &PORTE, &TRISE, &LATE, &ANSELE,  1, 21 },  /* AN21 */
    /* D55: RE2 */  { &PORTE, &TRISE, &LATE, &ANSELE,  2, 22 },  /* AN22 */
    /* D56: RE3 */  { &PORTE, &TRISE, &LATE, &ANSELE,  3, 23 },  /* AN23 */
    /* D57: RE4 */  { &PORTE, &TRISE, &LATE, NULL,     4, -1 },
    /* D58: RE5 */  { &PORTE, &TRISE, &LATE, NULL,     5, -1 },
    /* D59: RE6 */  { &PORTE, &TRISE, &LATE, NULL,     6, -1 },
    /* D60: RE7 */  { &PORTE, &TRISE, &LATE, NULL,     7, -1 },
    /* D61: RE8 */  { &PORTE, &TRISE, &LATE, NULL,     8, -1 },
    /* D62: RE9 */  { &PORTE, &TRISE, &LATE, NULL,     9, -1 },
    /* D63: RE10 */ { &PORTE, &TRISE, &LATE, NULL,    10, -1 },
    /* D64: RE11 */ { &PORTE, &TRISE, &LATE, NULL,    11, -1 },
    /* D65: RE12 */ { &PORTE, &TRISE, &LATE, NULL,    12, -1 },
    /* D66: RE13 */ { &PORTE, &TRISE, &LATE, NULL,    13, -1 },
    /* D67: RE14 */ { &PORTE, &TRISE, &LATE, NULL,    14, -1 },
    /* D68: RE15 */ { &PORTE, &TRISE, &LATE, NULL,    15, -1 },
};

#ifdef __cplusplus
}
#endif
