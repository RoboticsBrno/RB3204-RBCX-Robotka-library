/**
 * @file robotka.h
 *
 * Metody v tomto souboru vám dovolují jednoduše obsluhovat Robotku.
 *
 */

#ifndef _LIBRB_H
#define _LIBRB_H

#include <memory>

#include <fmt/core.h>
#include <fmt/printf.h>

#include <Arduino.h>

#include "RBCX.h"
#include "gridui.h"
#include "rbprotocol.h"

using namespace gridui;

/**
 * \defgroup general Inicializace
 *
 * Tato sekce je určená k počátečnímu nastavení knihovny Robotka.
 *
 * @{
 */

/**
 * \brief Nastavení čísel pinů různých periferií.
 *
 * Zde můžete přenastavit piny, pokud máte periferie připojené na desce na jíném pinu.
 */
struct rkPinsConfig {
    rkPinsConfig()
        : line_cs(4)
        , line_mosi(14)
        , line_miso(27)
        , line_sck(26) {
    }

    uint8_t line_cs;
    uint8_t line_mosi;
    uint8_t line_miso;
    uint8_t line_sck;
};

#define RK_DEFAULT_WIFI_AP_PASSWORD "flusflus" //!< Výchozí heslo pro WiFi AP

/**
 * \brief Nastavení SW pro Robotky
 *
 * Tato struktura obsahuje konfigurační hodnoty pro software Robotky.
 * Předává se funkci rkSetup(). Ve výchozím stavu má smysluplné hodnoty
 * a není třeba nastavovat všechny, ale jen ty, které chcete změnit.
 */
struct rkConfig {
    rkConfig()
        : rbcontroller_app_enable(false)
        , owner("Nenastaven")
        , name("Nenastaveno")
        , wifi_name("")
        , wifi_password("")
        , wifi_default_ap(false)
        , wifi_ap_password(RK_DEFAULT_WIFI_AP_PASSWORD)
        , wifi_ap_channel(1)
        , motor_id_left(4)
        , motor_id_right(1)
        , motor_max_power_pct(60)
        , motor_polarity_switch_left(false)
        , motor_polarity_switch_right(true)
        , motor_enable_failsafe(false) {
    }

    bool rbcontroller_app_enable; //!< povolit komunikaci s aplikací RBController. Výchozí: `false`

    const char* owner; //!< Jméno vlastníka robota. Podle tohoto jména filtruje RBController roboty. Výchozí: `""`
    const char* name; //!< Jméno robota. Výchozí: ""

    const char* wifi_name; //!< Jméno WiFi sítě, na kterou se připojovat. Výchozí: `""`
    const char* wifi_password; //!< Heslo k WiFi, na kterou se připojit. Výchozí: `""`

    bool wifi_default_ap; //!< Vytvářet WiFi síť místo toho, aby se připojovalo k wifi_name. Výchozí: `false`
    const char* wifi_ap_password; //!< Heslo k vytvořené síti. Výchozí: `"flusflus"`
    uint8_t wifi_ap_channel; //!< Kanál WiFi vytvořené sítě. Výchozí: `1`

    uint8_t motor_id_left; //!< Které M číslo motoru patří levému, podle čísla na desce. Výchozí: `2`
    uint8_t motor_id_right; //!< Které M číslo motoru patří pravému, podle čísla na desce. Výchozí: `1`
    uint8_t motor_max_power_pct; //!< Limit výkonu motoru v procentech od 0 do 100. Výchozí: `60`
    bool motor_polarity_switch_left; //!< Prohození polarity levého motoru. Výchozí: `false`
    bool motor_polarity_switch_right; //!< Prohození polarity pravého motoru. Výchozí: `true`
    bool motor_enable_failsafe; //!< Zastaví motory po 500ms, pokud není zavoláno rkSetMotorPower nebo rkSetMotorSpeed. Výchozí: `false`

    rkPinsConfig pins; //!< Konfigurace pinů pro periferie, viz rkPinsConfig
};

typedef enum {
    BTN_DOWN = rb::ButtonId::Down,
    BTN_UP = rb::ButtonId::Up,
    BTN_LEFT = rb::ButtonId::Left,
    BTN_RIGHT = rb::ButtonId::Right,
    BTN_ON = rb::ButtonId::On,
    BTN_OFF = rb::ButtonId::Off,
} rkButtonId;

/**
 * \brief Inicializační funkce Robotky
 *
 * Tuhle funci MUSÍTE zavolat vždy na začátku vaší funkce setup() v main.cpp.
 * Můžete jí předat nastavení ve formě struktury rkConfig.
 */
void rkSetup(const rkConfig& cfg = rkConfig());

/**@}*/
/**
 * \defgroup motors Motory
 *
 * Metody pro obsluhu motorů.
 * @{
 */

/**
 * \brief Nastavení výkonu motorů.
 *
 * \param left výkon levého motoru od od -100% do 100%
 * \param right výkon pravého motoru od od -100 do 100%
 */
void rkMotorsSetPower(int8_t left, int8_t right);

/**
 * \brief Nastavení výkonu levého motoru.
 *
 * \param power výkon levého motoru od od -100% do 100%
 */
void rkMotorsSetPowerLeft(int8_t power);

/**
 * \brief Nastavení výkonu pravého motoru.
 *
 * \param power výkon levého motoru od od -100% do 100%
 */
void rkMotorsSetPowerRight(int8_t power);

/**
 * \brief Nastavení výkonu motoru podle jeho čísla (M1...M4) na desce.
 *
 * \param id číslo motoru od 1 do 4 včetně
 * \param power výkon motoru od od -100% do 100%
 */
void rkMotorsSetPowerById(int id, int8_t power);

/**
 * \brief Nastavení rychlosti motorů.
 *
 * \param left rychlost levého motoru od od -100% do 100%
 * \param right rychlost pravého motoru od od -100% do 100%
 */
void rkMotorsSetSpeed(int8_t left, int8_t right);

/**
 * \brief Nastavení rychlosti levého motoru.
 *
 * \param speed rychlost levého motoru od od -100% do 100%
 */
void rkMotorsSetSpeedLeft(int8_t speed);

/**
 * \brief Nastavení rychlosti pravého motoru.
 *
 * \param speed rychlost levého motoru od od -100% do 100%
 */
void rkMotorsSetSpeedRight(int8_t speed);

/**
 * \brief Nastavení rychlosti motoru podle jeho čísla (M1...M4) na desce.
 *
 * \param id číslo motoru od 1 do 4 včetně
 * \param speed rychlost motoru od od -100% do 100%
 */
void rkMotorsSetSpeedById(int id, int8_t speed);

/**
 * \brief Nastavení rychlosti motorů podle joysticku.
 *
 * Tato funkce nastaví rychlost motorů podle výstupu z joysticku. Očekává dvě
 * hodnoty od -32768 do 32768, posílané například aplikací RBController.
 * Funkce tyto hodnoty převede na rychlost a nastaví ji.
 *
 * \param x X hodnota z joysticku.
 * \param y Y hodnota z joysticku.
 */
void rkMotorsJoystick(int32_t x, int32_t y);

/**@}*/
/**
 * \defgroup battery Baterie
 *
 * Metody pro získání informací o stavu baterie.
 * @{
 */

/**
 * \brief Úroveň baterie v procentech
 *
 * \return Hodnota od 0 do 100 udávající nabití baterie.
 */
uint32_t rkBatteryPercent();

/**
 * \brief Úroveň baterie v mV.
 *
 * \return Naměřené napětí na baterii v milivoltech.
 */
uint32_t rkBatteryVoltageMv();

/**@}*/
/**
 * \defgroup rbcontroller Aplikace RBController
 *
 * Metody pro komunikaci s aplikací RBController pro Android.
 * @{
 */

/**
 * \brief Odeslat text do aplikace.
 *
 * Tato metoda odešle text, který se zobrazí v aplikaci v černém okně nahoře.
 * Příklad:
 *
 *     rkControllerSendLog("Test logu! Stav Baterie: %u mv", rkBatteryVoltageMv());
 *
 * \param format Text k odeslání, může obsahovat formátovací značky jako C funkce printf().
 * \param ... argumenty pro format, funguje stejně jako printf().
 */
void rkControllerSendLog(const char* format, ...);
void rkControllerSendLog(const std::string& text);

void rkControllerSend(const char* cmd, rbjson::Object* data = nullptr);
void rkControllerSendMustArrive(const char* cmd, rbjson::Object* data = nullptr);

/**@}*/
/**
 * \defgroup leds LEDky
 *
 * Metody pro zapínaní a vypínání LEDek na desce.
 * @{
 */

/**
 * \brief Zapnout/vypnout červenou LED
 * \param on `true` pro zapnuto, `false` pro vypnuto.
 */
void rkLedRed(bool on = true);
/**
 * \brief Zapnout/vypnout žlutou LED
 * \param on `true` pro zapnuto, `false` pro vypnuto.
 */
void rkLedYellow(bool on = true);
/**
 * \brief Zapnout/vypnout zelenou LED
 * \param on `true` pro zapnuto, `false` pro vypnuto.
 */
void rkLedGreen(bool on = true);
/**
 * \brief Zapnout/vypnout modrou LED
 * \param on `true` pro zapnuto, `false` pro vypnuto.
 */
void rkLedBlue(bool on = true);

/**
 * \brief Zapnout/vypnout všechny LED zaráz
 * \param on `true` pro zapnuto, `false` pro vypnuto.
 */
void rkLedAll(bool on = true);

/**
 * \brief Zapnout/vypnout LED podle jejich čísla na desce, od 1 do 4 včetně.
 * \param on `true` pro zapnuto, `false` pro vypnuto.
 */
void rkLedById(uint8_t id, bool on = true);

/**@}*/
/**
 * \defgroup buttons Tlačítka
 *
 * Funkce pro vyčítání stavu tlačítek.
 * @{
 */

/**
 * \brief Je teď stisknuto tlačítko?
 *
 * \param id ID tlačítka z enumu rkButtonId
 * \param waitForRelease pokud je stisknuto, počká před vrácením výsledku na jeho uvolnění (default: false)
 * \return Vrátí `true` pokud je tlačítko stisknuto.
 */
bool rkButtonIsPressed(rkButtonId id, bool waitForRelease = false);

/**
 * \brief Je teď stisknuto "dolů"?
 *
 * \param waitForRelease pokud je stisknuto, počká před vrácením výsledku na jeho uvolnění (default: false)
 * \return Vrátí `true` pokud je tlačítko stisknuto.
 */
inline bool rkButtonDown(bool waitForRelease = false) {
    return rkButtonIsPressed(BTN_DOWN, waitForRelease);
}

/**
 * \brief Je teď stisknuto "nahoru"?
 *
 * \param waitForRelease pokud je stisknuto, počká před vrácením výsledku na jeho uvolnění (default: false)
 * \return Vrátí `true` pokud je tlačítko stisknuto.
 */
inline bool rkButtonUp(bool waitForRelease = false) {
    return rkButtonIsPressed(BTN_UP, waitForRelease);
}

/**
 * \brief Je teď stisknuto "doleva"?
 *
 * \param waitForRelease pokud je stisknuto, počká před vrácením výsledku na jeho uvolnění (default: false)
 * \return Vrátí `true` pokud je tlačítko stisknuto.
 */
inline bool rkButtonLeft(bool waitForRelease = false) {
    return rkButtonIsPressed(BTN_LEFT, waitForRelease);
}

/**
 * \brief Je teď stisknuto "doprava"?
 *
 * \param waitForRelease pokud je stisknuto, počká před vrácením výsledku na jeho uvolnění (default: false)
 * \return Vrátí `true` pokud je tlačítko stisknuto.
 */
inline bool rkButtonRight(bool waitForRelease = false) {
    return rkButtonIsPressed(BTN_RIGHT, waitForRelease);
}

/**
 * \brief Je teď stisknuto "ON"?
 *
 * \param waitForRelease pokud je stisknuto, počká před vrácením výsledku na jeho uvolnění (default: false)
 * \return Vrátí `true` pokud je tlačítko stisknuto.
 */
inline bool rkButtonOn(bool waitForRelease = false) {
    return rkButtonIsPressed(BTN_ON, waitForRelease);
}

/**
 * \brief Je teď stisknuto "OFF"?
 *
 * \param waitForRelease pokud je stisknuto, počká před vrácením výsledku na jeho uvolnění (default: false)
 * \return Vrátí `true` pokud je tlačítko stisknuto.
 */
inline bool rkButtonOff(bool waitForRelease = false) {
    return rkButtonIsPressed(BTN_OFF, waitForRelease);
}

/**
 * \brief Počkat, dokud není tlačítko uvolněno.
 * 
 * Pokud tlačítko není stisknuté, počká pouze několik desítek ms, tedy nečeká na stisknutí.
 *
 * \param id ID tlačítka z enumu rkButtonId
 */
void rkButtonWaitForRelease(rkButtonId id);

/**@}*/

/**@}*/
/**
 * \defgroup line Sledování čáry
 *
 * Funkce pro komunikaci se senzory na čáru.
 * @{
 */

/**
 * \brief Kalibrovat senzory na čáru.
 *
 * Otočí robota doprava a pak doleva a zase zpět tak, aby lišta se senzory
 * prošla celá nad čárou i nad okolím. Trvá to 2.2s, funkce po celou dobu
 * kalibrace čeká a vrátí se až po dokončení.
 *
 * Předpokládá se, že před zavoláním této metody roboto stojí tak, že prostřední senzory
 * jsou nad čárou.
 *
 * Kalibrační hodnoty se ukládají do paměti, kalibraci je třeba dělat pouze když
 * je ruka přesunuta na jiný podklad.
 *
 * \param motor_time_coef tento koeficient změní jak dlouho se roboto otáčí, změňte pokud se vaše ruka
 *                        neotočí tak, že senzory projedou všechny nad čárou.
 */
void rkLineCalibrate(float motor_time_coef = 1.0);

/**
 * \brief Vymazat kalibraci
 *
 * Vymazat nastavenou kalibraci a dále používat nezkalibrované hodnoty.
 */
void rkLineClearCalibration();

/**
 * \brief Hodnota z jednoho senzoru na čáru.
 *
 * \param sensorId číslo senzoru od 0 do 7 včetně
 * \return naměřená hodnota od 0 do 1023
 */
uint16_t rkLineGetSensor(uint8_t sensorId);

/**
 * \brief Pozice čáry pod senzory
 *
 * Tato funkce se pokouší najít černou čáru pod senzory.
 *
 * \param white_line nastavte na true, pokud sledujete bílou čáru na černém podkladu. Výchozí: false
 * \param line_threshold_pct Jak velký rozdíl v procentech musí mezi hodnotami být, aby byla čára považována za nalezenou. Výchozí: 25%
 * \return Desetinná hodnota od -1 do +1. -1 znamená, že čára je úplně vlevo, 0 že je uprostřed a 1 že je úplně vpravo.
 *         Vrátí NaN, pokud nenalezne čáru - výsledek otestujte funkcí isnan() - `isnan(line_position)`
 */
float rkLineGetPosition(bool white_line = false, uint8_t line_threshold_pct = 25);

/**@}*/

#endif // LIBRB_H
