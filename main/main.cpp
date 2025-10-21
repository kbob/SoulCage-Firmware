// C++ standard headers
#include <cmath>
#include <cstdio>
#include <cstring>
#include <numbers>

// ESP-IDF and FreeRTOS headers
#include "esp_partition.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"

// Project headers
#include "sdkconfig.h"

// Component headers
#include "benchmarks.h"
#include "driver_backlight.h"
#include "driver_buzzer.h"
#include "dsp_memcpy.h"
#include "flash_image.h"
#include "flicker_effect.h"
#include "memory_dma.h"
#include "random.h"
#include "spi_display.h"


// //  //   //    //     //      //       //      //     //    //   //  // //
// App Level Feature Flags

#define STATIC_FEATURE 1

#define TEST_LCD
// #define RUN_BENCHMARKS
#define PRINT_FLASH_IMAGES

// Disable to keep the backlight on.
static const bool ENABLE_FLICKER_EFFECT = true;

// The flicker effect loops for this many frames.
// We change the displayed soul randomly when it repeats,
// since we know the screen's longest dark period is then.
static const unsigned ANIM_FRAMES = 70 * 50; // 70 seconds at 50 FPS


// //  //   //    //     //      //       //      //     //    //   //  // //
// Globals

Buzzer the_buzzer;
Backlight the_backlight(ENABLE_FLICKER_EFFECT ? 0.0f : 1.0f);
SpookyFlickerEffect the_spooky_flicker_effect(
    the_backlight,
    ANIM_FRAMES,
    ENABLE_FLICKER_EFFECT);


// //  //   //    //     //      //       //      //     //    //   //  // //
// Reporting

static void identify_board()
{
// #ifdef CONFIG_BOARD_WAVESHARE_ESP32_S3_LCD_TOUCH_1_69
//     printf("This is the Waveshare 1.69 board!\n");
// #elif defined(CONFIG_BOARD_WAVESHARE_ESP32_S3_LCD_1_28)
//     printf("This is the Waveshare 1.28 board!\n");
// #else
//     printf("Is this an unknown board?");
// #error "unknown board"
// #endif
    printf("board = \"%s\"\n", board_name);
    printf("\n");
}


// //  //   //    //     //      //       //      //     //    //   //  // //
// 


static const uint32_t BIG_TICK_PERIOD_MSEC = 20;
static TimerHandle_t big_tick_timer;
static TaskHandle_t tick_handler_task;

static void big_tick_callback(TimerHandle_t)
{
    BaseType_t higher_priority_task_woken = pdFALSE;
    vTaskNotifyGiveIndexedFromISR(tick_handler_task, 1, &higher_priority_task_woken);
    // return higher_priority_task_woken == pdTRUE;
}

static void init_main_loop_timer()
{
    tick_handler_task = xTaskGetCurrentTaskHandle();
    const char *name = "big_tick";
    TickType_t period = pdMS_TO_TICKS(BIG_TICK_PERIOD_MSEC);
    BaseType_t auto_reload = pdTRUE;
    void *timer_ID = nullptr;
    big_tick_timer = xTimerCreate(name, period, auto_reload, timer_ID, big_tick_callback);
    xTimerStart(big_tick_timer, 2);
}

static void wait_for_tick()
{
    UBaseType_t index = 1;
    BaseType_t clear_count = pdFALSE;
    TickType_t timeout = portMAX_DELAY;
    (void)ulTaskNotifyTakeIndexed(index, clear_count, timeout);
}

// //  //   //    //     //      //       //      //     //    //   //  // //
// 


static bool in_intro = true;
static FlashImage *current_image;
static size_t image_frames;
static size_t current_frame;
static const size_t IMAGE_BUFFER_COUNT = 2;
static ScreenPixelType
    DMA_ATTR DSP_ALIGNED_ATTR
    image_buffers[IMAGE_BUFFER_COUNT]
                 [FlashImage::IMAGE_HEIGHT]
                 [FlashImage::IMAGE_WIDTH];
static size_t current_image_buffer;

static void init_flash_images()
{
    current_image = FlashImage::get_by_label("Intro");
    assert(current_image != nullptr);
    image_frames = current_image->frame_count();

    auto *dst = *image_buffers[current_image_buffer];
    const auto *src = current_image->frame_addr(current_frame);
    assert(dst);
    assert(src);
    size_t pixel_count = FlashImage::FRAME_PIXEL_COUNT;
    size_t size = pixel_count * sizeof (ScreenPixelType);

    std::memcpy(dst, src, size);

    current_frame = (current_frame + 1) % image_frames;
}

static const int PERCENT_ANIMATION_CHANGE = 70;

// This is sync'd with the backlight flicker because
// the backlight starts off dark for a few seconds.

static void maybe_change_animation()
{
    static int frame;
    frame = (frame + 1) % ANIM_FRAMES;
    if (frame != 0) {
        return;
    }

    if (Random::randint(0, 100) < PERCENT_ANIMATION_CHANGE) {
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
        current_frame = Random::randint(0, image_frames);
    }
}

static void update_animation()
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
    size_t size = pixel_count * sizeof (ScreenPixelType);

    std::memcpy(dst, src, size);

    current_image_buffer = next_i_buf;

    current_frame = (current_frame + 1) % image_frames;
    if (current_frame == 0 && in_intro) {
        // if we've finished the intro, start on one of the souls..
        in_intro = false;
        const char *next_image = Random::rand() % 2 ? "soul_f" : "soul_m";
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
    static bool update_static()
    {
        // "static" -- get it?
        static int static_active = 0;
        static int static_start = -1;
        if (static_start == -1) {
            // first time
            static_start = Random::randint(MIN_NO_STATIC, MAX_NO_STATIC);
        } else if (!static_active && --static_start == 0) {
            static_active = Random::randint(MIN_STATIC, MAX_STATIC);
        }
        if (static_active) {
            if (--static_active != 0) {
                return true;
            }
            static_start = Random::randint(MIN_NO_STATIC, MAX_NO_STATIC);
        }
        return false;
    }

#endif

static SPIDisplay *the_display;
static TransactionID last_SPI_trans;
static ScreenPixelType DMA_ATTR black_stripe[FlashImage::IMAGE_WIDTH];
static const size_t STRIPE_HEIGHT = 8;

static void init_SPI_display()
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

    static const size_t STATIC_STRIPE_COUNT = 6;
    static ScreenPixelType DMA_ATTR static_stripes[STATIC_STRIPE_COUNT][STRIPE_HEIGHT][240];
    static size_t static_rotor;

    static void send_static_stripe(size_t y) {
        // Random::srand(1);
        ScreenPixelType *stripe = *static_stripes[static_rotor];
        static_rotor = (static_rotor + 1) % STATIC_STRIPE_COUNT;
        for (size_t i = 0; i < STRIPE_HEIGHT * 240; i += 4) {
            int x = Random::rand();
            stripe[i + 0] = ScreenPixelType::from_grey8(x >> 0 & 0xFF);
            stripe[i + 1] = ScreenPixelType::from_grey8(x >> 8 & 0xFF);
            stripe[i + 2] = ScreenPixelType::from_grey8(x >> 16 & 0xFF);
            stripe[i + 3] = ScreenPixelType::from_grey8(x >> 23 & 0xFF);
        }
        last_SPI_trans = the_display->send_stripe(y, STRIPE_HEIGHT, stripe);
    }

#endif

static void send_image_stripe(size_t y)
{
    ScreenPixelType (*frame)[240] = image_buffers[current_image_buffer];
    ScreenPixelType *stripe = frame[y];
    last_SPI_trans = the_display->send_stripe(y, STRIPE_HEIGHT, stripe);
}

static void update_SPI_display()
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

    // init_prng();
    init_flash_images();
    init_SPI_display();
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
        the_spooky_flicker_effect.update();
        update_SPI_display();
        update_animation();
    }
}

#define STRIPE_HEIGHT 8
static ScreenPixelType blackk_stripe[STRIPE_HEIGHT][240];
static ScreenPixelType red_stripe[STRIPE_HEIGHT][240];
static ScreenPixelType blue_stripe[STRIPE_HEIGHT][240];
static ScreenPixelType colors[3] = {
    ScreenPixelType(0xFF, 0, 0),
    ScreenPixelType(0, 0xFF, 0),
    ScreenPixelType(0, 0, 0xFF),
};

// extern "C" void app_main()
extern "C" void the_function_formerly_known_as_app_main()
{
    printf("app_main\n");
#ifdef CONFIG_BOARD_WAVESHARE_ESP32_S3_LCD_TOUCH_1_69
    printf("This is the Waveshare 1.69 board!\n");
#elif defined(CONFIG_BOARD_WAVESHARE_ESP32_S3_LCD_1_28)
    printf("This is the Waveshare 1.28 board!\n");
#else
    #error "no board?"
    printf("Is this an unknown board?");
#endif
    printf("board = \"%s\"\n", board_name);
    printf("\n");

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

#ifdef RUN_BENCHMARKS
    run_memcpy_benchmarks();
#endif

#ifdef TEST_LCD
    the_spooky_flicker_effect.set_enabled(false);
    the_backlight.set_brightness(1.0f);
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
        for (int frame = 0; frame < 100; frame++) {
            the_spooky_flicker_effect.update();
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
#endif
}
