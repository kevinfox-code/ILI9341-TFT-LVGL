#ifndef MOCK_GPIO_H_
#define MOCK_GPIO_H_
#include "drv/drv_gpio.h"
#include <stdint.h>

typedef struct {
    int seq;
    drv_gpio_t pin;
    bool level;
} mock_gpio_event_t;

void mock_gpio_reset(void);
int  mock_gpio_event_count(void);
const mock_gpio_event_t *mock_gpio_event(int index);

/* Convenience: index of the most recent write matching `pin`, or -1. */
int mock_gpio_last_write_index(drv_gpio_t pin);
bool mock_gpio_current_level(drv_gpio_t pin);

/* Test-only: drive a pin from "outside" (e.g. simulate PENIRQ) without going
 * through the driver's own drv_gpio_write calls. */
void mock_gpio_set_level(drv_gpio_t pin, bool level);

#endif /* MOCK_GPIO_H_ */
