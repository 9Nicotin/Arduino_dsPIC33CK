/*
 * NanoSelfTest - dsPIC33CK256MC005 Curiosity Nano (EV08P02A)
 *
 * One sketch that exercises everything the board can test without extra parts:
 * digital output, digital input with the internal pull-up, the ADC, the
 * millis()/micros() timebase, and Serial over the debugger's CDC. It prints a
 * one-shot report in setup(), then a live status line once a second.
 *
 * NO EXTERNAL WIRING NEEDED.
 *
 * What to look for:
 *   - the banner appears at all         -> IDE, compiler, linker, upload, Serial
 *   - "FCY : 4000 kHz"                  -> clock is configured as expected
 *   - LED0 blinks ~1 Hz                 -> digitalWrite + delay
 *   - sw0 reads "up" when released      -> INPUT_PULLUP works (see below)
 *   - sw0 changes to "DOWN" when held   -> digitalRead works
 *   - a0 varies                         -> ADC converts
 *   - uptime tracks a wall clock        -> timebase is roughly right
 *
 * WHY THE PULL-UP LINE MATTERS: SW0 on this board has no external pull-up, so
 * INPUT_PULLUP is mandatory. If sw0 reports DOWN while nothing is pressed, the
 * pin is floating (a floating input reads LOW), which means the internal pull-up
 * did not get enabled for this port.
 *
 * Serial Monitor at 115200.
 *
 * An upload leaves the on-board debugger's USB-CDC bridge wedged, so the upload
 * recipe reboots the debugger for you and waits for USB to re-enumerate; that
 * needs pymcuprog on PATH ("pip install pymcuprog"). If the monitor is silent
 * anyway, unplug and replug the board.
 *
 * The banner only prints once, in setup(), so it is normally gone by the time
 * the monitor opens. To catch it, open the monitor first, then reset the target
 * only -- that leaves the CDC bridge alive:
 *     pymcuprog setsupplyvoltage -l 0
 *     pymcuprog setsupplyvoltage -l 3.3
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

#define BUTTON_PRESSED  LOW

#define ADC_MAX     1023
#define VREF_VOLTS  3.3

static unsigned long g_beat = 0;
static int           g_adc_min = ADC_MAX;
static int           g_adc_max = 0;

/* Rough check that the timebase is not wildly off. This is partly
 * self-referential -- millis() and delay() share a timebase -- so it catches a
 * gross misconfiguration, not a few percent of error. Compare the printed
 * uptime against a wall clock for the real answer. */
static void report_timebase(void)
{
    unsigned long t0, t1;

    t0 = millis();
    delay(200);
    t1 = millis();

    Serial.print  ("delay(200)  : measured ");
    Serial.print(t1 - t0);
    Serial.println(" ms by millis()");
}

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LED_OFF);
    pinMode(BUTTON_BUILTIN, INPUT_PULLUP);

    Serial.begin(115200);

    Serial.println();
    Serial.println("========================================");
    Serial.println(" Arduino_dsPIC33CK self-test");
    Serial.println("========================================");

    Serial.println("board      : EV08P02A / dsPIC33CK256MC005");

    Serial.print  ("FCY        : ");
    Serial.print(FCY / 1000UL);
    Serial.println(" kHz");

    Serial.print  ("pins       : LED0=D");
    Serial.print(LED_BUILTIN);
    Serial.print  ("  SW0=D");
    Serial.print(BUTTON_BUILTIN);
    Serial.print  ("  A0=D");
    Serial.println(A0);

    Serial.print  ("digital pins: ");
    Serial.print(NUM_DIGITAL_PINS);
    Serial.print  ("   analog inputs: ");
    Serial.print(NUM_ANALOG_INPUTS);
    Serial.print  ("   pwm pins: ");
    Serial.println(NUM_PWM_PINS);

    report_timebase();

    Serial.print  ("SW0 at boot : ");
    Serial.println(digitalRead(BUTTON_BUILTIN) == BUTTON_PRESSED
                   ? "DOWN  <-- unexpected if you are not holding it"
                   : "up    (pull-up OK)");

    Serial.println("----------------------------------------");
    Serial.println("ready - press SW0 to see it change");
}

void loop()
{
    int          sw0     = digitalRead(BUTTON_BUILTIN);
    int          pressed = (sw0 == BUTTON_PRESSED);
    int          a0      = analogRead(A0);
    double       volts   = (double)a0 * VREF_VOLTS / (double)ADC_MAX;

    if (a0 < g_adc_min) g_adc_min = a0;
    if (a0 > g_adc_max) g_adc_max = a0;

    /* Blink once per pass, inverted while the button is held, so the LED gives
     * independent visual confirmation that the button is being read. */
    digitalWrite(LED_BUILTIN, pressed ? LED_OFF : LED_ON);
    delay(100);
    digitalWrite(LED_BUILTIN, pressed ? LED_ON : LED_OFF);

    Serial.print  ("beat=");
    Serial.print(g_beat);
    Serial.print  ("  sw0=");
    Serial.print  (pressed ? "DOWN" : "up  ");
    Serial.print  ("  a0=");
    Serial.print(a0);
    Serial.print  (" (");
    Serial.print(volts, 2);
    Serial.print  ("V, seen ");
    Serial.print(g_adc_min);
    Serial.print  ("..");
    Serial.print(g_adc_max);
    Serial.print  (")  uptime=");
    Serial.print(millis() / 1000UL);
    Serial.println("s");

    g_beat++;
    delay(900);
}
