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


#include "RealtimeDefs.h"
#include "RealtimeMath.h"
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
int get_range_size( int range_begin, int range_end)
{ return range_end - range_begin;}

double relative_part_factor_from_absolute_value( int range_begin, int range_end, int value)
{ 
   int  range_size     = get_range_size( range_begin, range_end);
   int  relative_value = value - range_begin;
   
   assert( range_size);
   
   return (double)relative_value/(double)range_size;
}

int absolute_value_from_relative_part_factor(  int     range_begin, 
                                                int     range_end, 
                                                double   relative_part_factor)
{
   int  range_size = get_range_size( range_begin, range_end);
   float round_factor = 0.5;
   if( relative_part_factor < 0)
      round_factor = -0.5;
   return range_begin + (int)((relative_part_factor * range_size) + round_factor);
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
time_t time_get_range_size( time_t range_begin, time_t range_end)
{ return range_end - range_begin;}

double time_relative_part_factor_from_absolute_value( time_t range_begin, time_t range_end, time_t value)
{ 
   time_t  range_size     = time_get_range_size( range_begin, range_end);
   time_t  relative_value = value - range_begin;
   
   if( !range_size)
   {
      assert( range_size);
      return 0.F;
   }
   
   return (double)relative_value/(double)range_size;
}

time_t time_absolute_value_from_relative_part_factor( time_t   range_begin, 
                                                      time_t   range_end, 
                                                      double   relative_part_factor)
{
   time_t  range_size = time_get_range_size( range_begin, range_end);
   float round_factor = 0.5;
   if( relative_part_factor < 0)
      round_factor = -0.5;
   return range_begin + (time_t)((relative_part_factor * range_size) + round_factor);
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
RoadMapPosition   RoadMapPosition_from_relative_part_factor(   
                           RoadMapPosition   range_begin,
                           RoadMapPosition   range_end,
                           double            relative_part_factor)
{
   RoadMapPosition new_position;
   
   new_position.longitude  = absolute_value_from_relative_part_factor(  range_begin.longitude,
                                                                        range_end.  longitude,
                                                                        relative_part_factor);
   new_position.latitude   = absolute_value_from_relative_part_factor(  range_begin.latitude,
                                                                        range_end.  latitude,
                                                                        relative_part_factor);

   return new_position;
}                           
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
