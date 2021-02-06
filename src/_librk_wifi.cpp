#include <sstream>

#include <esp_log.h>
#include <esp_wifi.h>

#include <RBCX.h>
#include <RBCXNvs.h>
#include <rbwifi.h>

#include "_librk_wifi.h"

#ifndef RK_DISABLE_BLE
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#define RBCONTROL_SERVICE_UUID "be669d73-de9c-409e-848e-889a5fc66c0e"
#define OWNER_UUID "c89b7fef-341f-43c9-a561-58e5476add31"
#define NAME_UUID "41b706e4-af8f-4544-a271-d484863526fd"
#define WIFI_CONFIG_UUID "f33ff70e-4b87-484d-bf95-ff856582c82c"
#define WIFI_IP_UUID "9def56ce-80d7-4c17-a68c-3cc480763bd4"

#define BATTERY_SERVICE_UUID "180F"
#define BATTERY_LEVEL_UUID "2A19"
#endif

#define TAG "robotka"

#define NVS_NS "rkWiFi"
#define NVS_KEY "wifi_cfg"

namespace rk {

WiFi::WiFi() {
    m_esp_wifi_started = false;
    memset(m_ssid, 0, sizeof(m_ssid));

#ifndef RK_DISABLE_BLE
    m_ble_running = false;
    m_ip_update_running = false;
    m_ip_char = nullptr;
#endif
}

WiFi::~WiFi() {
}

void WiFi::init(const rkConfig& cfg) {
    auto& man = rb::Manager::get();

    const bool has_station_name = cfg.wifi_name && strlen(cfg.wifi_name) != 0;
#ifndef RK_DISABLE_BLE
    const bool has_ap_pass = cfg.wifi_ap_password && strlen(cfg.wifi_ap_password) != 0;
    const bool enable_ble = (cfg.rbcontroller_app_enable && !has_station_name && !cfg.wifi_default_ap && (!has_ap_pass || strcmp(cfg.wifi_ap_password, RK_DEFAULT_WIFI_AP_PASSWORD) == 0));
#else
    const bool enable_ble = false;
#endif

    bool station_mode = man.buttons().up() == cfg.wifi_default_ap && has_station_name;

    snprintf(m_ssid, sizeof(m_ssid), "%s-%s", cfg.owner, cfg.name);

    Config wifiCfg;
    if (!enable_ble || !wifiCfg.load()) {
        wifiCfg = Config(station_mode, cfg);
    }

    setupWifi(wifiCfg);

#ifndef RK_DISABLE_BLE
    if (enable_ble) {
        esp_bt_mem_release(ESP_BT_MODE_CLASSIC_BT);
        setupBle(cfg, wifiCfg);
    } else {
        esp_bt_mem_release(ESP_BT_MODE_BTDM);
    }
#endif
}

void WiFi::setupWifi(const Config& cfg) {
    const std::lock_guard<std::mutex> lock(m_mutex);

    auto& man = rb::Manager::get();

    if (m_esp_wifi_started) {
        esp_wifi_stop();
    }
    man.leds().yellow(false);
    man.leds().green(false);

#ifndef RK_DISABLE_BLE
    scheduleIpUpdateLocked();
#endif

    if (cfg.station_mode) {
        if (!cfg.name.empty()) {
            man.leds().yellow();
            printf("Connecting to WiFi %s\n", cfg.name.c_str());
            rb::WiFi::connect(cfg.name.c_str(), cfg.password.c_str());
            m_esp_wifi_started = true;
        } else {
            ESP_LOGE(TAG, "Can't connect to WiFi, the wifi_name is empty!");
        }
    } else {
        man.leds().green();
        printf("Starting WiFi AP %s\n", m_ssid);
        rb::WiFi::startAp(m_ssid, cfg.password.c_str(), cfg.channel);
        m_esp_wifi_started = true;
    }
}

void WiFi::disableBle() {
#ifndef RK_DISABLE_BLE
    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_ble_running)
            return;

        m_ip_char = nullptr;
        m_ble_running = false;
    }

    // There is no way to cleanly destroy all the Arduino stuff, try our best :/
    // The commented out stuff are not safe.
    /*for (auto* s : m_services) {
        m_server->removeService(s);
    }*/

    BLEDevice::getAdvertising()->stop();

    BLEDevice::deinit(false);
    esp_bt_mem_release(ESP_BT_MODE_BTDM);

    delete BLEDevice::getAdvertising();
    //delete m_server;
    m_server = nullptr;

    /*for (auto* c : m_chars) {
        delete c;
    }*/
    m_chars.clear();
    m_chars.shrink_to_fit();

    /*for (auto* s : m_services) {
        delete s;
    }*/
    m_services.clear();
    m_services.shrink_to_fit();
#endif
}

#ifndef RK_DISABLE_BLE
void WiFi::setupBle(const rkConfig& cfg, const Config& wifiCfg) {
    const std::lock_guard<std::mutex> lock(m_mutex);

    auto& man = rb::Manager::get();

    // Preinit in BLE-only mode, saves few KB of RAM
    if (esp_bt_controller_get_status() != ESP_BT_CONTROLLER_STATUS_ENABLED) {
        if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE) {
            esp_bt_controller_config_t cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
            cfg.mode = ESP_BT_MODE_BLE;
            esp_bt_controller_init(&cfg);
            while (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE) {
            }
        }
        if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_INITED) {
            esp_bt_controller_enable(ESP_BT_MODE_BLE);
        }
    }

    m_ble_running = true;

    BLEDevice::init(std::string(m_ssid, std::min(strlen(m_ssid), (size_t)16)));

    m_server = BLEDevice::createServer();

    {
        auto* service = m_server->createService(RBCONTROL_SERVICE_UUID);

        auto* owner = service->createCharacteristic(OWNER_UUID, BLECharacteristic::PROPERTY_READ);
        owner->setValue(cfg.owner);
        m_chars.push_back(owner);

        auto* name = service->createCharacteristic(NAME_UUID, BLECharacteristic::PROPERTY_READ);
        name->setValue(cfg.name);
        m_chars.push_back(name);

        auto ip = rb::WiFi::getIp();
        m_ip_char = service->createCharacteristic(WIFI_IP_UUID,
            BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
        m_ip_char->setValue(ip);
        m_chars.push_back(m_ip_char);

        auto* cfg_char = service->createCharacteristic(WIFI_CONFIG_UUID,
            BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
        cfg_char->setValue(wifiCfg.format().c_str());
        cfg_char->setCallbacks(this);
        m_chars.push_back(cfg_char);

        service->start();
        m_services.push_back(service);
    }

    {
        auto* service = m_server->createService(BATTERY_SERVICE_UUID);
        auto* const batt_chr = service->createCharacteristic(BATTERY_LEVEL_UUID,
            BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
        m_battery_level = man.battery().pct();
        batt_chr->setValue(m_battery_level);
        m_chars.push_back(batt_chr);

        service->start();
        m_services.push_back(service);

        man.schedule(5000, [=]() -> bool {
            const std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_ble_running)
                return false;

            const auto new_lvl = rb::Manager::get().battery().pct();
            if (new_lvl != m_battery_level) {
                m_battery_level = new_lvl;
                batt_chr->setValue(m_battery_level);
                batt_chr->notify();
            }
            return true;
        });
    }

    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(RBCONTROL_SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    scheduleIpUpdateLocked();
}

void WiFi::scheduleIpUpdateLocked() {
    if (m_ip_update_running || m_ip_char == nullptr) {
        return;
    }

    m_ip_update_running = true;
    rb::Manager::get().schedule(300, [=]() -> bool {
        return updateIpChar();
    });
}

bool WiFi::updateIpChar() {
    auto ip = rb::WiFi::getIp();
    if (ip == 0) {
        return true;
    }

    const std::lock_guard<std::mutex> lock(m_mutex);
    if (m_ip_char == nullptr)
        return false;

    m_ip_char->setValue(ip);
    m_ip_char->notify();
    m_ip_update_running = false;
    return false;
}

#endif // #ifndef RK_DISABLE_BLE

void WiFi::onWrite(BLECharacteristic* chr) {
#ifndef RK_DISABLE_BLE
    Config cfg;
    if (!cfg.parse(chr->getValue())) {
        ESP_LOGE(TAG, "failed to parse WiFi config submitted via BLE: %s", chr->getValue().c_str());
        return;
    }

    setupWifi(cfg);
    cfg.save();
#endif
}

bool WiFi::Config::load() {
    rb::Nvs nvs(NVS_NS);
    if (!nvs.existsString(NVS_KEY)) {
        return false;
    }

    auto cfg_string = nvs.getString(NVS_KEY);
    return parse(cfg_string);
}

void WiFi::Config::save() const {
    rb::Nvs nvs(NVS_NS);
    nvs.writeString(NVS_KEY, format());
}

WiFi::Config::Config() {
    station_mode = true;
    password = "flusflus";
}

WiFi::Config::Config(bool station_mode_, const rkConfig& cfg) {
    channel = cfg.wifi_ap_channel;
    station_mode = station_mode_;
    if (station_mode) {
        name = cfg.wifi_name ? cfg.wifi_name : "";
        password = cfg.wifi_password ? cfg.wifi_password : "";
    } else {
        password = cfg.wifi_ap_password ? cfg.wifi_ap_password : "";
    }
}

static const char CONFIG_SEP = '\n';

std::string WiFi::Config::format() const {
    std::ostringstream str;
    str << (int)station_mode << CONFIG_SEP;
    str << name << CONFIG_SEP;
    str << password << CONFIG_SEP;
    str << (int)channel << CONFIG_SEP;
    return str.str();
}

bool WiFi::Config::parse(const std::string& saved_data) {
    size_t pos = 0;
    size_t part = 0;

    while (true) {
        size_t idx = saved_data.find(CONFIG_SEP, pos);
        if (idx == std::string::npos) {
            break;
        }

        std::string tok = saved_data.substr(pos, idx - pos);
        pos = idx + 1;

        switch (part++) {
        case 0:
            station_mode = (tok == "1");
            break;
        case 1:
            name = tok;
            break;
        case 2:
            password = tok;
            break;
        case 3:
            channel = atoi(tok.c_str());
            channel = channel != 0 ? channel : 1;
            break;
        }
    }

    return part >= 4;
}

};
