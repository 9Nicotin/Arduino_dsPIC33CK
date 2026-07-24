// test_arduino_cpp.cpp — Test C++ with Arduino core headers
// Simulates what Arduino IDE does with .ino files

#include <Arduino.h>
#include <stdint.h>

// Test: C++ class for LED control
class LED {
    uint8_t pin;
public:
    LED(uint8_t p) : pin(p) {}
    void on()     { digitalWrite(pin, HIGH); }
    void off()    { digitalWrite(pin, LOW); }
    void toggle() { digitalWrite(pin, !digitalRead(pin)); }
};

// Test: Template
template<typename T, int N>
class Buffer {
    T data[N];
    int count;
public:
    Buffer() : count(0) {}
    bool push(T val) {
        if (count >= N) return false;
        data[count++] = val;
        return true;
    }
    T pop() { return data[--count]; }
    int size() const { return count; }
};

// Test: Function overloading
void report(int val)        { Serial.print_int(val, 10); }
void report(const char *s)  { Serial.print(s); }
void report(float f)        { Serial.print_float(f, 2); }

// Arduino entry points
LED redLed(5);
Buffer<uint16_t, 32> adcBuffer;

void setup()
{
    Serial.begin(115200);
    Serial.println("C++ on dsPIC33CK!");

    pinMode(5, OUTPUT);
    redLed.on();

    report(42);
    report("hello");
    report(3.14f);
}

void loop()
{
    uint16_t val = analogRead(A0);
    adcBuffer.push(val);

    if (adcBuffer.size() >= 10) {
        Serial.println("Buffer has 10 samples");
    }

    redLed.toggle();
    delay(500);
}
