#pragma once

#include "pixel_types.h"

class StaticInjector {

public:
    static const int MIN_STATIC = 20;  // 20 stripes = 160 scan lines
    static const int MAX_STATIC = 1200; // 1200 stripes = 40 frames = .67 sec
    static const int MIN_NO_STATIC = 150; // 150 stripes = 5 frames = 8.3 ms
    static const int MAX_NO_STATIC = 4000; // 4000 stripes = 133 frames = 2.2 s

    StaticInjector();

    bool update();

    void fill_with_static(pixel_type *pixels, size_t count);

private:
    int m_active;
    int m_start;
};
