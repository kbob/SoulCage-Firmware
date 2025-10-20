// This file's header
#include "driver_backlight.h"

// C++ standard headers
#include <cmath>
#include <cstring>

// ESP-IDF headers
#include "driver/gpio.h"
#include "driver/ledc.h"

// Component headers
#include "board_defs.h"

struct Backlight_private {

    const int MAX_DUTY = (1 << 13) - 1;

    int m_gpio;
    float m_gamma;
    float m_brightness;
    int m_duty;
    ledc_mode_t m_speed_mode;
    ledc_timer_t m_timer;
    ledc_channel_t m_channel;

    Backlight_private(int gpio)
    : m_gpio(gpio),
      m_gamma(1.0f),
      m_brightness(0.0f),
      m_duty(0),
      m_speed_mode(LEDC_LOW_SPEED_MODE),
      m_timer(LEDC_TIMER_0),
      m_channel(LEDC_CHANNEL_0)
    {
        ledc_timer_config_t tc = {};
        tc.speed_mode = m_speed_mode;
        tc.duty_resolution = LEDC_TIMER_13_BIT;
        tc.timer_num = m_timer;
        tc.freq_hz = 4000;
        tc.clk_cfg = LEDC_AUTO_CLK;
        ESP_ERROR_CHECK(ledc_timer_config(&tc));

        ledc_channel_config_t cc = {};
        cc.speed_mode = m_speed_mode;
        cc.channel = m_channel;
        cc.timer_sel = m_timer;
        cc.intr_type = LEDC_INTR_DISABLE;
        cc.gpio_num = m_gpio;
        cc.duty = 0;
        cc.hpoint = 0;
        ESP_ERROR_CHECK(ledc_channel_config(&cc));
    }

    ~Backlight_private()
    {
        ESP_ERROR_CHECK(ledc_stop(m_speed_mode, m_channel, 0));
        ESP_ERROR_CHECK(ledc_timer_rst(m_speed_mode, m_timer));
    }

    void set_gamma(float gamma)
    {
        m_gamma = gamma;
        set_brightness(m_brightness);
    }

    void set_brightness(float brightness)
    {

        m_brightness = brightness;
        if (m_gamma == 1.0f) {
            m_duty = brightness * MAX_DUTY;
        } else {
            m_duty = powf(brightness, m_gamma) * MAX_DUTY;
        }
        ESP_ERROR_CHECK(ledc_set_duty(m_speed_mode, m_channel, m_duty));
        ESP_ERROR_CHECK(ledc_update_duty(m_speed_mode, m_channel));
    }
};

static Backlight_private the_implementation(BACKLIGHT_GPIO);


Backlight::Backlight(float brightness)
 : m_private(&the_implementation)
{
    set_brightness(brightness);
}

void Backlight::set_gamma(float gamma)
{
    m_private->m_gamma = gamma;
    set_brightness(m_private->m_brightness);
}

void Backlight::set_brightness(float brightness)
{
    m_private->set_brightness(brightness);
}
