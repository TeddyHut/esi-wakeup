#pragma once

#include <libmodule.h>
#include <time.h>

#include "alarm.h"

namespace ui
{
    using libmodule::ui::Screen;

    struct Common
    {
        libmodule::userio::IC_HD44780 &display;
        libmodule::ui::Dpad &dpad;
        tm &now_tm;
        const char *now_isotime;
        weight_t &measured_weight;
    };

    class FocusElement;

    class FocusManager {
        virtual bool is_child_visible(FocusElement const *child) const = 0;
        virtual bool is_child_focused(FocusElement const *child) const = 0;
        virtual void child_pull_focus(FocusElement const *child) = 0;
        virtual void child_release_focus(FocusElement const *child) = 0;

        friend FocusElement;
    };

    class FocusElement {
    public:
        // calls parent::is child visible
        bool is_visible() const;
        // calls parent:: is child focused
        bool is_focused() const;
        // calls parent::chiold_pull_focus
        void pull_focus();
        // calsl parent::child_release_focus
        void release_focus();
        // Called if visible changes (maybe a bad idea, getting more spaghetti likes)
        virtual void on_visible_changed(bool const visible); // this maybe should be protected/private with a friend
    protected:
        FocusElement(FocusManager *parent);
    private:
        FocusManager *parent;
    };

    class FocusScreen : public Screen<Common>, public FocusElement {
    protected:
        FocusScreen(FocusManager *parent);
    };

    /* List of screeen.
     * Navigation:
     *  up: Next screen
     *  down: Previous screen
     *  
     */
    /* Want: if list item spawns child then don't call list item ui_update for that itme. So should call ui_management_update of list items from screenlist. When an element is focused up/down no longer navigate until focus released.*/
    class ScreenList : public Screen<Common>, public FocusManager {
    public:
        // Todo: Add vector size as optional constructor parameter so it can be sized correctly on creation
        ScreenList(bool const delete_screens);
        libmodule::utility::Vector<FocusScreen *> m_screens;
    protected:
        void ui_update() override;
    private:
        struct {
            uint8_t index;
            bool focused;
        } pm_childstate{0, false};
        // Whether to delete elements of m_screens when finished
        bool const pm_delete_screens : 1;

        bool is_child_visible(FocusElement const *child) const override;
        bool is_child_focused(FocusElement const *child) const override;
        void child_pull_focus(FocusElement const *child) override;
        void child_release_focus(FocusElement const *child) override;
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
            bool blink_cursor : 1;
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

    /* Left/right: Navigate between fields
     * Up/down: Adjust digits
     * Select: Confirm time
     */

    class TimeEdit : public Screen<Common>
    {
    public:
        TimeEdit(uint8_t const ddbase, tm const &initial, bool const blink_cursor = true);

        tm m_time;
    protected:
        void ui_update() override;

    private:
        using NumberIn_t = NumberInputDecimal<uint8_t>;
        enum class State : uint8_t {
            None,
            Hours,
            Minutes,
            Seconds,
            Exit,
        } pm_state = State::None;

        NumberIn_t &activeinput;
        uint8_t pm_activeinput_storage[sizeof(NumberIn_t)];
        uint8_t const pm_ddbase;
        bool const pm_blink_cursor;
    };

    class Status : public FocusScreen
    {
    public:
        Status(FocusManager *focus_parent);
    protected:
void ui_update() override;
        void ui_on_childComplete() override;
        void on_visible_changed(bool const visible) override;
private:
        enum class State : uint8_t {
            Idle,
            EditEnabled,
            EditTime,
        } pm_state = State::Idle;
        
        void print_enabled();
        void print_tiptime();
    };

    class Clock : public FocusScreen
    {
    public:
        Clock(FocusManager *focus_parent);
    protected:
        void ui_update() override;
        void ui_on_childComplete() override;
        void on_visible_changed(bool const visible) override;

    
    private:
        enum class State : uint8_t {
            None,
            ShowClock,
            EditTime,
            EditDate
        } pm_state = State::None;
        // Used to check if need to re-write the time/date to the display
        time_t pm_previous_time;
    };

    class WeightThreshold : public FocusScreen {
        using WeightEdit_t = NumberInputDecimal<uint16_t>;
    public:
        WeightThreshold(FocusManager *focus_parent);
    protected:
        void ui_update() override;
        void ui_on_childComplete() override;
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
        ui_common->display << hd::display_power::cursor_on << (pm_config.blink_cursor ? hd::display_power::cursorblink_on : hd::display_power::cursorblink_off) << hd::display_power::display_on;
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
                next_value -= (pm_config.max - pm_config.min) + 1;
                //next_value = pm_config.min + (next_value - pm_config.min ) % (pm_config.max - pm_config.min);
            else next_value = pm_config.max;
        }
    }
    if (ui_common->dpad.down.get()) {
        next_value = m_value - step;
        // If we go under min (which includes integer underflow)
        if (next_value < pm_config.min || next_value > m_value) {
            if (pm_config.wrap)
                next_value += (pm_config.max - pm_config.min) + 1;
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
        ui_common->display << hd::display_power::cursor_on << (pm_config.blink_cursor ? hd::display_power::cursorblink_on : hd::display_power::cursorblink_off) << hd::display_power::display_on;
    }
    pm_runinit = false;
}

template <typename T>
template <typename U>
constexpr U ui::NumberInputDecimal<T>::powi(U const base, U const exp)
{
    U res = 1;
    for(U i = 0; i < exp; i++) res *= base;
    return res;
}

template <typename T>
template <typename U>
constexpr U ui::NumberInputDecimal<T>::log10i(U const p)
{
    //Cannot take log(0)
    if(p == 0) return 0;
    U i = 0;
    for(; p / powi<U>(10, i) > 0; i++);
    return i - 1;
}

template <typename T>
template <typename U>
constexpr uint8_t ui::NumberInputDecimal<T>::extract_digit10i(U const p, uint8_t const exp)
{
    return p % powi<U>(10, exp + 1) / powi<U>(10, exp);
}
