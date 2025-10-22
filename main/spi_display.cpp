// This file's header
#include "spi_display.h"

#define NEW_CTLR

// C++ standard headers
#include <cassert>
#include <climits>
#include <cstring>

// ESP-IDF headers
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

// Project headers

// Component headers
#include "board_defs.h"
#include "display_controllers.h"
#include "driver_display.h"

// // Should move these to a central location for board info.
// struct DisplayDescriptor {
//     static const gpio_num_t GPIO_UNDEFINED = GPIO_NUM_MAX;
//     size_t height;
//     size_t width;
//     spi_host_device_t spi_host;
//     // The GPIO driver wants gpio_num_t, but the SPI driver wants int8_t.
//     // So we define some of each.
//     int8_t sclk_gpio;
//     int8_t pico_gpio;
//     gpio_num_t cs_gpio; 
//     gpio_num_t dc_gpio;
//     gpio_num_t reset_gpio;
//     uint32_t spi_clock_speed;
//     const DisplayController& ctlr;
//     // Could add...
//     //   feature flags: VE, bidirectional, SPI width...
// };

// // Keep in this file
// struct Display {
//     const SPIDisplayDescriptor& desc;
//     spi_device_handle_t dev_handle;
// };

#ifndef NEW_CTLR

// Board definitions.  Extend as needed.
#ifdef CONFIG_BOARD_WAVESHARE_ESP32_S3_LCD_TOUCH_1_69

static Display displays[] = {
    [0] = {
        // Display 0 (and only)
        .desc = {
            .height = 280,
            .width = 240,
            .spi_host = SPI2_HOST,
            .spi_clock_speed = 80'000'000,
            .ctlr = ST7789v2_controller,
            .sclk_gpio = GPIO_NUM_6,
            .pico_gpio = GPIO_NUM_7,
            .cs_gpio = GPIO_NUM_5,
            .dc_gpio = GPIO_NUM_4,
            .reset_gpio = GPIO_NUM_8,
        },
        .dev_handle = 0,
    },
};

#elif defined(CONFIG_BOARD_WAVESHARE_ESP32_S3_LCD_1_28)

// GC9A01

static Display displays[] = {
    [0] = {
        .desc = {
            .height = 240,
            .width = 240,
            .spi_host = SPI2_HOST,
            .sclk_gpio = 10,
            .pico_gpio = 11,
            .cs_gpio = GPIO_NUM_9,
            .dc_gpio = GPIO_NUM_8,
            .reset_gpio = GPIO_NUM_12,
            .spi_clock_speed = 80'000'000,
            .ctlr = GC9A01_controller,
        },
        .dev_handle = 0,
    },
};

#endif

static bool spi_verbose;

static void delay_msec(int ms)
{
    TickType_t ticks = ms / portTICK_PERIOD_MS + 1; // round up
    if (spi_verbose) {
        printf("delaying %d ms = %ld ticks\n", ms, ticks);
    }
    vTaskDelay(ticks);
}

static void init_display_SPI(Display &display)
{
    // printf("init_display_SPI\n");
    const SPIDisplayDescriptor desc = display.desc;

    // CS GPIO explicit control
    gpio_reset_pin(desc.cs_gpio);
    gpio_set_direction(desc.cs_gpio, GPIO_MODE_OUTPUT);
    gpio_set_level(desc.cs_gpio, 0);

    // DC GPIO explicit control
    gpio_reset_pin(desc.dc_gpio);
    gpio_set_direction(desc.dc_gpio, GPIO_MODE_OUTPUT);
    gpio_set_level(desc.dc_gpio, 0);

    // Reset the controller if it has a reset line.
    if (desc.reset_gpio >= 0) {
        gpio_reset_pin(desc.reset_gpio);
        gpio_set_direction(desc.reset_gpio, GPIO_MODE_OUTPUT);
        gpio_set_level(desc.reset_gpio, 1);
        delay_msec(100);
        gpio_set_level(desc.reset_gpio, 0);
        delay_msec(100);
        gpio_set_level(desc.reset_gpio, 1);
        delay_msec(100);
    }

    // Configure the SPI bus.
    spi_bus_config_t bus_config = {};
    memset(&bus_config, 0, sizeof bus_config);
    bus_config.mosi_io_num = desc.pico_gpio;
    bus_config.miso_io_num = -1;
    bus_config.sclk_io_num = desc.sclk_gpio;
    bus_config.quadwp_io_num = -1;
    bus_config.quadhd_io_num = -1;

    ESP_ERROR_CHECK(
        spi_bus_initialize(desc.spi_host, &bus_config, SPI_DMA_CH_AUTO)
    );

    // Configure the device interface (the channel to the controller chip).
    spi_device_interface_config_t dev_config = {};
    dev_config.mode = 3;        // CPOL=1, CPHA=1
    dev_config.clock_speed_hz = desc.spi_clock_speed;
    dev_config.spics_io_num = (int)desc.cs_gpio;
    dev_config.flags = SPI_DEVICE_NO_DUMMY;
    dev_config.queue_size = 7;  // tunable

    spi_device_handle_t dev_handle;
    ESP_ERROR_CHECK(
        spi_bus_add_device(desc.spi_host, &dev_config, &dev_handle)
    );

    size_t max_bytes;
    ESP_ERROR_CHECK(
        spi_bus_get_max_transaction_len(desc.spi_host, &max_bytes)
    );

    display.dev_handle = dev_handle;
}

static void free_display_SPI(Display &display)
{
    printf("free_display_SPI\n");
    const DisplayDescriptor& desc = display.desc;
    // spi_bus_remove_device
    ESP_ERROR_CHECK(
        spi_bus_remove_device(display.dev_handle)
    );

    // spi_bus_free
    ESP_ERROR_CHECK(
        spi_bus_free(desc.spi_host)
    );

    // gpio_resets
    if (desc.reset_gpio >= 0) {
        ESP_ERROR_CHECK(gpio_reset_pin(desc.reset_gpio));
    }
    ESP_ERROR_CHECK(gpio_reset_pin(desc.dc_gpio));
    ESP_ERROR_CHECK(gpio_reset_pin(desc.cs_gpio));
}

enum {
    SPI_COMMAND_MODE = 0,
    SPI_DATA_MODE = 1,
};

static void spi_write_bytes(Display& disp, const uint8_t *bytes, size_t byte_count)
{
    if (spi_verbose) {
        printf("spi_write_bytes: [");
        for (size_t i = 0; i < byte_count; i++) {
            printf("%s%02x", " " + !i, bytes[i]);
        }
        printf("]\n");
    }
    spi_transaction_t trans_desc;
    std::memset(&trans_desc, 0, sizeof trans_desc);
    trans_desc.length = byte_count * 8;
    trans_desc.tx_buffer = bytes;
    ESP_ERROR_CHECK(
        spi_device_transmit(disp.dev_handle, &trans_desc)
    );
}

static void spi_write_command(Display& disp, uint8_t cmd)
{
    static uint8_t byte;
    byte = cmd;
    if (spi_verbose) {
        printf("spi_write_command(disp, cmd=%#x)\n", cmd);
        printf("    byte = %#02x\n", byte);
    }
    gpio_set_level(disp.desc.dc_gpio, SPI_COMMAND_MODE);
    spi_write_bytes(disp, &byte, 1);
}

void spi_write_data_byte(Display& disp, uint8_t data)
{
    static uint8_t byte;
    byte = data;
    gpio_set_level(disp.desc.dc_gpio, SPI_DATA_MODE);
    spi_write_bytes(disp, &byte, 1);
}

static void spi_write_addr(Display& disp, uint16_t addr1, uint16_t addr2)
{
    static uint8_t bytes[4];
    bytes[0] = (uint8_t)(addr1 >> 8);
    bytes[1] = (uint8_t)addr1;
    bytes[2] = (uint8_t)(addr2 >> 8);
    bytes[3] = (uint8_t)addr2;

    gpio_set_level(disp.desc.dc_gpio, SPI_DATA_MODE);
    spi_write_bytes(disp, bytes, sizeof bytes);
}

static void spi_send_start_write_command(
    Display& disp,
    uint16_t x0, uint16_t y0,
    uint16_t x1, uint16_t y1)
{
    const DisplayController &ctlr = disp.desc.ctlr;
    x0 += ctlr.CASET_x0_adjust;
    x1 += ctlr.CASET_x1_adjust;
    y0 += ctlr.RASET_y0_adjust;
    y1 += ctlr.RASET_y1_adjust;
    gpio_set_level(disp.desc.dc_gpio, SPI_COMMAND_MODE);

    // spi_verbose = true;
    spi_write_command(disp, 0x2A);
    spi_write_addr(disp, x0, x1);
    spi_write_command(disp, 0x2B);
    spi_write_addr(disp, y0, y1);
    spi_write_command(disp, 0x2C);
    // spi_verbose = false;

    gpio_set_level(disp.desc.dc_gpio, SPI_DATA_MODE);
}

static void spi_enqueue_transaction(Display& disp, spi_transaction_t *trans_desc)
{
    // const uint8_t *tb = (const uint8_t *)trans_desc->tx_buffer;
    // printf("spi_enqueue_transaction: %u bytes @ %p [%x %x %x %x...]\n",
    //     trans_desc->length, tb, tb[0], tb[1], tb[2], tb[3]);
    if (trans_desc->length > SPI_MAX_DMA_LEN * CHAR_BIT) {
        printf("transaction is %zu bytes\n", trans_desc->length / CHAR_BIT);
    }
    assert(trans_desc->length <= SPI_MAX_DMA_LEN * CHAR_BIT);
    spi_device_handle_t handle = disp.dev_handle;
    TickType_t ticks_to_wait = pdMS_TO_TICKS(1000);
    ESP_ERROR_CHECK(
        spi_device_queue_trans(handle, trans_desc, ticks_to_wait)
    );
}

static spi_transaction_t *spi_await_transaction(Display& disp)
{
    spi_device_handle_t dev_handle = disp.dev_handle;
    spi_transaction_t *trans_desc = nullptr;
    TickType_t ticks_to_wait = pdMS_TO_TICKS(1000);
    ESP_ERROR_CHECK(
        spi_device_get_trans_result(dev_handle, &trans_desc, ticks_to_wait)
    );
    return trans_desc;
}

class SPIDisplayInitOps : public DisplayController::InitOps {

public:
    SPIDisplayInitOps(Display& disp)
    : m_disp(disp)
    {}

    void write_command(uint8_t cmd) override
    {
        spi_write_command(m_disp, cmd);
    }

    void write_data(const uint8_t *data, size_t size) override
    {
        for (size_t i = 0; i < size; i++) {
            spi_write_data_byte(m_disp, data[i]);
        }
    }

    void delay_msec(uint32_t msec) override
    {
        ::delay_msec(msec);
    }

private:
    Display& m_disp;

};

static void init_LCD(Display& disp)
{
    SPIDisplayInitOps init_ops(disp);
    disp.desc.ctlr.execute_init_string(init_ops);
}

#endif

SPIDisplay::SPIDisplay()
: m_display_height(DISPLAY_HEIGHT),
  m_display_width(DISPLAY_WIDTH),
  m_in_frame(false),
  m_frame_ready(false),
  m_frame(0),
  m_current_y(0)
{
#ifdef NEW_CTLR
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
#else
    const size_t DISPLAY_COUNT = sizeof displays / sizeof displays[0];
    assert(index < DISPLAY_COUNT);
    m_display = &displays[index];
    init_display_SPI(*m_display);
    init_LCD(*m_display);
#endif
}

SPIDisplay::~SPIDisplay()
{
#ifdef NEW_CTLR
    delete m_driver;
#else
    free_display_SPI(*m_display);
#endif
}

#ifdef NEW_CTLR

// size_t SPIDisplay::height() const { return m_display_height; }
// size_t SPIDisplay::width() const { return m_display_width; }

#else

size_t SPIDisplay::height() const { return m_display->desc.height; }

size_t SPIDisplay::width() const { return m_display->desc.width; }

#endif

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

typedef int32_t TransactionID;

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


#if 0

// //  //   //    //     //      //       //      //     //    //   //  // //
// Init String Test

class TestOps : public DisplayController::InitOps {
public:
    void write_command(uint8_t cmd) override
    {
        printf("write_command cmd=%#02x\n", cmd);
    }
    void write_data(const uint8_t *data, size_t size) override
    {
        printf("write_data    data={");
        const char *space = "";
        for (size_t i = 0; i < size; i++) {
            printf("%s%#02x", space, data[i]);
            space = " ";
        }
        printf("} size=%zu\n", size);
    }

    void delay_msec(uint32_t msec) override
    {
        printf("delay         %lu msec\n", msec);
    }

    void error() override
    {
        printf("ERROR!\n\n");
    }
};

void test_init_string()
{
    TestOps ops;
    bool ok = ST7789v2_controller.execute_init_string(ops);
    printf("exec returned %s\n", ok ? "true" : "false");
}

#endif
