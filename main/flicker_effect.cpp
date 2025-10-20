// This file's header
#include "flicker_effect.h"

// C++ standard headers
#include <cmath>
#include <numbers>

// This is the sum of 10 sine waves of various frequencies,
// rectified.  It ranges from 0 to about 4.5.
float SpookyFlickerEffect::spooky_flicker_function(unsigned frame) const {
    assert(frame < m_anim_frame_count);

    static float s_frequencies[HARMONICS];
    if (s_frequencies[0] == 0) {
        const float TAU = 2.0 * std::numbers::pi;
        for (size_t h = 0; h < HARMONICS; h++) {
            s_frequencies[h] = std::powf(h + 1, 1.5f) * TAU;
        }
    }

    float x = (float)frame / (float)m_anim_frame_count;
    float acc = 0.0f;
    for (int h = 0; h < HARMONICS; h++) {
        acc += std::sinf(s_frequencies[h] * x);
    }
    acc += 1.0f; // reduce the duty cycle a little
    return std::max(-acc, 0.0f); // dark part first
}

float SpookyFlickerEffect::calc_scale()
{
    float max = 0.0f;
    for (unsigned frame = 0; frame < m_anim_frame_count; frame++) {
        max = std::max(max, spooky_flicker_function(frame));
    }
    return 1.0f / max;
}
