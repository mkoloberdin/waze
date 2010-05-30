/* roadmap_sunrise.c - calculate sunrise/sunset time
 *
 * License:
 *
 *   Copyright 2009 Kalaev Maxim
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   RoadMap is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with RoadMap; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * SYNOPSYS:
 *
 * Based on algorithm description from:
 *
 *	Almanac for Computers, 1990
 *	published by Nautical Almanac Office
 *	United States Naval Observatory
 *	Washington, DC 20392
 *
 *  (http://williams.best.vwh.net/sunrise_sunset_algorithm.htm)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "websvc_trans/mkgmtime.h"
#include "roadmap_sunrise.h"
#include "websvc_trans/mkgmtime.h"
#define M_PI 3.1417

#define SIN(x)  (sin((M_PI/180)*x))
#define COS(x)  (cos((M_PI/180)*x))
#define TAN(x)  (tan((M_PI/180)*x))
#define ATAN(x) ((180/M_PI)*atan(x))
#define ACOS(x) ((180/M_PI)*acos(x))
#define ASIN(x) ((180/M_PI)*asin(x))

typedef enum {
	ESUNRISE,
	ESUNSET
}ERiseSet;


/* Sun rise-set calculation algorithm.
 * Algorithm description:   http://williams.best.vwh.net/sunrise_sunset_algorithm.htm
 *
 *	Almanac for Computers, 1990
 *	published by Nautical Almanac Office
 *	United States Naval Observatory
 *	Washington, DC 20392
 *
 * Parameters:
 *  yd					- day of the year (1..365)
 *  latitude, longitude	- Sample: 32.27, 34.85 for Netania israel
 *  riseset				- ESUNRISE or ESUNSET
 *
 * Returns: UTC hour of the event (a real number).
*/
double sunriseset( int yd, double latitude, double longitude, ERiseSet riseset )
{
	// 96 degrees    - Calculate Civil twilight time. Used as indication if
	//   it is (usually) bright enough for outdoor activities without additional lighting.
	// 90 degrees 5' - Calculate true Sunrise/Sunset time. Used to check if
	//   the Sun itself is visible above the horizont in ideal conditions.
	const double zenith = 96;

	double sinDec, cosDec, cosH;
	double H, T, UT;

	// Rise / Set
	int op = (riseset==ESUNRISE ? 1:-1);

	// Convert the longitude to hour value and calculate an approximate time
	double lngHour = longitude / 15;

	// if rising time is desired:
	double t = yd + ((12 - (6*op) - lngHour) / 24);

	// Calculate the Sun's mean anomaly
	double M = (0.9856 * t) - 3.289;

	// Calculate the Sun's true longitude
	double L = M + (1.916 * SIN(M)) + (0.020 * SIN(2 * M)) + 282.634;

	// Calculate the Sun's right ascension
	double RA = ATAN(0.91764 * TAN(L));

	// Right ascension value needs to be in the same quadrant as L
	double Lquadrant  = (floor( L/90)) * 90;
	double RAquadrant = (floor(RA/90)) * 90;

	RA = RA + (Lquadrant - RAquadrant);

	// Right ascension value needs to be converted into hours
	RA = RA / 15;

	// Calculate the Sun's declination
	sinDec = 0.39782 * SIN(L);
	cosDec = COS(ASIN(sinDec));

	// Calculate the Sun's local hour angle
	cosH = (COS(zenith) - (sinDec * SIN(latitude))) / (cosDec * COS(latitude));
	if( cosH < -1 || cosH > 1 )
		// The sun never rises or sets on this location (on the specified date)
		return -1;

	// Finish calculating H and convert into hours
	H = 180 + (180 - ACOS(cosH))*op;
	H = H / 15;

	// Calculate local mean time of rising/setting
	T = H + RA - (0.06571 * t) - 6.622;

	// Adjust back to UTC
	UT = T - lngHour;

	// UT potentially needs to be adjusted into the range [0,24) by adding/subtracting 24
	if (UT < 0)   UT += 24;
	if (UT >= 24) UT -= 24;

	return UT;
}

time_t roadmap_sunriseset( const RoadMapGpsPosition *position, const struct tm* gmt_now, ERiseSet riseset )
{
	// Function returns UTC time of the next nearest event with respect to the time
	//  passed in gmt_now.

	// Local copy of the current UTC time..
	struct tm tm_now = *gmt_now;

	// Get rise-set time (UTC).
	double UT = sunriseset( tm_now.tm_yday,
							(double)position->latitude / 1e6,
							(double)position->longitude / 1e6,
							riseset );

	// Convert UTC hours (real) to integer hours/minutes
	int hour = (int)UT;
	int min  = (int)((UT - hour)*60);
	if( min == 60 ) {
		min = 0;
		hour++;
	}

	// Calculate time of the next event:
	if( hour < tm_now.tm_hour || (hour == tm_now.tm_hour && min < tm_now.tm_min) ) {
		tm_now.tm_mday++;
	}

	tm_now.tm_hour = hour;
	tm_now.tm_min  = min;
	tm_now.tm_sec  = 0;
	return mkgmtime( &tm_now );
}


time_t roadmap_sunrise( const RoadMapGpsPosition *pos, time_t gmt_now )
{
	// Note: non thread-safe (gmtime)
   time_t sunrise;
//   struct tm *realtime;

   sunrise = roadmap_sunriseset( pos, gmtime( &gmt_now ), ESUNRISE );
//	realtime = localtime(&sunrise);
//	printf("sunset:  %d:%d\n", realtime->tm_hour, realtime->tm_min);
	return sunrise;
}


time_t roadmap_sunset( const RoadMapGpsPosition *pos, time_t gmt_now )
{
	// Note: non thread-safe (gmtime)
   time_t sunset;
//   struct tm *realtime;


	sunset = roadmap_sunriseset( pos, gmtime( &gmt_now ), ESUNSET );
//	realtime = localtime(&sunset);
//	printf("sunset:  %d:%d\n", realtime->tm_hour, realtime->tm_min);
	return sunset;
}
