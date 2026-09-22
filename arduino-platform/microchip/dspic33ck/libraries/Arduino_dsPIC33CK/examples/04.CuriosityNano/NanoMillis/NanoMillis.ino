/*
 * NanoMillis - timing, rollover and map() on the dsPIC33CK256MC005
 *              Curiosity Nano (EV08P02A)
 *
 * millis(), micros(), delay() and delayMicroseconds() are the four functions every
 * sketch uses and nobody checks. This one checks them: it measures each against
 * the others, prints requested-versus-measured tables, and demonstrates the two
 * failure modes that bite real sketches -- counter rollover, and time passing
 * while interrupts are off.
 *
 * WIRING: none.
 *
 * WHAT THE TIME BASE ACTUALLY IS
 *   Timer1, and Timer1 only. It is the single general-purpose timer on this part
 *   -- SCCP1-4 are the four analogWrite() channels and SCCP4 is also tone()'s --
 *   so millis() owns it outright. Its interrupt runs at IPL4.
 *
 *   The prescaler and period are chosen from FCY at Serial-independent startup,
 *   and FCY is 4 MHz on the default clock or 100 MHz on the 200 MHz PLL. Test 1
 *   reads the registers back rather than quoting numbers, so it tells the truth
 *   whichever clock you selected.
 *
 * THE FIVE THINGS WORTH KNOWING
 *
 * 1. millis() rolls over after about 49.7 days, micros() after about 71.6
 *    minutes. 71.6 minutes is not a theoretical concern -- a sketch left on a
 *    bench over lunch will hit it. Test 5 shows the arithmetic that survives it
 *    and the arithmetic that does not, without waiting an hour to find out.
 *
 * 2. delayMicroseconds() is a calibrated NOP loop, not a timer, and its
 *    calibration is approximate. On the default clock delayMicroseconds(1)
 *    produces NO DELAY AT ALL: the function computes 4 cycles of work, subtracts
 *    nothing because the value is too small, and the loop condition
 *    "while (cycles > 4)" is false on the first test. Test 3 measures the whole
 *    curve so you can see where it becomes trustworthy.
 *
 * 3. delay() is built on millis(), so it needs interrupts. Call it with
 *    interrupts disabled and it never returns -- not "runs slowly", hangs
 *    forever. delayMicroseconds() is the one that works with interrupts off,
 *    because it counts instructions rather than ticks.
 *
 * 4. Time is LOST, not deferred, while interrupts are off. Timer1's interrupt
 *    flag latches but does not count: hold interrupts off for 5 ms and the
 *    pending flag delivers exactly one tick when you let it through, so millis()
 *    is permanently 4 ms behind. Test 6 measures the loss.
 *
 * 5. map() truncates, does not clamp, and will reset the chip if you hand it a
 *    zero-width input range. Test 7.
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

#define REPS            50U     /* amortises micros() overhead out of a measurement */
#define BLINK_MS        150UL
#define BLINK_DEMO_MS   2000UL

static unsigned long g_microsOverhead = 0;   /* one micros() call, in ns */

/* --- small printing helpers ---------------------------------------------- */

static void printResult(const char *what, uint8_t pass)
{
    Serial.print("    ");
    Serial.print(what);
    Serial.println(pass ? " ... PASS" : " ... FAIL");
}

/* Print a nanosecond count as microseconds with one decimal, no floating point. */
static void printUs(unsigned long ns)
{
    Serial.print(ns / 1000UL);
    Serial.print('.');
    Serial.print((ns / 100UL) % 10UL);
}

/* --- test 1: what the hardware is actually set to ------------------------ */

static void reportTimeBase(void)
{
    static const unsigned int prescale[4] = { 1U, 8U, 64U, 256U };
    unsigned int  ps = prescale[T1CONbits.TCKPS & 0x3U];
    unsigned long pr = (unsigned long)PR1;
    unsigned long tickNs;

    Serial.println("--- 1. the time base, read back from the registers -------");
    Serial.print  ("  FCY                    : ");
    Serial.print(FCY / 1000UL);
    Serial.println(" kHz");
    Serial.print  ("  Timer1 prescaler       : 1:");
    Serial.println(ps);
    Serial.print  ("  PR1                    : ");
    Serial.println(pr);

    /*
     * One tick should be 1 ms. Computed as (cycles * 1000) / (FCY / 1000000)
     * rather than the more obvious cycles * 1e9 / FCY, because the numerator of
     * the obvious form overflows 32 bits at both clock settings.
     */
    tickNs = ((pr + 1UL) * (unsigned long)ps * 1000UL) / (FCY / 1000000UL);
    Serial.print  ("  => tick period         : ");
    printUs(tickNs);
    Serial.println(" us  (should be 1000.0)");
    printResult("Timer1 is configured for a 1 ms tick",
                (tickNs > 995000UL) && (tickNs < 1005000UL));

    Serial.print  ("  Timer1 interrupt IPL   : ");
    Serial.print(IPC0bits.T1IP);
    Serial.println("   (tone() is IPL5 and preempts it; Serial RX is IPL3)");

    Serial.println("  millis() rolls over at 2^32 ms  = 49.7 days");
    Serial.println("  micros() rolls over at 2^32 us  = 71.6 minutes");
    Serial.println();
}

/* --- test 2: how good is micros() at measuring anything? ----------------- */

static void testMicrosResolution(void)
{
    unsigned long a;
    unsigned long b;
    unsigned long total;
    unsigned long smallest = 0xFFFFFFFFUL;
    unsigned int  i;
    unsigned int  zeroes = 0;

    Serial.println("--- 2. micros() resolution and call cost ----------------");

    /* Cost of the call itself: time REPS back-to-back calls. */
    a = micros();
    for (i = 0; i < REPS; i++) { (void)micros(); }
    b = micros();
    total = b - a;
    g_microsOverhead = (total * 1000UL) / REPS;

    Serial.print  ("  one micros() call      : ");
    printUs(g_microsOverhead);
    Serial.println(" us");

    /* Smallest non-zero step two successive calls can show. */
    for (i = 0; i < 200U; i++) {
        a = micros();
        b = micros();
        if (b == a) { zeroes++; }
        else if ((b - a) < smallest) { smallest = b - a; }
    }
    Serial.print  ("  smallest visible step  : ");
    if (smallest == 0xFFFFFFFFUL) {
        Serial.println("none - every pair read identical");
    } else {
        Serial.print(smallest);
        Serial.println(" us");
    }
    Serial.print  ("  pairs reading equal    : ");
    Serial.print(zeroes);
    Serial.println(" of 200");
    Serial.println("  Anything you measure that is not several times the call");
    Serial.println("  cost above is mostly measuring micros() itself. That is");
    Serial.println("  why every table below times a batch and divides.");
    Serial.println();
}

/* --- test 3: delayMicroseconds(), requested vs measured ------------------ */

static void measureDelayUs(unsigned int request)
{
    unsigned long t0;
    unsigned long ns;
    unsigned int  i;
    long          errPct;

    t0 = micros();
    for (i = 0; i < REPS; i++) { delayMicroseconds(request); }
    ns = ((micros() - t0) * 1000UL) / REPS;

    /* Take the call overhead out: we want the delay, not the loop around it. */
    if (ns > g_microsOverhead) { ns -= g_microsOverhead; } else { ns = 0UL; }

    Serial.print  ("  requested ");
    if (request < 10U)   { Serial.print(' '); }
    if (request < 100U)  { Serial.print(' '); }
    if (request < 1000U) { Serial.print(' '); }
    Serial.print(request);
    Serial.print  (" us   measured ");
    printUs(ns);
    Serial.print  (" us");

    if (ns == 0UL) {
        Serial.println("   <- NO DELAY AT ALL");
        return;
    }
    errPct = (long)((ns * 100UL) / ((unsigned long)request * 1000UL)) - 100L;
    Serial.print  ("   (");
    if (errPct >= 0L) { Serial.print('+'); }
    Serial.print(errPct);
    Serial.println("%)");
}

static void testDelayMicroseconds(void)
{
    Serial.println("--- 3. delayMicroseconds() is a NOP loop, not a timer ----");
    Serial.println("  averaged over 50 calls, micros() overhead subtracted:");

    measureDelayUs(1U);
    measureDelayUs(2U);
    measureDelayUs(5U);
    measureDelayUs(10U);
    measureDelayUs(20U);
    measureDelayUs(50U);
    measureDelayUs(100U);
    measureDelayUs(500U);
    measureDelayUs(1000U);

    Serial.println("  The implementation is: cycles = us * (FCY/1000000); if");
    Serial.println("  cycles > 10 then cycles -= 10; while (cycles > 4) { nop;");
    Serial.println("  cycles -= 4; }  -- so at FCY = 4 MHz a 1 us request is 4");
    Serial.println("  cycles, the subtraction is skipped and the loop never runs.");
    Serial.println("  Short requests are unusable; from a few tens of us up it is");
    Serial.println("  good. If you need an exact short delay, count nops yourself.");
    Serial.println();
}

/* --- test 4: delay() against both other clocks --------------------------- */

static void testDelay(void)
{
    unsigned long m0;
    unsigned long u0;
    unsigned long dm;
    unsigned long du;

    Serial.println("--- 4. delay() against millis() and micros() ------------");

    m0 = millis();
    u0 = micros();
    delay(100);
    dm = millis() - m0;
    du = micros() - u0;

    Serial.print  ("  delay(100): millis says ");
    Serial.print(dm);
    Serial.print  (" ms, micros says ");
    printUs(du * 1000UL);
    Serial.println(" us");
    printResult("delay(100) took 99..102 ms by millis()", (dm >= 99UL) && (dm <= 102UL));
    Serial.println("  delay() polls millis(), so it inherits the 1 ms tick: a");
    Serial.println("  request can finish up to one tick early or late. Do not");
    Serial.println("  build a bit-banged protocol on it.");
    Serial.println();
}

/* --- test 5: rollover, without waiting 71 minutes ----------------------- */

/*
 * Passing the two values in makes rollover testable now: these are exactly the
 * numbers micros() will return 71 minutes from boot, and the comparison is the
 * same comparison the sketch would make then.
 */
static void rolloverCase(const char *label, unsigned long now, unsigned long deadline,
                         uint8_t trulyExpired)
{
    uint8_t naive = (now > deadline) ? 1U : 0U;
    uint8_t safe  = ((long)(now - deadline) >= 0L) ? 1U : 0U;

    Serial.print  ("  ");
    Serial.println(label);
    Serial.print  ("    now=");
    Serial.print(now);
    Serial.print  ("  deadline=");
    Serial.print(deadline);
    Serial.print  ("  truth: ");
    Serial.println(trulyExpired ? "expired" : "not yet");
    Serial.print  ("    now > deadline            -> ");
    Serial.print(naive ? "expired" : "not yet");
    Serial.println(naive == trulyExpired ? "   ok" : "   WRONG");
    Serial.print  ("    (long)(now-deadline) >= 0 -> ");
    Serial.print(safe ? "expired" : "not yet");
    Serial.println(safe == trulyExpired ? "   ok" : "   WRONG");
}

static void testRollover(void)
{
    Serial.println("--- 5. surviving rollover -------------------------------");
    Serial.println("  Two subtraction-based comparisons, given values that");
    Serial.println("  straddle the 2^32 wrap. Only one of them is right.");
    Serial.println();

    rolloverCase("normal, deadline 100 ms ago:", 5000UL, 4900UL, 1U);
    rolloverCase("normal, deadline 100 ms ahead:", 4900UL, 5000UL, 0U);
    Serial.println("  now the interesting one: the deadline was set 100 counts");
    Serial.println("  before the wrap and 'now' is 50 counts after it, so 150");
    Serial.println("  counts have really elapsed and it HAS expired:");
    rolloverCase("wrapped:", 50UL, 0xFFFFFF9CUL, 1U);

    Serial.println();
    Serial.println("  In the wrapped case the naive test says 'not yet' and stays");
    Serial.println("  wrong for another 49.7 days. The subtraction wraps too, and");
    Serial.println("  the two wraps cancel, which is why the difference is always");
    Serial.println("  correct as long as the interval is under 24.8 days.");
    printResult("subtraction form is right across the wrap",
                ((long)(50UL - 0xFFFFFF9CUL) >= 0L));
    Serial.println();
    Serial.println("  So write this, never a comparison of absolute times:");
    Serial.println("      if ((long)(millis() - g_next) >= 0) { g_next += PERIOD; ... }");
    Serial.println("  and keep timestamps in unsigned long, never int.");
    Serial.println();

    /* And run it for real, so the idiom above appears in working code. */
    {
        unsigned long next   = millis();
        unsigned long finish = millis() + BLINK_DEMO_MS;
        uint8_t       on     = 0;
        unsigned int  edges  = 0;

        Serial.println("  blinking with that idiom for 2 s (no delay() anywhere):");
        while ((long)(millis() - finish) < 0L) {
            if ((long)(millis() - next) >= 0L) {
                next += BLINK_MS;
                on = (uint8_t)(!on);
                digitalWrite(LED_BUILTIN, on ? LED_ON : LED_OFF);
                edges++;
            }
            /* real sketches do their other work right here */
        }
        digitalWrite(LED_BUILTIN, LED_OFF);
        Serial.print  ("    ");
        Serial.print(edges);
        Serial.print  (" edges in 2000 ms at ");
        Serial.print(BLINK_MS);
        Serial.println(" ms - expected about 13");
    }
    Serial.println();
}

/* --- test 6: time lost while interrupts are off ------------------------- */

static void testInterruptsOff(void)
{
    unsigned long m0;
    unsigned long m1;
    unsigned long lost;

    Serial.println("--- 6. what interrupts-off costs millis() ---------------");

    /*
     * 5 ms of NOP loop with the global interrupt enable cleared. Bounded and
     * short on purpose. delayMicroseconds() is used rather than delay() because
     * delay() polls millis(), which cannot advance here -- it would hang forever.
     */
    m0 = millis();
    noInterrupts();
    delayMicroseconds(5000);
    interrupts();
    m1 = millis();

    lost = (m1 - m0);
    Serial.print  ("  5 ms of work with interrupts off: millis() advanced ");
    Serial.print(lost);
    Serial.println(" ms");
    Serial.print  ("  time lost              : ");
    Serial.print((lost < 5UL) ? (5UL - lost) : 0UL);
    Serial.println(" ms, and it never comes back");
    printResult("millis() lost ticks, as expected", lost < 5UL);

    Serial.println("  Timer1's flag latches but does not count, so however long");
    Serial.println("  you stay off you get exactly one tick back. Keep critical");
    Serial.println("  sections to microseconds, and never call delay() inside one:");
    Serial.println("  delay() waits on millis(), millis() waits on the interrupt,");
    Serial.println("  and the interrupt waits on you. That is a hang, not a stall.");
    Serial.println();
}

/* --- test 7: map() ------------------------------------------------------ */

static void mapCase(const char *note, long v, long a, long b, long c, long d)
{
    Serial.print  ("  map(");
    Serial.print(v);    Serial.print(", ");
    Serial.print(a);    Serial.print(", ");
    Serial.print(b);    Serial.print(", ");
    Serial.print(c);    Serial.print(", ");
    Serial.print(d);    Serial.print(") = ");
    Serial.print(map(v, a, b, c, d));
    Serial.print  ("   ");
    Serial.println(note);
}

static void testMap(void)
{
    Serial.println("--- 7. map() truncates and does not clamp ---------------");

    mapCase("0..1023 -> 0..255, the usual case", 512L, 0L, 1023L, 0L, 255L);
    mapCase("truncated, not rounded (127.5)", 511L, 0L, 1023L, 0L, 255L);
    mapCase("exactly the top", 1023L, 0L, 1023L, 0L, 255L);
    mapCase("<- INPUT ABOVE RANGE, result above 255", 1500L, 0L, 1023L, 0L, 255L);
    mapCase("<- NEGATIVE input, negative result", -100L, 0L, 1023L, 0L, 255L);
    mapCase("reversed output range works fine", 250L, 0L, 1000L, 255L, 0L);

    Serial.println();
    Serial.println("  map() is (v-fromLow)*(toHigh-toLow)/(fromHigh-fromLow)+toLow");
    Serial.println("  in long arithmetic. Three consequences:");
    Serial.println("   - it truncates toward zero, so it under-reports by up to 1;");
    Serial.println("   - it does not clamp, so out-of-range in is out-of-range out.");
    Serial.println("     Clamp the INPUT yourself before calling it;");
    Serial.println("   - if fromHigh == fromLow it divides by zero. On this chip");
    Serial.println("     that is an arithmetic error trap, i.e. a RESET -- not a");
    Serial.println("     wrong answer. This sketch deliberately does not do it.");
    Serial.println("  Watch the multiply, too: (v-fromLow)*(toHigh-toLow) must fit");
    Serial.println("  in a signed long, which is easy to break with wide ranges.");
    Serial.println();
}

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LED_OFF);

    Serial.begin(115200);

    Serial.println();
    Serial.println("======================================================");
    Serial.println(" millis / micros / delay / map - dsPIC33CK256MC005");
    Serial.println("======================================================");
    Serial.println();
}

void loop()
{
    reportTimeBase();
    testMicrosResolution();
    testDelayMicroseconds();
    testDelay();
    testRollover();
    testInterruptsOff();
    testMap();

    Serial.print  ("=== pass complete at ");
    Serial.print(millis());
    Serial.println(" ms, again in 5 s ===============");
    Serial.println();
    delay(5000);
}
