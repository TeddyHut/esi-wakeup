#include "common.h"

#include <EEPROM.h>
#include <stdio.h>
#include <avr/pgmspace.h>
#include <Arduino.h>

tm_t::tm_t()
{
}

tm_t::tm_t(tm const &tm_v)
{
    // Since tm is a C struct cannot do : tm(tm_v) (I think)
    *static_cast<tm*>(this) = tm_v;
}

tm_t::tm_t(time_t const time)
{
    localtime_r(&time, this);
}

tm_t::operator time_t() const
{
    return mk_gmtime(this); //+ UNIX_OFFSET;
}

void tm_t::print() const
{
    char buf[sizeof("hh:mm:ss")];
    snprintf_P(buf, sizeof buf, PSTR("%02hhd:%02hhd:%02hhd"),
        tm_hour, tm_min, tm_sec);
    Serial.print(buf);
}

config::Settings config::settings;

void config::Settings::save() const
{
    EEPROM.put(0, *this);
}

void config::Settings::load()
{
    Settings candidate_settings;
    EEPROM.get(0, candidate_settings);
    if (candidate_settings.write_indicator == 0x5e)
        *this = candidate_settings;
    if (weight_threshold > 200 || weight_threshold < 0) {
        weight_threshold = 50;
        save();
    } 
}
