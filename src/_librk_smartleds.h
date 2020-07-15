#include <memory>
#include <mutex>

#include "SmartLeds.h"

#include "robotka.h"

namespace rk {

class SmartLeds {
public:
    SmartLeds();
    ~SmartLeds();

    void init(const rkConfig& cfg);

    uint16_t count() const { return m_count; }

    void setRGB(uint16_t idx, uint8_t r, uint8_t g, uint8_t b);
    void setHSV(uint16_t idx, uint8_t h, uint8_t s, uint8_t v);

    SmartLed& controller() { return *m_controller; }

private:
    bool update();
    void scheduleUpdateLocked();

    std::mutex m_mutex;
    std::unique_ptr<SmartLed> m_controller;
    uint16_t m_timerId;
    uint16_t m_count;
};
};
