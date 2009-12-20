/*
 * LICENSE:
 *
 *   Copyright 2008 PazO
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License V2 as published by
 *   the Free Software Foundation.
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


#ifndef	__FREEMAP_REALTIMEMATH_H__
#define	__FREEMAP_REALTIMEMATH_H__
//////////////////////////////////////////////////////////////////////////////////////////////////

#include <time.h>

//////////////////////////////////////////////////////////////////////////////////////////////////
double   relative_part_factor_from_absolute_value( int range_begin, int range_end, int     value);
int      absolute_value_from_relative_part_factor( int range_begin, int range_end, double   relative_part_factor);

double   time_relative_part_factor_from_absolute_value( time_t range_begin, time_t range_end, time_t value);
time_t   time_absolute_value_from_relative_part_factor( time_t range_begin, time_t range_end, double relative_part_factor);

RoadMapPosition RoadMapPosition_from_relative_part_factor(  RoadMapPosition   range_begin,
                                                            RoadMapPosition   range_end,
                                                            double            relative_part_factor);
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#endif	//	__FREEMAP_REALTIMEMATH_H__
