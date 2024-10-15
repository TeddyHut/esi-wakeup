#pragma once

#include <stdint.h>
#include <time.h>

struct tm_t : public tm {
    tm_t();
    tm_t(tm const &tm);
    tm_t(time_t const time);
    operator time_t() const;
};
