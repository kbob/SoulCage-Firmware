// This file's header
#include "random.h"

// ESP-IDF headers
#include "esp_random.h"

Random Random::s_instance;

Random::Random()
{
    std::srand(esp_random());
}
