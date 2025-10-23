#pragma once

#include <cstddef>
#include "spi_display.h"

class Animation;
class StaticInjector;

class VideoStreamer {

public:
    VideoStreamer(const Animation&, SPIDisplay&, bool inject_static);
    ~VideoStreamer();

    void update();

private:
    VideoStreamer(const VideoStreamer&) = delete;
    void operator = (const VideoStreamer&) = delete;    

    const Animation& m_source;
    StaticInjector *m_static_source;
    SPIDisplay& m_display;
    const bool m_inject_static;
    TransactionID m_last_trans;

    static size_t s_static_rotor;

    void fill_with_black();
    void send_image_stripe(size_t y, size_t height);
    void send_static_stripe(size_t y, size_t height);
};
