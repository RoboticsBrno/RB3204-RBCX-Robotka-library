#include <stdarg.h>
#include <stdio.h>

#include "_librk_context.h"
#include "robotka.h"

#include "RBCX.h"

#define TAG "robotka"

using namespace rb;
using namespace rk;
using namespace mcp3008;

// Empty loop in case the user won't supply one
void __attribute__((weak)) loop() {
}

void rkSetup(const rkConfig& cfg) {
    gCtx.setup(cfg);
}

void rkControllerSendLog(const char* format, ...) {
    if (gCtx.prot() == nullptr) {
        ESP_LOGE(TAG, "%s: protocol not initialized!", __func__);
        return;
    }
    va_list args;
    va_start(args, format);
    gCtx.prot()->send_log(format, args);
    va_end(args);
}

void rkControllerSendLog(const std::string& text) {
    if (gCtx.prot() == nullptr) {
        ESP_LOGE(TAG, "%s: protocol not initialized!", __func__);
        return;
    }

    gCtx.prot()->send_log(text);
}

void rkControllerSend(const char* cmd, rbjson::Object* data) {
    if (gCtx.prot() == nullptr) {
        ESP_LOGE(TAG, "%s: protocol not initialized!", __func__);
        return;
    }
    gCtx.prot()->send(cmd, data);
}

void rkControllerSendMustArrive(const char* cmd, rbjson::Object* data) {
    if (gCtx.prot() == nullptr) {
        ESP_LOGE(TAG, "%s: protocol not initialized!", __func__);
        return;
    }
    gCtx.prot()->send_mustarrive(cmd, data);
}

uint32_t rkBatteryPercent() {
    return Manager::get().battery().pct();
}

uint32_t rkBatteryVoltageMv() {
    return Manager::get().battery().voltageMv();
}

void rkMotorsSetPower(int8_t left, int8_t right) {
    gCtx.motors().setPower(left, right);
}

void rkMotorsSetPowerLeft(int8_t power) {
    gCtx.motors().setPowerById(gCtx.motors().idLeft(), power);
}

void rkMotorsSetPowerRight(int8_t power) {
    gCtx.motors().setPowerById(gCtx.motors().idRight(), power);
}

void rkMotorsSetPowerById(int id, int8_t power) {
    id -= 1;
    if (id < 0 || id >= (int)rb::MotorId::MAX) {
        ESP_LOGE(TAG, "%s: invalid motor id, %d is out of range <1;4>.", __func__, id + 1);
        return;
    }
    gCtx.motors().setPowerById(rb::MotorId(id), power);
}

void rkMotorsSetSpeed(int8_t left, int8_t right) {
    gCtx.motors().setSpeed(left, right);
}

void rkMotorsSetSpeedLeft(int8_t speed) {
    gCtx.motors().setSpeedById(gCtx.motors().idLeft(), speed);
}

void rkMotorsSetSpeedRight(int8_t speed) {
    gCtx.motors().setSpeedById(gCtx.motors().idRight(), speed);
}

void rkMotorsSetSpeedById(int id, int8_t speed) {
    id -= 1;
    if (id < 0 || id >= (int)rb::MotorId::MAX) {
        ESP_LOGE(TAG, "%s: invalid motor id, %d is out of range <1;4>.", __func__, id + 1);
        return;
    }
    gCtx.motors().setSpeedById(rb::MotorId(id), speed);
}

void rkMotorsJoystick(int32_t x, int32_t y) {
    gCtx.motors().joystick(x, y);
}

void rkLedRed(bool on) {
    Manager::get().leds().red(on);
}

void rkLedYellow(bool on) {
    Manager::get().leds().yellow(on);
}

void rkLedGreen(bool on) {
    Manager::get().leds().green(on);
}

void rkLedBlue(bool on) {
    Manager::get().leds().blue(on);
}

void rkLedAll(bool on) {
    auto& l = Manager::get().leds();
    l.red(on);
    l.yellow(on);
    l.green(on);
    l.blue(on);
}

void rkLedById(uint8_t id, bool on) {
    if (id == 0) {
        ESP_LOGE(TAG, "%s: invalid id %d, LEDs are indexed from 1, just like on the board (LED1, LED2...)!", __func__, id);
        return;
    } else if (id > 4) {
        ESP_LOGE(TAG, "%s: maximum LED id is 4, you are using %d!", __func__, id);
        return;
    }

    auto& l = Manager::get().leds();
    l.byId(LedId((1 << (id - 1))));
}

bool rkButtonIsPressed(rkButtonId id, bool waitForRelease) {
    auto& b = Manager::get().buttons();
    for (int i = 0; i < 3; ++i) {
        const bool pressed = b.byId(ButtonId(id));
        if (!pressed)
            return false;
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (waitForRelease) {
        rkButtonWaitForRelease(id);
    }
    return true;
}

void rkButtonOnChange(std::function<bool(rkButtonId, bool)> callback) {
    Manager::get().buttons().onChange([callback](rb::ButtonId id, bool pressed) -> bool {
        return callback(rkButtonId(id), pressed);
    });
}

void rkButtonWaitForRelease(rkButtonId id) {
    int counter = 0;
    auto& b = Manager::get().buttons();
    while (true) {
        const bool pressed = b.byId(ButtonId(id));
        if (!pressed) {
            if (++counter > 3)
                return;
        } else {
            counter = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void rkLineCalibrate(float motor_time_coef) {
    auto& man = Manager::get();

    const auto l = gCtx.motors().idLeft();
    const auto r = gCtx.motors().idRight();

    const auto maxleft = man.motor(l).pwmMaxPercent();
    const auto maxright = man.motor(r).pwmMaxPercent();

    constexpr int8_t pwr = 40;

    auto cal = gCtx.line().startCalibration();

    gCtx.motors().setPower(-pwr, pwr, 100, 100);

    for (int i = 0; i < 30 * motor_time_coef; ++i) {
        cal.record();
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    gCtx.motors().setPower(pwr, -pwr);
    for (int i = 0; i < 50 * motor_time_coef; ++i) {
        cal.record();
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    gCtx.motors().setPower(-pwr, pwr);
    for (int i = 0; i < 30 * motor_time_coef; ++i) {
        cal.record();
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    gCtx.motors().setPower(0, 0, maxleft, maxright);

    cal.save();
    gCtx.saveLineCalibration();
}

void rkLineClearCalibration() {
    LineSensor::CalibrationData cal;
    for (int i = 0; i < Driver::CHANNELS; ++i) {
        cal.min[i] = 0;
        cal.range[i] = Driver::MAX_VAL;
    }
    gCtx.line().setCalibration(cal);
    gCtx.saveLineCalibration();
}

uint16_t rkLineGetSensor(uint8_t sensorId) {
    return gCtx.line().calibratedReadChannel(sensorId);
}

float rkLineGetPosition(bool white_line, uint8_t line_threshold_pct) {
    return gCtx.line().readLine(white_line, float(line_threshold_pct) / 100.f);
}

uint32_t rkUltraMeasure(uint8_t id) {
    if (id == 0) {
        ESP_LOGE(TAG, "%s: invalid id %d, Ultrasounds are indexed from 1, just like on the board (U1, U2...)!", __func__, id);
        return 0;
    } else if (id > 4) {
        ESP_LOGE(TAG, "%s: maximum Ultrasound id is 4, you are using %d!", __func__, id);
        return 0;
    }

    return Manager::get().ultrasound(id - 1).measure();
}

void rkUltraMeasureAsync(uint8_t id, std::function<void(uint32_t)> callback) {
    if (id == 0) {
        ESP_LOGE(TAG, "%s: invalid id %d, Ultrasounds are indexed from 1, just like on the board (U1, U2...)!", __func__, id);
        callback(0);
        return;
    } else if (id > 4) {
        ESP_LOGE(TAG, "%s: maximum Ultrasound id is 4, you are using %d!", __func__, id);
        callback(0);
        return;
    }

    return Manager::get().ultrasound(id - 1).measureAsync(callback);
}

void rkBuzzerSet(bool on) {
    Manager::get().piezo().setState(on);
}

void rkSmartLedsRGB(uint8_t idx, uint8_t r, uint8_t g, uint8_t b) {
    auto& l = gCtx.smartLed();
    if (idx >= l.count()) {
        ESP_LOGE(TAG, "%s: invalid LED idx %d, must be <= %d!", __func__, idx, l.count());
        return;
    }
    l.setRGB(idx, r, g, b);
}

void rkSmartLedsHSV(uint8_t idx, uint8_t h, uint8_t s, uint8_t v) {
    auto& l = gCtx.smartLed();
    if (idx >= l.count()) {
        ESP_LOGE(TAG, "%s: invalid LED idx %d, must be <= %d!", __func__, idx, l.count());
        return;
    }
    l.setHSV(idx, h, s, v);
}

SmartLed& rkSmartLedsGetController() {
    return gCtx.smartLed().controller();
}
