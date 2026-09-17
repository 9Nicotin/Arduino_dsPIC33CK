/*
 * NanoBlink - dsPIC33CK256MC005 Curiosity Nano (EV08P02A)
 *
 * The smallest possible end-to-end test: compile, upload, blink. Run this first.
 * If the LED blinks, the whole chain works -- Arduino IDE, XC-DSC compiler, the
 * linker script, the nEDBG upload tool and the core's timing.
 *
 * NO EXTERNAL WIRING NEEDED. Uses the on-board yellow user LED only.
 *
 * Hardware:
 *   LED0 = RD10 = D37, wired ACTIVE LOW (cathode to the pin), so
 *   digitalWrite(LED_BUILTIN, LOW) turns it ON. The LED_ON/LED_OFF macros
 *   below hide that so the sketch reads the way you would expect.
 *
 * Board: Tools > Board > "Arduino_dsPIC33CK (dsPIC33CK256MC005 Curiosity Nano)"
 */

#include <Arduino.h>

/* The Nano's user LED sinks current into the pin, so the logic is inverted.
 * The variant header advertises this as LED_BUILTIN_ACTIVE_LOW. */
#if defined(LED_BUILTIN_ACTIVE_LOW) && LED_BUILTIN_ACTIVE_LOW
  #define LED_ON   LOW
  #define LED_OFF  HIGH
#else
  #define LED_ON   HIGH
  #define LED_OFF  LOW
#endif

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
}

void loop()
{
    digitalWrite(LED_BUILTIN, LED_ON);
    delay(500);
    digitalWrite(LED_BUILTIN, LED_OFF);
    delay(500);
}
