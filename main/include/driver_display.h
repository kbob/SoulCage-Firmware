#pragma once

#include <cstdint>
#include "driver/gpio.h"
#include "display_controllers.h"
#include "driver/spi_master.h"

struct SPIDisplayDesc {
        size_t height;
    size_t width;

    spi_host_device_t spi_host;
    uint32_t spi_clock_speed;
    const DisplayController& ctlr;

    gpio_num_t sclk_gpio;
    gpio_num_t pico_gpio;
    gpio_num_t cs_gpio; 
    gpio_num_t dc_gpio;
    gpio_num_t reset_gpio;
    // Could add...
    //   feature flags: VE, bidirectional, SPI width...
};

class SPIDisplayDriver : private DisplayController::InitOps {

public:
    SPIDisplayDriver(SPIDisplayDesc&);
    ~SPIDisplayDriver();

    // DMA interface
    void send_start_write_command(
        uint16_t x0, uint16_t y0,
        uint16_t x1, uint16_t y1);
    void enqueue_transaction(spi_transaction_t *);
    spi_transaction_t *await_transaction();

private:
    SPIDisplayDriver(const SPIDisplayDriver&) = delete;
    void operator = (const SPIDisplayDriver&) = delete;

    // initialization
    void init();
    void init_SPI_peripheral();
    void init_display_controller();

    // polling interface, including InitOps methods
    void delay_msec(uint32_t msec) override;
    void write_command(uint8_t cmd) override;
    void write_data(const uint8_t *data, size_t count) override;
    void write_address(uint16_t addr1, uint16_t addr2);
    void write_bytes(const uint8_t *bytes, size_t count);

    SPIDisplayDesc m_desc;
    spi_device_handle_t m_device_handle;
};
