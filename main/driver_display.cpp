// This file's header
#include "driver_display.h"

// ESP-IDF headers
#include "esp_err.h"

// Component headers
#include "board_defs.h"         // for UNDEFINED_GPIO

enum {
    SPI_COMMAND_MODE = 0,
    SPI_DATA_MODE = 1,
};

bool spi_verbose = false;


// //  //   //    //     //      //       //      //     //    //   //  // //
// Initialization

SPIDisplayDriver::SPIDisplayDriver(SPIDisplayDesc& desc)
: m_desc(desc),
  m_device_handle(0)
{
    init();
}

SPIDisplayDriver::~SPIDisplayDriver()
{
    // spi_bus_remove_device
    ESP_ERROR_CHECK(
        spi_bus_remove_device(m_device_handle)
    );

    // spi_bus_free
    ESP_ERROR_CHECK(
        spi_bus_free(m_desc.spi_host)
    );

    // gpio_resets
    if (m_desc.reset_gpio != UNDEFINED_GPIO) {
        ESP_ERROR_CHECK(gpio_reset_pin(m_desc.reset_gpio));
    }
    ESP_ERROR_CHECK(gpio_reset_pin(m_desc.dc_gpio));
    ESP_ERROR_CHECK(gpio_reset_pin(m_desc.cs_gpio));
}

void SPIDisplayDriver::init()
{
    init_SPI_peripheral();
    init_display_controller();
}

void SPIDisplayDriver::init_SPI_peripheral()
{
    // CS GPIO explicit control
    gpio_reset_pin(m_desc.cs_gpio);
    gpio_set_direction(m_desc.cs_gpio, GPIO_MODE_OUTPUT);
    gpio_set_level(m_desc.cs_gpio, 0);

    // DC GPIO explicit control
    gpio_reset_pin(m_desc.dc_gpio);
    gpio_set_direction(m_desc.dc_gpio, GPIO_MODE_OUTPUT);
    gpio_set_level(m_desc.dc_gpio, 0);

    // Reset the controller if it has a reset line.
    if (m_desc.reset_gpio != UNDEFINED_GPIO) {
        gpio_reset_pin(m_desc.reset_gpio);
        gpio_set_direction(m_desc.reset_gpio, GPIO_MODE_OUTPUT);
        gpio_set_level(m_desc.reset_gpio, 1);
        delay_msec(100);
        gpio_set_level(m_desc.reset_gpio, 0);
        delay_msec(100);
        gpio_set_level(m_desc.reset_gpio, 1);
        delay_msec(100);
    }

    // Configure the SPI bus.
    spi_bus_config_t bus_config = {};
    bus_config.mosi_io_num = m_desc.pico_gpio;
    bus_config.miso_io_num = -1;
    bus_config.sclk_io_num = m_desc.sclk_gpio;
    bus_config.quadwp_io_num = -1;
    bus_config.quadhd_io_num = -1;

    ESP_ERROR_CHECK(
        spi_bus_initialize(m_desc.spi_host, &bus_config, SPI_DMA_CH_AUTO)
    );

    // Configure the device interface (the channel to the controller chip).
    spi_device_interface_config_t dev_config = {};
    dev_config.mode = 3;        // CPOL=1, CPHA=1
    dev_config.clock_speed_hz = m_desc.spi_clock_speed;
    dev_config.spics_io_num = (int)m_desc.cs_gpio;
    dev_config.flags = SPI_DEVICE_NO_DUMMY;
    dev_config.queue_size = 7;  // tunable

    spi_device_handle_t dev_handle;
    ESP_ERROR_CHECK(
        spi_bus_add_device(m_desc.spi_host, &dev_config, &dev_handle)
    );

    size_t max_bytes;
    ESP_ERROR_CHECK(
        spi_bus_get_max_transaction_len(m_desc.spi_host, &max_bytes)
    );

    m_device_handle = dev_handle;
}

void SPIDisplayDriver::init_display_controller()
{
    m_desc.ctlr.execute_init_string(*this);
}


// //  //   //    //     //      //       //      //     //    //   //  // //
// Polling Interface

void SPIDisplayDriver::write_command(uint8_t cmd)
{
    static uint8_t byte;
    byte = cmd;
    if (spi_verbose) {
        printf("write_command(disp, cmd=%#x)\n", cmd);
        printf("    byte = %#02x\n", byte);
    }
    gpio_set_level(m_desc.dc_gpio, SPI_COMMAND_MODE);
    write_bytes(&byte, 1);
}

void SPIDisplayDriver::write_data(const uint8_t *data, size_t count)
{
    if (spi_verbose) {
        printf("write_data([");
        for (size_t i = 0; i < count; i++) {
            printf("%s%02x", " " + !i, data[i]);
        }
        printf("])\n");
    }
    gpio_set_level(m_desc.dc_gpio, SPI_DATA_MODE);
    write_bytes(data, count);
}

void SPIDisplayDriver::delay_msec(uint32_t ms)
{
    TickType_t ticks = pdMS_TO_TICKS(ms);
    if (spi_verbose) {
        printf("delaying %" PRIu32 " ms = %" PRIu32 " ticks\n", ms, ticks);
    }
    vTaskDelay(ticks);
}

void SPIDisplayDriver::write_address(uint16_t addr1, uint16_t addr2)
{
    static uint8_t bytes[4];
    bytes[0] = (uint8_t)(addr1 >> 8);
    bytes[1] = (uint8_t)addr1;
    bytes[2] = (uint8_t)(addr2 >> 8);
    bytes[3] = (uint8_t)addr2;

    write_data(bytes, sizeof bytes);
}

void SPIDisplayDriver::write_bytes(const uint8_t *bytes, size_t count)
{
    if (spi_verbose) {
        printf("write_bytes: [");
        for (size_t i = 0; i < count; i++) {
            printf("%s%02x", " " + !i, bytes[i]);
        }
        printf("]\n");
    }
    spi_transaction_t trans_desc = {};
    trans_desc.length = count * 8;
    trans_desc.tx_buffer = bytes;
    ESP_ERROR_CHECK(
        spi_device_transmit(m_device_handle, &trans_desc)
    );
}

void SPIDisplayDriver::send_start_write_command(
    uint16_t x0, uint16_t y0,
    uint16_t x1, uint16_t y1)
{
    const DisplayController &ctlr = m_desc.ctlr;
    x0 += ctlr.CASET_x0_adjust;
    x1 += ctlr.CASET_x1_adjust;
    y0 += ctlr.RASET_y0_adjust;
    y1 += ctlr.RASET_y1_adjust;

    gpio_set_level(m_desc.dc_gpio, SPI_COMMAND_MODE);

    // spi_verbose = true;
    write_command(CASET);
    write_address(x0, x1);
    write_command(PASET);
    write_address(y0, y1);
    write_command(RAMWR);
    // spi_verbose = false;

    gpio_set_level(m_desc.dc_gpio, SPI_DATA_MODE);
}


// //  //   //    //     //      //       //      //     //    //   //  // //
// DMA Interface

void SPIDisplayDriver::enqueue_transaction(spi_transaction_t *trans)
{
    // const uint8_t *tb = (const uint8_t *)trans_desc->tx_buffer;
    // printf("enqueue_transaction: %u bytes @ %p [%x %x %x %x...]\n",
    //     trans_desc->length, tb, tb[0], tb[1], tb[2], tb[3]);
    if (trans->length > SPI_MAX_DMA_LEN * CHAR_BIT) {
        printf("transaction is %zu bytes\n", trans->length / CHAR_BIT);
    }
    assert(trans->length <= SPI_MAX_DMA_LEN * CHAR_BIT);
    TickType_t ticks_to_wait = pdMS_TO_TICKS(1000);
    ESP_ERROR_CHECK(
        spi_device_queue_trans(m_device_handle, trans, ticks_to_wait)
    );
}

spi_transaction_t *SPIDisplayDriver::await_transaction()
{
    spi_transaction_t *trans = nullptr;
    TickType_t ticks_to_wait = pdMS_TO_TICKS(1000);
    ESP_ERROR_CHECK(
        spi_device_get_trans_result(m_device_handle, &trans, ticks_to_wait)
    );
    return trans;
}
