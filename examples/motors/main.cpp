#include <Arduino.h>

#include "robotka.h"

void setup() {
    rkConfig cfg;
    cfg.motor_max_power_pct = 30; // limit the power
    rkSetup(cfg);

    printf("Robotka started!\n");

    rkLedBlue();
}

void loop() {
    rkMotorsSetPower(100, 100);
    sleep(1);

    rkMotorsSetPower(0, 0);
    sleep(1);

    rkMotorsSetPower(-100, -100);
    sleep(1);

    rkMotorsSetPower(0, 0);
    sleep(5);
}
