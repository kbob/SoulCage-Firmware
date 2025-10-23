// This file's header
#include "random.h"

// ESP-IDF headers
#include "bootloader_random.h"
#include "esp_random.h"

Random Random::s_instance;

Random::Random()
{
    bootloader_random_enable();
}

unsigned Random::o_rand()
{
    static bool been_here;
    if (!been_here) {
        been_here = true;
        std::srand(esp_random());
        bootloader_random_disable();
    }
    return std::rand();
}
