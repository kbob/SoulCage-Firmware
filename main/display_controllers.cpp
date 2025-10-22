#include "display_controllers.h"

static const uint8_t DELAY_BIT = 0x80;


// //  //   //    //     //      //       //      //     //    //   //  // //
// GalaxyCore GC9A01

static const uint8_t GC9A01_init_string[] = {
    50,
    SWRST, DELAY_BIT, 150,
    0xef, 0,
    0xeb, 1, 0x14,
    0xfe, 0,
    0xef, 0,
    0xeb, 1, 0x14,
    0x84, 1, 0x40,
    0x85, 1, 0xff,
    0x86, 1, 0xff,
    0x87, 1, 0xff,
    0x88, 1, 0xa,
    0x89, 1, 0x21,
    0x8a, 1, 00,
    0x8b, 1, 0x80,
    0x8c, 1, 0x1,
    0x8d, 1, 0x1,
    0x8e, 1, 0xff,
    0x8f, 1, 0xff,
    0xb6, 2, 00, 0x20,
    0x3a, 1, 0x5,
    0x90, 4, 0x8, 0x8, 0x8, 0x8,
    0xbd, 1, 0x6,
    0xbc, 1, 00,
    0xff, 3, 0x60, 0x1, 0x4,
    0xc3, 1, 0x13,
    0xc4, 1, 0x13,
    0xc9, 1, 0x22,
    0xbe, 1, 0x11,
    0xe1, 2, 0x10, 0xe,
    0xdf, 3, 0x21, 0xc, 0x2,
    0xf0, 6, 0x45, 0x9, 0x8, 0x8, 0x26, 0x2a,
    0xf1, 6, 0x43, 0x70, 0x72, 0x36, 0x37, 0x6f,
    0xf2, 6, 0x45, 0x9, 0x8, 0x8, 0x26, 0x2a,
    0xf3, 6, 0x43, 0x70, 0x72, 0x36, 0x37, 0x6f,
    0xed, 2, 0x1b, 0xb,
    0xae, 1, 0x77,
    0xcd, 1, 0x63,
    0x70, 9, 0x7, 0x7, 0x4, 0xe, 0xf, 0x9, 0x7, 0x8, 0x3,
    0xe8, 1, 0x34,
    0x62, 12, 0x18, 0xd, 0x71, 0xed, 0x70, 0x70, 0x18, 0xf, 0x71, 0xef, 0x70, 0x70,
    0x63, 12, 0x18, 0x11, 0x71, 0xf1, 0x70, 0x70, 0x18, 0x13, 0x71, 0xf3, 0x70, 0x70,
    0x64, 7, 0x28, 0x29, 0xf1, 0x1, 0xf1, 00, 0x7,
    0x66, 10, 0x3c, 00, 0xcd, 0x67, 0x45, 0x45, 0x10, 00, 00, 00,
    0x67, 10, 00, 0x3c, 00, 00, 00, 0x1, 0x54, 0x10, 0x32, 0x98,
    0x74, 7, 0x10, 0x85, 0x80, 00, 00, 0x4e, 00,
    0x98, 2, 0x3e, 0x7,
    0x35, 0,
    INVON, 0,
    SLPOUT, DELAY_BIT, 120,
    DISPON, DELAY_BIT, 20,
};

const DisplayController GC9A01_controller = {
    .CASET_x0_adjust = 0,
    .CASET_x1_adjust = -1, // Why?
    .RASET_y0_adjust = 0,
    .RASET_y1_adjust = 0,
    .init_string = GC9A01_init_string,
    .init_string_size = sizeof GC9A01_init_string,
};

// //  //   //    //     //      //       //      //     //    //   //  // //
// Sitronix ST7789V2

static const uint8_t ST7789v2_init_string[] = {
    9,
    SWRST, DELAY_BIT, 150,
    SLPOUT, DELAY_BIT, 255,
    COLMOD, DELAY_BIT | 1, 0x55, 10,
    MADCTL, 1, 0x00,
    CASET, 4, 0x00, 0x00, 0x00, 0xF0,
    PASET, 4, 0x00, 0x00, 0x00, 0xF0,
    INVON, DELAY_BIT, 10,
    NORON, DELAY_BIT, 10,
    DISPON, DELAY_BIT, 255
};
const DisplayController ST7789v2_controller = {
    .CASET_x0_adjust = 0,
    .CASET_x1_adjust = 0,
    .RASET_y0_adjust = +20, // Is this all ST7789v2 or just the Waveshare 1.69?
    .RASET_y1_adjust = +20,
    .init_string = ST7789v2_init_string,
    .init_string_size = sizeof ST7789v2_init_string,
};


// //  //   //    //     //      //       //      //     //    //   //  // //
// Init String Processing

bool DisplayController::execute_init_string(InitOps& ops) const
{
    const uint8_t *cursor = init_string;
    const uint8_t *end = init_string + init_string_size;
    #define CHECK(b) ({ if (!(b)) goto check_failed; })
    #define NEXT() (CHECK(cursor < end), *cursor++)
    #define SKIP(n) \
        ({size_t n_ = (n); CHECK(n_ <= end - cursor), cursor += n_; })

    {
        // Create a block scope so goto won't break lifetimes

        uint8_t command_count = NEXT();

        for (uint8_t i = 0; i < command_count; i++) {
            uint8_t cmd = NEXT();
            uint8_t delay_and_data_count = NEXT();
            bool has_delay = (delay_and_data_count & DELAY_BIT) != 0;
            uint8_t data_count = delay_and_data_count & ~DELAY_BIT;
            CHECK(cursor + data_count <= end);
            ops.write_command(cmd);
            if (data_count) {
                ops.write_data(cursor, data_count);
                SKIP(data_count);
            }
            if (has_delay) {
                uint32_t delay_ms = NEXT();
                if (delay_ms == 255) delay_ms = 500;
                ops.delay_msec(delay_ms);
            }
        }
        CHECK(cursor == end);
    }
    return true;

check_failed:
    ops.error();
    return false;

    #undef SKIP
    #undef NEXT
    #undef CHECK
}
