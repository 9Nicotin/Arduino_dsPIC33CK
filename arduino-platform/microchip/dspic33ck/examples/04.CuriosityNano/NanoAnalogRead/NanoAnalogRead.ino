/*
 * NanoAnalogRead - dsPIC33CK256MC005 Curiosity Nano (EV08P02A)
 *
 * Reads A0 and prints the raw count, the equivalent voltage, and a text bar
 * graph. Also demonstrates map() and the float print form.
 *
 * WIRING: none required -- a floating A0 reads a drifting mid-scale value,
 * which is enough to prove the ADC converts. For a meaningful sweep, connect a
 * potentiometer:
 *   pot center -> A0 (RA0), pot ends -> VTG (3.3V) and GND
 * Touching the A0 pin with a finger also visibly moves the reading.
 *
 * Hardware:
 *   A0 = RA0 = D0 = AN0
 *
 * NOTE: analogRead() returns 0..1023. The dsPIC33CK ADC is 12-bit, but the core
 * scales the result down to 10 bits so that sketches written for standard
 * Arduino behave the same. VTG on this board is 3.3 V, not 5 V.
 *
 * Serial Monitor at 115200. (The upload recipe reboots the on-board debugger
 * afterwards, which is what keeps the COM port alive -- see NanoSerialHello.)
 *
 * Board: Tools > Board > "Arduino_dsPIC33CK (dsPIC33CK256MC005 Curiosity Nano)"
 */

#include <Arduino.h>

#define ADC_MAX      1023
#define VREF_VOLTS   3.3
#define BAR_WIDTH    32

void setup()
{
    Serial.begin(115200);
    Serial.println();
    Serial.println("=== A0 analog read ===");
    Serial.println("raw (0-1023) | volts (VTG=3.3V) | bar");
}

void loop()
{
    int    raw   = analogRead(A0);
    double volts = (double)raw * VREF_VOLTS / (double)ADC_MAX;
    long   bars  = map(raw, 0, ADC_MAX, 0, BAR_WIDTH);
    long   i;

    Serial.print("raw=");
    Serial.print(raw);

    Serial.print("  V=");
    Serial.print(volts, 3);

    Serial.print("  |");
    for (i = 0; i < BAR_WIDTH; i++) {
        Serial.print(i < bars ? "#" : " ");
    }
    Serial.println("|");

    delay(250);
}
