// C++ standard headers
#include <cstdio>
#include <cstring>

// ESP-IDF and FreeRTOS headers
#include "esp_partition.h"
#include "esp_timer.h"

// Component headers
#include "memory_dma.h"

#ifdef BENCHMARK_RAM_DMA

    #define IN_FLIGHT 4
    #define REPS 32
    #define BIG (240 * 240 * sizeof (uint16_t))
    uint8_t destination[BIG];
    uint8_t source[BIG];

    static void benchmark_RAM_DMA()
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

        printf("memcpy finished\n");
        printf("received \"%s\"\n", destination);
        printf("copied %llu bytes in %llu usec: %g MB/sec\n",
               bytes, dt_usec, Bps);
        printf("%g usec/frame, %g fps\n", uspf, fps);
    }

#endif

static void print_partition_table()
{
    auto iter = esp_partition_find(
        ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
    int i = 0;
    printf("Partition Table\n");
    do {
        auto part = esp_partition_get(iter);
        printf("  %d: (%#4x %#4x) %2s %#08lx %#8lx \"%s\"\n",
            i,
            part->type, part->subtype,
            part->readonly ? "ro" : "rw",
            part->address, part->size,
            part->label);
        i++;
    } while (esp_partition_next(iter));
    printf("\n");
}

static void map_partition()
{
    const char *labels[3] = {"Intro", "soul_f", "soul_m"};
    esp_partition_mmap_handle_t part_handles[3];
    const void *addrs[3];

    for (int i = 0; i < 3; i++) {
        auto type = ESP_PARTITION_TYPE_DATA;
        auto subtype = ESP_PARTITION_SUBTYPE_ANY;
        auto label = labels[i];
        auto *part = esp_partition_find_first(type, subtype, label);
        if (part == NULL) {
            printf("\"Intro\" partition not found\n");
            return;
        }

        auto memory = ESP_PARTITION_MMAP_DATA;
        const void *out_ptr;
        esp_partition_mmap_handle_t *handle_p = &part_handles[i];
        ESP_ERROR_CHECK(
            esp_partition_mmap(part, 0, part->size, memory, &out_ptr, handle_p)
        );
        printf("mmapped at %p, size %ld\n", out_ptr, part->size);
        addrs[i] = out_ptr;
    }

}

extern "C" void app_main()
{
    printf("app_main\n");
    print_partition_table();

    map_partition();
}

// define SPI
// define framebuffer
// write some pixels
// transmit some pixels
