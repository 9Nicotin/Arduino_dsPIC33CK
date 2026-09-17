/*
 * pins_arduino.h - Pin mapping for dsPIC33CK256MP508
 *
 * dsPIC33CK256MP508 is an 80-pin device with:
 *   - PORTA: RA0-RA4 (5 pins)
 *   - PORTB: RB0-RB15 (16 pins)
 *   - PORTC: RC0-RC15 (16 pins)
 *   - PORTD: RD0-RD15 (16 pins)
 *   - PORTE: RE0-RE15 (16 pins)
 *   - 256KB Flash, 24KB RAM
 *   - DFP: dsPIC33CK-MP_DFP
 *   - Board: dsPIC33CK Curiosity Development Board (DM330030)
 *
 * Arduino Pin Mapping:
 *   D0-D4   = RA0-RA4 (PORTA)
 *   D5-D20  = RB0-RB15 (PORTB)
 *   D21-D36 = RC0-RC15 (PORTC)
 *   D37-D52 = RD0-RD15 (PORTD)
 *   D53-D68 = RE0-RE15 (PORTE)
 *
 * Analog Inputs (23 channels):
 *   A0=RA0(AN0)   A1=RA2(AN9)   A2=RA3(AN3)   A3=RA4(AN4)
 *   A4=RB0(AN5)   A5=RB1(AN6)   A6=RB2(AN1)   A7=RB3(AN8)
 *   A8=RB7(AN2)   A9=RB8(AN10)  A10=RB9(AN11)
 *   A11=RC0(AN12)  A12=RC1(AN13)  A13=RC2(AN14)  A14=RC3(AN15)
 *   A15=RC6(AN17)  A16=RC7(AN16)
 *   A17=RD10(AN18) A18=RD11(AN19)
 *   A19=RE0(AN20)  A20=RE1(AN21)  A21=RE2(AN22)  A22=RE3(AN23)
 *
 * PWM Pins: D5(RB0), D6(RB1), D7(RB2), D8(RB3) via SCCP/MCCP
 * UART TX: D41 (RD4/RP68 via PPS — PKOB4 CDC on DM330030)
 * UART RX: D40 (RD3/RP67 via PPS — PKOB4 CDC on DM330030)
 * SPI:     SCK=D13(RB8), MISO=D12(RB7), MOSI=D11(RB6)
 * I2C:     SDA=D14(RB9), SCL=D15(RB10)
 * LED1:    D59 (RE6 on DM330030 Curiosity Board)
 * LED2:    D58 (RE5 on DM330030 Curiosity Board)
 */

#ifndef PINS_ARDUINO_H
#define PINS_ARDUINO_H

#include <stdint.h>

#define NUM_DIGITAL_PINS    69
#define NUM_ANALOG_INPUTS   23

/* LED_BUILTIN - RE6 (LED1 on DM330030 Curiosity Board) */
#define LED_BUILTIN         59
#define LED1                59  /* RE6 */
#define LED2                58  /* RE5 */

/* Analog pin aliases */
#define A0  0   /* RA0 = AN0 */
#define A1  2   /* RA2 = AN9 */
#define A2  3   /* RA3 = AN3 */
#define A3  4   /* RA4 = AN4 */
#define A4  5   /* RB0 = AN5 */
#define A5  6   /* RB1 = AN6 */
#define A6  7   /* RB2 = AN1 */
#define A7  8   /* RB3 = AN8 */
#define A8  12  /* RB7 = AN2 */
#define A9  13  /* RB8 = AN10 */
#define A10 14  /* RB9 = AN11 */
#define A11 21  /* RC0 = AN12 */
#define A12 22  /* RC1 = AN13 */
#define A13 23  /* RC2 = AN14 */
#define A14 24  /* RC3 = AN15 */
#define A15 27  /* RC6 = AN17 */
#define A16 28  /* RC7 = AN16 */
#define A17 47  /* RD10 = AN18 */
#define A18 48  /* RD11 = AN19 */
#define A19 53  /* RE0 = AN20 */
#define A20 54  /* RE1 = AN21 */
#define A21 55  /* RE2 = AN22 */
#define A22 56  /* RE3 = AN23 */

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

/* Serial pin definitions — PKOB4 CDC on DM330030 Curiosity Board */
#define PIN_SERIAL_TX       41  /* RD4 */
#define PIN_SERIAL_RX       40  /* RD3 */
#define SERIAL_TX_RP_REG    _RP68R  /* PPS output register for RD4 */
#define SERIAL_RX_RP_NUM    67      /* RP number for RD3 input */
#define SERIAL_TX_TRIS      TRISDbits.TRISD4
#define SERIAL_RX_TRIS      TRISDbits.TRISD3
/* #define SERIAL_RX_ANSEL  — RD3 has no analog function, always digital */

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
#define digitalPinHasPWM(p) (((p) >= 5 && (p) <= 8) || (p) == 58)

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
#define PPS_OUT_OCM5    19  /* SCCP5 Output Compare */
#define PPS_OUT_OCM6    20  /* SCCP6 Output Compare */

/* PWM pin definitions: Arduino pin, RP number, SCCP module */
#define PWM1_PIN        5   /* D5 = RB0 */
#define PWM1_RP         32  /* RP32 */
#define PWM2_PIN        6   /* D6 = RB1 */
#define PWM2_RP         33  /* RP33 */
#define PWM3_PIN        7   /* D7 = RB2 */
#define PWM3_RP         34  /* RP34 */
#define PWM4_PIN        8   /* D8 = RB3 */
#define PWM4_RP         35  /* RP35 */
#define PWM5_PIN        58  /* D58 = RE5 (LED2 on DM330030) */
#define PWM5_RP         181 /* RP181 */

#define NUM_PWM_PINS        5

#endif /* PINS_ARDUINO_H */
