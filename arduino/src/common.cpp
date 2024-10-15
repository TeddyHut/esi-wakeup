#include "common.h"

#include <EEPROM.h>

tm_t::tm_t()
{
}

tm_t::tm_t(tm const &tm_v)
{
    // Since tm is a C struct cannot do : tm(tm_v) (I think)
    *static_cast<tm*>(this) = tm_v;
}

tm_t::tm_t(time_t const time)
{
    localtime_r(&time, this);
}

tm_t::operator time_t() const
{
    return mk_gmtime(this) + UNIX_OFFSET;
}

config::Settings config::settings;

void config::Settings::save() const
{
    EEPROM.put(0, *this);
}

void config::Settings::load()
{
    Settings candidate_settings;
    EEPROM.get(0, candidate_settings);
    if (candidate_settings.write_indicator == 0x5e)
        *this = candidate_settings;
}
