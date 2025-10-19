// This file's header
#include "dsp_memcpy.h"

// C++ standard headers
#include <cassert>
#include <cstdint>

static void
__attribute__((noinline))
asm_copy_chunks(void *dest, const void *src, size_t chunk_count)
{
    uint32_t i = 0;

    asm volatile (

        "    movi %2, 0               \n"
        "loop%=:                      \n"

        "    ee.vld.128.ip q0, %1, 16 \n"
        "    ee.vld.128.ip q1, %1, 16 \n"
        "    ee.vld.128.ip q2, %1, 16 \n"
        "    ee.vld.128.ip q3, %1, 16 \n"
        "    ee.vld.128.ip q4, %1, 16 \n"
        "    ee.vld.128.ip q5, %1, 16 \n"
        "    ee.vld.128.ip q6, %1, 16 \n"
        "    ee.vld.128.ip q7, %1, 16 \n"

        "    ee.vst.128.ip q0, %0, 16 \n"
        "    ee.vst.128.ip q1, %0, 16 \n"
        "    ee.vst.128.ip q2, %0, 16 \n"
        "    ee.vst.128.ip q3, %0, 16 \n"
        "    ee.vst.128.ip q4, %0, 16 \n"
        "    ee.vst.128.ip q5, %0, 16 \n"
        "    ee.vst.128.ip q6, %0, 16 \n"
        "    ee.vst.128.ip q7, %0, 16 \n"

        "    addi.n %2, %2, 1         \n"
        "    bne %2, %6, loop%=         "

        : "=r" (dest),
          "=r" (src),
          "=r" (i)

        : "r" (dest),
          "r" (src),
          "r" (i),
          "r" (chunk_count)

        : "memory"
    );
}

void *dsp_memcpy(void *dest, const void *src, size_t size)
{
    const size_t REG_BYTES = 16;
    const size_t REG_COUNT = 8;
    const size_t CHUNK_BYTES = REG_BYTES * REG_COUNT;
    const size_t BAD_SIZE_MASK = CHUNK_BYTES - 1;
    const size_t DSP_ALIGN_MASK = DSP_ALIGNMENT - 1;

    assert(((intptr_t)dest & DSP_ALIGN_MASK) == 0);
    assert(((intptr_t)src & DSP_ALIGN_MASK) == 0);
    assert((size & BAD_SIZE_MASK) == 0);

    size_t chunk_count = size / CHUNK_BYTES;
    asm_copy_chunks(dest, src, chunk_count);
    return dest;
}
