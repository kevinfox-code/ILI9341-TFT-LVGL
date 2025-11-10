# LVGL 9.4 Drivers for STM32 - Index

## 📚 Documentation Files

1. **[README.md](README.md)** - Comprehensive documentation
   - Complete driver descriptions
   - Usage examples for each driver
   - Configuration instructions
   - Troubleshooting guide

2. **[QUICK_REFERENCE.md](QUICK_REFERENCE.md)** - Quick reference guide
   - Quick start checklist
   - Driver selection matrix
   - Common configurations
   - Essential API functions

3. **[lv_drv_conf_template.h](lv_drv_conf_template.h)** - Configuration template
   - Copy as `lv_drv_conf.h` to your project
   - Enable/disable drivers
   - Configure GPIO pins and peripherals

4. **[lvgl_integration_example.c](lvgl_integration_example.c)** - Integration examples
   - Complete initialization examples
   - RTOS integration
   - Touch input setup
   - UI creation examples

## 🗂️ Driver Files

### LTDC Driver (Parallel RGB Displays)
- **[LTDC/lv_st_ltdc.h](LTDC/lv_st_ltdc.h)** - Header file
- **[LTDC/lv_st_ltdc.c](LTDC/lv_st_ltdc.c)** - Implementation

**Use for:** STM32 with LTDC peripheral and parallel RGB LCD

### DMA2D Driver (Hardware Accelerator)
- **[DMA2D/lv_draw_dma2d.h](DMA2D/lv_draw_dma2d.h)** - Header file
- **[DMA2D/lv_draw_dma2d.c](DMA2D/lv_draw_dma2d.c)** - Implementation

**Use for:** Hardware-accelerated fills and blits on STM32 with DMA2D

### NeoChrom Driver (GPU)
- **[NeoChrom/lv_nema_gfx.h](NeoChrom/lv_nema_gfx.h)** - Header file
- **[NeoChrom/lv_nema_gfx.c](NeoChrom/lv_nema_gfx.c)** - Implementation

**Use for:** STM32U5 and similar with NeoChrom GPU for 2D/vector graphics

### SPI Display Driver (Serial TFT)
- **[SPI_Display/lv_st_spi_display.h](SPI_Display/lv_st_spi_display.h)** - Header file
- **[SPI_Display/lv_st_spi_display.c](SPI_Display/lv_st_spi_display.c)** - Implementation

**Use for:** SPI-based TFT displays (ILI9341, ST7735, ST7789, etc.)

## 🚀 Getting Started

1. Read [QUICK_REFERENCE.md](QUICK_REFERENCE.md) for quick setup
2. Copy [lv_drv_conf_template.h](lv_drv_conf_template.h) as `lv_drv_conf.h`
3. Enable the drivers you need in `lv_drv_conf.h`
4. Add driver directories to your include paths
5. Follow examples in [lvgl_integration_example.c](lvgl_integration_example.c)
6. Refer to [README.md](README.md) for detailed documentation

## 🎯 Driver Selection Guide

```
┌─────────────────────────────────────────────────┐
│         Do you have LTDC peripheral?            │
└────────────┬───────────────────────┬────────────┘
             │ Yes                   │ No
             ▼                       ▼
    ┌─────────────────┐    ┌─────────────────┐
    │  Use LTDC       │    │  Use SPI        │
    │  Driver         │    │  Display Driver │
    └────────┬────────┘    └─────────────────┘
             │
             ▼
    ┌─────────────────────────────────┐
    │  Do you have DMA2D or NeoChrom? │
    └────────┬─────────────┬──────────┘
             │ DMA2D       │ NeoChrom
             ▼             ▼
    ┌─────────────┐  ┌──────────────┐
    │  Enable     │  │  Enable      │
    │  DMA2D      │  │  NeoChrom    │
    └─────────────┘  └──────────────┘
```

## 📊 Feature Comparison

| Feature | LTDC | DMA2D | NeoChrom | SPI Display |
|---------|------|-------|----------|-------------|
| Display Interface | ✅ Yes | ❌ No | ❌ No | ✅ Yes |
| Hardware Acceleration | ❌ No | ✅ Yes | ✅✅ Best | ❌ No |
| Vector Graphics | ❌ No | ❌ No | ✅ Yes | ❌ No |
| Memory Efficiency | ⚠️ Depends | ✅ Yes | ✅ Yes | ✅ Yes |
| Performance | ⚡⚡ High | ⚡⚡ High | ⚡⚡⚡ Very High | ⚡ Low-Med |
| Setup Complexity | 🔧 Medium | 🔧 Easy | 🔧🔧 Hard | 🔧 Easy |

## 🔗 External Resources

- [LVGL Official Documentation](https://docs.lvgl.io)
- [LVGL STM32 Integration Guide](https://docs.lvgl.io/master/integration/chip/stm32.html)
- [STM32CubeIDE Download](https://www.st.com/en/development-tools/stm32cubeide.html)
- [LVGL Forum](https://forum.lvgl.io)
- [LVGL GitHub](https://github.com/lvgl/lvgl)

## 📝 Notes

- All drivers are compatible with LVGL 9.4+
- Tested on STM32F4, F7, H7, and U5 families
- Drivers use STM32 HAL library
- All lint errors for missing LVGL headers are expected (will resolve when integrated into project)

## 📄 License

MIT License - Same as LVGL core library

## 🤝 Support

For questions and issues:
1. Check [README.md](README.md) troubleshooting section
2. Review [QUICK_REFERENCE.md](QUICK_REFERENCE.md)
3. Search [LVGL Forum](https://forum.lvgl.io)
4. Check example projects listed in README.md

---

**Version:** 1.0.0  
**Compatible with:** LVGL 9.4+  
**Last Updated:** November 2025
