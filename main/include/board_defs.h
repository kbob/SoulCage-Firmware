#pragma once

#include "packed_color.h"

#if defined(WAVESHARE_ESP32_S3_LCD_TOUCH_1_69)

    typedef PackedColorEE<EOrder::RGB565, PixelEndian::BIG> ScreenPixelType;

#elif defined(WAVESHARE_ESP32_S3_LCD_1_28)

    typedef PackedColorEE<EOrder::BGR565, PixelEndian::BIG> ScreenPixelType;

#else

    #error "No board defined"

#endif
