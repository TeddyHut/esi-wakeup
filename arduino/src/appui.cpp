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
    //ui_spawn(new ui::Clock);
    auto screenlist = new ui::ScreenList(true);
    auto status = new ui::Status(screenlist);
    auto clock = new ui::Clock(screenlist);
    screenlist->m_screens.resize(2);
    screenlist->m_screens[0] = status;
    screenlist->m_screens[1] = clock;
    ui_spawn(screenlist);
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

ui::Clock::Clock(FocusManager *focus_parent) : FocusScreen(focus_parent)
{
}

void ui::Clock::ui_update()
{
    if (!is_visible())
        return;
    namespace hd = libmodule::userio::hd;
    auto nextstate = pm_state;
    switch (pm_state) {
    case State::None:
        nextstate = State::ShowClock;
        break;
    case State::ShowClock:
        if (ui_common->dpad.centre.get()) {
            nextstate = State::EditTime;
        }
        break;
    case State::EditTime:
        nextstate = State::ShowClock;
        break;
    default: break;
    }
    

    if (pm_state == State::None) {
        pm_state = nextstate;
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
            pull_focus();
            ui_spawn(new TimeEdit(8, ui_common->now_tm, true));
            break;
        case State::ShowClock:
            release_focus();
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

void ui::Clock::on_visible_changed(bool const visible)
{
    if (!visible)
        pm_state = State::None;
}

ui::ScreenList::ScreenList(bool const delete_screens) : pm_delete_screens(delete_screens)
{
}

void ui::ScreenList::ui_update()
{
    for (uint8_t i = 0; i < m_screens.size(); i++) {
        m_screens[i]->ui_common = ui_common;
        m_screens[i]->ui_management_update();
    }
    
    if (pm_childstate.focused)
        return;
    
    if (ui_common->dpad.left.get()) {
        if(pm_delete_screens) {
            for (uint8_t i = 0; i < m_screens.size(); i++)
                delete m_screens[i];
        }
        ui_finish();
    }
    auto old_child_index = pm_childstate.index;
    if (ui_common->dpad.up.get()) {
        pm_childstate.index = (pm_childstate.index == 0 ?  m_screens.size() : pm_childstate.index) - 1;
    }
    if (ui_common->dpad.down.get()) {
        if(++pm_childstate.index == m_screens.size())
            pm_childstate.index = 0;
    }
    if (old_child_index != pm_childstate.index) {
        m_screens[old_child_index]->on_visible_changed(false);
        m_screens[pm_childstate.index]->on_visible_changed(true);
    }
}

bool ui::ScreenList::is_child_visible(FocusElement const *child) const
{
    return child == m_screens[pm_childstate.index];
}

bool ui::ScreenList::is_child_focused(FocusElement const *child) const
{
    return pm_childstate.focused && is_child_visible(child);
}

void ui::ScreenList::child_pull_focus(FocusElement const *child)
{
    // For the moment only allow pulling focus if currently visible
    if (is_child_visible(child))
        pm_childstate.focused = true;
}

void ui::ScreenList::child_release_focus(FocusElement const *child)
{
    if (is_child_visible(child))
        pm_childstate.focused = false;
}

bool ui::FocusElement::is_visible() const
{
    return parent->is_child_visible(this);
}

bool ui::FocusElement::is_focused() const
{
    return parent->is_child_focused(this);
}

void ui::FocusElement::pull_focus()
{
    parent->child_pull_focus(this);
}

void ui::FocusElement::release_focus()
{
    parent->child_release_focus(this);
}

void ui::FocusElement::on_visible_changed([[maybe_unused]] bool const visible)
{
}

ui::FocusElement::FocusElement(FocusManager *parent) : parent(parent)
{
}

ui::FocusScreen::FocusScreen(FocusManager *parent) : FocusElement(parent)
{
}

ui::Status::Status(FocusManager *focus_parent) : FocusScreen(focus_parent)
{
}

void ui::Status::on_visible_changed(bool const visible)
{
    if (visible) {
        namespace hd = libmodule::userio::hd;
        ui_common->display << hd::instr::display_power << hd::display_power::cursor_off << hd::display_power::cursorblink_off << hd::display_power::display_on;
        ui_common->display << hd::instr::entry_mode_set << hd::entry_mode_set::cursormove_right << hd::entry_mode_set::displayshift_disable;
        ui_common->display << hd::instr::return_home << "Enabled         \nTip at: 00:00:00";
    }
}
