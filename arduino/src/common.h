#pragma once

#include <stdint.h>
#include <time.h>

using servo_position_t = uint16_t;
using weight_t = float;
using timeofday_t = uint32_t; // number of seconds past midnight

struct tm_t : public tm {
    tm_t();
    tm_t(tm const &tm);
    tm_t(time_t const time);
    operator time_t() const;
};

namespace config {
    struct Settings {
        void save() const;
        void load();

        uint8_t write_indicator = 0x5e;
        
        servo_position_t hold_position = 0, tip_position = 75;
        
        weight_t weight_threshold = 50;
        int32_t loadcell_divider;


        timeofday_t alarm_time = 60UL * 60UL * 6UL; // 6:00 am
        bool alarm_enabled = false;
    } extern settings;
}
