// This file's header
#include "animation.h"

// C++ standard headers
#include <cstring>

// ESP-IDF headers
#include "freertos/FreeRTOS.h"

// Component headers
#include "flash_image.h"
#include "random.h"

image_type DMA_ATTR Animation::s_image_buffers[IMAGE_BUFFER_COUNT];

Animation::Animation(unsigned anim_frames, float change_prob)
: m_anim_frame_count(anim_frames),
  m_soul_change_probability(change_prob),
  m_in_intro(true),
  m_current_frame(0),
  m_current_image_buffer(0)
{
    m_current_image = FlashImage::get_by_label("Intro");
    assert(m_current_image != nullptr);
    m_image_frame_count = m_current_image->frame_count();

    load_frame(m_current_image_buffer);
}

image_type *Animation::current_frame() const
{
    return s_image_buffers + m_current_image_buffer;
}

void Animation::update()
{
    maybe_change_animation();
    // update the animation every 8 frames
    static int frame;
    frame = (frame + 1) % 8;
    if (frame != 0) {
        return;
    }

    // printf("update animation frame %zu\n", m_current_frame);
    size_t next_i_buf = (m_current_image_buffer + 1) % IMAGE_BUFFER_COUNT;
    load_frame(next_i_buf);
    m_current_image_buffer = next_i_buf;

    m_current_frame = (m_current_frame + 1) % m_image_frame_count;
    if (m_current_frame == 0 && m_in_intro) {
        // if we've finished the intro, start on one of the souls..
        m_in_intro = false;
        const char *next_image = Random::rand() % 2 ? "soul_f" : "soul_m";
        m_current_image = FlashImage::get_by_label(next_image);
        assert(m_current_image != nullptr);
        m_image_frame_count = m_current_image->frame_count();
    }
}

void Animation::maybe_change_animation()
{
    static int frame;
    frame = (frame + 1) % m_anim_frame_count;
    if (frame != 0) {
        return;
    }

    if (Random::randint(0, 1000) < 1000.0f * m_soul_change_probability) {
        auto old_label = m_current_image->label();

        // Don't interrupt the intro.
        if (!std::strcmp(old_label, "Intro")) {
            return;
        }
        const char *new_label;
        if (!std::strcmp(old_label, "soul_m")) {
            new_label = "soul_f";
        } else {
            assert(strcmp(old_label, "soul_f") == 0);
            new_label = "soul_m";
        }
        m_current_image = FlashImage::get_by_label(new_label);
        assert(m_current_image != nullptr);
        m_image_frame_count = m_current_image->frame_count();
        m_current_frame = Random::randint(0, m_image_frame_count);
    }
}

void Animation::load_frame(size_t buffer_index)
{
    image_type *dest = s_image_buffers + buffer_index;
    const image_type *src = m_current_image->frame_addr(m_current_frame);
    size_t size = sizeof *dest;
    std::memcpy((void *)dest, (void *)src, size);
}