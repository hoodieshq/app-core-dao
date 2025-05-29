#include "time_helper.h"
#include <stdio.h>

// Gregorian leap-year test
static int is_leap(int y)
{
    return (y % 4 == 0) && ((y % 100 != 0) || (y % 400 == 0));
}

static void timestamp_to_date(unsigned long ts, datetime_t *out)
{
    // 1. Split into day count and seconds-within-day
    int days   = ts / 86400;
    int secday = ts % 86400;
    // keep remainder non-negative
    if (secday < 0) {
        secday += 86400;
        --days;
    }

    // 2. Walk years forward / backward from 1970
    int year = 1970;
    // pre-1970 area
    while (days < 0) {
        --year;
        days += is_leap(year) ? 366 : 365;
    }
    // 1970+ area
    while (1) {
        int yd = is_leap(year) ? 366 : 365;
        if (days >= yd) {
            days -= yd;
            ++year;
        } else
            break;
    }

    // 3. Month & day within current year
    static const unsigned char mdays[2][12] = {
        {31,28,31,30,31,30,31,31,30,31,30,31},
        {31,29,31,30,31,30,31,31,30,31,30,31}
    };
    int leap = is_leap(year);
    int month = 0;
    while (days >= mdays[leap][month]) {
        days -= mdays[leap][month];
        ++month;
    }
    int day = (int)days + 1;
    ++month;

    // 4. Break seconds-within-day into H : M : S
    int hour   =  secday / 3600;
    int minute = (secday % 3600) / 60;
    int second =  secday % 60;

    // 5. Populate result
    out->year   = year;
    out->month  = month;
    out->day    = day;
    out->hour   = hour;
    out->minute = minute;
    out->second = second;
}

void timestamp_to_string(unsigned long timestamp, char str[static DATETIME_STR_LEN]) {
  datetime_t dt;
  timestamp_to_date(timestamp, &dt);
  snprintf(str, DATETIME_STR_LEN, "%04d-%02d-%02d %02d:%02d:%02d", dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
}
