#include <Arduino.h>

#include <libmodule.h>
#include <libarduino_m328.h>

struct HD44780Bidirectional : public libmodule::utility::Bidirectional<uint8_t> {
    static constexpr auto N = 4;
    uint8_t get() const override;
    void set(uint8_t const state) override;
    HD44780Bidirectional(uint8_t const pins[N]);
private:
    uint8_t const pm_pins[N];
};

libmodule::userio::IC_HD44780 display;

void setup() {
    static uint8_t pins[HD44780Bidirectional::N] = {4, 5, 6, 7};
    static libarduino_m328::DigitalOut en(9), rw(12), rs(8);
    static HD44780Bidirectional data_pins(pins);
    display.pin.data = &data_pins;
    display.pin.en = &en;
    display.pin.rw = nullptr;//&rw;
    display.pin.rs = &rs;

    using namespace libmodule::userio::hd;
    _delay_ms(50);
	display << instr::init_4bit;
    display << instr::function_set << function_set::datalength_4 << function_set::font_5x8 << function_set::lines_2;

    display << instr::clear_display;
    display << instr::display_power << display_power::cursor_on << display_power::cursorblink_on << display_power::display_on;

    display << instr::clear_display;
    display << "    SEM LCD\nConnect to PCB";

    pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
    static int count = 0;
    using namespace libmodule::userio::hd;
    char buf[17];
    sprintf(buf, "%d", count++);
    display << instr::set_ddram_addr << static_cast<uint8_t>(0x0);
    display << buf; 
    digitalWrite(LED_BUILTIN, 0);
    delay(1000);
    digitalWrite(LED_BUILTIN, 1);
    delay(1000);
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
HD44780Bidirectional::HD44780Bidirectional(uint8_t const pins[N]) : pm_pins{pins[0], pins[1], pins[2], pins[3]}
{
}
