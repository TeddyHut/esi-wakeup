#include "sensors.h"
#include <Arduino.h>

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

RtcDS1302<ThreeWire> *rtc::ds1302 = nullptr;

void rtc::update_from_rtc()
{
    static time_t previous_time = 0;
    auto now_datetime = ds1302->GetDateTime();
    auto now_unix = ds1302->GetDateTime().Unix32Time();
    if (!now_datetime.IsValid())
        libmodule::hw::panic(/*"DateTime.IsValid() false"*/);
    if (now_unix != previous_time)
        set_system_time(now_unix - UNIX_OFFSET);
}

void rtc::write_time(time_t const time) {
    RtcDateTime datetime;
    datetime.InitWithUnix32Time(time);
    ds1302->SetDateTime(datetime);
}
