// C++ standard headers
#include <cmath>
#include <cstdio>
#include <cstring>
#include <numbers>

// ESP-IDF and FreeRTOS headers
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"

// Project headers
#include "sdkconfig.h"

// Component headers
#include "animation.h"
#include "driver_backlight.h"
#include "driver_buzzer.h"
#include "flicker_effect.h"
#include "random.h"
#include "spi_display.h"


// //  //   //    //     //      //       //      //     //    //   //  // //
// App Level Settings

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

// At the end of every ANIM_FRAMES loop, the animator might
// choose to change the displayed soul.
static constexpr float SOUL_CHANGE_PROBABILITY = 0.7;


// //  //   //    //     //      //       //      //     //    //   //  // //
// Globals

Buzzer the_buzzer;
Backlight the_backlight(ENABLE_FLICKER_EFFECT ? 0.0f : 1.0f);
SpookyFlickerEffect the_spooky_flicker_effect(
    the_backlight,
    ANIM_FRAMES,
    ENABLE_FLICKER_EFFECT);
Animation the_animation(ANIM_FRAMES, SOUL_CHANGE_PROBABILITY);


// //  //   //    //     //      //       //      //     //    //   //  // //
// Reporting

static void identify_board()
{
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
static pixel_type DMA_ATTR black_stripe[IMAGE_WIDTH];
static const size_t STRIPE_HEIGHT = SPIDisplay::STRIPE_HEIGHT;

static void init_SPI_display()
{
    the_display = new SPIDisplay();

    const size_t BLACK_STRIPE_HEIGHT = 1;

    // fill with black
    size_t w = the_display->width();
    size_t h = the_display->height();
    assert(w == 240);
    the_display->begin_frame(w, h, 0, 0);
    for (int y = 0; y < h; y += BLACK_STRIPE_HEIGHT) {
        last_SPI_trans =
            the_display->send_stripe(y, BLACK_STRIPE_HEIGHT, black_stripe);
    }
    the_display->end_frame();
}

#ifdef STATIC_FEATURE

    static const size_t STATIC_STRIPE_COUNT = 6;
    static pixel_type DMA_ATTR static_stripes[STATIC_STRIPE_COUNT][STRIPE_HEIGHT][240];
    static size_t static_rotor;

    static void send_static_stripe(size_t y) {
        // Random::srand(1);
        pixel_type *stripe = *static_stripes[static_rotor];
        static_rotor = (static_rotor + 1) % STATIC_STRIPE_COUNT;
        for (size_t i = 0; i < STRIPE_HEIGHT * 240; i += 4) {
            int x = Random::rand();
            stripe[i + 0] = pixel_type::from_grey8(x >> 0 & 0xFF);
            stripe[i + 1] = pixel_type::from_grey8(x >> 8 & 0xFF);
            stripe[i + 2] = pixel_type::from_grey8(x >> 16 & 0xFF);
            stripe[i + 3] = pixel_type::from_grey8(x >> 23 & 0xFF);
        }
        last_SPI_trans = the_display->send_stripe(y, STRIPE_HEIGHT, stripe);
    }

#endif

static void send_image_stripe(size_t y)
{
    const image_type *frame = the_animation.current_frame();
    const pixel_type *stripe = (*frame)[y];
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

    init_SPI_display();
    init_main_loop_timer();

    while (1) {
        wait_for_tick();
        the_spooky_flicker_effect.update();
        update_SPI_display();
        the_animation.update();
    }
}
