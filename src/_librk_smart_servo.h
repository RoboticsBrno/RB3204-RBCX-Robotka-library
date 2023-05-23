#pragma once

#include "SmartServoBus.hpp"
#include "RBCXManager.h"

namespace rk {

class SmartServoBusInitializer {
public:
    SmartServoBusInitializer(uint8_t servo_count) {
        auto& man = rb::Manager::get();
        m_bus.begin(servo_count, &man.smartServoBusBackend());
    }

    lx16a::SmartServoBus& bus() { return m_bus; }

private:
    lx16a::SmartServoBus m_bus;
};

};
