/*
 * CppDemo - dsPIC33CK C++ Features Demo
 *
 * Demonstrates real C++ on dsPIC33CK via XC-DSC v4.00:
 *   - Classes with constructors/methods
 *   - Templates
 *   - Function overloading
 *   - Encapsulation (private/public)
 *
 * Board: dsPIC33CK256MP508 Curiosity (DM330030)
 * Serial: 115200 baud via PKOB4 USB-CDC
 */

#include <Arduino.h>

// ===== Class: LED with blink pattern =====
class LED {
    uint8_t _pin;
    bool _state;
public:
    LED(uint8_t pin) : _pin(pin), _state(false) {}

    void begin() {
        pinMode(_pin, OUTPUT);
        off();
    }

    void on() {
        digitalWrite(_pin, HIGH);
        _state = true;
    }

    void off() {
        digitalWrite(_pin, LOW);
        _state = false;
    }

    void toggle() {
        _state = !_state;
        digitalWrite(_pin, _state ? HIGH : LOW);
    }

    bool isOn() const { return _state; }
};

// ===== Template: Ring buffer =====
template<typename T, uint8_t N>
class RingBuffer {
    T _data[N];
    uint8_t _head;
    uint8_t _count;
public:
    RingBuffer() : _head(0), _count(0) {}

    bool push(T val) {
        if (_count >= N) return false;
        _data[(_head + _count) % N] = val;
        _count++;
        return true;
    }

    T pop() {
        T val = _data[_head];
        _head = (_head + 1) % N;
        _count--;
        return val;
    }

    uint8_t size() const { return _count; }
    bool isFull() const { return _count >= N; }
    bool isEmpty() const { return _count == 0; }

    T average() const {
        if (_count == 0) return 0;
        long sum = 0;
        for (uint8_t i = 0; i < _count; i++) {
            sum += _data[(_head + i) % N];
        }
        return (T)(sum / _count);
    }
};

// ===== Function overloading =====
void printValue(int val) {
    Serial.print("Int: ");
    Serial.println_int(val, DEC);
}

void printValue(const char *str) {
    Serial.print("Str: ");
    Serial.println(str);
}

void printValue(float val) {
    Serial.print("Float: ");
    Serial.println_float(val, 2);
}

// ===== Global objects =====
LED led1(LED_BUILTIN);   // RE6 on DM330030
LED led2(LED2);          // RE5 on DM330030
RingBuffer<uint16_t, 8> adcSamples;

unsigned long lastPrint = 0;

void setup()
{
    Serial.begin(115200);
    delay(2000);
    Serial.println("=== C++ Demo on dsPIC33CK ===");
    Serial.println("XC-DSC v4.00 C++ Support");
    Serial.println("");

    led1.begin();
    led2.begin();

    // Demonstrate function overloading
    printValue(42);
    printValue("Hello C++!");
    printValue(3.14f);
    Serial.println("");
}

void loop()
{
    // Read ADC and store in ring buffer
    uint16_t adc = analogRead(A22);  // RE3/AN23 = potentiometer on DM330030
    adcSamples.push(adc);

    // Print average every 2 seconds
    unsigned long now = millis();
    if (now - lastPrint >= 500) {
        lastPrint = now;

        Serial.print("ADC avg(");
        Serial.print_int(adcSamples.size(), DEC);
        Serial.print(" samples): ");
        Serial.println_int(adcSamples.average(), DEC);

        // Drain buffer after reporting
        while (!adcSamples.isEmpty()) {
            adcSamples.pop();
        }
    }

    // Toggle LEDs alternately
    led1.toggle();
    led2.toggle();
    delay(250);
}
