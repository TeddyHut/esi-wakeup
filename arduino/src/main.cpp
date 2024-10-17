#include <Arduino.h>
#include <avr/sleep.h>
#include <libmodule.h>
#include <libarduino_m328.h>
#include <RtcDS1302.h>

#include "appui.h"
#include "sensors.h"
#include "common.h"

namespace {
    libmodule::userio::IC_HD44780 *display = nullptr;
}

void libmodule::hw::panic(const char *message)
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, true);
    Serial.println(message);
    namespace hd = libmodule::userio::hd;
    if (::display != nullptr)
        *::display << hd::instr::clear_display
                   << hd::instr::entry_mode_set
                      << hd::entry_mode_set::cursormove_left
                      << hd::entry_mode_set::displayshift_enable
                   << message;
    // asm("BREAK");
    while(true) {
        digitalWrite(10, true);
        delay(500);
        digitalWrite(10, false);
        delay(500);
    }
}

// Explicitly instantiate the timer InstanceList to prevent multiple instantiations
// for different translation units.
template class libmodule::utility::InstanceList<libmodule::time::TimerBase<1000>>;

void loop() {}

void setup() {
    Serial.begin(57600);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, false);
    pinMode(10, OUTPUT);
    digitalWrite(10, true);

    // Read previous settings from EEPROM
    config::settings.load();

    // Setup Dpad
    DFDpad dfdpad;
    libmodule::ui::Dpad dpad;
    libmodule::userio::RapidInput3L1k::Level rapidinput_level_0 = {500, 250};
	libmodule::userio::RapidInput3L1k::Level rapidinput_level_1 = {1500, 100};
	libmodule::userio::RapidInput3L1k::Level rapidinput_level_2 = {UINT16_MAX, 50};
	dpad.set_rapidInputLevel(0, rapidinput_level_0);
	dpad.set_rapidInputLevel(1, rapidinput_level_1);
	dpad.set_rapidInputLevel(2, rapidinput_level_2);

    dfdpad.atod.pin = A0;
    dpad.left.set_input(&dfdpad.left);
    dpad.right.set_input(&dfdpad.right);
    dpad.up.set_input(&dfdpad.up);
    dpad.down.set_input(&dfdpad.down);
    dpad.centre.set_input(&dfdpad.centre);

    // Setup display
    libmodule::userio::IC_HD44780 display;
    ::display = &display;
    uint8_t pins[HD44780Bidirectional::N] = {4, 5, 6, 7};
    libarduino_m328::DigitalOut en(9), rw(12), rs(8);
    HD44780Bidirectional data_pins(pins);
    display.pin.data = &data_pins;
    display.pin.en = &en;
    display.pin.rw = nullptr; //&rw;
    display.pin.rs = &rs;

    using namespace libmodule::userio::hd;
    _delay_ms(50);
	display << instr::init_4bit;
    display << instr::function_set << function_set::datalength_4 << function_set::font_5x8 << function_set::lines_2;

    display << instr::clear_display;
    display << instr::display_power << display_power::cursor_off << display_power::cursorblink_off << display_power::display_on;

    display << instr::clear_display;

    // Setup pump
    actuators::PumpTipper tipper(A4);
    
    // Setup RTC
    ThreeWire rtc_wire(A2, A1, A3);
    RtcDS1302 rtc(rtc_wire);
    rtc::setup_rtc(rtc);

    // Setup weight sensor
    // BedPresenceWeight presence(A4, A5);
    weight_t most_recent_weight = 0;
    struct BedPresence : public libmodule::utility::Input<bool>
    {
        weight_t &most_recent_weight;
        BedPresence(weight_t &weight) : most_recent_weight(weight) {}
        bool get() const override {
            // return true;
            return most_recent_weight > config::settings.weight_threshold;
        }
    } bed_presence{most_recent_weight};

    HX711 loadcell;
    loadcell.begin(A4, A5); // DOUT, SCK
    if (!loadcell.wait_ready_timeout(1000))
        libmodule::hw::panic();
        // libmodule::hw::panic("No HX711");
    loadcell.set_scale(config::settings.loadcell_divider);
    loadcell.set_scale(-8220.0f);
    loadcell.tare();

    // Setup UI
    tm now_tm;
    char now_isotime[sizeof("2013-03-23 01:03:52")];
    ui::Common ui_common{display, dpad, now_tm, now_isotime, most_recent_weight};
    ui::Main ui_main(&ui_common);

    // Setup alarm
    alarm::Alarm alarm(config::settings, tipper, bed_presence);
    alarm::alarm = &alarm;

    // Setup main loop timer
    libmodule::Timer1k main_timer;
	main_timer.finished = true;
	main_timer.start();

    time_t previous_rtc_time = 0;
    // Start timers
    libmodule::time::start_timer_daemons<1000>();

    // tipper.set(true);
    static constexpr auto refresh_rate_hz = 60;
    while (true) {
        if (!main_timer) {
            sleep_cpu();
            continue;
        }
        main_timer.reset();
        main_timer = 1000 / refresh_rate_hz;
        main_timer.start();

        // Read from load cells when a result is ready
        if (loadcell.wait_ready_timeout(1000 / (refresh_rate_hz * 2))) {
            Serial.println(loadcell.read());
            most_recent_weight = loadcell.get_units(1);
        }

        tipper.update();

        dfdpad.atod.cycle_read();
        dpad.left.update();
        dpad.right.update();
        dpad.up.update();
        dpad.down.update();
        dpad.centre.update();

        rtc::update_from_rtc();
        auto now = time(nullptr);
        localtime_r(&now, &now_tm);
        isotime_r(&now_tm, now_isotime);
        
        // Update alarm once every second
        if (now != previous_rtc_time) {
            alarm.update(now);
            previous_rtc_time = now;
        }

        ui_main.update();
        Serial.flush();

        using namespace libmodule::userio::hd;

        if (serialEventRun) serialEventRun();
    }
}
