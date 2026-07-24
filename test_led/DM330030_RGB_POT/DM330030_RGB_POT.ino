/*
 * DM330030_RGB_POT.ino
 *
 * Peripheral verification sketch for dsPIC33CK256MP508 Curiosity Dev Board:
 *   - ADC:  Read potentiometer on AN23 (RE3) via analogRead()
 *   - PWM:  Software PWM drives RGB LED (R=RE15, G=RE14, B=RE13)
 *   - UART: Print POT/RGB values via Serial (PKOB4 CDC, TX=RD4, RX=RD3)
 *
 * POT position maps through an HSV color wheel:
 *   0    -> Red
 *   1/3  -> Green
 *   2/3  -> Blue
 *   Full -> Red (wraps)
 *
 * Software PWM uses SCCP3 in timer mode (~25.6 kHz ISR, 100 Hz PWM).
 * Timer1 is reserved for millis()/micros().
 *
 * Board: DM330030 dsPIC33CK Curiosity Development Board
 * Clock: 8 MHz FRC -> FCY = 4 MHz
 */

#include <Arduino.h>

/* ============================================================
 * Pin Definitions
 * ============================================================ */
#define POT_PIN         A22     /* RE3 = D56, AN23 */

#define RGB_R_PIN       68      /* RE15 = D68 */
#define RGB_G_PIN       67      /* RE14 = D67 */
#define RGB_B_PIN       66      /* RE13 = D66 */

/* Port E bit masks for direct register manipulation in ISR */
#define RGB_R_MASK      (1U << 15)  /* RE15 */
#define RGB_G_MASK      (1U << 14)  /* RE14 */
#define RGB_B_MASK      (1U << 13)  /* RE13 */
#define RGB_ALL_MASK    (RGB_R_MASK | RGB_G_MASK | RGB_B_MASK)

/* ============================================================
 * Software PWM via SCCP3 Timer Mode
 *
 * SCCP3 in 16-bit timer mode, period = 155 counts
 * At FCY = 4 MHz: ISR rate = 4,000,000 / 156 = 25,641 Hz
 * 8-bit counter (256 steps): PWM frequency = 25,641 / 256 = 100 Hz
 * ============================================================ */
static volatile uint8_t pwm_counter = 0;
static volatile uint8_t pwm_r = 0;
static volatile uint8_t pwm_g = 0;
static volatile uint8_t pwm_b = 0;

void __attribute__((interrupt, no_auto_psv)) _CCT3Interrupt(void)
{
    uint8_t cnt = ++pwm_counter;
    uint16_t lat_val = LATE & ~RGB_ALL_MASK;

    if (cnt < pwm_r) lat_val |= RGB_R_MASK;
    if (cnt < pwm_g) lat_val |= RGB_G_MASK;
    if (cnt < pwm_b) lat_val |= RGB_B_MASK;

    LATE = lat_val;
    IFS2bits.CCT3IF = 0;
}

static void softpwm_init(void)
{
    PMD2bits.CCP3MD = 0;

    TRISE &= ~((1U << 13) | (1U << 14) | (1U << 15));
    LATE  &= ~((1U << 13) | (1U << 14) | (1U << 15));

    CCP3CON1L = 0x0000;
    CCP3CON1H = 0x0000;
    CCP3CON2L = 0x0000;
    CCP3CON2H = 0x0000;
    CCP3CON3H = 0x0000;

    CCP3CON1Lbits.CLKSEL = 0b000;
    CCP3CON1Lbits.TMRPS = 0b00;
    CCP3CON1Lbits.T32 = 0;
    CCP3CON1Lbits.MOD = 0b0000;

    CCP3PRL = 155;
    CCP3TMRL = 0;

    _CCT3IP = 5;
    _CCT3IF = 0;
    _CCT3IE = 1;

    CCP3CON1Lbits.CCPON = 1;
}

/* ============================================================
 * HSV Color Wheel: map 0-1023 POT value -> RGB (0-255 each)
 * ============================================================ */
static void pot_to_rgb(int pot, uint8_t *r, uint8_t *g, uint8_t *b)
{
    int region = pot / 341;
    int remainder = pot % 341;
    uint8_t rising  = (uint8_t)((remainder * 255L) / 341);
    uint8_t falling = 255 - rising;

    switch (region) {
        case 0:
            *r = falling;
            *g = rising;
            *b = 0;
            break;
        case 1:
            *r = 0;
            *g = falling;
            *b = rising;
            break;
        default:
            *r = rising;
            *g = 0;
            *b = falling;
            break;
    }
}

/* ============================================================
 * Arduino setup() / loop()
 * ============================================================ */
void setup()
{
    Serial.begin(9600);
    Serial.println("\r\n=== DM330030 RGB POT Demo ===");

    /* Quick GPIO test: flash all three RGB LEDs for 1 second */
    TRISE &= ~((1U << 13) | (1U << 14) | (1U << 15));
    LATE  |=  ((1U << 13) | (1U << 14) | (1U << 15));
    Serial.println("RGB GPIO test: all ON...");
    delay(1000);
    LATE  &= ~((1U << 13) | (1U << 14) | (1U << 15));
    Serial.println("RGB GPIO test: all OFF");
    delay(500);

    /* Set initial PWM to dim white (15%) */
    pwm_r = 38;
    pwm_g = 38;
    pwm_b = 38;

    /* Start software PWM via SCCP3 timer */
    softpwm_init();
    Serial.println("SCCP3 PWM started");

    /* POT pin as analog input */
    pinMode(POT_PIN, INPUT);

    /* LED1 as heartbeat indicator */
    pinMode(LED1, OUTPUT);

    /* Quick ADC test */
    Serial.print("ADC test read: ");
    Serial.println_int(analogRead(POT_PIN), DEC);

    Serial.println("POT: AN23/RE3 -> RGB LED (R=RE15, G=RE14, B=RE13)");
    Serial.println("UART: 9600 baud, TX=RD4, RX=RD3 (PKOB4 CDC)\r\n");
}

void loop()
{
    static unsigned long last_print = 0;
    static uint8_t led_state = 0;

    /* Read potentiometer (returns 0-1023) */
    int pot_val = analogRead(POT_PIN);

    /* Convert to RGB via color wheel */
    uint8_t r, g, b;
    pot_to_rgb(pot_val, &r, &g, &b);

    /* Update software PWM duty cycles */
    pwm_r = r;
    pwm_g = g;
    pwm_b = b;

    /* Print to Serial every 500ms */
    if (millis() - last_print >= 500) {
        last_print = millis();

        Serial.print("POT=");
        Serial.print_int(pot_val, DEC);
        Serial.print("  R=");
        Serial.print_int(r, DEC);
        Serial.print(" G=");
        Serial.print_int(g, DEC);
        Serial.print(" B=");
        Serial.println_int(b, DEC);

        /* Toggle LED1 as heartbeat */
        led_state ^= 1;
        digitalWrite(LED1, led_state);
    }

    delay(10);
}
