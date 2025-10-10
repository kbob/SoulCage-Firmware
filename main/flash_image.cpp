// my header
#include "flash_image.h"

// C++ standard headers
#include <cstring>

// ESP-IDF headers
#include "esp_check.h"
#include "esp_partition.h"

FlashImage FlashImage::images[FlashImage::MAX_IMAGES];
size_t FlashImage::image_count;

FlashImage::~FlashImage()
{
    esp_partition_munmap(m_handle);
    m_label[0] = '\0';
}

FlashImage *FlashImage::get_by_label(const char *label)
{
    if (image_count == 0) {
        find_images();
    }
    for (size_t i = 0; i < image_count; i++) {
        FlashImage& image = images[i];
        if (!std::strcmp(label, image.m_label)) {
            return &image;
        }
    }
    return nullptr;
}

FlashImage *FlashImage::get_by_index(size_t index)
{
    if (image_count == 0) {
        find_images();
    }
    if (index >= image_count) {
        return nullptr;
    }
    return &images[index];
}

void FlashImage::find_images()
{
    // find all flash partitions of type data, subtype 0x40.
    // mmap eaach one.
    
    auto type = ESP_PARTITION_TYPE_DATA;
    auto subtype = (esp_partition_subtype_t) 0x40;
    auto label = nullptr;
    auto iter = esp_partition_find(type, subtype, label);
    image_count = 0;
    for (size_t i = 0; iter && i < MAX_IMAGES; i++) {

        // find a partition
        auto *part = esp_partition_get(iter);
        if (part == NULL) {
            break;
        }
        iter = esp_partition_next(iter);

        // Map it into our address space
        auto memory = ESP_PARTITION_MMAP_DATA;
        const void *ptr = nullptr;
        esp_partition_mmap_handle_t handle = 0;
        ESP_ERROR_CHECK(
            esp_partition_mmap(part, 0, part->size, memory, &ptr, &handle)
        );

        FlashImage *image = &images[i];
        strncpy(image->m_label, part->label, MAX_LABEL_SIZE);
        image->m_label[MAX_LABEL_SIZE] = '\0';
        image->m_addr = ptr;
        image->m_size = part->size;
        image->m_handle = handle;
        image_count++;
    }

    esp_partition_iterator_release(iter);
}

