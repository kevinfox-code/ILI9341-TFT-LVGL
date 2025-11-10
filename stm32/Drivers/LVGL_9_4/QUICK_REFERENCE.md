# LVGL 9.4 STM32 Drivers - Quick Reference

## 📋 Quick Start Checklist

- [ ] Copy `LVGL_9_4` folder to your project
- [ ] Copy `lv_drv_conf_template.h` to `lv_drv_conf.h`
- [ ] Configure `lv_drv_conf.h` for your hardware
- [ ] Add driver directories to include paths
- [ ] Initialize LVGL and drivers in your code
- [ ] Create display and start rendering

## 🔌 Driver Selection Matrix

| Hardware | Driver | Configuration |
|----------|--------|---------------|
| LTDC + RGB LCD | `LTDC/lv_st_ltdc` | `LV_USE_ST_LTDC 1` |
| SPI TFT (ILI9341, ST7789) | `SPI_Display/lv_st_spi_display` | `LV_USE_ST_SPI_DISPLAY 1` |
| DMA2D Accelerator | `DMA2D/lv_draw_dma2d` | `LV_USE_DRAW_DMA2D 1` |
| NeoChrom GPU | `NeoChrom/lv_nema_gfx` | `LV_USE_NEMA_GFX 1` |

## 💾 Memory Requirements

### LTDC Direct Mode
```
Single Buffer:  Width × Height × BPP
Double Buffer:  Width × Height × BPP × 2

Example (800×480 RGB565):
  800 × 480 × 2 = 750 KB (single)
  800 × 480 × 2 × 2 = 1500 KB (double)
```

### LTDC/SPI Partial Mode
```
Buffer: Width × Lines × BPP

Example (800 width, 40 lines, RGB565):
  800 × 40 × 2 = 62.5 KB per buffer
  Double buffered = 125 KB total
```

## ⚡ Performance Comparison

| Configuration | Frame Rate | CPU Usage | Memory |
|--------------|------------|-----------|---------|
| LTDC Direct + DMA2D | 60+ FPS | Low | High |
| LTDC Partial + DMA2D | 30-60 FPS | Medium | Low |
| LTDC + NeoChrom | 60+ FPS | Very Low | Low-Medium |
| SPI + DMA | 15-30 FPS | Medium | Low |
| SPI (no DMA) | 5-15 FPS | High | Low |

## 🎯 Common Configurations

### High-End (STM32F7/H7/U5)
```c
#define LV_USE_ST_LTDC 1
#define LV_USE_DRAW_DMA2D 1  // or LV_USE_NEMA_GFX for U5
#define LV_USE_OS LV_OS_FREERTOS

// Direct mode with double buffering
lv_st_ltdc_create_direct(fb1, fb2, 0);
```

### Mid-Range (STM32F4)
```c
#define LV_USE_ST_LTDC 1
#define LV_USE_DRAW_DMA2D 1

// Partial mode with DMA2D flush
lv_st_ltdc_create_partial(buf1, buf2, 65536, 0);
```

### Low-End (STM32F1/F0)
```c
#define LV_USE_ST_SPI_DISPLAY 1
#define SPI_DISPLAY_USE_DMA 1

// Partial mode
lv_st_spi_display_create(240, 320, false);
```

## 🔧 Essential lv_conf.h Settings

```c
/* Enable LVGL */
#define LV_COLOR_DEPTH 16  // or 32 for ARGB8888

/* Memory */
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (64 * 1024)  // Adjust as needed

/* Tick source */
// Call lv_tick_set_cb(HAL_GetTick) in code

/* OS support (optional) */
#define LV_USE_OS LV_OS_NONE  // or LV_OS_FREERTOS

/* Features */
#define LV_USE_ASSERT_NULL 1
#define LV_USE_ASSERT_MALLOC 1
#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
```

## 🐛 Troubleshooting Quick Fixes

### Display shows nothing
```c
// Check 1: Verify display creation succeeded
if (disp == NULL) Error_Handler();

// Check 2: Ensure lv_timer_handler() is called
while(1) {
    lv_timer_handler();
    HAL_Delay(2);
}

// Check 3: Create something visible
lv_obj_t * label = lv_label_create(lv_screen_active());
lv_label_set_text(label, "Test");
lv_obj_center(label);
```

### Display corrupted/garbage
```c
// For LTDC: Clean data cache after rendering
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
    SCB_CleanDCache();
#endif

// Check framebuffer address matches linker script
// Check LTDC color format matches LVGL color format
```

### Slow performance
```c
// Enable hardware acceleration
#define LV_USE_DRAW_DMA2D 1

// Use double buffering
lv_st_ltdc_create_direct(fb1, fb2, 0);

// Reduce display resolution or use partial mode
lv_st_ltdc_create_partial(buf1, buf2, size, 0);
```

### Build errors
```c
// Add driver directories to include paths
// Ensure lv_drv_conf.h exists
// Check LV_STM32_HAL_INCLUDE matches your STM32 family
```

## 📞 Initialization Call Order

```c
/* 1. HAL and peripherals */
HAL_Init();
SystemClock_Config();
MX_GPIO_Init();
MX_LTDC_Init();  // or MX_SPI_Init()

/* 2. LVGL core */
lv_init();
lv_tick_set_cb(HAL_GetTick);

/* 3. Display driver */
disp = lv_st_ltdc_create_direct(...);

/* 4. Hardware acceleration (optional) */
lv_draw_dma2d_init();
// or
lv_nema_gfx_init();

/* 5. Touch input (optional) */
// Setup touch indev

/* 6. Create UI */
create_ui();

/* 7. Main loop */
while(1) {
    lv_timer_handler();
    HAL_Delay(2);
}
```

## 🔗 Key API Functions

### Display Creation
```c
// LTDC
lv_display_t * lv_st_ltdc_create_direct(void *fb1, void *fb2, uint32_t layer);
lv_display_t * lv_st_ltdc_create_partial(void *buf1, void *buf2, size_t size, uint32_t layer);

// SPI
lv_display_t * lv_st_spi_display_create(uint16_t w, uint16_t h, bool direct);
void lv_st_spi_display_set_orientation(lv_display_t *disp, spi_display_orientation_t orient);
```

### Hardware Acceleration
```c
// DMA2D
void lv_draw_dma2d_init(void);
void lv_draw_dma2d_transfer_complete_interrupt_handler(void);  // In ISR

// NeoChrom
void lv_nema_gfx_init(void);
void lv_nema_vg_init(void);  // For vector graphics
```

### LVGL Core
```c
void lv_init(void);
void lv_tick_set_cb(uint32_t (*tick_get_cb)(void));
void lv_timer_handler(void);  // Call in main loop
lv_obj_t * lv_screen_active(void);
```

## 📖 Documentation Links

- Full Documentation: `README.md`
- Configuration: `lv_drv_conf_template.h`
- Integration Example: `lvgl_integration_example.c`
- LVGL Docs: https://docs.lvgl.io
- STM32 + LVGL Guide: https://docs.lvgl.io/master/integration/chip/stm32.html

## ⚖️ License

MIT License - Same as LVGL
