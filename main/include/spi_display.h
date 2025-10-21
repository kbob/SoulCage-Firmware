#pragma once

#include <cstddef>
#include <cstdint>
#include "board_defs.h"

struct Display;
typedef int32_t TransactionID;

class SPIDisplay {
public:
    SPIDisplay(size_t index); // displays 0, 1, ...
    ~SPIDisplay();

    // streaming interface
    // set image size -- calculate offsets
    // this is the SPI access functions, not the async feeder model
    // no backlight control

    size_t height() const;
    size_t width() const;

    // To stream video, call begin_frame, call send_stripe with
    // whatever size stripes you want, then call end_frame.
    // Repeat for every frame at your own pace.
    // send_stripe blocks.  begin_frame() and end_frame() don't
    // send_stripe returns a transaction ID; clients should
    // call await_transaction before reusing the pixel memory.

    void begin_frame_centered(size_t width, size_t height);
    void begin_frame(size_t width, size_t height, 
                     size_t x_offset, size_t y_offset);
    void end_frame();
    TransactionID send_stripe(size_t y, size_t height, ScreenPixelType *pixels);
    void await_transaction(TransactionID);

private:
    SPIDisplay(const SPIDisplay&) = delete;
    void operator = (const SPIDisplay&) = delete;

    Display *m_display;
    size_t m_display_height;
    size_t m_display_width;
    size_t m_frame_height;
    size_t m_frame_width;
    size_t m_frame_top;
    size_t m_frame_left;
    size_t m_frame_bottom;
    size_t m_frame_right;
    bool m_in_frame;
    bool m_frame_ready;
    uint8_t m_frame;
    size_t m_current_y;

};

// For testing.
extern void test_init_string();
