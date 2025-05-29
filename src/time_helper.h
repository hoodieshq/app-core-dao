#pragma once

typedef struct {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
} datetime_t;

#define DATETIME_STR_LEN 20

void timestamp_to_string(unsigned long timestamp, char str[static DATETIME_STR_LEN]);
