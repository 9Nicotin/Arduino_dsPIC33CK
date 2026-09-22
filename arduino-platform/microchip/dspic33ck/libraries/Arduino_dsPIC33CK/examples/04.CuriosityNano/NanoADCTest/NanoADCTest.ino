/*
 * NanoADCTest - exercise every ADC channel on the dsPIC33CK256MC005 Curiosity
 *               Nano (EV08P02A), and measure what analogRead() actually costs
 *
 * NO EXTERNAL WIRING NEEDED to run it. With nothing connected the inputs float
 * and the numbers are meaningless but the sweep still proves every channel
 * converts. To see something real, put a 10k potentiometer between 3V3 and GND
 * with its wiper on A0 (= D0 = RA0), or just touch a jumper from A0 to 3V3 and
 * then to GND.
 *
 * WHAT IT DOES
 *   1. Sweeps all 20 analog inputs and prints raw counts and volts.
 *   2. Repeats the sweep continuously, so a jumper moved from 3V3 to GND shows
 *      up immediately.
 *   3. Measures per-channel noise (min / max / peak-to-peak over N samples).
 *   4. Times analogRead() with micros().
 *
 * WHAT analogRead() DOES HERE, precisely
 *   - The shared ADC core runs at 12-bit (SHRRES = 0b11). The core reads the
 *     12-bit result and returns it SHIFTED RIGHT BY 2, i.e. 0..1023, so sketches
 *     written for AVR scale correctly. The bottom two bits are discarded -- this
 *     board has more resolution than the Arduino API exposes.
 *   - The reference is AVDD (3.3 V on this board). analogReference() stores the
 *     mode and does nothing else: there is no alternate reference wired here, so
 *     EXTERNAL behaves exactly like DEFAULT. Do not rely on it.
 *   - Each call sets ANSELx and TRISx for the pin, triggers one conversion on
 *     the shared core and spins until the channel's ANxRDY flag comes up. It is
 *     blocking, with a bounded spin count as a safety net.
 *   - A pin with no ADC channel returns 0. So does a grounded pin. The two are
 *     indistinguishable from the return value alone -- check
 *     analogPinToChannel(pin) if it matters.
 *
 * THREE PINS THAT NEED CARE ON THIS BOARD
 *   A11 (D13 = RB8) and A12 (D14 = RB9) are the dedicated I2C1 SCL/SDA pins.
 *     Reading them is harmless, but Wire.begin() takes them over afterwards.
 *   A19 (D37 = RD10) is LED0. analogRead() makes it an analog INPUT, which
 *     turns the LED off and leaves it off. This sketch reads it last and then
 *     puts it back to OUTPUT, which is exactly what your own code has to do.
 *   D38 (RD13, SW0) is ANN0 -- an ADC *negative* input only. It is not an
 *     analog input and has no alias here.
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

#define ADC_MAX      1023
#define VREF_VOLTS   3.3
#define NOISE_SAMPLES 64
#define TIMING_LOOPS  200

/* The 20 analog inputs, in alias order. The pin numbers are not contiguous --
 * RB5/RB6 are the debugger's and RB10-RB15 have no ADC channel -- so the table
 * is spelled out rather than generated. */
typedef struct {
    const char *name;
    uint8_t     pin;
    const char *note;
} analog_input_t;

static const analog_input_t g_inputs[] = {
    { "A0 ",  A0,  "RA0  AN0   CMP1A"     },
    { "A1 ",  A1,  "RA1  AN16"            },
    { "A2 ",  A2,  "RA2  AN9"             },
    { "A3 ",  A3,  "RA3  AN3"             },
    { "A4 ",  A4,  "RA4  AN4"             },
    { "A5 ",  A5,  "RB0  AN5   PWM D5"    },
    { "A6 ",  A6,  "RB1  AN6   PWM D6"    },
    { "A7 ",  A7,  "RB2  AN1   PWM D7"    },
    { "A8 ",  A8,  "RB3  AN8   PWM D8"    },
    { "A9 ",  A9,  "RB4  AN17"            },
    { "A10",  A10, "RB7  AN2"             },
    { "A11",  A11, "RB8  AN10  I2C SCL"   },
    { "A12",  A12, "RB9  AN11  I2C SDA"   },
    { "A13",  A13, "RC0  AN12"            },
    { "A14",  A14, "RC1  AN13  CMP1B"     },
    { "A15",  A15, "RC2  AN14"            },
    { "A16",  A16, "RC3  AN15"            },
    { "A17",  A17, "RC6  AN19"            },
    { "A18",  A18, "RC7  AN7"             },
    { "A19",  A19, "RD10 AN18  LED0 (!)"  },
};

#define NUM_INPUTS  (sizeof(g_inputs) / sizeof(g_inputs[0]))

static unsigned long g_sweep = 0;

static void printVolts(int raw)
{
    double volts = (double)raw * VREF_VOLTS / (double)ADC_MAX;
    Serial.print(volts, 3);
    Serial.print("V");
}

/* One pass over every channel, one conversion each. */
static void sweepAll(void)
{
    size_t i;

    Serial.println();
    Serial.print  ("sweep #");
    Serial.print(g_sweep);
    Serial.println("   alias  pin  chan  raw   volts    pin notes");
    Serial.println("           -----  ---  ----  ----  -------  --------------------");

    for (i = 0; i < NUM_INPUTS; i++) {
        uint8_t pin = g_inputs[i].pin;
        int     raw = analogRead(pin);

        Serial.print  ("           ");
        Serial.print(g_inputs[i].name);
        Serial.print  ("   D");
        Serial.print(pin);
        if (pin < 10) Serial.print(" ");
        Serial.print  ("   ");
        Serial.print(analogPinToChannel(pin));
        if (analogPinToChannel(pin) < 10) Serial.print(" ");
        Serial.print  ("   ");
        Serial.print(raw);
        if (raw < 1000) Serial.print(" ");
        if (raw < 100)  Serial.print(" ");
        if (raw < 10)   Serial.print(" ");
        Serial.print  ("  ");
        printVolts(raw);
        Serial.print  ("   ");
        Serial.println(g_inputs[i].note);
    }

    /* A19 is LED0. analogRead() just made it an analog input, so give it back.
     * Leaving this out is a real bug that looks like "the LED stopped working". */
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LED_OFF);

    g_sweep++;
}

/* Noise on one channel: NOISE_SAMPLES back-to-back conversions, no delay. */
static void measureNoise(uint8_t pin, const char *label)
{
    int      lo = ADC_MAX;
    int      hi = 0;
    uint32_t sum = 0;
    uint8_t  i;

    for (i = 0; i < NOISE_SAMPLES; i++) {
        int v = analogRead(pin);
        if (v < lo) lo = v;
        if (v > hi) hi = v;
        sum += (uint32_t)v;
    }

    Serial.print  ("  ");
    Serial.print(label);
    Serial.print  ("  mean=");
    Serial.print((double)sum / (double)NOISE_SAMPLES, 1);
    Serial.print  ("  min=");
    Serial.print(lo);
    Serial.print  ("  max=");
    Serial.print(hi);
    Serial.print  ("  p-p=");
    Serial.print(hi - lo);
    Serial.print  (" LSB (");
    Serial.print((double)(hi - lo) * VREF_VOLTS * 1000.0 / (double)ADC_MAX, 1);
    Serial.println(" mV)");
}

/* Cost of one analogRead(), averaged. micros() has ~1 us granularity here, so
 * TIMING_LOOPS conversions are timed as a block. */
static void measureTiming(uint8_t pin)
{
    unsigned long t0;
    unsigned long t1;
    uint16_t      i;
    int           sink = 0;

    t0 = micros();
    for (i = 0; i < TIMING_LOOPS; i++) {
        sink += analogRead(pin);
    }
    t1 = micros();

    Serial.print  ("  ");
    Serial.print(TIMING_LOOPS);
    Serial.print  (" conversions took ");
    Serial.print(t1 - t0);
    Serial.print  (" us -> ");
    Serial.print((double)(t1 - t0) / (double)TIMING_LOOPS, 2);
    Serial.println(" us each");

    Serial.print  ("  (sum ");
    Serial.print(sink);
    Serial.println(" printed only so the loop cannot be optimised away)");
}

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LED_OFF);

    Serial.begin(115200);

    Serial.println();
    Serial.println("======================================================");
    Serial.println(" ADC test - dsPIC33CK256MC005 Curiosity Nano");
    Serial.println("======================================================");
    Serial.print  ("FCY               : ");
    Serial.print(FCY / 1000UL);
    Serial.println(" kHz");
    Serial.print  ("analog inputs     : ");
    Serial.println(NUM_ANALOG_INPUTS);
    Serial.print  ("table entries     : ");
    Serial.println((unsigned long)NUM_INPUTS);
    Serial.println("core resolution   : 12-bit, returned as 0..1023 (>>2)");
    Serial.print  ("reference         : AVDD = ");
    Serial.print(VREF_VOLTS, 1);
    Serial.println(" V   (analogReference() is a no-op)");
    Serial.print  ("1 LSB             : ");
    Serial.print(VREF_VOLTS * 1000.0 / (double)ADC_MAX, 2);
    Serial.println(" mV");

    Serial.println();
    Serial.println("-- conversion timing ---------------------------------");
    measureTiming(A0);

    Serial.println();
    Serial.println("-- input noise, nothing connected --------------------");
    Serial.println("   A floating pin wanders. Large p-p here is expected;");
    Serial.println("   with a low-impedance source it should be 1-3 LSB.");
    measureNoise(A0, "A0 ");
    measureNoise(A2, "A2 ");
    measureNoise(A13, "A13");

    /* measureNoise() never touches LED0, but sweepAll() does. Restore once here
     * too so the state is the same whichever order a reader adds calls in. */
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LED_OFF);

    Serial.println();
    Serial.println("-- full sweep, repeating every 2 s -------------------");
}

void loop()
{
    digitalWrite(LED_BUILTIN, LED_ON);
    sweepAll();
    digitalWrite(LED_BUILTIN, LED_OFF);
    delay(2000);
}
