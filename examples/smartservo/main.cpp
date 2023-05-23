#include <Arduino.h>

#include "robotka.h"

using namespace lx16a; // aby nebylo třeba to psát všude

void setup() {
    rkConfig cfg;
    cfg.motor_max_power_pct = 30; // limit the power
    rkSetup(cfg);

    printf("Robotka started!\n");

    // Pripoj jedni chytre servo s id 0
    auto &bus = rkSmartServoBus(1);
    printf("Servo 0 je na pozici %f stupnu\n", bus.pos(0).deg());
}

void loop() {

    // Protože tohle už je druhé volání rkSmartServoBus, lze použít 0 jako servo_count
    auto &bus = rkSmartServoBus(0); 

    uint8_t pos = 0;
    while(true) {
        bus.set(0, Angle::deg(pos));
        pos += 20;
        sleep(1);
    }
}
