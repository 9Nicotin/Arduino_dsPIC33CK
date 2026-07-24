/*
 * Bare-metal LED test for dsPIC33CK256MP508 on DM330030
 * RE6 = LED1, RE5 = LED2
 * Compiles directly with XC-DSC, no Arduino framework
 */
#include <xc.h>

/* Configuration bits - minimal set */
#pragma config FNOSC = FRC
#pragma config IESO = OFF
#pragma config POSCMD = NONE
#pragma config FCKSM = CSECMD
#pragma config JTAGEN = OFF
#pragma config FWDTEN = ON_SW
#pragma config OSCIOFNC = OFF
#pragma config ICS = PGD3
#pragma config DMTDIS = ON

int main(void)
{
    /* Disable analog on PORTE */
    ANSELE = 0x0000;

    /* RE5 and RE6 as outputs */
    TRISEbits.TRISE5 = 0;
    TRISEbits.TRISE6 = 0;

    /* Turn on both LEDs */
    LATEbits.LATE5 = 1;
    LATEbits.LATE6 = 1;

    /* Loop forever */
    while (1) {
        /* Simple delay by counting */
        volatile unsigned long i;
        for (i = 0; i < 200000UL; i++);

        /* Toggle LEDs */
        LATE ^= (1 << 5) | (1 << 6);
    }

    return 0;
}
