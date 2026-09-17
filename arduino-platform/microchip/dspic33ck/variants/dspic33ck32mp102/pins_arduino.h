/*
 * pins_arduino.h - Pin mapping for dsPIC33CK32MP102
 *
 * dsPIC33CK32MP102 is a 28-pin device with:
 *   - PORTA: RA0-RA4
 *   - PORTB: RB0-RB15
 *
 * Arduino Pin Mapping:
 *   D0-D4   = RA0-RA4 (PORTA)
 *   D5-D20  = RB0-RB15 (PORTB)
 *   A0-A5   = AN0-AN5 (RB0-RB5, shared with digital)
 *
 * PWM Pins: D5(RB0), D6(RB1), D7(RB2), D8(RB3) via SCCP/MCCP
 * UART TX: D10 (RB5 via PPS)
 * UART RX: D9  (RB4 via PPS)
 * SPI:     SCK=D13(RB8), MISO=D12(RB7), MOSI=D11(RB6)
 * I2C:     SDA=D14(RB9), SCL=D15(RB10)
 */

#ifndef PINS_ARDUINO_H
#define PINS_ARDUINO_H

#include <stdint.h>

#define NUM_DIGITAL_PINS    21
#define NUM_ANALOG_INPUTS   6
#define NUM_PWM_PINS        4

/* LED_BUILTIN - default to RA0 (D0) */
#define LED_BUILTIN         0

/* Analog pin aliases */
#define A0  5
#define A1  6
#define A2  7
#define A3  8
#define A4  9
#define A5  10

/* SPI pin definitions */
#define PIN_SPI_SS          16  /* RB11 */
#define PIN_SPI_MOSI        11  /* RB6 */
#define PIN_SPI_MISO        12  /* RB7 */
#define PIN_SPI_SCK         13  /* RB8 */

static const uint8_t SS   = PIN_SPI_SS;
static const uint8_t MOSI = PIN_SPI_MOSI;
static const uint8_t MISO = PIN_SPI_MISO;
static const uint8_t SCK  = PIN_SPI_SCK;

/* SPI PPS mapping macros */
#define SPI_SCK_RP_REG      _RP40R      /* PPS output register for RB8 (SCK) */
#define SPI_MOSI_RP_REG     _RP38R      /* PPS output register for RB6 (SDO1) */
#define SPI_MISO_RP_NUM     39          /* RP number for RB7 (SDI1 input) */
#define SPI_SS_RP_REG       _RP43R      /* PPS output register for RB11 (SS1) */
#define SPI_SCK_TRIS        TRISBbits.TRISB8
#define SPI_MOSI_TRIS       TRISBbits.TRISB6
#define SPI_MISO_TRIS       TRISBbits.TRISB7
#define SPI_SS_TRIS         TRISBbits.TRISB11
#define SPI_MISO_ANSEL      ANSELBbits.ANSELB7

/* I2C pin definitions (dedicated I2C1 pins, no PPS needed) */
#define PIN_WIRE_SDA        14  /* RB9 */
#define PIN_WIRE_SCL        15  /* RB10 */
#define WIRE_SDA_TRIS       TRISBbits.TRISB9
#define WIRE_SCL_TRIS       TRISBbits.TRISB10

static const uint8_t SDA = PIN_WIRE_SDA;
static const uint8_t SCL = PIN_WIRE_SCL;

/* Serial pin definitions */
#define PIN_SERIAL_TX       10  /* RB5 */
#define PIN_SERIAL_RX       9   /* RB4 */
#define SERIAL_TX_RP_REG    _RP37R  /* PPS output register for RB5 */
#define SERIAL_RX_RP_NUM    36      /* RP number for RB4 input */
#define SERIAL_TX_TRIS      TRISBbits.TRISB5
#define SERIAL_RX_TRIS      TRISBbits.TRISB4
#define SERIAL_RX_ANSEL     ANSELBbits.ANSELB4

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
#define PPS_OUT_SDO1    5
#define PPS_OUT_SCK1OUT 6
#define PPS_OUT_SS1OUT  7
#define PPS_OUT_OCM1    15  /* SCCP1 Output Compare */
#define PPS_OUT_OCM2    16  /* SCCP2 Output Compare */
#define PPS_OUT_OCM3    17  /* SCCP3 Output Compare */
#define PPS_OUT_OCM4    18  /* SCCP4 Output Compare */

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
