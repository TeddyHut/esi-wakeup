#include "appui.h"

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
    ui::NumberInputDecimal<uint16_t>::Config c;
    c.exit_left = false;
    c.exit_right = false;
    c.wrap = false;
    ui_spawn(new ui::NumberInputDecimal<uint16_t>(0, c, 0, 0));
}

void ui::Main::ui_on_childComplete()
{

}
