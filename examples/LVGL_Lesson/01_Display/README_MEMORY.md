# Memory Configuration for LVGL on STM32F446RE

## Hardware Constraints
- **Total RAM**: 128 KB
- **Total Flash**: 512 KB

## Memory Allocations

### FreeRTOS Configuration
**File**: `Core/Inc/FreeRTOSConfig.h`

```c
#define configTOTAL_HEAP_SIZE    ((size_t)(32 * 1024))  // 32 KB FreeRTOS heap
#define configMINIMAL_STACK_SIZE ((uint16_t)128)         // 512 bytes minimum stack
```

### LVGL Configuration  
**File**: `Drivers/lv_conf.h`

```c
#define LV_MEM_SIZE (16 * 1024U)  // 16 KB LVGL internal heap
```

### LVGL Task Stack
**File**: `Core/Src/freertos.c`

```c
const osThreadAttr_t lvglTask_attributes = {
  .name = "lvglTask",
  .stack_size = 2048 * 4,  // 8 KB stack for LVGL task
  .priority = (osPriority_t) osPriorityLow,
};
```

### Linker Configuration
**File**: `STM32F446XX_FLASH.ld`

```c
_Min_Heap_Size = 0x200;   // 512 bytes minimum heap
_Min_Stack_Size = 0x1000; // 4 KB main stack (MSP)
```

### LVGL Display Buffers
**File**: `Drivers/ILI9341/LCDController.c`

```c
// Two buffers for partial rendering (320x10 pixels each)
static uint8_t buf_2_1[MY_DISP_HOR_RES * 10 * BYTE_PER_PIXEL];  // 6.4 KB
static uint8_t buf_2_2[MY_DISP_HOR_RES * 10 * BYTE_PER_PIXEL];  // 6.4 KB
// Total: 12.8 KB
```

## Total RAM Usage Summary

| Component | Size | Location |
|-----------|------|----------|
| FreeRTOS Heap | 32 KB | configTOTAL_HEAP_SIZE |
| LVGL Heap | 16 KB | LV_MEM_SIZE |
| Display Buffers | 12.8 KB | Static buffers in LCDController.c |
| LVGL Task Stack | 8 KB | lvglTask stack_size |
| Main Stack (MSP) | 4 KB | _Min_Stack_Size |
| Other Task Stacks | ~3 KB | normalTask, readAndLoadTime, etc. |
| **Total** | **~76 KB** | **Fits within 128 KB** |

## Key Tasks

| Task Name | Stack Size | Priority | Purpose |
|-----------|------------|----------|---------|
| lvglTask | 8 KB | osPriorityLow | LVGL timer handler (rendering) |
| readAndLoadTime | 1 KB | osPriorityLow3 | Read RTC and queue data |
| dmaUart | 1 KB | osPriorityLow2 | Send UART messages via DMA |
| updateTimeGui | 1 KB | osPriorityLow7 | Update time/date labels |
| normalTask | 512 bytes | osPriorityNormal | User task |
| belowNormalTask | 512 bytes | osPriorityBelowNormal | LED blink, debug counter |

## Important Notes

1. **FPU Enabled**: `configENABLE_FPU = 1` required for ARM Cortex-M4F
2. **FreeRTOS Port**: Using `ARM_CM4F` port (hardware FP support)
3. **LVGL OS Integration**: `LV_USE_OS = LV_OS_CMSIS_RTOS2`
4. **Color Depth**: `LV_COLOR_DEPTH = 16` (RGB565, 2 bytes per pixel)
5. **Tick Source**: LVGL uses 1ms timer (`lvglTimerHandle`) for `lv_tick_inc(1)`

## Troubleshooting

- **Linker Error "region RAM overflowed"**: Reduce heap sizes or buffer sizes
- **Stack Overflow**: Increase task stack_size or enable `configCHECK_FOR_STACK_OVERFLOW`
- **LVGL Out of Memory**: Increase `LV_MEM_SIZE` or simplify GUI
- **Build > 128 KB RAM**: Check `.bss` section - may need to use external SRAM
