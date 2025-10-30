// C++ standard headers
#include <cmath>
#include <cstdio>
#include <cstring>
#include <numbers>

// Component headers
#include "animation.h"
#include "battery_monitor.h"
#include "board_defs.h"
#include "refresh_clock.h"
#include "driver_backlight.h"
#include "driver_buzzer.h"
#include "flicker_effect.h"
#include "spi_display.h"
#include "video_streamer.h"


// //  //   //    //     //      //       //      //     //    //   //  // //
// App Level Settings

// Enable to make the backlight slowly flicker
static const bool ENABLE_FLICKER_EFFECT = true;

// Enable to inject static bursts into the video
static const bool ENABLE_STATIC_EFFECT = true;

// Screen refresh rate
static constexpr float SCREEN_REFRESH_HZ = 60.0f;

// How often to log battery voltage to serial port
static const int BATTERY_LOG_PERIOD_SEC = 10;

// The flicker effect loops for this many frames.
// We change the displayed soul randomly when it repeats,
// since we know the screen's longest dark period is then.
static const unsigned ANIM_FRAMES = 70 * SCREEN_REFRESH_HZ; // 70 seconds
// static const unsigned ANIM_FRAMES = SCREEN_REFRESH_HZ;

// At the end of every ANIM_FRAMES loop, the animator might
// choose to change the displayed soul.
static constexpr float SOUL_CHANGE_PROBABILITY = 0.7;


// //  //   //    //     //      //       //      //     //    //   //  // //
// App Main Task

extern "C" void app_main()
{
    printf("app_main\n");
    printf("board = \"%s\"\n", board_name);

    // Create the world.
    // create and blank the screen before turning on the backlight.
    Buzzer the_buzzer;
    BatteryMonitor the_battery(BATTERY_LOG_PERIOD_SEC);

    Animation the_animation(ANIM_FRAMES, SOUL_CHANGE_PROBABILITY);

    SPIDisplay the_display;

    VideoStreamer the_streamer(
        the_animation,
        the_display,
        ENABLE_STATIC_EFFECT);

    Backlight the_backlight(ENABLE_FLICKER_EFFECT ? 0.0f : 1.0f);

    SpookyFlickerEffect the_spooky_flicker_effect(
        the_backlight,
        ANIM_FRAMES,
        ENABLE_FLICKER_EFFECT);

    RefreshClock the_refresh_clock(SCREEN_REFRESH_HZ);

    while (1) {
        the_refresh_clock.wait();
        the_spooky_flicker_effect.update();
        the_streamer.update();
        the_battery.update();
        the_animation.update(); // Do this last, every 8th frame is slow
    }
}
