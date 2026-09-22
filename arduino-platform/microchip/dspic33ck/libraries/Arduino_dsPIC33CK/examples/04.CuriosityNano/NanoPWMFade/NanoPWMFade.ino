/*
 * NanoPWMFade - dsPIC33CK256MC005 Curiosity Nano (EV08P02A)
 *
 * Fades an external LED up and down with analogWrite(), and reports the duty
 * over Serial.
 *
 * WIRING REQUIRED -- this is the one sketch here that needs a part:
 *   D5 (RB0) --- [220-330 ohm resistor] --- LED anode
 *   LED cathode --- GND
 * Any of D5, D6, D7, D8 works; change PWM_PIN below.
 *
 * The on-board LED0 (D37 = RD10) CANNOT be used here: this part has only
 * SCCP1-4, whose outputs the core maps to D5-D8, and RD10 is not one of them.
 * digitalPinHasPWM(p) is true only for pins 5..8 on this board.
 *
 * Hardware:
 *   D5 = RB0 (SCCP1), D6 = RB1 (SCCP2), D7 = RB2 (SCCP3), D8 = RB3 (SCCP4)
 *   PWM frequency is set by the core (~490 Hz, matching standard Arduino).
 *   Duty is 0-255, as on standard Arduino.
 *
 * Note on RB0/RB1: the Nano's headers label these OSCI/OSCO because the board's
 * 8 MHz MEMS oscillator lands there. It is NOT connected by default (that needs
 * a strap cut and a solder blob, see the kit guide 5.3), so RB0/RB1 are free
 * GPIO on an unmodified board.
 *
 * Serial Monitor at 115200. (The upload recipe reboots the on-board debugger
 * afterwards, which is what keeps the COM port alive -- see NanoSerialHello.)
 *
 * Board: Tools > Board > "Arduino_dsPIC33CK (dsPIC33CK256MC005 Curiosity Nano)"
 */

#include <Arduino.h>

#define PWM_PIN     5      /* D5 = RB0 = SCCP1 */
#define STEP        5
#define STEP_DELAY  20

static int g_duty = 0;
static int g_dir  = STEP;

void setup()
{
    Serial.begin(115200);
    Serial.println();
    Serial.println("=== analogWrite() fade ===");
    Serial.print  ("PWM output on D");
    Serial.print(PWM_PIN);
    Serial.println(" -- connect an LED + 220-330 ohm resistor to GND");

    if (!digitalPinHasPWM(PWM_PIN)) {
        /* Guards against editing PWM_PIN to something the part cannot drive. */
        Serial.println("ERROR: that pin has no PWM on this device (use D5-D8)");
    }
}

void loop()
{
    analogWrite(PWM_PIN, g_duty);

    /* Only log at the turning points, so the monitor stays readable. */
    if (g_duty == 0 || g_duty >= 255) {
        Serial.print  ("duty=");
        Serial.println(g_duty);
    }

    g_duty += g_dir;
    if (g_duty >= 255) {
        g_duty = 255;
        g_dir  = -STEP;
    } else if (g_duty <= 0) {
        g_duty = 0;
        g_dir  = STEP;
    }

    delay(STEP_DELAY);
}
