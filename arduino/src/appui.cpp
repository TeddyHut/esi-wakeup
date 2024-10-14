#include "appui.h"

#include <stdio.h>
#include "sensors.h"

// ui::TimeEdit::TimeEdit(uint8_t const ddbase, tm const &initial) :
//     pm_ddbase(ddbase), m_time(initial), pm_runinit(true) {}

// void ui::TimeEdit::ui_update()
// {

//     if (pm_runinit) {
//         pm_runinit = false;
//     }
// }

void ui::Main::update()
{
    ui_management_update();
}

ui::Main::Main(ui::Common *const ui_common) : Screen(ui_common)
{
}

void ui::Main::ui_update()
{
    // ui::NumberInputDecimal<uint16_t>::Config c;
    // c.exit_left = false;
    // c.exit_right = false;
    // c.wrap = false;
    // ui_spawn(new ui::NumberInputDecimal<uint16_t>(0, c, 0, 0));
    // tm time;
    // time.tm_hour = 12;
    // time.tm_min = 30;
    // time.tm_sec = 30;
    // ui_spawn(new ui::TimeEdit(0, time, false));
    ui_spawn(new ui::Clock);
}

void ui::Main::ui_on_childComplete()
{

}

ui::TimeEdit::TimeEdit(uint8_t const ddbase, tm const &initial, bool const blink_cursor) :
    m_time(initial),
    activeinput(*reinterpret_cast<NumberIn_t *>(pm_activeinput_storage)),
    pm_ddbase(ddbase),
    pm_blink_cursor(blink_cursor)
{}

void ui::TimeEdit::ui_update()
{
    using ExitMode = NumberIn_t::ExitMode;
    auto nextstate = pm_state;
    switch (pm_state) {
    case State::None:
        nextstate = State::Hours;
        break;
    case State::Hours:
        switch(activeinput.m_exitmode) {
        case ExitMode::Right:
            nextstate = State::Minutes;
            break;
        case ExitMode::Select:
            [[fallthrough]];
        default:
            nextstate = State::Exit;
            break;
        }
        break;
    case State::Minutes:
        switch(activeinput.m_exitmode) {
        case ExitMode::Left:
            nextstate = State::Hours;
            break;
        case ExitMode::Right:
            nextstate = State::Seconds;
            break;
        case ExitMode::Select:
            [[fallthrough]];
        default:
            nextstate = State::Exit;
            break;
        }
        break;
    case State::Seconds:
        switch(activeinput.m_exitmode) {
        case ExitMode::Left:
            nextstate = State::Minutes;
            break;
        case ExitMode::Select:
            [[fallthrough]];
        default:
            nextstate = State::Exit;
            break;
        }
        break;
    default: break;
    }

    if (nextstate != pm_state) {
        namespace hd = libmodule::userio::hd;
        auto clean_activeinput = [this]() {
            if (pm_state != State::None && pm_state != State::Exit)
                activeinput.~NumberIn_t();
        };

        NumberIn_t::Config c;
        c.min = 0;
        c.sig10 = 0;
        c.width = 2;
        c.left_pad = '0';
        c.wrap = true;
        c.blink_cursor = pm_blink_cursor;

        // Initialise Update time object with value from edit
        switch (pm_state) {
        case State::None:
            // Print the time for all three fields
            char buf[sizeof("2013-03-23 01:03:52")];
            isotime_r(&m_time, buf);
            ui_common->display << hd::instr::set_ddram_addr << pm_ddbase << static_cast<const char *>(buf + 11);
            break;
        case State::Hours:
            m_time.tm_hour = activeinput.m_value;
            break;
        case State::Minutes:
            m_time.tm_min = activeinput.m_value;
            break;
        case State::Seconds:
            m_time.tm_sec = activeinput.m_value;
            break;
        default:
            break;
        }

        // Allocate new edit or signal finished
        switch (nextstate) {
        case State::Hours:
            c.max = 23;
            c.exit_left = false;
            c.exit_right = true;
            clean_activeinput();
            ui_spawn(new (pm_activeinput_storage) NumberIn_t(pm_ddbase, c, m_time.tm_hour, 0), false);
            break;
        case State::Minutes:
            c.max = 59;
            c.exit_left = true;
            c.exit_right = true;
            clean_activeinput();
            // Cursor starts on left if moving from hours
            ui_spawn(new (pm_activeinput_storage) NumberIn_t(pm_ddbase + 3, c, m_time.tm_min, pm_state == State::Hours), false);
            break;
        case State::Seconds:
            c.max = 59;
            c.exit_left = true;
            c.exit_right = false;
            clean_activeinput();
            ui_spawn(new (pm_activeinput_storage) NumberIn_t(pm_ddbase + 6, c, m_time.tm_sec, 1), false);
            break;
        case State::Exit:
            clean_activeinput();
            ui_common->display << hd::instr::entry_mode_set << hd::entry_mode_set::cursormove_right << hd::entry_mode_set::displayshift_disable;
            ui_finish();
            break;
        default: break;
        }
    }
    pm_state = nextstate;
}

void ui::Clock::ui_update()
{
    namespace hd = libmodule::userio::hd;
    auto nextstate = pm_state;
    switch (pm_state) {
    case State::None:
        nextstate = State::ShowClock;
        break;
    case State::ShowClock:
        if (ui_common->dpad.centre.get())
            nextstate = State::EditTime;
        break;
    case State::EditTime:
        nextstate = State::ShowClock;
        break;
    default: break;
    }
    

    if (pm_state == State::None) {
        ui_common->display << hd::instr::display_power << hd::display_power::cursor_off << hd::display_power::cursorblink_off << hd::display_power::display_on;
        ui_common->display << hd::instr::entry_mode_set << hd::entry_mode_set::cursormove_right << hd::entry_mode_set::displayshift_disable;
    }
    if (pm_state == State::None || pm_previous_time != time(nullptr)) {
        pm_previous_time = time(nullptr);
        char buf[16 + 16 + 2];
        snprintf_P(buf, sizeof buf, PSTR("Time:   %.8s\nDate: %.10s"), ui_common->now_isotime + 11, ui_common->now_isotime);
        ui_common->display << hd::instr::return_home << buf;
    }

    if (nextstate != pm_state) {
        switch (nextstate) {
        case State::EditTime:
            ui_spawn(new TimeEdit(8, ui_common->now_tm, true));
            break;
        default: break;
        }
    }

    pm_state = nextstate;
}

void ui::Clock::ui_on_childComplete()
{
    switch (pm_state) {
    case State::EditTime: {
        auto &child_edit = *static_cast<ui::TimeEdit *>(ui_child);
        rtc::write_time(mktime(&child_edit.m_time) + UNIX_OFFSET);
        break;
    }
    default:
        break;
    }
}

