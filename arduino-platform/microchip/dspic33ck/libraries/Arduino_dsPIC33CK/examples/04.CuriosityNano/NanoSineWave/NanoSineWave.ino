/*
 * NanoSineWave - generating a real analog voltage on the dsPIC33CK256MC005
 *                Curiosity Nano (EV08P02A) with PWM and one RC filter
 *
 * WHY THIS SKETCH EXISTS
 * This device's DAC has no output buffer -- DACOEN does not exist on
 * dsPIC33CK256MC005, so DAC1 can only feed the analog comparator (see NanoDAC).
 * There is no analog output pin. What there is, is PWM: a pin switching between
 * 0 V and 3.3 V at 490 Hz whose average is anything in between. Put a low-pass
 * filter on it and the average is all that is left. That is a DAC -- slow, and
 * with visible ripple, but a real one, and it costs two passive parts.
 *
 * WIRING - one resistor, one capacitor, and a jumper to read the result back
 *
 *   D5 (RB0) ---[ 10k ]---+--- filtered output
 *                         |
 *                       [ 1uF ]
 *                         |
 *                        GND
 *
 *   filtered output ---> D1 / A1 (RA1)      so the sketch can measure itself
 *
 *   Optional second channel, 90 degrees behind the first:
 *   D6 (RB1) ---[ 10k ]---+--- second output ---[ 1uF ]--- GND
 *
 * Any values with R*C around 10 ms work. 10k + 1uF gives a corner frequency of
 * about 16 Hz: 30 dB down on the 490 Hz carrier, essentially flat at the few-Hz
 * signal. Electrolytic or ceramic, polarity as drawn if electrolytic. Keep R at
 * 1k or above so the pin is not driving a near-short.
 *
 * THE TRADE-OFF, IN ONE PARAGRAPH
 * The carrier is 490 Hz, fixed by the core's analogWrite(). Reconstructing a
 * waveform needs the filter to average over at least a full carrier cycle
 * (2.04 ms), which puts a hard ceiling on the output frequency: with 64 samples
 * per cycle and one carrier period per sample the fastest clean sine is about
 * 7.6 Hz. This sketch uses two carrier periods per sample and gets 3.9 Hz.
 * Wanting audio out of this is wanting a different peripheral; this is the right
 * tool for a control voltage, a bias, a reference, or a slow sweep.
 *
 * WHAT TO LOOK FOR
 *   1. A voltmeter on the filtered output tracking a smooth sine, triangle and
 *      sawtooth. A scope shows the ripple the filter did not remove.
 *   2. The staircase test: the measured DC agrees with the commanded duty to
 *      within a few tens of millivolts across the range, and does NOT at the two
 *      ends, where analogWrite() stops doing PWM entirely.
 *   3. The two channels in quadrature, if you build the second filter.
 *
 * Deliberately non-blocking: the waveform is stepped from loop() on a micros()
 * deadline and nothing calls delay(). A delay() here would show up as a flat spot
 * in the output.
 *
 * Note this sketch never calls round(). On this core round() is a MACRO from
 * Arduino.h that shadows libm's function and returns long -- see NanoRoundMacro.
 * Table building adds 0.5 and casts, which is what the macro does anyway, just
 * visibly.
 *
 * Serial Monitor at 115200.
 *
 * Board: Tools > Board > "Arduino_dsPIC33CK (dsPIC33CK256MC005 Curiosity Nano)"
 */

#include <Arduino.h>
#include <math.h>

#define OUT_PIN         5           /* RB0, PWM */
#define OUT_PIN_Q       6           /* RB1, PWM, quarter-cycle behind */
#define SENSE_PIN       A1          /* RA1 - jumper the filter output here */

#define TABLE_LEN       64U
#define SAMPLE_US       4000UL      /* ~2 carrier periods per sample */

/* Stay inside 1..254. analogWrite(pin, 0) and analogWrite(pin, 255) do not mean
 * 0% and 100%: the core tears the PWM channel down and drives the pin as plain
 * GPIO. Harmless for a DC level, but in the middle of a waveform it drops the
 * carrier for one sample and the filter output kinks. */
#define DUTY_CENTER     128
#define DUTY_AMPLITUDE  120         /* 8 .. 248 */

#define ADC_VREF_MV     3300UL

typedef enum {
    WAVE_SINE = 0,
    WAVE_TRIANGLE,
    WAVE_SAWTOOTH,
    WAVE_COUNT
} wave_t;

static uint8_t  g_table[TABLE_LEN];
static uint8_t  g_index      = 0;
static wave_t   g_wave       = WAVE_SINE;
static unsigned long g_nextSampleUs = 0;
static unsigned long g_nextPrintMs  = 0;
static unsigned long g_nextWaveMs   = 0;

/* --- table building -------------------------------------------------------- */

/* Not using Arduino.h's PI/TWO_PI: those are macros whose type depends on the
 * core, and this arithmetic is worth keeping explicit. double is 64-bit on
 * xc-dsc v4.00 (__DBL_MANT_DIG__ == 53), so precision is not a concern here. */
static const double kTwoPi = 6.28318530717958647692;

static void buildSine(void)
{
    unsigned int i;

    for (i = 0; i < TABLE_LEN; i++) {
        double phase = (kTwoPi * (double)i) / (double)TABLE_LEN;
        double v     = (double)DUTY_CENTER + ((double)DUTY_AMPLITUDE * sin(phase));

        g_table[i] = (uint8_t)(v + 0.5);
    }
}

static void buildTriangle(void)
{
    unsigned int i;
    unsigned int half = TABLE_LEN / 2U;

    for (i = 0; i < TABLE_LEN; i++) {
        unsigned int up = (i < half) ? i : (TABLE_LEN - 1U - i);

        /* Integer maths on purpose: a triangle is exactly representable and the
         * multiply-then-divide order keeps it that way. */
        g_table[i] = (uint8_t)((DUTY_CENTER - DUTY_AMPLITUDE) +
                               (int)((2UL * DUTY_AMPLITUDE * up) / (half - 1U)));
    }
}

static void buildSawtooth(void)
{
    unsigned int i;

    for (i = 0; i < TABLE_LEN; i++) {
        g_table[i] = (uint8_t)((DUTY_CENTER - DUTY_AMPLITUDE) +
                               (int)((2UL * DUTY_AMPLITUDE * i) / (TABLE_LEN - 1U)));
    }
}

static void buildTable(wave_t w)
{
    switch (w) {
    case WAVE_TRIANGLE: buildTriangle(); break;
    case WAVE_SAWTOOTH: buildSawtooth(); break;
    case WAVE_SINE:
    default:            buildSine();     break;
    }
}

static const char *waveName(wave_t w)
{
    switch (w) {
    case WAVE_TRIANGLE: return "triangle";
    case WAVE_SAWTOOTH: return "sawtooth";
    case WAVE_SINE:     return "sine";
    default:            return "?";
    }
}

/* --- printing -------------------------------------------------------------- */

static unsigned long adcRawToMillivolts(int raw)
{
    return ((unsigned long)raw * ADC_VREF_MV) / 1024UL;
}

static unsigned long dutyToMillivolts(uint8_t duty)
{
    /* The ideal average of a 0/3.3 V square wave at this duty. The real output
     * sits slightly below it: the pin's high level is a little under AVDD and the
     * filter draws a trickle of current through R. */
    return ((unsigned long)duty * ADC_VREF_MV) / 255UL;
}

static void printMillivolts(unsigned long mv)
{
    Serial.print(mv / 1000UL);
    Serial.print('.');

    if ((mv % 1000UL) < 100UL) { Serial.print('0'); }
    if ((mv % 1000UL) < 10UL)  { Serial.print('0'); }

    Serial.print(mv % 1000UL);
}

/* --- the staircase test, run once at startup ------------------------------- */

/*
 * Hold a fixed duty, wait several filter time constants, and measure. This is
 * the part that proves the pseudo-DAC is linear and tells you what its real
 * output range is.
 *
 * 0 and 255 are included on purpose. They are the two values analogWrite() does
 * not treat as PWM -- the channel is released and the pin is driven LOW or HIGH
 * as GPIO -- so they bracket the range rather than extending it, and they are
 * the only two duties that reach the rails.
 */
static void staircaseTest(void)
{
    static const uint8_t duties[] = { 0, 1, 8, 32, 64, 96, 128, 160, 192, 224,
                                      248, 254, 255 };
    size_t i;

    Serial.println("--- staircase: commanded duty vs measured DC -------------");
    Serial.println("  duty   ideal      measured   error   note");

    for (i = 0; i < (sizeof(duties) / sizeof(duties[0])); i++) {
        unsigned long ideal;
        unsigned long measured;
        long          error;
        int           raw;

        analogWrite(OUT_PIN, duties[i]);

        /* 10k * 1uF = 10 ms; 200 ms is 20 time constants, settled to well under
         * one ADC count. Then average 16 reads to knock down the ripple. */
        delay(200);

        {
            unsigned long sum = 0;
            uint8_t       n;

            for (n = 0; n < 16U; n++) {
                sum += (unsigned long)analogRead(SENSE_PIN);
                delayMicroseconds(200);
            }

            raw = (int)(sum / 16UL);
        }

        ideal    = dutyToMillivolts(duties[i]);
        measured = adcRawToMillivolts(raw);
        error    = (long)measured - (long)ideal;

        Serial.print("   ");
        Serial.print(duties[i]);
        Serial.print("\t ");
        printMillivolts(ideal);
        Serial.print(" V\t  ");
        printMillivolts(measured);
        Serial.print(" V\t");
        Serial.print(error);
        Serial.print(" mV\t");

        if (duties[i] == 0) {
            Serial.println("GPIO LOW, not PWM");
        } else if (duties[i] == 255) {
            Serial.println("GPIO HIGH, not PWM");
        } else {
            Serial.println("");
        }
    }

    Serial.println();
    Serial.println("  If every measurement reads near 0 V, the jumper from the");
    Serial.println("  filter output to D1/A1 is missing. If they all read the");
    Serial.println("  same, the capacitor is not connected.");
    Serial.println();
}

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);

    Serial.begin(115200);

    Serial.println();
    Serial.println("======================================================");
    Serial.println(" PWM + RC pseudo-DAC - dsPIC33CK256MC005 Curiosity Nano");
    Serial.println("======================================================");
    Serial.println("This device has no analog output pin (DAC1 has no output");
    Serial.println("buffer). This is the substitute: PWM through an RC filter.");
    Serial.println();
    Serial.print  ("output   : D");
    Serial.print(OUT_PIN);
    Serial.println(" (RB0) -> 10k -> 1uF -> GND");
    Serial.print  ("quadrature: D");
    Serial.print(OUT_PIN_Q);
    Serial.println(" (RB1), same filter, optional");
    Serial.print  ("sense    : D1 / A1 (RA1), jumpered to the filter output");
    Serial.println();
    Serial.print  ("carrier  : 490 Hz    samples/cycle: ");
    Serial.println(TABLE_LEN);
    Serial.print  ("sample   : ");
    Serial.print(SAMPLE_US);
    Serial.print  (" us  ->  output ");
    /* 1e6 / (SAMPLE_US * TABLE_LEN), in millihertz so it prints exactly. */
    Serial.print(1000000000UL / (SAMPLE_US * TABLE_LEN));
    Serial.println(" mHz");
    Serial.println();

    staircaseTest();

    buildTable(g_wave);

    Serial.print("now generating: ");
    Serial.println(waveName(g_wave));
    Serial.println();

    g_nextSampleUs = micros();
    g_nextPrintMs  = millis();
    g_nextWaveMs   = millis() + 10000UL;
}

void loop()
{
    unsigned long now;

    /* --- step the waveform ------------------------------------------------- */
    now = micros();

    /* Subtract-and-compare, never "now >= deadline": micros() wraps every ~71
     * minutes and the subtraction is correct across the wrap while the direct
     * comparison stalls the waveform for the next 71 minutes. */
    if ((long)(now - g_nextSampleUs) >= 0L) {
        uint8_t q;

        g_nextSampleUs += SAMPLE_US;

        analogWrite(OUT_PIN, g_table[g_index]);

        /* Quarter of a cycle behind: cosine to the main channel's sine. */
        q = (uint8_t)((g_index + (TABLE_LEN / 4U)) % TABLE_LEN);
        analogWrite(OUT_PIN_Q, g_table[q]);

        g_index = (uint8_t)((g_index + 1U) % TABLE_LEN);

        /* LED0 marks the start of each cycle, so the output frequency is
         * visible without any instrument. */
        if (g_index == 0U) {
            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        }
    }

    /* --- report ------------------------------------------------------------
     * Once a second, and kept short on purpose. Serial.print() blocks until the
     * bytes are out: at 115200 a 60-character line costs about 5 ms, which is
     * longer than a sample. The generator recovers -- the deadline is advanced by
     * SAMPLE_US rather than set from now, so the missed samples are emitted
     * back-to-back and the phase is preserved -- but the filter output shows a
     * small step each time. A long chatty line here would be visible on a scope.
     */
    if ((long)(millis() - g_nextPrintMs) >= 0L) {
        int     raw  = analogRead(SENSE_PIN);
        uint8_t duty = g_table[g_index];

        g_nextPrintMs += 1000UL;

        Serial.print(waveName(g_wave));
        Serial.print(" duty ");
        Serial.print(duty);
        Serial.print(" cmd ");
        printMillivolts(dutyToMillivolts(duty));
        Serial.print(" meas ");
        printMillivolts(adcRawToMillivolts(raw));
        Serial.println(" V");
    }

    /* --- next waveform ---------------------------------------------------- */
    if ((long)(millis() - g_nextWaveMs) >= 0L) {
        g_nextWaveMs += 10000UL;

        g_wave = (wave_t)((g_wave + 1) % WAVE_COUNT);
        buildTable(g_wave);

        Serial.println();
        Serial.print("switching to: ");
        Serial.println(waveName(g_wave));
        Serial.println();
    }
}
