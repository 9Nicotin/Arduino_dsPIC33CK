/*
 * Blink - dsPIC33CK32MP102 Arduino Example
 *
 * Blinks the LED on pin D0 (RA0) every 1 second.
 *
 * Hardware:
 *   - Connect LED + 330 ohm resistor between RA0 and GND
 *   - Or use onboard LED if your board has one on RA0
 *
 * Pin Mapping:
 *   LED_BUILTIN = D0 = RA0
 */

#include <Arduino.h>

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
}

void loop()
{
    digitalWrite(LED_BUILTIN, HIGH);   // LED ON
    delay(1000);                       // Wait 1 second
    digitalWrite(LED_BUILTIN, LOW);    // LED OFF
    delay(1000);                       // Wait 1 second
}
