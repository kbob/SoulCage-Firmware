// This file's header
#include "driver_battery.h"

// C++ standard headers
#include <cstdio>

// ESP-IDF headers
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"

// Component headers
#include "board_defs.h"

struct BatteryDriver_private {

    adc_cali_handle_t m_cali_handle;
    adc_oneshot_unit_handle_t m_unit_handle;
    adc_channel_t m_channel;
    float m_scale;

    BatteryDriver_private()
    : m_cali_handle(0),
      m_unit_handle(0),
      m_scale(0)
    {
        // Look up unit and channel

        const gpio_num_t GPIO = BATTERY_ADC_GPIO;
        const adc_unit_t UNIT = gpio_to_unit(GPIO);
        const adc_atten_t ATTEN = ADC_ATTEN_DB_2_5;
        m_channel = gpio_to_channel(GPIO);

        // calculate voltage scale: convert mV at ADC to V at battery
        float above = BATTERY_ADC_R_ABOVE;
        float below = BATTERY_ADC_R_BELOW;
        m_scale = 0.001f * (above + below) / below;
        
        // calibrate
        adc_cali_scheme_ver_t scheme_mask;
        ESP_ERROR_CHECK(adc_cali_check_scheme(&scheme_mask));
        switch (scheme_mask) {

#ifdef ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED            
        case ADC_CALI_SCHEME_VER_LINE_FITTING:
            {
                adc_cali_line_fitting_config_t cali_cfg = {
                    .unit_id = UNIT,
                    .chan = m_channel,
                    .atten = ATTEN,
                    .bitwidth = ADC_BITWIDTH_DEFAULT,
                };
                ESP_ERROR_CHECK(
                    adc_cali_create_scheme_line_fitting(
                        &cali_cfg,
                        &m_cali_handle)
                );
            }
            break;
#endif

#ifdef ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
        case ADC_CALI_SCHEME_VER_CURVE_FITTING:
            {
                adc_cali_curve_fitting_config_t cali_cfg = {
                    .unit_id = UNIT,
                    .chan = m_channel,
                    .atten = ATTEN,
                    .bitwidth = ADC_BITWIDTH_DEFAULT,
                };
                ESP_ERROR_CHECK(
                    adc_cali_create_scheme_curve_fitting(
                        &cali_cfg,
                        &m_cali_handle)
                );
            }
            break;
#endif

        default:
            printf("Unknown scheme mask %#x\n", (int)scheme_mask);
            assert(false);
        }

        // init ADC unit
        adc_oneshot_unit_init_cfg_t unit_cfg = {
            .unit_id = UNIT,
            .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
            .ulp_mode = ADC_ULP_MODE_DISABLE,
        };
        ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &m_unit_handle));

        // init ADC channel
        adc_oneshot_chan_cfg_t chan_cfg = {
            .atten = ATTEN,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ESP_ERROR_CHECK(
            adc_oneshot_config_channel(m_unit_handle, m_channel, &chan_cfg)
        );
    }

    ~BatteryDriver_private()
    {
        ESP_ERROR_CHECK(adc_oneshot_del_unit(m_unit_handle));
        ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(m_cali_handle));
    }

    float voltage() const
    {
        int raw_reading, voltage_mV;

        ESP_ERROR_CHECK(
            adc_oneshot_read(m_unit_handle, m_channel, &raw_reading)
        );
        ESP_ERROR_CHECK(
            adc_cali_raw_to_voltage(m_cali_handle, raw_reading, &voltage_mV)
        );
        float voltage_V = m_scale * voltage_mV;
        printf("battery voltage: raw reading = %d\n", raw_reading);
        return voltage_V;
    }

private:
    static adc_unit_t gpio_to_unit(gpio_num_t gpio)
    {
#ifdef CONFIG_IDF_TARGET_ESP32S3
        switch (gpio) {

        case GPIO_NUM_1:
        case GPIO_NUM_2:
        case GPIO_NUM_3:
        case GPIO_NUM_4:
        case GPIO_NUM_5:
        case GPIO_NUM_6:
        case GPIO_NUM_7:
        case GPIO_NUM_8:
        case GPIO_NUM_9:
        case GPIO_NUM_10:
            return ADC_UNIT_1;

        case GPIO_NUM_11:
        case GPIO_NUM_12:
        case GPIO_NUM_13:
        case GPIO_NUM_14:
        case GPIO_NUM_15:
        case GPIO_NUM_16:
        case GPIO_NUM_17:
            return ADC_UNIT_2;

        default:
            assert(false && "unsupported ADC GPIO");
        }
#else
        #error "gpio_to_unit: unknown target"
#endif
    }

    static adc_channel_t gpio_to_channel(gpio_num_t gpio)
    {
#ifdef CONFIG_IDF_TARGET_ESP32S3
        switch (gpio) {

        case GPIO_NUM_1:
            return ADC_CHANNEL_0;

        case GPIO_NUM_2:
            return ADC_CHANNEL_1;

        case GPIO_NUM_3:
            return ADC_CHANNEL_2;

        case GPIO_NUM_4:
            return ADC_CHANNEL_3;

        case GPIO_NUM_5:
            return ADC_CHANNEL_4;

        case GPIO_NUM_6:
            return ADC_CHANNEL_5;

        case GPIO_NUM_7:
            return ADC_CHANNEL_6;

        case GPIO_NUM_8:
            return ADC_CHANNEL_7;

        case GPIO_NUM_9:
            return ADC_CHANNEL_8;

        case GPIO_NUM_10:
            return ADC_CHANNEL_9;

        case GPIO_NUM_11:
            return ADC_CHANNEL_0;

        case GPIO_NUM_12:
            return ADC_CHANNEL_1;

        case GPIO_NUM_13:
            return ADC_CHANNEL_2;

        case GPIO_NUM_14:
            return ADC_CHANNEL_3;

        case GPIO_NUM_15:
            return ADC_CHANNEL_4;

        case GPIO_NUM_16:
            return ADC_CHANNEL_5;

        case GPIO_NUM_17:
            return ADC_CHANNEL_6;

        default:
            assert(false && "gpio_to_channel: unsupported ADC GPIO");
        }
#else
        static_assert(false, "unknown target"
#endif
    }

    static int gpio_to_bit_width(gpio_num_t)
    {
#ifdef CONFIG_IDF_TARGET_ESP32S3
        return 12;
#else
        #error "gpio_to_bit_width: unknown target"
#endif        
    }
};

BatteryDriver::BatteryDriver()
: m_private(new BatteryDriver_private)
{
    assert(m_private);
}

BatteryDriver::~BatteryDriver()
{
    delete m_private;
}

float BatteryDriver::voltage() const
{
    return m_private->voltage();
}