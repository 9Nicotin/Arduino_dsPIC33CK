/*
 * Arduino.h - Main Arduino API header for dsPIC33CK
 *
 * Provides the standard Arduino functions:
 *   - Digital I/O: pinMode, digitalWrite, digitalRead
 *   - Analog I/O: analogRead, analogWrite (PWM)
 *   - Timing: millis, micros, delay, delayMicroseconds
 *   - Math/Bit: min, max, abs, constrain, map, bit ops
 *   - Serial communication via HardwareSerial
 */

#ifndef ARDUINO_H
#define ARDUINO_H

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * Constants
 * ============================================================ */

#define HIGH    1
#define LOW     0

#define INPUT           0
#define OUTPUT          1
#define INPUT_PULLUP    2

#define PI          3.1415926535897932384626433832795
#define HALF_PI     1.5707963267948966192313216916398
#define TWO_PI      6.283185307179586476925286766559
#define DEG_TO_RAD  0.017453292519943295769236907684886
#define RAD_TO_DEG  57.295779513082320876798154814105
#define EULER       2.718281828459045235360287471352

#define SERIAL      0x0
#define DISPLAY     0x1

#define LSBFIRST    0
#define MSBFIRST    1

#define CHANGE      1
#define FALLING     2
#define RISING      3

#define DEFAULT     0
#define EXTERNAL    1

#ifndef F_CPU
#define F_CPU       8000000UL
#endif

#ifndef FCY
#define FCY         (F_CPU / 2)
#endif

/* ============================================================
 * Macros
 * ============================================================ */

#define min(a,b)        ((a)<(b)?(a):(b))
#define max(a,b)        ((a)>(b)?(a):(b))
#define abs(x)          ((x)>0?(x):-(x))
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#define round(x)        ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))
#define radians(deg)    ((deg)*DEG_TO_RAD)
#define degrees(rad)    ((rad)*RAD_TO_DEG)
#define sq(x)           ((x)*(x))

#define lowByte(w)      ((uint8_t)((w) & 0xff))
#define highByte(w)     ((uint8_t)((w) >> 8))

#define bitRead(value, bit)         (((value) >> (bit)) & 0x01)
#define bitSet(value, bit)          ((value) |= (1UL << (bit)))
#define bitClear(value, bit)        ((value) &= ~(1UL << (bit)))
#define bitToggle(value, bit)       ((value) ^= (1UL << (bit)))
#define bitWrite(value, bit, bitvalue) ((bitvalue) ? bitSet(value, bit) : bitClear(value, bit))
#define bit(b)                      (1UL << (b))

typedef unsigned int word;
typedef uint8_t byte;
typedef bool boolean;

/* ============================================================
 * Digital I/O
 * ============================================================ */

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int  digitalRead(uint8_t pin);

/* ============================================================
 * Analog I/O
 * ============================================================ */

int  analogRead(uint8_t pin);
void analogWrite(uint8_t pin, int val);
void analogReference(uint8_t mode);

/* ============================================================
 * Timing
 * ============================================================ */

unsigned long millis(void);
unsigned long micros(void);
void delay(unsigned long ms);
void delayMicroseconds(unsigned int us);

/* ============================================================
 * Advanced I/O
 * ============================================================ */

void tone(uint8_t pin, unsigned int frequency, unsigned long duration);
void noTone(uint8_t pin);
unsigned long pulseIn(uint8_t pin, uint8_t state, unsigned long timeout);
void shiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val);
uint8_t shiftIn(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder);

/* ============================================================
 * External Interrupts
 * ============================================================ */

void attachInterrupt(uint8_t interruptNum, void (*userFunc)(void), int mode);
void detachInterrupt(uint8_t interruptNum);
void interrupts(void);
void noInterrupts(void);

/* ============================================================
 * Map function
 * ============================================================ */

long map(long value, long fromLow, long fromHigh, long toLow, long toHigh);

/* ============================================================
 * System
 * ============================================================ */

void init(void);
void setup(void);
void loop(void);

#ifdef __cplusplus
}
#endif

/* Include variant pin definitions */
#include "pins_arduino.h"

/* Include Serial object (must be after pins_arduino.h for PPS macros) */
#include "HardwareSerial.h"

#endif /* ARDUINO_H */
