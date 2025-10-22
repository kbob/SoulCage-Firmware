// This file's header
#include "spi_display.h"

// C++ standard headers
#include <cassert>
#include <climits>
#include <cstring>

// ESP-IDF headers
#include "driver/spi_master.h"

// Component headers
#include "board_defs.h"
#include "display_controllers.h"
#include "driver_display.h"

SPIDisplay::SPIDisplay()
: m_display_height(DISPLAY_HEIGHT),
  m_display_width(DISPLAY_WIDTH),
  m_in_frame(false),
  m_frame_ready(false),
  m_frame(0),
  m_current_y(0)
{
    SPIDisplayDesc desc = {
        .height = m_display_height,
        .width = m_display_width,

        .spi_host = DISPLAY_SPI_HOST,
        .spi_clock_speed = DISPLAY_SPI_CLOCK_SPEED,
        .ctlr = DISPLAY_CTLR,

        .sclk_gpio = DISPLAY_SCLK_GPIO,
        .pico_gpio = DISPLAY_PICO_GPIO,
        .cs_gpio = DISPLAY_CS_GPIO,
        .dc_gpio = DISPLAY_DC_GPIO,
        .reset_gpio = DISPLAY_RESET_GPIO,
    };
    m_driver = new SPIDisplayDriver(desc);
}

SPIDisplay::~SPIDisplay()
{
    delete m_driver;
}

void SPIDisplay::begin_frame_centered(size_t width, size_t height)
{
    assert(width <= m_display_width);
    assert(height <= m_display_height);
    size_t x_offset = (m_display_width - width) / 2;
    size_t y_offset = (m_display_height - height) / 2;
    begin_frame(width, height, x_offset, y_offset);
}

void SPIDisplay::begin_frame(size_t width, size_t height,
                    size_t x_offset, size_t y_offset)
{
    // printf("begin_frame(w=%zu, h=%zu, xo=%zu, y0=%zu)\n",
    //     width, height, x_offset, y_offset);
    assert(width + x_offset <= m_display_width);
    assert(height + y_offset <= m_display_height);
    assert(!m_in_frame);
    m_in_frame = true;
    m_frame_width = width;
    m_frame_height = height;
    m_frame_left = x_offset;
    m_frame_top = y_offset;
    m_frame_right = x_offset + width;
    m_frame_bottom = y_offset + height;
    // printf("begin_frame: left=%zu right=%zu top=%zu bottom=%zu\n",
    //     m_frame_left, m_frame_right, m_frame_top, m_frame_bottom);
    m_frame_ready = true;
}

void SPIDisplay::end_frame()
{
    assert(m_in_frame);
    m_in_frame = false;
}

// This should be called a data header or something
struct Transaction {

    enum State {
        IDLE,
        BUSY,
    };

    State m_state;
    uint8_t m_frame;
    uint16_t m_y;
    spi_transaction_t m_trans;

    Transaction()
    : m_state(IDLE),
      m_frame(0),
      m_y(0),
      m_trans{}
    {}

    explicit operator TransactionID() const
    {
        ptrdiff_t index = this - s_pool;
        assert(index <= 0x7F && index < TRANSACTION_POOL_COUNT);
        return TransactionID(index << 24 | m_frame << 16 | m_y);
    }

    void enqueue_write(SPIDisplayDriver *driver, uint8_t *data, size_t byte_count)
    {
        m_state = BUSY;
        m_trans.tx_buffer = data;
        m_trans.length = byte_count * 8;
        driver->enqueue_transaction(&m_trans);
    }

    static Transaction *find_ID(TransactionID id)
    {
        size_t index = id >> 24;
        assert(index <= 0x7F && index < TRANSACTION_POOL_COUNT);
        uint8_t frame = id >> 16 & 0xFF;
        uint16_t y = id & 0xFFFF;
        Transaction *trans = s_pool + index;
        if (trans->m_frame == frame && trans->m_y == y) {
            return trans;
        }
        return nullptr;
    }

    static Transaction *get_idle_transaction(SPIDisplayDriver *driver)
    {
        Transaction *trans = &s_pool[s_idle_rotor];
        while (trans->m_state == BUSY) {
            spi_transaction_t *trans_desc = driver->await_transaction();
            assert(trans_desc == &trans->m_trans);
            trans->m_state = IDLE;
        }
        s_idle_rotor = (s_idle_rotor + 1) % TRANSACTION_POOL_COUNT;
        return trans;
    }

    const char *state_name()
    {
        switch (m_state) {
        case IDLE: return "IDLE";
        case BUSY: return "BUSY";
        default: return "?..?";
        }
    }
    static void await_all_transactions(SPIDisplayDriver *driver)
    {
        while (s_pool[s_idle_rotor].m_state != IDLE) {
            (void)get_idle_transaction(driver);
        }
    }

    static const size_t TRANSACTION_POOL_COUNT = 6;
    static Transaction s_pool[TRANSACTION_POOL_COUNT];
    static size_t s_idle_rotor;
};

Transaction Transaction::s_pool[TRANSACTION_POOL_COUNT];
size_t Transaction::s_idle_rotor;

TransactionID SPIDisplay::send_stripe(size_t y, size_t height, ScreenPixelType *pixels)
{
    // printf("send_stripe\n");
    assert(m_in_frame);
    if (m_frame_ready) {
        Transaction::await_all_transactions(m_driver);
        m_driver->send_start_write_command(
            m_frame_left, m_frame_top,
            m_frame_right, m_frame_bottom
        );
        m_frame++;
        m_frame_ready = false;
        m_current_y = 0;
    }
    assert(y == m_current_y);
    Transaction *trans = Transaction::get_idle_transaction(m_driver);
    trans->m_frame = m_frame;
    trans->m_y = m_current_y;
    size_t byte_count = height * m_frame_width * sizeof pixels[0];
    trans->enqueue_write(m_driver, (uint8_t *)pixels, byte_count);
    m_current_y += height;
    return (TransactionID)*trans;
}

void SPIDisplay::await_transaction(TransactionID id)
{
    Transaction *trans = Transaction::find_ID(id);
    assert(trans != nullptr);
    while (trans->m_state == Transaction::BUSY) {
        Transaction::get_idle_transaction(m_driver);
    }
}
