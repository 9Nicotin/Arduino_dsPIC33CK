/*
 * pins_arduino.h - Pin mapping for dsPIC33CK256MC005
 *
 * dsPIC33CK256MC005 is a 48-pin TQFP Value Line Motor Control device with:
 *   - PORTA: RA0-RA4        (5 pins, all analog-capable, NOT remappable)
 *   - PORTB: RB0-RB15       (16 pins, RP32-RP47)
 *   - PORTC: RC0-RC13       (14 pins, RP48-RP61)
 *   - PORTD: RD1, RD8, RD10, RD13 only (RP65, RP72, RP74, RP77)
 *   - 256KB Flash (ECC), 16KB RAM
 *   - SCCP1-4, PWM generators PG1-PG4, U1-U3, SPI1-2, I2C1, DAC1
 *   - DFP: dsPIC33CK-MC_DFP >= 1.10.386 (earlier packs have no MC005)
 *   - Board: dsPIC33CK Value Line Curiosity Nano (EV08P02A)
 *
 * Board wiring (verified against Microchip's dspic33ck256mc005-curiosity-nano
 * out-of-box demo, v1.0.0 — mcc_generated_files/system/src/pins.c, bsp/led0.c,
 * bsp/sw0.c, system/src/config_bits.c):
 *   LED0      = RD10, ACTIVE LOW  (see LED_BUILTIN_ACTIVE_LOW below)
 *   SW0       = RD13, internal pull-up, ACTIVE LOW
 *   CDC UART  = U1TX on RC10 (RP58), U1RX on RC11 (RP59)  -> nEDBG virtual COM
 *   Debugger  = ICS = PGD3/PGC3, i.e. RB5 and RB6 are RESERVED on this board
 *   ALTI2C1   = OFF, so I2C1 uses SDA1 = RB9, SCL1 = RB8
 *
 * Arduino Pin Mapping (contiguous over bonded pins only):
 *   D0-D4   = RA0-RA4
 *   D5-D20  = RB0-RB15
 *   D21-D34 = RC0-RC13
 *   D35     = RD1
 *   D36     = RD8
 *   D37     = RD10  (LED0)
 *   D38     = RD13  (SW0)
 *
 * Analog Inputs (20 channels, mapping taken from the device's edc pin database
 * DSPIC33CK256MC005.PIC — note it differs from MP508 and MC002):
 *   A0 =RA0(AN0)   A1 =RA1(AN16)  A2 =RA2(AN9)   A3 =RA3(AN3)   A4 =RA4(AN4)
 *   A5 =RB0(AN5)   A6 =RB1(AN6)   A7 =RB2(AN1)   A8 =RB3(AN8)   A9 =RB4(AN17)
 *   A10=RB7(AN2)   A11=RB8(AN10)  A12=RB9(AN11)
 *   A13=RC0(AN12)  A14=RC1(AN13)  A15=RC2(AN14)  A16=RC3(AN15)
 *   A17=RC6(AN19)  A18=RC7(AN7)   A19=RD10(AN18)
 *   RD13 is ANN0 (ADC negative input only) and is NOT an analog input here.
 *
 * PWM Pins: D5(RB0), D6(RB1), D7(RB2), D8(RB3) via SCCP1-4.
 *           This part has only CCP1-4 — there is no PWM5 (unlike MP508).
 * UART TX:  D31 (RC10/RP58 via PPS — nEDBG CDC)
 * UART RX:  D32 (RC11/RP59 via PPS — nEDBG CDC)
 * SPI:      SCK=D25(RC4), MOSI=D26(RC5), MISO=D33(RC12), SS=D34(RC13)
 *           Chosen to avoid RB5/RB6 (debugger), RB8/RB9 (I2C1),
 *           RB0-RB3 (PWM), RB10-RB15/RD1 (PWM generator outputs used by the
 *           HRPWM library) and RC10/RC11 (CDC UART). None of the four costs
 *           an ADC channel.
 * I2C:      SDA=D14(RB9), SCL=D13(RB8) — dedicated I2C1 pins, no PPS
 */

#ifndef PINS_ARDUINO_H
#define PINS_ARDUINO_H

#include <stdint.h>

#define NUM_DIGITAL_PINS    39
#define NUM_ANALOG_INPUTS   20
#define NUM_PWM_PINS        4

/* LED_BUILTIN - RD10 (LED0 on EV08P02A Curiosity Nano).
 * NOTE: the LED is wired ACTIVE LOW (cathode to the pin), so
 * digitalWrite(LED_BUILTIN, LOW) turns it ON. Stock Blink still blinks. */
#define LED_BUILTIN             37
#define LED0                    37
#define LED_BUILTIN_ACTIVE_LOW  1

/* SW0 on EV08P02A - RD13, needs CNPUD13 pull-up, reads LOW when pressed */
#define SW0                     38
#define BUTTON_BUILTIN          38
#define BUTTON_BUILTIN_ACTIVE_LOW 1

/* Analog pin aliases (PORTA) */
#define A0  0   /* RA0 = AN0  */
#define A1  1   /* RA1 = AN16 */
#define A2  2   /* RA2 = AN9  */
#define A3  3   /* RA3 = AN3  */
#define A4  4   /* RA4 = AN4  */

/* Analog pin aliases (PORTB) */
#define A5  5   /* RB0 = AN5  */
#define A6  6   /* RB1 = AN6  */
#define A7  7   /* RB2 = AN1  */
#define A8  8   /* RB3 = AN8  */
#define A9  9   /* RB4 = AN17 */
#define A10 12  /* RB7 = AN2  */
#define A11 13  /* RB8 = AN10 — also I2C1 SCL1 */
#define A12 14  /* RB9 = AN11 — also I2C1 SDA1 */

/* Analog pin aliases (PORTC) */
#define A13 21  /* RC0 = AN12 */
#define A14 22  /* RC1 = AN13 */
#define A15 23  /* RC2 = AN14 */
#define A16 24  /* RC3 = AN15 */
#define A17 27  /* RC6 = AN19 */
#define A18 28  /* RC7 = AN7  */

/* Analog pin alias (PORTD) */
#define A19 37  /* RD10 = AN18 — also LED0 on EV08P02A */

/* SPI pin definitions */
#define PIN_SPI_SS          34  /* RC13 */
#define PIN_SPI_MOSI        26  /* RC5  */
#define PIN_SPI_MISO        33  /* RC12 */
#define PIN_SPI_SCK         25  /* RC4  */

static const uint8_t SS   = PIN_SPI_SS;
static const uint8_t MOSI = PIN_SPI_MOSI;
static const uint8_t MISO = PIN_SPI_MISO;
static const uint8_t SCK  = PIN_SPI_SCK;

/* SPI PPS mapping macros */
#define SPI_SCK_RP_REG      _RP52R      /* PPS output register for RC4  (SCK1) */
#define SPI_MOSI_RP_REG     _RP53R      /* PPS output register for RC5  (SDO1) */
#define SPI_MISO_RP_NUM     60          /* RP number for RC12 (SDI1 input) */
#define SPI_SS_RP_REG       _RP61R      /* PPS output register for RC13 (SS1) */
#define SPI_SCK_TRIS        TRISCbits.TRISC4
#define SPI_MOSI_TRIS       TRISCbits.TRISC5
#define SPI_MISO_TRIS       TRISCbits.TRISC12
#define SPI_SS_TRIS         TRISCbits.TRISC13
/* #define SPI_MISO_ANSEL   — RC12 has no analog function, always digital */

/* I2C pin definitions (dedicated I2C1 pins, no PPS needed).
 * ALTI2C1 must stay OFF (the default in cores/arduino/system_config.c). */
#define PIN_WIRE_SDA        14  /* RB9 = SDA1 */
#define PIN_WIRE_SCL        13  /* RB8 = SCL1 */
#define WIRE_SDA_TRIS       TRISBbits.TRISB9
#define WIRE_SCL_TRIS       TRISBbits.TRISB8

static const uint8_t SDA = PIN_WIRE_SDA;
static const uint8_t SCL = PIN_WIRE_SCL;

/* Serial pin definitions — nEDBG CDC virtual COM port on EV08P02A */
#define PIN_SERIAL_TX       31  /* RC10 */
#define PIN_SERIAL_RX       32  /* RC11 */
#define SERIAL_TX_RP_REG    _RP58R  /* PPS output register for RC10 */
#define SERIAL_RX_RP_NUM    59      /* RP number for RC11 input */
#define SERIAL_TX_TRIS      TRISCbits.TRISC10
#define SERIAL_RX_TRIS      TRISCbits.TRISC11
/* #define SERIAL_RX_ANSEL  — RC11 has no analog function, always digital */

/* ============================================================
 * Internal pin table structure
 * Each Arduino pin maps to a PORT register and bit position
 * ============================================================ */

typedef struct {
    volatile uint16_t *port_reg;   /* PORTx register */
    volatile uint16_t *tris_reg;   /* TRISx register */
    volatile uint16_t *lat_reg;    /* LATx register */
    volatile uint16_t *ansel_reg;  /* ANSELx register (NULL if no analog) */
    uint8_t bit;                   /* bit position 0-15 */
    int8_t  adc_channel;           /* ADC channel or -1 if none */
} pin_map_t;

/* Port register addresses (defined in variant.c) */
#ifdef __cplusplus
extern "C" {
#endif
extern const pin_map_t g_pin_map[];
#ifdef __cplusplus
}
#endif

/* PWM capable pins */
#define digitalPinHasPWM(p) ((p) >= 5 && (p) <= 8)

/* Every pin can carry an interrupt (Change Notification covers all ports), so
 * this is the identity -- it exists only so sketches written for AVR, where the
 * mapping is real, compile unchanged. attachInterrupt() takes the pin number. */
#define digitalPinToInterrupt(p) (((p) < NUM_DIGITAL_PINS) ? (int)(p) : -1)

/* Analog channel for a pin (-1 if not analog) */
#define analogPinToChannel(p) (g_pin_map[p].adc_channel)

/* PPS output function codes (from device header _RPOUT_ defines) */
#define PPS_OUT_U1TX    1
#define PPS_OUT_U1RTS   2
#define PPS_OUT_U2TX    3
#define PPS_OUT_SDO1    5
#define PPS_OUT_SCK1OUT 6
#define PPS_OUT_SS1OUT  7
#define PPS_OUT_OCM1    15  /* SCCP1 Output Compare */
#define PPS_OUT_OCM2    16  /* SCCP2 Output Compare */
#define PPS_OUT_OCM3    17  /* SCCP3 Output Compare */
#define PPS_OUT_OCM4    18  /* SCCP4 Output Compare */
/* No OCM5/OCM6 — dsPIC33CK256MC005 has only SCCP1-4 */

/* PWM pin definitions: Arduino pin, RP number, SCCP module */
#define PWM1_PIN        5   /* D5 = RB0 */
#define PWM1_RP         32  /* RP32 */
#define PWM2_PIN        6   /* D6 = RB1 */
#define PWM2_RP         33  /* RP33 */
#define PWM3_PIN        7   /* D7 = RB2 */
#define PWM3_RP         34  /* RP34 */
#define PWM4_PIN        8   /* D8 = RB3 */
#define PWM4_RP         35  /* RP35 */

#endif /* PINS_ARDUINO_H */
