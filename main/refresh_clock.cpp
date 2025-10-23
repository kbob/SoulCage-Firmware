// This file's header
#include "refresh_clock.h"

// ESP-IDF headers
#include "freertos/FreeRTOS.h"

// Project headers
#include "sdkconfig.h"

struct RefreshClock_private {
    TimerHandle_t timer;
    TaskHandle_t task;

    // ticks_heuristic - find the whole number of ticks with the smallest error.
    TickType_t ticks_heuristic(float target_freq_hz)
    {
        // fast_ticks: round down for a faster clock
        // slow_ticks: round up for a slower clock
        float sys_freq = (float)CONFIG_FREERTOS_HZ;
        TickType_t fast_ticks = (TickType_t)(sys_freq / target_freq_hz);
        float fast_freq = sys_freq / fast_ticks;
        float fast_error = fast_freq / sys_freq; // >= 1.0
        TickType_t slow_ticks = fast_ticks + 1;
        float slow_freq = sys_freq / slow_ticks;
        float slow_error = sys_freq / slow_freq;

        // printf("Ticks Heuristic:\n");
        // printf("    target_freq = %g\n", target_freq_hz);
        // printf("    sys_freq    = %g\n", sys_freq);
        // printf("    fast_ticks  = %" PRIu32 "\n", fast_ticks);
        // printf("    slow_ticks  = %" PRIu32 "\n", slow_ticks);
        // printf("    fast_error  = %g\n", fast_error);
        // printf("    slow_error  = %g\n", slow_error);
        
        // Choose fast or slow to minimize error
        TickType_t ticks;
        float freq;
        if (fast_error < slow_error) {
            // printf("using fast\n");
            ticks = fast_ticks;
            freq = fast_freq;
        } else {
            // printf("using slow\n");
            ticks = slow_ticks;
            freq = slow_freq;
        }

        // Print warning if chosen frequency isn't exact
        if (freq != target_freq_hz) {
            printf("RefreshClock: "
                   "approximating %g Hz using %" PRIu32 " ticks = %g Hz\n",
                   target_freq_hz, ticks, freq);
        }
        return ticks;
    }
};

static void timer_callback(TimerHandle_t handle)
{
    auto *priv = (RefreshClock_private *)pvTimerGetTimerID(handle);
    assert(priv);

    BaseType_t higher_priority_task_woken = pdFALSE;
    vTaskNotifyGiveFromISR(priv->task, &higher_priority_task_woken);
    // return higher_priority_task_woken == pdTRUE;
}

RefreshClock::RefreshClock(float freq_hz)
{
    m_private = new RefreshClock_private;
    assert(m_private);

    TickType_t period = m_private->ticks_heuristic(freq_hz);

    const char *timer_name = "refresh_clock";
    BaseType_t auto_reload = pdTRUE;
    void *timer_ID = (void *)m_private;
    m_private->timer =
        xTimerCreate(timer_name, period, auto_reload, timer_ID, timer_callback);    

    m_private->task = xTaskGetCurrentTaskHandle();

    xTimerStart(m_private->timer, period);
}

void RefreshClock::wait()
{
    BaseType_t clear_count = pdFALSE;
    TickType_t timeout = portMAX_DELAY;
    (void)ulTaskNotifyTake(clear_count, timeout);
}