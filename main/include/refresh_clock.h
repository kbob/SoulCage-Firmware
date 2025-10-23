#pragma once

// RefreshClock - block until it's time to refresh the screen
// It unblocks at a steady 60 Hz (or whatever) so on-screen
// animations are smooth.
class RefreshClock {

public:
    RefreshClock(float freq_hz);
    ~RefreshClock();

    void wait();

private:
    // put the implementation into a private object to
    // keep the FreeRTOS dependency out of the header.
    struct RefreshClock_private *m_private;
};
