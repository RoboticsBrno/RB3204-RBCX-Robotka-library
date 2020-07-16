#include "nvs_flash.h"

#include "robotka.h"

#include "RBCX.h"

#include "rbprotocol.h"
#include "rbwebserver.h"
#include "rbwifi.h"
#include <stdio.h>

#include "_librk_context.h"

using namespace rb;
using namespace mcp3008;

#define TAG "robotka"

namespace rk {

Context gCtx;

Context::Context() {
    m_prot = nullptr;
}

Context::~Context() {
}

void Context::setup(const rkConfig& cfg) {
    bool expected = false;
    if (!m_initialized.compare_exchange_strong(expected, true)) {
        ESP_LOGE(TAG, "rkSetup was called more than once, this is WRONG!");
        return;
    }

    rb::Timers::deleteFreeRtOsTimerTask();

    // Initialize the robot manager
    auto& man = Manager::get();

    auto man_flags = MAN_NONE;
    if (!cfg.motor_enable_failsafe) {
        man_flags = ManagerInstallFlags(man_flags | MAN_DISABLE_MOTOR_FAILSAFE);
    }

    man.install(man_flags, 6 * 1024);

    m_line_cfg.pin_cs = (gpio_num_t)cfg.pins.line_cs;
    m_line_cfg.pin_mosi = (gpio_num_t)cfg.pins.line_mosi;
    m_line_cfg.pin_miso = (gpio_num_t)cfg.pins.line_miso;
    m_line_cfg.pin_sck = (gpio_num_t)cfg.pins.line_sck;

    m_ir_left = cfg.pins.ir_adc_chan_left;
    m_ir_right = cfg.pins.ir_adc_chan_right;

    m_stupid_servo_min = cfg.stupid_servo_min;
    m_stupid_servo_max = cfg.stupid_servo_max;

    m_motors.init(cfg);
    m_smartLeds.init(cfg);

    if (cfg.rbcontroller_app_enable) {
        m_wifi.init(cfg);

        // Start web server with control page (see data/index.html)
        rb_web_start(80);

        // Initialize the communication protocol
        using namespace std::placeholders;
        m_prot = new Protocol(cfg.owner, cfg.name, "Compiled at " __DATE__ " " __TIME__,
            std::bind(&Context::handleRbcontrollerMessage, this, _1, _2));
        m_prot->start();

        UI.begin(m_prot);
    }

    const auto& v = man.coprocFwVersion();
    printf("STM32 FW version: %06x %.8s%s\n", v.number, v.revision,
        v.dirty ? "-dirty" : "");
}

void Context::handleRbcontrollerMessage(const std::string& cmd, rbjson::Object* pkt) {
    m_wifi.disableBle();

    if (UI.handleRbPacket(cmd, pkt))
        return;
}

LineSensor& Context::line() {
    bool ex = false;
    if (!m_line_installed.compare_exchange_strong(ex, true))
        return m_line;

    auto res = m_line.install(m_line_cfg);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "failed to install linesensor: %d!", res);
        m_line_installed = false;
        return m_line;
    }

    LineSensor::CalibrationData data;
    if (loadLineCalibration(data)) {
        m_line.setCalibration(data);
    }

    return m_line;
}

bool Context::loadLineCalibration(LineSensor::CalibrationData& data) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ret = nvs_flash_erase();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "failed to nvs_flash_erase: %d!", ret);
            return false;
        }
        ret = nvs_flash_init();
    }

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed to nvs_flash_init: %d!", ret);
        return false;
    }

    nvs_handle nvs_ns;
    ret = nvs_open("robotka", NVS_READONLY, &nvs_ns);
    if (ret != ESP_OK) {
        if (ret != ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGE(TAG, "failed to nvs_open: %d", ret);
        }
        return false;
    }

    size_t size = sizeof(data);
    ret = nvs_get_blob(nvs_ns, "linecal", &data, &size);
    nvs_commit(nvs_ns);
    nvs_close(nvs_ns);

    if (ret == ESP_OK) {
        return true;
    } else if (ret != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "failed to nvs_get_blob: %d", ret);
    }
    return false;
}

void Context::saveLineCalibration() {
    const auto& data = m_line.getCalibration();

    nvs_handle nvs_ns;
    esp_err_t ret = nvs_open("robotka", NVS_READWRITE, &nvs_ns);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed to nvs_open: %d", ret);
        return;
    }

    ret = nvs_set_blob(nvs_ns, "linecal", &data, sizeof(data));
    nvs_close(nvs_ns);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed to nvs_set_blob: %d", ret);
    }
}

void Context::initIrSensors() {
    adc1_config_width(ADC_WIDTH_12Bit);
    adc1_config_channel_atten(m_ir_left, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(m_ir_right, ADC_ATTEN_DB_11);
}

uint16_t Context::irRead(adc1_channel_t chan, uint16_t samples) {
    bool ex = false;
    if (m_ir_installed.compare_exchange_strong(ex, true)) {
        initIrSensors();
    }

    uint32_t reading = 0;
    for (uint16_t i = 0; i < samples; ++i) {
        reading += adc1_get_raw(chan);
    }
    return reading / samples;
}

void Context::stupidServoSet(uint8_t id, float positionDegrees) {
    const auto coef = rb::clamp(positionDegrees, -90.f, 90.f) / 90.f;
    const auto val = coef >= 0.f ? coef * m_stupid_servo_max : coef * m_stupid_servo_min * -1;
    Manager::get().stupidServo(id).setPosition(val);
}

float Context::stupidServoGet(uint8_t id) {
    const auto val = Manager::get().stupidServo(id).position();
    if (std::isnan(val))
        return val;

    if (val >= 0.f) {
        return val / m_stupid_servo_max * 90.f;
    } else {
        return val / m_stupid_servo_min * -90.f;
    }
}

}; // namespace rk
