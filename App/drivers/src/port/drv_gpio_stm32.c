#include "drv/drv_gpio.h"
#include "drv/drv_config.h"

void drv_gpio_write(drv_gpio_t g, bool level)
{
    HAL_GPIO_WritePin((GPIO_TypeDef *)g.port, g.pin, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

bool drv_gpio_read(drv_gpio_t g)
{
    return HAL_GPIO_ReadPin((GPIO_TypeDef *)g.port, g.pin) == GPIO_PIN_SET;
}
