#pragma once

#include <cstddef>
#include "esp_async_memcpy.h"
#include "freertos/FreeRTOS.h"


class MemoryDMA {

public:
    static void async_memcpy(void *dst, void *src, size_t size,
                             TaskHandle_t notified)
    {
        the_instance.start_DMA(dst, src, size, notified);
    }

private:
    MemoryDMA();
    ~MemoryDMA();
    MemoryDMA(const MemoryDMA&) = delete;
    void operator = (const MemoryDMA&) = delete;

    void start_DMA(void *dst, void *src, size_t size, TaskHandle_t notified);
    static bool callback(async_memcpy_handle_t, async_memcpy_event_t *, void *);

    static MemoryDMA the_instance;
    async_memcpy_handle_t mcp;

};
