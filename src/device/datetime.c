#include "datetime.h"
#include <stdlib.h>
#include <time.h>

Byte datetime_dei(Uxn *uxn, Byte addr) {
  time_t t = time(NULL);
  struct tm *tm = localtime(&t);

  if (!tm)
    return 0;

  switch (addr) {
  case DATETIME_YEAR_PORT:
    return tm->tm_year >> 8;
  case DATETIME_YEAR_PORT + 1:
    return tm->tm_year & 0xff;
  case DATETIME_MONTH_PORT:
    return tm->tm_mon;
  case DATETIME_DAY_PORT:
    return tm->tm_mday;
  case DATETIME_HOUR_PORT:
    return tm->tm_hour;
  case DATETIME_MINUTE_PORT:
    return tm->tm_min;
  case DATETIME_SECOND_PORT:
    return tm->tm_sec;
  case DATETIME_DOTW_PORT:
    return tm->tm_wday;
  case DATETIME_DOTY_PORT:
    return tm->tm_yday >> 8;
  case DATETIME_DOTY_PORT + 1:
    return tm->tm_yday & 0xff;
  case DATETIME_ISDST_PORT:
    return tm->tm_isdst;
  default:
    return Uxn_dev_read(uxn, addr);
  }

  free(tm);
}