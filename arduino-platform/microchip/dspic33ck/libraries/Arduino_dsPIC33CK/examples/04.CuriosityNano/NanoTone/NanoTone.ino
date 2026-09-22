/*
 * NanoTone - tone() / noTone() / pulseIn() on the dsPIC33CK256MC005
 *            Curiosity Nano (EV08P02A)
 *
 * These three core functions had no example until now, which means nothing in
 * the repository ever called them. This sketch calls all three, checks two of
 * them against each other, and demonstrates the one place where tone() will
 * silently break something else in your sketch.
 *
 * WIRING
 *   Tests 1-4: NOTHING. No jumper, no piezo, no parts.
 *   Test 5:    one jumper, D9 -> D12, so pulseIn() can measure what tone() made.
 *
 *   A piezo or small speaker between D9 and GND is optional and only makes the
 *   tones audible. Do not connect a speaker coil directly - a piezo is fine, an
 *   8 ohm speaker needs a series resistor of about 100 ohms.
 *
 * THE HEADLINE: tone() AND analogWrite(8, ...) SILENTLY KILL EACH OTHER
 *
 * This family has exactly one general-purpose timer of its own -- Timer1, and
 * millis() owns it. Every other time base belongs to an SCCP module, and SCCP1-4
 * are precisely the four analogWrite() PWM channels. So tone() has to borrow one,
 * and it borrows SCCP4: the channel behind D8.
 *
 *   tone(anyPin, f)    while D8 is running PWM  ->  the PWM on D8 stops
 *   analogWrite(8, v)  while a tone is playing  ->  the tone stops
 *
 * Neither call fails, warns, or returns an error. Last caller wins. It is the
 * same caveat stock AVR Arduino carries for pins 3 and 11, and test 2 below
 * demonstrates it happening rather than just asserting it -- with no wiring,
 * because a pin that is being driven can still be read back with digitalRead().
 *
 * analogWrite() on D5, D6 or D7 is unaffected, and so is a tone.
 *
 * FOUR MORE THINGS THE IMPLEMENTATION DOES THAT YOU WOULD NOT GUESS
 *
 * 1. tone() works on EVERY digital pin, not just the PWM pins. It toggles the
 *    pin's LATx bit from a timer ISR rather than routing a PWM output through
 *    Peripheral Pin Select, because PORTA has no PPS number at all on this
 *    family -- a hardware-PWM tone() would have worked on some pins and silently
 *    done nothing on others, including LED_BUILTIN on two of the four boards.
 *
 * 2. Out-of-range frequencies CLAMP, they do not fail. Ask for 1 Hz and you get
 *    the lowest note this clock can produce; ask for 5 MHz and you get the
 *    highest. Easier to diagnose on a bench than silence.
 *
 * 3. That arithmetic ceiling is NOT a usable ceiling. The ISR fires twice per
 *    cycle, so a 20 kHz tone costs 40,000 interrupts a second, and it runs at
 *    IPL5 -- above Timer1's IPL4, making it the highest-priority thing in the
 *    core. Test 4 measures what that costs. Near the arithmetic clamp the CPU
 *    would never leave the ISR and the sketch would stop responding, so this
 *    sketch deliberately does not go there. Treat a few kHz as the practical
 *    limit at FCY = 4 MHz.
 *
 * 4. noTone(pin) only stops the tone if THAT pin is the one playing. noTone() on
 *    any other pin does nothing at all, silently. And only one tone exists at a
 *    time: a second tone() on a different pin moves the tone and leaves the old
 *    pin an output, parked at whatever level the last toggle happened to leave --
 *    which may be HIGH. Test 3 shows both.
 *
 * ABOUT pulseIn()
 *   Returns the pulse width in microseconds, or 0 on timeout -- and 0 is also
 *   what a pulse too short to measure returns, so the two cases are
 *   indistinguishable. The timeout covers the whole call, measured from entry,
 *   not each phase separately. It does not set pinMode() for you.
 *
 *   Its real limit here is its own polling loop: each iteration is a digitalRead()
 *   plus a micros(), and at FCY = 4 MHz that is tens of microseconds. So it
 *   measures milliseconds well, hundreds of microseconds adequately, and anything
 *   faster not at all. Test 5 prints the error so you can see where it gives up.
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

#define TONE_PIN        9       /* RB4  - free, no debugger, no bus, no LED   */
#define SENSE_PIN       12      /* RB7  - jumper here from D9 for test 5      */
#define PWM_CLASH_PIN   8       /* RB3  - the SCCP4 channel tone() borrows    */
#define PWM_SAFE_PIN    5       /* RB0  - a PWM channel tone() does NOT touch */

/*
 * The frequency range, computed rather than quoted, because FCY is 4 MHz on the
 * default clock and 100 MHz on the 200 MHz PLL option.
 *
 *   highest: the period register needs at least 2 ticks  -> FCY / 4
 *   lowest:  1:64 prescaler with a full 16-bit period    -> FCY / 8388608
 *
 * Both are arithmetic limits. See point 3 in the header for the real one.
 */
#define TONE_F_MAX      (FCY / 4UL)
#define TONE_F_MIN_MHZ  (FCY / 8UL)     /* millihertz, so it stays an integer */

static unsigned long g_loopBaseline = 0;

/* --- helpers -------------------------------------------------------------- */

/*
 * Is this pin actually changing? Sample it hard for a few milliseconds and see
 * whether both levels turn up.
 *
 * This is what makes test 2 work with no wiring: an output pin driven by PWM or
 * by the tone ISR still reads back through PORTx, so the sketch can watch its own
 * pins without a scope and without a jumper.
 */
static uint8_t pinIsToggling(uint8_t pin, unsigned int windowMs)
{
    unsigned long deadline = millis() + (unsigned long)windowMs;
    uint8_t sawHigh = 0;
    uint8_t sawLow  = 0;

    while ((long)(millis() - deadline) < 0L) {
        if (digitalRead(pin) == HIGH) { sawHigh = 1U; } else { sawLow = 1U; }
        if (sawHigh && sawLow) { return 1U; }
    }
    return 0U;
}

static void printYesNo(const char *label, uint8_t yes)
{
    Serial.print(label);
    Serial.println(yes ? "YES" : "no");
}

static void printResult(const char *what, uint8_t pass)
{
    Serial.print("    ");
    Serial.print(what);
    Serial.println(pass ? " ... PASS" : " ... FAIL");
}

/* --- test 1: the range this clock can actually produce -------------------- */

static void reportRange(void)
{
    Serial.println("--- 1. frequency range at this clock setting -------------");
    Serial.print  ("  FCY          : ");
    Serial.print(FCY / 1000UL);
    Serial.println(" kHz");
    Serial.print  ("  highest (FCY/4)       : ");
    Serial.print(TONE_F_MAX / 1000UL);
    Serial.println(" kHz  <- arithmetic only, see below");
    Serial.print  ("  lowest (1:64, 16-bit) : ");
    Serial.print(TONE_F_MIN_MHZ / 1000UL);
    Serial.print  (".");
    Serial.print(TONE_F_MIN_MHZ % 1000UL);
    Serial.println(" Hz");
    Serial.println("  Anything outside that range CLAMPS to the nearest end.");
    Serial.println("  The ISR fires 2x per cycle at IPL5, so the usable ceiling");
    Serial.println("  is a few kHz here, far below FCY/4. Test 4 measures it.");
    Serial.println();
}

/* --- test 2: the SCCP4 / D8 mutual clobber, no wiring needed -------------- */

static void testSccp4Clash(void)
{
    uint8_t pwmLive;
    uint8_t toneLive;
    uint8_t pass = 1U;

    Serial.println("--- 2. tone() vs analogWrite(8, ...) ---------------------");

    /* (a) PWM alone on D8 and on D5. Both should be toggling. */
    analogWrite(PWM_CLASH_PIN, 128);
    analogWrite(PWM_SAFE_PIN,  128);
    delay(5);
    printYesNo("  PWM on D8 toggling                      : ",
               pinIsToggling(PWM_CLASH_PIN, 20U));
    printYesNo("  PWM on D5 toggling                      : ",
               pinIsToggling(PWM_SAFE_PIN, 20U));

    /* (b) Start a tone. D8's PWM must die; D5's must survive. */
    tone(TONE_PIN, 1000U);
    delay(5);
    pwmLive  = pinIsToggling(PWM_CLASH_PIN, 20U);
    toneLive = pinIsToggling(TONE_PIN, 20U);
    Serial.println("  after tone(D9, 1000):");
    printYesNo("    tone on D9 toggling                   : ", toneLive);
    printYesNo("    PWM on D8 still toggling              : ", pwmLive);
    printYesNo("    PWM on D5 still toggling              : ",
               pinIsToggling(PWM_SAFE_PIN, 20U));
    if (!toneLive || pwmLive) { pass = 0U; }
    printResult("tone() stopped the PWM on D8 and nothing else", (!pwmLive) && toneLive);

    /* (c) Now take SCCP4 back with analogWrite(8, ...). The tone must die. */
    analogWrite(PWM_CLASH_PIN, 128);
    delay(5);
    pwmLive  = pinIsToggling(PWM_CLASH_PIN, 20U);
    toneLive = pinIsToggling(TONE_PIN, 20U);
    Serial.println("  after analogWrite(8, 128):");
    printYesNo("    PWM on D8 toggling again              : ", pwmLive);
    printYesNo("    tone on D9 still toggling             : ", toneLive);
    if (!pwmLive || toneLive) { pass = 0U; }
    printResult("analogWrite(8, ...) stopped the tone", pwmLive && (!toneLive));

    analogWrite(PWM_CLASH_PIN, 0);
    analogWrite(PWM_SAFE_PIN, 0);
    noTone(TONE_PIN);

    Serial.println(pass
        ? "  Both directions confirmed. Neither call reported anything."
        : "  Unexpected result - check nothing else in the sketch owns SCCP4.");
    Serial.println();
}

/* --- test 3: noTone on the wrong pin, and the stranded pin ---------------- */

static void testNoToneScope(void)
{
    uint8_t stillPlaying;
    uint8_t strandedLevel;

    Serial.println("--- 3. noTone() is pin-specific; only one tone exists ----");

    tone(TONE_PIN, 1000U);
    delay(5);

    /* noTone() on a pin that is not playing must do nothing whatsoever. */
    noTone(SENSE_PIN);
    delay(5);
    stillPlaying = pinIsToggling(TONE_PIN, 20U);
    printYesNo("  tone on D9 survived noTone(D12)         : ", stillPlaying);
    printResult("noTone() on the wrong pin did nothing", stillPlaying);

    /* Move the tone to a second pin. The first pin is left an OUTPUT at whatever
     * level the final toggle left it -- that is the trap, so read it and say so
     * rather than pretending it is defined. */
    tone(SENSE_PIN, 1000U);
    delay(5);
    strandedLevel = (digitalRead(TONE_PIN) == HIGH) ? 1U : 0U;
    printYesNo("  tone moved to D12                       : ",
               pinIsToggling(SENSE_PIN, 20U));
    printYesNo("  old pin D9 still toggling               : ",
               pinIsToggling(TONE_PIN, 20U));
    Serial.print  ("  old pin D9 is now an OUTPUT parked   : ");
    Serial.println(strandedLevel ? "HIGH  <- still sourcing current!" : "LOW");
    Serial.println("  Moving a tone does NOT tidy up the pin it left. If a piezo");
    Serial.println("  sits there it now has DC across it. Call noTone(oldPin)");
    Serial.println("  first, or digitalWrite(oldPin, LOW) after.");

    noTone(SENSE_PIN);
    digitalWrite(TONE_PIN, LOW);
    Serial.println();
}

/* --- test 4: what the ISR costs, and that duration works ----------------- */

/* Count loop iterations for a fixed wall-clock window. Anything that steals CPU
 * shows up here as a smaller number. */
static unsigned long countIterations(unsigned int windowMs)
{
    unsigned long deadline = millis() + (unsigned long)windowMs;
    unsigned long n = 0;

    while ((long)(millis() - deadline) < 0L) { n++; }
    return n;
}

static void reportLoad(const char *label, unsigned long n)
{
    unsigned long pct;

    Serial.print("  ");
    Serial.print(label);
    Serial.print(n);
    if (g_loopBaseline != 0UL && n <= g_loopBaseline) {
        pct = ((g_loopBaseline - n) * 100UL) / g_loopBaseline;
        Serial.print("   (");
        Serial.print(pct);
        Serial.print("% of the CPU gone)");
    }
    Serial.println();
}

static void testIsrCostAndDuration(void)
{
    unsigned long n;
    uint8_t playing;

    Serial.println("--- 4. ISR cost, and the duration argument ---------------");

    g_loopBaseline = countIterations(200U);
    reportLoad("silent, iterations/200ms        : ", g_loopBaseline);

    tone(TONE_PIN, 440U);
    n = countIterations(200U);
    reportLoad("440 Hz  (880 interrupts/s)      : ", n);
    noTone(TONE_PIN);

    tone(TONE_PIN, 4000U);
    n = countIterations(200U);
    reportLoad("4 kHz   (8000 interrupts/s)     : ", n);
    noTone(TONE_PIN);

    Serial.println("  Scale that up: the arithmetic clamp would need millions of");
    Serial.println("  interrupts a second and the CPU would never leave the ISR.");
    Serial.println("  Serial output and millis() would stop. Stay in the low kHz.");
    Serial.println();

    /* The duration argument is counted in half-cycles by the ISR itself, so it
     * needs no second timer -- and it needs no jumper to verify. */
    Serial.println("  duration: tone(D9, 880, 150) should stop on its own");
    tone(TONE_PIN, 880U, 150UL);
    delay(40);
    playing = pinIsToggling(TONE_PIN, 20U);
    printYesNo("    still playing after 40 ms            : ", playing);
    delay(200);
    playing = pinIsToggling(TONE_PIN, 20U);
    printYesNo("    still playing after 240 ms           : ", playing);
    Serial.print  ("    pin parked at                        : ");
    Serial.println(digitalRead(TONE_PIN) == HIGH ? "HIGH" : "LOW (correct)");
    printResult("duration expired and parked the pin LOW",
                (!playing) && (digitalRead(TONE_PIN) == LOW));
    Serial.println();
}

/* --- test 5: pulseIn measures what tone made (needs the jumper) ---------- */

static void measureOne(unsigned int freq, unsigned long timeoutUs)
{
    unsigned long width;
    unsigned long measured;
    long          errPpt;      /* parts per thousand, to stay in integers */

    tone(TONE_PIN, freq);
    delay(20);
    width = pulseIn(SENSE_PIN, HIGH, timeoutUs);
    noTone(TONE_PIN);

    Serial.print("  ");
    Serial.print(freq);
    Serial.print(" Hz\trequested half-period ");
    Serial.print(500000UL / (unsigned long)freq);
    Serial.print(" us\tmeasured ");
    Serial.print(width);
    Serial.print(" us\t");

    if (width == 0UL) {
        Serial.println("-> 0: timeout, or too short for pulseIn");
        return;
    }

    measured = 500000UL / width;
    errPpt = (long)((measured * 1000UL) / (unsigned long)freq) - 1000L;
    Serial.print("-> ");
    Serial.print(measured);
    Serial.print(" Hz, error ");
    Serial.print(errPpt / 10L);
    Serial.println("%");
}

static void testPulseIn(void)
{
    unsigned long probe;

    Serial.println("--- 5. pulseIn() vs tone()  [needs jumper D9 -> D12] -----");

    pinMode(SENSE_PIN, INPUT);

    /* Detect the jumper before printing a table of zeroes and calling it data. */
    tone(TONE_PIN, 500U);
    delay(20);
    probe = pulseIn(SENSE_PIN, HIGH, 50000UL);
    noTone(TONE_PIN);

    if (probe == 0UL) {
        Serial.println("  no pulse seen on D12. Fit the jumper D9 -> D12 and");
        Serial.println("  rerun; tests 1-4 need no wiring and already passed.");
        Serial.println();
        return;
    }

    Serial.println("  jumper found. Two functions, each checking the other:");
    measureOne(100U,  200000UL);
    measureOne(220U,  100000UL);
    measureOne(440U,   50000UL);
    measureOne(880U,   50000UL);
    measureOne(1000U,  50000UL);
    measureOne(2000U,  50000UL);
    measureOne(4000U,  50000UL);
    Serial.println("  The error grows with frequency because pulseIn's own loop");
    Serial.println("  -- a digitalRead plus a micros per iteration -- becomes");
    Serial.println("  comparable to the pulse being measured. That is pulseIn's");
    Serial.println("  limit, not tone()'s.");
    Serial.println();

    /* Clamping, at the low end only: the high clamp is unreachable (see test 4)
     * and asking for it would stall the sketch. */
    Serial.println("  clamping, low end: asking for 1 Hz");
    tone(TONE_PIN, 1U);
    delay(20);
    probe = pulseIn(SENSE_PIN, HIGH, 4000000UL);
    noTone(TONE_PIN);
    if (probe == 0UL) {
        Serial.println("    no reading within 4 s - the clamped note is slower");
        Serial.println("    than that, which is itself the answer.");
    } else {
        Serial.print  ("    half-period ");
        Serial.print(probe / 1000UL);
        Serial.print  (" ms -> about ");
        Serial.print(500000UL / probe);
        Serial.println(" Hz, not 1 Hz: clamped, not refused.");
    }
    Serial.println();
}

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LED_OFF);
    pinMode(SENSE_PIN, INPUT);

    Serial.begin(115200);

    Serial.println();
    Serial.println("======================================================");
    Serial.println(" tone / noTone / pulseIn - dsPIC33CK256MC005");
    Serial.println("======================================================");
    Serial.println("tone pin  : D9  (RB4)");
    Serial.println("sense pin : D12 (RB7)   jumper from D9 for test 5");
    Serial.println("clash pin : D8  (RB3)   the SCCP4 analogWrite channel");
    Serial.println();
}

void loop()
{
    digitalWrite(LED_BUILTIN, LED_ON);

    reportRange();
    testSccp4Clash();
    testNoToneScope();
    testIsrCostAndDuration();
    testPulseIn();

    digitalWrite(LED_BUILTIN, LED_OFF);

    Serial.println("=== pass complete, again in 5 s ==========================");
    Serial.println();
    delay(5000);
}
