#pragma once

#include <functional>
#include <list>
#include <mutex>
#include <stdint.h>

#include "RBCXPinout.h"
#include "robotka.h"

namespace rk {

class Motors {
public:
    Motors();
    ~Motors();

    typedef std::function<void()> dual_callback_t;

    void init(const rkConfig& cfg);

    void setPower(int8_t left, int8_t right);
    void setPower(int8_t left, int8_t right, uint8_t pwm_pct_left, uint8_t pwm_pct_right);
    void setPowerById(rb::MotorId id, int8_t power);

    void setSpeed(int8_t left, int8_t right);
    void setSpeed(int8_t left, int8_t right, uint8_t pwm_pct_left, uint8_t pwm_pct_right);
    void setSpeedById(rb::MotorId id, int8_t speed);

    void drive(float left, float right, uint8_t speed, dual_callback_t callback = nullptr);
    void driveById(rb::MotorId id, float mm, uint8_t speed, std::function<void()> callback = nullptr);

    float position(rb::MotorId id);
    void setPosition(rb::MotorId id, float positionMm);

    void joystick(int32_t x, int32_t y);

    rb::MotorId idLeft() const { return m_id_left; }
    rb::MotorId idRight() const { return m_id_right; }

private:
    Motors(const Motors&) = delete;

    static int32_t scale(int32_t val);
    static int16_t pctToPower(int8_t pct);
    int16_t pctToSpeed(int8_t pct) const;
    int32_t mmToTicks(float mm) const;
    float ticksToMm(int32_t ticks) const;

    struct DualCb {
        DualCb(dual_callback_t&& cb)
            : final_cb(std::move(cb))
            , count(0) {}

        dual_callback_t final_cb;
        uint8_t count;
    };

    std::list<DualCb> m_dual_callbacks;
    std::mutex m_dual_callbacks_mu;

    rb::MotorId m_id_left;
    rb::MotorId m_id_right;
    float m_wheel_circumference;
    int32_t m_max_speed;
    bool m_polarity_switch_left;
    bool m_polarity_switch_right;
};

}; // namespace rk
