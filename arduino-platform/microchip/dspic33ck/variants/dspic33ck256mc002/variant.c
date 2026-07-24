/*
 * variant.c - Pin map table for dsPIC33CK256MC002
 *
 * Maps Arduino pin numbers to physical dsPIC33CK port/bit/ADC channel.
 *
 * Key difference from MP102: PORTA pins (RA0-RA4) have ANSELA and
 * are analog-capable. PORTB RB7-RB9 also gain analog (AN8-AN10).
 */

#include <xc.h>
#include <stddef.h>
#include "pins_arduino.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Pin Map Table
 * Index = Arduino pin number
 *
 * D0  = RA0/ANA0   D7  = RB2/AN2    D14 = RB9/AN10 (SDA)
 * D1  = RA1/ANA1   D8  = RB3/AN3    D15 = RB10 (SCL)
 * D2  = RA2/ANA2   D9  = RB4/AN4    D16 = RB11 (SS)
 * D3  = RA3/ANA3   D10 = RB5         D17 = RB12
 * D4  = RA4/ANA4   D11 = RB6         D18 = RB13
 * D5  = RB0/AN0    D12 = RB7/AN8     D19 = RB14
 * D6  = RB1/AN1    D13 = RB8/AN9     D20 = RB15
 *
 * ADC channel mapping:
 *   AN0-AN4  = RB0-RB4 (PORTB)
 *   AN8-AN10 = RB7-RB9 (PORTB)
 *   ANA0-ANA4 mapped as channels 16-20 for PORTA analog
 */

const pin_map_t g_pin_map[NUM_DIGITAL_PINS] = {
    /* D0:  RA0 */  { &PORTA, &TRISA, &LATA, &ANSELA, 0, 16 },
    /* D1:  RA1 */  { &PORTA, &TRISA, &LATA, &ANSELA, 1, 17 },
    /* D2:  RA2 */  { &PORTA, &TRISA, &LATA, &ANSELA, 2, 20 },
    /* D3:  RA3 */  { &PORTA, &TRISA, &LATA, &ANSELA, 3, 21 },
    /* D4:  RA4 */  { &PORTA, &TRISA, &LATA, &ANSELA, 4, -1 },
    /* D5:  RB0 */  { &PORTB, &TRISB, &LATB, &ANSELB, 0,  0 },
    /* D6:  RB1 */  { &PORTB, &TRISB, &LATB, &ANSELB, 1,  1 },
    /* D7:  RB2 */  { &PORTB, &TRISB, &LATB, &ANSELB, 2,  2 },
    /* D8:  RB3 */  { &PORTB, &TRISB, &LATB, &ANSELB, 3,  3 },
    /* D9:  RB4 */  { &PORTB, &TRISB, &LATB, &ANSELB, 4,  4 },
    /* D10: RB5 */  { &PORTB, &TRISB, &LATB, NULL,    5, -1 },
    /* D11: RB6 */  { &PORTB, &TRISB, &LATB, NULL,    6, -1 },
    /* D12: RB7 */  { &PORTB, &TRISB, &LATB, &ANSELB, 7,  8 },
    /* D13: RB8 */  { &PORTB, &TRISB, &LATB, &ANSELB, 8,  9 },
    /* D14: RB9 */  { &PORTB, &TRISB, &LATB, &ANSELB, 9, 10 },
    /* D15: RB10 */ { &PORTB, &TRISB, &LATB, NULL,   10, -1 },
    /* D16: RB11 */ { &PORTB, &TRISB, &LATB, NULL,   11, -1 },
    /* D17: RB12 */ { &PORTB, &TRISB, &LATB, NULL,   12, -1 },
    /* D18: RB13 */ { &PORTB, &TRISB, &LATB, NULL,   13, -1 },
    /* D19: RB14 */ { &PORTB, &TRISB, &LATB, NULL,   14, -1 },
    /* D20: RB15 */ { &PORTB, &TRISB, &LATB, NULL,   15, -1 },
};

#ifdef __cplusplus
}
#endif
