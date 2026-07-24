/*
 * HRPWM.h - High-Resolution PWM library for dsPIC33CK
 *
 * Provides access to PWM Generators PG1-PG8 with:
 *   - 250ps edge resolution (high-resolution mode)
 *   - 16-bit duty cycle and period
 *   - Complementary/Independent/Push-Pull output modes
 *   - Hardware dead-time (independent rise/fall)
 *   - ADC trigger synchronization
 *   - Hardware fault protection
 *
 * Pin mapping (dsPIC33CK256MP508, 80-pin):
 *   PG1: PWM1H=RB14(D19), PWM1L=RB15(D20)
 *   PG2: PWM2H=RB12(D17), PWM2L=RB13(D18)
 *   PG3: PWM3H=RB10(D15), PWM3L=RB11(D16)
 *   PG4: PPS routable (any RP pin)
 *   PG5: PWM5H=RC0(D21),  PWM5L=RC1(D22)
 *   PG6: PWM6H=RC2(D23),  PWM6L=RC3(D24)
 *   PG7: PWM7H=RC4(D25),  PWM7L=RC5(D26)
 *   PG8: PWM8H=RC6(D27),  PWM8L=RC7(D28)
 */

#ifndef HRPWM_H
#define HRPWM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Output modes (PGxIOCONH.PMOD) */
#define HRPWM_COMPLEMENTARY     0   /* PWMxH and PWMxL are complementary */
#define HRPWM_INDEPENDENT       1   /* PWMxH and PWMxL are independent */
#define HRPWM_PUSH_PULL         2   /* Push-pull (alternating cycles) */

/* ADC trigger source selection */
#define HRPWM_TRIGA             0   /* Trigger A compare */
#define HRPWM_TRIGB             1   /* Trigger B compare */
#define HRPWM_TRIGC             2   /* Trigger C compare */

/* Fault output action (what outputs do during fault) */
#define HRPWM_FAULT_LOW         0   /* Both outputs driven LOW */
#define HRPWM_FAULT_HIGH        3   /* Both outputs driven HIGH */
#define HRPWM_FAULT_TRISTATE    0   /* Outputs tri-stated (same as LOW with PENH/PENL cleared) */

/* PWM alignment modes */
#define HRPWM_EDGE_ALIGNED      0   /* Standard edge-aligned (MODSEL=0b000) */
#define HRPWM_CENTER_ALIGNED    4   /* Center-aligned (MODSEL=0b100) */

/* Channel range */
#define HRPWM_CH_MIN            1
#define HRPWM_CH_MAX            8

/* Library API struct (dot-notation via function pointers) */
typedef struct {
    /* Core control */
    void (*begin)(uint8_t channel, uint32_t freq_hz);
    void (*stop)(uint8_t channel);
    void (*duty)(uint8_t channel, uint16_t val16);
    void (*dutyRaw)(uint8_t channel, uint16_t raw_dc);
    uint16_t (*getPeriod)(uint8_t channel);

    /* Configuration */
    void (*setMode)(uint8_t channel, uint8_t mode);
    void (*setAlignment)(uint8_t channel, uint8_t alignment);
    void (*setDeadTime)(uint8_t channel, uint16_t rise_ns, uint16_t fall_ns);
    void (*setPhase)(uint8_t channel, uint16_t phase_ticks);

    /* ADC trigger */
    void (*setADCTrigger)(uint8_t channel, uint8_t trig_source, uint16_t compare_val);

    /* Fault protection */
    void (*enableFault)(uint8_t channel, uint8_t fault_action);
    void (*clearFault)(uint8_t channel);
    uint8_t (*isFaulted)(uint8_t channel);

    /* Status */
    uint8_t (*isRunning)(uint8_t channel);
} HRPWMClass_t;

extern HRPWMClass_t HRPWM;

#ifdef __cplusplus
}
#endif

#endif /* HRPWM_H */
