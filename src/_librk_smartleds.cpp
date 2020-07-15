#include "_librk_smartleds.h"

namespace rk {

SmartLeds::SmartLeds()
    : m_timerId(0)
    , m_count(0) {}

SmartLeds::~SmartLeds() {
}

void SmartLeds::init(const rkConfig& cfg) {
    m_count = cfg.smart_leds_count;
    if (m_count != 0)
        m_controller.reset(new SmartLed(LED_WS2812B, cfg.smart_leds_count, cfg.pins.smartled_sig));
}

void SmartLeds::setRGB(uint16_t idx, uint8_t r, uint8_t g, uint8_t b) {
    std::lock_guard<std::mutex> l(m_mutex);
    if (m_count == 0)
        return;
    (*m_controller)[idx] = Rgb { r, g, b };
    scheduleUpdateLocked();
}

void SmartLeds::setHSV(uint16_t idx, uint8_t h, uint8_t s, uint8_t v) {
    std::lock_guard<std::mutex> l(m_mutex);
    if (m_count == 0)
        return;
    (*m_controller)[idx] = Hsv { h, s, v };
    scheduleUpdateLocked();
}

void SmartLeds::scheduleUpdateLocked() {
    if (m_timerId != 0)
        return;
    m_timerId = rb::Timers::get().schedule(std::max(10, (int)m_count), [this]() {
        return this->update();
    });
}

bool SmartLeds::update() {
    if(!m_controller->wait(0)) {
        return true;
    }

    std::lock_guard<std::mutex> l(m_mutex);
    m_controller->show();
    m_timerId = 0;
    return false;
}
};
