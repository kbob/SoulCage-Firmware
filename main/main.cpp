// C++ standard headers
#include <cmath>
#include <cstdio>
#include <cstring>
#include <numbers>

// ESP-IDF and FreeRTOS headers
#include "driver/gpio.h"        // XXX remove me
#include "driver/ledc.h"
#include "esp_partition.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"

// Project headers

// Component headers
#include "flash_image.h"
#include "spi_display.h"


#define STATIC_FEATURE 1
#define FLICKER_FEATURE 1

#undef BENCHMARK_DMA

#ifdef BENCHMARK_DMA

    #define IN_FLIGHT 4
    #define REPS 32
    #define BIG (240 * 240 * sizeof (uint16_t))
    uint8_t destination[BIG];
    uint8_t source[BIG];

    static void benchmark_FLASH_DMA(const uint8_t *src, size_t count)
    {
        uint64_t before = esp_timer_get_time();
        for (int i = 0; i < IN_FLIGHT; i++) {
            MemoryDMA::async_memcpy(destination, (void *)src, BIG, NULL);
            src += BIG;
        }
        for (int i = IN_FLIGHT; i < count; i++) {
            uint32_t n = ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
            assert(n <= IN_FLIGHT);
            MemoryDMA::async_memcpy(destination, (void *)src, BIG, NULL);
            src += BIG;
        }
        for (int i = IN_FLIGHT; --i >= 0; ) {
            uint32_t n = ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        assert(n != 0);
        }
        uint64_t after = esp_timer_get_time();
        uint64_t dt_usec = after - before;
        uint64_t bytes = count * BIG;
        float Bps = (float)bytes / (float)dt_usec;
        float uspf = (float)dt_usec / (float)count;
        float fps = 1'000'000.0f / uspf;

        printf("FLASH to SRAM\n");
        printf("    copied %llu bytes in %llu usec: %g MB/sec\n",
               bytes, dt_usec, Bps);
        printf("    %g usec/frame, %g fps\n", uspf, fps);
        printf("\n");
    }

    static void benchmark_SRAM_DMA()
    {
        strcpy((char *)source, "Hello, World!");

        uint64_t before = esp_timer_get_time();
        for (int i = 0; i < IN_FLIGHT; i++) {
            MemoryDMA::async_memcpy(destination, source, BIG, NULL);
        }
        for (int i = IN_FLIGHT; i < REPS; i++) {
            uint32_t n = ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
            assert(n <= IN_FLIGHT);
            MemoryDMA::async_memcpy(destination, source, BIG, NULL);
        }
        for (int i = IN_FLIGHT; --i >= 0; ) {
            uint32_t n = ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        assert(n != 0);
        }
        uint64_t after = esp_timer_get_time();
        uint64_t dt_usec = after - before;
        uint64_t bytes = REPS * BIG;
        float Bps = (float)bytes / (float)dt_usec;
        float uspf = (float)dt_usec / (float)REPS;
        float fps = 1'000'000.0f / uspf;

        printf("SRAM to SRAM\n");
        printf("    copied %llu bytes in %llu usec: %g MB/sec\n",
               bytes, dt_usec, Bps);
        printf("    %g usec/frame, %g fps\n", uspf, fps);
        printf("\n");
    }

#endif

static void identify_board()
{
    #ifdef WAVESHARE_ESP32_S3_LCD_TOUCH_1_69
    printf("This is the Waveshare 1.69 board!\n");
#elif defined(WAVESHARE_ESP32_S3_LCD_1_28)
    printf("This is the Waveshare 1.28 board!\n");
#else
    printf("Is this an unknown board?");
#endif
    printf("board = \"%s\"\n", BOARD);
    printf("\n");
}

// PRNG - pseudo-random number generator
static void init_prng()
{
    std::srand(esp_random());
}

static int randint(int min, int max)
{
    // This PRNG is biased.  I don't care.
    return min + std::rand() % (max - min);
}


const uint32_t BIG_TICK_PERIOD_MSEC = 20;
TimerHandle_t big_tick_timer;
TaskHandle_t tick_handler_task;

void big_tick_callback(TimerHandle_t)
{
    BaseType_t higher_priority_task_woken = pdFALSE;
    vTaskNotifyGiveIndexedFromISR(tick_handler_task, 1, &higher_priority_task_woken);
    // return higher_priority_task_woken == pdTRUE;
}

void init_main_loop_timer()
{
    tick_handler_task = xTaskGetCurrentTaskHandle();
    const char *name = "big_tick";
    TickType_t period = pdMS_TO_TICKS(BIG_TICK_PERIOD_MSEC);
    BaseType_t auto_reload = pdTRUE;
    void *timer_ID = nullptr;
    big_tick_timer = xTimerCreate(name, period, auto_reload, timer_ID, big_tick_callback);
    xTimerStart(big_tick_timer, 2);
}

void wait_for_tick()
{
    UBaseType_t index = 1;
    BaseType_t clear_count = pdFALSE;
    TickType_t timeout = portMAX_DELAY;
    (void)ulTaskNotifyTakeIndexed(index, clear_count, timeout);
}

#ifdef WAVESHARE_ESP32_S3_LCD_TOUCH_1_69
    const gpio_num_t BACKLIGHT_GPIO = GPIO_NUM_15;
#elif defined(WAVESHARE_ESP32_S3_LCD_1_28)
    const gpio_num_t BACKLIGHT_GPIO = GPIO_NUM_40;
#else
    #error "unknown board - backlight GPIO not defined"
#endif

static const int ANIM_FRAMES = 50 * 70;
// static const int ANIM_FRAMES = 70;

#ifdef FLICKER_FEATURE

#define BACKLIGHT_SPOOKY 1


    struct Backlight {
        static const int MIN = 0;
        static const int MAX = (1 << 13);
        static constexpr float GAMMA = 2.8f;
#ifdef BACKLIGHT_SPOOKY
        // static const int ANIM_FRAMES = 50 * 70;
        static const int HARMONICS = 10;
        int frame;
        float scale;
#else
        static const int STEP = (MAX - MIN) / 100;
        int inc;
#endif
        ledc_mode_t mode;
        ledc_channel_t channel;
        int duty;

#ifdef BACKLIGHT_SPOOKY

        static float spooky_light(int frame) {
            float x = (float)frame / (float)ANIM_FRAMES;
            float acc = 0.0f;
            const float TAU = 2.0 * std::numbers::pi;
            for (int h = 1; h <= HARMONICS; h++) {
                acc += std::sinf(powf(h, 1.5) * x * TAU);
            }
            acc += 1.0f; // reduce the duty cycle a little
            return std::max(-acc, 0.0f); // dark part first
        }

#endif

        void update()
        {
#ifdef BACKLIGHT_SPOOKY
            frame = (frame + 1) % ANIM_FRAMES;
            float brightness = scale * spooky_light(frame);
            // printf("Bl::up: frame = %d, scale = %g, sl() = %g\n",
            //        frame, scale, spooky_light(frame));
            duty = (int)(brightness * (float)MAX);
            // printf("        duty = %d, brightness = %g, MAX = %d\n",
            //        duty, brightness, MAX);
#else
            duty += inc;
            if (duty > MAX) {
                duty = MAX;
                inc = -STEP;
            } else if (duty < MIN) {
                duty = MIN;
                inc = +STEP;
            }
#endif
#ifdef BACKLIGHT_USE_GAMMA
            float fduty = (float)duty / (float)MAX;
            float fmapped = std::powf(fduty, GAMMA);
            int32_t mapped_duty = (uint32_t)(fmapped * MAX);
#else
            int32_t mapped_duty = duty;            
#endif
            ESP_ERROR_CHECK(ledc_set_duty(mode, channel, mapped_duty));
            ESP_ERROR_CHECK(ledc_update_duty(mode, channel));
        }
    };
    static struct Backlight backlight;

    static void init_backlight()
    {
        const ledc_mode_t mode = LEDC_LOW_SPEED_MODE;        
        const ledc_timer_t timer = LEDC_TIMER_0;
        const ledc_channel_t channel = LEDC_CHANNEL_0;

        ledc_timer_config_t tc;
        std::memset(&tc, 0, sizeof tc);
        tc.speed_mode = mode;
        tc.duty_resolution = LEDC_TIMER_13_BIT;
        tc.timer_num = timer;
        tc.freq_hz = 4000;
        tc.clk_cfg = LEDC_AUTO_CLK;
        ESP_ERROR_CHECK(ledc_timer_config(&tc));

        ledc_channel_config_t cc;
        std::memset(&cc, 0, sizeof cc);
        cc.speed_mode = mode;
        cc.channel = channel;
        cc.timer_sel = timer;
        cc.intr_type = LEDC_INTR_DISABLE;
        cc.gpio_num = BACKLIGHT_GPIO;
        cc.duty = 0;
        cc.hpoint = 0;
        ESP_ERROR_CHECK(ledc_channel_config(&cc));

        backlight.mode = mode;
        backlight.channel = channel;
#ifdef BACKLIGHT_SPOOKY
        float max_y = 0.0f;
        for (int f = 0; f < ANIM_FRAMES; f++) {
            float y = Backlight::spooky_light(f);
            if (max_y < y) {
                max_y = y;
            }
        }
        printf("backlight_init: max_y = %g\n", max_y);
        backlight.frame = 0;
        backlight.scale = 1.0f / max_y;
#else
        backlight.duty = 0;
        backlight.inc = Backlight::STEP;
#endif
    }

    void update_backlight()
    {
        backlight.update();
    }

#else

    void init_backlight()
    {
        gpio_reset_pin(BACKLIGHT_GPIO);
        gpio_set_direction(BACKLIGHT_GPIO, GPIO_MODE_OUTPUT);
        gpio_set_level(BACKLIGHT_GPIO, 1);
    }

    void update_backlight() {}

#endif

static bool in_intro = true;
FlashImage *current_image;
size_t image_frames;
size_t current_frame;
const size_t IMAGE_BUFFER_COUNT = 2;
ScreenPixelType DMA_ATTR image_buffers[IMAGE_BUFFER_COUNT][FlashImage::IMAGE_HEIGHT][FlashImage::IMAGE_WIDTH];
size_t current_image_buffer;

void copy_image_to_screen_pixels(ScreenPixelType *dst, const FlashImage::pixel_type *src, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        dst[i] = ScreenPixelType::from_PackedColor(src[i]);
    }
}

void init_flash_images()
{
    current_image = FlashImage::get_by_label("Intro");
    assert(current_image != nullptr);
    image_frames = current_image->frame_count();

    int64_t before = esp_timer_get_time();

    auto *dst = *image_buffers[current_image_buffer];
    const auto *src = current_image->frame_addr(current_frame);
    assert(dst);
    assert(src);
    size_t pixel_count = FlashImage::FRAME_PIXEL_COUNT;
    copy_image_to_screen_pixels(dst, src, pixel_count);

    int64_t after = esp_timer_get_time();
    int32_t dt_usec = after - before;
    printf("memcpy in %ld usec\n", dt_usec);

    current_frame = (current_frame + 1) % image_frames;
}

const int PERCENT_ANIMATION_CHANGE = 70;

// This is sync'd with the backlight flicker because
// the backlight starts off dark for a few seconds.

void maybe_change_animation()
{
    static int frame;
    frame = (frame + 1) % ANIM_FRAMES;
    if (frame != 0) {
        return;
    }

    if (std::rand() % 100 < PERCENT_ANIMATION_CHANGE) {
        auto old_label = current_image->label();

        // Don't interrupt the intro.
        if (!std::strcmp(old_label, "Intro")) {
            return;
        }
        const char *new_label;
        if (!std::strcmp(old_label, "soul_m")) {
            new_label = "soul_f";
        } else {
            assert(strcmp(old_label, "soul_f") == 0);
            new_label = "soul_m";
        }
        current_image = FlashImage::get_by_label(new_label);
        assert(current_image != nullptr);
        image_frames = current_image->frame_count();
        current_frame = randint(0, image_frames);
    }
}

void update_animation()
{
    maybe_change_animation();
    // update the animation every 7 frames
    static int frame;
    frame = (frame + 1) % 7;
    if (frame != 0) {
        return;
    }

    // printf("update animation frame %zu\n", current_frame);
    size_t next_i_buf = (current_image_buffer + 1) % IMAGE_BUFFER_COUNT;
    auto *dst = *image_buffers[next_i_buf];
    const auto *src = current_image->frame_addr(current_frame);
    assert(dst);
    assert(src);
    size_t pixel_count = FlashImage::FRAME_PIXEL_COUNT;
    copy_image_to_screen_pixels(dst, src, pixel_count);
    current_image_buffer = next_i_buf;

    current_frame = (current_frame + 1) % image_frames;
    if (current_frame == 0 && in_intro) {
        // if we've finished the intro, start on one of the souls..
        in_intro = false;
        const char *next_image = std::rand() % 2 ? "soul_f" : "soul_m";
        current_image = FlashImage::get_by_label(next_image);
        assert(current_image != nullptr);
        image_frames = current_image->frame_count();
    }
}

#ifdef STATIC_FEATURE

    // These are in units of stripes.  The comments show approximate duration.

    #define MIN_STATIC  20      // 20: 280 msec, 80 scan lines
    #define MAX_STATIC 1200     // 1200: .5 seconds, 34 frames
    #define MIN_NO_STATIC 150   // 150: 60 msec, 4 frames
    #define MAX_NO_STATIC 4000  // 4000: 1.6 sec, 114 frames

    // returns true if caller should replace next stripe with static.
    bool update_static()
    {
        // "static" -- get it?
        static int static_active = 0;
        static int static_start = -1;
        if (static_start == -1) {
            // first time
            static_start = randint(MIN_NO_STATIC, MAX_NO_STATIC);
        } else if (!static_active && --static_start == 0) {
            static_active = randint(MIN_STATIC, MAX_STATIC);
        }
        if (static_active) {
            if (--static_active != 0) {
                return true;
            }
            static_start = randint(MIN_NO_STATIC, MAX_NO_STATIC);
        }
        return false;
    }

#endif

SPIDisplay *the_display;
TransactionID last_SPI_trans;
ScreenPixelType DMA_ATTR black_stripe[FlashImage::IMAGE_WIDTH];
const size_t STRIPE_HEIGHT = 8;

void init_SPI_display()
{
    the_display = new SPIDisplay(0);

    const size_t BLACK_STRIPE_HEIGHT = 1;

    // fill with black
    size_t w = the_display->width();
    size_t h = the_display->height();
    assert(w == 240);
    the_display->begin_frame(w, h, 0, 0);
    for (int y = 0; y < h; y += BLACK_STRIPE_HEIGHT) {
        last_SPI_trans = the_display->send_stripe(y, BLACK_STRIPE_HEIGHT, black_stripe);
    }
    the_display->end_frame();
}

#ifdef STATIC_FEATURE

    const size_t STATIC_STRIPE_COUNT = 6;
    ScreenPixelType DMA_ATTR static_stripes[STATIC_STRIPE_COUNT][STRIPE_HEIGHT][240];
    size_t static_rotor;

    void send_static_stripe(size_t y) {
        // std::srand(1);
        ScreenPixelType *stripe = *static_stripes[static_rotor];
        static_rotor = (static_rotor + 1) % STATIC_STRIPE_COUNT;
        for (size_t i = 0; i < STRIPE_HEIGHT * 240; i += 4) {
            int x = std::rand();
            stripe[i + 0] = ScreenPixelType::from_grey8(x >> 0 & 0xFF);
            stripe[i + 1] = ScreenPixelType::from_grey8(x >> 8 & 0xFF);
            stripe[i + 2] = ScreenPixelType::from_grey8(x >> 16 & 0xFF);
            stripe[i + 3] = ScreenPixelType::from_grey8(x >> 23 & 0xFF);
        }
        last_SPI_trans = the_display->send_stripe(y, STRIPE_HEIGHT, stripe);
    }

#endif

void send_image_stripe(size_t y)
{
    ScreenPixelType (*frame)[240] = image_buffers[current_image_buffer];
    ScreenPixelType *stripe = frame[y];
    last_SPI_trans = the_display->send_stripe(y, STRIPE_HEIGHT, stripe);
}

void update_SPI_display()
{

    the_display->begin_frame_centered(240, 240);

    for (size_t y = 0; y < 240; y += STRIPE_HEIGHT) {
#ifdef STATIC_FEATURE
        if (update_static()) {
            send_static_stripe(y);
        } else {
            send_image_stripe(y);
        }
#else
        send_image_stripe(y);
#endif
        // uint16_t *stripe = frame[y];
        // last_SPI_trans = the_display->send_stripe(y, STRIPE_HEIGHT, stripe);
    }

    the_display->end_frame();
}

// extern "C" void the_once_and_future_app_main()
extern "C" void app_main()
{
    printf("app_main\n");
    identify_board();

    init_prng();
    init_flash_images();
    init_SPI_display();
    init_backlight();
    init_main_loop_timer();

    // Update:
    // change frames every 7 ticks
    // update screen every tick
    // update backlight every tick
    // change animation:
    //     from intro after it's done
    //     between him and her when the backlight cycles (randomly)

    while (1) {
        wait_for_tick();
        update_backlight();
        update_SPI_display();
        update_animation();
    }
}

#define STRIPE_HEIGHT 8
ScreenPixelType blackk_stripe[STRIPE_HEIGHT][240];
ScreenPixelType red_stripe[STRIPE_HEIGHT][240];
ScreenPixelType blue_stripe[STRIPE_HEIGHT][240];
ScreenPixelType colors[3] = {
    ScreenPixelType(0xFF, 0, 0),
    ScreenPixelType(0, 0xFF, 0),
    ScreenPixelType(0, 0, 0xFF),
};

// extern "C" void app_main()
extern "C" void the_function_formerly_known_as_app_main()
{
    printf("app_main\n");
#ifdef WAVESHARE_ESP32_S3_LCD_TOUCH_1_69
    printf("This is the Waveshare 1.69 board!\n");
#elif defined(WAVESHARE_ESP32_S3_LCD_1_28)
    printf("This is the Waveshare 1.28 board!\n");
#else
    printf("Is this an unknown board?");
#endif
    printf("board = \"%s\"\n", BOARD);
    printf("\n");

    // Backlight the_backlight;
    // the_backlight.set_brightness(255);
    const gpio_num_t BACKLIGHT_GPIO = GPIO_NUM_15;
    gpio_reset_pin(BACKLIGHT_GPIO);
    gpio_set_direction(BACKLIGHT_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(BACKLIGHT_GPIO, 1);

    {
        SPIDisplay the_display(0);
        TransactionID id;

        for (size_t y = 0; y < STRIPE_HEIGHT; y++) {
            for (size_t x = 0; x < 240; x++) {
                red_stripe[y][x] = colors[0];
                blue_stripe[y][x] = colors[2];
            }
        }

        // fill with black
        size_t w = the_display.width();
        size_t h = the_display.height();
        the_display.begin_frame(w, h, 0, 0);
        for (int y = 0; y < 280; y += STRIPE_HEIGHT) {
            id = the_display.send_stripe(y, STRIPE_HEIGHT, *blackk_stripe);
        }
        the_display.end_frame();

        int64_t next = esp_timer_get_time() + 16666;
        for (int frame = 0; 1; frame++) {
            ScreenPixelType *stripe = (frame & 1) ? *blackk_stripe : *red_stripe;

            the_display.begin_frame_centered(240, 240);
            for (size_t y = 0; y < 240; y += STRIPE_HEIGHT) {
                id = the_display.send_stripe(y, STRIPE_HEIGHT, stripe);
            }
            the_display.end_frame();
            while (esp_timer_get_time() < next)
                ;
            next += 100'000;
        }
        the_display.await_transaction(id); // only wait for the last one ever
        vTaskDelay(1000); // 10 seconds
    }

#ifdef PRINT_FLASH_IMAGES    
    printf("Flash images found:\n");
    for (size_t i = 0; i < FlashImage::MAX_IMAGES; i++) {
        if (FlashImage *image = FlashImage::get_by_index(i)) {
            printf("  \"%s\"%*s %zu bytes, %zu frames\n",
                image->label(),
                (int)(FlashImage::MAX_LABEL_SIZE - std::strlen(image->label())), "",
                image->size_bytes(),image->frame_count());
        }
    }
    printf("\n");
#endif

#ifdef BENCHMARK_DMA
    benchmark_SRAM_DMA();
    benchmark_FLASH_DMA((const uint8_t *)part_addrs[2], part_sizes[2] / BIG);
#endif
}

// Update:
// change frames every 16 ticks
// update screen every 2 ticks
// update backlight every 2 ticks
// change animation:
//     from intro after it's done
//     between him and her when the backlight cycles (randomly)
//
// block at:
//    waiting for stripes
//    waiting for next update cycle
//
//
// start with an every-tick 