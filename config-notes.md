# sdkconfig changes

## defaults

- partition table
- reboot delay

```
# This file was generated using idf.py save-defconfig. It can be edited manually.
# Espressif IoT Development Framework (ESP-IDF) 5.5.1 Project Minimal Configuration
#
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_ESP_SYSTEM_PANIC_REBOOT_DELAY_SECONDS=10
```
```
424c424
424c424
< CONFIG_PARTITION_TABLE_SINGLE_APP=y
---
> # CONFIG_PARTITION_TABLE_SINGLE_APP is not set
428c428
< # CONFIG_PARTITION_TABLE_CUSTOM is not set
---
> CONFIG_PARTITION_TABLE_CUSTOM=y
430c430
< CONFIG_PARTITION_TABLE_FILENAME="partitions_singleapp.csv"
---
> CONFIG_PARTITION_TABLE_FILENAME="partitions.csv"
681c681
< CONFIG_ESP_SYSTEM_PANIC_REBOOT_DELAY_SECONDS=0
---
> CONFIG_ESP_SYSTEM_PANIC_REBOOT_DELAY_SECONDS=10
```

## ESP32S3

- esptool flashmode qio
- flashmod qio
- CPU freq 240 MHz (or maybe slower for soul cage)

```
CONFIG_IDF_TARGET="esp32s3"
CONFIG_ESPTOOLPY_FLASHMODE_QIO=y
CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240=y
```

## Board

- enable spiram
- spiram mode
- spiram speed
- spiram size
- flash size
```
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y
CONFIG_SPIRAM=y
CONFIG_SPIRAM_SPEED_80M=y
CONFIG_SPIRAM_IGNORE_NOTFOUND=y
```