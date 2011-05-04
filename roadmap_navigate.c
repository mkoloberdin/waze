/* roadmap_navigate.c - Basic navigation engine for RoadMap.
 *
 * LICENSE:
 *
 *   Copyright 2003 Pascal F. Martin
 *   Copyright 2008 Ehud Shabtai
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
 *   See roadmap_navigate.h.
 */

#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_gui.h"
#include "roadmap_math.h"
#include "roadmap_config.h"
#include "roadmap_message.h"
#include "roadmap_line.h"
#include "roadmap_line_route.h"
#include "roadmap_square.h"
#include "roadmap_street.h"
#include "roadmap_layer.h"
#include "roadmap_display.h"
#include "roadmap_locator.h"
#include "roadmap_fuzzy.h"
#include "roadmap_adjust.h"
#include "roadmap_plugin.h"
#include "roadmap_point.h"
#include "roadmap_string.h"
#include "roadmap_trip.h"
#include "roadmap_alerter.h"
#include "animation/roadmap_animation.h"
#include "roadmap_time.h"

//FIXME remove when navigation will support plugin lines
#include "editor/editor_plugin.h"
#include "editor/editor_cleanup.h"

#include "roadmap_navigate.h"
#include "roadmap_plugin.h"

static RoadMapConfigDescriptor RoadMapNavigateMinMobileSpeedCfg =
                        ROADMAP_CONFIG_ITEM("Navigation", "Min Mobile Knots");
static RoadMapConfigDescriptor RoadMapNavigateMaxJamSpeedCfg =
                        ROADMAP_CONFIG_ITEM("Navigation", "Max Jam Knots");
static RoadMapConfigDescriptor RoadMapNavigateMinJamDistanceFromEndCfg =
                        ROADMAP_CONFIG_ITEM("Navigation", "Min Jam Distance");
static int RoadMapNavigateMinMobileSpeed = -1;
static int RoadMapNavigateMaxJamSpeed = -1;
static int RoadMapNavigateJammedSince = 0;
static int RoadMapNavigateMinJamDistanceFromEnd = 0;

static RoadMapConfigDescriptor RoadMapNavigateFlag =
                        ROADMAP_CONFIG_ITEM("Navigation", "Enable");

#define MIN_SPEED_INIT_TIME   10
#define HIGH_SPEED_FILTER     32 //32 knot = 60 kph

//#define DEBUG_PRINTS //TODO: remove this when not debugging

typedef struct {
   RoadMapNavigateRouteCB callbacks;
   PluginLine current_line;
   PluginLine next_line;
   int enabled;
   int initialized;

} RoadMapNavigateRoute;

typedef struct{
   int index;
   RoadMapFuzzy result;
   BOOL found;
   BOOL in_route;
   RoadMapTracking tracking;
} RoadMapNavCandidates;

static void animation_set_callback (void *context);
static void animation_ended_callback (void *context);

static RoadMapAnimationCallbacks gAnimationCallbacks =
{
   animation_set_callback,
   animation_ended_callback
};

static const char* GPS_OBJECT = "displayed_gps";


#define ROADMAP_NAVIGATE_ROUTE_NULL \
{ {0}, PLUGIN_LINE_NULL, PLUGIN_LINE_NULL, 0 }

static RoadMapNavigateRoute RoadMapRouteInfo;

/* The navigation flag will be set according to the session's content,
 * so that the user selected state is kept from one session to
 * the next.
 */
static int RoadMapNavigateEnabled = 0;

/* Avoid doing navigation work when the position has not changed. */
static RoadMapGpsPosition RoadMapLatestGpsPosition = {0, 0, 0, -1, INVALID_STEERING};
static RoadMapPosition    RoadMapLatestPosition;
static time_t             RoadMapLatestUpdate;
static time_t				  RoadMapLatestGpsTime = 0;

#define ROADMAP_NEIGHBOURHOUD  16

static RoadMapTracking  RoadMapConfirmedStreet = ROADMAP_TRACKING_NULL;

static RoadMapNeighbour RoadMapConfirmedLine = ROADMAP_NEIGHBOUR_NULL;
static RoadMapNeighbour RoadMapNeighbourhood[ROADMAP_NEIGHBOURHOUD];

#define MAX_SEGMENT_CHANGED_CALLBACKS 5
static RoadMapNavigateSegmentChangedCB NavigateSegmentChangedCB[MAX_SEGMENT_CHANGED_CALLBACKS];


void roadmap_navigate_register_segment_changed(RoadMapNavigateSegmentChangedCB cb){
   int index;
   for (index = 0; index < MAX_SEGMENT_CHANGED_CALLBACKS; index++) {
      if (NavigateSegmentChangedCB[index] == NULL){
         NavigateSegmentChangedCB[index] = cb;
         break;
      }
   }
}

void roadmap_navigate_unregister_segment_changed(RoadMapNavigateSegmentChangedCB cb){
   int index;
   for (index = 0; index < MAX_SEGMENT_CHANGED_CALLBACKS; index++) {
      if (NavigateSegmentChangedCB[index] == cb){
         NavigateSegmentChangedCB[index] = NULL;
         break;
      }
   }
}

static void on_segment_changed_inform_clients(PluginLine *line, int direction){
   int index;
   for (index = 0; index < MAX_SEGMENT_CHANGED_CALLBACKS; index++) {
      if (NavigateSegmentChangedCB[index] != NULL)
         (*NavigateSegmentChangedCB[index])(line, direction);
   }
}


static void roadmap_navigate_trace (const char *format, PluginLine *line) {

    char text[1024];
    PluginStreet street;
    PluginStreetProperties properties;

    if (! roadmap_log_enabled (ROADMAP_DEBUG)) return;

#ifdef __SYMBIAN32__
    return;
#endif

    roadmap_plugin_get_street (line, &street);

    roadmap_plugin_get_street_properties (line, &properties, 0);

    roadmap_message_set ('#', properties.address);
    roadmap_message_set ('N', properties.street);
    roadmap_message_set ('C', properties.city);

    text[0] = 0;
    if (roadmap_message_format (text, sizeof(text), format)) {
        printf ("%s (plugin_id %d, line_id %d, street_id %d)\n",
      text,
      roadmap_plugin_get_id (line),
      roadmap_plugin_get_line_id (line),
      roadmap_plugin_get_street_id (&street));
    }
}


static void roadmap_navigate_adjust_focus
                (RoadMapArea *focus, const RoadMapGuiPoint *focused_point) {

    RoadMapPosition focus_position;

    roadmap_math_to_position (focused_point, &focus_position, 1);

    if (focus_position.longitude < focus->west) {
        focus->west = focus_position.longitude;
    }
    if (focus_position.longitude > focus->east) {
        focus->east = focus_position.longitude;
    }
    if (focus_position.latitude < focus->south) {
        focus->south = focus_position.latitude;
    }
    if (focus_position.latitude > focus->north) {
        focus->north = focus_position.latitude;
    }
}

#ifdef OGL_TILE
static void roadmap_navigate_animate_gps (const RoadMapGpsPosition *new_gps_pos) {
   RoadMapAnimation *animation;
   const RoadMapGpsPosition *current_gps_pos;
   RoadMapPosition from_pos, to_pos;
   int from_dir, to_dir;
   RoadMapNeighbour candidate;
   int distance = 0;
   
   
   current_gps_pos = roadmap_trip_get_gps_position("GPS");
   
   if (current_gps_pos) {
      from_dir = current_gps_pos->steering;
      roadmap_math_normalize_orientation(&from_dir);
      to_dir = new_gps_pos->steering;
      roadmap_math_normalize_orientation(&to_dir);
      if (to_dir - from_dir >= 180 ||
          to_dir - from_dir <= -180) {
         if (360 - from_dir < 360 - to_dir)
            from_dir = from_dir - 360;
         else
            to_dir = to_dir - 360;
      }
      
      to_pos.longitude = new_gps_pos->longitude;
      to_pos.latitude = new_gps_pos->latitude;
      
      if (RoadMapConfirmedStreet.valid &&
          roadmap_plugin_get_distance
          (&to_pos,
           &RoadMapConfirmedLine.line,
           &candidate) &&
          roadmap_plugin_same_line (&candidate.line, &RoadMapConfirmedLine.line)) {
         if (current_gps_pos->latitude == candidate.intersection.latitude &&
             current_gps_pos->longitude == candidate.intersection.longitude &&
             current_gps_pos->speed == new_gps_pos->speed &&
             from_dir == to_dir) {
            return;
         }
      } else if (current_gps_pos->latitude == new_gps_pos->latitude &&
                 current_gps_pos->longitude == new_gps_pos->longitude &&
                 current_gps_pos->speed == new_gps_pos->speed &&
                 from_dir == to_dir) {
         return;
      }
      
      from_pos.longitude = current_gps_pos->longitude;
      from_pos.latitude = current_gps_pos->latitude;
      distance = roadmap_math_distance(&from_pos, &to_pos);
   }
   
   if (!current_gps_pos ||
       distance > 100) {
      roadmap_trip_set_mobile ("GPS", new_gps_pos);
      return;
   }
   
   animation = roadmap_animation_create();
   
   if (animation) {
      snprintf(animation->object_id, ANIMATION_MAX_OBJECT_LENGTH, "%s", GPS_OBJECT);
      animation->properties_count = 4;
      // lon / lat
      animation->properties[0].type = ANIMATION_PROPERTY_POSITION_X;
      animation->properties[0].from = current_gps_pos->longitude;
      animation->properties[0].to = new_gps_pos->longitude;
      animation->properties[1].type = ANIMATION_PROPERTY_POSITION_Y;
      animation->properties[1].from = current_gps_pos->latitude;
      animation->properties[1].to = new_gps_pos->latitude;
      // rotation
      animation->properties[2].type = ANIMATION_PROPERTY_ROTATION;
      animation->properties[2].from = from_dir;
      animation->properties[2].to = to_dir;
      // speed
      animation->properties[3].type = ANIMATION_PROPERTY_SPEED;
      animation->properties[3].from = current_gps_pos->speed;
      animation->properties[3].to = new_gps_pos->speed;
      
      //altitude is currently not used...
      
      
      animation->duration = RM_SCREEN_CAR_ANIMATION;
      animation->priority = ANIMATION_PRIORITY_GPS;
      animation->callbacks = &gAnimationCallbacks;
      roadmap_animation_register(animation);
   }
}
#endif

static void roadmap_navigate_set_mobile (const RoadMapGpsPosition *gps,
                                         const RoadMapPosition *fixed_pos,
                                         int fixed_streering) {

   RoadMapGpsPosition fixed_gps_pos;

   fixed_gps_pos = *gps;

   if (fixed_pos != NULL) {
      fixed_gps_pos.longitude = fixed_pos->longitude;
      fixed_gps_pos.latitude = fixed_pos->latitude;

      if (roadmap_math_delta_direction(fixed_streering, gps->steering) < 90) {
         fixed_gps_pos.steering = fixed_streering;
      } else if (roadmap_math_delta_direction(
                  fixed_streering + 180, gps->steering) < 90) {
         fixed_gps_pos.steering = fixed_streering + 180;
         if (fixed_gps_pos.steering > 359) fixed_gps_pos.steering -= 360;
      }
   }

#ifdef OGL_TILE
   roadmap_navigate_animate_gps (&fixed_gps_pos);
#else
   roadmap_trip_set_mobile ("GPS", &fixed_gps_pos);
#endif
   roadmap_navigate_check_alerts (&fixed_gps_pos);
   RoadMapLatestUpdate = time(NULL);
}

int roadmap_navigate_get_neighbours
              (const RoadMapPosition *position, int scale, int accuracy, int max_shapes,
               RoadMapNeighbour *neighbours, int max, int type) {

    int count = 0;
    int layers[128];
    int layer_count;

    RoadMapArea focus;

    RoadMapGuiPoint focus_point;
    RoadMapPosition focus_position;


    roadmap_log_push ("roadmap_navigate_get_neighbours");

    if (roadmap_math_point_is_visible (position)) {
       accuracy = ADJ_SCALE(accuracy); //AviR - patch

       roadmap_math_coordinate (position, &focus_point);
       roadmap_math_rotate_project_coordinate (&focus_point);

       focus_point.x += accuracy;
       focus_point.y += accuracy;
       roadmap_math_to_position (&focus_point, &focus_position, 1);

       focus.west = focus_position.longitude;
       focus.east = focus_position.longitude;
       focus.north = focus_position.latitude;
       focus.south = focus_position.latitude;

       accuracy *= 2;

       focus_point.x -= accuracy;
       roadmap_navigate_adjust_focus (&focus, &focus_point);

       focus_point.y -= accuracy;
       roadmap_navigate_adjust_focus (&focus, &focus_point);

       focus_point.x += accuracy;
       roadmap_navigate_adjust_focus (&focus, &focus_point);

    } else {

       accuracy *= 100;

       focus.west = position->longitude - accuracy;
       focus.east = position->longitude + accuracy;
       focus.north = position->latitude + accuracy;
       focus.south = position->latitude - accuracy;
    }

    if (type == LAYER_VISIBLE_ROADS) {
       layer_count = roadmap_layer_visible_roads (layers, 128);
    } else {
       layer_count = roadmap_layer_all_roads (layers, 128);
    }

    if (layer_count > 0) {

        roadmap_math_set_focus (&focus);

        count = roadmap_street_get_closest
	                    (position, scale, layers, layer_count, max_shapes, neighbours, max);

        count = roadmap_plugin_get_closest
                    (position, layers, layer_count, max_shapes, neighbours, count, max);

        roadmap_math_release_focus ();
    }

    roadmap_log_pop ();

    return count;
}


void roadmap_navigate_disable (void) {

    RoadMapNavigateEnabled = 0;
    roadmap_display_hide ("Approach");
    roadmap_display_hide ("Current Street");

    roadmap_config_set (&RoadMapNavigateFlag, "no");
}


void roadmap_navigate_enable  (void) {

    RoadMapNavigateEnabled  = 1;

    roadmap_config_set (&RoadMapNavigateFlag, "yes");
}


int roadmap_navigate_retrieve_line
        (const RoadMapPosition *position, int scale, int accuracy, PluginLine *line,
         int *distance, int type) {

    RoadMapNeighbour closest;

    if (roadmap_navigate_get_neighbours
           (position, scale, accuracy, 1, &closest, 1, type) <= 0) {

       return -1;
    }

    *distance = closest.distance;

    if (roadmap_plugin_activate_db (&closest.line) == -1) {
       return -1;
    }

    *line = closest.line;
    return 0;
}


int roadmap_navigate_retrieve_line_force_name
        (const RoadMapPosition *position, int scale, int accuracy, PluginLine *line,
         int *distance, int type, int orig_square, const char *name,
         int route, int fake_from, int fake_to,
         RoadMapPosition *force_from, RoadMapPosition *force_to) {

    RoadMapNeighbour candidate[5];
    RoadMapStreetProperties street;
    int i;
    int n;

    n = roadmap_navigate_get_neighbours
           (position, scale, accuracy, 1, candidate, 5, type);
    if (n <= 0) {
       return -1;
    }

    for (i = 0; i < n; i++) {

		  if (candidate[i].line.plugin_id == ROADMAP_PLUGIN_ID &&
		  		orig_square != candidate[i].line.square) {
	        roadmap_square_set_current (candidate[i].line.square);
   		  roadmap_street_get_properties (candidate[i].line.line_id, &street);
	        if ((!strcmp (name, roadmap_street_get_full_name (&street))) &&
	        		(roadmap_line_route_get_direction (candidate[i].line.line_id, ROUTE_DIRECTION_ANY) == route) ) {
	        	  if ((fake_from && !roadmap_line_from_is_fake (candidate[i].line.line_id)) ||
	        	  		(fake_to && !roadmap_line_to_is_fake (candidate[i].line.line_id))) {
	        	     continue;
	        	  }
		        if (force_from || force_to) {
			        	RoadMapPosition from;
			        	RoadMapPosition to;
			        	roadmap_street_extend_line_ends (&candidate[i].line, &from, &to, FLAG_EXTEND_BOTH, NULL, NULL);
			        	if (abs (from.longitude - force_from->longitude) < 4 &&
			        		 abs (from.latitude - force_from->latitude) < 4 &&
			        		 abs (to.longitude - force_to->longitude) < 4 &&
			        		 abs (to.latitude - force_to->latitude) < 4) {
			        		break;
			        	}
		        } else {
			     		break;
		        }
	        }
		  }
    }

    if (i >= n) {
        return -1;
    }

    if (roadmap_plugin_activate_db (&(candidate[i].line)) == -1) {
       return -1;
    }

    *distance = candidate[i].distance;

    *line = candidate[i].line;
    return 0;
}


int roadmap_navigate_fuzzify_internal (RoadMapTracking *tracked,
                                       RoadMapTracking *previous_street,
                                       RoadMapNeighbour *previous_line,
                                       RoadMapNeighbour *line,
                                       int against_direction,
                                       int direction,
                                       int confidence,
                                       int speed) {
   
   RoadMapFuzzy fuzzyfied_distance;
   RoadMapFuzzy fuzzyfied_direction;
   RoadMapFuzzy fuzzyfied_direction_with_line = 0;
   RoadMapFuzzy fuzzyfied_direction_against_line = 0;
   RoadMapFuzzy connected;
   int line_direction = 0;
   int azymuth_with_line = 0;
   int azymuth_against_line = 0;
   int symetric = 0;
   
   tracked->debug.direction = roadmap_fuzzy_false();
   tracked->debug.distance  = roadmap_fuzzy_false();
   tracked->debug.connected = roadmap_fuzzy_false();
   
   tracked->opposite_street_direction = against_direction;
   
   if (!previous_street || !previous_street->valid) {
      previous_line = NULL;
   }
   
   fuzzyfied_distance = roadmap_fuzzy_distance (line->distance);
   
   if (! roadmap_fuzzy_is_acceptable (fuzzyfied_distance)) {
      return roadmap_fuzzy_false ();
   }
   
   line_direction = roadmap_plugin_get_direction (&line->line, ROUTE_CAR_ALLOWED);
   
   if ((line_direction == ROUTE_DIRECTION_NONE) ||
       (line_direction == ROUTE_DIRECTION_ANY)) {
      symetric = 1;
   } else if (against_direction) {
      if (line_direction == ROUTE_DIRECTION_WITH_LINE) {
         line_direction = ROUTE_DIRECTION_AGAINST_LINE;
      } else {
         line_direction = ROUTE_DIRECTION_WITH_LINE;
      }
   }
   
   if (symetric || (line_direction == ROUTE_DIRECTION_WITH_LINE)) {
      azymuth_with_line = roadmap_math_azymuth (&line->from, &line->to);
      fuzzyfied_direction_with_line =
      roadmap_fuzzy_direction (azymuth_with_line, direction, 0);
      
   }
   
   if (symetric || (line_direction == ROUTE_DIRECTION_AGAINST_LINE)) {
      azymuth_against_line = roadmap_math_azymuth (&line->to, &line->from);
      fuzzyfied_direction_against_line =
      roadmap_fuzzy_direction (azymuth_against_line, direction, 0);
   }
   
   if (fuzzyfied_direction_against_line >
       fuzzyfied_direction_with_line) {
      
      fuzzyfied_direction = fuzzyfied_direction_against_line;
      tracked->azymuth = azymuth_against_line;
      tracked->line_direction = ROUTE_DIRECTION_AGAINST_LINE;
   } else {
      
      fuzzyfied_direction = fuzzyfied_direction_with_line;
      tracked->azymuth = azymuth_with_line;
      tracked->line_direction = ROUTE_DIRECTION_WITH_LINE;
   }
   
   if (! roadmap_fuzzy_is_acceptable (fuzzyfied_direction)) {
      return roadmap_fuzzy_false ();
   }
   
   if (previous_line != NULL) {
      connected = roadmap_fuzzy_connected(line, previous_line, previous_street->line_direction,
                                          tracked->line_direction, &tracked->entry);
   } else {
      connected = roadmap_fuzzy_true ();
   }
   
   tracked->debug.direction = fuzzyfied_direction;
   tracked->debug.distance  = fuzzyfied_distance;
   tracked->debug.connected = connected;
   
   
   
   return roadmap_fuzzy_and (connected,
                             roadmap_fuzzy_and (fuzzyfied_distance, fuzzyfied_direction));
   
   
   
   
   
   
   if (speed > HIGH_SPEED_FILTER) {
#ifdef DEBUG_PRINTS
      printf("high speed filter\n");
#endif
      if (confidence == roadmap_fuzzy_true())
         return connected * 3/4 + roadmap_fuzzy_and (fuzzyfied_distance, fuzzyfied_direction) * 1/4;
      else if (roadmap_fuzzy_is_certain(confidence))
         return connected * 2/3 + roadmap_fuzzy_and (fuzzyfied_distance, fuzzyfied_direction) * 1/3;
      else
         return roadmap_fuzzy_and (connected,
                                   roadmap_fuzzy_and (fuzzyfied_distance, fuzzyfied_direction));
   } else {
#ifdef DEBUG_PRINTS
      printf("low speed filter\n");
#endif
      //if (roadmap_fuzzy_is_certain(confidence))
      //   return connected * 17/24 + fuzzyfied_distance * 7/24;// + fuzzyfied_direction * 1/24;
      //else
      if (roadmap_fuzzy_is_good(confidence)) 
         return connected * 8/24 + fuzzyfied_distance * 31/48 + fuzzyfied_direction * 1/48;
      else
         return roadmap_fuzzy_and (connected,
                                   roadmap_fuzzy_and (fuzzyfied_distance, fuzzyfied_direction));
#if 0
      if (confidence == roadmap_fuzzy_true())
         return connected * 17/24 + fuzzyfied_distance * 7/24;// + fuzzyfied_direction * 1/24;
      else if (roadmap_fuzzy_is_certain(confidence))
         return connected * 13/24 + fuzzyfied_distance * 11/24;// + fuzzyfied_direction * 1/24;
      else
         return connected * 12/24 + fuzzyfied_distance * 11/24 + fuzzyfied_direction * 1/24;
#endif
   }
   
}

int roadmap_navigate_fuzzify (RoadMapTracking *tracked,
                              RoadMapTracking *previous_street,
                              RoadMapNeighbour *previous_line,
                              RoadMapNeighbour *line,
                              int against_direction,
                              int direction) {
   return roadmap_navigate_fuzzify_internal (tracked, previous_street, previous_line, line, against_direction, direction, 0, -1);
}


static int roadmap_navigate_confirm_intersection
                             (const RoadMapGpsPosition *position) {

    int delta;

    if (!PLUGIN_VALID(RoadMapConfirmedStreet.intersection)) return 0;

    delta = roadmap_math_delta_direction (position->steering,
                                     RoadMapConfirmedStreet.azymuth);

    return (delta < 90 && delta > -90);
}


/* Check if the given street block is a possible intersection and
 * evaluate how good an intersection guess it would be.
 * The criteria for a good intersection is that it must be as far
 * as possible from matching our current direction.
 */
static RoadMapFuzzy roadmap_navigate_is_intersection
                        (PluginLine *line,
                         int direction,
                         const RoadMapPosition *crossing) {

    RoadMapPosition line_to;
    RoadMapPosition line_from;

    if (roadmap_plugin_same_line (line, &RoadMapConfirmedLine.line))
          return roadmap_fuzzy_false();

    roadmap_plugin_line_to   (line, &line_to);
    roadmap_plugin_line_from (line, &line_from);

    if ((line_from.latitude != crossing->latitude) ||
        (line_from.longitude != crossing->longitude)) {

        if ((line_to.latitude != crossing->latitude) ||
            (line_to.longitude != crossing->longitude)) {

            return roadmap_fuzzy_false();
        }
    }

    return roadmap_fuzzy_not
               (roadmap_fuzzy_direction
                    (roadmap_math_azymuth (&line_from, &line_to), direction, 1));
}


/* Find the next intersection point.
 * We compare the current direction with the azimuth to the two ends of
 * the closest segment of line. From there, we deduct which end of the line
 * we are going to. Note that we do not consider the azimuth toward the ends
 * of the line, as the line might be a curve, and the result might be quite
 * confusing (imagine a mountain road..).
 * Once we know where we are going, we select a street that intersects
 * this point and seems the best guess as an intersection.
 */
static int roadmap_navigate_find_intersection
                  (const RoadMapGpsPosition *position, PluginLine *found) {

    int cfcc;
    int square;
    int first_line;
    int last_line;

    int delta_to;
    int delta_from;
    RoadMapPosition crossing;

    RoadMapFuzzy match;
    RoadMapFuzzy best_match;

    RoadMapStreetProperties properties;

    int fips = roadmap_locator_active ();

    PluginLine plugin_lines[100];
    int count;
    int i;

    delta_from =
        roadmap_math_delta_direction
            (position->steering,
             roadmap_math_azymuth (&RoadMapLatestPosition,
                                   &RoadMapConfirmedLine.from));

    delta_to =
        roadmap_math_delta_direction
            (position->steering,
             roadmap_math_azymuth (&RoadMapLatestPosition,
                                   &RoadMapConfirmedLine.to));

    if (delta_to < delta_from - 30) {
       roadmap_plugin_line_to (&RoadMapConfirmedLine.line, &crossing);
    } else if (delta_from < delta_to - 30) {
       roadmap_plugin_line_from (&RoadMapConfirmedLine.line, &crossing);
    } else {
       return 0; /* Not sure enough. */
    }

    /* Should we compare to RoadMapConfirmedStreet.entry so
     * to detect U-turns ?
     */

    /* Now we know which endpoint we are headed toward.
     * Let find if there is any street block that represents a valid
     * intersection street. We select the one that matches the best.
     */
    best_match = roadmap_fuzzy_false();
    square = roadmap_square_search (&crossing, 0);

    if (square != -1) {

	     roadmap_square_set_current (square);
        for (cfcc = ROADMAP_ROAD_FIRST; cfcc <= ROADMAP_ROAD_LAST; ++cfcc) {

            if (roadmap_line_in_square
                (square, cfcc, &first_line, &last_line) > 0) {

                int line;
                for (line = first_line; line <= last_line; ++line) {

                    PluginLine p_line;
                    p_line.plugin_id = ROADMAP_PLUGIN_ID;
                    p_line.line_id = line;
                    p_line.cfcc = cfcc;
                    p_line.square = square;
                    p_line.fips = fips;

                    if (roadmap_plugin_override_line (line, cfcc, fips)) {
                        continue;
                    }

                    match = roadmap_navigate_is_intersection
                                    (&p_line, position->steering, &crossing);
                    if (best_match < match) {

                        PluginStreet street;
                        roadmap_street_get_properties (line, &properties);

                        roadmap_plugin_set_street
                            (&street, ROADMAP_PLUGIN_ID, square, properties.street);

                        if ((properties.street > 0) &&
                           !roadmap_plugin_same_street
                              (&street,
                              &RoadMapConfirmedStreet.street)) {

                            *found = p_line;
                            best_match = match;
                        }
                    }
                }
            }
        }
    }

    /* search plugin lines */
    count = roadmap_plugin_find_connected_lines
                  (&crossing,
                   plugin_lines,
                   sizeof(plugin_lines)/sizeof(*plugin_lines));

    for (i=0; i<count; i++) {
        PluginStreet street;

        match = roadmap_navigate_is_intersection
                    (&plugin_lines[i], position->steering, &crossing);

        if (best_match < match) {

            roadmap_plugin_get_street (&plugin_lines[i], &street);

            if (!roadmap_plugin_same_street
                  (&street,
                   &RoadMapConfirmedStreet.street)) {

                *found = plugin_lines[i];
                best_match = match;
            }
        }
    }

    if (! roadmap_fuzzy_is_acceptable (best_match)) {
        roadmap_display_hide ("Current Street");
        return 0;
    }

    roadmap_navigate_trace
        ("Announce crossing %N, %C|Announce crossing %N", found);

    if (!roadmap_plugin_same_line
          (found, &RoadMapConfirmedStreet.intersection)) {
       RoadMapConfirmedStreet.intersection = *found;

       if (!RoadMapRouteInfo.enabled) {
         // roadmap_display_activate ("Approach", found, &crossing, &street);
       }
    }

    return 1;
}


static void roadmap_navigate_update_jammed_status (int gps_speed) {

	static int is_mobile = 0;

	if ((!is_mobile) && gps_speed >= RoadMapNavigateMinMobileSpeed) {
		roadmap_log(ROADMAP_INFO, "Hit mobility speed");
		is_mobile = 1;
	}

	if (is_mobile) {
		if (RoadMapNavigateJammedSince == 0 && gps_speed < RoadMapNavigateMaxJamSpeed) {
			RoadMapNavigateJammedSince = time (NULL);
			roadmap_log(ROADMAP_INFO, "Entering Jam");
		} else if (RoadMapNavigateJammedSince > 0 && gps_speed > RoadMapNavigateMaxJamSpeed) {
			RoadMapNavigateJammedSince = 0;
			roadmap_log(ROADMAP_INFO, "Leaving Jam");
		}
	}
}


int roadmap_navigate_is_jammed (PluginLine *line, int *direction) {

   PluginLine           currentLine;
   int                  currentDirection;
   RoadMapGpsPosition   pos_curr;
   RoadMapPosition      pos_end;
   int                  distance_to_end;

	if (!RoadMapNavigateJammedSince) {
      //roadmap_log (ROADMAP_DEBUG, "Not in jammed state -- not sending data to server");
      return 0;
   }

   if (-1 == roadmap_navigate_get_current (&pos_curr, &currentLine, &currentDirection)) {
      roadmap_log (ROADMAP_ERROR, "Don't have current location -- can't decide jammed state");
      return 0;
   }

   // check if we are close to segment's end point
   if (ROUTE_DIRECTION_AGAINST_LINE == currentDirection) {
      roadmap_street_extend_line_ends(&currentLine, &pos_end, NULL, FLAG_EXTEND_FROM, NULL, NULL);
   } else {
      roadmap_street_extend_line_ends(&currentLine, NULL, &pos_end, FLAG_EXTEND_TO, NULL, NULL);
   }
   distance_to_end = roadmap_math_distance((RoadMapPosition*)&pos_curr, &pos_end);
   roadmap_log (ROADMAP_DEBUG, "Jammed, distance to end of segment id=%d is %d:", currentLine.line_id, distance_to_end);
   *line = currentLine;
   *direction = currentDirection;
   return (distance_to_end > RoadMapNavigateMinJamDistanceFromEnd);
}



void roadmap_navigate_locate (const RoadMapGpsPosition *gps_position, time_t gps_time) {

   RoadMapNavCandidates candidates[ROADMAP_NEIGHBOURHOUD];
   BOOL candidate_in_route;
   BOOL current_in_route = FALSE;

	int i;
	int count;
	RoadMapPosition context_save_pos;
	zoom_t context_save_zoom;
	static RoadMapPosition last_azymuth_point;
   static time_t  first_speed_time = 0;

	RoadMapFuzzy result;
   static RoadMapFuzzy confidence = 0;
   static uint32_t confidence_time = 0;
   uint32_t now_ms = roadmap_time_get_millis();
   int confidence_delta = now_ms - confidence_time;

	static RoadMapTracking candidate;
   static time_t lost_route_time = 0;
   int old_direction = RoadMapConfirmedStreet.line_direction;

	zoom_t zoom = 20;

   if (confidence_delta > 1000)
      confidence_delta = 1000;
   
	// editor cleanup on first GPS point
	if (RoadMapLatestGpsTime == 0) {
		editor_cleanup_test( (int)(gps_time - 3 * 24 * 60 * 60), FALSE);
	}
   
   if (first_speed_time == 0 && gps_position->speed > 0) {
      first_speed_time = time (NULL);
   }
   
   RoadMapLatestGpsTime = gps_time;
   roadmap_navigate_update_jammed_status (gps_position->speed);

   roadmap_math_get_context (&context_save_pos, &context_save_zoom);

   if ( roadmap_screen_is_hd_screen() )
#ifndef IPHONE_NATIVE
      zoom *= 2;
#else
      zoom = 33; //TODO: check this logic
#endif //IPHONE_NATIVE
   
   roadmap_math_set_context ((RoadMapPosition *)gps_position, zoom);

	roadmap_square_request_location ((const RoadMapPosition *)gps_position);

   if (0 && (gps_position->speed < roadmap_gps_speed_accuracy()) &&
       first_speed_time > 0 &&
       (time(NULL) - first_speed_time > MIN_SPEED_INIT_TIME)) {
#ifdef DEBUG_PRINTS
      printf ("speed: %d\n", gps_position->speed);
#endif
      
      RoadMapLatestGpsPosition.speed = gps_position->speed;

      if ((RoadMapLatestGpsPosition.latitude == gps_position->latitude) &&
          (RoadMapLatestGpsPosition.longitude == gps_position->longitude))
         goto ret;

      /* Check if last gps position is valid */
      if (RoadMapLatestGpsPosition.steering == INVALID_STEERING) {
         RoadMapLatestGpsPosition = *gps_position;
         last_azymuth_point = *(RoadMapPosition*)gps_position;
         roadmap_navigate_set_mobile(gps_position, NULL, 0);
      } else {
         if ((time(NULL) - RoadMapLatestUpdate) >= 2) {
            int distance = roadmap_math_distance(
                                                 (RoadMapPosition *) &RoadMapLatestGpsPosition,
                                                 (RoadMapPosition *) gps_position);

            if (distance > 15) {
               if (RoadMapConfirmedStreet.valid) {
                  RoadMapNeighbour candidate;

                  if (!roadmap_plugin_get_distance
                      ((RoadMapPosition *)gps_position,
                       &RoadMapConfirmedLine.line,
                       &candidate) ||
                      !roadmap_plugin_same_line
                      (&candidate.line, &RoadMapConfirmedLine.line)) {
                     goto ret;
                  }

                  RoadMapLatestGpsPosition.longitude = gps_position->longitude;
                  RoadMapLatestGpsPosition.latitude = gps_position->latitude;
         			last_azymuth_point = *(RoadMapPosition*)gps_position;
                  
                  roadmap_navigate_set_mobile(&RoadMapLatestGpsPosition,
                                              &RoadMapConfirmedLine.intersection,
                                              roadmap_math_azymuth(&RoadMapConfirmedLine.from,
                                                                   &RoadMapConfirmedLine.to)
                                              );
                  
                  goto ret;

               } else {

                  int azymuth = roadmap_math_azymuth(
                                                     (RoadMapPosition *) &RoadMapLatestGpsPosition,
                                                     (RoadMapPosition *) gps_position);
                  RoadMapLatestGpsPosition = *gps_position;
         			last_azymuth_point = *(RoadMapPosition*)gps_position;
                  roadmap_navigate_set_mobile (gps_position,
                                                  (RoadMapPosition *)gps_position,
                                                  azymuth);
               }
            }
         }
      }

      goto ret;
   }


   if (RoadMapLatestGpsPosition.steering == INVALID_STEERING) {
      const RoadMapGpsPosition *gps = roadmap_trip_get_gps_position ("GPS");
      RoadMapPosition trip_gps_pos = *(RoadMapPosition*)gps;
      last_azymuth_point = *(RoadMapPosition*)gps_position;
      if (roadmap_math_distance(&last_azymuth_point, &trip_gps_pos) < 100)
         //For first tine, use the last session's steering. current steering is probably not correct
         RoadMapLatestGpsPosition.steering = gps->steering;
      else
         RoadMapLatestGpsPosition.steering = gps_position->steering;
   } else if (gps_position->speed > roadmap_gps_speed_accuracy()*2) {
      last_azymuth_point = *(RoadMapPosition*)&RoadMapLatestGpsPosition;
		RoadMapLatestGpsPosition.steering = gps_position->steering;
	} else if (gps_position->speed == 0) {
      last_azymuth_point = *(RoadMapPosition*)gps_position;
   } else if (roadmap_math_distance(
			   		(RoadMapPosition *) &last_azymuth_point,
			   		(RoadMapPosition *) gps_position) > 20) {
		RoadMapLatestGpsPosition.steering = roadmap_math_azymuth
               (&last_azymuth_point, (RoadMapPosition *) gps_position);
      last_azymuth_point = *(RoadMapPosition*)gps_position;
   }
   
   RoadMapLatestGpsPosition.longitude = gps_position->longitude;
   RoadMapLatestGpsPosition.latitude = gps_position->latitude;
   RoadMapLatestGpsPosition.altitude = gps_position->altitude;
   RoadMapLatestGpsPosition.speed = gps_position->speed;


   if (! RoadMapNavigateEnabled) {

      if (strcasecmp
          (roadmap_config_get(&RoadMapNavigateFlag), "yes") == 0) {
         RoadMapNavigateEnabled = 1;
      } else {
         roadmap_navigate_set_mobile(&RoadMapLatestGpsPosition, NULL, 0);
         goto ret;
      }
   }

   roadmap_adjust_position (&RoadMapLatestGpsPosition, &RoadMapLatestPosition);
#ifdef DEBUG_PRINTS
   roadmap_trip_set_mobile ("GPS_LOCATE", &RoadMapLatestGpsPosition); //AviR: debug
#endif

   if (RoadMapConfirmedStreet.valid) {

      /* We have an existing street match: check it is still valid. */

      RoadMapFuzzy before = RoadMapConfirmedStreet.cur_fuzzyfied;
      RoadMapFuzzy current_fuzzy;
      int previous_direction = RoadMapConfirmedStreet.line_direction;
      RoadMapNeighbour candidate;
      BOOL moving_away = FALSE;

      if (roadmap_plugin_activate_db
          (&RoadMapConfirmedLine.line) == -1) {
         goto ret;
      }
      
      if (RoadMapRouteInfo.enabled)
         current_in_route = RoadMapRouteInfo.callbacks.line_in_route(&RoadMapConfirmedLine.line,
                                                                     RoadMapConfirmedStreet.line_direction);

      if (!roadmap_plugin_get_distance
          (&RoadMapLatestPosition,
           &RoadMapConfirmedLine.line,
           &candidate) ||
          !roadmap_plugin_same_line (&candidate.line, &RoadMapConfirmedLine.line)) {

         current_fuzzy = roadmap_fuzzy_false();
      } else {
         if (RoadMapConfirmedLine.distance < candidate.distance)
            moving_away = TRUE;
			RoadMapConfirmedLine = candidate;
#ifdef DEBUG_PRINTS
         printf("new_distance: %d (%d)\n", candidate.distance, moving_away);
#endif
         current_fuzzy = roadmap_navigate_fuzzify_internal (&RoadMapConfirmedStreet,
                                                            &RoadMapConfirmedStreet,
                                                            &RoadMapConfirmedLine,
                                                            &RoadMapConfirmedLine,
                                                            0,
                                                            RoadMapLatestGpsPosition.steering,
                                                            confidence,
                                                            RoadMapLatestGpsPosition.speed);
      }
      
      RoadMapConfirmedStreet.cur_fuzzyfied = current_fuzzy;

      if ((previous_direction == RoadMapConfirmedStreet.line_direction) &&
          (current_fuzzy >= before) &&
          roadmap_fuzzy_is_good(current_fuzzy)) {
#ifdef DEBUG_PRINTS
         printf("same street current_fuzzy: %d %d/%d/%d\n", current_fuzzy,
                RoadMapConfirmedStreet.debug.distance, RoadMapConfirmedStreet.debug.direction, RoadMapConfirmedStreet.debug.connected);
#endif

         //RoadMapConfirmedStreet.cur_fuzzyfied = current_fuzzy;

         if (RoadMapRouteInfo.enabled) {

            RoadMapRouteInfo.callbacks.update
            (&RoadMapLatestPosition,
             &RoadMapConfirmedLine.line,
             RoadMapLatestGpsPosition.speed,
             TRUE);

         } else if (!roadmap_navigate_confirm_intersection (&RoadMapLatestGpsPosition)) {
            PluginLine p_line;
            roadmap_navigate_find_intersection (&RoadMapLatestGpsPosition, &p_line);
         }
         
         roadmap_navigate_set_mobile(&RoadMapLatestGpsPosition,
                                     &RoadMapConfirmedLine.intersection,
                                     roadmap_math_azymuth(&RoadMapConfirmedLine.from,
                                                          &RoadMapConfirmedLine.to)
                                     );
         
         if (confidence == roadmap_fuzzy_false()) {
            //confidence = roadmap_fuzzy_good();
            confidence = RoadMapConfirmedStreet.cur_fuzzyfied;
         } else if (confidence < roadmap_fuzzy_true() && !moving_away && roadmap_fuzzy_is_certain(current_fuzzy)){
            confidence = confidence * 140 / 100;
            if (confidence > roadmap_fuzzy_true())
               confidence = roadmap_fuzzy_true();
         }else if (0 && confidence > roadmap_fuzzy_acceptable() && moving_away){ //disabled
            confidence = confidence * 100 / 120;
            if (confidence < roadmap_fuzzy_acceptable() -1)
               confidence = roadmap_fuzzy_acceptable() -1;
         }
#ifdef DEBUG_PRINTS      
         printf("moving away: %d\n", moving_away);
#endif
         
         goto ret; /* We are on the same street. */
      }
   }

   /* We must search again for the best street match. */

#ifndef J2ME
   //FIXME remove when navigation will support plugin lines
   if (RoadMapRouteInfo.enabled) {
      editor_plugin_set_override (0);
   }
#endif
   count = roadmap_navigate_get_neighbours (&RoadMapLatestPosition, 0, roadmap_fuzzy_max_distance(),
                                            3, RoadMapNeighbourhood, ROADMAP_NEIGHBOURHOUD, LAYER_ALL_ROADS);
   
   //editor_screen_reset_selected();
   

   candidates[0].found = candidates[1].found = candidates[2].found = FALSE;
   for (i = 0; i < count; ++i) {
      //editor_screen_select_line(&RoadMapNeighbourhood[i].line);
      
      result = roadmap_navigate_fuzzify_internal (&candidate,
                                                  &RoadMapConfirmedStreet,
                                                  &RoadMapConfirmedLine,
                                                  RoadMapNeighbourhood+i,
                                                  0,
                                                  RoadMapLatestGpsPosition.steering,
                                                  confidence,
                                                  RoadMapLatestGpsPosition.speed);
      
      //printf("--- result: %d (%d) dir: %d dis: %d con: %d\n", result, RoadMapNeighbourhood[i].line.line_id,
      //       candidate.debug.direction, candidate.debug.distance, candidate.debug.connected);
      if (RoadMapRouteInfo.enabled &&
          RoadMapRouteInfo.callbacks.line_in_route(&RoadMapNeighbourhood[i].line,
                                                   candidate.line_direction)) {
             
             candidate_in_route = 1;
          } else {
             
             candidate_in_route = 0;
          }
      
      if (!candidates[0].found ||
          (result > candidates[0].result && (candidate_in_route == candidates[0].in_route || !roadmap_fuzzy_is_acceptable(confidence))) ||
          (!candidates[0].in_route && candidate_in_route && roadmap_fuzzy_is_acceptable (result) && roadmap_fuzzy_is_acceptable(confidence))) {
         if (!candidates[2].found ||
             candidates[1].result >= candidates[2].result)
            candidates[2] = candidates[1];
         if (!candidates[1].found ||
             candidates[0].result >= candidates[1].result)
            candidates[1] = candidates[0];
         candidates[0].found = TRUE;
         candidates[0].in_route = candidate_in_route;
         candidates[0].index = i;
         candidates[0].result = result;
         candidates[0].tracking = candidate;
      } else if (!candidates[1].found ||
                 result > candidates[1].result) {
         if (!candidates[2].found ||
             candidates[1].result >= candidates[2].result)
            candidates[2] = candidates[1];
         candidates[1].found = TRUE;
         candidates[1].in_route = candidate_in_route;
         candidates[1].index = i;
         candidates[1].result = result;
         candidates[1].tracking = candidate;
      } else if (!candidates[2].found ||
                 result > candidates[2].result) {
         candidates[2].found = TRUE;
         candidates[2].in_route = candidate_in_route;
         candidates[2].index = i;
         candidates[2].result = result;
         candidates[2].tracking = candidate;
      }

      
      //if ((result > best) ||
//          (!nominated_in_route &&
//           candidate_in_route &&
//           roadmap_fuzzy_is_acceptable (result))) {
//             
//             if (nominated_in_route && !candidate_in_route) {
//                /* Prefer one of the routing segments */
//                
//                if (result > second_best) {
//                   second_best = result;
//                   track_second_best = candidate;
//                }
//                continue;
//             }
//             
//             track_best = candidate;
//             alt_found = found;
//             found = i;
//             second_best = best;
//             track_second_best = nominated;
//             best = result;
//             nominated = candidate;
//             nominated_in_route = candidate_in_route;
//          } else if (result > second_best) {
//             track_second_best = candidate;
//             second_best = result;
//             alt_found =i;
//          }
      
   }
   //printf("###########################################\n");
   
#ifdef DEBUG_PRINTS
   
   printf("current: %d (%d)",
          RoadMapConfirmedStreet.cur_fuzzyfied, RoadMapConfirmedLine.line.line_id);
   
   if (candidates[0].found)
      printf("best: %d (%d) %d/%d/%d ",
             candidates[0].result, RoadMapNeighbourhood[candidates[0].index].line.line_id,
             candidates[0].tracking.debug.distance, candidates[0].tracking.debug.direction, candidates[0].tracking.debug.connected);
   
   if (candidates[1].found)
      printf("second: %d (%d) %d/%d/%d ",
             candidates[1].result, RoadMapNeighbourhood[candidates[1].index].line.line_id,
             candidates[1].tracking.debug.distance, candidates[1].tracking.debug.direction, candidates[1].tracking.debug.connected);
   
   if (candidates[2].found)
      printf("third: %d (%d) %d/%d/%d ",
             candidates[2].result, RoadMapNeighbourhood[candidates[2].index].line.line_id,
             candidates[2].tracking.debug.distance, candidates[2].tracking.debug.direction, candidates[2].tracking.debug.connected);
   
   printf("\n");
#endif
   
   
#ifndef J2ME
   //FIXME remove when navigation will support plugin lines
   if (RoadMapRouteInfo.enabled) {
      editor_plugin_set_override (1);
   }
#endif
   
   if (RoadMapConfirmedStreet.valid && editor_ignore_new_roads()) {
      if ((!candidates[0].found || !roadmap_fuzzy_is_acceptable(candidates[0].result)  &&
           (roadmap_fuzzy_false() != roadmap_fuzzy_or(RoadMapConfirmedStreet.cur_fuzzyfied, confidence))) ||
          ((!candidates[0].in_route && current_in_route)/* && RoadMapConfirmedStreet.cur_fuzzyfied != roadmap_fuzzy_false()*/ &&
           (roadmap_fuzzy_is_good(roadmap_fuzzy_and(RoadMapConfirmedStreet.cur_fuzzyfied, confidence)) ||
            (roadmap_fuzzy_is_acceptable(confidence) && !roadmap_fuzzy_is_good(candidates[0].tracking.debug.connected)) ||
            (roadmap_fuzzy_is_good(confidence) && RoadMapConfirmedLine.line.cfcc < ROADMAP_ROAD_RAMP))))
      {
         /* we don't have a good alternative and current is confident */
         RoadMapNeighbour candidate;
         
         if (roadmap_plugin_get_distance
             (&RoadMapLatestPosition,
              &RoadMapConfirmedLine.line,
              &candidate)) {
                RoadMapConfirmedLine = candidate;
                
                roadmap_navigate_set_mobile(&RoadMapLatestGpsPosition,
                                            &RoadMapConfirmedLine.intersection,
                                            roadmap_math_azymuth(&RoadMapConfirmedLine.from,
                                                                 &RoadMapConfirmedLine.to)
                                            );
                
                if (confidence > roadmap_fuzzy_acceptable() -1){
                   confidence = confidence * 100 / 130; //decrease confidence
                   if (confidence < roadmap_fuzzy_acceptable() -1)
                      confidence = roadmap_fuzzy_acceptable() -1;
                }
                
#ifdef DEBUG_PRINTS
                printf("confidence decreased - maybe not required !!\n");
#endif
                goto ret; /* We are on the same street. */
             }
      }
   }
   
   if ((RoadMapConfirmedStreet.valid && candidates[0].found && roadmap_fuzzy_is_acceptable (candidates[0].result)) ||
       (!RoadMapConfirmedStreet.valid && candidates[0].found && roadmap_fuzzy_is_certain (candidates[0].result))){
      
      /* found alternative street */
      
      PluginLine old_line = RoadMapConfirmedLine.line;
      RoadMapTracking nominated;
      int nominated_index;
      int nominated_result;
      
      
      if (roadmap_plugin_activate_db
          (&RoadMapNeighbourhood[candidates[0].index].line) == -1) {
         goto ret;
      }
      
      nominated = candidates[0].tracking;
      nominated_index = candidates[0].index;
      nominated_result = candidates[0].result;
      
      if (confidence == roadmap_fuzzy_false()) {
         if (RoadMapConfirmedStreet.valid)
            confidence = candidates[0].result;
         else
            confidence = candidates[0].result/4;
#ifdef DEBUG_PRINTS
         printf("confidence reset\n");
#endif
      } else if (//!roadmap_fuzzy_is_certain(candidates[0].result) &&
                 candidates[1].found && roadmap_fuzzy_is_good(candidates[1].result) ||
                 candidates[1].found && candidates[1].result - candidates[0].result > roadmap_fuzzy_true()/10) {
         if (roadmap_fuzzy_is_certain(candidates[1].result))
            confidence = confidence * 100 / 140;
         else
            confidence = confidence * 100 / 120;
         if (confidence < roadmap_fuzzy_acceptable() -1)
            confidence = roadmap_fuzzy_acceptable() -1;
#ifdef DEBUG_PRINTS
         printf("confidence decreased !\n");
#endif
      } else if (roadmap_fuzzy_is_certain(candidates[0].result) &&
                 (!candidates[1].found || !roadmap_fuzzy_is_good(candidates[1].result))) {
         confidence = confidence * 120 / 100;
         if (confidence > roadmap_fuzzy_true())
            confidence = roadmap_fuzzy_true();
#ifdef DEBUG_PRINTS
         printf("confidence increased !\n");
#endif
      }
      
      
     // if (!roadmap_plugin_same_line (&RoadMapConfirmedLine.line, &RoadMapNeighbourhood[candidates[0].index].line) &&
//          !roadmap_fuzzy_is_certain(candidates[0].result) ||
//          confidence == roadmap_fuzzy_false()) {
//         confidence = candidates[0].result;
//         printf("confidence reset\n");
//      } else if (roadmap_plugin_same_line
//                 (&RoadMapConfirmedLine.line, &RoadMapNeighbourhood[candidates[0].index].line) &&
//                 RoadMapConfirmedLine.from.latitude == RoadMapNeighbourhood[candidates[0].index].from.latitude &&
//                 RoadMapConfirmedLine.from.longitude == RoadMapNeighbourhood[candidates[0].index].from.longitude &&
//                 RoadMapConfirmedLine.to.latitude == RoadMapNeighbourhood[candidates[0].index].to.latitude &&
//                 RoadMapConfirmedLine.to.longitude == RoadMapNeighbourhood[candidates[0].index].to.longitude) {
//         /* same line, if got far reduce confidence */
//         if (confidence > roadmap_fuzzy_acceptable() -1 &&
//             
//         /* && 
//             RoadMapConfirmedLine.distance < RoadMapNeighbourhood[candidates[0].index].distance*/){
//            confidence = confidence * 100 / (100 + (now_ms - confidence_time)/10);
//            if (confidence < roadmap_fuzzy_acceptable() -1)
//               confidence = roadmap_fuzzy_acceptable() -1;
//            printf("confidence reduced !\n");
//         }
//      }
      
#ifdef DEBUG_PRINTS
      printf("changed line from: %d to: %d\n", RoadMapConfirmedLine.line.line_id, RoadMapNeighbourhood[nominated_index].line.line_id);
#endif

      //handle some special cases:
      //
      //special case: lost route
      if (current_in_route && !candidates[0].in_route) {
         if (time(NULL) - lost_route_time < 30) {
            //It is better to reset the location based only on distance and direction if we recalc too often
            int candidate_0_confidence = 0;
            int candidate_1_confidence = 0;
            int candidate_2_confidence = 0;
            
            candidate_0_confidence = roadmap_fuzzy_and(candidates[0].tracking.debug.distance, candidates[0].tracking.debug.direction);
            
            if (candidates[1].found)
               candidate_1_confidence = roadmap_fuzzy_and(candidates[1].tracking.debug.distance, candidates[1].tracking.debug.direction);
            
            if (candidates[2].found)
               candidate_2_confidence = roadmap_fuzzy_and(candidates[2].tracking.debug.distance, candidates[2].tracking.debug.direction);
                
            
            if (candidate_1_confidence > candidate_0_confidence &&
                candidate_1_confidence > candidate_2_confidence) {
               nominated = candidates[1].tracking;
               nominated_index = candidates[1].index;
               nominated_result = candidates[1].result;
               
               confidence = candidate_1_confidence;
#ifdef DEBUG_PRINTS
               printf("selected candidate 1 & confidence reset due to multiple reroutes\n");
#endif
            } else if (candidate_2_confidence > candidate_0_confidence &&
                candidate_2_confidence > candidate_1_confidence) {
               nominated = candidates[2].tracking;
               nominated_index = candidates[2].index;
               nominated_result = candidates[2].result;
               
               confidence = candidate_2_confidence;
#ifdef DEBUG_PRINTS
               printf("selected candidate 2 & confidence reset due to multiple reroutes\n");
#endif
            }
            
            lost_route_time = 0;
         } else {
            lost_route_time = time(NULL);
         }
      }
      
      //special case: first line - prefer major road
      if (!RoadMapConfirmedStreet.valid && !candidates[0].in_route &&
          RoadMapNeighbourhood[candidates[0].index].line.cfcc >= ROADMAP_ROAD_RAMP) {
         if (RoadMapNeighbourhood[candidates[1].index].line.cfcc < ROADMAP_ROAD_RAMP &&
             roadmap_fuzzy_is_good(candidates[1].result) &&
             RoadMapLatestGpsPosition.speed > roadmap_gps_speed_accuracy()) {
            nominated = candidates[1].tracking;
            nominated_index = candidates[1].index;
            nominated_result = candidates[1].result;
            
            confidence = nominated_result;
         }
      }


      RoadMapConfirmedLine   = RoadMapNeighbourhood[nominated_index];
      RoadMapConfirmedStreet = nominated;

      RoadMapConfirmedStreet.valid = 1;
      RoadMapConfirmedStreet.cur_fuzzyfied = nominated_result;
      INVALIDATE_PLUGIN(RoadMapConfirmedStreet.intersection);



      roadmap_display_activate ("Current Street",
                                &RoadMapConfirmedLine.line,
                                NULL,
                                &RoadMapConfirmedStreet.street);
      
      if (old_direction != RoadMapConfirmedStreet.line_direction ||
            !roadmap_plugin_same_line(&RoadMapConfirmedLine.line, &old_line)) {
         //AviR: is entry_fuzzyfied used anywhere?
         RoadMapConfirmedStreet.entry_fuzzyfied = nominated_result;

         if (PLUGIN_VALID(old_line)) {
            roadmap_navigate_trace ("Quit street %N", &old_line);
         }
         roadmap_navigate_trace ("Enter street %N, %C|Enter street %N",
                                 &RoadMapConfirmedLine.line);
         
         on_segment_changed_inform_clients(&RoadMapConfirmedLine.line, RoadMapConfirmedStreet.line_direction);

         roadmap_display_hide ("Approach");

         if (RoadMapRouteInfo.enabled) {

            RoadMapRouteInfo.current_line = RoadMapConfirmedLine.line;

            RoadMapRouteInfo.callbacks.get_next_line
            (&RoadMapConfirmedLine.line,
             RoadMapConfirmedStreet.line_direction,
             &RoadMapRouteInfo.next_line);
         }
      }

      if (!RoadMapRouteInfo.enabled) {
         if (RoadMapLatestGpsPosition.speed > roadmap_gps_speed_accuracy()) {
            PluginLine p_line;
            roadmap_navigate_find_intersection (&RoadMapLatestGpsPosition, &p_line);
         }
      }

      if (1 || candidates[0].in_route || 
          !roadmap_fuzzy_is_certain(candidates[1].result) ||
          roadmap_plugin_same_line (&old_line, &RoadMapNeighbourhood[candidates[1].index].line)) {
         roadmap_navigate_set_mobile(&RoadMapLatestGpsPosition,
                                     &RoadMapConfirmedLine.intersection,
                                     roadmap_math_azymuth(&RoadMapConfirmedLine.from,
                                                          &RoadMapConfirmedLine.to)
                                     );
      } else {
         const RoadMapGpsPosition *gps = roadmap_trip_get_gps_position ("GPS");
         RoadMapPosition avg_point = RoadMapConfirmedLine.intersection;
         int steering = RoadMapLatestGpsPosition.steering;

         avg_point.longitude = (avg_point.longitude + RoadMapNeighbourhood[candidates[1].index].intersection.longitude) / 2;
         avg_point.latitude = (avg_point.latitude + RoadMapNeighbourhood[candidates[1].index].intersection.latitude) / 2;

         if (gps) steering = gps->steering;
         roadmap_navigate_set_mobile(&RoadMapLatestGpsPosition, &avg_point, steering);
      }

   } else {
      confidence = roadmap_fuzzy_false();
      if (PLUGIN_VALID(RoadMapConfirmedLine.line)) {

         if (roadmap_plugin_activate_db
             (&RoadMapConfirmedLine.line) == -1)
            goto ret;

         roadmap_navigate_trace ("Lost street %N",
                                 &RoadMapConfirmedLine.line);
         roadmap_display_hide ("Current Street");
         roadmap_display_hide ("Approach");
      }

      INVALIDATE_PLUGIN(RoadMapConfirmedLine.line);
      RoadMapConfirmedStreet.valid = 0;

      roadmap_navigate_set_mobile(&RoadMapLatestGpsPosition, NULL, 0);
   }

   if (RoadMapRouteInfo.enabled) {

      RoadMapRouteInfo.callbacks.update
      (&RoadMapLatestPosition,
       &RoadMapConfirmedLine.line,
       RoadMapLatestGpsPosition.speed,
       TRUE);
   }

ret:
#ifdef DEBUG_PRINTS
   printf("confidence: %d\n==========\n", confidence);
#endif
   confidence_time = now_ms;
   roadmap_math_set_context (&context_save_pos, context_save_zoom);
}

static void animation_set_callback (void *context) {
   int i;
   RoadMapNeighbour candidate;;
   RoadMapPosition position;
   RoadMapAnimation *animation = (RoadMapAnimation *)context;
   RoadMapGpsPosition pos = *roadmap_trip_get_gps_position("GPS");
   
   if (strcmp(animation->object_id, GPS_OBJECT)) {
      roadmap_log (ROADMAP_WARNING, "animation_set_callback() - unknown object '%s'", animation->object_id);
      return;
   }
   
   for (i = 0; i < animation->properties_count; i++) {
      switch (animation->properties[i].type) {
         case ANIMATION_PROPERTY_ROTATION:
            pos.steering = animation->properties[i].current;
            break;
         case ANIMATION_PROPERTY_SPEED:
            pos.speed = animation->properties[i].current;
            break;
         case ANIMATION_PROPERTY_POSITION_X:
            pos.longitude = animation->properties[i].current;
            break;
         case ANIMATION_PROPERTY_POSITION_Y:
            pos.latitude = animation->properties[i].current;
            break;
         default:
            break;
      }
   }   
   
   position.longitude = pos.longitude;
   position.latitude = pos.latitude;
   
   if (RoadMapConfirmedStreet.valid &&
       roadmap_plugin_get_distance
       (&position,
        &RoadMapConfirmedLine.line,
        &candidate) &&
       roadmap_plugin_same_line (&candidate.line, &RoadMapConfirmedLine.line)) {
      pos.longitude = candidate.intersection.longitude;
      pos.latitude = candidate.intersection.latitude;
   }
      
   roadmap_trip_set_mobile ("GPS", &pos);
}

static void animation_ended_callback (void *context) {
}


void roadmap_navigate_check_alerts (RoadMapGpsPosition *gps_position) {


   if (RoadMapConfirmedLine.line.line_id == -1)
      return;

	roadmap_alerter_check (gps_position, &RoadMapConfirmedLine.line);
}


void roadmap_navigate_initialize (void) {

    roadmap_fuzzy_reset_cycle ();

    roadmap_config_declare_enumeration
        ("session", &RoadMapNavigateFlag, NULL, "yes", "no", NULL);
    roadmap_config_declare
        ("preferences", &RoadMapNavigateMinMobileSpeedCfg, "20", NULL);
    roadmap_config_declare
        ("preferences", &RoadMapNavigateMaxJamSpeedCfg, "10", NULL);
    roadmap_config_declare
        ("preferences", &RoadMapNavigateMinJamDistanceFromEndCfg, "150", NULL);

	RoadMapNavigateMinMobileSpeed = roadmap_config_get_integer (&RoadMapNavigateMinMobileSpeedCfg);
	RoadMapNavigateMaxJamSpeed = roadmap_config_get_integer (&RoadMapNavigateMaxJamSpeedCfg);
   RoadMapNavigateMinJamDistanceFromEnd = roadmap_config_get_integer (&RoadMapNavigateMinJamDistanceFromEndCfg);
}


void roadmap_navigate_route (RoadMapNavigateRouteCB callbacks) {


    static int inside_route = 0;

    if (inside_route) {
        roadmap_log (ROADMAP_ERROR, "recursive call to roadmap_navigate_route");
        return; /* avoid recursion */
    }
    inside_route = 1;

   RoadMapRouteInfo.callbacks = callbacks;

   if (RoadMapConfirmedStreet.valid) {

      RoadMapRouteInfo.current_line = RoadMapConfirmedLine.line;

      callbacks.get_next_line (&RoadMapConfirmedLine.line,
                               RoadMapConfirmedStreet.line_direction,
                               &RoadMapRouteInfo.next_line);

      callbacks.update (&RoadMapLatestPosition, &RoadMapConfirmedLine.line, RoadMapLatestGpsPosition.speed, TRUE);
   }

   RoadMapRouteInfo.enabled = 1;
   RoadMapRouteInfo.initialized = 1;

   inside_route = 0;
}


void roadmap_navigate_end_route (void) {

   RoadMapRouteInfo.enabled = 0;
}


void roadmap_navigate_resume_route (void) {

	if (RoadMapRouteInfo.initialized)
   	RoadMapRouteInfo.enabled = 1;
}


int roadmap_navigate_get_current (RoadMapGpsPosition *position,
                                  PluginLine *line,
                                  int *direction) {

   if (position) *position = RoadMapLatestGpsPosition;

   if (!line || !direction) return 0;

   if (RoadMapConfirmedStreet.valid) {

      *line = RoadMapConfirmedLine.line;
      *direction = RoadMapConfirmedStreet.line_direction;
      return 0;
   } else {

      line->plugin_id = -1;
      *direction = ROUTE_DIRECTION_NONE;
      return -1;
   }
}


time_t roadmap_navigate_get_time (void) {

	if (RoadMapLatestGpsTime) return RoadMapLatestGpsTime;
	return time (NULL);
}
