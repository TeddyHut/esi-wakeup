#include <Arduino.h>
#include <avr/sleep.h>
#include <libmodule.h>
#include <libarduino_m328.h>

#include "appui.h"

// Explicitly instantiate the timer InstanceList to prevent multiple instantiations
// for different translation units.
template class libmodule::utility::InstanceList<libmodule::time::TimerBase<1000>>;

struct HD44780Bidirectional : public libmodule::utility::Bidirectional<uint8_t> {
    static constexpr auto N = 4;
    uint8_t get() const override;
    void set(uint8_t const state) override;
    HD44780Bidirectional(uint8_t const pins[N]);
private:
    uint8_t const pm_pins[N];
};

template <typename T>
struct CycleSensor : public libmodule::utility::Input<T> {
    //Will return the value of the sensor for this cycle
    T get() const override { return cycle_value; }
    //Will read the sensor value from hardware and store it for the cycle
    void cycle_read() { cycle_value = get_sensor_value(); }
protected:
    //Should return the value of the sensor as found in hardware
    virtual T get_sensor_value() = 0;
    T cycle_value;
};

using Sensor_t = CycleSensor<float>;
using DigiSensor_t = CycleSensor<bool>;

struct DFDpad {
    enum Button : uint8_t {
        None, Left, Right, Up, Down, Centre
    };
    struct AnalogToDpadButton : public CycleSensor<Button> {
        int pin;
    private:
        Button get_sensor_value() override;
    } atod;
    struct DirectionOut : public libmodule::utility::Input<bool> {
        bool get() const override;
        DirectionOut(AnalogToDpadButton &button, Button const dir);
    private:
        AnalogToDpadButton &button;
        Button dir;
    } left{atod, Left}, right{atod, Right},
      up{atod, Up},     down{atod, Down},
      centre{atod, Centre};
};

void loop() {}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, false);
    pinMode(10, OUTPUT);
    digitalWrite(10, true);

    // Setup Dpad
    DFDpad dfdpad;
    libmodule::ui::Dpad dpad;
    libmodule::userio::RapidInput3L1k::Level rapidinput_level_0 = {500, 250};
	libmodule::userio::RapidInput3L1k::Level rapidinput_level_1 = {1500, 100};
	libmodule::userio::RapidInput3L1k::Level rapidinput_level_2 = {4000, 35};
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
    display << "Line 1\nLine 2";

    // Setup UI
    ui::Common ui_common{display, dpad};
    ui::Main ui_main(&ui_common);

    // Setup main loop timer
    libmodule::Timer1k main_timer;
	main_timer.finished = true;
	main_timer.start();

    // Start timers
    libmodule::time::start_timer_daemons<1000>();

    Serial.begin(9600);

    while (true) {
        if (!main_timer) {
            sleep_cpu();
            continue;
        }
        main_timer.reset();
        main_timer = 1000 / 60;
        main_timer.start();

        dfdpad.atod.cycle_read();
        dpad.left.update();
        dpad.right.update();
        dpad.up.update();
        dpad.down.update();
        dpad.centre.update();

        ui_main.update();

        using namespace libmodule::userio::hd;

        if (serialEventRun) serialEventRun();
    }
}

uint8_t HD44780Bidirectional::get() const
{
    uint8_t rtrn = 0;
    for (uint8_t i = 0; i < N; i++) {
        pinMode(pm_pins[i], INPUT);
        rtrn |= digitalRead(pm_pins[i]) << i;
    }
    return rtrn;
}

void HD44780Bidirectional::set(uint8_t const state)
{
    for (uint8_t i = 0; i < N; i++) {
        pinMode(pm_pins[i], OUTPUT);
        digitalWrite(pm_pins[i], state & (1 << i));
    }
}
HD44780Bidirectional::HD44780Bidirectional(uint8_t const pins[N]) :
    pm_pins{pins[0], pins[1], pins[2], pins[3]} {}

DFDpad::Button DFDpad::AnalogToDpadButton::get_sensor_value()
{
    auto a = analogRead(pin);
    if (a > 1000) return None;
    if (a < 50)   return Right; 
    if (a < 250)  return Up; 
    if (a < 450)  return Down; 
    if (a < 650)  return Left; 
    return Centre;
}

bool DFDpad::DirectionOut::get() const
{
    return button.get() == dir;
}

DFDpad::DirectionOut::DirectionOut(AnalogToDpadButton &button, Button const dir) : button(button), dir(dir) {}
