/*
 * HardwareSerial.c - UART1 implementation for dsPIC33CK Arduino core
 *
 * Uses UART1 with PPS auto-configured from variant pins_arduino.h:
 *   TX -> SERIAL_TX_RP_REG (PPS output function 0x01)
 *   RX -> SERIAL_RX_RP_NUM (PPS input source)
 *
 * Interrupt-driven receive with ring buffer.
 *
 * The serial_* functions below are the real implementation and have external
 * linkage. How they are reached depends on the language:
 *   - C   (MPLAB X projects): through the HardwareSerial_t function-pointer
 *          struct defined at the bottom of this file.
 *   - C++ (Arduino IDE):      through the inline HardwareSerial class in
 *          HardwareSerial.h, which gives real print()/println() overloads.
 * Both faces call exactly these functions, so there is one implementation.
 */

#include "Arduino.h"
#include "HardwareSerial.h"

#ifdef __cplusplus
extern "C" {
#endif

static ring_buffer_t _rx_buffer = { {0}, 0, 0 };

/* ============================================================
 * UART1 RX Interrupt
 * ============================================================ */
void __attribute__((interrupt, auto_psv)) _U1RXInterrupt(void)
{
    uint16_t next_head = (_rx_buffer.head + 1) % SERIAL_BUFFER_SIZE;
    if (next_head != _rx_buffer.tail) {
        _rx_buffer.buffer[_rx_buffer.head] = U1RXREG;
        _rx_buffer.head = next_head;
    } else {
        (void)U1RXREG;
    }
    IFS0bits.U1RXIF = 0;
}

/* ============================================================
 * Implementation functions
 * ============================================================ */
void serial_begin(unsigned long baud)
{
    /* Idle high before the pin becomes an output. An asynchronous line idles
     * high, so driving it low even briefly looks like a start bit followed by a
     * break, and the first character out is then framed wrong. Microchip's own
     * driver carries the same warning next to its enable sequence. */
    digitalWrite(PIN_SERIAL_TX, HIGH);

    SERIAL_TX_TRIS = 0;
    SERIAL_RX_TRIS = 1;
#ifdef SERIAL_RX_ANSEL
    SERIAL_RX_ANSEL = 0;
#endif

    __builtin_write_RPCON(0x0000);
    SERIAL_TX_RP_REG = PPS_OUT_U1TX;
    _U1RXR = SERIAL_RX_RP_NUM;
    __builtin_write_RPCON(0x0800);

    U1MODEbits.UARTEN = 0;

    /* Baud rate, via the fractional baud generator.
     *
     * The 20-bit divisor in U1BRGH:U1BRG counts baud-clock cycles per bit
     * directly (baud = FCY / BRG), rather than the coarser prescaled forms
     * FCY/(16*(BRG+1)) with BRGH=0 or FCY/(4*(BRG+1)) with BRGH=1. This is the
     * mode Microchip's own MCC driver for these parts selects first, and the
     * finer step is what makes fast baud rates reachable on a slow clock: at
     * the 8 MHz FRC default (FCY = 4 MHz) the /16 divisor for 115200 lands on
     * 2, i.e. 125000 baud -- an 8.5% error that no receiver can decode.
     * Fractional gives BRG = 35 -> 114286 baud, -0.7%, comfortably in spec.
     *
     * Rounded rather than truncated: truncation alone throws away up to a full
     * count, which at small BRG values is several percent of the bit period.
     * The fallback prescaled modes are only needed below FCY/0xFFFFF baud
     * (3.8 baud at FCY = 4 MHz), so they are not worth carrying here. */
    U1MODEHbits.BCLKSEL = 0b00;     /* baud clock = FOSC/2 = FCY */
    U1MODEHbits.BCLKMOD = 1;        /* fractional baud generator */
    U1MODEbits.BRGH = 0;            /* must be 0 in fractional mode */

    if (baud == 0UL) {
        baud = 9600UL;              /* don't divide by zero on Serial.begin(0) */
    }
    {
        unsigned long brg = (FCY + (baud / 2UL)) / baud;   /* round to nearest */

        if (brg < 1UL) {
            brg = 1UL;
        } else if (brg > 0xFFFFFUL) {
            brg = 0xFFFFFUL;        /* clamp to the 20-bit field */
        }
        U1BRG  = (uint16_t)(brg & 0xFFFFUL);
        U1BRGH = (uint16_t)((brg >> 16) & 0x000FUL);
    }

    U1MODEbits.MOD = 0b0000;
    U1MODEHbits.STSEL = 0b00;

    _rx_buffer.head = 0;
    _rx_buffer.tail = 0;
    IPC2bits.U1RXIP = 3;
    IFS0bits.U1RXIF = 0;
    IEC0bits.U1RXIE = 1;

    /* Enable order matters: UARTEN must go high BEFORE UTXEN/URXEN. On this
     * UART module the transmitter and receiver enables are gated by UARTEN, so
     * setting them while the module is still off does not stick -- the result
     * is a UART that looks fully configured and never transmits a single bit.
     * This is the order Microchip's generated driver uses for these parts. */
    U1MODEbits.UARTEN = 1;
    U1MODEbits.UTXEN = 1;
    U1MODEbits.URXEN = 1;
}

void serial_end(void)
{
    while (!U1STAbits.TRMT);
    IEC0bits.U1RXIE = 0;
    U1MODEbits.UARTEN = 0;
    _rx_buffer.head = 0;
    _rx_buffer.tail = 0;
}

int serial_available(void)
{
    return (int)(((unsigned int)(SERIAL_BUFFER_SIZE + _rx_buffer.head - _rx_buffer.tail)) % SERIAL_BUFFER_SIZE);
}

int serial_read(void)
{
    if (_rx_buffer.head == _rx_buffer.tail) {
        return -1;
    }
    uint8_t c = _rx_buffer.buffer[_rx_buffer.tail];
    _rx_buffer.tail = (_rx_buffer.tail + 1) % SERIAL_BUFFER_SIZE;
    return (int)c;
}

int serial_peek(void)
{
    if (_rx_buffer.head == _rx_buffer.tail) {
        return -1;
    }
    return (int)_rx_buffer.buffer[_rx_buffer.tail];
}

void serial_flush(void)
{
    while (!U1STAbits.TRMT);
}

size_t serial_write(uint8_t c)
{
    while (U1STAHbits.UTXBF);
    U1TXREG = c;
    return 1;
}

size_t serial_print(const char *str)
{
    size_t n = 0;
    if (str == NULL) {
        return 0;
    }
    while (*str) {
        serial_write((uint8_t)*str++);
        n++;
    }
    return n;
}

size_t serial_println(const char *str)
{
    size_t n = serial_print(str);
    serial_write('\r');
    serial_write('\n');
    return n + 2;
}

/* Unsigned digit conversion, shared by both integer entry points. Split out so
 * that unsigned values above LONG_MAX print correctly -- Serial.println(millis())
 * is the common case, and millis() exceeds 2^31 after 24.8 days. */
size_t serial_print_uint(unsigned long uval, int base)
{
    char buf[34];
    char *p = buf + sizeof(buf) - 1;
    *p = '\0';

    /* base 0 would divide by zero and base 1 would never terminate. */
    if (base < 2 || base > 36) {
        base = 10;
    }

    do {
        unsigned int digit = (unsigned int)(uval % (unsigned long)base);
        *--p = (digit < 10) ? (char)('0' + digit) : (char)('A' + digit - 10);
        uval /= (unsigned long)base;
    } while (uval > 0);

    return serial_print(p);
}

size_t serial_print_int(long val, int base)
{
    /* Sign is base-10 only, matching standard Arduino: print(-1, HEX) shows the
     * two's-complement pattern rather than "-1". */
    if (val < 0 && base == 10) {
        serial_write('-');
        /* 0 - (unsigned)val rather than -val: negating LONG_MIN is undefined,
         * but the unsigned subtraction yields its magnitude correctly. */
        return 1 + serial_print_uint(0UL - (unsigned long)val, 10);
    }
    return serial_print_uint((unsigned long)val, base);
}

size_t serial_println_int(long val, int base)
{
    size_t n = serial_print_int(val, base);
    serial_write('\r');
    serial_write('\n');
    return n + 2;
}

size_t serial_println_uint(unsigned long val, int base)
{
    size_t n = serial_print_uint(val, base);
    serial_write('\r');
    serial_write('\n');
    return n + 2;
}

size_t serial_print_float(double val, int decimals)
{
    char buf[32];
    char *p = buf;
    size_t n = 0;

    /* buf holds sign + up to 10 integer digits + '.' + decimals + NUL. Clamp so
     * a large "decimals" cannot run off the end of the buffer. */
    if (decimals < 0) {
        decimals = 0;
    } else if (decimals > 16) {
        decimals = 16;
    }

    if (val < 0.0) {
        *p++ = '-';
        val = -val;
    }

    unsigned long int_part = (unsigned long)val;
    double remainder = val - (double)int_part;

    /* Integer part */
    char tmp[12];
    char *tp = tmp + sizeof(tmp) - 1;
    *tp = '\0';
    if (int_part == 0) {
        *--tp = '0';
    } else {
        while (int_part > 0) {
            *--tp = '0' + (int_part % 10);
            int_part /= 10;
        }
    }
    while (*tp) *p++ = *tp++;

    /* Decimal part */
    if (decimals > 0) {
        *p++ = '.';
        int i;
        for (i = 0; i < decimals; i++) {
            remainder *= 10.0;
            int digit = (int)remainder;
            *p++ = '0' + digit;
            remainder -= digit;
        }
    }
    *p = '\0';

    n = serial_print(buf);
    return n;
}

size_t serial_println_float(double val, int decimals)
{
    size_t n = serial_print_float(val, decimals);
    serial_write('\r');
    serial_write('\n');
    return n + 2;
}

#ifdef __cplusplus
}   /* extern "C" */
#endif

/* ============================================================
 * The global "Serial" object, in whichever form the language allows.
 * ============================================================ */
#ifndef __cplusplus

/* C: a struct of function pointers, which is what makes dot-notation work. */
HardwareSerial_t Serial = {
    .begin       = serial_begin,
    .end         = serial_end,
    .available   = serial_available,
    .read        = serial_read,
    .peek        = serial_peek,
    .flush       = serial_flush,
    .write       = serial_write,
    .print       = serial_print,
    .println     = serial_println,
    .print_int   = serial_print_int,
    .println_int = serial_println_int,
    .print_float = serial_print_float,
    .println_float = serial_println_float,
};

#else

/* C++: a stateless class whose inline methods call the same functions directly.
 * Defined outside the extern "C" block above so its language linkage matches
 * the declaration in HardwareSerial.h. One byte of .bss, no vtable. */
HardwareSerial Serial;

#endif
