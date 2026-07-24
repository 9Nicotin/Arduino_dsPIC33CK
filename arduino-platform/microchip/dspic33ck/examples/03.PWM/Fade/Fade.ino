/*
 * Fade - Classic Arduino LED fade example for dsPIC33CK
 *
 * Fades onboard LED2 (RE5/D58) using analogWrite().
 * No external components needed on DM330030 board.
 *
 * Board: dsPIC33CK256MP508 Curiosity (DM330030)
 * PWM frequency: ~490 Hz
 */

#include <Arduino.h>

int brightness = 0;
int fadeAmount = 5;

void setup()
{
    pinMode(LED2, OUTPUT);
}

void loop()
{
    analogWrite(LED2, brightness);

    brightness = brightness + fadeAmount;

    if (brightness <= 0 || brightness >= 255) {
        fadeAmount = -fadeAmount;
    }

    delay(30);
}
