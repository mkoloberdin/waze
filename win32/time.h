#ifndef _TIME_H_
#define _TIME_H_

#include <windows.h>

struct tm {
	int tm_sec;		/* seconds after the minute - [0,59] */
	int tm_min;		/* minutes after the hour - [0,59] */
	int tm_hour;	/* hours since midnight - [0,23] */
	int tm_mday;	/* day of the month - [1,31] */
	int tm_mon;		/* months since January - [0,11] */
	int tm_year;	/* years since 1900 */
	int tm_wday;	/* days since Sunday - [0,6] */
	int tm_yday;	/* days since January 1 - [0,365] */
	int tm_isdst;	/* daylight savings time flag */
	};

time_t time(time_t*);

struct tm *gmtime(const time_t *);
struct tm *localtime(const time_t *);
time_t mktime(struct tm*);

#endif /* _TIME_H */

