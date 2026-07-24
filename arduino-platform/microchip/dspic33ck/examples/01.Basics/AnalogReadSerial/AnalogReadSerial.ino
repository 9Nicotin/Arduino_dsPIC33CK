/*
 * AnalogReadSerial - dsPIC33CK Arduino Example
 *
 * Reads analog input on A0 (RB0/AN0) and prints to Serial.
 *
 * Hardware:
 *   - Connect potentiometer center pin to RB0 (A0/D5)
 *   - Pot ends to 3.3V and GND
 *   - UART TX (RB5/D10) to USB-Serial adapter RX
 *   - UART RX (RB4/D9) to USB-Serial adapter TX
 *
 * Pin Mapping:
 *   A0 = D5 = RB0/AN0 (analog input)
 *   Serial TX = D10 = RB5
 *   Serial RX = D9  = RB4
 */

#include <Arduino.h>

void setup()
{
    Serial.begin(9600);
    Serial.println("dsPIC33CK Arduino - Analog Read");
}

void loop()
{
    int sensorValue = analogRead(A0);

    Serial.print("ADC Value: ");
    Serial.println_int(sensorValue, DEC);

    delay(250);
}
