#pragma once

#include <cstddef>
#include "driver_backlight.h"

class SpookyFlickerEffect {

    static const size_t HARMONICS = 10; // Not actually harmonics anymore.

public:

    SpookyFlickerEffect(
        Backlight& backlight,
        unsigned anim_frame_count,
        bool enabled = true)
    : m_backlight(backlight),
      m_anim_frame_count(anim_frame_count),
      m_frame(0),
      m_scale(calc_scale()),
      m_enabled(enabled)
    {}

    unsigned animation_frame_count() const { return m_anim_frame_count; }
    float frame_brightness(unsigned frame) const
    {
        return spooky_flicker_function(frame) * m_scale;
    }

    bool enabled() const { return m_enabled; }

    void set_animation_frame_count(unsigned count)
    {
        m_anim_frame_count = count;
        m_frame = 0;
        m_scale = calc_scale();
    }

    void set_enabled(bool enabled = true)
    {
        m_enabled = enabled;
    }

    void update()
    {
        if (m_enabled) {
            float brightness = frame_brightness(m_frame);
            m_backlight.set_brightness(brightness);
        }
        m_frame = (m_frame + 1) % m_anim_frame_count;
    }

private:

    float spooky_flicker_function(unsigned frame) const;
    float calc_scale();

    Backlight& m_backlight;
    unsigned m_anim_frame_count;
    unsigned m_frame;
    float m_scale;
    bool m_enabled;

    // static float s_frequencies[HARMONICS];
};
