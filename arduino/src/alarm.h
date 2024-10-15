#pragma once

#include <libmodule.h>
#include <time.h>

#include "common.h"
#include "actuators.h"
#include "sensors.h"

namespace alarm {
    class Alarm {
    public:
        void update();

        time_t get_next_alarm_time() const;
        tm_t const &get_next_alarm_tm() const;
        Alarm(config::Settings const &settings, actuators::Tipper_t &tipper,
            libmodule::utility::Input<bool> const &is_in_bed);
    private:
        config::Settings const &pr_settings; // r is for 'reference'
        actuators::Tipper_t &pr_tipper;
        tm_t pm_nextalarm_tm;
        time_t pm_nextalarm_time;
        libmodule::utility::Input<bool> const &pr_in_bed;
        bool pm_alarm_triggered : 1;
    } extern *alarm;
}
