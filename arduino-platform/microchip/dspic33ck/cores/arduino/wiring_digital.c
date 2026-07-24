/*
 * wiring_digital.c - Digital I/O for dsPIC33CK Arduino core
 *
 * Implements: pinMode, digitalWrite, digitalRead
 * Uses the pin_map_t table from variant.c for port/bit lookup.
 */

#include "Arduino.h"

#ifdef __cplusplus
extern "C" {
#endif

void pinMode(uint8_t pin, uint8_t mode)
{
    if (pin >= NUM_DIGITAL_PINS) return;

    const pin_map_t *p = &g_pin_map[pin];

    /* If pin has analog capability, set it to digital mode */
    if (p->ansel_reg != NULL) {
        *(p->ansel_reg) &= ~(1U << p->bit);
    }

    switch (mode) {
        case INPUT:
            *(p->tris_reg) |= (1U << p->bit);   /* TRIS=1 -> input */
            /* Disable internal pull-up (CNPU register) */
            if (p->port_reg == &PORTB) {
                CNPUB &= ~(1U << p->bit);
            } else {
                CNPUA &= ~(1U << p->bit);
            }
            break;

        case INPUT_PULLUP:
            *(p->tris_reg) |= (1U << p->bit);   /* TRIS=1 -> input */
            /* Enable internal pull-up */
            if (p->port_reg == &PORTB) {
                CNPUB |= (1U << p->bit);
            } else {
                CNPUA |= (1U << p->bit);
            }
            break;

        case OUTPUT:
        default:
            *(p->tris_reg) &= ~(1U << p->bit);  /* TRIS=0 -> output */
            break;
    }
}

void digitalWrite(uint8_t pin, uint8_t val)
{
    if (pin >= NUM_DIGITAL_PINS) return;

    const pin_map_t *p = &g_pin_map[pin];

    if (val == HIGH) {
        *(p->lat_reg) |= (1U << p->bit);
    } else {
        *(p->lat_reg) &= ~(1U << p->bit);
    }
}

int digitalRead(uint8_t pin)
{
    if (pin >= NUM_DIGITAL_PINS) return LOW;

    const pin_map_t *p = &g_pin_map[pin];

    return (*(p->port_reg) & (1U << p->bit)) ? HIGH : LOW;
}

#ifdef __cplusplus
}
#endif
