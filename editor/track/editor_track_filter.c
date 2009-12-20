/* editor_track_filter.c - filter GPS trace
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
 *
 * NOTE:
 * This file implements all the "dynamic" editor functionality.
 * The code here is experimental and needs to be reorganized.
 * 
 * SYNOPSYS:
 *
 *   See editor_track_filter.h
 */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "roadmap.h"
#include "roadmap_math.h"
#include "roadmap_gps.h"

#include "../editor_log.h"

#include "editor_track_filter.h"

typedef struct GPSFilter {

   int max_distance;
   time_t max_stand_time;
   time_t timeout;
   int point_distance;

   int first_point;
   RoadMapGpsPosition last_gps_point;
   time_t last_gps_time;
   time_t last_gps_move_time;
   int last_azymuth;

   RoadMapGpsPosition normalized_gps_point;

} GPSFilter;


void editor_track_filter_reset (GPSFilter *filter) {

   filter->first_point = 1;
}


GPSFilter *editor_track_filter_new (int max_distance,
                                    int max_stand_time,
                                    int timeout,
                                    int point_distance) {

   GPSFilter *filter = malloc (sizeof(GPSFilter));

   filter->max_distance = max_distance;
   filter->max_stand_time = max_stand_time;
   filter->timeout = timeout;
   filter->point_distance = point_distance;

   editor_track_filter_reset (filter);

   return filter;
}


int editor_track_filter_add (GPSFilter *filter,
                             time_t gps_time,
                             const RoadMapGpsPrecision *dilution,
                             const RoadMapGpsPosition* gps_position) {

   int azymuth;
   
   if (filter->first_point) {

      filter->first_point = 0;
      filter->last_azymuth = gps_position->steering;
      filter->last_gps_point = *gps_position;
      filter->last_gps_move_time = gps_time;
      filter->last_gps_time = gps_time;
      filter->normalized_gps_point = *gps_position;
      return 0;
   }
   
   if (gps_time > filter->last_gps_time + filter->timeout ||
      gps_time < filter->last_gps_time - filter->timeout) {
      editor_track_filter_reset (filter);
      editor_track_filter_add (filter, gps_time, dilution, gps_position);

      return ED_TRACK_END;
   }

   filter->last_gps_time = gps_time;

   //if (gps_position->speed == 0) return 0;

   if ((filter->last_gps_point.latitude == gps_position->latitude) &&
       (filter->last_gps_point.longitude == gps_position->longitude)) return 0;

   if (dilution->dilution_horizontal > 6) {

      return 0;
   }

   if ((roadmap_math_distance
            ((RoadMapPosition *) &filter->last_gps_point,
              (RoadMapPosition*) gps_position) >= filter->max_distance) ||
         
         (gps_time < filter->last_gps_move_time) ||
         ((gps_time - filter->last_gps_move_time) > filter->max_stand_time)) {

      editor_track_filter_reset (filter);
      editor_track_filter_add (filter, gps_time, dilution, gps_position);

      return ED_TRACK_END;
   }

   filter->last_gps_move_time = gps_time;

   filter->normalized_gps_point.longitude =
         (filter->normalized_gps_point.longitude +
          gps_position->longitude) / 2;
            
   filter->normalized_gps_point.latitude =
         (filter->normalized_gps_point.latitude +
         gps_position->latitude) / 2;

   filter->normalized_gps_point.speed =
          gps_position->speed;

   azymuth = roadmap_math_azymuth
               ((RoadMapPosition *) &filter->last_gps_point,
                (RoadMapPosition *) &filter->normalized_gps_point);

   filter->normalized_gps_point.steering = azymuth;

	//TODO is this better?
   filter->normalized_gps_point.steering = gps_position->steering;

   /* ignore consecutive points which generate a big curve */
   if (roadmap_math_delta_direction (azymuth, filter->last_azymuth) > 90) {

      filter->last_gps_point = filter->normalized_gps_point;
   }

   filter->last_azymuth = azymuth;
   return 0;
}


const RoadMapGpsPosition *editor_track_filter_get (GPSFilter *filter) {

   if (roadmap_math_distance
            ( (RoadMapPosition *) &filter->last_gps_point,
              (RoadMapPosition*) &filter->normalized_gps_point) >=
            filter->point_distance) {

      RoadMapGpsPosition interpolated_point = filter->normalized_gps_point;

      while (roadmap_math_distance
            ( (const RoadMapPosition*) &filter->last_gps_point,
              (const RoadMapPosition*) (void*)&interpolated_point ) >=
                                 (filter->point_distance * 2)) {

            interpolated_point.longitude =
               (interpolated_point.longitude +
                filter->last_gps_point.longitude) / 2;
            
            interpolated_point.latitude =
               (interpolated_point.latitude +
               filter->last_gps_point.latitude) / 2;
      }

      filter->last_gps_point = interpolated_point;
      
      return &filter->last_gps_point;
   }

   return NULL;
}


int editor_track_filter_get_current (const GPSFilter *filter, RoadMapPosition *pos, time_t *time) {

	if (filter->first_point) return 0;

	pos->longitude = filter->normalized_gps_point.longitude;
	pos->latitude = filter->normalized_gps_point.latitude;
	*time = filter->last_gps_time;
	return 1;	
}

