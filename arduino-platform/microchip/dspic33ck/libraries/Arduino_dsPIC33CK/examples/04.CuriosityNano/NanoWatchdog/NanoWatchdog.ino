/*
 * NanoWatchdog - the watchdog timer and reset causes on the
 *                dsPIC33CK256MC005 Curiosity Nano (EV08P02A)
 *
 * A watchdog is the difference between a product that recovers from a hang and one
 * that has to be unplugged. This sketch enables it, deliberately lets it bite,
 * MEASURES how long it took, proves that feeding it prevents the reset, and shows
 * the single most common way sketches get bitten by their own watchdog.
 *
 * It runs itself across three resets and then settles down. Press SW0 to run the
 * whole sequence again.
 *
 * WIRING: none. LED0 and SW0 are on the board.
 *
 * READ THIS BEFORE RUNNING IT
 *   This sketch resets the board on purpose, twice. On a Curiosity Nano the
 *   on-board debugger is also the USB-serial bridge, so the Serial Monitor may
 *   disconnect at each reset and need reopening. That is the bridge reacting to
 *   the reset, not a fault. If the port goes quiet for good, unplug and replug --
 *   see the nEDBG note in the platform docs.
 *
 * WHY THE TIMEOUT IS MEASURED AND NOT CALCULATED
 *   The period is (WDT clock) / 2^RUNDIV, and RUNDIV is readable, so two thirds of
 *   the sum is easy. The WDT clock is the problem. RCLKSEL picks between LPRC
 *   (~32 kHz), FRC and FCY -- and system_config.c sets RCLKSEL explicitly only in
 *   the MP508 branch. On this device it is left at the erased configuration-word
 *   default, so the honest thing to do is measure it. A ~1 s result means LPRC is
 *   driving it; single-digit milliseconds means FCY at 4 MHz. Test 3 prints the
 *   candidates and then the measurement, so you can see which one is real.
 *
 * HOW THE MEASUREMENT WORKS -- persistent variables
 *   __attribute__((persistent)) tells the linker to place a variable in a section
 *   that the C startup code does NOT clear. It therefore survives any reset that
 *   does not cut power, which is exactly what is needed to carry a measurement out
 *   through a reset and read it back afterwards. The sketch stores millis() into a
 *   persistent variable as fast as it can while starving the watchdog; whatever
 *   value is there when the board comes back is how long the watchdog took.
 *
 *   The variables are volatile as well as persistent. Without volatile the
 *   compiler would notice that only the last write of the starve loop can matter
 *   and hoist it out of the loop -- and the loop never finishes normally, so the
 *   write would never happen at all and the measurement would read zero.
 *
 *   After a fresh programming cycle RAM holds garbage, so a magic number
 *   validates the block before any of it is trusted.
 *
 * FIVE FACTS ABOUT THE WATCHDOG ON THIS PART
 *
 * 1. It is OFF at startup and a sketch may turn it on. system_config.c sets
 *    FWDTEN = ON_SW, which hands control to software: WDTCONLbits.ON = 1 arms it,
 *    = 0 disarms it. Had it been FWDTEN = ON the watchdog would be permanently on
 *    and this sketch could not disable it at the end.
 *
 * 2. There is no ClrWdt() macro in this device header -- grep for it and you get
 *    nothing. The instruction is reached with __builtin_clrwdt(), which is what
 *    "feeding the dog" compiles to: a single CLRWDT instruction.
 *
 * 3. RCON tells you why you rebooted, and NOTHING EVER CLEARS IT. The bits are
 *    sticky across resets by design, so after a power-up followed by a watchdog
 *    reset both POR and WDTO are set and you cannot tell which came last. This
 *    core does not clear them for you: read RCON early, save what you need, then
 *    clear the bits you consumed. Test 1 does exactly that.
 *
 * 4. A watchdog reset is not a power-on reset. Peripherals return to their reset
 *    state and millis() restarts from zero, but RAM keeps its contents -- which is
 *    what makes the persistent trick work, and also means a corrupt global stays
 *    corrupt through the reset that was supposed to fix it. If a hang might be
 *    caused by bad state, validate that state on the way up.
 *
 * 5. THE TRAP: delay() does not feed the watchdog. Nor does Serial.print() on a
 *    long string, nor pulseIn() waiting for its timeout, nor any of the library's
 *    spin loops -- and this library's I2C and SPI wait loops have no timeouts at
 *    all, so a stuck bus is an infinite loop. Any of them longer than the watchdog
 *    period is a reset. Test 5 demonstrates it with delay().
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

#define MAGIC           0x33C5U     /* marks the persistent block as initialised */

#define STAGE_IDLE      0U
#define STAGE_STARVING  1U          /* deliberately not feeding: expect a reset  */
#define STAGE_IN_DELAY  2U          /* inside delay() with no feeding: ditto     */
#define STAGE_FINISHED  3U

/* Give up rather than hang forever if the watchdog turns out never to fire. */
#define STARVE_LIMIT_MS 30000UL

/*
 * The persistent block. Survives reset; not cleared by startup code.
 * volatile for the reason explained in the header.
 */
static volatile unsigned int  __attribute__((persistent)) g_magic;
static volatile unsigned int  __attribute__((persistent)) g_stage;
static volatile unsigned long __attribute__((persistent)) g_heartbeat;
static volatile unsigned long __attribute__((persistent)) g_measuredMs;
static volatile unsigned int  __attribute__((persistent)) g_wdtResets;
static volatile unsigned int  __attribute__((persistent)) g_runs;

static unsigned int g_rconAtBoot = 0;   /* RCON as it was, before we cleared it */

/* --- reset cause --------------------------------------------------------- */

static void reportBit(const char *name, unsigned int set, const char *meaning)
{
    Serial.print("  ");
    Serial.print(name);
    Serial.print(set ? " = 1  " : " = 0  ");
    if (set) { Serial.println(meaning); } else { Serial.println(); }
}

static void decodeRcon(void)
{
    Serial.println("--- 1. why did we just boot? (RCON) ----------------------");
    Serial.print  ("  RCON at boot = 0x");
    Serial.println(g_rconAtBoot, HEX);

    reportBit("POR   ", (g_rconAtBoot >> 0)  & 1U, "power-on reset");
    reportBit("BOR   ", (g_rconAtBoot >> 1)  & 1U, "brown-out reset");
    reportBit("IDLE  ", (g_rconAtBoot >> 2)  & 1U, "woke from Idle");
    reportBit("SLEEP ", (g_rconAtBoot >> 3)  & 1U, "woke from Sleep");
    reportBit("WDTO  ", (g_rconAtBoot >> 4)  & 1U, "WATCHDOG TIME-OUT");
    reportBit("SWR   ", (g_rconAtBoot >> 6)  & 1U, "software reset (__asm__ reset)");
    reportBit("EXTR  ", (g_rconAtBoot >> 7)  & 1U, "external MCLR reset");
    reportBit("VREGS ", (g_rconAtBoot >> 8)  & 1U, "regulator stayed on in Sleep");
    reportBit("CM    ", (g_rconAtBoot >> 9)  & 1U, "CLOCK MONITOR failure");
    reportBit("IOPUWR", (g_rconAtBoot >> 14) & 1U, "ILLEGAL OPCODE or uninit W reg");
    reportBit("TRAPR ", (g_rconAtBoot >> 15) & 1U, "TRAP conflict reset");

    Serial.println("  TRAPR, IOPUWR and CM mean a real fault, not a design choice:");
    Serial.println("  a bad pointer, a division by zero, a jump into blank flash.");
    Serial.println("  Any of those set here deserves investigating before anything");
    Serial.println("  else in this list.");
    Serial.println("  The bits are sticky and nothing in the core clears them, so");
    Serial.println("  this sketch clears what it has read -- otherwise POR would");
    Serial.println("  still be set after every later watchdog reset.");
    Serial.println();
}

/* --- watchdog configuration --------------------------------------------- */

static unsigned long wdtCounts(void)
{
    return 1UL << (WDTCONLbits.RUNDIV & 0x1FU);
}

static void reportWdtConfig(void)
{
    unsigned long counts = wdtCounts();

    Serial.println("--- 2. watchdog configuration, read back ----------------");
    Serial.print  ("  WDTCONL.ON     : ");
    Serial.println(WDTCONLbits.ON ? "1 (running)" : "0 (stopped - FWDTEN = ON_SW)");
    Serial.print  ("  WDTCONL.RUNDIV : ");
    Serial.print(WDTCONLbits.RUNDIV);
    Serial.print  ("  -> postscale 1:");
    Serial.println(counts);
    Serial.print  ("  WDTCONL.CLKSEL : ");
    Serial.println(WDTCONLbits.CLKSEL);
    Serial.print  ("  window mode    : ");
    Serial.println(WDTCONLbits.WDTWINEN
        ? "WINDOWED - feeding too EARLY also resets!" : "off (feed whenever)");

    Serial.println("  so the period is one of:");
    Serial.print  ("    if LPRC (~32 kHz) : ");
    Serial.print(counts / 32UL);
    Serial.println(" ms");
    Serial.print  ("    if FCY (");
    Serial.print(FCY / 1000UL);
    Serial.print  (" kHz)  : ");
    Serial.print(counts / (FCY / 1000000UL));
    Serial.println(" us");
    Serial.println("  RCLKSEL is not set for this device in system_config.c, so");
    Serial.println("  which of those is true has to be measured. Test 3 does.");
    Serial.println();
}

/* --- test 3: arm it and let it bite ------------------------------------- */

static void starveTheDog(void)
{
    unsigned long start;

    Serial.println("--- 3. measuring the real timeout -----------------------");
    Serial.println("  arming the watchdog and then NOT feeding it.");
    Serial.println("  millis() is stored to a persistent variable as fast as");
    Serial.println("  possible; the last value written is the timeout.");
    Serial.println("  The board will reset in a moment. This is intentional.");
    Serial.flush();     /* get all of that out of the UART before we die */

    g_stage = STAGE_STARVING;
    g_heartbeat = 0UL;

    __builtin_clrwdt();             /* start from a known-empty counter */
    WDTCONLbits.ON = 1;             /* arm */

    start = millis();
    for (;;) {
        g_heartbeat = millis() - start;
        if (g_heartbeat > STARVE_LIMIT_MS) {
            /* It never fired. That is a finding, not a hang - report it. */
            WDTCONLbits.ON = 0;
            g_stage = STAGE_IDLE;
            Serial.println("  !! no reset after 30 s. The watchdog did not arm.");
            Serial.println("  !! Check FWDTEN in system_config.c is ON_SW.");
            Serial.println();
            return;
        }
    }
}

static void reportMeasurement(void)
{
    unsigned long counts = wdtCounts();
    unsigned long hz;

    Serial.println("--- 3. result: the watchdog bit -------------------------");
    Serial.print  ("  measured timeout : ");
    Serial.print(g_measuredMs);
    Serial.println(" ms");

    if (g_measuredMs == 0UL) {
        Serial.println("  0 ms means the reset happened before the first store -");
        Serial.println("  the period is well under a millisecond, so the WDT is");
        Serial.println("  clocked from FCY, not LPRC.");
    } else {
        hz = (counts * 1000UL) / g_measuredMs;
        Serial.print  ("  implied WDT clock: ");
        Serial.print(hz);
        Serial.println(" Hz");
        if (hz > 20000UL && hz < 50000UL) {
            Serial.println("  ~32 kHz: this is LPRC, the low-power RC oscillator.");
            Serial.println("  LPRC is a factory-trimmed RC, so expect this to move");
            Serial.println("  by tens of percent over temperature and voltage.");
            Serial.println("  Feed the dog at a quarter of the period, not at 90%.");
        } else {
            Serial.println("  Not ~32 kHz - compare against the two candidates in");
            Serial.println("  test 2 to see which source is feeding it.");
        }
    }
    Serial.println("  Note millis() restarted from 0 on the way back up: a WDT");
    Serial.println("  reset resets peripherals, Timer1 included.");
    Serial.println();
}

/* --- test 4: feeding it works ------------------------------------------- */

static void testFeeding(void)
{
    unsigned long window;
    unsigned long start;
    unsigned long feeds = 0;

    Serial.println("--- 4. __builtin_clrwdt() keeps it quiet ----------------");

    window = (g_measuredMs > 0UL) ? (g_measuredMs * 3UL) : 3000UL;
    if (window > 6000UL) { window = 6000UL; }

    Serial.print  ("  running ");
    Serial.print(window);
    Serial.println(" ms - three times the measured timeout - while feeding:");
    Serial.flush();

    __builtin_clrwdt();
    WDTCONLbits.ON = 1;

    start = millis();
    while ((millis() - start) < window) {
        __builtin_clrwdt();
        feeds++;
        digitalWrite(LED_BUILTIN, ((millis() - start) & 0x80UL) ? LED_ON : LED_OFF);
    }
    WDTCONLbits.ON = 0;
    digitalWrite(LED_BUILTIN, LED_OFF);

    Serial.print  ("  survived. ");
    Serial.print(feeds);
    Serial.println(" calls to __builtin_clrwdt(), no reset.");
    Serial.println("  One CLRWDT instruction per pass of the main loop is the");
    Serial.println("  usual pattern - and put it in exactly ONE place. A clrwdt");
    Serial.println("  sprinkled inside every long function turns the watchdog");
    Serial.println("  into decoration: it can no longer detect the hang it was");
    Serial.println("  put there to catch.");
    Serial.println();
}

/* --- test 5: the delay() trap ------------------------------------------- */

static void testDelayTrap(void)
{
    unsigned long sleepMs = (g_measuredMs > 0UL) ? (g_measuredMs * 2UL) : 2000UL;

    Serial.println("--- 5. the trap: delay() does not feed the dog ----------");
    Serial.print  ("  arming, then calling delay(");
    Serial.print(sleepMs);
    Serial.println(") with no feeding.");
    Serial.println("  delay() is a millis() spin loop and contains no CLRWDT, so");
    Serial.println("  this cannot possibly survive. Expect the second reset now.");
    Serial.flush();

    g_stage = STAGE_IN_DELAY;
    g_heartbeat = sleepMs;

    __builtin_clrwdt();
    WDTCONLbits.ON = 1;
    delay(sleepMs);

    /* Only reached if the watchdog failed to fire. */
    WDTCONLbits.ON = 0;
    g_stage = STAGE_IDLE;
    Serial.println("  !! delay() returned. The watchdog did not fire - the");
    Serial.println("  !! measured timeout must be longer than we thought.");
    Serial.println();
}

static void reportDelayTrap(void)
{
    Serial.println("--- 5. result: delay() was cut short --------------------");
    Serial.print  ("  the delay(");
    Serial.print(g_heartbeat);
    Serial.println(") never returned; the watchdog reset the board.");
    Serial.println("  Fixes, in order of preference:");
    Serial.println("   1. do not block. Use the (long)(millis() - next) >= 0");
    Serial.println("      idiom from NanoMillis and feed once per loop() pass.");
    Serial.println("   2. if you must wait, write your own feeding wait:");
    Serial.println("         while ((long)(millis() - end) < 0) __builtin_clrwdt();");
    Serial.println("   3. choose a period longer than your worst blocking call -");
    Serial.println("      the weakest option, since the watchdog then takes that");
    Serial.println("      long to notice a genuine hang.");
    Serial.println("  The same applies to any spin loop. This library's Wire and");
    Serial.println("  SPI wait loops have no timeouts, so a shorted SDA line is an");
    Serial.println("  infinite loop - one a watchdog turns from a dead board into");
    Serial.println("  a reboot you can log.");
    Serial.println();
}

/* --- main flow ----------------------------------------------------------- */

static void resetStateMachine(void)
{
    g_magic      = MAGIC;
    g_stage      = STAGE_IDLE;
    g_heartbeat  = 0UL;
    g_measuredMs = 0UL;
    g_wdtResets  = 0U;
    g_runs       = 0U;
}

void setup()
{
    unsigned int wdto;

    /* RCON first, before anything else can add to it, and keep a copy. */
    g_rconAtBoot = RCON;
    wdto = RCONbits.WDTO;

    /* Consume the bits we report, so the next boot's RCON means the next boot. */
    RCONbits.POR  = 0;
    RCONbits.BOR  = 0;
    RCONbits.WDTO = 0;
    RCONbits.SWR  = 0;
    RCONbits.EXTR = 0;
    RCONbits.IOPUWR = 0;
    RCONbits.TRAPR  = 0;
    RCONbits.CM     = 0;

    /* The watchdog is off after any reset (FWDTEN = ON_SW). Be explicit anyway. */
    WDTCONLbits.ON = 0;

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LED_OFF);
    pinMode(BUTTON_BUILTIN, INPUT_PULLUP);

    Serial.begin(115200);

    if (g_magic != MAGIC) {
        resetStateMachine();
        Serial.println();
        Serial.println("(persistent block was uninitialised - cold start)");
    }

    if (wdto) {
        g_wdtResets++;
        if (g_stage == STAGE_STARVING) {
            g_measuredMs = g_heartbeat;
        }
    } else if (g_stage == STAGE_STARVING || g_stage == STAGE_IN_DELAY) {
        /* Reset arrived from somewhere else mid-test; the result is not usable. */
        g_stage = STAGE_IDLE;
    }

    Serial.println();
    Serial.println("======================================================");
    Serial.println(" watchdog and reset causes - dsPIC33CK256MC005");
    Serial.println("======================================================");
    Serial.print  ("watchdog resets so far : ");
    Serial.println(g_wdtResets);
    Serial.print  ("full runs completed    : ");
    Serial.println(g_runs);
    Serial.println();
}

void loop()
{
    unsigned int stage = g_stage;

    decodeRcon();

    if (stage == STAGE_STARVING) {
        /* Came back from test 3. Report, then go on to tests 4 and 5. */
        reportMeasurement();
        testFeeding();
        testDelayTrap();
        /* testDelayTrap resets the board; only reached if it failed to. */
    } else if (stage == STAGE_IN_DELAY) {
        /* Came back from test 5: the sequence is complete. */
        reportDelayTrap();
        g_stage = STAGE_FINISHED;
        g_runs++;
    } else if (stage == STAGE_FINISHED) {
        Serial.println("--- all tests done --------------------------------------");
        Serial.print  ("  measured watchdog timeout : ");
        Serial.print(g_measuredMs);
        Serial.println(" ms");
        Serial.print  ("  watchdog resets this power cycle: ");
        Serial.println(g_wdtResets);
        Serial.println("  The watchdog is now DISABLED, so the board is stable.");
        Serial.println("  Press SW0 to run the whole sequence again.");
        Serial.println();

        /* Idle here: heartbeat the LED and watch the button. */
        for (;;) {
            digitalWrite(LED_BUILTIN,
                ((millis() / 500UL) & 1UL) ? LED_ON : LED_OFF);
            if (digitalRead(BUTTON_BUILTIN) == LOW) {
                delay(50);                              /* debounce */
                if (digitalRead(BUTTON_BUILTIN) == LOW) {
                    while (digitalRead(BUTTON_BUILTIN) == LOW) { }
                    Serial.println("SW0 pressed - starting over.");
                    Serial.println();
                    resetStateMachine();
                    return;                             /* loop() runs again */
                }
            }
        }
    } else {
        /* Fresh start: describe the configuration, then begin the sequence. */
        reportWdtConfig();
        starveTheDog();
        /* starveTheDog resets the board; only reached if the WDT never armed. */
    }
}
