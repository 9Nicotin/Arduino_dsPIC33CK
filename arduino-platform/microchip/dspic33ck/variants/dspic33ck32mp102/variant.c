/*
 * variant.c - Pin map table for dsPIC33CK32MP102
 *
 * Maps Arduino pin numbers to physical dsPIC33CK port/bit/ADC channel.
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
 * D0  = RA0    D7  = RB2/AN2   D14 = RB9 (SDA)
 * D1  = RA1    D8  = RB3/AN3   D15 = RB10 (SCL)
 * D2  = RA2    D9  = RB4/AN4   D16 = RB11 (SS)
 * D3  = RA3    D10 = RB5/AN5   D17 = RB12
 * D4  = RA4    D11 = RB6       D18 = RB13
 * D5  = RB0/AN0  D12 = RB7     D19 = RB14
 * D6  = RB1/AN1  D13 = RB8     D20 = RB15
 */

const pin_map_t g_pin_map[NUM_DIGITAL_PINS] = {
    /* D0:  RA0 */  { &PORTA, &TRISA, &LATA, NULL,    0, -1 },
    /* D1:  RA1 */  { &PORTA, &TRISA, &LATA, NULL,    1, -1 },
    /* D2:  RA2 */  { &PORTA, &TRISA, &LATA, NULL,    2, -1 },
    /* D3:  RA3 */  { &PORTA, &TRISA, &LATA, NULL,    3, -1 },
    /* D4:  RA4 */  { &PORTA, &TRISA, &LATA, NULL,    4, -1 },
    /* D5:  RB0 */  { &PORTB, &TRISB, &LATB, &ANSELB, 0,  0 },
    /* D6:  RB1 */  { &PORTB, &TRISB, &LATB, &ANSELB, 1,  1 },
    /* D7:  RB2 */  { &PORTB, &TRISB, &LATB, &ANSELB, 2,  2 },
    /* D8:  RB3 */  { &PORTB, &TRISB, &LATB, &ANSELB, 3,  3 },
    /* D9:  RB4 */  { &PORTB, &TRISB, &LATB, &ANSELB, 4,  4 },
    /* D10: RB5 */  { &PORTB, &TRISB, &LATB, &ANSELB, 5,  5 },
    /* D11: RB6 */  { &PORTB, &TRISB, &LATB, &ANSELB, 6, -1 },
    /* D12: RB7 */  { &PORTB, &TRISB, &LATB, &ANSELB, 7, -1 },
    /* D13: RB8 */  { &PORTB, &TRISB, &LATB, &ANSELB, 8, -1 },
    /* D14: RB9 */  { &PORTB, &TRISB, &LATB, &ANSELB, 9, -1 },
    /* D15: RB10 */ { &PORTB, &TRISB, &LATB, &ANSELB, 10, -1 },
    /* D16: RB11 */ { &PORTB, &TRISB, &LATB, &ANSELB, 11, -1 },
    /* D17: RB12 */ { &PORTB, &TRISB, &LATB, &ANSELB, 12, -1 },
    /* D18: RB13 */ { &PORTB, &TRISB, &LATB, &ANSELB, 13, -1 },
    /* D19: RB14 */ { &PORTB, &TRISB, &LATB, &ANSELB, 14, -1 },
    /* D20: RB15 */ { &PORTB, &TRISB, &LATB, &ANSELB, 15, -1 },
};

#ifdef __cplusplus
}
#endif
