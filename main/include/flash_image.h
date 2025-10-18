#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include "board_defs.h"

class FlashImage {

public:
    static const size_t MAX_IMAGES = 3;
    static const size_t IMAGE_HEIGHT = 240; // XXX get these from config
    static const size_t IMAGE_WIDTH = 240;
    static const size_t FRAME_PIXEL_COUNT = IMAGE_HEIGHT * IMAGE_WIDTH;
    typedef ScreenPixelType pixel_type;

    static const size_t MAX_LABEL_SIZE = 16;
    static const size_t FRAME_SIZE = (240 * 240 * 2); // XXX parameterize this
    static FlashImage *get_by_label(const char *);
    static FlashImage *get_by_index(size_t);

    const char *label() const { return m_label; }
    size_t size_bytes() const { return m_size; }
    size_t frame_count() const { return m_size / FRAME_SIZE; }
    const void *base_addr() const { return m_addr; }
    const pixel_type *frame_addr(size_t i) const
    {
        assert(i < IMAGE_HEIGHT);
        return (const pixel_type *)m_addr + i * FRAME_PIXEL_COUNT;
    }

private:
    FlashImage() = default;
    FlashImage(const FlashImage&) = delete;
    void operator = (const FlashImage&) = delete;
    ~FlashImage();

    // instance members
    char m_label[MAX_LABEL_SIZE + 1];
    const void *m_addr;
    size_t m_size;
    uint32_t m_handle;

    // static members
    static FlashImage images[MAX_IMAGES];
    static size_t image_count;
    static void find_images();
};
