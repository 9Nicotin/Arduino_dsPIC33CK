/*
 * NanoI2CTest - the Wire library on the dsPIC33CK256MC005 Curiosity Nano
 *               (EV08P02A): bus checks, an address scan, and a safe read
 *
 * WIRING
 *   D13 (RB8) = SCL1        these are DEDICATED I2C1 pins on this device.
 *   D14 (RB9) = SDA1        There is no peripheral pin select for I2C here, so
 *                           the bus is on these two pins and nowhere else.
 *   GND       = the device's ground. Always.
 *
 *   PULL-UP RESISTORS ARE NOT OPTIONAL: 4.7k from SCL to 3V3 and 4.7k from SDA
 *   to 3V3. I2C drivers only ever pull a line DOWN; the resistors are what pulls
 *   it back up. Most breakout boards already have them -- two boards on one bus
 *   means two pairs in parallel, which is usually still fine; five boards is not.
 *
 *   The MCU's internal pull-ups are tens of kilohms, far too weak for I2C's
 *   rise-time requirement. This sketch uses them to TEST the bus and then turns
 *   them off. Do not rely on them to run it.
 *
 * With nothing connected the scan simply finds nothing, which is a valid result
 * and worth seeing once, because it looks different from a hung bus.
 *
 * READ THIS BEFORE YOU DEBUG A HANG: THE DRIVER HAS NO TIMEOUTS
 * Every wait in Wire.c is a bare spin -- `while (I2C1CONLbits.SEN);`,
 * `while (I2C1STATbits.TRSTAT);` and so on. If SCL or SDA is stuck LOW (missing
 * pull-ups, a slave that was reset mid-byte, a short), the first transaction
 * never completes and your sketch stops dead with no message. Not slow, not an
 * error code: stopped.
 *
 * That is why this sketch checks the two lines BEFORE calling Wire.begin() and
 * refuses to go on if they are not both high. Worth copying into your own code:
 * a five-line pre-check is cheaper than a debugging session.
 *
 * WHAT WIRE GIVES YOU HERE
 *   Wire.begin()                  master only. There is NO slave mode in this
 *                                 library -- no begin(address), no onReceive(),
 *                                 no onRequest().
 *   Wire.setClock(hz)             100 kHz default; 400 kHz works on most parts.
 *   Wire.beginTransmission(addr)  addr is the 7-bit address; the library adds
 *                                 the R/W bit. If a datasheet gives you 0xA0,
 *                                 that is 8-bit -- use 0x50.
 *   Wire.write(b) / writeBytes()  buffered, 32 bytes max (WIRE_BUFFER_SIZE).
 *                                 write() past the end returns 0 and DROPS the
 *                                 byte; check the return value on long writes.
 *   Wire.endTransmission()        0 = all bytes acknowledged
 *                                 2 = nobody answered the address
 *                                 3 = a data byte was not acknowledged
 *                                 (upstream Arduino's 1 and 4 never occur here)
 *   Wire.requestFrom(addr, n)     returns the number of bytes actually read, so
 *                                 0 means the device did not answer. n is capped
 *                                 at 32, silently.
 *   Wire.available() / read()     the usual pair.
 *
 * Everything is polled. A transaction occupies the CPU for its whole duration:
 * about 90 us per byte at 100 kHz, which is roughly 360 instruction cycles the
 * loop does not get at FCY = 4 MHz.
 *
 * NOTE ON PINS: RB8 and RB9 are also A11 and A12. Calling analogRead(A11) while
 * the bus is up sets ANSEL on SCL and takes the pin away from I2C. Do not.
 *
 * Serial Monitor at 115200.
 *
 * Board: Tools > Board > "Arduino_dsPIC33CK (dsPIC33CK256MC005 Curiosity Nano)"
 */

#include <Arduino.h>
#include <Wire.h>

#if defined(LED_BUILTIN_ACTIVE_LOW) && LED_BUILTIN_ACTIVE_LOW
  #define LED_ON   LOW
  #define LED_OFF  HIGH
#else
  #define LED_ON   HIGH
  #define LED_OFF  LOW
#endif

/* 0x00-0x07 and 0x78-0x7F are reserved by the I2C specification (general call,
 * 10-bit addressing, device ID). Scanning them can confuse conforming devices,
 * so the range below is the one every scanner uses. */
#define SCAN_FIRST  0x08
#define SCAN_LAST   0x77

#define MAX_FOUND   8

static uint8_t g_found[MAX_FOUND];
static uint8_t g_foundCount = 0;
static uint8_t g_busOk      = 0;

/* --- bus checks, before the peripheral is enabled -------------------------- */

/*
 * Is an external pull-up actually fitted?
 *
 * Drive the line low, release it as a plain input with no internal pull-up, and
 * read it immediately. With a 4.7k pull-up and normal bus capacitance the line is
 * back high in well under a microsecond. With nothing fitted, only the pin's own
 * leakage can charge it and it stays low for milliseconds.
 *
 * Briefly pulling an idle I2C line low is what every master does, so this is safe
 * at startup. Do not call it in the middle of a transaction.
 */
static uint8_t hasPullup(uint8_t pin)
{
    uint8_t level;

    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    delayMicroseconds(10);

    pinMode(pin, INPUT);        /* releases the pin AND clears the internal pull-up */
    delayMicroseconds(5);

    level = (digitalRead(pin) == HIGH) ? 1U : 0U;

    return level;
}

/* Is anything holding the line down? Tested with the internal pull-up on, so a
 * LOW here means something is actively driving it, not that it is floating. */
static uint8_t lineIsFree(uint8_t pin)
{
    pinMode(pin, INPUT_PULLUP);
    delayMicroseconds(50);

    return (digitalRead(pin) == HIGH) ? 1U : 0U;
}

static uint8_t checkBus(void)
{
    uint8_t sclPull = hasPullup(SCL);
    uint8_t sdaPull = hasPullup(SDA);
    uint8_t sclFree = lineIsFree(SCL);
    uint8_t sdaFree = lineIsFree(SDA);

    Serial.println("--- bus pre-check (before Wire.begin) --------------------");
    Serial.print  ("  SCL (D13/RB8): external pull-up ");
    Serial.print(sclPull ? "yes" : "NO");
    Serial.print  (", line free ");
    Serial.println(sclFree ? "yes" : "NO");

    Serial.print  ("  SDA (D14/RB9): external pull-up ");
    Serial.print(sdaPull ? "yes" : "NO");
    Serial.print  (", line free ");
    Serial.println(sdaFree ? "yes" : "NO");

    /* Leave both pins as plain inputs; Wire.begin() hands them to the
     * peripheral and the internal pull-ups must not be left on. */
    pinMode(SCL, INPUT);
    pinMode(SDA, INPUT);

    if (!sclFree || !sdaFree) {
        Serial.println();
        Serial.println("  STOP: a line is being held LOW. Wire.c has no");
        Serial.println("  timeouts, so the first transaction would hang this");
        Serial.println("  sketch permanently. Causes, in order of likelihood:");
        Serial.println("    - no pull-up resistors at all");
        Serial.println("    - a slave reset part-way through a byte (power-cycle it)");
        Serial.println("    - SDA or SCL shorted to GND");
        Serial.println();
        return 0;
    }

    if (!sclPull || !sdaPull) {
        Serial.println();
        Serial.println("  WARNING: no external pull-up detected. The scan may");
        Serial.println("  still appear to work at 100 kHz on a short wire, and");
        Serial.println("  will fail intermittently forever after. Fit 4.7k to");
        Serial.println("  3V3 on both lines.");
        Serial.println();
    } else {
        Serial.println("  bus looks healthy");
        Serial.println();
    }

    return 1;
}

/* --- the scan -------------------------------------------------------------- */

static void printHex2(uint8_t v)
{
    if (v < 0x10U) {
        Serial.print('0');
    }
    Serial.print(v, HEX);
}

/*
 * Address a device for writing and send no data at all. A device that is present
 * acknowledges its address, endTransmission() returns 0, and nothing was written
 * -- which is what makes this safe to do to an unknown device.
 *
 * Status 3 (NACK on data) cannot occur here because there is no data. If you
 * ever see it from this loop, something is wrong with the driver, not the bus.
 */
static void scan(void)
{
    uint8_t addr;

    g_foundCount = 0;

    Serial.println("--- address scan 0x08..0x77 ------------------------------");
    Serial.println("       0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f");

    for (addr = 0; addr < 0x80U; addr++) {
        if ((addr & 0x0FU) == 0U) {
            Serial.println();
            Serial.print("  ");
            printHex2((uint8_t)(addr & 0xF0U));
            Serial.print(":");
        }

        if (addr < SCAN_FIRST || addr > SCAN_LAST) {
            Serial.print("   ");          /* reserved - not probed */
            continue;
        }

        Wire.beginTransmission(addr);

        if (Wire.endTransmission() == 0U) {
            Serial.print(" ");
            printHex2(addr);

            if (g_foundCount < MAX_FOUND) {
                g_found[g_foundCount] = addr;
                g_foundCount++;
            }
        } else {
            Serial.print(" --");
        }
    }

    Serial.println();
    Serial.println();
    Serial.print("  devices found: ");
    Serial.println(g_foundCount);

    if (g_foundCount == 0U) {
        Serial.println("  (a clean 'nothing there' - the bus is alive and every");
        Serial.println("   address returned status 2, NACK on address)");
    } else {
        uint8_t i;

        for (i = 0; i < g_foundCount; i++) {
            Serial.print("    0x");
            printHex2(g_found[i]);
            Serial.print("  (8-bit write address 0x");
            printHex2((uint8_t)(g_found[i] << 1));
            Serial.print(", read 0x");
            printHex2((uint8_t)((g_found[i] << 1) | 1U));
            Serial.println(")");
        }
    }

    Serial.println();
}

/* --- clock speeds ---------------------------------------------------------- */

/*
 * Re-scan at each speed. A device that answers at 100 kHz and not at 400 kHz is
 * either a standard-mode-only part or a sign that the bus capacitance is too high
 * for the pull-ups you fitted -- weaker resistors, shorter wires, or stay at
 * 100 kHz.
 *
 * setClock() computes BRG = FCY/(2*clock) - 1. At FCY = 4 MHz (the default 8 MHz
 * internal clock setting) that is 19 for 100 kHz and 4 for 400 kHz, so the step
 * from 400 kHz upwards gets very coarse. The 200 MHz PLL board option gives
 * much finer control.
 */
static void scanAtSpeeds(void)
{
    static const unsigned long speeds[] = { 100000UL, 400000UL };
    size_t i;

    for (i = 0; i < (sizeof(speeds) / sizeof(speeds[0])); i++) {
        Serial.print("=== SCL = ");
        Serial.print(speeds[i] / 1000UL);
        Serial.println(" kHz ===================================");

        Wire.setClock(speeds[i]);
        delay(5);
        scan();
    }

    Wire.setClock(100000UL);
}

/* --- a read that is safe on an unknown device ------------------------------ */

/*
 * Reading is safe; writing is not. A bare write of one byte to an unknown device
 * is a register address on some parts, a command on others, and the first byte of
 * an EEPROM page write on others again -- there is no way to know which, so this
 * sketch never writes data to a device it discovered.
 *
 * If you know what is on your bus, the write test at the bottom of this file is
 * three lines. Enable it deliberately, for a device you have a datasheet for.
 */
static void readFirstDevice(void)
{
    uint8_t addr;
    uint8_t got;
    uint8_t n = 0;

    if (g_foundCount == 0U) {
        return;
    }

    addr = g_found[0];

    Serial.print("--- reading 4 bytes from 0x");
    printHex2(addr);
    Serial.println(" ----------------------------");

    got = Wire.requestFrom(addr, 4U);

    Serial.print("  requestFrom returned ");
    Serial.print(got);
    Serial.print(", available() = ");
    Serial.println(Wire.available());

    if (got == 0U) {
        Serial.println("  the device acknowledged a write address but not a read");
        Serial.println("  address. Some devices are write-only; others need a");
        Serial.println("  register address written first.");
        Serial.println();
        return;
    }

    Serial.print("  bytes:");
    while (Wire.available() > 0) {
        int b = Wire.read();

        Serial.print(" 0x");
        printHex2((uint8_t)b);
        n++;
    }

    Serial.println();
    Serial.print("  read ");
    Serial.print(n);
    Serial.println(" byte(s). What they mean is the device's business, not ours.");
    Serial.println();

#if 0
    /* --- WRITE TEST - enable only for a device you have a datasheet for ----
     * This writes one byte. On an EEPROM that starts a page write. On a sensor
     * it selects a register. On a display it is a command. Know which before
     * you uncomment it. */
    Wire.beginTransmission(addr);
    Wire.write(0x00);                       /* register / address byte */
    Serial.print("  write status: ");
    Serial.println(Wire.endTransmission()); /* 0 = acknowledged */
#endif
}

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LED_OFF);

    Serial.begin(115200);

    Serial.println();
    Serial.println("======================================================");
    Serial.println(" I2C / Wire - dsPIC33CK256MC005 Curiosity Nano");
    Serial.println("======================================================");
    Serial.print  ("SCL : D");
    Serial.print(SCL);
    Serial.println(" (RB8)  - dedicated I2C1 pin, also A11");
    Serial.print  ("SDA : D");
    Serial.print(SDA);
    Serial.println(" (RB9)  - dedicated I2C1 pin, also A12");
    Serial.println("mode: master only, polled, 32-byte buffer, no timeouts");
    Serial.println();

    g_busOk = checkBus();

    if (!g_busOk) {
        Serial.println("Halted. Fix the wiring and reset the board.");
        return;
    }

    Wire.begin();
    delay(10);
}

void loop()
{
    if (!g_busOk) {
        /* Slow blink means "check the bus". Not stepping on the serial port with
         * a repeated error message is deliberate. */
        digitalWrite(LED_BUILTIN, LED_ON);
        delay(100);
        digitalWrite(LED_BUILTIN, LED_OFF);
        delay(900);
        return;
    }

    digitalWrite(LED_BUILTIN, LED_ON);

    scanAtSpeeds();
    readFirstDevice();

    digitalWrite(LED_BUILTIN, LED_OFF);

    Serial.println("=== pass complete, again in 5 s ==========================");
    Serial.println();
    delay(5000);
}
