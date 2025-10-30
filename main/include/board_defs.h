#pragma once

#include "driver/gpio.h"
#include "sdkconfig.h"

const gpio_num_t UNDEFINED_GPIO = GPIO_NUM_MAX; // 49 on esp32s3

// BATTERY_ADC_R_ABOVE and R_BELOW are the resistor values of the
// battery ADC sensor's voltage divider.

#if defined(CONFIG_BOARD_WAVESHARE_ESP32_S3_LCD_TOUCH_1_69)

    const char *const board_name = "Waveshare ESP32-S3-LCD-Touch-1.69";

    #define DISPLAY_HEIGHT          280
    #define DISPLAY_WIDTH           240
    #define DISPLAY_PIXEL_ORDER     EOrder::RGB565
    #define DISPLAY_PIXEL_ENDIAN    PixelEndian::BIG
    #define DISPLAY_SPI_HOST        SPI2_HOST
    #define DISPLAY_SPI_CLOCK_SPEED 80'000'000
    #define DISPLAY_CTLR            ST7789v2_controller
    #define DISPLAY_SCLK_GPIO       GPIO_NUM_6
    #define DISPLAY_PICO_GPIO       GPIO_NUM_7
    #define DISPLAY_CS_GPIO         GPIO_NUM_5
    #define DISPLAY_DC_GPIO         GPIO_NUM_4
    #define DISPLAY_RESET_GPIO      GPIO_NUM_8

    #define BACKLIGHT_GPIO          GPIO_NUM_15

    #define BATTERY_ADC_GPIO        GPIO_NUM_1
    #define BATTERY_ADC_R_ABOVE     200e3f  /* per schematic */
    #define BATTERY_ADC_R_BELOW     100e3f

    #define BUZZER_GPIO             GPIO_NUM_42

#elif defined(CONFIG_BOARD_WAVESHARE_ESP32_S3_LCD_1_28)

    const char *const board_name = "Waveshare ESP32-S3-LCD-1.28";

    #define DISPLAY_HEIGHT          240
    #define DISPLAY_WIDTH           240
    #define DISPLAY_PIXEL_ORDER     EOrder::BGR565
    #define DISPLAY_PIXEL_ENDIAN    PixelEndian::BIG
    #define DISPLAY_SPI_HOST        SPI2_HOST
    #define DISPLAY_SPI_CLOCK_SPEED 80'000'000
    #define DISPLAY_CTLR            GC9A01_controller
    #define DISPLAY_SCLK_GPIO       GPIO_NUM_10
    #define DISPLAY_PICO_GPIO       GPIO_NUM_11
    #define DISPLAY_CS_GPIO         GPIO_NUM_9
    #define DISPLAY_DC_GPIO         GPIO_NUM_8
    #define DISPLAY_RESET_GPIO      GPIO_NUM_12

    #define BACKLIGHT_GPIO          GPIO_NUM_40

    #define BATTERY_ADC_GPIO        GPIO_NUM_1
    #define BATTERY_ADC_R_ABOVE     200e3f /* XXX Schematic says 100K, 100K; */
    #define BATTERY_ADC_R_BELOW     80e3f  /*     meter says 200K, 80K */

    #define BUZZER_GPIO             UNDEFINED_GPIO

#else

    #error "No board defined"

#endif
