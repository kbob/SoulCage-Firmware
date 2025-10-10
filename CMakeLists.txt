cmake_minimum_required(VERSION 3.22)

set(board waveshare-esp32-s3-lcd-touch-1.69)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
set(SDKCONFIG_DEFAULTS
    sdkconfig.${board}
    sdkconfig.defaults.esp32s3
    sdkconfig.defaults)
# idf_build_set_property(MINIMAL_BUILD ON)
set(COMPONENTS main partition_table custom)

project(SoulCage)

# Build binary video file partitions
idf_build_get_property(build_dir BUILD_DIR)
idf_build_get_property(python PYTHON)
# set(blank_file ${build_dir}/blank_file.bin)
set(Intro_file ${build_dir}/Intro.bin)
set(soul_f_file ${build_dir}/soul_f.bin)
set(soul_m_file ${build_dir}/soul_m.bin)

# add_custom_command(OUTPUT ${blank_file}
#     COMMAND echo "here we go"
#     COMMAND ${python} ${partition_table_dir}/gen_empty_partition.py
#     ${partition_size} ${blank_file})

add_custom_command(OUTPUT ${Intro_file}
    COMMAND echo "Here we go ${Intro_file}"
    COMMAND ${python}
    ${PROJECT_DIR}/tools/gen_raw_image.py
        -o ${Intro_file}
        ${PROJECT_DIR}/images/Intro.h)

add_custom_command(OUTPUT ${soul_f_file}
    COMMAND echo "Here we go ${soul_f_file}"
    COMMAND ${python}
    ${PROJECT_DIR}/tools/gen_raw_image.py
        -o ${soul_f_file}
        ${PROJECT_DIR}/images/soul_f.h)

add_custom_command(OUTPUT ${soul_m_file}
    COMMAND echo "Here we go ${soul_m_file}"
    COMMAND ${python}
    ${PROJECT_DIR}/tools/gen_raw_image.py
        -o ${soul_m_file}
        ${PROJECT_DIR}/images/soul_m.h)

add_custom_target(Intro_bin ALL DEPENDS ${Intro_file})
add_custom_target(soul_f_bin ALL DEPENDS ${soul_f_file})
add_custom_target(soul_m_bin ALL DEPENDS ${soul_m_file})

idf_build_get_property(target IDF_TARGET)
if(NOT ${target} STREQUAL "linux")
    add_dependencies(flash Intro_bin)
    add_dependencies(flash soul_f_bin)
    add_dependencies(flash soul_m_bin)
    esptool_py_flash_to_partition(flash "Intro" "${Intro_file}")
    esptool_py_flash_to_partition(flash "soul_f" "${soul_f_file}")
    esptool_py_flash_to_partition(flash "soul_m" "${soul_m_file}")
endif()
