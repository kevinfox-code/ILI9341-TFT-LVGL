#include "drv/xpt2046.h"
#include <string.h>

#ifndef XPT2046_MAX_INSTANCES
#define XPT2046_MAX_INSTANCES 1
#endif

/* Command bytes: address + 12-bit mode + differential + PD1PD0=00 (power
 * down between conversions, PENIRQ enabled). Both already end in 0b00, so
 * every transaction in the sampling burst below leaves PENIRQ re-armed —
 * no extra "power down" transaction is required. */
#define CMD_X 0xD0u
#define CMD_Y 0x90u

struct xpt2046 {
    bool in_use;
    drv_spi_bus_t *bus;
    drv_gpio_t cs;
    drv_gpio_t irq;

    drv_mutex_t cal_mutex;
    uint16_t raw_x_min, raw_x_max, raw_y_min, raw_y_max;

    xpt2046_pen_cb_t pen_cb;
    void *pen_cb_user;
};

static struct xpt2046 s_instances[XPT2046_MAX_INSTANCES];

volatile xpt2046_debug_t g_xpt2046_debug;

static uint16_t spi_xfer(xpt2046_t *t, uint8_t cmd)
{
    uint8_t tx[3] = { cmd, 0, 0 };
    uint8_t rx[3] = { 0, 0, 0 };
    drv_spi_txrx(t->bus, tx, rx, 3, DRV_TOUCH_BUS_TIMEOUT_MS);
    return (uint16_t)((((uint16_t)rx[1] << 8) | rx[2]) >> 4);
}

static uint16_t clamp_u16(uint16_t v, uint16_t lo, uint16_t hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

drv_status_t XPT2046_Init(xpt2046_t **out, const xpt2046_config_t *cfg)
{
    if (out == NULL || cfg == NULL || cfg->bus == NULL) {
        return DRV_ERR_PARAM;
    }
    if (cfg->raw_x_min >= cfg->raw_x_max || cfg->raw_y_min >= cfg->raw_y_max) {
        return DRV_ERR_PARAM;
    }

    xpt2046_t *t = NULL;
    for (int i = 0; i < XPT2046_MAX_INSTANCES; i++) {
        if (!s_instances[i].in_use) {
            t = &s_instances[i];
            break;
        }
    }
    if (t == NULL) {
        return DRV_ERR_STATE;
    }

    memset(t, 0, sizeof(*t));
    t->in_use = true;
    t->bus = cfg->bus;
    t->cs = cfg->cs;
    t->irq = cfg->irq;
    t->raw_x_min = cfg->raw_x_min;
    t->raw_x_max = cfg->raw_x_max;
    t->raw_y_min = cfg->raw_y_min;
    t->raw_y_max = cfg->raw_y_max;
    t->cal_mutex = drv_mutex_create("xpt_cal");

    drv_gpio_write(t->cs, true);

    *out = t;
    return DRV_OK;
}

bool XPT2046_PenDown(xpt2046_t *t)
{
    if (t == NULL || t->irq.port == NULL) {
        return false;
    }
    /* PENIRQ is active-low: true when a finger is present. */
    return !drv_gpio_read(t->irq);
}

bool XPT2046_ReadRaw(xpt2046_t *t, uint16_t *rx, uint16_t *ry)
{
    if (t == NULL || rx == NULL || ry == NULL) {
        return false;
    }
    if (t->irq.port != NULL && !XPT2046_PenDown(t)) {
        return false;
    }

    if (drv_spi_bus_lock(t->bus, DRV_TOUCH_BUS_TIMEOUT_MS) != DRV_OK) {
        return false;
    }

    drv_gpio_write(t->cs, false);
    /* Discard first read of each axis (settling time), then average two. */
    (void)spi_xfer(t, CMD_X);
    uint16_t x1 = spi_xfer(t, CMD_X);
    uint16_t x2 = spi_xfer(t, CMD_X);
    (void)spi_xfer(t, CMD_Y);
    uint16_t y1 = spi_xfer(t, CMD_Y);
    uint16_t y2 = spi_xfer(t, CMD_Y);
    drv_gpio_write(t->cs, true);

    drv_spi_bus_unlock(t->bus);

    *rx = (uint16_t)((x1 + x2) >> 1);
    *ry = (uint16_t)((y1 + y2) >> 1);
    return true;
}

bool XPT2046_ReadPoint(xpt2046_t *t, uint16_t *px, uint16_t *py,
                       uint16_t hor_res, uint16_t ver_res, uint8_t rotation)
{
    if (t == NULL || px == NULL || py == NULL || hor_res == 0u || ver_res == 0u || rotation > 3u) {
        return false;
    }

    g_xpt2046_debug.read_count++;

    uint16_t rx, ry;
    if (!XPT2046_ReadRaw(t, &rx, &ry)) {
        g_xpt2046_debug.pen_down = 0;
        return false;
    }
    g_xpt2046_debug.pen_down = 1;
    g_xpt2046_debug.press_count++;
    g_xpt2046_debug.raw_x = rx;
    g_xpt2046_debug.raw_y = ry;

    drv_mutex_lock(t->cal_mutex, DRV_OS_WAIT_FOREVER);
    uint16_t xmin = t->raw_x_min, xmax = t->raw_x_max;
    uint16_t ymin = t->raw_y_min, ymax = t->raw_y_max;
    drv_mutex_unlock(t->cal_mutex);

    rx = clamp_u16(rx, xmin, xmax);
    ry = clamp_u16(ry, ymin, ymax);

    float fx = (float)(rx - xmin) / (float)(xmax - xmin);
    float fy = (float)(ry - ymin) / (float)(ymax - ymin);

    /* rotation 3 (landscape, MX|MY|MV) is the empirically-validated legacy
     * mapping: pixel-x from raw Y, pixel-y from raw X, no inversion. Other
     * rotations are derived from it by successive 90 deg CW rotations of
     * the normalized touch point, since the physical touch panel axes are
     * fixed regardless of display rotation. */
    float u = fy;
    float v = fx;
    uint8_t steps = (uint8_t)((rotation + 1u) % 4u);
    for (uint8_t i = 0; i < steps; i++) {
        float nu = 1.0f - v;
        float nv = u;
        u = nu;
        v = nv;
    }

    int32_t x = (int32_t)(u * (float)(hor_res - 1u) + 0.5f);
    int32_t y = (int32_t)(v * (float)(ver_res - 1u) + 0.5f);
    if (x < 0) x = 0;
    if (x > (int32_t)(hor_res - 1u)) x = (int32_t)(hor_res - 1u);
    if (y < 0) y = 0;
    if (y > (int32_t)(ver_res - 1u)) y = (int32_t)(ver_res - 1u);

    *px = (uint16_t)x;
    *py = (uint16_t)y;
    g_xpt2046_debug.px = *px;
    g_xpt2046_debug.py = *py;
    return true;
}

void XPT2046_SetCalibration(xpt2046_t *t, uint16_t xmin, uint16_t xmax,
                            uint16_t ymin, uint16_t ymax)
{
    if (t == NULL || xmin >= xmax || ymin >= ymax) {
        return;
    }
    drv_mutex_lock(t->cal_mutex, DRV_OS_WAIT_FOREVER);
    t->raw_x_min = xmin;
    t->raw_x_max = xmax;
    t->raw_y_min = ymin;
    t->raw_y_max = ymax;
    drv_mutex_unlock(t->cal_mutex);
}

void XPT2046_GetCalibration(xpt2046_t *t, uint16_t *xmin, uint16_t *xmax,
                            uint16_t *ymin, uint16_t *ymax)
{
    if (t == NULL) {
        return;
    }
    drv_mutex_lock(t->cal_mutex, DRV_OS_WAIT_FOREVER);
    if (xmin) *xmin = t->raw_x_min;
    if (xmax) *xmax = t->raw_x_max;
    if (ymin) *ymin = t->raw_y_min;
    if (ymax) *ymax = t->raw_y_max;
    drv_mutex_unlock(t->cal_mutex);
}

void XPT2046_PenIRQ_ISR(xpt2046_t *t)
{
    if (t == NULL) {
        return;
    }
    if (t->pen_cb != NULL) {
        t->pen_cb(t->pen_cb_user);
    }
}

void XPT2046_SetPenDownCallback(xpt2046_t *t, xpt2046_pen_cb_t cb, void *user)
{
    if (t == NULL) {
        return;
    }
    t->pen_cb = cb;
    t->pen_cb_user = user;
}
