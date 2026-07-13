#include "mock_gpio.h"
#include "mock_seq.h"
#include <string.h>

#ifndef MOCK_GPIO_MAX_EVENTS
#define MOCK_GPIO_MAX_EVENTS 4096
#endif

static mock_gpio_event_t s_events[MOCK_GPIO_MAX_EVENTS];
static int s_event_count = 0;

static bool same_pin(drv_gpio_t a, drv_gpio_t b)
{
    return a.port == b.port && a.pin == b.pin;
}

void mock_gpio_reset(void)
{
    memset(s_events, 0, sizeof(s_events));
    s_event_count = 0;
}

int mock_gpio_event_count(void)
{
    return s_event_count;
}

const mock_gpio_event_t *mock_gpio_event(int index)
{
    if (index < 0 || index >= s_event_count) {
        return NULL;
    }
    return &s_events[index];
}

int mock_gpio_last_write_index(drv_gpio_t pin)
{
    for (int i = s_event_count - 1; i >= 0; i--) {
        if (same_pin(s_events[i].pin, pin)) {
            return i;
        }
    }
    return -1;
}

bool mock_gpio_current_level(drv_gpio_t pin)
{
    int idx = mock_gpio_last_write_index(pin);
    return (idx >= 0) ? s_events[idx].level : false;
}

static void record_write(drv_gpio_t g, bool level)
{
    if (s_event_count < MOCK_GPIO_MAX_EVENTS) {
        s_events[s_event_count].seq = mock_next_seq();
        s_events[s_event_count].pin = g;
        s_events[s_event_count].level = level;
        s_event_count++;
    }
}

void drv_gpio_write(drv_gpio_t g, bool level)
{
    record_write(g, level);
}

void mock_gpio_set_level(drv_gpio_t pin, bool level)
{
    record_write(pin, level);
}

bool drv_gpio_read(drv_gpio_t g)
{
    return mock_gpio_current_level(g);
}
