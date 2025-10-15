# To Do

## Bugs

&#x2705; Fix the quick hacks in spi_display.cpp that change PASET
and CASET commands

&#x2705; Fix pixel format on 1.28 board (GC9A01)

Either fix async memcpy or create a low-priority task to copy
frames.  (How fast can it run without interfering with SPI?)

## Cleanup

Remove hardcoded constants everywhere.  Have one set of constants
for screen resolution and for image size.

Break stuff into separate modules.

  * buzzer
  * backlight
  * packed color  (This needs to be templated on color order)
  * PRNG
  * image update
  * break spi_display into 3 or 4 modules

Find out why PSRAM isn't working on the 1.28 board

And so much more...

## Features

Change tick counter to 60 Hz

Interpolate video frames.  Blend video with static.
