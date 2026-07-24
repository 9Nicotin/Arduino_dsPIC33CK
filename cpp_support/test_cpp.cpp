/*
 * test_cpp.cpp — Test C++ features on dsPIC33CK
 *
 * Compile with:
 *   xc-dsc-g++ -mcpu=dsPIC33CK32MP102 -fno-exceptions -fno-rtti \
 *     -fno-threadsafe-statics -I<dfp-include-path> -c test_cpp.cpp
 *
 * This file tests:
 *   1. Classes with constructor/destructor
 *   2. Inheritance and virtual functions
 *   3. Function overloading
 *   4. Templates
 *   5. References
 */

#include <xc.h>
#include <stdint.h>

/* ============================================================
 * Test 1: Basic class
 * ============================================================ */
class Pin {
    volatile uint16_t *lat_reg;
    uint8_t bit;
public:
    Pin(volatile uint16_t *lat, uint8_t b) : lat_reg(lat), bit(b) {}
    void high() { *lat_reg |= (1U << bit); }
    void low()  { *lat_reg &= ~(1U << bit); }
    void toggle() { *lat_reg ^= (1U << bit); }
};

/* ============================================================
 * Test 2: Inheritance + virtual functions
 * ============================================================ */
class Stream {
public:
    virtual void write(uint8_t c) = 0;
    virtual void print(const char *str) {
        while (*str) write(*str++);
    }
};

class HardwareSerial : public Stream {
public:
    void begin(unsigned long baud) {
        U1BRG = (FCY / (16UL * baud)) - 1;
        U1MODEbits.UARTEN = 1;
    }
    void write(uint8_t c) override {
        while (U1STAHbits.UTXBF);
        U1TXREG = c;
    }
};

/* ============================================================
 * Test 3: Function overloading
 * ============================================================ */
void print_value(int val) {
    (void)val;
}

void print_value(const char *str) {
    (void)str;
}

void print_value(float val) {
    (void)val;
}

/* ============================================================
 * Test 4: Template
 * ============================================================ */
template<typename T, int SIZE>
class RingBuffer {
    T data[SIZE];
    uint8_t head;
    uint8_t tail;
public:
    RingBuffer() : head(0), tail(0) {}
    bool push(T item) {
        uint8_t next = (head + 1) % SIZE;
        if (next == tail) return false;
        data[head] = item;
        head = next;
        return true;
    }
    bool pop(T &item) {
        if (head == tail) return false;
        item = data[tail];
        tail = (tail + 1) % SIZE;
        return true;
    }
    bool empty() const { return head == tail; }
};

/* ============================================================
 * Test 5: Putting it all together
 * ============================================================ */
#ifndef FCY
#define FCY 4000000UL
#endif

Pin led(&LATA, 0);
HardwareSerial serial;
RingBuffer<uint8_t, 64> rxBuffer;

int main()
{
    led.high();

    serial.begin(9600);
    serial.print("Hello from C++!");

    print_value(42);
    print_value("test");
    print_value(3.14f);

    rxBuffer.push(0x55);
    uint8_t byte;
    rxBuffer.pop(byte);

    while (1) {
        led.toggle();
    }

    return 0;
}
