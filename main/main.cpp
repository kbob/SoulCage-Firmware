// C++ standard headers
#include <cstdio>
#include <cstring>

// ESP-IDF and FreeRTOS headers
#include "esp_partition.h"
#include "esp_timer.h"

// Project headers

// Component headers
#include "flash_image.h"
#include "memory_dma.h"

#undef BENCHMARK_DMA

#ifdef BENCHMARK_DMA

    #define IN_FLIGHT 4
    #define REPS 32
    #define BIG (240 * 240 * sizeof (uint16_t))
    uint8_t destination[BIG];
    uint8_t source[BIG];

    static void benchmark_FLASH_DMA(const uint8_t *src, size_t count)
    {
        uint64_t before = esp_timer_get_time();
        for (int i = 0; i < IN_FLIGHT; i++) {
            MemoryDMA::async_memcpy(destination, (void *)src, BIG, NULL);
            src += BIG;
        }
        for (int i = IN_FLIGHT; i < count; i++) {
            uint32_t n = ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
            assert(n <= IN_FLIGHT);
            MemoryDMA::async_memcpy(destination, (void *)src, BIG, NULL);
            src += BIG;
        }
        for (int i = IN_FLIGHT; --i >= 0; ) {
            uint32_t n = ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        assert(n != 0);
        }
        uint64_t after = esp_timer_get_time();
        uint64_t dt_usec = after - before;
        uint64_t bytes = count * BIG;
        float Bps = (float)bytes / (float)dt_usec;
        float uspf = (float)dt_usec / (float)count;
        float fps = 1'000'000.0f / uspf;

        printf("FLASH to SRAM\n");
        printf("    copied %llu bytes in %llu usec: %g MB/sec\n",
               bytes, dt_usec, Bps);
        printf("    %g usec/frame, %g fps\n", uspf, fps);
        printf("\n");
    }

    static void benchmark_SRAM_DMA()
    {
        strcpy((char *)source, "Hello, World!");

        uint64_t before = esp_timer_get_time();
        for (int i = 0; i < IN_FLIGHT; i++) {
            MemoryDMA::async_memcpy(destination, source, BIG, NULL);
        }
        for (int i = IN_FLIGHT; i < REPS; i++) {
            uint32_t n = ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
            assert(n <= IN_FLIGHT);
            MemoryDMA::async_memcpy(destination, source, BIG, NULL);
        }
        for (int i = IN_FLIGHT; --i >= 0; ) {
            uint32_t n = ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        assert(n != 0);
        }
        uint64_t after = esp_timer_get_time();
        uint64_t dt_usec = after - before;
        uint64_t bytes = REPS * BIG;
        float Bps = (float)bytes / (float)dt_usec;
        float uspf = (float)dt_usec / (float)REPS;
        float fps = 1'000'000.0f / uspf;

        printf("SRAM to SRAM\n");
        printf("    copied %llu bytes in %llu usec: %g MB/sec\n",
               bytes, dt_usec, Bps);
        printf("    %g usec/frame, %g fps\n", uspf, fps);
        printf("\n");
    }

#endif

extern "C" void app_main()
{
    printf("app_main\n");
#ifdef WAVESHARE_ESP32_S3_LCD_TOUCH_1_69
    printf("This is the Waveshare 1.69 board!\n");
#endif
    printf("board = \"%s\"\n", BOARD);
    printf("\n");

    printf("Flash images found:\n");
    for (size_t i = 0; i < FlashImage::MAX_IMAGES; i++) {
        if (FlashImage *image = FlashImage::get_by_index(i)) {
            printf("  \"%s\"%*s %zu bytes, %zu frames\n",
                image->label(),
                (int)(FlashImage::MAX_LABEL_SIZE - std::strlen(image->label())), "",
                image->size_bytes(),image->frame_count());
        }
    }
    printf("\n");

#ifdef BENCHMARK_DMA
    benchmark_SRAM_DMA();
    benchmark_FLASH_DMA((const uint8_t *)part_addrs[2], part_sizes[2] / BIG);
#endif
}

// define SPI
// define framebuffer
// write some pixels
// transmit some pixels
