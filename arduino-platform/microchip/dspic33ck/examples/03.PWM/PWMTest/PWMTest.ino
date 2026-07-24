/*
 * PWMTest - dsPIC33CK Hardware PWM Verification (Phase 9)
 *
 * Tests PWM via analogWrite() on onboard LED2 (RE5/D58):
 *   - Test 1: LED2 at 50% brightness
 *   - Test 2: LED2 at 25%, 50%, 75%, full
 *   - Test 3: Smooth fade (breathing)
 *   - Test 4: Off/On transition (PWM -> digital -> PWM)
 *
 * Expected PWM frequency: ~490 Hz (Arduino standard)
 * At FCY=100MHz: prescaler 1:64, period=3187, actual freq=490.5 Hz
 *
 * Board: dsPIC33CK256MP508 Curiosity (DM330030)
 * LED2 = RE5 = D58 = RP181 -> SCCP5
 * Serial: 115200 baud via PKOB4 USB-CDC
 *
 * Verification:
 *   - Observe LED2 brightness changes visually
 *   - LED1 (RE6) used as digital toggle indicator
 */

#include <Arduino.h>

#define PWM_LED     LED2        /* D58 = RE5, PWM via SCCP5 */
#define STATUS_LED  LED_BUILTIN /* D59 = RE6, digital toggle */

static int testNum = 0;

void printHeader(const char *title)
{
    Serial.println("");
    Serial.print("=== Test ");
    Serial.print_int(++testNum, DEC);
    Serial.print(": ");
    Serial.print(title);
    Serial.println(" ===");
}

void setup()
{
    Serial.begin(115200);
    delay(2000);

    pinMode(STATUS_LED, OUTPUT);
    digitalWrite(STATUS_LED, HIGH);

    Serial.println("=== PWM Hardware Test (Phase 9) ===");
    Serial.print("F_CPU = ");
    Serial.print_int((long)F_CPU, DEC);
    Serial.println(" Hz");
    Serial.print("FCY   = ");
    Serial.print_int((long)FCY, DEC);
    Serial.println(" Hz");
    Serial.println("");
    Serial.println("PWM on LED2 (RE5/D58) via SCCP5");
    Serial.println("Expected frequency: ~490 Hz");
    Serial.println("Waiting 3s before starting tests...");
    delay(3000);

    /* --- Test 1: 50% brightness --- */
    printHeader("LED2 at 50% duty");
    analogWrite(PWM_LED, 128);
    Serial.println("analogWrite(LED2, 128) -> 50% brightness");
    Serial.println("LED2 should be visibly dimmer than full ON");
    delay(4000);

    /* --- Test 2: Step through brightness levels --- */
    printHeader("Brightness steps");

    Serial.println("  25% ...");
    analogWrite(PWM_LED, 64);
    delay(2000);

    Serial.println("  50% ...");
    analogWrite(PWM_LED, 128);
    delay(2000);

    Serial.println("  75% ...");
    analogWrite(PWM_LED, 191);
    delay(2000);

    Serial.println("  100% (digital HIGH) ...");
    analogWrite(PWM_LED, 255);
    delay(2000);

    Serial.println("  0% (OFF) ...");
    analogWrite(PWM_LED, 0);
    delay(2000);

    /* --- Test 3: Off then back on --- */
    printHeader("PWM off -> re-enable");
    Serial.println("OFF...");
    analogWrite(PWM_LED, 0);
    delay(1000);
    Serial.println("Back to 50%...");
    analogWrite(PWM_LED, 128);
    delay(2000);
    Serial.println("OK - PWM re-enabled after stop");

    /* --- Done with static tests --- */
    Serial.println("");
    Serial.println("=== Static tests complete ===");
    Serial.println("Entering breathing loop on LED2...");
    Serial.println("LED1 toggles each cycle as heartbeat.");
    Serial.println("");
}

void loop()
{
    static uint8_t brightness = 0;
    static int8_t direction = 1;
    static uint8_t led1_state = 0;

    analogWrite(PWM_LED, brightness);

    brightness += direction;
    if (brightness == 255 || brightness == 0) {
        direction = -direction;
        /* Toggle LED1 at each breath peak/valley */
        led1_state = !led1_state;
        digitalWrite(STATUS_LED, led1_state);
    }

    delay(5);
}
