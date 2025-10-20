#pragma once

#include "sdkconfig.h"
#include "packed_color.h"

#if defined(CONFIG_BOARD_WAVESHARE_ESP32_S3_LCD_TOUCH_1_69)

    const char *const board_name = "Waveshare ESP32-L3-LCD-Touch-1.69";

    typedef PackedColorEE<EOrder::RGB565, PixelEndian::BIG> ScreenPixelType;

    #define BACKLIGHT_GPIO 15
    #define BUZZER_GPIO    42

#elif defined(CONFIG_BOARD_WAVESHARE_ESP32_S3_LCD_1_28)

    const char *const board_name = "Waveshare ESP32-L3-LCD-1.28";

    // Undocumented, this board wants pixel data in BGR order.
    typedef PackedColorEE<EOrder::BGR565, PixelEndian::BIG> ScreenPixelType;

    #define BACKLIGHT_GPIO 40
    #define BUZZER_GPIO   (-1)

#else

    #error "No board defined"

#endif
