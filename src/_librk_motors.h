#pragma once

#include <stdint.h>

#include "RBCXPinout.h"
#include "robotka.h"

namespace rk {

class Motors {
public:
    Motors();
    ~Motors();

    void init(const rkConfig& cfg);

    void setPower(int8_t left, int8_t right);
    void setPower(int8_t left, int8_t right, uint8_t pwm_pct_left, uint8_t pwm_pct_right);
    void setPowerById(rb::MotorId id, int8_t power);

    void setSpeed(int8_t left, int8_t right);
    void setSpeed(int8_t left, int8_t right, uint8_t pwm_pct_left, uint8_t pwm_pct_right);
    void setSpeedById(rb::MotorId id, int8_t speed);

    void joystick(int32_t x, int32_t y);

    rb::MotorId idLeft() const { return m_id_left; }
    rb::MotorId idRight() const { return m_id_right; }

private:
    Motors(const Motors&) = delete;

    static int32_t scale(int32_t val);
    static int16_t pctToPower(int8_t pct);
    static int16_t pctToSpeed(int8_t pct);

    rb::MotorId m_id_left;
    rb::MotorId m_id_right;
    bool m_polarity_switch_left;
    bool m_polarity_switch_right;
};

}; // namespace rk
