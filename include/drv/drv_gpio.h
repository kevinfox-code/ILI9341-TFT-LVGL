#ifndef DRV_GPIO_H_
#define DRV_GPIO_H_
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* port is a HAL GPIO_TypeDef*, kept opaque here to stay HAL-free. */
typedef struct {
    void *port;
    uint16_t pin;
} drv_gpio_t;

void drv_gpio_write(drv_gpio_t g, bool level);
bool drv_gpio_read(drv_gpio_t g);

#ifdef __cplusplus
}
#endif
#endif /* DRV_GPIO_H_ */
