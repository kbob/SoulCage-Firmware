#include "main/include/packed_color.h"

#include <cassert>
#include <cstdio>

typedef PackedColorEE<EOrder::RGB565, PixelEndian::BIG> rgb_big;
typedef PackedColorEE<EOrder::BGR565, PixelEndian::BIG> bgr_big;

int main()
{
    for (int red8 = 0; red8 < 256; red8 += 256 / 32) {
        uint16_t c = rgb_big::rgb565(red8, 0, 0);
        // printf("rgb(%d) = %#x\n", red8, c);
        assert(c == red8 << 8);
    }

    for (int red8 = 0; red8 < 256; red8 += 256 / 32) {
        auto c = rgb_big(red8, 0, 0);
        // printf("rgb(%d) = %#x = [%02x %02x] = (%d %d %d)\n",
        //        red8,
        //        c.color(),
        //        c.b[0], c.b[1],
        //        c.red5(), c.green6(), c.blue5());
        assert(c.color() == red8 << 8);
        assert(c.b[0] == (red8 & 0xF8));
        assert(c.b[1] == 0);
        assert(c.red5() == red8 >> 3);
        assert(c.green6() == 0);
        assert(c.blue5() == 0);
    }

    for (int green8 = 0; green8 < 256; green8 += 256 / 64) {
        auto c = rgb_big(0, green8, 0);
        // printf("rgb(%d) = %#x = (%d %d %d)\n", green8, c.color(), c.red5(), c.green6(), c.blue5());
        assert(c.color() == green8 << (5 - 2));
        assert(c.red5() == 0);
        assert(c.green6() == green8 >> 2);
        assert(c.blue5() == 0);
    }

    for (int blue8 = 0; blue8 < 256; blue8 += 256 / 32) {
        auto c = rgb_big(0, 0, blue8);
        // printf("rgb(%d) = %#x = (%d %d %d)\n", blue8, c.color(), c.red5(), c.green6(), c.blue5());
        assert(c.color() == blue8 >> 3);
        assert(c.red5() == 0);
        assert(c.green6() == 0);
        assert(c.blue5() == blue8 >> 3);
    }

    for (int red8 = 0; red8 < 256; red8 += 256 / 32) {
        auto rgb = rgb_big(red8, 0, 0);
        bgr_big bgr = bgr_big::from_PackedColor(rgb);
        // printf("red %u -> %#x\n", red8, bgr.color());
        assert(bgr.color() == red8 >> 3);
        assert(bgr.red5() == red8 >> 3);
        assert(bgr.green6() == 0);
        assert(bgr.blue5() == 0);
    }
        for (int green8 = 0; green8 < 256; green8 += 256 / 64) {
        auto rgb = rgb_big(0, green8, 0);
        bgr_big bgr = bgr_big::from_PackedColor(rgb);
        // printf("green %u -> %#x\n", green8, bgr.color());
        assert(bgr.color() == green8 << (5 - 2));
        assert(bgr.red5() == 0);
        assert(bgr.green6() == green8 >> 2);
        assert(bgr.blue5() == 0);
    }
    for (int blue8 = 0; blue8 < 256; blue8 += 256 / 32) {
        auto rgb = rgb_big(0, 0, blue8);
        bgr_big bgr = bgr_big::from_PackedColor(rgb);
        // printf("blue %u -> %#x\n", blue8, bgr.color());
        assert(bgr.color() == blue8 << 8);
        assert(bgr.red5() == 0);
        assert(bgr.green6() == 0);
        assert(bgr.blue5() == blue8 >> 3);
    }

    return 0;
}