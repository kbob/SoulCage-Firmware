// This file's header
#include "driver_buzzer.h"

// C++ standard headers
#include <cstdio>

// ESP-IDF headers
#include "driver/gpio.h"

// Component headers
#include "board_defs.h"

// The buzzer will drain the battery unless we drive its GPIO low.

Buzzer::Buzzer()
{
#if defined(BUZZER_GPIO) && BUZZER_GPIO >= 0
    printf("Turning off buzzer\n");
    // Disable the buzzer.
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 1ull << BUZZER_GPIO;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    ESP_ERROR_CHECK(
        gpio_config(&io_conf)
    );
    ESP_ERROR_CHECK(
        gpio_set_level((gpio_num_t)BUZZER_GPIO, 0)
    );
#else
    printf("No buzzer configured\n");
#endif
}

Buzzer::~Buzzer()
{
#if defined(BUZZER_GPIO) && BUZZER_GPIO >= 0
    ESP_ERROR_CHECK(
        gpio_reset_pin((gpio_num_t)BUZZER_GPIO)
    );
#endif
}