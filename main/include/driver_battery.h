#pragma once

class BatteryDriver {

public:
    BatteryDriver();
    ~BatteryDriver();

    float voltage() const;

private:
    BatteryDriver(const BatteryDriver&) = delete;
    void operator = (const BatteryDriver&) = delete;

    struct BatteryDriver_private *m_private;
};
