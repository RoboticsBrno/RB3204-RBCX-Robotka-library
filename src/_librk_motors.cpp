#include "_librk_motors.h"
#include "RBCX.h"

namespace rk {

Motors::Motors()
    : m_id_left(rb::MotorId::M1)
    , m_id_right(rb::MotorId::M2) {
}

Motors::~Motors() {
}

void Motors::init(const rkConfig& cfg) {
    m_id_left = (rb::MotorId)(cfg.motor_id_left - 1);
    m_id_right = (rb::MotorId)(cfg.motor_id_right - 1);
    m_polarity_switch_left = cfg.motor_polarity_switch_left;
    m_polarity_switch_right = cfg.motor_polarity_switch_right;

    m_wheel_circumference = M_PI * cfg.motor_wheel_diameter;
    m_max_speed = cfg.motor_max_ticks_per_second;

    auto& man
        = rb::Manager::get();

    // Set motor power limits
    man
        .setMotors()
        .pwmMaxPercent(m_id_left, cfg.motor_max_power_pct)
        .pwmMaxPercent(m_id_right, cfg.motor_max_power_pct)
        .set();

    if (rb::Manager::get().coprocFwVersion().number >= 0x010100) {
        const MotorConfig motorConf = {
            .velEpsilon = 3,
            .posEpsilon = 8,
            .maxAccel = cfg.motor_max_acceleration,
        };

        man.motor(m_id_left).setConfig(motorConf);
        man.motor(m_id_right).setConfig(motorConf);
    }
}

void Motors::setPower(int8_t left, int8_t right) {
    if (m_polarity_switch_left)
        left = -left;
    if (m_polarity_switch_right)
        right = -right;

    rb::Manager::get()
        .setMotors()
        .power(m_id_left, pctToPower(left))
        .power(m_id_right, pctToPower(right))
        .set();
}

void Motors::setPower(int8_t left, int8_t right, uint8_t pwm_pct_left, uint8_t pwm_pct_right) {
    if (m_polarity_switch_left)
        left = -left;
    if (m_polarity_switch_right)
        right = -right;

    rb::Manager::get()
        .setMotors()
        .pwmMaxPercent(m_id_left, pwm_pct_left)
        .pwmMaxPercent(m_id_right, pwm_pct_right)
        .power(m_id_left, pctToPower(left))
        .power(m_id_right, pctToPower(right))
        .set();
}

void Motors::setPowerById(rb::MotorId id, int8_t power) {
    if ((m_polarity_switch_left && id == m_id_left) || (m_polarity_switch_right && id == m_id_right))
        power = -power;

    rb::Manager::get()
        .setMotors()
        .power(id, pctToPower(power))
        .set();
}

void Motors::setSpeed(int8_t left, int8_t right) {
    if (m_polarity_switch_left)
        left = -left;
    if (m_polarity_switch_right)
        right = -right;

    rb::Manager::get()
        .setMotors()
        .speed(m_id_left, pctToSpeed(left))
        .speed(m_id_right, pctToSpeed(right))
        .set();
}

void Motors::setSpeed(int8_t left, int8_t right, uint8_t pwm_pct_left, uint8_t pwm_pct_right) {
    if (m_polarity_switch_left)
        left = -left;
    if (m_polarity_switch_right)
        right = -right;

    rb::Manager::get()
        .setMotors()
        .pwmMaxPercent(m_id_left, pwm_pct_left)
        .pwmMaxPercent(m_id_right, pwm_pct_right)
        .speed(m_id_left, pctToSpeed(left))
        .speed(m_id_right, pctToSpeed(right))
        .set();
}

void Motors::setSpeedById(rb::MotorId id, int8_t power) {
    if ((m_polarity_switch_left && id == m_id_left) || (m_polarity_switch_right && id == m_id_right))
        power = -power;

    rb::Manager::get()
        .setMotors()
        .speed(id, pctToSpeed(power))
        .set();
}

void Motors::drive(float left, float right, uint8_t speed, dual_callback_t callback) {
    if (m_polarity_switch_left)
        left = -left;
    if (m_polarity_switch_right)
        right = -right;

    rb::Motor::callback_t cb;
    if (callback) {
        std::lock_guard<std::mutex> l(m_dual_callbacks_mu);
        auto itr = m_dual_callbacks.emplace(m_dual_callbacks.end(), DualCb(std::move(callback)));

        cb = [this, itr](rb::Motor& m) {
            std::unique_lock<std::mutex> lock(m_dual_callbacks_mu);
            if (++itr->count == 2) {
                dual_callback_t cb;
                cb.swap(itr->final_cb);
                this->m_dual_callbacks.erase(itr);
                lock.unlock();
                cb();
            }
        };
    }

    auto& ml = rb::Manager::get().motor(m_id_left);
    auto& mr = rb::Manager::get().motor(m_id_right);

    ml.requestInfo(nullptr);
    mr.requestInfo([this, left, right, speed, cb](rb::Motor& mr) {
        auto& ml = rb::Manager::get().motor(m_id_left);
        rb::Manager::get()
            .setMotors()
            .driveToValue(m_id_left, ml.position() + mmToTicks(left), pctToSpeed(speed), cb)
            .driveToValue(m_id_right, mr.position() + mmToTicks(right), pctToSpeed(speed), cb)
            .set();
    });
}

void Motors::driveById(rb::MotorId id, float mm, uint8_t speed, std::function<void()> callback) {
    if ((m_polarity_switch_left && id == m_id_left) || (m_polarity_switch_right && id == m_id_right))
        mm = -mm;

    auto& m = rb::Manager::get().motor(id);
    m.requestInfo([this, mm, speed, id, callback](rb::Motor& m) {
        rb::Manager::get()
            .setMotors()
            .driveToValue(id, m.position() + mmToTicks(mm), pctToSpeed(speed), [this, callback, id](rb::Motor& m) {
                callback();
            })
            .set();
    });
}

float Motors::position(rb::MotorId id) {
    auto res = ticksToMm(rb::Manager::get().motor(id).position());
    if ((m_polarity_switch_left && id == m_id_left) || (m_polarity_switch_right && id == m_id_right))
        res = -res;
    return res;
}

void Motors::setPosition(rb::MotorId id, float positionMm) {
    auto ticks = mmToTicks(positionMm);
    if ((m_polarity_switch_left && id == m_id_left) || (m_polarity_switch_right && id == m_id_right))
        ticks = -ticks;
    rb::Manager::get().motor(id).setCurrentPosition(ticks);
}

void Motors::joystick(int32_t x, int32_t y) {
    x = scale(x);
    y = scale(y);

    int r = ((y - (x / 1.5f)));
    int l = ((y + (x / 1.5f)));

    r = rb::clamp(r, -100, 100);
    l = rb::clamp(l, -100, 100);

    if (r < 0 && l < 0) {
        std::swap(r, l);
    }
    setSpeed(l, r);
}

int32_t Motors::scale(int32_t val) {
    return val * 100 / RBPROTOCOL_AXIS_MAX;
}

int16_t Motors::pctToPower(int8_t pct) {
    return rb::clamp(pct * INT16_MAX / 100, -INT16_MAX, INT16_MAX);
}

int16_t Motors::pctToSpeed(int8_t pct) const {
    return rb::clamp(pct * m_max_speed / 100, -INT16_MAX, INT16_MAX);
}

int32_t Motors::mmToTicks(float mm) const {
    return (mm / m_wheel_circumference) * 12.f * 48.f;
}

float Motors::ticksToMm(int32_t ticks) const {
    return float(ticks) / 12.f / 48.f * m_wheel_circumference;
}

}; // namespacer rk
