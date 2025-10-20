#pragma once

// N.B., the backlight is turned off by default.
// Pass a nonzero brightness to the contructor or call `set_brightness()`.

class Backlight {

public:
    Backlight(float brightness = 0.0f);
    ~Backlight() {}

    void set_gamma(float);
    void set_brightness(float);

private:
    Backlight(const Backlight&) = delete;
    void operator = (const Backlight&) = delete;

    struct Backlight_private *m_private;

};
