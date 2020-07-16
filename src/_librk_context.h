#pragma once

#include <atomic>

#include "mcp3008_linesensor.h"

#include "_librk_motors.h"
#include "_librk_smartleds.h"
#include "_librk_wifi.h"

namespace rk {

class Context {
public:
    Context();
    ~Context();

    void setup(const rkConfig& cfg);

    rb::Protocol* prot() const { return m_prot; }
    Motors& motors() { return m_motors; }
    mcp3008::LineSensor& line();
    rk::SmartLeds& smartLed() { return m_smartLeds; }

    void saveLineCalibration();

    adc1_channel_t irChanLeft() const { return m_ir_left; }
    adc1_channel_t irChanRight() const { return m_ir_right; }

    uint16_t irRead(adc1_channel_t chan, uint16_t samples = 32);

    void stupidServoSet(uint8_t id, float positionDegrees);
    float stupidServoGet(uint8_t id);

private:
    void handleRbcontrollerMessage(const std::string& cmd, rbjson::Object* pkt);
    bool loadLineCalibration(mcp3008::LineSensor::CalibrationData& data);

    void initIrSensors();

    Motors m_motors;
    WiFi m_wifi;
    rb::Protocol* m_prot;

    std::atomic<bool> m_initialized;

    std::atomic<bool> m_line_installed;
    std::atomic<bool> m_ir_installed;

    mcp3008::Driver::Config m_line_cfg;
    mcp3008::LineSensor m_line;
    SmartLeds m_smartLeds;

    adc1_channel_t m_ir_left;
    adc1_channel_t m_ir_right;

    float m_stupid_servo_min;
    float m_stupid_servo_max;
};

extern Context gCtx;

};
