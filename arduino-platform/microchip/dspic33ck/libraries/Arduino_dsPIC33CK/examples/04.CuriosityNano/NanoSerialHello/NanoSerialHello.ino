/*
 * NanoSerialHello - dsPIC33CK256MC005 Curiosity Nano (EV08P02A)
 *
 * Prints a banner and a running uptime line over the on-board debugger's
 * virtual COM port. Run this second, after NanoBlink.
 *
 * NO EXTERNAL WIRING NEEDED. The nEDBG's USB-CDC bridge is already wired to
 * U1TX/U1RX on the board, so the same USB cable that uploads carries Serial.
 *
 * Open Tools > Serial Monitor and set the baud rate to 115200.
 *
 * IF YOU SEE NOTHING: programming wedges the on-board debugger's CDC bridge, so
 * the upload recipe reboots the debugger for you and waits for USB to
 * re-enumerate. That needs pymcuprog on PATH ("pip install pymcuprog"); the
 * upload prints a note if it is missing. Failing that, unplug and replug.
 *
 * Hardware:
 *   U1TX = RC10 = D31  -> nEDBG CDC RX
 *   U1RX = RC11 = D32  -> nEDBG CDC TX
 *   LED0 = RD10 = D37, active low (blinks so you can see the sketch is alive)
 *
 * Board: Tools > Board > "Arduino_dsPIC33CK (dsPIC33CK256MC005 Curiosity Nano)"
 */

#include <Arduino.h>

#if defined(LED_BUILTIN_ACTIVE_LOW) && LED_BUILTIN_ACTIVE_LOW
  #define LED_ON   LOW
  #define LED_OFF  HIGH
#else
  #define LED_ON   HIGH
  #define LED_OFF  LOW
#endif

static unsigned long g_ticks = 0;

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LED_OFF);

    Serial.begin(115200);

    Serial.println();
    Serial.println("=== dsPIC33CK256MC005 Curiosity Nano ===");
    Serial.print  ("FCY        : ");
    Serial.print(FCY / 1000UL);
    Serial.println(" kHz");
    Serial.print  ("Serial     : 115200 baud over nEDBG CDC");
    Serial.println();
    Serial.print  ("LED_BUILTIN: D");
    Serial.println(LED_BUILTIN);
    Serial.println("ready");
}

void loop()
{
    /* Blink on every pass so a dark LED distinguishes "sketch stopped" from
     * "serial is wedged". */
    digitalWrite(LED_BUILTIN, LED_ON);
    delay(100);
    digitalWrite(LED_BUILTIN, LED_OFF);

    Serial.print  ("tick=");
    Serial.print(g_ticks);
    Serial.print  ("  uptime_ms=");
    Serial.println(millis());

    g_ticks++;
    delay(900);
}
