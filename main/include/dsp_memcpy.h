#pragma once

#include <cstddef>

#define DSP_ALIGNMENT 16
#define DSP_ALIGNED_ATTR __attribute__ ((aligned(DSP_ALIGNMENT)))

extern void *dsp_memcpy(void *dest, const void *src, size_t size);
