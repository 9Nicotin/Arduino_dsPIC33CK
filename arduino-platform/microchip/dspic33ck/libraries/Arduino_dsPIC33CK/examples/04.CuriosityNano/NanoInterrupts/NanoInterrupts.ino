/*
 * NanoInterrupts - attachInterrupt() on the dsPIC33CK256MC005 Curiosity Nano
 *                  (EV08P02A): edges, debounce, shared state, and the traps
 *
 * WIRING: none required. SW0 on the board is enough for the first half.
 * For the second half, one jumper:
 *   D22 (RC1) --- tap to GND to generate edges
 * D22 is held HIGH by its internal pull-up, so touching it to GND gives a
 * falling edge and letting go gives a rising one (plus a lot of contact bounce,
 * which is exactly what makes the point).
 *
 * WHAT YOU GET ON THIS CORE
 *   attachInterrupt(pin, handler, mode)
 *     - pin is the ARDUINO PIN NUMBER. digitalPinToInterrupt(p) exists and is
 *       the identity function; it is there so AVR sketches compile unchanged.
 *     - EVERY pin can carry an interrupt. The core uses Change Notification,
 *       which covers all of PORTA..PORTD, rather than the three remappable INTx
 *       lines -- PORTA has no PPS at all on this family, so INTx could not reach
 *       RA0..RA4.
 *     - mode is RISING, FALLING or CHANGE. There is no LOW and no HIGH: CN is an
 *       edge detector, and a level-triggered mode would re-enter forever.
 *     - An out-of-range pin, a NULL handler or an unsupported mode is a SILENT
 *       no-op. The function returns void, same as upstream Arduino, so there is
 *       nothing to check. Get the arguments right.
 *     - It makes the pin an input for you, because CN only fires on pins whose
 *       TRIS bit is set. It deliberately leaves CNPUx alone, so the standard
 *         pinMode(p, INPUT_PULLUP); attachInterrupt(p, fn, FALLING);
 *       idiom works -- the pull-up survives.
 *
 *   detachInterrupt(pin) removes one handler. The port's vector stays enabled if
 *   any other pin on that port is still attached.
 *
 *   interrupts() / noInterrupts() are the global enable (INTCON2.GIE).
 *
 * PRIORITY, AND WHY IT MATTERS
 *   Your handlers run at IPL2. The core's own interrupts sit ABOVE them:
 *   Timer1 (millis/micros) at IPL4 and UART1 receive higher still. So:
 *     - millis() keeps time while your handler runs. Good.
 *     - Serial keeps receiving while your handler runs. Also good.
 *     - Your handler can be interrupted by them, so a multi-word variable it
 *       writes can be seen half-updated by... nothing, actually: the main loop
 *       is BELOW IPL2, so the danger runs the other way. The MAIN LOOP is what
 *       gets interrupted mid-read. See readLongSafely() below.
 *
 * THE FOUR RULES
 *   1. Anything shared between a handler and loop() is volatile.
 *   2. Reading anything wider than 16 bits (long, unsigned long, double,
 *      structs) from loop() needs a noInterrupts()/interrupts() pair. A 32-bit
 *      read is two instructions on a 16-bit core and an edge can land between
 *      them, giving you a value that never existed.
 *   3. Do not print, delay or block in a handler. Set a flag; act in loop().
 *   4. Do not hold noInterrupts() for longer than about a millisecond. Timer1
 *      rolls over that fast and only ever adds 1 to the millis() counter per
 *      interrupt, so time is lost -- and delay(), which spins on millis(), never
 *      returns at all.
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

#define EDGE_PIN        22          /* RC1 - tap to GND */
#define DEBOUNCE_MS     40UL

/* --- shared with the handlers. Every one of these is volatile. ------------- */

/* 16-bit, so loop() can read it in one instruction: no critical section needed
 * on this core. Relying on that is a portability trap, though -- it is true
 * because dsPIC is a 16-bit machine, not because the C standard says so. */
static volatile uint16_t g_pressCount   = 0;
static volatile uint16_t g_bounceCount  = 0;
static volatile uint16_t g_risingCount  = 0;
static volatile uint16_t g_fallingCount = 0;

/* 32-bit. NOT safe to read without a critical section. */
static volatile unsigned long g_lastPressMs = 0;
static volatile unsigned long g_lastEdgeUs  = 0;

/* Flag-and-handle: the handler sets this, loop() does the printing. */
static volatile uint8_t g_pressFlag = 0;

/* --- handlers -------------------------------------------------------------- */

/*
 * SW0, debounced. A cheap mechanical switch produces a burst of edges over a few
 * milliseconds; without the millis() guard a single press reports as five or six.
 *
 * millis() is safe to call here: its Timer1 interrupt is IPL4, above this
 * handler at IPL2, so the counter is still being maintained and the read itself
 * is protected by a DISI pair inside millis().
 *
 * g_bounceCount counts the edges this filter threw away, so you can see how
 * bouncy your switch really is rather than guessing at a debounce time.
 */
static void onButtonPress(void)
{
    unsigned long now = millis();

    if ((now - g_lastPressMs) < DEBOUNCE_MS) {
        g_bounceCount++;
        return;
    }

    g_lastPressMs = now;
    g_pressCount++;
    g_pressFlag = 1;
}

/*
 * D22 on CHANGE. One handler, both edges: CN tells you a change happened, not
 * which way, so read the pin to find out. That read is a few cycles after the
 * edge, which is close enough for a switch and NOT close enough for a fast
 * signal -- a pulse shorter than the handler's own latency can be over before
 * the digitalRead(), and you will log the wrong direction.
 */
static void onEdgeChange(void)
{
    g_lastEdgeUs = micros();

    if (digitalRead(EDGE_PIN) == HIGH) {
        g_risingCount++;
    } else {
        g_fallingCount++;
    }
}

/* --- safe access to the 32-bit values ------------------------------------- */

/*
 * The whole reason rule 2 exists. On a 16-bit core an unsigned long is loaded as
 * two words; an interrupt between them yields a mix of old high word and new low
 * word. The window is small, which is worse than if it were large: the bug shows
 * up once an hour and is blamed on the hardware.
 *
 * The pair costs about a dozen cycles. Use it.
 */
static unsigned long readLongSafely(volatile unsigned long *p)
{
    unsigned long v;

    noInterrupts();
    v = *p;
    interrupts();

    return v;
}

/* --- reporting ------------------------------------------------------------- */

static void printStatus(void)
{
    unsigned long lastPress = readLongSafely(&g_lastPressMs);
    unsigned long lastEdge  = readLongSafely(&g_lastEdgeUs);

    Serial.print  ("presses=");
    Serial.print(g_pressCount);
    Serial.print  ("  bounces filtered=");
    Serial.print(g_bounceCount);
    Serial.print  ("  D22 rising=");
    Serial.print(g_risingCount);
    Serial.print  (" falling=");
    Serial.print(g_fallingCount);
    Serial.print  ("  last press ");

    if (lastPress == 0UL) {
        Serial.print("never");
    } else {
        Serial.print((millis() - lastPress) / 1000UL);
        Serial.print("s ago");
    }

    Serial.print  ("  last D22 edge at ");
    Serial.print(lastEdge);
    Serial.println(" us");
}

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LED_OFF);

    Serial.begin(115200);

    Serial.println();
    Serial.println("======================================================");
    Serial.println(" Interrupts - dsPIC33CK256MC005 Curiosity Nano");
    Serial.println("======================================================");
    Serial.print  ("SW0        : D");
    Serial.print(BUTTON_BUILTIN);
    Serial.println(" (RD13), internal pull-up, active LOW -> FALLING");
    Serial.print  ("edge input : D");
    Serial.print(EDGE_PIN);
    Serial.println(" (RC1), internal pull-up -> CHANGE");
    Serial.print  ("debounce   : ");
    Serial.print(DEBOUNCE_MS);
    Serial.println(" ms, in the handler");
    Serial.print  ("digitalPinToInterrupt(38) = ");
    Serial.println(digitalPinToInterrupt(38));
    Serial.print  ("digitalPinToInterrupt(99) = ");
    Serial.println(digitalPinToInterrupt(99));
    Serial.println("           (identity, or -1 for a pin that does not exist)");

    /* INPUT_PULLUP first, then attach. In that order the pull-up survives --
     * attachInterrupt() sets TRIS but never touches CNPU. The other order also
     * works here, but only by luck; write it this way. */
    pinMode(BUTTON_BUILTIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON_BUILTIN), onButtonPress, FALLING);

    pinMode(EDGE_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(EDGE_PIN), onEdgeChange, CHANGE);

    /* Two silent no-ops, shown deliberately: neither reports an error, so a typo
     * here costs you a debugging session with a handler that never fires. */
    attachInterrupt(99, onButtonPress, FALLING);   /* no such pin */
    attachInterrupt(EDGE_PIN, onEdgeChange, 0);    /* not a valid mode */

    Serial.println();
    Serial.println("Press SW0. Tap D22 to GND. Status prints once a second and");
    Serial.println("immediately after each debounced press.");
    Serial.println("After 10 presses SW0 is detached, to show that too.");
    Serial.println();
}

void loop()
{
    static unsigned long lastReport = 0;
    static uint8_t       detached   = 0;

    /* Rule 3 in practice: the handler set a flag, the printing happens here. */
    if (g_pressFlag) {
        g_pressFlag = 0;

        /* Toggle LED0 so a press is visible without the monitor. */
        digitalWrite(LED_BUILTIN,
                     (g_pressCount & 1U) ? LED_ON : LED_OFF);

        Serial.print("press! -> ");
        printStatus();
    }

    if ((millis() - lastReport) >= 1000UL) {
        lastReport = millis();
        Serial.print("tick   -> ");
        printStatus();
    }

    if (!detached && g_pressCount >= 10U) {
        detached = 1;
        detachInterrupt(digitalPinToInterrupt(BUTTON_BUILTIN));

        Serial.println();
        Serial.println("SW0 detached. Further presses are ignored; D22 still");
        Serial.println("fires, because detachInterrupt() only removed one pin");
        Serial.println("and PORTC's vector was never involved anyway.");
        Serial.println();
    }
}
