#include "actuators.h"

#include <Arduino.h>

// void actuators::ServoTipper::set(bool const p)
// {
//     if (p) {
//         servo.write(pr_settings.tip_position);
//         pm_timer = 1000;
//         tipped = true;
//         pm_timer.start();
//     }
//     else {
//         pm_timer.reset();
//         tipped = false;
//         servo.write(pr_settings.hold_position);
//     }
// }

// void actuators::ServoTipper::update()
// {
//     if (tipped && pm_timer.finished)
//         set(false);
// }

// actuators::ServoTipper::ServoTipper(int const pin, config::Settings const &settings) : pr_settings(settings)
// {
//     servo.attach(pin, 350, 2400);
//     set(false);
// }

// actuators::ServoTipper actuators::servotipper(13, config::settings);

void actuators::PumpTipper::set(bool const tip)
{
    if (tip) {
        digitalWrite(pin, true);
        pm_timer = 60U * 1000U; // run pump for 1 minute
        tipped = true;
        pm_timer.start();
    }
    else {
        digitalWrite(pin, false);
        tipped = false;
        pm_timer.reset();
    }
}

void actuators::PumpTipper::update()
{
    if (tipped && pm_timer.finished)
        set(false);
}

actuators::PumpTipper::PumpTipper(int const pin) : pin(pin), tipped(false)
{
    pinMode(pin, OUTPUT);
    set(false);
}
