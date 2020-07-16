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

#include "SmartLeds.h"

#include "RBCX.h"
#include "gridui.h"
#include "rbprotocol.h"

using namespace gridui;

/**
 * \defgroup general .INICIALIZACE ROBOTA
 *
 * Tato sekce je určená k počátečnímu nastavení knihovny pro Robotku.
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
        : line_cs(5)
        , line_mosi(23)
        , line_miso(19)
        , line_sck(18)
        , smartled_sig(12)
        , ir_adc_chan_left(ADC1_CHANNEL_0)
        , ir_adc_chan_right(ADC1_CHANNEL_3) {
    }

    uint8_t line_cs;
    uint8_t line_mosi;
    uint8_t line_miso;
    uint8_t line_sck;

    uint8_t smartled_sig;

    adc1_channel_t ir_adc_chan_left;
    adc1_channel_t ir_adc_chan_right;
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
        , motor_id_left(1)
        , motor_id_right(4)
        , motor_max_power_pct(80)
        , motor_polarity_switch_left(false)
        , motor_polarity_switch_right(true)
        , motor_enable_failsafe(false)
        , motor_wheel_diameter(67)
        , motor_max_ticks_per_second(2000)
        , motor_max_acceleration(10000)
        , stupid_servo_min(-1.65f)
        , stupid_servo_max(1.65f)
        , smart_leds_count(8) {
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
    uint16_t motor_wheel_diameter; //!< Průměr kol robota v mm, použito na počítání ujeté vzdálenosti. Výchozí: `67` mm.

    /**
     * \brief Maximální rychlost motorů v ticích enkodéru za vteřinu. Výchozí: `2000`
     *
     * Ovlivňuje regulátor rychlosti motorů, který se používá u funkcí rkMotorsSetSpeed
     * a rkMotorsDrive. Oba dva motory by měli být schopny dosáhnout tuto rychlost.
     */
    uint32_t motor_max_ticks_per_second;

    /**
     * \brief Maximální zrychlení motorů v ticích enkodéru za vteřinu. Výchozí: `50000`
     *
     * Ovlivňuje regulátor rychlosti motorů, který se používá u funkcí rkMotorsSetSpeed
     * a rkMotorsDrive. Vyšší číslo znamená, že motory budou mít rychlejší náběh na cílovou rychlost,
     * ale za to se mohou smýkat po podlaze.
     */
    uint32_t motor_max_acceleration;

    float stupid_servo_min; //!< Spodní hranice signálu pro hloupá serva, která se robvná -90 stupňům. Výchozí: `-1.65`
    float stupid_servo_max; //!< Horní hranice signálu pro hloupá serva, která se rovná 90 stupňům. Výchozí: `1.65`

    uint16_t smart_leds_count; //!< Nastavení počtu připojených chytrých LED. Výchozí: 8

    rkPinsConfig pins; //!< Konfigurace pinů pro periferie, viz rkPinsConfig
};

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
void rkMotorsSetPowerById(uint8_t id, int8_t power);

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
void rkMotorsSetSpeedById(uint8_t id, int8_t speed);

/**
 * \brief Popojetí oběma motory o vzdálenost v mm (blokující).
 *
 * Tato funkce zablokuje program, dokud robot danou vzdálenost neujede.
 *
 * \param mmLeft kolik milimetrů má ujet levý motor
 * \param mmRight kolik milimetrů má ujet pravý motor
 * \param speed rychlost, kterou má robot jet od -100% do 100%
 */
void rkMotorsDrive(float mmLeft, float mmRight, uint8_t speed);

/**
 * \brief Popojetí levým motorem o vzdálenost v mm (blokující).
 *
 * Tato funkce zablokuje program, dokud robot danou vzdálenost neujede.
 *
 * \param mm kolik milimetrů má ujet levý motor
 * \param speed rychlost, kterou má motor jet od -100% do 100%
 */
void rkMotorsDriveLeft(float mm, uint8_t speed);

/**
 * \brief Popojetí pravým motorem o vzdálenost v mm (blokující).
 *
 * Tato funkce zablokuje program, dokud robot danou vzdálenost neujede.
 *
 * \param mm kolik milimetrů má ujet pravý motor
 * \param speed rychlost, kterou má motor jet od -100% do 100%
 */
void rkMotorsDriveRight(float mm, uint8_t speed);

/**
 * \brief Popojetí motorem podle ID o vzdálenost v mm (blokující).
 *
 * Tato funkce zablokuje program, dokud robot danou vzdálenost neujede.
 *
 * \param id číslo motoru, který má jet, od 1 do 4 včetně
 * \param mm kolik milimetrů má motor ujet
 * \param speed rychlost, kterou má motor jet od -100% do 100%
 */
void rkMotorsDriveById(uint8_t id, float mm, uint8_t speed);

/**
 * \brief Popojetí oběma motory o vzdálenost v mm (asynchroní).
 *
 * Tato funkce vrátí okamžítě, a po dojetí zavolá callback.
 *
 * Příklad použití:
 * \code{cpp}
 * rkMotorsDriveAsync(100, 100, 50, []() {
 *     printf("Dojel jsem!\n");
 * });
 * \endcode
 *
 * \param mmLeft kolik milimetrů má ujet levý motor
 * \param mmRight kolik milimetrů má ujet pravý motor
 * \param speed rychlost, kterou má robot jet od -100% do 100%
 * \param callback funkce, která je zavolána jakmile robot dojede o určenou vzdálenost
 */
void rkMotorsDriveAsync(float mmLeft, float mmRight, uint8_t speed, std::function<void()> callback = nullptr);

/**
 * \brief Popojetí levým motorem o vzdálenost v mm (asynchroní).
 *
 * Tato funkce vrátí okamžítě, a po dojetí zavolá callback.
 *
 * Příklad použití:
 * \code{cpp}
 * rkMotorsDriveLeftAsync(100, 50, []() {
 *     printf("Dojel jsem!\n");
 * });
 * \endcode
 *
 * \param mm kolik milimetrů má ujet levý motor
 * \param speed rychlost, kterou má robot jet od -100% do 100%
 * \param callback funkce, která je zavolána jakmile robot dojede o určenou vzdálenost
 */
void rkMotorsDriveLeftAsync(float mm, uint8_t speed, std::function<void()> callback = nullptr);

/**
 * \brief Popojetí pravým motorem o vzdálenost v mm (asynchroní).
 *
 * Tato funkce vrátí okamžítě, a po dojetí zavolá callback.
 *
 * Příklad použití:
 * \code{cpp}
 * rkMotorsDriveRightAsync(100, 50, []() {
 *     printf("Dojel jsem!\n");
 * });
 * \endcode
 *
 * \param mm kolik milimetrů má ujet pravý motor
 * \param speed rychlost, kterou má robot jet od -100% do 100%
 * \param callback funkce, která je zavolána jakmile robot dojede o určenou vzdálenost
 */
void rkMotorsDriveRightAsync(float mm, uint8_t speed, std::function<void()> callback = nullptr);

/**
 * \brief Popojetí motorem podle ID o vzdálenost v mm (asynchroní).
 *
 * Tato funkce vrátí okamžítě, a po dojetí zavolá callback.
 *
 * Příklad použití:
 * \code{cpp}
 * rkMotorsDriveByIdAsync(2, 100, 50, []() {
 *     printf("Dojel jsem!\n");
 * });
 * \endcode
 *
 * \param id číslo motoru, který má jet, od 1 do 4 včetně
 * \param mm kolik milimetrů má ujet pravý motor
 * \param speed rychlost, kterou má robot jet od -100% do 100%
 * \param callback funkce, která je zavolána jakmile robot dojede o určenou vzdálenost
 */
void rkMotorsDriveByIdAsync(uint8_t id, float mm, uint8_t speed, std::function<void()> callback = nullptr);

/**
 * \brief Vrátí absolutní najetou vzálenost na levém motoru v mm
 *
 * \return absolutní (celková) najetá vzdálenost na levém motoru v mm
 */
float rkMotorsGetPositionLeft();

/**
 * \brief Vrátí absolutní najetou vzálenost na pravém motoru v mm
 *
 * \return absolutní (celková) najetá vzdálenost na pravém motoru v mm
 */
float rkMotorsGetPositionRight();

/**
 * \brief Vrátí absolutní najetou vzálenost na motoru podle ID
 *
 * \param id číslo motoru od 1 do 4 včetně
 * \return absolutní (celková) najetá vzdálenost na motoru v mm
 */
float rkMotorsGetPositionById(uint8_t id);

/**
 * \brief Nastaví absolutní počítadlo vzdálenosti na levém motoru na hodnotu v mm
 *
 * \param positionMm absolutní pozice, na kterou nastavit čítač
 */
void rkMotorsSetPositionLeft(float positionMm = 0.f);

/**
 * \brief Nastaví absolutní počítadlo vzdálenosti na pravém motoru na hodnotu v mm
 *
 * \param positionMm absolutní pozice, na kterou nastavit čítač
 */
void rkMotorsSetPositionRight(float positionMm = 0.f);

/**
 * \brief Nastaví absolutní počítadlo vzdálenosti na motoru podle ID na hodnotu v mm
 *
 * \param id číslo motoru od 1 do 4 včetně
 * \param positionMm absolutní pozice, na kterou nastavit čítač
 */
void rkMotorsSetPositionById(uint8_t id, float positionMm = 0.f);

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

/**
 * \brief Teplota desky v Robotce
 *
 * \return Naměřená teplota v °C
 */
int16_t rkTemperature();

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
 * \brief Seznam konstant označujícíh tlačítka, pro použítí v rkButtonIsPressed a dalších.
 */
enum rkButtonId {
    BTN_DOWN = rb::ButtonId::Down, //!< Tlačítko dolů
    BTN_UP = rb::ButtonId::Up, //!< Tlačítko nahoru
    BTN_LEFT = rb::ButtonId::Left, //!< Tlačítko doleva
    BTN_RIGHT = rb::ButtonId::Right, //!< Tlačítko doprava
    BTN_ON = rb::ButtonId::On, //!< Tlačítko ON/Ok (prostřední)
    BTN_OFF = rb::ButtonId::Off, //!< Tlačítko Off/Esc
};

#define EXTRA_BUTTON1 27 //!< Číslo pinu pro připojení extra koncového tlačítka, blíže okraji desky
#define EXTRA_BUTTON2 14 //!< Číslo pinu pro připojení extra koncového tlačítka, blíže do středu desky

/**
 * \brief Je teď stisknuto tlačítko?
 *
 * \param id ID tlačítka z enumu rkButtonId
 * \param waitForRelease pokud je stisknuto, počká před vrácením výsledku na jeho uvolnění (default: false)
 * \return Vrátí `true` pokud je tlačítko stisknuto.
 */
bool rkButtonIsPressed(rkButtonId id, bool waitForRelease = false);

/**
 * \brief Asynchroní zpracování události o stisku tlačítka.
 *
 * Ukázka použití:
 *
 * \code{cpp}
 * rkButtonOnChange([](rkButtonId id, bool pressed) -> bool {
 *     if (id == BTN_DOWN) {
 *         printf("Dolů: %d\n", pressed);
 *     } else if (id == BTN_UP) {
 *         printf("Nahoru: %d\n", pressed);
 *     } else if (id == BTN_LEFT) {
 *         printf("Doleva: %d\n", pressed);
 *     } else if (id == BTN_RIGHT) {
 *         printf("Doprava: %d\n", pressed);
 *     }
 *     return true;
 * });
 * \endcode
 *
 * \param callback funkce, která je zavolána pokud se stav kteréhokoliv tlačítka změní.
 *    parametry jsou ID tlačítka z rkButtonId a bool isPressed. Funkce musí vrátit
 *    true nebo false, podle toho, jestli má čekat na další události (true) nebo
 *    se má odstranit a další události už nepřijmat (false).
 */
void rkButtonOnChangeAsync(std::function<bool(rkButtonId, bool)> callback = nullptr);

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
/**
 * \defgroup line Sledování čáry (!! senzorická lišta !!)
 *
 * Funkce pro komunikaci se senzorickou lištou na čáru. Tyto funkce jsou
 * pro půlměsícovou desku s 8 senzory, ne pro malé, obdélníkové desky s 1 senzorem!
 *
 * @{
 */

/**
 * \brief Kalibrovat senzorickou lištu.
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
 * \brief Pozice čáry pod senzorickou lištou
 *
 * Tato funkce se pokouší najít černou čáru pod senzorickou lištou.
 *
 * \param white_line nastavte na true, pokud sledujete bílou čáru na černém podkladu. Výchozí: false
 * \param line_threshold_pct Jak velký rozdíl v procentech musí mezi hodnotami být, aby byla čára považována za nalezenou. Výchozí: 25%
 * \return Desetinná hodnota od -1 do +1. -1 znamená, že čára je úplně vlevo, 0 že je uprostřed a 1 že je úplně vpravo.
 *         Vrátí NaN, pokud nenalezne čáru - výsledek otestujte funkcí isnan() - `isnan(line_position)`
 */
float rkLineGetPosition(bool white_line = false, uint8_t line_threshold_pct = 25);

/**@}*/
/**
 * \defgroup irmodules Sledování čáry (!! IR moduly !!)
 *
 * Funkce pro čtení hodnot z obdélníkových IR modulů, které mají každý jeden senzor.
 *
 * @{
 */

/**
 * \brief Hodnota z levého IR senzoru
 * \return naměřená hodnota od 0(nejvíce bílá) do 4095(nejvíce černá)
 */
uint16_t rkIrLeft();

/**
 * \brief Hodnota z pravého IR senzoru
 * \return naměřená hodnota od 0(nejvíce bílá) do 4095(nejvíce černá)
 */
uint16_t rkIrRight();

/**@}*/
/**
 * \defgroup ultrasound Ultrazvuky
 *
 * Funkce pro meření vzálenosti pomocí ultrazvuků.
 * @{
 */

/**
 * \brief Změřit vzálenost (blokující)
 *
 * Změří vzálenost ultrazvukem a vrátí výsledek v milimetrech.
 * Může blokovat program až 30ms, podle toho, co ultrazvuk naměří.
 *
 * \param id Id ultrazvuku, od 1 do 4 včetně, podle popisků na desce.
 * \return Naměřená vzdálenost v mm, 0 pokud se měření nepodaří.
 */
uint32_t rkUltraMeasure(uint8_t id);

/**
 * \brief Změřit vzálenost (asynchroní)
 *
 * Přidá požadavek na měření vzdálenosti do fronty a okamžitě vrátí.
 * Jakmile je změřeno, je zavolána funkce předaná jako callback,
 * jako parametr bude mít naměřenou vzálenost v mm.
 *
 * Příklad použití:
 *
 * \code{.cpp}
 * rkUltraMeasureAsync(1, [](uint32_t distance_mm) -> bool {
 *    printf("Namereno: %u mm\n", distance_mm);
 *    return true;
 * });
 *
 * rkLedBlue(true); // provede se ještě před tím, než se stihne změřit vzdálenost.
 * \endcode
 *
 * \param id Id ultrazvuku, od 1 do 4 včetně, podle popisků na desce.
 * \param callback funkce, která bude zavolána po naměření. Do jejího parametru
 *     bude předána naměřená vzdálenost v mm. 0 znamená chybu v měření.
 *     Pokud chcete měřit jen jednou, vraťe z callbacku `false`, pokud opakovaně,
 *     pak vracejte `true`.
 */
void rkUltraMeasureAsync(uint8_t id, std::function<bool(uint32_t)> callback);

/**@}*/

/**@}*/
/**
 * \defgroup buzzer Bzučák
 *
 * Ovládání bzučáku na desce.
 * @{
 */

/**
 * \brief Zapnout/vypnout bzučák
 * \param on Zapnout(true) nebo vypnout(false)
 */
void rkBuzzerSet(bool on);

/**@}*/
/**
 * \defgroup smartleds Chytré LED
 *
 * Ovládání pásku chytrých LED.
 *
 * @{
 */

/**
 * \brief Nastavit barvu chytré led ve formátu RGB (Red, Green, Blue).
 *
 * Hodnoty barev můžete najít pomocí stránky https://htmlcolorcodes.com/
 *
 * \param idx Číslo LED, kterou chcete nastavit, od 0 do počtu led - 1 (tedy s jedním páskem od 0 do 7 včetně)
 * \param r hodnota červeného kanálu od 0 do 255
 * \param g hodnota zeleného kanálu od 0 do 255
 * \param b hodnota modrého kanálu od 0 do 255
 */
void rkSmartLedsRGB(uint16_t idx, uint8_t r, uint8_t g, uint8_t b);

/**
 * \brief Nastavit barvu chytré led ve formátu HSV (Hue, Saturation, Value).
 *
 * Hodnoty barev můžete najít pomocí stránky https://alloyui.com/examples/color-picker/hsv.html
 *
 * \param idx Číslo LED, kterou chcete nastavit, od 0 do počtu led - 1 (tedy s jedním páskem od 0 do 7 včetně)
 * \param h hodnota odstínu od 0 do 255
 * \param s hodnota sytosti barvy od 0 do 255
 * \param v hodnota jasu od 0 do 255
 */
void rkSmartLedsHSV(uint16_t idx, uint8_t h, uint8_t s, uint8_t v);

/**
 * \brief Nastavit barvu chytré led podle RGB HEX kódu 
 * \param idx Číslo LED, kterou chcete nastavit, od 0 do počtu led - 1 (tedy s jedním páskem od 0 do 7 včetně)
 * \param hexCode barevný hexadecimální RGB kód (0xFF0000)
 **/
void rkSmartLedsHEX(uint16_t idx, uint32_t hexCode);
SmartLed& rkSmartLedsGetController();

/**@}*/
/**
 * \defgroup stupidservos Serva (hloupá)
 *
 * Metody pro ovládání hloupých servo.
 * @{
 */

/**
 * \brief Nastaví pozici hloupého serva na zadaný úhel
 *
 * \param id číslo serva od 1 do 4 včetně, podle popisku na desce (SERVO1...SERVO4)
 * \param angleDegrees úhel natočení serva od -90 do 90 stupňů.
 */
void rkServosSetPosition(uint8_t id, float angleDegrees);

/**
 * \brief Vrátí naposledy nastavenou pozici hloupého serva.
 *
 * \param id číslo serva od 1 do 4 včetně, podle popisku na desce (SERVO1...SERVO4)
 * \return nastavený úhel serva ve stupňích, nebo NaN (viz std::isnan()) pokud je servo vypnuté.
 */
float rkServosGetPosition(uint8_t id);

/**
 * \brief Vypne hloupé servo
 *
 * Přestane se do serva posílát signál, ono tedy přestane držet svoji pozici.
 *
 * \param id číslo serva od 1 do 4 včetně, podle popisku na desce (SERVO1...SERVO4)
 */
void rkServosDisable(uint8_t id);

/**@}*/

#endif // LIBRB_H
