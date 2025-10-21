#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

// Read-only description of a display controller
//
// The `init_string` format is based on TFT-eSPI's.  It is compact
// and easy to write and parse.
//
// The first byte is the number of commands.  It's followed by
// the command descriptions.  For each command, we have
//
//   - the command byte (register address)
//
//   - one byte containing a delay flag and a data byte count
//
//   - data_byte_count bytes of data
//
//   - if the delay flag was set, one byte of delay.
//     If delay == 255, it means delay 500 msec.
//     Otherwise, it is the number of msec to delay.
//
// The execute_init_string method encapsulates that format.

struct DisplayController {

    // Some controllers need slightly different CASET and PASET parameters.
    int16_t CASET_x0_adjust;
    int16_t CASET_x1_adjust;
    int16_t RASET_y0_adjust;
    int16_t RASET_y1_adjust;

    const uint8_t *init_string;
    size_t init_string_size;

    class InitOps {
    public:
        virtual void write_command(uint8_t cmd) = 0;
        virtual void write_data(const uint8_t *data, size_t count) = 0;
        virtual void delay_msec(uint32_t msec) = 0;
        virtual void error() { assert(false && "malformed init string"); }
    };

    // parse init string, call back to ops, return true on success
    bool execute_init_string(InitOps&) const;
};

// These are the known display controllers. Maybe more will come along.

extern const DisplayController GC9A01_controller;
extern const DisplayController ST7789v2_controller;
