/*
 * NanoComparator - analog comparator CMP1 on the dsPIC33CK256MC005 Curiosity
 *                  Nano (EV08P02A): thresholds, edge interrupts, hysteresis
 *
 * WIRING: none required for tests 1-3. The sketch generates its own analog
 * signal by PWM-ing D7 (RB2), which happens to be comparator input CMP1D -- so
 * the comparator watches a 490 Hz square wave produced by the same chip, and the
 * interrupt rate can be checked against a number we already know.
 *
 * For test 4, one external connection:
 *   D0 / A0 (RA0, pin 8) = CMP1A -- a 10k pot wiper between 3V3 and GND.
 * Without it, test 4 reads a floating pin and says so.
 *
 * WHAT A COMPARATOR IS FOR
 * analogRead() tells you a voltage, eventually: it costs microseconds, returns a
 * number, and only answers when asked. A comparator answers one question --
 * "is the input above the threshold?" -- continuously, in hardware, in tens of
 * nanoseconds, and can raise an interrupt the moment the answer changes. That is
 * what you want for overcurrent trips, zero-crossing detection, end-of-travel
 * sensing, and anything where a late answer is a broken answer.
 *
 * THE THRESHOLD IS THE DAC
 * There is no CMPxCON register on this family. The comparator's control bits
 * live in DAC1CONL, because the comparator and DAC1 are one module: DAC1 exists
 * to provide a programmable threshold. That also means the threshold costs you
 * nothing in pins and can be changed at run time -- a resistor divider can do
 * neither.
 *
 * Note that DAC1 on this device has NO output buffer (no DACOEN bit exists), so
 * it can only feed the comparator. See NanoDAC for the details and for what that
 * rules out.
 *
 * WHAT TO LOOK FOR
 *   1. ~490 interrupts/second on one edge, ~980 on both -- matching the PWM
 *      frequency the core sets up. The comparator is counting real edges.
 *   2. Zero interrupts when the input is static (duty 0 or 255), whatever the
 *      threshold is.
 *   3. Zero interrupts when the threshold is outside the signal's range, which
 *      is how you tell a mis-set threshold from a dead peripheral.
 *   4. With hysteresis on, a slow noisy input crosses once instead of chattering.
 *
 * INTERRUPT NOTE, and it is the one that bites: a .ino is compiled as C++, so an
 * ISR needs extern "C" or its name is mangled, the vector never gets filled, the
 * default handler traps, and the symptom is a reset loop with no message.
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

/* INSEL<2:0>, the comparator's input multiplexer. On the 48-pin MC005:
 *   CMP1A = RA0 = D0/A0   (pin 8)  - external analog input
 *   CMP1B = RC1 = D22/A14 (pin 15)
 *   CMP1C = RA3 = D3/A3   (pin 11)
 *   CMP1D = RB2 = D7/A7   (pin 25) - also a PWM pin, which is why it is useful
 *                                    here: the chip can feed its own comparator */
#define INSEL_CMP1A     0
#define INSEL_CMP1B     1
#define INSEL_CMP1C     2
#define INSEL_CMP1D     3

/* IRQM<1:0>, per the family reference manual. The sketch checks these by
 * counting rather than assuming: at 490 Hz, "rising" and "falling" must each
 * give half as many interrupts as "both". */
#define IRQM_DISABLED   0
#define IRQM_RISING     1
#define IRQM_FALLING    2
#define IRQM_BOTH       3

#define SIGNAL_PIN      7           /* RB2, CMP1D, PWM-capable */
#define EXTERNAL_PIN    A0          /* RA0, CMP1A */

#define DAC_MAX_CODE    4095U
#define DAC_CODES       4096UL
#define DAC_VREF_MV     3300UL
#define DAC_SETTLE_US   10U

#define PWM_FREQ_HZ     490UL       /* what the core's analogWrite() produces */

/* --- state shared with the ISR --------------------------------------------- */

/* uint16_t, so loop() reads it atomically on this 16-bit core. At 980 edges per
 * second it wraps every 67 seconds, which is why the counting windows below are
 * one second long and the counter is reset before each. */
static volatile uint16_t g_edges = 0;

/* Last known comparator output, latched by the ISR. Reading CMPSTAT from loop()
 * would tell you the state now; this tells you the state at the edge, which is
 * what an event handler actually cares about. */
static volatile uint8_t g_lastState = 0;

/* --- the ISR ---------------------------------------------------------------- */

/*
 * extern "C" is not optional. Without it this is _Z14_CMP1Interruptv, the vector
 * table keeps its default entry, and the first comparator edge takes you to the
 * trap handler.
 *
 * auto_psv matches the core's own handlers: it saves and restores PSVPAG so the
 * ISR may touch const data in program space. no_auto_psv is a few cycles faster
 * and silently corrupts the interrupted code if you get it wrong, so use it only
 * when you have checked what the handler touches.
 *
 * Clearing the flag FIRST and doing the work second means an edge arriving during
 * the handler is not lost. The other order drops it.
 */
extern "C" void __attribute__((interrupt, auto_psv)) _CMP1Interrupt(void)
{
    IFS4bits.CMP1IF = 0;

    g_edges++;
    g_lastState = DAC1CONLbits.CMPSTAT ? 1U : 0U;
}

/* --- setup ----------------------------------------------------------------- */

static void dacSetCode(uint16_t code)
{
    DAC1DATH = (code > DAC_MAX_CODE) ? DAC_MAX_CODE : code;
    delayMicroseconds(DAC_SETTLE_US);
}

static unsigned long dacCodeToMillivolts(uint16_t code)
{
    return ((unsigned long)code * DAC_VREF_MV) / DAC_CODES;
}

static void comparatorSetInput(uint8_t insel)
{
    DAC1CONLbits.INSEL = insel;
    delayMicroseconds(DAC_SETTLE_US);
}

static void comparatorSetIrqMode(uint8_t irqm)
{
    /* Disable, clear, re-arm. Changing IRQM with the interrupt enabled can leave
     * a flag set from the old mode, which fires once immediately and looks like a
     * spurious edge. */
    IEC4bits.CMP1IE = 0;
    DAC1CONLbits.IRQM = irqm;
    IFS4bits.CMP1IF = 0;

    if (irqm != IRQM_DISABLED) {
        IEC4bits.CMP1IE = 1;
    }
}

static void comparatorBegin(void)
{
    /* Out of reset the whole module is held disabled by Peripheral Module
     * Disable. Register writes before this line go nowhere at all. */
    PMD7bits.CMP1MD = 0;

    /* Both comparator input pins analog. ANSEL turns off the digital input
     * buffer, which is what you want on a pin carrying a mid-rail voltage; the
     * pad's OUTPUT driver is unaffected, so RB2 can still be driven by PWM while
     * the comparator watches it. (The core's analogWrite() does not touch ANSEL
     * -- it drives the pad through peripheral pin select, which overrides TRIS.) */
    ANSELAbits.ANSELA0 = 1;
    TRISAbits.TRISA0   = 1;
    ANSELBbits.ANSELB2 = 1;

    /* DAC module clock, feeding the module's internal timers. Neither the slope
     * generator nor transition mode is used here, so its rate is irrelevant and
     * FCLKDIV takes its maximum divide -- in spec at both clock settings the
     * board menu offers (FCY 4 MHz internal, FCY 100 MHz PLL). */
    DACCTRL1L = 0;
    DACCTRL1Lbits.CLKSEL  = 1;
    DACCTRL1Lbits.FCLKDIV = 7;

    DAC1CONL = 0;
    DAC1CONLbits.INSEL  = INSEL_CMP1D;
    DAC1CONLbits.CMPPOL = 0;        /* CMPSTAT = 1 when input is above threshold */
    DAC1CONLbits.HYSSEL = 0;
    DAC1CONLbits.HYSPOL = 0;
    DAC1CONLbits.IRQM   = IRQM_DISABLED;
    DAC1CONLbits.DACEN  = 1;

    DACCTRL1Lbits.DACON = 1;

    dacSetCode(DAC_MAX_CODE / 2U);  /* threshold at mid-rail, ~1.65 V */

    /* IPL2, the same level the core gives attachInterrupt() handlers. That keeps
     * it below Timer1 (IPL4), so millis() stays accurate no matter how fast the
     * comparator is firing. */
    IPC19bits.CMP1IP = 2;
    IFS4bits.CMP1IF  = 0;
    IEC4bits.CMP1IE  = 0;
}

/* --- counting --------------------------------------------------------------- */

/*
 * Count edges over one second. Reset under noInterrupts() so a counter increment
 * cannot land between the clear and the timestamp; the pair is a few cycles, far
 * short of the ~1 ms that would cost millis() a tick.
 */
static uint16_t countEdgesForOneSecond(void)
{
    uint16_t n;

    noInterrupts();
    g_edges = 0;
    interrupts();

    delay(1000);

    noInterrupts();
    n = g_edges;
    interrupts();

    return n;
}

static void printCount(uint16_t n, unsigned long expected)
{
    Serial.print(n);
    Serial.print(" edges/s  (expected ~");
    Serial.print(expected);
    Serial.print(") ");

    if (expected == 0UL) {
        Serial.println((n == 0U) ? "OK" : "unexpected activity");
    } else {
        unsigned long lo = (expected * 9UL) / 10UL;
        unsigned long hi = (expected * 11UL) / 10UL;

        Serial.println(((unsigned long)n >= lo && (unsigned long)n <= hi)
                       ? "OK" : "OFF");
    }
}

/* --- test 1: the interrupt path, verified against a known frequency --------- */

static void testEdgeModes(void)
{
    Serial.println("--- 1. IRQM edge modes vs a 490 Hz square wave ------------");

    analogWrite(SIGNAL_PIN, 128);   /* 50% duty on D7 = CMP1D */
    comparatorSetInput(INSEL_CMP1D);
    dacSetCode(DAC_MAX_CODE / 2U);

    Serial.print("  IRQM=1 rising : ");
    comparatorSetIrqMode(IRQM_RISING);
    printCount(countEdgesForOneSecond(), PWM_FREQ_HZ);

    Serial.print("  IRQM=2 falling: ");
    comparatorSetIrqMode(IRQM_FALLING);
    printCount(countEdgesForOneSecond(), PWM_FREQ_HZ);

    Serial.print("  IRQM=3 both   : ");
    comparatorSetIrqMode(IRQM_BOTH);
    printCount(countEdgesForOneSecond(), PWM_FREQ_HZ * 2UL);

    Serial.print("  IRQM=0 off    : ");
    comparatorSetIrqMode(IRQM_DISABLED);
    printCount(countEdgesForOneSecond(), 0UL);

    Serial.println("  Half as many on one edge as on both: the comparator is");
    Serial.println("  following the actual waveform, not oscillating.");
    Serial.println();
}

/* --- test 2: a static input produces no events ----------------------------- */

static void testStaticInput(void)
{
    Serial.println("--- 2. static input -> no interrupts ---------------------");

    comparatorSetIrqMode(IRQM_BOTH);

    /* analogWrite(pin, 0) and (pin, 255) do not produce 0% and 100% PWM -- the
     * core tears the channel down and drives the pin as plain GPIO. Either way
     * the comparator sees a DC level. */
    Serial.print("  duty 0   (pin LOW)  : ");
    analogWrite(SIGNAL_PIN, 0);
    printCount(countEdgesForOneSecond(), 0UL);

    Serial.print("  duty 255 (pin HIGH) : ");
    analogWrite(SIGNAL_PIN, 255);
    printCount(countEdgesForOneSecond(), 0UL);

    Serial.print("  duty 128 (switching): ");
    analogWrite(SIGNAL_PIN, 128);
    printCount(countEdgesForOneSecond(), PWM_FREQ_HZ * 2UL);

    Serial.println();
}

/* --- test 3: the threshold has to be inside the signal's range ------------- */

static void testThresholdSweep(void)
{
    static const uint16_t codes[] = { 0, 200, 1024, 2048, 3072, 3900, 4095 };
    size_t i;

    Serial.println("--- 3. threshold sweep, 50% duty square wave -------------");
    Serial.println("  DACDAT  threshold   edges/s");

    analogWrite(SIGNAL_PIN, 128);
    comparatorSetIrqMode(IRQM_BOTH);

    for (i = 0; i < (sizeof(codes) / sizeof(codes[0])); i++) {
        dacSetCode(codes[i]);

        Serial.print("   ");
        Serial.print(codes[i]);
        Serial.print("\t  ");
        Serial.print(dacCodeToMillivolts(codes[i]));
        Serial.print(" mV\t  ");
        Serial.println(countEdgesForOneSecond());
    }

    Serial.println("  A threshold outside the signal's range gives zero edges,");
    Serial.println("  which looks exactly like a broken peripheral. When a");
    Serial.println("  comparator seems dead, suspect the threshold first.");
    Serial.println();

    dacSetCode(DAC_MAX_CODE / 2U);
}

/* --- test 4: an external input, with hysteresis ---------------------------- */

/*
 * The part of the sketch that does what a comparator is actually for. Turn the
 * pot slowly through the threshold: without hysteresis a slow, noisy crossing
 * produces a burst of edges, because noise on the input crosses the threshold
 * many times on the way past. With hysteresis the input has to move a further
 * 15-45 mV to come back, and the burst collapses to one edge.
 *
 * This is why a bare comparator on a slow signal is a mistake, and why HYSSEL
 * exists.
 */
static void testExternalInput(void)
{
    unsigned long deadline;
    uint8_t       hyssel;

    Serial.println("--- 4. external input on D0/A0 (CMP1A), hysteresis -------");
    Serial.println("  Turn a pot on A0 slowly back and forth through 1.65 V.");
    Serial.println("  LED0 follows the comparator output. Each line is a");
    Serial.println("  5-second window at one HYSSEL setting.");
    Serial.println();

    analogWrite(SIGNAL_PIN, 0);     /* release D7; nothing left driving CMP1D */
    comparatorSetInput(INSEL_CMP1A);
    dacSetCode(DAC_MAX_CODE / 2U);
    comparatorSetIrqMode(IRQM_BOTH);

    for (hyssel = 0; hyssel < 4U; hyssel++) {
        uint16_t n;

        DAC1CONLbits.HYSSEL = hyssel;
        delayMicroseconds(DAC_SETTLE_US);

        noInterrupts();
        g_edges = 0;
        interrupts();

        /* Five seconds of live LED tracking rather than a blind delay(5000):
         * the point of the test is watching the output while you turn the pot. */
        deadline = millis() + 5000UL;
        while ((long)(millis() - deadline) < 0L) {
            digitalWrite(LED_BUILTIN, g_lastState ? LED_ON : LED_OFF);
        }

        noInterrupts();
        n = g_edges;
        interrupts();

        Serial.print("  HYSSEL=");
        Serial.print(hyssel);
        Serial.print(" (~");
        Serial.print((unsigned int)(hyssel * 15U));
        Serial.print(" mV): ");
        Serial.print(n);
        Serial.println(" edges in 5 s");
    }

    DAC1CONLbits.HYSSEL = 0;
    digitalWrite(LED_BUILTIN, LED_OFF);

    Serial.println("  Fewer edges at higher HYSSEL for the same hand movement.");
    Serial.println("  If every line reads 0, A0 is not connected to anything.");
    Serial.println();
}

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LED_OFF);

    Serial.begin(115200);

    comparatorBegin();

    Serial.println();
    Serial.println("======================================================");
    Serial.println(" CMP1 - dsPIC33CK256MC005 Curiosity Nano");
    Serial.println("======================================================");
    Serial.print  ("signal source : D");
    Serial.print(SIGNAL_PIN);
    Serial.println(" (RB2) = CMP1D, driven by analogWrite()");
    Serial.print  ("external input : D0/A0 (RA0) = CMP1A, optional pot");
    Serial.println();
    Serial.print  ("threshold      : DAC1, 12-bit, ");
    Serial.print(DAC_VREF_MV * 1000UL / DAC_CODES);
    Serial.println(" uV per code");
    Serial.println("handler        : _CMP1Interrupt at IPL2, extern \"C\"");
    Serial.println();
}

void loop()
{
    testEdgeModes();
    testStaticInput();
    testThresholdSweep();
    testExternalInput();

    /* Leave the comparator quiet between passes so the pin and the module are in
     * a known state if you stop the sketch here with a debugger. */
    comparatorSetIrqMode(IRQM_DISABLED);
    analogWrite(SIGNAL_PIN, 0);

    Serial.println("=== pass complete, restarting in 3 s =====================");
    Serial.println();
    delay(3000);
}
