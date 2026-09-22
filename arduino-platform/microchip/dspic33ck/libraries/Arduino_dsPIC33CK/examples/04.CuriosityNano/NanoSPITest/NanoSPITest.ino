/*
 * NanoSPITest - the SPI library on the dsPIC33CK256MC005 Curiosity Nano
 *               (EV08P02A), tested against itself with one jumper wire
 *
 * WIRING - one jumper, no parts, no peripheral needed
 *
 *   D26 (RC5, MOSI) ----------- D33 (RC12, MISO)
 *
 * That is loopback: every byte the master shifts out comes straight back in. If
 * transfer(0x5A) returns 0x5A then the clock, the output pin, the input pin, the
 * peripheral pin select mapping and the shift register are all working -- which is
 * everything except the device you have not connected yet. It is the first thing
 * to try when a real SPI peripheral will not talk, because it separates "my wiring
 * is wrong" from "my configuration is wrong".
 *
 * The full bus on this board:
 *   SCK  = D25 (RC4)
 *   MOSI = D26 (RC5)
 *   MISO = D33 (RC12)
 *   SS   = D34 (RC13)   -- SOFTWARE controlled, see below
 *
 * These four were chosen to stay clear of the debugger (RB5/RB6), I2C (RB8/RB9),
 * the four PWM pins (RB0-RB3), the CDC UART (RC10/RC11) and the HRPWM generator
 * outputs. None of them costs an ADC channel.
 *
 * THREE THINGS ABOUT THIS IMPLEMENTATION THAT WILL SURPRISE YOU
 *
 * 1. SS is not touched for you. SPI.begin() makes D34 an output and then leaves
 *    it alone forever. beginTransaction() does not assert it and endTransaction()
 *    does not release it. Every real device needs it toggled around each
 *    transaction, and that is your sketch's job:
 *        digitalWrite(SS, LOW);  ... transfers ...  digitalWrite(SS, HIGH);
 *
 * 2. setBitOrder() does nothing at all. The dsPIC33CK SPI has no bit-order
 *    control bit; it is MSB-first in hardware, always. The call compiles, returns,
 *    and changes nothing -- so a driver ported from AVR that relies on LSBFIRST
 *    will be quietly wrong. Test 4 below demonstrates this rather than just
 *    asserting it. If you need LSB-first, reverse the bits yourself.
 *
 * 3. Asking for a clock that is too fast gives you the SLOWEST clock, not the
 *    fastest. beginTransaction() computes BRG = FCY/(2*clock) - 1 in unsigned
 *    arithmetic. Request more than FCY/2 and FCY/(2*clock) is 0, the subtraction
 *    wraps to 4294967295, and the cap at 8191 hands you the slowest setting the
 *    hardware has. Test 6 measures it, and it is a difference of four orders of
 *    magnitude. The maximum usable SCK is FCY/2.
 *
 * ALSO WORTH KNOWING
 *   - transfer16() is not a 16-bit frame. It is two 8-bit transfers, high byte
 *     first, so SS stays low across both but the peripheral sees two bytes. A
 *     device expecting a genuine 16-bit frame will not be satisfied by it.
 *   - transfer() is polled and blocks. There is no timeout, but the shift is
 *     hardware and always completes, so it cannot hang the way the I2C driver can.
 *   - setClockDivider(n) gives SCK = FCY/n. n = 1 is not valid arithmetic here
 *     ((1/2)-1 underflows) -- use SPI_CLOCK_DIV2 for the fastest clock.
 *   - The object is really called ArduinoSPI. SPI.h does `#define SPI ArduinoSPI`
 *     because every p33CK device header already typedefs the bare name SPI for
 *     its SFR blocks, and a typedef cannot be undefined. Write SPI.* as normal.
 *
 * Serial Monitor at 115200.
 *
 * Board: Tools > Board > "Arduino_dsPIC33CK (dsPIC33CK256MC005 Curiosity Nano)"
 */

#include <Arduino.h>
#include <SPI.h>

#if defined(LED_BUILTIN_ACTIVE_LOW) && LED_BUILTIN_ACTIVE_LOW
  #define LED_ON   LOW
  #define LED_OFF  HIGH
#else
  #define LED_ON   HIGH
  #define LED_OFF  LOW
#endif

/* Fastest SCK this device can produce: BRG = 0 gives FCY/2. Computed from FCY so
 * it is right for both board clock options (FCY 4 MHz internal, FCY 100 MHz PLL). */
#define SPI_MAX_SCK     (FCY / 2UL)

#define TIMING_BYTES    200U

static uint8_t g_loopback = 0;

/* --- small helpers --------------------------------------------------------- */

static void printHex2(uint8_t v)
{
    if (v < 0x10U) { Serial.print('0'); }
    Serial.print(v, HEX);
}

static void printHex4(uint16_t v)
{
    printHex2((uint8_t)(v >> 8));
    printHex2((uint8_t)(v & 0xFFU));
}

/* SS is ours to drive. Wrapping it in a pair of helpers is not ceremony -- it is
 * the only thing keeping a forgotten digitalWrite() from turning into a device
 * that ignores every transaction. */
static void selectSlave(void)   { digitalWrite(SS, LOW);  }
static void deselectSlave(void) { digitalWrite(SS, HIGH); }

static const char *modeName(uint8_t mode)
{
    switch (mode) {
    case SPI_MODE0: return "MODE0 (CPOL=0 CPHA=0)";
    case SPI_MODE1: return "MODE1 (CPOL=0 CPHA=1)";
    case SPI_MODE2: return "MODE2 (CPOL=1 CPHA=0)";
    case SPI_MODE3: return "MODE3 (CPOL=1 CPHA=1)";
    default:        return "?";
    }
}

/* --- test 1: is the jumper there? ----------------------------------------- */

static uint8_t detectLoopback(void)
{
    uint8_t a;
    uint8_t b;

    SPI.beginTransaction(SPISettings(SPI_MAX_SCK / 4UL, MSBFIRST, SPI_MODE0));
    selectSlave();
    a = SPI.transfer(0xA5);
    b = SPI.transfer(0x5A);
    deselectSlave();
    SPI.endTransaction();

    Serial.println("--- 1. loopback detection --------------------------------");
    Serial.print  ("  sent 0xA5 0x5A, received 0x");
    printHex2(a);
    Serial.print  (" 0x");
    printHex2(b);
    Serial.println();

    if (a == 0xA5U && b == 0x5AU) {
        Serial.println("  jumper present - full test suite will run");
        Serial.println();
        return 1U;
    }

    Serial.println("  no loopback. Fit a jumper from D26 (MOSI) to D33 (MISO).");
    Serial.print  ("  All-zero means MISO is held low; all-ones (0xFF) means it");
    Serial.println(" is floating high.");
    Serial.println("  The clock and timing tests still run; the data checks are");
    Serial.println("  skipped because there is nothing to compare against.");
    Serial.println();
    return 0U;
}

/* --- test 2: every byte value round-trips --------------------------------- */

static void testAllBytes(void)
{
    unsigned int errors = 0;
    unsigned int v;
    uint8_t      firstBad = 0;
    uint8_t      firstGot = 0;

    Serial.println("--- 2. all 256 byte values -------------------------------");

    SPI.beginTransaction(SPISettings(SPI_MAX_SCK / 4UL, MSBFIRST, SPI_MODE0));
    selectSlave();

    for (v = 0; v < 256U; v++) {
        uint8_t got = SPI.transfer((uint8_t)v);

        if (got != (uint8_t)v) {
            if (errors == 0U) {
                firstBad = (uint8_t)v;
                firstGot = got;
            }
            errors++;
        }
    }

    deselectSlave();
    SPI.endTransaction();

    if (errors == 0U) {
        Serial.println("  256/256 correct");
    } else {
        Serial.print  ("  ");
        Serial.print(errors);
        Serial.print  (" mismatches, first at 0x");
        printHex2(firstBad);
        Serial.print  (" -> 0x");
        printHex2(firstGot);
        Serial.println();
    }

    Serial.println();
}

/* --- test 3: transfer16 is two bytes, not one frame ----------------------- */

static void testTransfer16(void)
{
    uint16_t got;

    Serial.println("--- 3. transfer16 ----------------------------------------");

    SPI.beginTransaction(SPISettings(SPI_MAX_SCK / 4UL, MSBFIRST, SPI_MODE0));
    selectSlave();
    got = SPI.transfer16(0x1234);
    deselectSlave();
    SPI.endTransaction();

    Serial.print  ("  sent 0x1234, received 0x");
    printHex4(got);
    Serial.println((got == 0x1234U) ? "  OK" : "  mismatch");
    Serial.println("  Note this was TWO 8-bit transfers, 0x12 then 0x34. SS");
    Serial.println("  stayed low across both, but a device expecting a real");
    Serial.println("  16-bit frame sees two separate bytes.");
    Serial.println();
}

/* --- test 4: setBitOrder does nothing ------------------------------------- */

/*
 * 0x01 is the useful probe: MSB-first it goes out as 00000001 and comes back as
 * 0x01; if the hardware honoured LSBFIRST the same bits would arrive reversed and
 * read back as 0x80. Getting 0x01 after asking for LSBFIRST is the proof.
 */
static void testBitOrder(void)
{
    uint8_t msb;
    uint8_t lsb;

    Serial.println("--- 4. setBitOrder(LSBFIRST) is a no-op ------------------");

    SPI.beginTransaction(SPISettings(SPI_MAX_SCK / 4UL, MSBFIRST, SPI_MODE0));
    selectSlave();

    SPI.setBitOrder(MSBFIRST);
    msb = SPI.transfer(0x01);

    SPI.setBitOrder(LSBFIRST);
    lsb = SPI.transfer(0x01);

    deselectSlave();
    SPI.endTransaction();

    SPI.setBitOrder(MSBFIRST);

    Serial.print  ("  0x01 with MSBFIRST -> 0x");
    printHex2(msb);
    Serial.print  ("   with LSBFIRST -> 0x");
    printHex2(lsb);
    Serial.println();

    if (msb == lsb) {
        Serial.println("  identical, as documented: the bit order did not change.");
        Serial.println("  A real LSB-first result would have read 0x80.");
    } else {
        Serial.println("  they differ - the library has gained bit-order support");
        Serial.println("  since this sketch was written. Good news; update it.");
    }

    Serial.println();
}

/* --- test 5: the four data modes ------------------------------------------ */

static void testDataModes(void)
{
    static const uint8_t modes[] = { SPI_MODE0, SPI_MODE1, SPI_MODE2, SPI_MODE3 };
    size_t i;

    Serial.println("--- 5. data modes ----------------------------------------");

    for (i = 0; i < (sizeof(modes) / sizeof(modes[0])); i++) {
        uint8_t got;

        SPI.beginTransaction(SPISettings(SPI_MAX_SCK / 4UL, MSBFIRST, modes[i]));
        selectSlave();
        got = SPI.transfer(0x96);
        deselectSlave();
        SPI.endTransaction();

        Serial.print("  ");
        Serial.print(modeName(modes[i]));
        Serial.print(" : 0x");
        printHex2(got);
        Serial.println((got == 0x96U) ? "  OK" : "  mismatch");
    }

    Serial.println("  Loopback passing in all four modes is expected: the same");
    Serial.println("  clock edge that shifts a bit out shifts it back in. It says");
    Serial.println("  nothing about which mode your peripheral wants -- for that,");
    Serial.println("  read its datasheet.");
    Serial.println();
}

/* --- test 6: clock dividers, and the too-fast trap ----------------------- */

static unsigned long timeBytes(unsigned int count)
{
    unsigned long start;
    unsigned long elapsed;
    unsigned int  i;

    selectSlave();
    start = micros();

    for (i = 0; i < count; i++) {
        (void)SPI.transfer(0x00);
    }

    elapsed = micros() - start;
    deselectSlave();

    return elapsed;
}

static void testClockDividers(void)
{
    static const uint8_t divs[] = { SPI_CLOCK_DIV2,  SPI_CLOCK_DIV4,
                                    SPI_CLOCK_DIV8,  SPI_CLOCK_DIV16,
                                    SPI_CLOCK_DIV32, SPI_CLOCK_DIV64,
                                    SPI_CLOCK_DIV128 };
    size_t i;

    Serial.println("--- 6. setClockDivider: SCK = FCY / divider --------------");
    Serial.print  ("  FCY = ");
    Serial.print(FCY / 1000UL);
    Serial.print  (" kHz, so the fastest SCK is ");
    Serial.print(SPI_MAX_SCK / 1000UL);
    Serial.println(" kHz");
    Serial.println("  divider   expected SCK   measured us/byte");

    for (i = 0; i < (sizeof(divs) / sizeof(divs[0])); i++) {
        unsigned long elapsed;

        SPI.setClockDivider(divs[i]);
        elapsed = timeBytes(TIMING_BYTES);

        Serial.print("     ");
        Serial.print((unsigned int)divs[i]);
        Serial.print("\t   ");
        Serial.print(FCY / (unsigned long)divs[i] / 1000UL);
        Serial.print(" kHz\t     ");
        Serial.println(elapsed / TIMING_BYTES);
    }

    Serial.println("  us/byte should roughly double down the list. It is always");
    Serial.println("  more than 8 SCK periods: the driver loads the buffer and");
    Serial.println("  polls for completion between bytes, and at high clocks that");
    Serial.println("  software overhead dominates.");
    Serial.println();

    /* --- the trap ---------------------------------------------------------- */
    Serial.println("--- 6b. asking for a clock faster than FCY/2 -------------");
    Serial.println("  (the second measurement is genuinely slow - wait for it)");

    SPI.beginTransaction(SPISettings(SPI_MAX_SCK, MSBFIRST, SPI_MODE0));
    Serial.print  ("  requested FCY/2 (");
    Serial.print(SPI_MAX_SCK / 1000UL);
    Serial.print  (" kHz): ");
    Serial.print(timeBytes(TIMING_BYTES) / TIMING_BYTES);
    Serial.println(" us/byte");
    SPI.endTransaction();

    SPI.beginTransaction(SPISettings(FCY, MSBFIRST, SPI_MODE0));
    Serial.print  ("  requested FCY   (");
    Serial.print(FCY / 1000UL);
    Serial.print  (" kHz): ");
    Serial.print(timeBytes(TIMING_BYTES) / TIMING_BYTES);
    Serial.println(" us/byte  <-- SLOWER, not faster");
    SPI.endTransaction();

    Serial.println("  BRG = FCY/(2*clock) - 1 in unsigned arithmetic: the");
    Serial.println("  division gives 0, the subtraction wraps, and the cap at");
    Serial.println("  8191 selects the slowest clock available. Clamp your own");
    Serial.println("  clock requests to FCY/2 - nothing in the library will.");
    Serial.println();

    /* Leave the bus at a sane speed for whatever runs next. */
    SPI.beginTransaction(SPISettings(SPI_MAX_SCK / 4UL, MSBFIRST, SPI_MODE0));
    SPI.endTransaction();
}

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LED_OFF);

    Serial.begin(115200);

    SPI.begin();

    /* SPI.begin() made SS an output but left its level undefined. Deselect it
     * before anything else, or the first transaction may find it already low. */
    pinMode(SS, OUTPUT);
    deselectSlave();

    Serial.println();
    Serial.println("======================================================");
    Serial.println(" SPI1 loopback - dsPIC33CK256MC005 Curiosity Nano");
    Serial.println("======================================================");
    Serial.print  ("SCK  : D");
    Serial.print(SCK);
    Serial.println(" (RC4)");
    Serial.print  ("MOSI : D");
    Serial.print(MOSI);
    Serial.println(" (RC5)   <-- jumper this...");
    Serial.print  ("MISO : D");
    Serial.print(MISO);
    Serial.println(" (RC12)  <-- ...to this");
    Serial.print  ("SS   : D");
    Serial.print(SS);
    Serial.println(" (RC13)  software controlled, always");
    Serial.print  ("max SCK: ");
    Serial.print(SPI_MAX_SCK / 1000UL);
    Serial.println(" kHz (FCY/2)");
    Serial.println();
}

void loop()
{
    digitalWrite(LED_BUILTIN, LED_ON);

    g_loopback = detectLoopback();

    if (g_loopback) {
        testAllBytes();
        testTransfer16();
        testBitOrder();
        testDataModes();
    }

    testClockDividers();

    digitalWrite(LED_BUILTIN, LED_OFF);

    Serial.println("=== pass complete, again in 5 s ==========================");
    Serial.println();
    delay(5000);
}
