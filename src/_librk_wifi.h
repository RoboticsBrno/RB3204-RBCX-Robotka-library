#pragma once

#include <mutex>
#include <string.h>

#ifndef RK_DISABLE_BLE
#include <BLEServer.h>
#else
class BLECharacteristicCallbacks {
};
typedef void BLECharacteristic;
typedef int Status;
#endif

#include "robotka.h"

namespace rk {

class WiFi;

class WiFi : public BLECharacteristicCallbacks {
public:
    WiFi();
    virtual ~WiFi();

    void init(const rkConfig& cfg);

    void disableBle();

    virtual void onRead(BLECharacteristic*) {}
    virtual void onNotify(BLECharacteristic*) {}
    virtual void onStatus(BLECharacteristic*, Status, uint32_t) {}

    virtual void onWrite(BLECharacteristic* chr);

private:
    struct Config {
        Config();
        Config(bool station_mode_, const rkConfig& cfg);

        std::string format() const;
        bool parse(const std::string& saved_data);

        bool load();
        void save() const;

        bool station_mode;
        std::string name;
        std::string password;
        uint8_t channel;
    };

    void setupWifi(const Config& cfg);

#ifndef RK_DISABLE_BLE
    void setupBle(const rkConfig& cfg, const Config& wifiCfg);

    void scheduleIpUpdateLocked();
    bool updateIpChar();

    bool m_ip_update_running;
    BLECharacteristic* m_ip_char;

    uint32_t m_battery_level;

    bool m_ble_running;
    BLEServer* m_server;
    std::vector<BLEService*> m_services;
    std::vector<BLECharacteristic*> m_chars;
#endif

    std::mutex m_mutex;
    bool m_esp_wifi_started;
    char m_ssid[32];
};
};
