# Soul Cage - Alternate Firmware

This is an alternate firmware for [SoulCage -- the Digital Trapped Soul Pendant](https://github.com/vishalsoniindia/SoulCage---The-Digital-Trapped-Soul-Pendant).

I created it because the video inside is an 8 second loop.  It gets repetitive.  I do some things to interrupt the repetition.

* The backlight flickers erratically, giving brief glimpses of the souls inside.

* There are short bursts of white noise static to interrupt the video and obscure it even more.

Other features

 - faster frame rate - I keep the video at ~7 FPS, but the screen refreshes at 50 FPS so the static bursts can be fast.

 - It randomly changes between the male and female souls during the longer dark periods.  There's no more nvs/power cycling trick -- you always get a random soul, and it will switch eventually.

# Install from Binaries

There are prebuild binaries in the prebuilt directory.  To install them, you need the `esptool` utility.  `esptool` is included in Arduino, ESP-IDF, PlatformIO, and ESPhome, among others, so you may already have it.  It may be called `esptool.py` or `esptool.exe` on Windows.  If so, substitute the tool name you have.

Attach the SoulCage with a USB cable and find its serial port.

Open a terminal (use WSL if you're on Windows), navigate into the `prebuilt` directory, and type this command.  Substitute the actual name or path of your `esptool` and serial port.

    [YOUR_ESPTOOL_PATH] --chip esp32s3 \
                        -p [YOUR_SERIAL_PORT] \
                        -b 460800 \
                        --before=default_reset \
                        --after=hard_reset \
                          write_flash \
                        --flash_mode dio \
                        --flash_freq 80m \
                        --flash_size 16MB \
                          0x0 bootloader.bin \
                          0x10000 SoulCage.bin \
                          0x8000 partition-table.bin \
                          0x100000 Intro.bin \
                          0x36b000 soul_f.bin \
                          0x906000 soul_m.bin





# More Needed

I want to make this usable for Halloween 2025.  So I'll get 
some prebuilt binaries and instructions for flashing them.

Real cleanup will take a while.  As the last commit message says,
this repo is a real mess but it's (barely) functional.