#include "memory_dma.h"

MemoryDMA MemoryDMA::the_instance;

MemoryDMA::MemoryDMA()
{
    async_memcpy_config_t config = {};
    config.dma_burst_size = 16;

    ESP_ERROR_CHECK(
        esp_async_memcpy_install_gdma_ahb(&config, &mcp)
    );
}

MemoryDMA::~MemoryDMA()
{
    esp_async_memcpy_uninstall(mcp);
}

void MemoryDMA::start_DMA(void *dst, void *src, size_t size,
                          TaskHandle_t notified)
{
    if (notified == NULL) {
        notified = xTaskGetCurrentTaskHandle();
    }
    ESP_ERROR_CHECK(
        esp_async_memcpy(mcp, dst, src, size, callback, notified)
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
