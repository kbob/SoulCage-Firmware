#include "benchmarks.h"

// C++ standard headers
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

// ESP-IDF headers
#include "esp_timer.h"

// Component headers
#include "dsp_memcpy.h"
#include "flash_image.h"
#include "memory_dma.h"

// Should benchmark
//   SOURCES X DESTINATIONS X ALGORITHMS
//   sources: flash, sram, psram
//   destinations: sram, psram
//   algorithms: std::memcpy, DMA, DSP memcpy
//   3 * 2 * 3 = 18

#define IN_FLIGHT 4
#define REPS 100
#define BIG (240 * 240 * sizeof (uint16_t))

intptr_t psram_start = 0x3c00'0000;
intptr_t psram_end   = 0x3dff'ffff;
intptr_t sram_start  = 0x3fc8'8000;
intptr_t sram_end    = 0x3fcf'ffff;

typedef void copy_fn(void *dest, const void *src, size_t);
typedef void wait_fn();

class Algorithm {
public:
    virtual ~Algorithm() = default;
    virtual const char *label() const = 0;
    virtual void copy(void *, const void *, size_t) = 0;
    virtual void wait() {}
};

class Source {
public:
    virtual ~Source() = default;
    virtual const char *label() const = 0;
    virtual const void *addr(size_t index) const = 0;
};

class Destination {
public:
    virtual ~Destination() = default;
    virtual const char *label() const = 0;
    virtual void *addr(size_t index) = 0;
};

class StdMemcpy : public Algorithm {
public:
    const char *label() const override { return "memcpy"; }
    void copy(void *dest, const void *src, size_t size) override
    {
        std::memcpy(dest, src, size);
    }
};

class DMACopy : public Algorithm {
    const char *label() const override { return "DMA copy"; }
    void copy(void *dest, const void *src, size_t size) override
    {
        MemoryDMA::async_memcpy(dest, src, size, nullptr);
    }
    void wait() override
    {
        uint32_t n = ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        assert(n <= IN_FLIGHT);
    }
};

class DSPCopy : public Algorithm {
public:
    const char *label() const override { return "DSP copy"; }
    void copy(void *dest, const void *src, size_t size) override
    {
        dsp_memcpy(dest, src, size);
    }
};

class SRAMSource : public Source {
public:
    SRAMSource()
    {
        auto addr = (intptr_t)s_memory;
        assert(sram_start <= addr && addr <= sram_end);
    }
    const char *label() const override { return "SRAM"; }
    const void *addr(size_t index) const override
    {
        return (void *)s_memory;
    }
private:
    static uint8_t s_memory[BIG]; // can't be `const` or it'll go to flash
};
uint8_t DMA_ATTR DSP_ALIGNED_ATTR SRAMSource::s_memory[BIG] = {};

class PSRAMSource : public Source {
public:
    PSRAMSource()
    {
        auto addr = (intptr_t)s_memory;
        assert(psram_start <= addr && addr <= psram_end);
    }
    const char *label() const override { return "PSRAM"; }
    const void *addr(size_t index) const override
    {
        return (void *)s_memory;
    }
private:
    static const uint8_t s_memory[BIG];
};
const uint8_t EXT_RAM_BSS_ATTR DSP_ALIGNED_ATTR PSRAMSource::s_memory[BIG] = {};

class FlashSource : public Source {
public:
    FlashSource()
    : m_image(FlashImage::get_by_index(0)),
        m_frame_count(m_image->frame_count())
    {}
    const char *label() const override { return "flash"; }
    const void *addr(size_t index) const override
    {
        return (const void *)m_image->frame_addr(index % m_frame_count);
    }
private:
    FlashImage *m_image;
    size_t m_frame_count;
};

class SRAMDestination : public Destination {
public:
    SRAMDestination()
    {
        auto addr = (intptr_t)s_memory;
        assert(sram_start <= addr && addr <= sram_end);
    }
    const char *label() const override { return "SRAM"; }
    void *addr(size_t index) override { return s_memory; }
private:
    static uint8_t s_memory[BIG];
};
uint8_t DSP_ALIGNED_ATTR SRAMDestination::s_memory[BIG];

class PSRAMDestination : public Destination {
public:
    PSRAMDestination()
    : m_memory(heap_caps_malloc(BIG + DSP_ALIGNMENT - 1, MALLOC_CAP_SPIRAM))
    {
        assert(m_memory);
        auto addr = (intptr_t)m_memory;
        assert(psram_start <= addr && addr <= psram_end);
        intptr_t low_bits = DSP_ALIGNMENT - 1;
        m_addr = (void *)((addr + low_bits) & ~low_bits);
        // printf("m_memory=%p addr=%x m_addr=%p\n", m_memory, addr, m_addr);
    }
    ~PSRAMDestination()
    {
        heap_caps_free(m_memory);
    }
    const char *label() const override { return "PSRAM"; }
    void *addr(size_t index) override { return m_addr; }
private:
    void *m_memory;
    void *m_addr;
};

void run_one_benchmark(Source *source, Destination *destination, Algorithm *algo)
{
    int16_t before = esp_timer_get_time();

    for (size_t i = 0; i < REPS; i++) {
        void *dest = destination->addr(i);
        const void *src = source->addr(i);
        size_t size = BIG;
        // printf("dest=%p src=%p size=%zu\n", dest, src, size);
        algo->copy(dest, src, size);
        if (i >= IN_FLIGHT) {
            algo->wait();
        }
    }
    for (size_t i = 0; i < IN_FLIGHT; i++) {
        algo->wait();
    }

    int64_t after = esp_timer_get_time();
    int64_t dt_usec = after - before;
    size_t bytes = REPS * BIG;
    float MBps = (float)bytes / (float)dt_usec;
    float fps = (float)REPS / (float)dt_usec * 1'000'000.0f;

    static bool been_here;
    if (!been_here) {
        printf("Src   Dest  Algo     |    Bytes    uSec   MB/s    FPS\n");
        printf("===== ===== ======== | ======== ======= ====== ======\n");
        been_here = true;
    }
    printf("%-5s %-5s %-8s | %7zu %7lld %6.4g %6.4g\n",
            source->label(),
            destination->label(),
            algo->label(),
            bytes,
            dt_usec,
            MBps,
            fps);
}

void run_memcpy_benchmarks()
{
    std::vector<Algorithm *> algorithms = {
        new StdMemcpy(),
        new DMACopy(),
        new DSPCopy(),
    };
    std::vector<Source *> sources = {
        new SRAMSource(),
        new PSRAMSource(),
        new FlashSource(),
    };
    std::vector<Destination *> destinations = {
        new SRAMDestination(),
        new PSRAMDestination(),
    };

    for (auto src : sources) {
        for (auto dest : destinations) {
            for (auto algo : algorithms) {
                run_one_benchmark(src, dest, algo);
            }
            printf("\n");
        }
    }

    // Yeah, I know...  std::make_array(std::make_unique<StdMemcpy>(), ...)
    // ... or something.  Seems worse than this.
    for (auto& algo : algorithms) { delete algo; algo = nullptr; }
    for (auto& src : sources) { delete src; src = nullptr; }
    for (auto& dest : destinations) { delete dest; dest = nullptr; }
}
