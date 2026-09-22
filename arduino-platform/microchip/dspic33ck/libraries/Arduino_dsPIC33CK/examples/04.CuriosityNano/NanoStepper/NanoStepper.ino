/*
 * NanoStepper - driving a 28BYJ-48 unipolar stepper from the dsPIC33CK256MC005
 *               Curiosity Nano (EV08P02A) through a ULN2003 board
 *
 * This platform ships no Stepper library, so the sketch is the driver: four
 * coils, a phase table, and a non-blocking scheduler. That is all a unipolar
 * stepper driver is, and writing it out is more useful than hiding it.
 *
 * WIRING - you need a motor and a driver board. Do NOT drive coils from the MCU.
 *
 *   Nano D21 (RC0) ---- IN1  on the ULN2003 board
 *   Nano D22 (RC1) ---- IN2
 *   Nano D23 (RC2) ---- IN3
 *   Nano D24 (RC3) ---- IN4
 *   Nano GND       ---- ULN2003 GND        <-- REQUIRED, and easy to forget
 *   5 V supply +   ---- ULN2003 5-12V in
 *   5 V supply GND ---- ULN2003 GND (same node as Nano GND)
 *   motor          ---- the 5-pin connector on the board
 *
 * POWER: the 28BYJ-48 draws around 250 mA when stepping and rather more when
 * stalled. That must come from its own 5 V supply, not from the Curiosity Nano --
 * the board's 5 V rail comes through the debugger's USB and is not there to run
 * motors. Grounds must be common or the logic inputs have no reference and the
 * motor will behave erratically or not at all.
 *
 * The ULN2003 is an inverting Darlington array with the coils on its outputs, so
 * a HIGH on INn energises coil n. The board's four LEDs show the phase pattern,
 * which makes the three step modes below visible even with no motor attached --
 * a reasonable way to check the sketch before wiring anything up.
 *
 * PIN CHOICE: RC0-RC3 are four consecutive free pins that collide with nothing.
 * They avoid SPI (D25/D26/D33/D34), I2C (D13/D14), the CDC UART (D31/D32), the
 * four PWM pins (D5-D8) and the debugger's RB5/RB6. They are analog-capable
 * (A13-A16), which costs nothing: pinMode(OUTPUT) turns the analog function off.
 *
 * THE THREE STEP MODES
 *   wave  (1 phase on)   4 steps/cycle, least torque, least current
 *   full  (2 phases on)  4 steps/cycle, about 1.4x the torque of wave
 *   half  (alternating)  8 steps/cycle, twice the resolution, uneven torque
 * Half-stepping needs twice as many steps for the same rotation, which the
 * sketch accounts for rather than leaving to you.
 *
 * GEAR RATIO, AND WHY YOUR REVOLUTION IS NOT QUITE A REVOLUTION
 * The motor is 32 full steps per internal revolution behind a gearbox usually
 * quoted as 64:1, giving 2048 full steps at the output shaft. The real ratio is
 * closer to 63.68:1 -- about 2037.9 steps -- so commanding 2048 overshoots by
 * roughly 2 degrees per revolution. Fine for a demo, not fine for positioning;
 * if it matters, measure your own motor and change STEPS_PER_REV_FULL.
 *
 * WHAT TO LOOK FOR
 *   1. The shaft turning a quarter, half and full revolution, each way.
 *   2. Acceleration: the motor starts slowly and speeds up. Comment out the ramp
 *      (set RAMP_STEPS to 0) and a fast target speed will make it buzz and sit
 *      still -- a stepper cannot start at speed.
 *   3. Press SW0 mid-move: the motor reverses and returns the way it came.
 *   4. LED0 toggles once per commanded revolution.
 *   5. Between moves the coils are released. The motor loses holding torque and
 *      goes quiet, and the ULN2003 stops dissipating. Holding a stepper still is
 *      its hottest state, not its coolest.
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

#define COIL_1_PIN      21          /* RC0 -> IN1 */
#define COIL_2_PIN      22          /* RC1 -> IN2 */
#define COIL_3_PIN      23          /* RC2 -> IN3 */
#define COIL_4_PIN      24          /* RC3 -> IN4 */

#define STEPS_PER_REV_FULL  2048L   /* 32 steps * 64:1, nominal */

/* Timing. A 28BYJ-48 on 5 V will not follow much faster than this, and pushing
 * past its pull-out torque makes it stall silently -- the pattern keeps advancing
 * and the shaft does not, so the position counter lies. */
#define STEP_US_START   9000UL      /* slow enough to start from standstill */
#define STEP_US_MIN     1800UL      /* ~555 steps/s, near this motor's limit */
#define RAMP_STEPS      200L        /* steps spent accelerating and decelerating */

#define DEBOUNCE_MS     50UL

typedef enum {
    MODE_WAVE = 0,
    MODE_FULL,
    MODE_HALF
} step_mode_t;

/* Phase tables as 4-bit masks, bit 0 = IN1 ... bit 3 = IN4.
 *
 * Wave and full both have four entries; half interleaves them, which is exactly
 * what half-stepping is -- alternate between one coil on and two coils on, and
 * the rotor settles between the full-step positions. */
static const uint8_t g_waveSeq[4] = { 0x01, 0x02, 0x04, 0x08 };
static const uint8_t g_fullSeq[4] = { 0x03, 0x06, 0x0C, 0x09 };
static const uint8_t g_halfSeq[8] = { 0x01, 0x03, 0x02, 0x06,
                                      0x04, 0x0C, 0x08, 0x09 };

static const uint8_t g_coilPins[4] = { COIL_1_PIN, COIL_2_PIN,
                                       COIL_3_PIN, COIL_4_PIN };

/* --- motion state ---------------------------------------------------------- */

static step_mode_t   g_mode        = MODE_FULL;
static uint8_t       g_phase       = 0;      /* index into the active sequence */
static long          g_position    = 0;      /* steps, signed, since reset */
static long          g_target      = 0;      /* where we are heading */
static long          g_moveStart   = 0;      /* g_position when the move began */
static unsigned long g_nextStepUs  = 0;
static unsigned long g_lastBtnMs   = 0;
static long          g_revCounter  = 0;      /* steps since the last LED toggle */

/* --- coils ----------------------------------------------------------------- */

static uint8_t sequenceLength(step_mode_t m)
{
    return (m == MODE_HALF) ? 8U : 4U;
}

static uint8_t sequenceValue(step_mode_t m, uint8_t phase)
{
    switch (m) {
    case MODE_WAVE: return g_waveSeq[phase & 3U];
    case MODE_HALF: return g_halfSeq[phase & 7U];
    case MODE_FULL:
    default:        return g_fullSeq[phase & 3U];
    }
}

/* Steps needed for one output revolution in the current mode. */
static long stepsPerRev(step_mode_t m)
{
    return (m == MODE_HALF) ? (STEPS_PER_REV_FULL * 2L) : STEPS_PER_REV_FULL;
}

static void writeCoils(uint8_t mask)
{
    uint8_t i;

    for (i = 0; i < 4U; i++) {
        digitalWrite(g_coilPins[i], (mask & (uint8_t)(1U << i)) ? HIGH : LOW);
    }
}

/*
 * All coils off. Call this whenever the motor does not need to hold position:
 * an energised stepper at standstill dissipates its full rated power, all of it
 * as heat, and provides nothing you are using.
 */
static void releaseCoils(void)
{
    writeCoils(0x00);
}

/* --- one step -------------------------------------------------------------- */

static void stepOnce(int8_t dir)
{
    uint8_t len = sequenceLength(g_mode);

    if (dir >= 0) {
        g_phase = (uint8_t)((g_phase + 1U) % len);
        g_position++;
    } else {
        g_phase = (uint8_t)((g_phase + len - 1U) % len);
        g_position--;
    }

    writeCoils(sequenceValue(g_mode, g_phase));

    /* One LED toggle per commanded revolution, in whichever direction. */
    g_revCounter++;
    if (g_revCounter >= stepsPerRev(g_mode)) {
        g_revCounter = 0;
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }
}

/* --- the speed ramp -------------------------------------------------------- */

/*
 * Trapezoidal: accelerate for RAMP_STEPS, run flat, decelerate for the last
 * RAMP_STEPS. The interval is interpolated linearly, which is not the
 * mathematically ideal profile (constant acceleration wants 1/sqrt(n)) but is
 * close enough for a geared motor and is obvious to read.
 *
 * Whichever of "steps taken" and "steps remaining" is smaller decides, so a move
 * shorter than 2*RAMP_STEPS simply never reaches full speed instead of
 * overrunning its deceleration.
 */
static unsigned long currentStepInterval(void)
{
    long done      = g_position - g_moveStart;
    long remaining = g_target - g_position;
    long n;

    if (done < 0)      { done = -done; }
    if (remaining < 0) { remaining = -remaining; }

    n = (done < remaining) ? done : remaining;

    if (RAMP_STEPS <= 0L || n >= RAMP_STEPS) {
        return STEP_US_MIN;
    }

    return STEP_US_START -
           (((STEP_US_START - STEP_US_MIN) * (unsigned long)n) /
            (unsigned long)RAMP_STEPS);
}

/* --- moves ----------------------------------------------------------------- */

static void beginMove(long steps)
{
    g_moveStart  = g_position;
    g_target     = g_position + steps;
    g_nextStepUs = micros();
}

static uint8_t moveInProgress(void)
{
    return (g_position != g_target) ? 1U : 0U;
}

/* SW0 turns the move around: the remaining distance is mirrored about the
 * current position, so the motor retraces its path and the ramp restarts from
 * here. */
static void reverseMove(void)
{
    long remaining = g_target - g_position;

    g_target    = g_position - remaining;
    g_moveStart = g_position;
}

static void serviceMotor(void)
{
    unsigned long now;

    if (!moveInProgress()) {
        return;
    }

    now = micros();

    /* Subtract-and-compare so the ~71-minute micros() wrap is a non-event. */
    if ((long)(now - g_nextStepUs) < 0L) {
        return;
    }

    g_nextStepUs = now + currentStepInterval();

    stepOnce((g_target > g_position) ? (int8_t)1 : (int8_t)-1);
}

/* --- the demo script ------------------------------------------------------- */

typedef struct {
    const char  *what;
    step_mode_t  mode;
    int8_t       quarters;      /* quarter-revolutions, signed */
} move_step_t;

static const move_step_t g_script[] = {
    { "full-step, one quarter turn forward",  MODE_FULL,  1 },
    { "full-step, one quarter turn back",     MODE_FULL, -1 },
    { "wave-step, half a turn forward",       MODE_WAVE,  2 },
    { "half-step, half a turn back",          MODE_HALF, -2 },
    { "full-step, one whole turn forward",    MODE_FULL,  4 },
    { "full-step, one whole turn back",       MODE_FULL, -4 },
};

#define SCRIPT_LEN  (sizeof(g_script) / sizeof(g_script[0]))

static size_t g_scriptIndex = 0;

static void startScriptStep(void)
{
    const move_step_t *m = &g_script[g_scriptIndex];
    long steps;

    g_mode  = m->mode;
    g_phase = 0;
    steps   = (stepsPerRev(g_mode) / 4L) * (long)m->quarters;

    Serial.print(g_scriptIndex + 1U);
    Serial.print("/");
    Serial.print((unsigned int)SCRIPT_LEN);
    Serial.print("  ");
    Serial.print(m->what);
    Serial.print("  (");
    Serial.print(steps);
    Serial.print(" steps, ");
    Serial.print(STEP_US_MIN);
    Serial.println(" us/step at speed)");

    beginMove(steps);
}

/* --- button ---------------------------------------------------------------- */

/*
 * Polled, not interrupt-driven, and that is the right call here: the button only
 * matters between steps, polling costs nothing next to a 1.8 ms step interval,
 * and an ISR that reaches into the motion state would need every one of these
 * longs guarded. NanoInterrupts covers the interrupt-driven version.
 */
static void serviceButton(void)
{
    if (digitalRead(BUTTON_BUILTIN) != LOW) {
        return;
    }

    if ((millis() - g_lastBtnMs) < DEBOUNCE_MS) {
        return;
    }

    g_lastBtnMs = millis();

    if (moveInProgress()) {
        reverseMove();
        Serial.print("   SW0 -> reversing, ");
        Serial.print(g_target - g_position);
        Serial.println(" steps to go");
    }
}

void setup()
{
    uint8_t i;

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LED_OFF);
    pinMode(BUTTON_BUILTIN, INPUT_PULLUP);

    for (i = 0; i < 4U; i++) {
        pinMode(g_coilPins[i], OUTPUT);
    }
    releaseCoils();

    Serial.begin(115200);

    Serial.println();
    Serial.println("======================================================");
    Serial.println(" 28BYJ-48 stepper - dsPIC33CK256MC005 Curiosity Nano");
    Serial.println("======================================================");
    Serial.println("coils : D21..D24 (RC0..RC3) -> ULN2003 IN1..IN4");
    Serial.println("power : separate 5 V to the ULN2003, grounds common");
    Serial.print  ("full  : ");
    Serial.print(STEPS_PER_REV_FULL);
    Serial.println(" steps/rev   half: twice that");
    Serial.print  ("ramp  : ");
    Serial.print(STEP_US_START);
    Serial.print  (" us -> ");
    Serial.print(STEP_US_MIN);
    Serial.print  (" us over ");
    Serial.print(RAMP_STEPS);
    Serial.println(" steps");
    Serial.println();
    Serial.println("Press SW0 during a move to send the motor back.");
    Serial.println("No motor attached? Watch the four LEDs on the driver board.");
    Serial.println();

    startScriptStep();
}

void loop()
{
    serviceMotor();
    serviceButton();

    if (!moveInProgress()) {
        /* Move finished: release the coils, report, pause, take the next one. */
        releaseCoils();

        Serial.print("      done, position ");
        Serial.print(g_position);
        Serial.println(" steps");

        delay(700);

        g_scriptIndex++;
        if (g_scriptIndex >= SCRIPT_LEN) {
            g_scriptIndex = 0;

            Serial.println();
            Serial.print("--- script complete, net position ");
            Serial.print(g_position);
            Serial.println(" steps (should be 0) ---");
            Serial.println();
            delay(1500);
        }

        startScriptStep();
    }
}
