/*
 * HardwareSerial.h - UART Serial for dsPIC33CK Arduino core
 *
 * Provides Arduino-style Serial object using struct + function pointers.
 * Usage is identical to standard Arduino:
 *   Serial.begin(9600);
 *   Serial.println("Hello");
 *   Serial.print(analogRead(A0), DEC);
 */

#ifndef HARDWARE_SERIAL_H
#define HARDWARE_SERIAL_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SERIAL_BUFFER_SIZE 64

#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2

typedef struct {
    uint8_t buffer[SERIAL_BUFFER_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
} ring_buffer_t;

typedef struct {
    void   (*begin)(unsigned long baud);
    void   (*end)(void);
    int    (*available)(void);
    int    (*read)(void);
    int    (*peek)(void);
    void   (*flush)(void);
    size_t (*write)(uint8_t c);
    size_t (*print)(const char *str);
    size_t (*println)(const char *str);
    size_t (*print_int)(long val, int base);
    size_t (*println_int)(long val, int base);
    size_t (*print_float)(double val, int decimals);
    size_t (*println_float)(double val, int decimals);
} HardwareSerial_t;

extern HardwareSerial_t Serial;

#ifdef __cplusplus
}
#endif

#endif /* HARDWARE_SERIAL_H */
