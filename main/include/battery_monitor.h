#pragma once

#include <cstdint>
#include <cstdio>
#include "esp_timer.h"
#include "driver_battery.h"

class BatteryMonitor {

public:
    BatteryMonitor(float period_sec)
    : m_period_usec(period_sec * 1'000'000.f),
      m_next_usec(period_sec * 1'000'000.f)
    {}

    void update()
    {
        int64_t now_usec = esp_timer_get_time();
        if (now_usec >= m_next_usec) {
            float next_sec = (float)m_next_usec / 1'000'000.f;
            printf("%g: battery level = %g V\n", next_sec, m_driver.voltage());
            m_next_usec += m_period_usec;
        }
    }

private:
    int64_t m_period_usec;
    int64_t m_next_usec;
    BatteryDriver m_driver;
};
