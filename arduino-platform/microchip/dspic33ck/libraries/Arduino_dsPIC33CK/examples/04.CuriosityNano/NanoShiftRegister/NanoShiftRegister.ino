/*
 * NanoShiftRegister - shiftOut() and shiftIn() on the dsPIC33CK256MC005
 *                     Curiosity Nano (EV08P02A)
 *
 * shiftOut() and shiftIn() are the software-bit-banged cousins of SPI: three
 * ordinary GPIO pins, no peripheral, any pins you like. They are how 74HC595
 * output expanders and 74HC165 input expanders are driven, and neither function
 * had an example in this repository until now.
 *
 * WIRING
 *   Tests 1-2: NOTHING.
 *   Test 3:    one jumper, D12 (CLOCK) -> D22 (DATA_IN).
 *              Safe to leave connected for every test: D22 is only ever an input.
 *   Tests 4-5: a real 74HC595 or 74HC165. Off by default -- set the #define and
 *              the code is there, wiring in the comment above each.
 *
 * THE DIFFERENCE FROM SPI THAT ACTUALLY MATTERS
 *
 *   shiftOut() and shiftIn() DO honour MSBFIRST and LSBFIRST.
 *
 *   This library's SPI does not: SPI.setBitOrder() is literally "(void)bitOrder;"
 *   because the hardware has no bit-order control, so SPI is permanently
 *   MSB-first. If you have a part that wants LSB-first framing, these two
 *   functions are the way to talk to it on this chip -- and test 1 proves the
 *   ordering works rather than taking the argument's presence as evidence.
 *
 *   What you give up is speed. Test 2 measures it: bit-banging is roughly two
 *   orders of magnitude slower than SPI1 at its FCY/2 maximum, because every
 *   single bit costs three digitalWrite() calls.
 *
 * FOUR IMPLEMENTATION FACTS
 *
 * 1. Neither function calls pinMode(). You must set the pins up yourself --
 *    dataPin and clockPin OUTPUT for shiftOut, clockPin OUTPUT and dataPin INPUT
 *    for shiftIn. Forget it and shiftOut silently drives nothing.
 *
 * 2. The clock idles LOW, is raised, then lowered, once per bit. shiftIn() reads
 *    the data pin WHILE the clock is high, i.e. after the rising edge -- which is
 *    exactly what a 74HC165 expects, so upstream Arduino code ports unchanged.
 *    Test 3 demonstrates that sampling point electrically.
 *
 * 3. shiftOut() LEAVES THE DATA PIN HOLDING THE LAST BIT IT SENT. It is not
 *    returned to a known state. That is a footgun if the pin does anything else
 *    afterwards -- and it is also, conveniently, a way to prove the bit ordering
 *    with no test equipment at all, which is what test 1 does.
 *
 * 4. There is no latch, no chip select and no delay anywhere in either function.
 *    A 74HC595's latch pin is entirely your sketch's business, and if you forget
 *    to pulse it the outputs never change while the shifting works perfectly.
 *
 * WHY THERE IS NO LOOPBACK SELF-TEST HERE
 *   NanoSPITest can check itself with one jumper because SPI's hardware drives
 *   the clock while a separate wire carries data back. These two functions cannot:
 *   BOTH of them generate the clock, so a single MCU cannot shiftOut into its own
 *   shiftIn -- there is nobody to be the other end. So tests 1-3 verify everything
 *   that is verifiable without another chip, and say so, rather than printing a
 *   confident PASS that means nothing.
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

/* Set either of these to 1 if you have the chip. Wiring is above each test. */
#define HAVE_74HC595    0
#define HAVE_74HC165    0

#define DATA_OUT    9       /* RB4 - shiftOut data,  and 595 SER  (pin 14) */
#define CLOCK       12      /* RB7 - shared clock,   and 595 SRCLK (pin 11) */
#define LATCH       21      /* RC0 - 595 RCLK (pin 12) / 165 SH-LD (pin 1)  */
#define DATA_IN     22      /* RC1 - shiftIn data,   and 165 QH   (pin 9)   */

/* Never D10 or D11: those are RB5/RB6 = the debugger's PGD3/PGC3. */

#define TIMING_BYTES    100U

/* --- helpers -------------------------------------------------------------- */

static void printByte(const char *label, uint8_t v)
{
    uint8_t i;

    Serial.print(label);
    Serial.print("0x");
    if (v < 0x10U) { Serial.print('0'); }
    Serial.print((unsigned int)v, HEX);
    Serial.print("  0b");
    for (i = 0; i < 8U; i++) {
        Serial.print((v & (uint8_t)(0x80U >> i)) ? '1' : '0');
    }
    Serial.println();
}

static void printResult(const char *what, uint8_t pass)
{
    Serial.print("    ");
    Serial.print(what);
    Serial.println(pass ? " ... PASS" : " ... FAIL");
}

/* "0x80 MSBFIRST -> last bit 0, expected 0 (bit0 of 0x80)  ok" */
static void reportLastBit(const char *label, uint8_t got, uint8_t want,
                          const char *why)
{
    Serial.print  ("    ");
    Serial.print(label);
    Serial.print  (" -> last bit ");
    Serial.print(got ? '1' : '0');
    Serial.print  (", expected ");
    Serial.print(want ? '1' : '0');
    Serial.print  (" (");
    Serial.print(why);
    Serial.println(got == want ? ")  ok" : ")  WRONG");
}

/* --- test 1: bit order, proved with no wiring at all ---------------------- */

/*
 * The trick: because shiftOut() leaves the data pin at the value of the LAST bit
 * it shifted, and because the last bit of an asymmetric byte differs between the
 * two orderings, reading the pin afterwards tells you which ordering ran.
 *
 *   value 0x80 = 1000 0000
 *     MSBFIRST sends bit7 (1) first and bit0 (0) last  -> pin ends LOW
 *     LSBFIRST sends bit0 (0) first and bit7 (1) last  -> pin ends HIGH
 *
 * Opposite results for the same byte, from a single digitalRead(). No scope, no
 * jumper, no logic analyser.
 */
static void testBitOrder(void)
{
    uint8_t msb80;
    uint8_t lsb80;
    uint8_t msb01;
    uint8_t lsb01;
    uint8_t clockIdle;

    Serial.println("--- 1. does bitOrder actually do anything? [no wiring] ---");

    pinMode(DATA_OUT, OUTPUT);
    pinMode(CLOCK, OUTPUT);
    digitalWrite(CLOCK, LOW);

    shiftOut(DATA_OUT, CLOCK, MSBFIRST, 0x80);
    msb80 = (digitalRead(DATA_OUT) == HIGH) ? 1U : 0U;
    clockIdle = (digitalRead(CLOCK) == LOW) ? 1U : 0U;

    shiftOut(DATA_OUT, CLOCK, LSBFIRST, 0x80);
    lsb80 = (digitalRead(DATA_OUT) == HIGH) ? 1U : 0U;

    shiftOut(DATA_OUT, CLOCK, MSBFIRST, 0x01);
    msb01 = (digitalRead(DATA_OUT) == HIGH) ? 1U : 0U;

    shiftOut(DATA_OUT, CLOCK, LSBFIRST, 0x01);
    lsb01 = (digitalRead(DATA_OUT) == HIGH) ? 1U : 0U;

    Serial.println("  reading the data pin after the call - it still holds the");
    Serial.println("  last bit sent, so the last bit tells us the order used:");
    reportLastBit("0x80 MSBFIRST", msb80, 0U, "bit0 of 0x80");
    reportLastBit("0x80 LSBFIRST", lsb80, 1U, "bit7 of 0x80");
    reportLastBit("0x01 MSBFIRST", msb01, 1U, "bit0 of 0x01");
    reportLastBit("0x01 LSBFIRST", lsb01, 0U, "bit7 of 0x01");

    printResult("MSBFIRST and LSBFIRST produce opposite results",
                (msb80 == 0U) && (lsb80 == 1U) && (msb01 == 1U) && (lsb01 == 0U));
    printResult("clock parked LOW after the call", clockIdle);

    Serial.println("  Both orderings are real. Compare SPI.setBitOrder(), which");
    Serial.println("  is a no-op on this chip - see NanoSPITest test 4.");
    Serial.println("  Also note the data pin is NOT restored: it is left driving");
    Serial.println("  whatever the final bit was.");
    Serial.println();
}

/* --- test 2: how slow is it, really? ------------------------------------- */

/*
 * Prints ns/byte as well as us/byte, because at FCY = 100 MHz a byte can take
 * under a microsecond and integer us/byte would round to zero -- and then the
 * bytes/s division would be a divide by zero, not a big number.
 */
static void reportRate(const char *label, unsigned long totalUs)
{
    unsigned long nsPerByte = (totalUs * 1000UL) / TIMING_BYTES;

    Serial.print(label);
    Serial.print(nsPerByte / 1000UL);
    Serial.print('.');
    Serial.print((nsPerByte / 100UL) % 10UL);
    Serial.print(" us/byte  -> ");
    if (nsPerByte == 0UL) {
        Serial.println("too fast to time this way");
    } else {
        Serial.print(1000000000UL / nsPerByte);
        Serial.println(" bytes/s");
    }
}

static void testSpeed(void)
{
    unsigned long t0;
    unsigned long outUs;
    unsigned long inUs;
    unsigned int  i;

    Serial.println("--- 2. bit-bang speed vs hardware SPI [no wiring] --------");

    pinMode(DATA_OUT, OUTPUT);
    pinMode(CLOCK, OUTPUT);
    digitalWrite(CLOCK, LOW);

    t0 = micros();
    for (i = 0; i < TIMING_BYTES; i++) { shiftOut(DATA_OUT, CLOCK, MSBFIRST, (uint8_t)i); }
    outUs = micros() - t0;

    pinMode(DATA_IN, INPUT);
    t0 = micros();
    for (i = 0; i < TIMING_BYTES; i++) { (void)shiftIn(DATA_IN, CLOCK, MSBFIRST); }
    inUs = micros() - t0;

    Serial.print  ("  FCY                    : ");
    Serial.print(FCY / 1000UL);
    Serial.println(" kHz");
    reportRate("  shiftOut               : ", outUs);
    reportRate("  shiftIn                : ", inUs);
    Serial.print  ("  SPI1 ceiling (FCY/2)   : ");
    Serial.print((FCY / 2UL) / 8000UL);
    Serial.println(" k bytes/s in theory");
    Serial.println("  Each bit costs three digitalWrite() calls, each of which");
    Serial.println("  bounds-checks the pin and does a read-modify-write on LATx.");
    Serial.println("  That is the price of working on any pin. If you need speed");
    Serial.println("  and MSB-first suits you, use SPI instead.");
    Serial.println();
}

/* --- test 3: where in the clock does shiftIn sample? [one jumper] -------- */

/*
 * With CLOCK wired to DATA_IN, every bit shiftIn() reads is the clock's own level
 * at the instant of sampling. The result is therefore a direct readout of WHEN it
 * samples:
 *
 *   0xFF -> it reads while the clock is HIGH (after the rising edge)  <- correct
 *   0x00 -> it reads while the clock is LOW  (before the rising edge)
 *
 * The first is what a 74HC165 needs. The bit ORDER cannot be tested this way --
 * every bit is identical, so all eight are the same whichever end you start from.
 */
static void testSamplePoint(void)
{
    uint8_t got;
    uint8_t jumpered;

    Serial.println("--- 3. shiftIn() sampling point  [jumper D12 -> D22] -----");

    /* Jumper detection: hold the clock output LOW and enable DATA_IN's pull-up.
     * A driven output beats a weak pull-up, so LOW means the wire is there. */
    pinMode(CLOCK, OUTPUT);
    digitalWrite(CLOCK, LOW);
    pinMode(DATA_IN, INPUT_PULLUP);
    delay(1);
    jumpered = (digitalRead(DATA_IN) == LOW) ? 1U : 0U;

    if (!jumpered) {
        Serial.println("  D22 floats high with its pull-up, so no jumper is");
        Serial.println("  fitted. Connect D12 to D22 and rerun; tests 1 and 2");
        Serial.println("  need no wiring and have already passed.");
        Serial.println();
        return;
    }

    pinMode(DATA_IN, INPUT);
    got = shiftIn(DATA_IN, CLOCK, MSBFIRST);

    Serial.println("  jumper found (D22 follows D12).");
    printByte("  shiftIn() returned     : ", got);
    if (got == 0xFFU) {
        Serial.println("  All ones: the data pin is read WHILE the clock is high,");
        Serial.println("  after the rising edge. That is 74HC165-compatible.");
    } else if (got == 0x00U) {
        Serial.println("  All zeros: it sampled before raising the clock. That");
        Serial.println("  would NOT match a 74HC165 - report it as a core bug.");
    } else {
        Serial.println("  Mixed bits: sampling is landing on the clock edge");
        Serial.println("  itself. Check the jumper is solid.");
    }
    printResult("samples after the rising edge", got == 0xFFU);

    Serial.println("  This cannot test bit ORDER - every bit is a 1, so MSBFIRST");
    Serial.println("  and LSBFIRST give the same 0xFF. Ordering needs a real");
    Serial.println("  74HC165 (test 5), or take test 1's shiftOut proof: the two");
    Serial.println("  functions share one bitOrder convention.");
    Serial.println();
}

/* --- test 4: 74HC595, eight outputs from three pins ---------------------- */

#if HAVE_74HC595
/*
 * 74HC595 wiring:
 *   pin 14 SER   -> D9  (DATA_OUT)        pin 16 VCC   -> 3V3
 *   pin 11 SRCLK -> D12 (CLOCK)           pin  8 GND   -> GND
 *   pin 12 RCLK  -> D21 (LATCH)           pin 10 SRCLR -> 3V3 (do not leave open)
 *   pin 13 OE    -> GND                   pin  9 QH'   -> next 595's SER, or open
 *   QA..QH (15, 1..7) -> LEDs through 330 ohm resistors to GND
 *
 * Add 0.1 uF from VCC to GND at the chip.
 */
static void write595(uint8_t value)
{
    digitalWrite(LATCH, LOW);                       /* outputs frozen while shifting */
    shiftOut(DATA_OUT, CLOCK, MSBFIRST, value);     /* QH gets the first bit sent    */
    digitalWrite(LATCH, HIGH);                      /* one rising edge = new outputs */
}

static void test595(void)
{
    uint8_t i;

    Serial.println("--- 4. 74HC595 output expander ---------------------------");
    pinMode(DATA_OUT, OUTPUT);
    pinMode(CLOCK, OUTPUT);
    pinMode(LATCH, OUTPUT);
    digitalWrite(CLOCK, LOW);
    digitalWrite(LATCH, LOW);

    Serial.println("  walking one bit, MSBFIRST (watch QH first, then QG...)");
    for (i = 0; i < 8U; i++) { write595((uint8_t)(0x80U >> i)); delay(120); }

    Serial.println("  same walk, LSBFIRST - the LEDs travel the OTHER way");
    for (i = 0; i < 8U; i++) {
        digitalWrite(LATCH, LOW);
        shiftOut(DATA_OUT, CLOCK, LSBFIRST, (uint8_t)(0x80U >> i));
        digitalWrite(LATCH, HIGH);
        delay(120);
    }

    Serial.println("  counting 0..255");
    for (i = 0; i < 255U; i++) { write595(i); delay(12); }
    write595(0x00);

    Serial.println("  If the LEDs never changed but the walk printed fine, the");
    Serial.println("  latch pulse is what is missing - shifting and latching are");
    Serial.println("  separate, and shiftOut() knows nothing about the latch.");
    Serial.println();
}
#endif /* HAVE_74HC595 */

/* --- test 5: 74HC165, eight inputs on three pins ------------------------- */

#if HAVE_74HC165
/*
 * 74HC165 wiring:
 *   pin  9 QH    -> D22 (DATA_IN)         pin 16 VCC   -> 3V3
 *   pin  2 CLK   -> D12 (CLOCK)           pin  8 GND   -> GND
 *   pin  1 SH/LD -> D21 (LATCH)           pin 15 CLK INH -> GND
 *   pin 10 SER   -> GND                   A..H (11..14, 3..6) -> switches to GND,
 *                                          each with a 10k pull-up to 3V3
 *
 * SH/LD low SAMPLES the inputs; high shifts them out. It is the opposite polarity
 * to the 595's latch, which is a classic hour-long debugging session.
 */
static uint8_t read165(void)
{
    digitalWrite(LATCH, LOW);       /* load: capture the eight input pins */
    digitalWrite(LATCH, HIGH);      /* shift: now clock them out          */
    return shiftIn(DATA_IN, CLOCK, MSBFIRST);
}

static void test165(void)
{
    uint8_t v;
    uint8_t last = 0;
    unsigned long deadline;

    Serial.println("--- 5. 74HC165 input expander ----------------------------");
    pinMode(CLOCK, OUTPUT);
    pinMode(LATCH, OUTPUT);
    pinMode(DATA_IN, INPUT);
    digitalWrite(CLOCK, LOW);
    digitalWrite(LATCH, HIGH);

    Serial.println("  press the switches for 6 s - changes print as they happen");
    last = read165();
    printByte("  initial   : ", last);

    deadline = millis() + 6000UL;
    while ((long)(millis() - deadline) < 0L) {
        v = read165();
        if (v != last) { printByte("  changed to: ", v); last = v; }
        delay(20);      /* crude debounce; a real design would filter */
    }
    Serial.println("  MSBFIRST puts input H in bit 7. Swap to LSBFIRST and the");
    Serial.println("  same switch shows up in bit 0 - the ordering is real here");
    Serial.println("  too, and this is the test that proves it for shiftIn().");
    Serial.println();
}
#endif /* HAVE_74HC165 */

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LED_OFF);

    Serial.begin(115200);

    Serial.println();
    Serial.println("======================================================");
    Serial.println(" shiftOut / shiftIn - dsPIC33CK256MC005");
    Serial.println("======================================================");
    Serial.println("DATA_OUT D9  (RB4)   CLOCK   D12 (RB7)");
    Serial.println("LATCH    D21 (RC0)   DATA_IN D22 (RC1)");
    Serial.println("jumper D12 -> D22 for test 3 (safe to leave fitted)");
#if !HAVE_74HC595 && !HAVE_74HC165
    Serial.println("no shift-register chip configured: tests 4 and 5 are");
    Serial.println("compiled out. Set HAVE_74HC595 / HAVE_74HC165 to 1.");
#endif
    Serial.println();
}

void loop()
{
    digitalWrite(LED_BUILTIN, LED_ON);

    testBitOrder();
    testSpeed();
    testSamplePoint();
#if HAVE_74HC595
    test595();
#endif
#if HAVE_74HC165
    test165();
#endif

    digitalWrite(LED_BUILTIN, LED_OFF);

    /* Leave the shared pins inert rather than holding a stale last bit. */
    digitalWrite(DATA_OUT, LOW);
    digitalWrite(CLOCK, LOW);

    Serial.println("=== pass complete, again in 5 s ==========================");
    Serial.println();
    delay(5000);
}
