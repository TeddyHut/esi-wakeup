#pragma once

#include <libmodule.h>
#include <Servo.h>

#include "common.h"

namespace actuators {
    using Tipper_t = libmodule::utility::Output<bool>;

    struct ServoTipper : public Tipper_t
    {
        void set(bool const p) override;
        void update();
        ServoTipper(int const pin, config::Settings const &settings);
        Servo servo;
    private:
        config::Settings const &pr_settings;
        libmodule::Timer1k pm_timer;
        bool tipped : 1;
    } extern servotipper;
}
