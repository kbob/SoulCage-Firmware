// This file's header
#include "memory_dma.h"

// C++ standard headers
#include <cstring>

MemoryDMA MemoryDMA::the_instance;

MemoryDMA::MemoryDMA()
{
    async_memcpy_config_t config = {};
    config.backlog = 8;
    config.dma_burst_size = 16;
    config.flags = 0;

    ESP_ERROR_CHECK(
        esp_async_memcpy_install(&config, &mcp)
    );
}

MemoryDMA::~MemoryDMA()
{
    esp_async_memcpy_uninstall(mcp);
}

void MemoryDMA::start_DMA(void *dst, const void *src, size_t size,
                          TaskHandle_t notified)
{
    if (notified == NULL) {
        notified = xTaskGetCurrentTaskHandle();
    }
    ESP_ERROR_CHECK(
        esp_async_memcpy(mcp, dst, const_cast<void *>(src), size, callback, notified)
    );
}

bool MemoryDMA::callback(async_memcpy_handle_t mcp,
                         async_memcpy_event_t *event,
                         void *cb_args)
{
    TaskHandle_t notified = (TaskHandle_t)cb_args;
    BaseType_t high_task_wakeup = pdFALSE;
    vTaskNotifyGiveFromISR(notified, &high_task_wakeup);
    return high_task_wakeup == pdTRUE;
}
