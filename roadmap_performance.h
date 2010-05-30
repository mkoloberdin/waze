/* roadmap_performance.h - macros and definitions for the performance tests
 *
 * LICENSE:
 *
 *   Copyright 2010,  Alex Agranovich (AGA), Waze Ltd
 *
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
 */

#ifndef INCLUDE__ROADMAP_PERFORMANCE__H
#define INCLUDE__ROADMAP_PERFORMANCE__H


#if defined(ANDROID) || defined(IPHONE) || defined(__SYMBIAN32__)
#include <sys/time.h>

#define CUR_TIME_MSEC( var_out ) \
{ \
	struct timeval timeval;	\
	long val;				\
	gettimeofday( &timeval, NULL ); \
	val = ( timeval.tv_sec * 1000 + timeval.tv_usec / 1000 ); \
	var_out = val;	\
}

#define CUR_TIME_USEC( var_out ) \
{ \
	struct timeval timeval;	\
	long val;				\
	gettimeofday( &timeval, NULL ); \
	val = ( timeval.tv_sec * 1000000L + timeval.tv_usec ); \
	var_out = val;	\
}

#define TIMESTAMP_START() \
{ \
	struct timeval start_time, end_time;	\
	long delta = -1;				\
	gettimeofday( &start_time, NULL );


#define TIMESTAMP_END( printString, threshold ) \
	gettimeofday( &end_time, NULL );			\
	delta = ( end_time.tv_sec * 1000 + end_time.tv_usec / 1000 ) -	\
	( start_time.tv_sec * 1000 + start_time.tv_usec / 1000 );			\
	if ( delta > threshold )		\
	{								\
		roadmap_log_raw_data_fmt( "#### PROFILING TIMESTAMP. %s. Timeout: %d msec \n", printString, delta );	\
	}	\
}

#define TIMESTAMP_END_U( printString, threshold ) \
	gettimeofday( &end_time, NULL );			\
	delta = ( end_time.tv_sec * 1000000L + end_time.tv_usec ) -	\
	( start_time.tv_sec * 1000000L + start_time.tv_usec );			\
	if ( delta > threshold )		\
	{								\
		roadmap_log_raw_data_fmt( "#### PROFILING TIMESTAMP. %s. Timeout: %d usec \n", printString, delta );	\
	}	\
}
/*
 * Average tests
 */
#define TIMESTAMP_AVG_START() \
{ \
	struct timeval start_time, end_time;	\
	long delta = -1;				\
	static long sample_count = 0; \
    static long sample_acc_time = 0; \
    int enough_samples_for_print = 0; \
	gettimeofday( &start_time, NULL );



/*
 * Average tests - end block in milliseconds
 */
#define TIMESTAMP_AVG_END( print_string, time_threshold, count_threshold ) \
	gettimeofday( &end_time, NULL );			\
	delta = ( end_time.tv_sec * 1000 + end_time.tv_usec/1000 ) -	\
	( start_time.tv_sec * 1000 + start_time.tv_usec/1000 );			\
	sample_count++;			\
	sample_acc_time += delta; \
	enough_samples_for_print =  ( ( sample_count % count_threshold ) == 0 ); \
	if ( enough_samples_for_print && delta > time_threshold )		\
	{								\
		roadmap_log_raw_data_fmt( "#### PROFILING TIMESTAMP AVERAGE. %s. Timeout average: %d msec. Count: %d. Last Delta: %d \n",		\
							print_string, sample_acc_time/sample_count, sample_count, delta );	                            \
	}	\
}

/*
 * Average tests - end block in microseconds
 */
#define TIMESTAMP_AVG_END_U( print_string, time_threshold, count_threshold ) \
	gettimeofday( &end_time, NULL );			\
	delta = ( end_time.tv_sec * 1000000L + end_time.tv_usec ) -	\
	( start_time.tv_sec * 1000000L + start_time.tv_usec );			\
	sample_count++;			\
	sample_acc_time += delta; \
	enough_samples_for_print =  ( ( sample_count % count_threshold ) == 0 ); \
	if ( enough_samples_for_print && delta > time_threshold )		\
	{								\
		roadmap_log_raw_data_fmt( "#### PROFILING TIMESTAMP AVERAGE. %s. Timeout average: %d usec. Count: %d. Last Delta: %d \n",	\
							print_string, sample_acc_time/sample_count, sample_count, delta );	                            \
	}	\
}

#endif


#endif // INCLUDE__ROADMAP_PERFORMANCE__H

