#include "../common.h"
#include "../uxn.h"

#ifndef datetime_h
#define datetime_h

#define DATETIME_YEAR_PORT 0xc0
#define DATETIME_MONTH_PORT 0xc2
#define DATETIME_DAY_PORT 0xc3
#define DATETIME_HOUR_PORT 0xc4
#define DATETIME_MINUTE_PORT 0xc5
#define DATETIME_SECOND_PORT 0xc6
#define DATETIME_DOTW_PORT 0xc7
#define DATETIME_DOTY_PORT 0xc8
#define DATETIME_ISDST_PORT 0xca

Byte datetime_dei(Uxn *uxn, Byte addr);

#endif // datetime_h