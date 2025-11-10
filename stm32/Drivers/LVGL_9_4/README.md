# LVGL 9.4 Drivers for STM32

This directory contains LVGL 9.4-compatible drivers for STM32 microcontrollers, providing hardware acceleration and display controller integration for embedded applications.

## 📁 Directory Structure

```
LVGL_9_4/
├── LTDC/                    # LCD-TFT Display Controller driver
│   ├── lv_st_ltdc.h
│   └── lv_st_ltdc.c
├── DMA2D/                   # Chrom-ART hardware accelerator
│   ├── lv_draw_dma2d.h
│   └── lv_draw_dma2d.c
├── NeoChrom/                # NeoChrom GPU (STM32U5+)
│   ├── lv_nema_gfx.h
│   └── lv_nema_gfx.c
├── SPI_Display/             # SPI TFT display driver template
│   ├── lv_st_spi_display.h
│   └── lv_st_spi_display.c
├── lv_drv_conf_template.h   # Driver configuration template
└── README.md                # This file
```

## 🚀 Quick Start

### 1. Prerequisites

- **LVGL 9.4** integrated into your project
- **STM32CubeIDE** or **STM32CubeMX** for peripheral configuration
- **STM32 HAL** libraries
- Configured `lv_conf.h` file

### 2. Basic Integration Steps

1. **Copy drivers to your project**
   ```
   Copy this LVGL_9_4 folder to your project's drivers directory
   ```

2. **Create `lv_drv_conf.h`**
   ```
   cp lv_drv_conf_template.h lv_drv_conf.h
   ```
   Edit `lv_drv_conf.h` to enable the drivers you need.

3. **Add include paths in STM32CubeIDE**
   - Right-click project → Properties
   - C/C++ Build → Settings → MCU GCC Compiler → Include paths
   - Add paths to each driver subdirectory you're using

4. **Include driver headers in your code**
   ```c
   #include "lv_drv_conf.h"
   #include "LTDC/lv_st_ltdc.h"  // If using LTDC
   // or
   #include "SPI_Display/lv_st_spi_display.h"  // If using SPI
   ```

5. **Initialize LVGL and drivers**
   ```c
   lv_init();
   lv_tick_set_cb(HAL_GetTick);
   
   // Create display (see driver-specific examples below)
   lv_display_t * disp = lv_st_ltdc_create_direct(...);
   ```

## 📺 Driver Documentation

### LTDC (LCD-TFT Display Controller)

**Supported STM32 families:** F4, F7, H7, U5, etc. (models with LTDC peripheral)

**Features:**
- Direct mode (single/double buffered)
- Partial render mode (memory efficient)
- Multi-layer support
- Hardware rotation support (partial mode only)
- DMA2D-accelerated flushing (partial mode)

**Configuration:**
```c
// In lv_drv_conf.h
#define LV_USE_ST_LTDC 1
#define LV_ST_LTDC_USE_DMA2D_FLUSH 0  // Optional: enable DMA2D for flushing
```

**Usage Example - Direct Mode:**
```c
// Define framebuffer address (must be in RAM and reserved in linker script)
void * fb_addr = (void *)0x20000000;
void * fb_addr2 = (void *)0x20096000;  // Optional second buffer

// Create display
lv_display_t * disp = lv_st_ltdc_create_direct(fb_addr, fb_addr2, 0);

// Main loop
while (1) {
    lv_timer_handler();
    HAL_Delay(2);
}
```

**Usage Example - Partial Mode:**
```c
// Allocate partial buffers (much smaller than full framebuffer)
static uint8_t buf1[65536];
static uint8_t buf2[65536];

// Create display (framebuffer address read from LTDC config)
lv_display_t * disp = lv_st_ltdc_create_partial(buf1, buf2, 65536, 0);

// Main loop
while (1) {
    lv_timer_handler();
    HAL_Delay(2);
}
```

**Linker Script Configuration:**
```ld
/* Reserve memory for framebuffer */
MEMORY
{
    FB_RAM (xrw)   : ORIGIN = 0x20000000, LENGTH = 1125K  /* 800x480 RGB888 */
    RAM    (xrw)   : ORIGIN = 0x20119400, LENGTH = 1883K
    FLASH  (rx)    : ORIGIN = 0x08000000, LENGTH = 4096K
}
```

---

### DMA2D (Chrom-ART Accelerator)

**Supported STM32 families:** F4, F7, H7, U5, etc. (models with DMA2D peripheral)

**Features:**
- Hardware-accelerated fill operations
- Image blitting with color format conversion
- Alpha blending
- Parallel operation with software renderer
- Interrupt-driven async mode (with RTOS)

**Configuration:**
```c
// In lv_drv_conf.h
#define LV_USE_DRAW_DMA2D 1
#define LV_DRAW_DMA2D_HAL_INCLUDE "stm32f4xx_hal.h"
#define LV_USE_DRAW_DMA2D_INTERRUPT 0  // Set to 1 for async mode

// In lv_conf.h (if using interrupts)
#define LV_USE_OS LV_OS_FREERTOS  // or LV_OS_CMSIS_RTOS2
```

**Usage Example:**
```c
// After lv_init() and display creation
lv_draw_dma2d_init();

// If using interrupts, add to your interrupt handler:
void DMA2D_IRQHandler(void)
{
    lv_draw_dma2d_transfer_complete_interrupt_handler();
}

// Main loop
while (1) {
    lv_timer_handler();
    HAL_Delay(2);
}
```

**Important Notes:**
- Cannot use `LV_USE_DRAW_DMA2D` and `LV_ST_LTDC_USE_DMA2D_FLUSH` simultaneously
- DMA2D and NeoChrom can coexist
- Works best with partial render mode

---

### NeoChrom GPU

**Supported STM32 families:** U5 (with NeoChrom core)

**Features:**
- Hardware 2D graphics acceleration
- Vector graphics support (VG)
- SVG rendering
- TSC compressed image format
- GPU tessellation (U5F/U5G models)

**Configuration:**
```c
// In lv_drv_conf.h
#define LV_USE_NEMA_GFX 1
#define LV_USE_NEMA_VG 1  // For vector graphics
#define LV_NEMA_GFX_MAX_RESX 800
#define LV_NEMA_GFX_MAX_RESY 480

// In lv_conf.h
#define LV_USE_VECTOR_GRAPHIC 1  // For VG support
#define LV_USE_SVG 1             // For SVG support
#define LV_USE_MATRIX 1
#define LV_USE_FLOAT 1
#define LV_CACHE_DEF_SIZE 65536  // Optional: cache SVG data
```

**STM32CubeIDE Linker Configuration:**

1. Right-click project → Properties
2. C/C++ Build → Settings → MCU G++ Linker → Libraries
3. Add library: `nemagfx-float-abi-hard`
4. Add library search path: `${workspace_loc:/${ProjName}/path/to/lvgl/libs/nema_gfx/lib/core/cortex_m33_revC/gcc}`
5. Under MCU GCC Compiler → Include paths
6. Add: `${workspace_loc:/${ProjName}/path/to/lvgl/libs/nema_gfx/include}`

**Usage Example:**
```c
// After lv_init() and display creation
lv_nema_gfx_init();
lv_nema_vg_init();  // If using vector graphics

// Now LVGL will automatically use NeoChrom for rendering

// Main loop
while (1) {
    lv_timer_handler();
    HAL_Delay(2);
}
```

**TSC Image Support:**
```c
// Define TSC compressed image
const lv_image_dsc_t img_tsc = {
    .header.cf = LV_COLOR_FORMAT_NEMA_TSC6A,
    .header.w = 144,
    .header.h = 144,
    .data = img_data,
    .data_size = sizeof(img_data),
};

// Use in LVGL
lv_img_set_src(img_widget, &img_tsc);
```

---

### SPI Display Driver

**Supported displays:** ILI9341, ST7735, ST7789, and similar SPI TFT displays

**Features:**
- STM32 HAL SPI integration
- RGB565 color format
- Direct and partial render modes
- Optional DMA support
- Hardware-specific initialization
- Orientation control

**Configuration:**
```c
// In lv_drv_conf.h
#define LV_USE_ST_SPI_DISPLAY 1
#define SPI_DISPLAY_SPI_HANDLE hspi1

// GPIO pin definitions (modify for your hardware)
#define LCD_CS_GPIO_Port GPIOA
#define LCD_CS_Pin GPIO_PIN_4
#define LCD_DC_GPIO_Port GPIOA
#define LCD_DC_Pin GPIO_PIN_3
#define LCD_RST_GPIO_Port GPIOA
#define LCD_RST_Pin GPIO_PIN_2

#define SPI_DISPLAY_WIDTH  240
#define SPI_DISPLAY_HEIGHT 320
#define SPI_DISPLAY_USE_DMA 1  // Recommended for performance
```

**STM32CubeMX SPI Configuration:**
- Mode: Full-Duplex Master
- Hardware NSS: Disabled (manual CS control)
- Data Size: 8 Bits
- Clock: As fast as display supports (typically 18-42 MHz)
- DMA: Enable TX DMA if using `SPI_DISPLAY_USE_DMA`

**Usage Example:**
```c
// Create display
bool direct_mode = false;  // Use partial mode for memory efficiency
lv_display_t * disp = lv_st_spi_display_create(240, 320, direct_mode);

// Optional: set orientation
lv_st_spi_display_set_orientation(disp, SPI_DISPLAY_ORIENTATION_LANDSCAPE);

// Main loop
while (1) {
    lv_timer_handler();
    HAL_Delay(2);
}
```

**Adapting for Other Display Controllers:**

The SPI driver is a template. To adapt it for your specific display:

1. Modify initialization commands in `lv_st_spi_display_hw_init()`
2. Update command definitions (e.g., replace `ILI9341_*` constants)
3. Adjust MADCTL register values in `lv_st_spi_display_set_orientation()`

---

## 🔧 Advanced Configuration

### Using with RTOS

If using FreeRTOS or CMSIS-RTOS2:

```c
// In lv_conf.h
#define LV_USE_OS LV_OS_FREERTOS  // or LV_OS_CMSIS_RTOS2

// LVGL will use OS synchronization primitives automatically
```

### Memory Considerations

**LTDC Direct Mode:**
- RAM = Width × Height × Bytes_Per_Pixel
- Example: 800×480 RGB888 = 1,125 KB
- Double buffered = 2,250 KB

**LTDC Partial Mode:**
- RAM = Buffer_Lines × Width × Bytes_Per_Pixel
- Example: 40 lines × 800 × 2 (RGB565) = 64 KB
- Double buffered = 128 KB

**SPI Display:**
- Direct mode = same as LTDC direct
- Partial mode = typically 10-50 lines of buffer

### Performance Tips

1. **Enable hardware acceleration:** Use DMA2D or NeoChrom when available
2. **Use double buffering:** Reduces tearing and improves frame rate
3. **Optimize LVGL config:** Reduce `LV_MEM_SIZE` if tight on RAM
4. **Enable CPU cache:** Clean cache after framebuffer updates
5. **Use DMA for SPI:** Massive performance improvement for SPI displays

---

## 🐛 Troubleshooting

### Display shows garbage/corruption
- Check framebuffer address is correct and reserved in linker script
- Ensure LTDC pixel format matches LVGL color format
- Clean CPU data cache after rendering (`SCB_CleanDCache()`)

### DMA2D doesn't accelerate
- Verify DMA2D draw unit is initialized: `lv_draw_dma2d_init()`
- Check that tasks are simple (no gradients, transforms, etc.)
- Enable logging to see which draw unit handles tasks

### NeoChrom not working
- Verify libraries are linked correctly in project settings
- Check include paths include `nema_gfx/include`
- Ensure VG memory allocation is sufficient
- Check `LV_USE_MATRIX` and `LV_USE_FLOAT` are enabled

### SPI display is blank
- Check SPI clock speed (start slow, then increase)
- Verify GPIO pins are configured correctly
- Check CS/DC/RST pins are controlling properly
- Try different initialization sequences

### Compilation errors
- Ensure `lv_conf.h` is properly configured
- Check that `lv_drv_conf.h` exists and is included
- Verify STM32 HAL header path is correct
- Add all driver subdirectories to include paths

---

## 📚 Additional Resources

### Official Documentation
- [LVGL Documentation](https://docs.lvgl.io)
- [STM32 + LVGL Guide](https://docs.lvgl.io/master/integration/chip/stm32.html)
- [STM32CubeIDE Documentation](https://www.st.com/en/development-tools/stm32cubeide.html)

### Example Projects
- [lv_port_riverdi_stm32u5](https://github.com/lvgl/lv_port_riverdi_stm32u5)
- [lv_port_stm32u5g9j-dk2](https://github.com/lvgl/lv_port_stm32u5g9j_dk2)

### Community
- [LVGL Forum](https://forum.lvgl.io)
- [LVGL Discord](https://discord.gg/lvgl)

---

## 📄 License

These drivers follow the MIT license, same as LVGL.

---

## 🤝 Contributing

Contributions are welcome! If you've improved a driver or added support for new peripherals, please share your changes.

---

## ✨ Features Summary

| Driver | Hardware Acceleration | Vector Graphics | DMA Support | RTOS Support | Memory Efficient |
|--------|----------------------|-----------------|-------------|--------------|------------------|
| LTDC | ✅ Yes | ❌ No | ✅ Yes | ✅ Yes | ⚠️ Direct: No, Partial: Yes |
| DMA2D | ✅ Yes | ❌ No | ✅ Yes | ✅ Yes | ✅ Yes |
| NeoChrom | ✅✅ Best | ✅ Yes | ✅ Yes | ⚠️ Experimental | ✅ Yes |
| SPI Display | ❌ No | ❌ No | ✅ Optional | ✅ Yes | ⚠️ Partial: Yes |

---

**Compatibility:** LVGL 9.4+, STM32 HAL-based projects

**Tested On:** STM32F4, STM32F7, STM32H7, STM32U5 families

**Status:** Production Ready ✅
