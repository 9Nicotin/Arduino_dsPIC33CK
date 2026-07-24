/*
 * HardwareSerial.c - UART1 implementation for dsPIC33CK Arduino core
 *
 * Uses UART1 with PPS auto-configured from variant pins_arduino.h:
 *   TX -> SERIAL_TX_RP_REG (PPS output function 0x01)
 *   RX -> SERIAL_RX_RP_NUM (PPS input source)
 *
 * Interrupt-driven receive with ring buffer.
 * Exposes a global "Serial" struct with function pointers for dot-notation.
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
 * Implementation functions (static — accessed via Serial struct)
 * ============================================================ */
static void _serial_begin(unsigned long baud)
{
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

    U1MODEHbits.BCLKSEL = 0b00;
    U1BRG = (FCY / (16UL * baud)) - 1;

    U1MODEbits.BRGH = 0;
    U1MODEbits.MOD = 0b0000;
    U1MODEHbits.STSEL = 0b00;

    U1MODEbits.UTXEN = 1;
    U1MODEbits.URXEN = 1;

    _rx_buffer.head = 0;
    _rx_buffer.tail = 0;
    IPC2bits.U1RXIP = 3;
    IFS0bits.U1RXIF = 0;
    IEC0bits.U1RXIE = 1;

    U1MODEbits.UARTEN = 1;
}

static void _serial_end(void)
{
    while (!U1STAbits.TRMT);
    IEC0bits.U1RXIE = 0;
    U1MODEbits.UARTEN = 0;
    _rx_buffer.head = 0;
    _rx_buffer.tail = 0;
}

static int _serial_available(void)
{
    return (int)(((unsigned int)(SERIAL_BUFFER_SIZE + _rx_buffer.head - _rx_buffer.tail)) % SERIAL_BUFFER_SIZE);
}

static int _serial_read(void)
{
    if (_rx_buffer.head == _rx_buffer.tail) {
        return -1;
    }
    uint8_t c = _rx_buffer.buffer[_rx_buffer.tail];
    _rx_buffer.tail = (_rx_buffer.tail + 1) % SERIAL_BUFFER_SIZE;
    return (int)c;
}

static int _serial_peek(void)
{
    if (_rx_buffer.head == _rx_buffer.tail) {
        return -1;
    }
    return (int)_rx_buffer.buffer[_rx_buffer.tail];
}

static void _serial_flush(void)
{
    while (!U1STAbits.TRMT);
}

static size_t _serial_write(uint8_t c)
{
    while (U1STAHbits.UTXBF);
    U1TXREG = c;
    return 1;
}

static size_t _serial_print(const char *str)
{
    size_t n = 0;
    while (*str) {
        _serial_write((uint8_t)*str++);
        n++;
    }
    return n;
}

static size_t _serial_println(const char *str)
{
    size_t n = _serial_print(str);
    _serial_write('\r');
    _serial_write('\n');
    return n + 2;
}

static size_t _serial_print_int(long val, int base)
{
    char buf[34];
    char *p = buf + sizeof(buf) - 1;
    *p = '\0';
    int negative = 0;

    if (val < 0 && base == 10) {
        negative = 1;
        val = -val;
    }

    unsigned long uval = (unsigned long)val;
    do {
        int digit = uval % base;
        *--p = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
        uval /= base;
    } while (uval > 0);

    if (negative) *--p = '-';

    return _serial_print(p);
}

static size_t _serial_println_int(long val, int base)
{
    size_t n = _serial_print_int(val, base);
    _serial_write('\r');
    _serial_write('\n');
    return n + 2;
}

static size_t _serial_print_float(double val, int decimals)
{
    char buf[32];
    char *p = buf;
    size_t n = 0;

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

    n = _serial_print(buf);
    return n;
}

static size_t _serial_println_float(double val, int decimals)
{
    size_t n = _serial_print_float(val, decimals);
    _serial_write('\r');
    _serial_write('\n');
    return n + 2;
}

/* ============================================================
 * Global Serial object — the "magic" that gives us dot-notation
 * ============================================================ */
HardwareSerial_t Serial = {
    .begin       = _serial_begin,
    .end         = _serial_end,
    .available   = _serial_available,
    .read        = _serial_read,
    .peek        = _serial_peek,
    .flush       = _serial_flush,
    .write       = _serial_write,
    .print       = _serial_print,
    .println     = _serial_println,
    .print_int   = _serial_print_int,
    .println_int = _serial_println_int,
    .print_float = _serial_print_float,
    .println_float = _serial_println_float,
};

#ifdef __cplusplus
}
#endif
