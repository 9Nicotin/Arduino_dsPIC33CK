/*
 * NanoButtonLED - dsPIC33CK256MC005 Curiosity Nano (EV08P02A)
 *
 * The on-board switch SW0 controls the on-board LED, and every press/release
 * edge is reported over Serial with the press duration.
 *
 * NO EXTERNAL WIRING NEEDED.
 *
 * This sketch exercises INPUT_PULLUP specifically, which matters on this board:
 * there is NO external pull-up resistor on SW0, so the internal one is not
 * optional. If digitalRead() reports the button as permanently pressed, the
 * pin is floating rather than pulled up -- a floating input reads LOW.
 *
 * Hardware:
 *   SW0  = RD13 = D38, internal pull-up required, reads LOW when pressed
 *   LED0 = RD10 = D37, active low
 *
 * Serial Monitor at 115200. (The upload recipe reboots the on-board debugger
 * afterwards, which is what keeps the COM port alive -- see NanoSerialHello.)
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

/* SW0 shorts the pin to GND, so pressed == LOW. */
#define BUTTON_PRESSED   LOW

#define DEBOUNCE_MS      25

static int           g_stable_state  = HIGH;   /* debounced button state */
static int           g_last_reading  = HIGH;
static unsigned long g_last_change   = 0;
static unsigned long g_press_started = 0;

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LED_OFF);

    /* Without INPUT_PULLUP this pin floats and the button reads as held. */
    pinMode(BUTTON_BUILTIN, INPUT_PULLUP);

    Serial.begin(115200);
    Serial.println();
    Serial.println("=== SW0 -> LED0 test ===");
    Serial.println("Press SW0. The LED follows the button.");

    g_stable_state = digitalRead(BUTTON_BUILTIN);
    g_last_reading = g_stable_state;

    Serial.print("initial SW0 reading: ");
    Serial.println(g_stable_state == BUTTON_PRESSED ? "DOWN (suspect - is the"
                   " pull-up working?)" : "up (correct when released)");
}

void loop()
{
    int reading = digitalRead(BUTTON_BUILTIN);

    /* Simple debounce: only accept a level that has held for DEBOUNCE_MS. */
    if (reading != g_last_reading) {
        g_last_reading = reading;
        g_last_change  = millis();
    }
    else if (reading != g_stable_state &&
             (millis() - g_last_change) >= DEBOUNCE_MS) {
        g_stable_state = reading;

        if (g_stable_state == BUTTON_PRESSED) {
            g_press_started = millis();
            Serial.println("SW0 pressed");
        } else {
            Serial.print  ("SW0 released after ");
            Serial.print(millis() - g_press_started);
            Serial.println(" ms");
        }
    }

    digitalWrite(LED_BUILTIN,
                 (g_stable_state == BUTTON_PRESSED) ? LED_ON : LED_OFF);
}
