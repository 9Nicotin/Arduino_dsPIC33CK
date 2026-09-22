/*
 * NanoDAC - the DAC on the dsPIC33CK256MC005 Curiosity Nano (EV08P02A):
 *           what it is, what it is NOT, and how to use it as a converter
 *
 * READ THIS FIRST - THE DAC ON THIS DEVICE HAS NO OUTPUT PIN
 *
 * The dsPIC33CK256MC005 has one DAC, DAC1, 12 bits, referenced to AVDD. It is
 * the reference input of analog comparator 1 -- and that is ALL it is. There is
 * no output buffer and no way to put its voltage on a pin:
 *
 *   - DACOEN, the output-enable bit, appears ZERO times in p33CK256MC005.h and
 *     zero times in the device's EDC description. On the MP-series parts (e.g.
 *     p33CK256MP508.h) the same bit appears fifteen times.
 *   - The pin table does list DACOUT1 on RA3, inherited from the family, but
 *     with no enable bit behind it that label is decoration.
 *
 * So if you came here looking for analogWriteDAC() or a spare analog output:
 * this part does not have one, and the core deliberately exposes no DAC API,
 * because an API whose output goes nowhere is worse than no API. To generate an
 * analog voltage on this board, low-pass-filter a PWM pin -- see NanoSineWave.
 *
 * WHAT THIS SKETCH DOES INSTEAD
 * Uses the DAC for its real purpose, and gets something useful out of it: DAC1
 * sets a threshold, comparator 1 says whether the input is above it, and a
 * binary search over the threshold converts the input voltage to a number. That
 * is a successive-approximation converter, hand-built out of the two pieces a
 * SAR ADC is made of. Twelve comparisons resolve twelve bits.
 *
 * Then it cross-checks the answer against analogRead() on the same pin. Two
 * completely separate peripherals measuring one voltage should agree; when they
 * do, you know both the DAC and the comparator are configured correctly.
 *
 * WIRING - you must supply a DC voltage to measure
 *   D0 / A0 (RA0, pin 8) is comparator input CMP1A. Connect ONE of:
 *     - the wiper of a 10k potentiometer, ends to 3V3 and GND  (best - sweepable)
 *     - a two-resistor divider between 3V3 and GND             (fixed midpoint)
 *     - a jumper to 3V3 or to GND                              (proves the rails)
 *   Leave it floating and you get a meaningless drifting number, which is a
 *   correct result for a floating input.
 *
 *   ABSOLUTE MAXIMUM: 0 V to 3.3 V. There is no protection here beyond the pin's
 *   own clamp diodes.
 *
 * WHAT TO LOOK FOR
 *   1. The DAC/comparator answer and analogRead() track each other to within a
 *      few tens of millivolts as you turn the pot.
 *   2. The measured hysteresis gap grows as HYSSEL is stepped 0..3.
 *   3. Inverting CMPPOL inverts CMPSTAT without touching anything else.
 *
 * REGISTER-LEVEL CODE AHEAD, deliberately. There is no core API for this
 * peripheral, so the sketch talks to the SFRs directly. Register and bit names
 * are from p33CK256MC005.h; the semantics are from the "High-Speed Analog
 * Comparator with Slope Compensation DAC" family reference manual. Where a bit's
 * exact value is a convention rather than something the header states, the
 * sketch MEASURES the effect rather than trusting the comment -- see
 * measureHysteresis().
 *
 * Serial Monitor at 115200.
 *
 * Board: Tools > Board > "Arduino_dsPIC33CK (dsPIC33CK256MC005 Curiosity Nano)"
 */

#include <Arduino.h>

/* 12-bit DAC, full-scale = AVDD = 3.3 V nominal.
 *
 * The search covers all 4096 codes. Be aware that the DAC cannot actually drive
 * to either rail -- the datasheet's guaranteed output range stops short of both
 * ends -- so a result within a few percent of 0 or 4095 means "at or past the
 * end of the usable range", not a precise voltage. */
#define DAC_CODES       4096UL
#define DAC_MAX_CODE    4095U
#define DAC_VREF_MV     3300UL

/* Time for the DAC output to settle and the comparator to respond before
 * CMPSTAT is believable. Ten microseconds is generous for both. */
#define DAC_SETTLE_US   10U

#define CMP_INPUT_PIN   A0          /* RA0, CMP1A */

/* INSEL<2:0> selects which of the four comparator inputs is compared against
 * the DAC. On the 48-pin MC005: CMP1A = RA0 (D0/A0), CMP1B = RC1 (D22/A14),
 * CMP1C = RA3 (D3/A3), CMP1D = RB2 (D7/A7). */
#define INSEL_CMP1A     0
#define INSEL_CMP1B     1
#define INSEL_CMP1C     2
#define INSEL_CMP1D     3

/* --- DAC and comparator setup --------------------------------------------- */

static void dacSetCode(uint16_t code)
{
    if (code > DAC_MAX_CODE) {
        code = DAC_MAX_CODE;
    }

    /* DAC1DATH holds the threshold used in normal (non-slope) mode. DAC1DATL is
     * the low limit for the slope generator and is irrelevant here. */
    DAC1DATH = code;
    delayMicroseconds(DAC_SETTLE_US);
}

/*
 * CMPSTAT with CMPPOL = 0 reads 1 when the selected input is ABOVE the DAC
 * threshold. Read it through the bitfield so the compiler emits a single bit
 * test rather than loading and masking the whole word.
 */
static uint8_t inputAboveThreshold(void)
{
    return DAC1CONLbits.CMPSTAT ? 1U : 0U;
}

static void comparatorBegin(void)
{
    /* 1. Power the module up. Peripheral Module Disable holds CMP1 in reset out
     *    of POR on this family; writes to its registers are ignored until this
     *    bit is cleared, which is a memorably silent way to waste an evening. */
    PMD7bits.CMP1MD = 0;

    /* 2. RA0 as an analog input. An analog comparator input needs ANSEL set (so
     *    the digital input buffer is off and cannot be damaged by a mid-rail
     *    voltage) and TRIS set (so nothing drives the pad). */
    ANSELAbits.ANSELA0 = 1;
    TRISAbits.TRISA0   = 1;

    /* 3. DAC module clock. DACCLK feeds the module's transition-mode and
     *    steady-state timers. This sketch uses neither -- it writes DAC1DATH and
     *    reads CMPSTAT -- so nothing here depends on DACCLK's rate, and FCLKDIV
     *    is set to its maximum divide. That keeps the module comfortably in spec
     *    at both clock settings the board menu offers (FCY 4 MHz internal and
     *    FCY 100 MHz PLL), and dividing too far is the harmless direction. */
    DACCTRL1L = 0;
    DACCTRL1Lbits.CLKSEL   = 1;
    DACCTRL1Lbits.FCLKDIV  = 7;

    /* 4. The comparator itself, and where it takes its input from.
     *    HYSSEL = 0 -> no hysteresis, which is what a converter wants: any
     *    hysteresis would show up directly as a measurement error. */
    DAC1CONL = 0;
    DAC1CONLbits.INSEL  = INSEL_CMP1A;
    DAC1CONLbits.CMPPOL = 0;
    DAC1CONLbits.HYSSEL = 0;
    DAC1CONLbits.HYSPOL = 0;
    DAC1CONLbits.IRQM   = 0;        /* no interrupts - see NanoComparator */
    DAC1CONLbits.DACEN  = 1;

    /* 5. Common enable, last. Everything above is configuration; this is the
     *    switch. */
    DACCTRL1Lbits.DACON = 1;

    dacSetCode(DAC_MAX_CODE / 2U);
}

/* --- the successive-approximation search ---------------------------------- */

/* Return values distinguishing "measured" from "off the end of the scale". */
#define SAR_BELOW_RANGE  0xFFFEU
#define SAR_ABOVE_RANGE  0xFFFFU

/*
 * Twelve iterations, one per bit, each halving the interval. The invariant is
 * "the input is above lo and not above hi", so the loop ends with hi - lo == 1
 * and lo is the largest threshold the input still exceeds.
 *
 * Endpoints are tested first, because a binary search cannot report "outside the
 * interval" -- it would silently converge on the nearest end and look confident.
 */
static uint16_t sarConvert(uint16_t *comparisons)
{
    uint16_t lo = 0;
    uint16_t hi = DAC_MAX_CODE;
    uint16_t n  = 0;

    dacSetCode(0);
    n++;
    if (!inputAboveThreshold()) {
        if (comparisons) { *comparisons = n; }
        return SAR_BELOW_RANGE;
    }

    dacSetCode(DAC_MAX_CODE);
    n++;
    if (inputAboveThreshold()) {
        if (comparisons) { *comparisons = n; }
        return SAR_ABOVE_RANGE;
    }

    while ((uint16_t)(hi - lo) > 1U) {
        uint16_t mid = (uint16_t)(lo + ((hi - lo) / 2U));

        dacSetCode(mid);
        n++;

        if (inputAboveThreshold()) {
            lo = mid;
        } else {
            hi = mid;
        }
    }

    if (comparisons) { *comparisons = n; }
    return lo;
}

/* --- printing helpers ----------------------------------------------------- */

static unsigned long dacCodeToMillivolts(uint16_t code)
{
    return ((unsigned long)code * DAC_VREF_MV) / DAC_CODES;
}

static unsigned long adcRawToMillivolts(int raw)
{
    /* analogRead() returns 0..1023 (the 12-bit result shifted down by two). */
    return ((unsigned long)raw * DAC_VREF_MV) / 1024UL;
}

static void printMillivolts(unsigned long mv)
{
    Serial.print(mv / 1000UL);
    Serial.print('.');

    if ((mv % 1000UL) < 100UL) { Serial.print('0'); }
    if ((mv % 1000UL) < 10UL)  { Serial.print('0'); }

    Serial.print(mv % 1000UL);
    Serial.print(" V");
}

/* --- test 1: does the comparator respond at all? -------------------------- */

/*
 * Worth doing before trusting any measurement, and it works no matter what the
 * input voltage is -- even with the pin at a rail, where the threshold sweep
 * tells you nothing.
 *
 * CMPPOL inverts the comparator output. Flip it, and CMPSTAT must flip too. If
 * it does not, the module is not running: PMD still set, DACON still clear, or
 * the writes went nowhere.
 */
static void testPolarity(void)
{
    uint8_t before;
    uint8_t after;

    Serial.println("--- 1. comparator alive? (CMPPOL inversion test) ---------");

    dacSetCode(DAC_MAX_CODE / 2U);
    before = inputAboveThreshold();

    DAC1CONLbits.CMPPOL = 1;
    delayMicroseconds(DAC_SETTLE_US);
    after = inputAboveThreshold();

    DAC1CONLbits.CMPPOL = 0;
    delayMicroseconds(DAC_SETTLE_US);

    Serial.print  ("  CMPSTAT with CMPPOL=0: ");
    Serial.print(before);
    Serial.print  ("   with CMPPOL=1: ");
    Serial.println(after);

    if (before != after) {
        Serial.println("  OK - the comparator output is live.");
    } else {
        Serial.println("  FAIL - CMPSTAT did not invert. The module is not");
        Serial.println("  running: check PMD7.CMP1MD, DACCTRL1L.DACON and");
        Serial.println("  DAC1CONL.DACEN. Every measurement below is garbage.");
    }

    Serial.println();
}

/* --- test 2: the conversion, cross-checked against the ADC ---------------- */

static void testConvert(void)
{
    uint16_t comparisons = 0;
    uint16_t code;
    int      adcRaw;

    Serial.println("--- 2. SAR conversion vs analogRead() --------------------");

    code   = sarConvert(&comparisons);
    adcRaw = analogRead(CMP_INPUT_PIN);

    if (code == SAR_BELOW_RANGE) {
        Serial.print  ("  input is below the lowest DAC code after ");
        Serial.print(comparisons);
        Serial.println(" comparison(s)");
        Serial.println("  (pin at or near GND, or below the DAC's output floor)");
    } else if (code == SAR_ABOVE_RANGE) {
        Serial.print  ("  input is above the highest DAC code after ");
        Serial.print(comparisons);
        Serial.println(" comparison(s)");
        Serial.println("  (pin at or near 3V3, or above the DAC's output ceiling)");
    } else {
        unsigned long dacMv = dacCodeToMillivolts(code);

        Serial.print  ("  DAC+CMP : code ");
        Serial.print(code);
        Serial.print  (" / 4095 = ");
        printMillivolts(dacMv);
        Serial.print  ("   (");
        Serial.print(comparisons);
        Serial.println(" comparisons)");

        Serial.print  ("  analogRead: raw ");
        Serial.print(adcRaw);
        Serial.print  (" / 1023 = ");
        printMillivolts(adcRawToMillivolts(adcRaw));
        Serial.println();

        {
            unsigned long adcMv = adcRawToMillivolts(adcRaw);
            unsigned long diff  = (dacMv > adcMv) ? (dacMv - adcMv)
                                                  : (adcMv - dacMv);

            Serial.print  ("  difference: ");
            Serial.print(diff);
            Serial.print  (" mV - ");
            Serial.println((diff < 60UL) ? "they agree" : "look into this");
        }
    }

    Serial.println();
}

/* --- test 3: hysteresis, measured rather than assumed --------------------- */

/*
 * Hysteresis means the comparator switches at one threshold going up and a
 * different one coming down. With a fixed input and a SWEEPING threshold the gap
 * between the two crossing codes IS the hysteresis, in DAC codes, which converts
 * straight to millivolts.
 *
 * HYSSEL<1:0> selects the amount; the reference manual gives the four settings
 * as roughly 0, 15, 30 and 45 mV. This function does not take that on trust --
 * it steps through all four and prints what it actually measures. If your
 * numbers differ from the manual's, believe your numbers.
 *
 * A coarse step keeps this fast: 4096 settling delays per sweep would take a
 * noticeable fraction of a second.
 */
#define HYST_STEP   8U

static uint16_t sweepUpFindCrossing(void)
{
    uint32_t code;

    for (code = 0; code <= DAC_MAX_CODE; code += HYST_STEP) {
        dacSetCode((uint16_t)code);
        if (!inputAboveThreshold()) {
            return (uint16_t)code;
        }
    }

    return SAR_ABOVE_RANGE;
}

static uint16_t sweepDownFindCrossing(void)
{
    int32_t code;

    for (code = DAC_MAX_CODE; code >= 0; code -= HYST_STEP) {
        dacSetCode((uint16_t)code);
        if (inputAboveThreshold()) {
            return (uint16_t)code;
        }
    }

    return SAR_BELOW_RANGE;
}

static void measureHysteresis(void)
{
    uint8_t sel;

    Serial.println("--- 3. hysteresis, measured for HYSSEL 0..3 --------------");
    Serial.println("  HYSSEL  up-crossing  down-crossing  gap(codes)  gap(mV)");

    for (sel = 0; sel < 4U; sel++) {
        uint16_t up;
        uint16_t down;

        DAC1CONLbits.HYSSEL = sel;
        delayMicroseconds(DAC_SETTLE_US);

        up   = sweepUpFindCrossing();
        down = sweepDownFindCrossing();

        Serial.print("     ");
        Serial.print(sel);
        Serial.print("    ");

        if (up >= SAR_BELOW_RANGE || down >= SAR_BELOW_RANGE) {
            Serial.println("    input is at a rail - no crossing to find");
            continue;
        }

        {
            uint16_t gap = (up > down) ? (uint16_t)(up - down)
                                       : (uint16_t)(down - up);

            Serial.print("     ");
            Serial.print(up);
            Serial.print("          ");
            Serial.print(down);
            Serial.print("            ");
            Serial.print(gap);
            Serial.print("          ");
            Serial.println(dacCodeToMillivolts(gap));
        }
    }

    DAC1CONLbits.HYSSEL = 0;        /* back to no hysteresis for conversions */
    delayMicroseconds(DAC_SETTLE_US);

    Serial.println("  (gap at HYSSEL=0 is the sweep's own 8-code resolution,");
    Serial.println("   about 6 mV, plus input noise - not hysteresis)");
    Serial.println();
}

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);

    Serial.begin(115200);

    comparatorBegin();

    Serial.println();
    Serial.println("======================================================");
    Serial.println(" DAC1 + CMP1 - dsPIC33CK256MC005 Curiosity Nano");
    Serial.println("======================================================");
    Serial.println("This device's DAC has NO output pin (no DACOEN bit). It");
    Serial.println("is the comparator's reference only. For an analog OUTPUT,");
    Serial.println("see NanoSineWave (PWM + RC filter).");
    Serial.println();
    Serial.print  ("input   : D0 / A0 (RA0, CMP1A)   resolution ");
    Serial.print(DAC_VREF_MV * 1000UL / DAC_CODES);
    Serial.println(" uV per code");
    Serial.println("supply a DC voltage there: pot wiper, divider, or 3V3/GND");
    Serial.println();

    testPolarity();
    measureHysteresis();
}

void loop()
{
    /* Heartbeat, so a stalled sketch is obvious without the monitor. LED0 is
     * active low; XOR-ing the latch is the cheapest toggle. */
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

    testConvert();
    delay(1000);
}
