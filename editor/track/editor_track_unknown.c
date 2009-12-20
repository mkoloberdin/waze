/* editor_track_unknown.c - handle tracks of unknown points
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
 *   See editor_track_unknown.h
 */

#include <assert.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_math.h"
#include "roadmap_gps.h"

#include "../editor_log.h"
#include "editor_track_main.h"
#include "editor_track_util.h"

#include "editor_track_unknown.h"

#define ROADMAP_NEIGHBOURHOUD  16
static RoadMapNeighbour RoadMapNeighbourhood[ROADMAP_NEIGHBOURHOUD];

/* Find if the current point matches a point on the current new line.
 * (Detect loops)
 */
static int match_distance_from_current
                    (const RoadMapGpsPosition *gps_point,
                     int last_point,
                     RoadMapPosition *best_intersection,
                     int *match_point) {

   int i;
   int distance;
   int smallest_distance;
   int azymuth = 0;
   RoadMapPosition intersection;

   smallest_distance = 0x7fffffff;

   for (i = 0; i < (last_point-1); i++) {

      distance =
         roadmap_math_get_distance_from_segment
            ((RoadMapPosition *)gps_point,
             track_point_pos (i),
             track_point_pos (i+1),
             &intersection, NULL);

      if (distance < smallest_distance) {
         smallest_distance = distance;
         *best_intersection = intersection;
         *match_point = i;
         azymuth = roadmap_math_azymuth
             (track_point_pos (i),
             track_point_pos (i+1));
      }
   }

   return (smallest_distance < editor_track_point_distance ()) &&
            (roadmap_math_delta_direction (azymuth, gps_point->steering) < 10);
}

typedef struct angle_pair_s {
   int index;
   int angle;
} angle_pair;


/* we want a descending list so this function inverts the real comparison */
static int angle_pair_cmp (const void *p1, const void *p2) {

   const angle_pair *pair1 = (const angle_pair *)p1;
   const angle_pair *pair2 = (const angle_pair *)p2;

   if (pair1->angle > pair2->angle) return -1;
   else if (pair1->angle < pair2->angle) return 1;
   else return 0;
}


static int find_line_break (int last_point,
                            int last_azymuth,
                            int current_point,
                            int *middle1,
                            int *middle2) {

#define MAX_TURN_POINTS 50

   angle_pair pairs[MAX_TURN_POINTS];
   int i;
   int j;
   int max_azymuth_diff = 0;
   int max_point_azymuth = 0;
   int max_azymuth_point = -1;
   int current_azymuth;
   
   if ((current_point - last_point + 1) > MAX_TURN_POINTS) return 0;

   j=0;
   for (i=last_point; i<=current_point; i++) {

      int diff;
      int prev_azymuth;

      current_azymuth = 
         roadmap_math_azymuth
            (track_point_pos (i),
             track_point_pos (i+1));

      diff = roadmap_math_delta_direction (last_azymuth, current_azymuth);

      if (diff > max_azymuth_diff) {
         max_azymuth_diff = diff;
      }

      if (i == last_point) {
	      prev_azymuth = last_azymuth;
		} else {
	      prev_azymuth = 
   	      roadmap_math_azymuth
      	      (track_point_pos (i-1),
         	    track_point_pos (i));
		}

      diff = roadmap_math_delta_direction (prev_azymuth, current_azymuth);
      pairs[j].index = i;
      pairs[j].angle = diff;
      j++;

      if (diff > max_point_azymuth) {
         max_point_azymuth = diff;
         max_azymuth_point = i;
      }
   }

   qsort (pairs, j, sizeof(angle_pair), angle_pair_cmp);

   if (max_azymuth_diff > 45) {

      int angle_sum = pairs[0].angle;
      int middle1_angle = pairs[0].angle;
      int middle2_angle = pairs[0].angle;
      *middle1 = pairs[0].index;
      *middle2 = pairs[0].index;

      j=1;

      while ((100 * angle_sum / max_azymuth_diff) < 70) {
         
         assert (j<(current_point-last_point+1));
         if (j>=(current_point-last_point+1)) break;

         angle_sum += pairs[j].angle;
         if (pairs[j].index < *middle1) {
            *middle1 = pairs[j].index;
            middle1_angle = pairs[j].angle;
         } else if (pairs[j].index > *middle2) {
            *middle2 = pairs[j].index;
            middle2_angle = pairs[j].angle;
         }
         j++;
      }

      return 1;
   }

   return 0;
}


/* Detect roundabouts */
static int is_roundabout (int first_point, int last_point) {

   //RoadMapPosition middle;
   int length;
   //int i;

   length = editor_track_util_length (first_point, last_point);

   if ((length < editor_track_point_distance ()*3) || (length > editor_track_point_distance ()*20)) {

      return 0;
   }

   return 1;
#if 0
   middle.longitude = middle.latitude = 0;
   for (i=first_point; i<last_point; i++) {

      middle.longitude += GpsPoints[i].longitude;
      middle.latitude += GpsPoints[i].latitude;
   }

   middle.longitude /= (last_point - first_point);
   middle.latitude /= (last_point - first_point);
    
   /* Fix the end of the segment before the roundabout */

   GpsPoints[first_point-1].longitude = middle.longitude;
   GpsPoints[first_point-1].latitude = middle.latitude;
#endif

}


static int detect_turn(int point_id,
                       const RoadMapGpsPosition *gps_position,
                       TrackNewSegment *new_segment,
                       int max,
                       int road_type) {

   static int last_straight_line_point = -1;
   static int last_straight_azymuth = 0;

   int middle1;
   int middle2;

   if (point_id == 0) return 0;

   if (point_id == 1) {
      last_straight_line_point = point_id;
      last_straight_azymuth =
         roadmap_math_azymuth
            (track_point_pos (point_id-1),
             track_point_pos (point_id));
      return 0;
   }

   assert (point_id > last_straight_line_point);

   /* trivial case */
   if ((last_straight_line_point == (point_id-1)) &&
      (roadmap_math_delta_direction
         (roadmap_math_azymuth
             (track_point_pos (point_id - 1),
              track_point_pos (point_id)),
             last_straight_azymuth) > 60)) {
      
      new_segment[0].type = road_type;
      new_segment[0].point_id = point_id - 1;

      last_straight_line_point = point_id;
      last_straight_azymuth =
         roadmap_math_azymuth
            (track_point_pos (point_id-1),
             track_point_pos (point_id));

      return 1;
   }

   if (roadmap_math_delta_direction
         (roadmap_math_azymuth
             (track_point_pos (point_id - 1),
              (RoadMapPosition *)gps_position),
          roadmap_math_azymuth
            (track_point_pos (point_id - 3),
             track_point_pos (point_id - 2))) < 10) {

      int this_straight_azymuth;

      /* this is a straight line, save it */

      this_straight_azymuth =
         roadmap_math_azymuth
             (track_point_pos (point_id - 3), track_point_pos (point_id));

      if (last_straight_line_point == -1) {
         last_straight_line_point = point_id;
         last_straight_azymuth = this_straight_azymuth;
         return 0;
      }

      if (!find_line_break
            (last_straight_line_point, last_straight_azymuth,
             point_id-3, &middle1, &middle2)) {

         last_straight_line_point = point_id;
         last_straight_azymuth = this_straight_azymuth;
         return 0;
      }

      last_straight_line_point = point_id - middle2;
      last_straight_azymuth = this_straight_azymuth;
     
      assert (middle1 > 0);
      if (middle1 > 0) {
         new_segment[0].type = road_type;
         new_segment[0].point_id = middle1;
      }

      if (middle1 != middle2) {
         /* make the curve a segment of its own */
         new_segment[1].type = TRACK_ROAD_TURN;
         new_segment[1].point_id = middle2;

         return 2;
      } else {

         return 1;
      }
   }

   return 0;
}


static int detect_loop(int point_id,
                       const RoadMapGpsPosition *gps_position,
                       TrackNewSegment *new_segment,
                       int max,
                       int road_type) {

   RoadMapPosition intersection;
   int loop_start_point = 0;

   /* see if we can match the position to the current line */
   if ((point_id > 4)  &&
         match_distance_from_current
               (gps_position, point_id, &intersection, &loop_start_point)) {

      //TODO roundabout - is this needed?
      //GpsPoints[point_id].longitude = intersection.longitude;
      //GpsPoints[point_id].latitude = intersection.latitude;

      if (is_roundabout (loop_start_point, point_id)) {
         
         new_segment[0].type = road_type;
         new_segment[0].point_id = loop_start_point;

         new_segment[1].type = TRACK_ROAD_ROUNDABOUT;
         new_segment[1].point_id = point_id;

         return 2;
      } else {

         new_segment[0].type = road_type;
         new_segment[0].point_id = point_id;

         return 1;
      }

   }

   return 0;
}


static int detect_road_segment (int point_id,
                                const RoadMapGpsPosition *gps_position,
                                TrackNewSegment *new_segment,
                                int max,
                                int point_type) {

   int count;
   
   int road_type = TRACK_ROAD_REG;
   if (point_type & POINT_GAP) {
      road_type = TRACK_ROAD_CONNECTION;
   }

   count = detect_loop (point_id, gps_position, new_segment, max, road_type);
   if (count) return count;

   count = detect_turn (point_id, gps_position, new_segment, max, road_type);
   return count;
}


int editor_track_unknown_locate_point (int point_id,
                                       const RoadMapGpsPosition *gps_position,
                                       RoadMapTracking *confirmed_street,
                                       RoadMapNeighbour *confirmed_line,
                                       TrackNewSegment *new_segment,
                                       int max_segments,
                                       int point_type) {

   RoadMapFuzzy best;
   RoadMapFuzzy second_best;
   RoadMapTracking nominated;
   int found;
   int count;
   
   if (point_type & POINT_UNKNOWN) {
      /* this point is already known to be unknown */
      return detect_road_segment
               (point_id, gps_position, new_segment, max_segments, point_type);
   }

   /* let's see if we got to a known street line */
   count = editor_track_util_find_street
            (gps_position, &nominated, confirmed_street, confirmed_line,
             RoadMapNeighbourhood, ROADMAP_NEIGHBOURHOUD, &found, &best,
             &second_best);

   if (roadmap_fuzzy_is_good (best) ||
         confirmed_street->valid) {

      /* we found an existing road, let's close the new road */
      
      /* delay ending the line until we find the best location to end it */
      if (roadmap_fuzzy_is_good (best) &&
               ( !confirmed_street->valid ||
                 (best > confirmed_street->cur_fuzzyfied))) {
//                 RoadMapNeighbourhood[found].distance <
//                 confirmed_line->distance)) {

         *confirmed_line   = RoadMapNeighbourhood[found];
         *confirmed_street = nominated;
         confirmed_street->valid = 1;
         confirmed_street->entry_fuzzyfied = best;
         confirmed_street->cur_fuzzyfied = best;

/*         
         if (!roadmap_fuzzy_is_certain (best)) {
            return 0;
         }
*/         
         if (confirmed_line->distance > editor_track_point_distance ()) {
            return 0;
         }
      }


      if (point_type & POINT_GAP) {
         new_segment->type = TRACK_ROAD_CONNECTION;
      } else {
         new_segment->type = TRACK_ROAD_REG;
      }
      new_segment->point_id = point_id;

      if (best != confirmed_street->cur_fuzzyfied) {
         new_segment->point_id--;
      }

      return 1;
   } else {

      /* point is unknown, see if we can detect a road segment */

      return detect_road_segment
               (point_id, gps_position, new_segment, max_segments, point_type);
   }
}

