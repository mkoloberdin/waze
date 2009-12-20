/* editor_track_compress.c - track compression algorithm
 *
 * LICENSE:
 *
 *   Copyright 2005 Ehud Shabtai
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

#include "editor_track_compress.h"
#include "roadmap_math.h"

#define  TRACK_MIN_VARIANT_THRESHOLD            (5)

#define	COMPRESSION_STATS 0

//////////////////////////////////////////////////////////////////////////////////////////////////
static int get_range_size( int range_begin, int range_end)
{ return range_end - range_begin;}

static int absolute_value_from_relative_part_factor(  int     range_begin, 
                                                		int     range_end, 
                                                		double  relative_part_factor)
{
   int  range_size = get_range_size( range_begin, range_end);
   float round_factor = 0.5;
   if( relative_part_factor < 0)
      round_factor = -0.5;
   return range_begin + (int)((relative_part_factor * range_size) + round_factor);
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
static time_t time_get_range_size( time_t range_begin, time_t range_end)
{ return range_end - range_begin;}

static double time_relative_part_factor_from_absolute_value( time_t range_begin, time_t range_end, double value)
{ 
   time_t  range_size     = time_get_range_size( range_begin, range_end);
   double  relative_value = value - range_begin;
   
   if( !range_size)
   {
		assert( range_size);
      //return 0.5F;
   }
   
   return relative_value/(double)range_size;
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
static RoadMapPosition   RoadMapPosition_from_relative_part_factor(   
                           RoadMapPosition   *range_begin,
                           RoadMapPosition   *range_end,
                           double            relative_part_factor)
{
   RoadMapPosition new_position;
   
   new_position.longitude  = absolute_value_from_relative_part_factor(  range_begin->longitude,
                                                                        range_end->  longitude,
                                                                        relative_part_factor);
   new_position.latitude   = absolute_value_from_relative_part_factor(  range_begin->latitude,
                                                                        range_end->  latitude,
                                                                        relative_part_factor);

   return new_position;
}                           
//////////////////////////////////////////////////////////////////////////////////////////////////



static int point_distance_from_expected_position(int   range_begin,
                                             	 int   range_end,
                                             	 int   point)
{
	RoadMapPosition pos1;
	RoadMapPosition pos2;
	if (track_point_time (range_begin) + 1 >= track_point_time (range_end)) {
		pos1 = *track_point_pos (range_begin);
		pos2 = *track_point_pos (range_end);
	} else {
	   double            factor;
	   factor      = time_relative_part_factor_from_absolute_value(
                                                                  track_point_time (range_begin), 
                                                                  track_point_time (range_end), 
                                                                  track_point_time (point) - 0.5);
	   pos1 = RoadMapPosition_from_relative_part_factor(  
                                                       track_point_pos (range_begin),
                                                       track_point_pos (range_end),
                                                       factor);
	   factor      = time_relative_part_factor_from_absolute_value(
                                                                  track_point_time (range_begin), 
                                                                  track_point_time (range_end), 
                                                                  track_point_time (point) + 0.5);
	   pos2 = RoadMapPosition_from_relative_part_factor(  
                                                       track_point_pos (range_begin),
                                                       track_point_pos (range_end),
                                                       factor);
	}

	return roadmap_math_get_distance_from_segment (track_point_pos (point),
																  &pos1, &pos2,
																  NULL, NULL);
}



static void  editor_track_compress_range (int from, int to)
{
   int               i;
   int               distance    =  0;
   int               farest      = -1;    // Index of the most distanced point
   
   *track_point_status (from) = POINT_STATUS_SAVE;
   *track_point_status (to) = POINT_STATUS_SAVE;
   
   if( (to - from) < 2)
      return;

   for( i=(from+1); i<to; i++)
   {
      int               cur_distance= point_distance_from_expected_position( 
                                                         from,   // Range begin
                                                         to,     // Range end
                                                         i);        // Point to test

      if(distance < cur_distance)
      {
         distance = cur_distance;
         farest   = i;
      }
   }
   
   if( (-1 == farest) || (distance < TRACK_MIN_VARIANT_THRESHOLD))
      return;

   editor_track_compress_range(from, farest);
   editor_track_compress_range(farest, to);
}


void  editor_track_compress_track (int from, int to)
{
   int               i;
#if COMPRESSION_STATS
   static int total_points_before = 0;
   static int total_points_after = 0;
#endif
   
   for (i = from; i <= to; i++) {
   
   	*track_point_status (i) = POINT_STATUS_IGNORE;
   }	
   
   editor_track_compress_range (from, to);
   
#if COMPRESSION_STATS
	for (i = from; i <= to; i++) {
		total_points_before++;
		if (POINT_STATUS_SAVE == *track_point_status (i)) {
			total_points_after++;
		}
	}
	printf ("Progressive compression ratio %d%% (%d/%d)\n", total_points_after * 100 / total_points_before,
				total_points_after, total_points_before);
#endif
   
}
