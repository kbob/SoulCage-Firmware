FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/main/*.*)

set(priv_requirements
    bootloader_support
    esp_adc
    esp_driver_gpio
    esp_driver_ledc
    esp_driver_spi
    esp_hw_support
    esp_partition
    esp_psram
    esp_timer
)

idf_component_register(SRCS ${app_sources}
                       PRIV_INCLUDE_DIRS "include"
                       PRIV_REQUIRES ${priv_requirements})
