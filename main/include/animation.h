// This file's header
#pragma once

#include "pixel_types.h"

class FlashImage;

class Animation {

public:
    Animation(unsigned anim_frame_count, float soul_change_probability);

    image_type *current_frame() const;

    void update();

private:
    static const size_t IMAGE_BUFFER_COUNT = 2;

    unsigned m_anim_frame_count;
    float m_soul_change_probability;
    bool m_in_intro;
    const FlashImage *m_current_image;
    size_t m_image_frame_count;
    size_t m_current_frame;
    size_t m_current_image_buffer;

    Animation(const Animation&) = delete;
    void operator = (const Animation&) = delete;

    void maybe_change_animation();
    void load_frame(size_t);

    static image_type s_image_buffers[IMAGE_BUFFER_COUNT];
};
