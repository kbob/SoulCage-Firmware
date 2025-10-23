// This file's header
#include "static_injector.h"

// Component headers
#include "pixel_types.h"
#include "random.h"

StaticInjector::StaticInjector()
: m_active(0),
  m_start(-1)
{}

bool StaticInjector::update()
{
    if (m_start == -1) {
        // first time
        m_start = Random::randint(MIN_NO_STATIC, MAX_NO_STATIC);
    } else if (!m_active && --m_start == 0) {
        // finished inactive period
        m_active = Random::randint(MIN_STATIC, MAX_STATIC);
    }
    if (m_active) {
        if (--m_active != 0) {
            // still active
            return true;
        }
        // finished active period
        m_start = Random::randint(MIN_NO_STATIC, MAX_NO_STATIC);
    }
    return false;
}

void StaticInjector::fill_with_static(pixel_type *pixels, size_t count)
{
    assert(count % 4 == 0);
    for (size_t i = 0; i < count; i += 4) {
        int x = Random::rand();
        pixels[i + 0] = pixel_type::from_grey8(x >> 0 & 0xFF);
        pixels[i + 1] = pixel_type::from_grey8(x >> 8 & 0xFF);
        pixels[i + 2] = pixel_type::from_grey8(x >> 16 & 0xFF);
        pixels[i + 3] = pixel_type::from_grey8(x >> 23 & 0xFF);
    }
}
