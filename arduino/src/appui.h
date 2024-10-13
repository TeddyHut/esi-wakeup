#pragma once

#include <libmodule.h>
#include <time.h>

namespace ui
{
    using libmodule::ui::Screen;

    struct Common
    {
        libmodule::userio::IC_HD44780 &display;
        libmodule::ui::Dpad &dpad;
    };

    /* Number input. Upon finishing, result will be stored in m_value.
     * config: The configuration for the number input.
     * default_value: The value to start from when entering the number input.
     * T: Expected to be an unsigned integer type
     * Navigation:
     *  up: add step
     *  down: subtract step
     *  left: ui_finish, m_confirmed set to false
     *  right: reset m_value to default
     *  centre: ui_finish, m_confirmed set to true
     */
    template <typename T = uint16_t>
    class NumberInputDecimal : public Screen<Common>
    {
    public:
        struct Config
        {
            // The minimum value
            T min = 0;
            // The maximum value
            T max = 100;
            // The position of the decimal point - zero means no decimal point, 1 means 1 digit to right of decimal point.
            uint8_t sig10 = 0;
            // The number of characters for the field including the decimal point
            uint8_t width = 3;
            char left_pad = '0';
            bool wrap : 1;
            bool exit_left : 1;
            bool exit_right : 1;
        };
        enum class ExitMode : uint8_t {
            None,
            Left,
            Right,
            Select,
        } m_exitmode = ExitMode::None;

        NumberInputDecimal(uint8_t const ddbase, Config const &config, T const default_value, uint8_t const default_cursor_sig = 0);

        T m_value;
    protected:
        void ui_update() override;

        uint8_t pm_cursor_sig;
        uint8_t const pm_max_sig;

        uint8_t const pm_ddbase;
        Config const pm_config;
        bool pm_runinit : 1;
    private:
        template <typename U>
        static constexpr U powi(U const base, U const exp);
        template <typename U>
        static constexpr U log10i(U const p);
        template <typename U>
        static constexpr uint8_t extract_digit10i(U const p, uint8_t const exp);
    };

    /* Top-level Screen
	 */
	class Main : public Screen<Common> {
	public:
		void update();
		Main(Common *const ui_common);
	private:
		void ui_update() override;
		void ui_on_childComplete() override;

		bool runinit = true;
	};
}

template <typename T>
ui::NumberInputDecimal<T>::NumberInputDecimal(uint8_t const ddbase, Config const &config, T const default_value, uint8_t const default_cursor_sig)
    : m_value(default_value),
      pm_cursor_sig(default_cursor_sig),
      pm_max_sig(log10i<T>(config.max)),
      pm_ddbase(ddbase),
      pm_config(config),
      pm_runinit(true)
{}

template <typename T>
void ui::NumberInputDecimal<T>::ui_update() {
    namespace hd = libmodule::userio::hd;

    auto restore_cursor_position = [this]() {
        ui_common->display << hd::instr::set_ddram_addr <<
        (pm_ddbase + pm_config.width - 1) -
            (pm_cursor_sig + (pm_config.sig10 > 0 && pm_cursor_sig >= pm_config.sig10));
    };

    if (pm_runinit) {
        ui_common->display << hd::instr::entry_mode_set << hd::entry_mode_set::cursormove_left << hd::entry_mode_set::displayshift_disable;
        restore_cursor_position();
        ui_common->display << hd::display_power::cursor_on << hd::display_power::cursorblink_on << hd::display_power::display_on;
    }

    // Move cursor if left/right are pressed
    auto next_cursor_sig = pm_cursor_sig;
    if (ui_common->dpad.left.get()) {
        next_cursor_sig = pm_cursor_sig + 1;
        if (next_cursor_sig > pm_max_sig || next_cursor_sig < pm_cursor_sig) {
            if (pm_config.exit_left) {
                m_exitmode = ExitMode::Left;
                ui_finish();
                return;
            }
            next_cursor_sig = pm_max_sig;
        }
    }
    if (ui_common->dpad.right.get()) {
        if (next_cursor_sig == 0) {
            if (pm_config.exit_right) {
                m_exitmode = ExitMode::Right;
                ui_finish();
                return;
            }
        }
        else next_cursor_sig = pm_cursor_sig - 1;
    }
    if (next_cursor_sig != pm_cursor_sig) {
        pm_cursor_sig = next_cursor_sig;
        restore_cursor_position();
    }

    // Exit if select is pressed
    if (ui_common->dpad.centre.get()) {
        m_exitmode = ExitMode::Select;
        ui_finish();
        return;
    }

    // Increment/decrement if up/down are pressed
    auto step = powi<T>(10, pm_cursor_sig);
    auto next_value = m_value;
    if (ui_common->dpad.up.get()) {
        next_value = m_value + step;
        // If we go past max (which includes integer overflow)
        if (next_value > pm_config.max || next_value < m_value) {
            if (pm_config.wrap)
                next_value -= (pm_config.max - pm_config.min);
                //next_value = pm_config.min + (next_value - pm_config.min ) % (pm_config.max - pm_config.min);
            else next_value = pm_config.max;
        }
    }
    if (ui_common->dpad.down.get()) {
        next_value = m_value - step;
        // If we go under min (which includes integer underflow)
        if (next_value < pm_config.min || next_value > m_value) {
            if (pm_config.wrap)
                next_value += (pm_config.max - pm_config.min);
            else next_value = pm_config.min;
        }
    }

    // If value has changed rewrite to display
    if (next_value != m_value || pm_runinit) {
        m_value = next_value;
        // Serial.println(m_value);

        ui_common->display << hd::display_power::cursor_off << hd::display_power::cursorblink_off << hd::display_power::display_on;
        ui_common->display << hd::instr::set_ddram_addr << pm_ddbase + pm_config.width - 1;
        for (uint8_t i = 0; i < pm_config.width; i++) {
            char c = pm_config.left_pad;
            if (i == 0 && next_value == 0) {
                c = '0';
            } else if (i > 0 && i == pm_config.sig10) {
                c = '.';
            } else if (next_value != 0) {
                c = '0' + next_value % 10;
                next_value /= 10;
            }
            ui_common->display << hd::instr::write << c;
        }
        restore_cursor_position();
        ui_common->display << hd::display_power::cursor_on << hd::display_power::cursorblink_on << hd::display_power::display_on;
    }
    pm_runinit = false;
}

template <typename U>
template <typename T>
constexpr T ui::NumberInputDecimal<U>::powi(T const base, T const exp)
{
    T res = 1;
    for(T i = 0; i < exp; i++) res *= base;
    return res;
}

template <typename U>
template <typename T>
constexpr T ui::NumberInputDecimal<U>::log10i(T const p)
{
    //Cannot take log(0)
    if(p == 0) return 0;
    T i = 0;
    for(; p / powi<T>(10, i) > 0; i++);
    return i - 1;
}

template <typename U>
template <typename T>
constexpr uint8_t ui::NumberInputDecimal<U>::extract_digit10i(T const p, uint8_t const exp)
{
    return p % powi<T>(10, exp + 1) / powi<T>(10, exp);
}
