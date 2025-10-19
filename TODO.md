# To Do

<!--
Green check: &#x2705;
Red X:       &#x274c;
-->

## Bugs

&#x2705; Fix the quick hacks in spi_display.cpp that change PASET
and CASET commands

&#x2705; Fix pixel format on 1.28 board (GC9A01)

&#x2705; Either fix async memcpy or create a low-priority task to copy
frames.  (How fast can it run without interfering with SPI?)

 - Hmmm.  It's currently taking 17 msec to copy an image.  That's
   a little longer than the 60 Hz frame rate I'd like to run at.
   So if I dedicated a whole core...  or else I get the vector unit
   vectoring...

 - &#x2705; first I'll try writing the swizzling code directly.  It might
   just be that the compiler can't optimize the nonsense I wrote.
   I can specialize on the exact source and destination formats needed.

 - &#x2705; better is to parameterize the build so the images on flash have
   the order the screen needs.

 - &#x2705; I benchmarked various ways of copying from flash.  memcpy
   is faster than DMA or copying using the DSP instructions.  It's also
   faster than the LCD refresh rate.  So I'll just leave it.

## Cleanup

&#x2705; In `main/CMakeLists.txt`, change driver to list of drivers actually used.

Remove hardcoded constants everywhere.  Have one set of constants
for screen resolution and for image size.

Break stuff into separate modules.

  * buzzer
  * backlight
  * packed color  (This needs to be templated on color order)
  * PRNG
  * image update
  * break spi_display into 3 or 4 modules

&#x2705; Find out why PSRAM isn't working on the 1.28 board

And so much more...


## Features

Change tick counter to 60 Hz

Interpolate video frames.  Blend video with static.
