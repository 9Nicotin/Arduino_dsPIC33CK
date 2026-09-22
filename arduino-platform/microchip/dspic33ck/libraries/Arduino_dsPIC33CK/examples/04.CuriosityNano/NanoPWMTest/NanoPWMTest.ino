/*
 * NanoPWMTest - all four PWM channels on the dsPIC33CK256MC005 Curiosity Nano
 *               (EV08P02A), plus the edge cases that surprise people
 *
 * Where NanoPWMFade just fades one LED, this sketch is the reference for what
 * analogWrite() does and does not do on this part.
 *
 * WIRING: optional, but the whole point is visual. Any or all of:
 *   D5 (RB0) --[220-330R]--|>|-- GND
 *   D6 (RB1) --[220-330R]--|>|-- GND
 *   D7 (RB2) --[220-330R]--|>|-- GND
 *   D8 (RB3) --[220-330R]--|>|-- GND
 * A scope or logic analyser on any of the four is better still. With nothing
 * connected the sketch still runs and prints; you just cannot see the result.
 *
 * The on-board LED0 (D37 = RD10) is NOT a PWM pin and cannot be. This device
 * has SCCP1-4 only -- there is no SCCP5, unlike the MP508 Curiosity board -- and
 * the core maps those four to D5-D8. digitalPinHasPWM(p) is true for 5..8 only.
 *
 * RB0/RB1 are silkscreened OSCI/OSCO because the board's 8 MHz MEMS oscillator
 * lands there, but it is not connected unless you cut a strap and add a solder
 * blob (kit guide 5.3), so D5 and D6 are ordinary GPIO on a stock board.
 *
 * WHAT THIS SKETCH DEMONSTRATES
 *   1. The four channels are independent: four different duties at once.
 *   2. The frequency is fixed by the core at ~490 Hz and is NOT settable
 *      through the Arduino API. The arithmetic is printed so you can check it.
 *   3. Duty is 0..255, as on AVR -- even though the hardware period register
 *      holds far more steps than that. The mapping is duty*PERIOD/255.
 *   4. analogWrite(pin, 0) and analogWrite(pin, 255) do NOT emit 0% and 100%
 *      PWM. They tear the channel down and leave the pin as a plain GPIO
 *      output, LOW and HIGH respectively.
 *   5. THE ONE THAT BITES: pinMode() and digitalWrite() do NOT stop a running
 *      PWM. The SCCP output is routed to the pad through PPS, so the peripheral
 *      keeps driving it and your LAT write is invisible. The only supported way
 *      to release the pin is analogWrite(pin, 0) or analogWrite(pin, 255).
 *   6. analogWrite() on a non-PWM pin silently degrades to
 *      digitalWrite(pin, val >= 128), it does not fail and does not warn.
 *
 * Serial Monitor at 115200.
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

/* The core picks its SCCP prescaler from FCY and targets 490 Hz. Recomputed here
 * with the same formula so the sketch can print the real numbers instead of a
 * comment that goes stale. Keep in step with cores/arduino/wiring_analog.c. */
#if (FCY > 50000000UL)
  #define PWM_PRESCALER_VAL 64UL
#elif (FCY > 10000000UL)
  #define PWM_PRESCALER_VAL 16UL
#elif (FCY > 2000000UL)
  #define PWM_PRESCALER_VAL 4UL
#else
  #define PWM_PRESCALER_VAL 1UL
#endif

#define PWM_TARGET_FREQ 490UL
#define PWM_PERIOD      ((unsigned long)(FCY / (PWM_PRESCALER_VAL * PWM_TARGET_FREQ) - 1))

static const uint8_t g_pwmPins[] = { 5, 6, 7, 8 };
#define NUM_PINS (sizeof(g_pwmPins) / sizeof(g_pwmPins[0]))

static int g_step = 0;

static void banner(const char *title)
{
    Serial.println();
    Serial.print  ("-- ");
    Serial.print(title);
    Serial.println(" ------------------------------");
}

/* Duty as the core will program it, so the printed value is the truth about the
 * hardware and not just the byte that was asked for. */
static void printDuty(uint8_t pin, int val)
{
    unsigned long ticks = ((unsigned long)val * PWM_PERIOD) / 255UL;

    Serial.print  ("   D");
    Serial.print(pin);
    Serial.print  ("  duty=");
    if (val < 100) Serial.print(" ");
    if (val < 10)  Serial.print(" ");
    Serial.print(val);
    Serial.print  ("/255 = ");
    Serial.print((double)val * 100.0 / 255.0, 1);
    Serial.print  ("%  -> CCPxRB=");
    Serial.print(ticks);
    Serial.print  (" of ");
    Serial.print(PWM_PERIOD);
    Serial.print  ("  (");
    Serial.print((double)ticks * (double)PWM_PRESCALER_VAL * 1000000.0 / (double)FCY, 1);
    Serial.println(" us high)");
}

static void testFourAtOnce(void)
{
    static const int duties[NUM_PINS] = { 32, 96, 160, 224 };
    size_t i;

    banner("1. four independent channels");
    Serial.println("   Four SCCP modules, four duties, all running together.");

    for (i = 0; i < NUM_PINS; i++) {
        analogWrite(g_pwmPins[i], duties[i]);
        printDuty(g_pwmPins[i], duties[i]);
    }

    Serial.println("   Held for 4 s - measure any of them now.");
    delay(4000);
}

static void testSweep(void)
{
    int      phase;
    size_t   i;

    banner("2. duty sweep, quarter-cycle apart");
    Serial.println("   Each channel offset by 64/256, so four LEDs breathe out");
    Serial.println("   of step. Updating a running channel only rewrites CCPxRB,");
    Serial.println("   which is glitch-free - the period keeps running.");

    for (phase = 0; phase < 512; phase += 4) {
        for (i = 0; i < NUM_PINS; i++) {
            int p = (phase + (int)i * 64) & 0x1FF;
            int v = (p < 256) ? p : (511 - p);

            /* Keep away from 0 and 255: those tear the channel down, which is
             * the next test, not this one. */
            if (v < 1)   v = 1;
            if (v > 254) v = 254;
            analogWrite(g_pwmPins[i], v);
        }
        delay(8);
    }
}

static void testEndpoints(void)
{
    banner("3. duty 0 and 255 are not PWM");
    Serial.println("   analogWrite(pin,0)   -> channel torn down, pin driven LOW");
    Serial.println("   analogWrite(pin,255) -> channel torn down, pin driven HIGH");
    Serial.println("   analogWrite(pin,1)   -> real PWM, the narrowest pulse");
    Serial.println("   analogWrite(pin,254) -> real PWM, the widest pulse");
    Serial.println();

    analogWrite(5, 0);
    Serial.println("   D5 = 0    (static LOW, no switching on a scope)");
    analogWrite(6, 1);
    printDuty(6, 1);
    analogWrite(7, 254);
    printDuty(7, 254);
    analogWrite(8, 255);
    Serial.println("   D8 = 255  (static HIGH, no switching on a scope)");

    Serial.println("   Held for 4 s.");
    delay(4000);
}

static void testPinModeDoesNotStopPWM(void)
{
    banner("4. pinMode() does NOT stop PWM");

    analogWrite(5, 128);
    Serial.println("   D5 at 50%. Now calling:");
    Serial.println("     pinMode(5, OUTPUT);");
    Serial.println("     digitalWrite(5, LOW);");

    pinMode(5, OUTPUT);
    digitalWrite(5, LOW);

    Serial.println("   ...and D5 is STILL switching at 50%. The SCCP output is");
    Serial.println("   mapped onto the pad through PPS (RPORn), so the LAT bit");
    Serial.println("   the core just wrote never reaches the pin.");
    delay(3000);

    Serial.println();
    Serial.println("   The supported release is analogWrite(5, 0), which clears");
    Serial.println("   CCPON and tears the PPS mapping back down:");
    analogWrite(5, 0);
    Serial.println("     analogWrite(5, 0);   -> now a plain GPIO, LOW");

    /* And now digitalWrite really does something. */
    digitalWrite(5, HIGH);
    Serial.println("     digitalWrite(5, HIGH); -> D5 is HIGH, statically");
    delay(1500);
    digitalWrite(5, LOW);
}

static void testNonPwmPin(void)
{
    banner("5. analogWrite() on a non-PWM pin");

    Serial.print  ("   digitalPinHasPWM(5)  = ");
    Serial.println(digitalPinHasPWM(5) ? "yes" : "no");
    Serial.print  ("   digitalPinHasPWM(8)  = ");
    Serial.println(digitalPinHasPWM(8) ? "yes" : "no");
    Serial.print  ("   digitalPinHasPWM(9)  = ");
    Serial.println(digitalPinHasPWM(9) ? "yes" : "no");
    Serial.print  ("   digitalPinHasPWM(37) = ");
    Serial.println(digitalPinHasPWM(LED_BUILTIN) ? "yes" : "no");
    Serial.println();
    Serial.println("   analogWrite(37, v) on LED0 falls through to");
    Serial.println("   digitalWrite(37, v >= 128). No error, no warning, and no");
    Serial.println("   brightness control - just a threshold at half scale.");
    Serial.println("   Watch LED0: it steps rather than fades.");

    {
        int v;
        for (v = 0; v <= 255; v += 15) {
            analogWrite(LED_BUILTIN, v);
            delay(120);
        }
    }

    /* LED0 is active low, so leave it deliberately off rather than wherever the
     * threshold happened to land. */
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LED_OFF);
}

static void releaseAll(void)
{
    size_t i;

    for (i = 0; i < NUM_PINS; i++) {
        analogWrite(g_pwmPins[i], 0);
    }
}

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LED_OFF);

    Serial.begin(115200);

    Serial.println();
    Serial.println("======================================================");
    Serial.println(" PWM test - dsPIC33CK256MC005 Curiosity Nano");
    Serial.println("======================================================");
    Serial.print  ("FCY            : ");
    Serial.print(FCY / 1000UL);
    Serial.println(" kHz");
    Serial.print  ("SCCP prescaler : 1:");
    Serial.println(PWM_PRESCALER_VAL);
    Serial.print  ("period register: ");
    Serial.println(PWM_PERIOD);
    Serial.print  ("actual freq    : ");
    Serial.print((double)FCY / ((double)PWM_PRESCALER_VAL * (double)(PWM_PERIOD + 1UL)), 2);
    Serial.print  (" Hz   (target ");
    Serial.print(PWM_TARGET_FREQ);
    Serial.println(" Hz)");
    Serial.print  ("period         : ");
    Serial.print((double)(PWM_PERIOD + 1UL) * (double)PWM_PRESCALER_VAL * 1000.0 / (double)FCY, 3);
    Serial.println(" ms");
    Serial.print  ("duty steps     : 256 asked for, ");
    Serial.print(PWM_PERIOD + 1UL);
    Serial.println(" available in hardware");
    Serial.print  ("pwm pins       : D5 D6 D7 D8   (NUM_PWM_PINS = ");
    Serial.print(NUM_PWM_PINS);
    Serial.println(")");
    Serial.println("mapping        : D5=SCCP1 D6=SCCP2 D7=SCCP3 D8=SCCP4");
    Serial.println();
    Serial.println("The frequency is not adjustable through analogWrite().");
    Serial.println("For a settable frequency, drive the SCCP registers directly");
    Serial.println("or use the HRPWM library's PWM generators.");
}

void loop()
{
    Serial.println();
    Serial.print  ("################ pass ");
    Serial.print(++g_step);
    Serial.println(" ################");

    testFourAtOnce();
    testSweep();
    testEndpoints();
    testPinModeDoesNotStopPWM();
    testNonPwmPin();

    releaseAll();

    Serial.println();
    Serial.println("pass complete - all four channels released, repeating in 3 s");
    delay(3000);
}
