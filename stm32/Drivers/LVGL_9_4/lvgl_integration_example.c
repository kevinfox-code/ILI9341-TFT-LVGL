/**
 * @file lvgl_integration_example.c
 * @brief Complete LVGL 9.4 Integration Example for STM32
 * 
 * This file demonstrates how to integrate LVGL 9.4 with STM32 using the provided drivers.
 * Choose the appropriate driver section for your hardware.
 */

/*********************
 *      INCLUDES
 *********************/
#include "main.h"
#include "lvgl.h"
#include "lv_drv_conf.h"

/* Include the display driver you're using */
#if LV_USE_ST_LTDC
    #include "LTDC/lv_st_ltdc.h"
#elif LV_USE_ST_SPI_DISPLAY
    #include "SPI_Display/lv_st_spi_display.h"
#endif

/* Include optional hardware acceleration */
#if LV_USE_DRAW_DMA2D
    #include "DMA2D/lv_draw_dma2d.h"
#endif

#if LV_USE_NEMA_GFX
    #include "NeoChrom/lv_nema_gfx.h"
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_display_t * disp;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lvgl_init_display(void);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * Initialize LVGL and all drivers
 * Call this once at startup, after HAL_Init() and peripheral initialization
 */
void lvgl_setup(void)
{
    /* Initialize LVGL */
    lv_init();
    
    /* Set tick callback (LVGL needs to know elapsed time) */
    lv_tick_set_cb(HAL_GetTick);
    
    /* Initialize display */
    lvgl_init_display();
    
    /* Initialize hardware acceleration if available */
#if LV_USE_DRAW_DMA2D
    lv_draw_dma2d_init();
#endif

#if LV_USE_NEMA_GFX
    lv_nema_gfx_init();
    
    #if LV_USE_NEMA_VG
        lv_nema_vg_init();
    #endif
#endif
    
    /* Create your UI here or in a separate function */
    /* Example:
    lv_obj_t * label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello LVGL 9.4!");
    lv_obj_center(label);
    */
}

/**
 * LVGL task handler - call this in your main loop
 * Recommended interval: every 1-5 ms
 */
void lvgl_task_handler(void)
{
    lv_timer_handler();
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lvgl_init_display(void)
{
    /******************************************
     * OPTION 1: LTDC Display (Parallel RGB)
     ******************************************/
#if LV_USE_ST_LTDC
    
    /* Method A: Direct mode with double buffering (best quality, uses more RAM) */
    #if 0
        /* Define framebuffer addresses (must be reserved in linker script) */
        void * fb1 = (void *)0x20000000;
        void * fb2 = (void *)0x20096000;  /* Offset depends on display size */
        
        /* Create display on LTDC layer 0 */
        disp = lv_st_ltdc_create_direct(fb1, fb2, 0);
    #endif
    
    /* Method B: Partial mode (memory efficient, slightly slower) */
    #if 1
        /* Allocate partial buffers */
        static uint8_t buf1[65536];  /* Adjust size as needed */
        static uint8_t buf2[65536];  /* Second buffer optional but recommended */
        
        /* Create display on LTDC layer 0 */
        disp = lv_st_ltdc_create_partial(buf1, buf2, sizeof(buf1), 0);
    #endif
    
    /******************************************
     * OPTION 2: SPI Display
     ******************************************/
#elif LV_USE_ST_SPI_DISPLAY
    
    /* Create SPI display (partial mode recommended for memory efficiency) */
    bool use_direct = false;  /* false = partial mode, true = direct mode */
    disp = lv_st_spi_display_create(SPI_DISPLAY_WIDTH, SPI_DISPLAY_HEIGHT, use_direct);
    
    /* Optional: set display orientation */
    // lv_st_spi_display_set_orientation(disp, SPI_DISPLAY_ORIENTATION_LANDSCAPE);
    
#else
    #error "No display driver enabled! Enable LV_USE_ST_LTDC or LV_USE_ST_SPI_DISPLAY in lv_drv_conf.h"
#endif

    /* Check if display was created successfully */
    if (disp == NULL) {
        Error_Handler();  /* Your error handler */
    }
}

/**********************
 *   MAIN LOOP EXAMPLE
 **********************/

/**
 * Example main loop integration
 */
void example_main_loop(void)
{
    /* Initialize everything */
    HAL_Init();
    SystemClock_Config();
    
    /* Initialize peripherals (SPI, LTDC, GPIO, etc.) */
    MX_GPIO_Init();
    
    #if LV_USE_ST_LTDC
        MX_LTDC_Init();
    #endif
    
    #if LV_USE_ST_SPI_DISPLAY
        MX_SPI1_Init();  /* Or whatever your SPI handle is */
    #endif
    
    /* Initialize LVGL and drivers */
    lvgl_setup();
    
    /* Create your UI */
    create_ui();  /* Your UI creation function */
    
    /* Main loop */
    while (1)
    {
        /* Handle LVGL tasks */
        lvgl_task_handler();
        
        /* Small delay to prevent 100% CPU usage */
        /* Adjust based on your requirements */
        HAL_Delay(2);  /* 2ms = ~500 FPS max, good for most applications */
    }
}

/**********************
 *   RTOS INTEGRATION
 **********************/

#if LV_USE_OS

/**
 * Example FreeRTOS task for LVGL
 */
void lvgl_task(void * pvParameters)
{
    (void)pvParameters;
    
    /* Initialize LVGL (do this in the LVGL task or before task creation) */
    /* lvgl_setup(); */
    
    /* Infinite loop */
    while (1)
    {
        /* Handle LVGL tasks */
        lv_timer_handler();
        
        /* Yield to other tasks */
        vTaskDelay(pdMS_TO_TICKS(5));  /* 5ms delay */
    }
}

/**
 * Create LVGL task
 */
void create_lvgl_task(void)
{
    xTaskCreate(
        lvgl_task,              /* Task function */
        "LVGL",                 /* Task name */
        4096,                   /* Stack size (adjust as needed) */
        NULL,                   /* Parameters */
        osPriorityNormal,       /* Priority */
        NULL                    /* Task handle */
    );
}

#endif /* LV_USE_OS */

/**********************
 *   TOUCH INPUT EXAMPLE
 **********************/

/**
 * Example touch input integration
 * Create a separate file for touch driver implementation
 */
#if 0
static void touch_read_cb(lv_indev_t * indev, lv_indev_data_t * data)
{
    static int16_t last_x = 0;
    static int16_t last_y = 0;
    static bool is_pressed = false;
    
    /* Read touch controller (implement based on your hardware) */
    /* This is just a template */
    bool pressed = read_touch_pressed();  /* Your function */
    
    if (pressed) {
        last_x = read_touch_x();  /* Your function */
        last_y = read_touch_y();  /* Your function */
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
    
    data->point.x = last_x;
    data->point.y = last_y;
}

void lvgl_init_touch(void)
{
    lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touch_read_cb);
}
#endif

/**********************
 *   UI CREATION EXAMPLE
 **********************/

/**
 * Simple UI creation example
 */
void create_ui(void)
{
    /* Get the active screen */
    lv_obj_t * screen = lv_screen_active();
    
    /* Create a label */
    lv_obj_t * label = lv_label_create(screen);
    lv_label_set_text(label, "LVGL 9.4 on STM32");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 20);
    
    /* Create a button */
    lv_obj_t * btn = lv_button_create(screen);
    lv_obj_set_size(btn, 120, 50);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);
    
    /* Add label to button */
    lv_obj_t * btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Click Me");
    lv_obj_center(btn_label);
    
    /* Create a slider */
    lv_obj_t * slider = lv_slider_create(screen);
    lv_obj_set_width(slider, 200);
    lv_obj_align(slider, LV_ALIGN_BOTTOM_MID, 0, -40);
}

/**********************
 *   MEMORY MANAGEMENT
 **********************/

/**
 * Optional: Custom LVGL memory allocation using STM32 heap
 * Add these to lv_conf.h:
 * #define LV_USE_STDLIB_MALLOC LV_STDLIB_CUSTOM
 * #define LV_STDLIB_INCLUDE <stdlib.h>
 */

/**********************
 *   LOGGING
 **********************/

/**
 * Optional: Custom LVGL logging function
 * Redirects LVGL logs to UART or other interface
 */
#if LV_USE_LOG && 0

void lv_log_print_custom(lv_log_level_t level, const char * buf)
{
    /* Send to UART, USB, or debug interface */
    /* Example: HAL_UART_Transmit(&huart1, (uint8_t*)buf, strlen(buf), 100); */
    
    (void)level;
    (void)buf;
}

void lvgl_setup_logging(void)
{
    lv_log_register_print_cb(lv_log_print_custom);
}

#endif

/**********************
 *   PERFORMANCE MONITORING
 **********************/

/**
 * Monitor LVGL performance
 * Enable with LV_USE_PERF_MONITOR in lv_conf.h
 */
void show_performance_stats(void)
{
    #if LV_USE_PERF_MONITOR
        /* Performance monitor is automatically shown if enabled */
        /* You can customize its appearance or create custom stats */
    #endif
}
