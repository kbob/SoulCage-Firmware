#pragma once

#include <cstddef>
#include <cstdint>

// Color channels are encoded into six hex digits like this.
//    0xR_____ - number of bits in red channel
//    0x_r____ - how far left to shift the red bits
//    0x__G___ - number of bits in green channel
//    0x___g__ - how far left to shift the green bits
//    0x____B_ - ... blue bits...
//    0x_____b - ... blue shift...

enum EOrder {
    RGB565 = 0x5B6550,
    BGR565 = 0x50655B,
    // add others as needed
};

enum PixelEndian {
    BIG = 0b10,
    LITTLE = 0b01,
};

template <EOrder RGB_ORDER, PixelEndian ENDIAN> class PackedColorEE {

public:
    uint8_t b[2];

    typedef PackedColorEE<RGB_ORDER, ENDIAN> PackedColor;

    PackedColorEE() = default;
    PackedColorEE(const PackedColor&) = default;
    PackedColor& operator = (const PackedColor&) = default;

    PackedColorEE(uint8_t red8, uint8_t green8, uint8_t blue8) {
        const size_t lsb = ENDIAN >> 1 & 1;
        const size_t msb = ENDIAN >> 0 & 1;
        uint16_t color = rgb565(red8, green8, blue8);
        b[lsb] = color & 0xFF;
        b[msb] = color >> 8 & 0xFF;
    }

    static PackedColor from_grey8(uint8_t grey8)
    {
        return PackedColor(grey8, grey8, grey8);
    }
    static PackedColor from_gray8(uint8_t gray8) { return from_grey8(gray8); }

    // bit-swizzle a different PackedColor into our format.
    // (I wonder how well the optimizer will cope with this.)
    template <EOrder that_eorder, PixelEndian that_endian>
    static PackedColor
    from_PackedColor(const PackedColorEE<that_eorder, that_endian>& that)
    {
        return PackedColor(that.red8(), that.green8(), that.blue8());
    }

    // convert to uint16_t 
    uint16_t color() const
    {
        const size_t lsb = ENDIAN >> 1 & 1;
        const size_t msb = ENDIAN >> 0 & 1;
        return b[msb] << 8 | b[lsb];
    }

    // access the components at 5, 6, 5 bit resolution.
    uint8_t red5() const
    {
        const uint32_t rb = RGB_ORDER >> 20 & 0xF;
        const uint32_t rs = RGB_ORDER >> 16 & 0xF;
        return color() >> rs & ((1 << rb) - 1);
    }

    uint8_t green6() const
    {
        const uint32_t gb = RGB_ORDER >> 12 & 0xF;
        const uint32_t gs = RGB_ORDER >> 8 & 0xF;
        return color() >> gs & ((1 << gb) - 1);
    }

    uint8_t blue5() const
    {
        const uint32_t bb = RGB_ORDER >> 4 & 0xF;
        const uint32_t bs = RGB_ORDER >> 0 & 0xF;
        return color() >> bs & ((1 << bb) - 1);
    }

    // access the components expanded to 8 bit resolution.
    uint8_t red8() const
    {
        const uint32_t rb = RGB_ORDER >> 20 & 0xF;
        uint8_t r5 = red5();
        return r5 << (8 - rb) | r5 >> (rb + rb - 8);
    }

    uint8_t green8() const
    {
        const uint32_t gb = RGB_ORDER >> 12 & 0xF;
        uint8_t g6 = green6();
        return g6 << (8 - gb) | g6 >> (gb + gb - 8);
    }

    uint8_t blue8() const {
        const uint32_t bb = RGB_ORDER >> 4 & 0xF;
        uint8_t b5 = blue5();
        return b5 << (8 - bb) | b5 >> (bb + bb - 8);
    }

    static uint16_t rgb565(uint8_t red8, uint8_t green8, uint8_t blue8)
    {
        const uint32_t rb = RGB_ORDER >> 20 & 0xF;
        const uint32_t rs = RGB_ORDER >> 16 & 0xF;
        const uint32_t gb = RGB_ORDER >> 12 & 0xF;
        const uint32_t gs = RGB_ORDER >> 8 & 0xF;
        const uint32_t bb = RGB_ORDER >> 4 & 0xF;
        const uint32_t bs = RGB_ORDER >> 0 & 0xF;

        uint16_t r = (uint16_t)red8 >> (8 - rb) << rs;
        uint16_t g = (uint16_t)green8 >> (8 - gb) << gs;
        uint16_t b = (uint16_t)blue8 >> (8 - bb) << bs;
        uint16_t rgb = r | g | b;

        return rgb;
    }

    // convert 8 bit greyscale value to a packed uint16_t;
    static uint16_t grey565(uint8_t grey)
    {
        return rgb565(grey, grey, grey);
    }

    // there are two ways to spell gray/grey.
    static uint16_t gray565(uint8_t gray)
    {
        return grey565(gray);
    }
};

#if 0 /* old non-template version */


struct PackedColor {
    uint8_t b[2];

    PackedColor() = default;
    PackedColor(const PackedColor&) = default;
    PackedColor& operator = (const PackedColor&) = default;

    // PackedColor(uint16_t color) { b[0] = color >> 8; b[1] = color; }

    PackedColor(uint8_t red, uint8_t green, uint8_t blue)
    {
        uint16_t color = rgb565(red, green, blue);
        b[0] = color >> 8;
        b[1] = color;
    }
    PackedColor(uint8_t grey)
    {
        uint16_t color = grey565(grey);
        b[0] = color >> 8;
        b[1] = color;
    }

    // access the components at 5, 6, 5 bit resolution.
    uint8_t red5() const { return b[0] >> 3; }
    uint8_t green6() const { return ((b[0] & 0x07) << 3) | (b[1] >> 5); }
    uint8_t blue5() const { return b[1] & 0x1F; }

    // access the components expanded to 8 bit resolution.
    uint8_t red8() const { uint8_t r = red5(); return r << 3 | r >> 2; }
    uint8_t green8() const { uint8_t g = green6(); return g << 2 | g >> 4; }
    uint8_t blue8() const { uint8_t b = blue5(); return b << 3 | b >> 2; }

    // convert 8 bit r, g, b to a packed uint16_t.
    uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue)
    {
        return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3);
    }

    // convert 8 bit greyscale value to a packed uint16_t;
    uint16_t grey565(uint8_t grey)
    {
        return rgb565(grey, grey, grey);
    }

    // there are two ways to spell gray/grey.
    uint16_t gray565(uint8_t gray)
    {
        return grey565(gray);
    }
};

// some random colors
const PackedColor PC_DARK_BLUE = PackedColor(0, 0, 127);
const PackedColor PC_PINK = PackedColor(255, 63, 63);

#endif
