/* Arduino LED blink test - compiled as C */
#include "Arduino.h"

void setup(void) {
    pinMode(59, OUTPUT);  /* RE6 = LED1 */
    pinMode(58, OUTPUT);  /* RE5 = LED2 */
}

void loop(void) {
    digitalWrite(59, HIGH);
    digitalWrite(58, LOW);
    delay(500);
    digitalWrite(59, LOW);
    digitalWrite(58, HIGH);
    delay(500);
}
