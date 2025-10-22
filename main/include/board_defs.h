#pragma once

#include "driver/gpio.h"
#include "sdkconfig.h"
#include "packed_color.h"

const gpio_num_t UNDEFINED_GPIO = GPIO_NUM_MAX; // 49 on esp32s3

#if defined(CONFIG_BOARD_WAVESHARE_ESP32_S3_LCD_TOUCH_1_69)

    const char *const board_name = "Waveshare ESP32-L3-LCD-Touch-1.69";

    typedef PackedColorEE<EOrder::RGB565, PixelEndian::BIG> ScreenPixelType;

    #define DISPLAY_HEIGHT          280
    #define DISPLAY_WIDTH           240
    #define DISPLAY_SPI_HOST        SPI2_HOST
    #define DISPLAY_SPI_CLOCK_SPEED 80'000'000
    #define DISPLAY_CTLR            ST7789v2_controller
    #define DISPLAY_SCLK_GPIO       GPIO_NUM_6
    #define DISPLAY_PICO_GPIO       GPIO_NUM_7
    #define DISPLAY_CS_GPIO         GPIO_NUM_5
    #define DISPLAY_DC_GPIO         GPIO_NUM_4
    #define DISPLAY_RESET_GPIO      GPIO_NUM_8

    #define BACKLIGHT_GPIO          GPIO_NUM_15

    #define BUZZER_GPIO             GPIO_NUM_42

#elif defined(CONFIG_BOARD_WAVESHARE_ESP32_S3_LCD_1_28)

    const char *const board_name = "Waveshare ESP32-L3-LCD-1.28";

    // Undocumented, this board wants pixel data in BGR order.
    typedef PackedColorEE<EOrder::BGR565, PixelEndian::BIG> ScreenPixelType;

    #define DISPLAY_HEIGHT          240
    #define DISPLAY_WIDTH           240
    #define DISPLAY_SPI_HOST        SPI2_HOST
    #define DISPLAY_SPI_CLOCK_SPEED 80'000'000
    #define DISPLAY_CTLR            GC9A01_controller
    #define DISPLAY_SCLK_GPIO       GPIO_NUM_10
    #define DISPLAY_PICO_GPIO       GPIO_NUM_11
    #define DISPLAY_CS_GPIO         GPIO_NUM_9
    #define DISPLAY_DC_GPIO         GPIO_NUM_8
    #define DISPLAY_RESET_GPIO      GPIO_NUM_12

    #define BACKLIGHT_GPIO          GPIO_NUM_40

    #define NONSENSE (foo_42 + BazXX::YY)

    #define BUZZER_GPIO             UNDEFINED_GPIO

#else

    #error "No board defined"

#endif
