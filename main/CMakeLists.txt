FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/main/*.*)

# set(srcs "main.cpp"
# )

set(priv_requirements
    driver
    esp_hw_support
    esp_partition
    esp_psram
    esp_timer
)

idf_component_register(SRCS ${app_sources}
                       PRIV_INCLUDE_DIRS "include"
                       PRIV_REQUIRES ${priv_requirements})
