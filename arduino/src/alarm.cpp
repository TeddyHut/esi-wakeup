#include "alarm.h"

alarm::Alarm *alarm::alarm = nullptr;

void alarm::Alarm::update()
{
    // If the previously determined next alarm time is now and still in bed
    time_t now = time(nullptr);
    if (now == pm_nextalarm_time && pr_in_bed.get()) {
        if (!pm_alarm_triggered)
            pr_tipper.set(true);
        pm_alarm_triggered = true;
        return;
    }
    pm_alarm_triggered = false;

    // Update next alarm time
    tm_t alarm_timeofday_tm;
    tm_t now_tm; // this is superflouous but makes it easier to read
    localtime_r(&pr_settings.alarm_time, &alarm_timeofday_tm);
    localtime_r(&now, &now_tm);

    // Assume next alarm time is today...
    pm_nextalarm_tm = now_tm;
    if (!((now_tm.tm_hour < alarm_timeofday_tm.tm_hour) || 
        (now_tm.tm_hour == alarm_timeofday_tm.tm_hour && now_tm.tm_min < alarm_timeofday_tm.tm_min) ||
        (now_tm.tm_hour == alarm_timeofday_tm.tm_hour && now_tm.tm_min == alarm_timeofday_tm.tm_min && now_tm.tm_sec < alarm_timeofday_tm.tm_sec))) {
        // ...but add a day if it is tomorrow
        pm_nextalarm_tm.tm_mday += 1;
    }
    pm_nextalarm_tm.tm_hour = alarm_timeofday_tm.tm_hour;
    pm_nextalarm_tm.tm_min = alarm_timeofday_tm.tm_min;
    pm_nextalarm_tm.tm_sec = alarm_timeofday_tm.tm_sec;

    // Normalize the time structure in case of overflow in tm_mday
    // This also handles month and year overflow
    pm_nextalarm_time = mktime(&pm_nextalarm_tm);
}

tm_t const &alarm::Alarm::get_next_alarm_tm() const
{
    return pm_nextalarm_tm;
}

alarm::Alarm::Alarm(config::Settings const &settings, actuators::Tipper_t &tipper,
    libmodule::utility::Input<bool> const &is_in_bed) :
    pr_settings(settings), pr_tipper(tipper), pr_in_bed(is_in_bed),  pm_alarm_triggered(true)
{
}

time_t alarm::Alarm::get_next_alarm_time() const
{
    return pm_nextalarm_time;
}
