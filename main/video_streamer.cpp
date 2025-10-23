// This file's header
#include "video_streamer.h"

// C++ standard headers
#include <cstring>

// ESP-IDF headers
#include "freertos/FreeRTOS.h"

// Component headers
#include "animation.h"
#include "board_defs.h"
#include "random.h"
#include "spi_display.h"
#include "static_injector.h"
#include "pixel_types.h"


static const size_t STATIC_STRIPE_COUNT = 6;
static const size_t STRIPE_HEIGHT = SPIDisplay::STRIPE_HEIGHT;
static const size_t STRIPE_SIZE =
    STRIPE_HEIGHT * IMAGE_WIDTH * sizeof (pixel_type);

size_t VideoStreamer::s_static_rotor;

static pixel_type DMA_ATTR
    static_stripes[STATIC_STRIPE_COUNT]
                  [STRIPE_HEIGHT]
                  [IMAGE_WIDTH];


VideoStreamer::VideoStreamer(
    const Animation& src,
    SPIDisplay& dest,
    bool inject_static)
: m_source(src),
  m_display(dest),
  m_inject_static(inject_static)
{
    m_static_source = new StaticInjector;
    assert(m_static_source);
    fill_with_black();
}

VideoStreamer::~VideoStreamer()
{
    delete m_static_source;
}

void VideoStreamer::update()
{
    m_display.begin_frame_centered(IMAGE_WIDTH, IMAGE_HEIGHT);

    static_assert(IMAGE_HEIGHT % STRIPE_HEIGHT == 0);
    for (size_t y = 0; y < IMAGE_HEIGHT; y += STRIPE_HEIGHT) {
        if (m_inject_static && m_static_source->update()) {
            send_static_stripe(y, STRIPE_HEIGHT);
        } else {
            send_image_stripe(y, STRIPE_HEIGHT);
        }
    }
    m_display.end_frame();
}

void VideoStreamer::send_image_stripe(size_t y, size_t height)
{
    const image_type *frame = m_source.current_frame();
    const pixel_type *stripe = (*frame)[y];
    m_last_trans = m_display.send_stripe(y, height, stripe);
}

void VideoStreamer::send_static_stripe(size_t y, size_t height)
{
    pixel_type *stripe = *static_stripes[s_static_rotor];
    s_static_rotor = (s_static_rotor + 1) % STATIC_STRIPE_COUNT;
    for (size_t i = 0; i < STRIPE_HEIGHT * IMAGE_WIDTH; i += 4) {
        int x = Random::rand();
        stripe[i + 0] = pixel_type::from_grey8(x >> 0 & 0xFF);
        stripe[i + 1] = pixel_type::from_grey8(x >> 8 & 0xFF);
        stripe[i + 2] = pixel_type::from_grey8(x >> 16 & 0xFF);
        stripe[i + 3] = pixel_type::from_grey8(x >> 23 & 0xFF);
    }
    m_last_trans = m_display.send_stripe(y, STRIPE_HEIGHT, stripe);
}

void VideoStreamer::fill_with_black()
{
    size_t w = m_display.width();
    size_t h = m_display.height();
    assert(h % STRIPE_HEIGHT == 0);
    pixel_type *black_stripe = static_stripes[0][0];
    std::memset(black_stripe, 0, STRIPE_SIZE);
    // Rely on this being zeroed at app startup.  Sloppy.
    m_display.begin_frame(w, h, 0, 0);
    for (int y = 0; y < h; y += STRIPE_HEIGHT) {
        m_last_trans =
            m_display.send_stripe(y, STRIPE_HEIGHT, black_stripe);
    }
    m_display.end_frame();
    m_display.await_transaction(m_last_trans);
}