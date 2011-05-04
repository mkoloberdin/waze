/* roadmap_trip.c - Manage a trip: destination & waypoints.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
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
 *   See roadmap_trip.h.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_string.h"
#include "roadmap_time.h"
#include "roadmap_path.h"
#include "roadmap_file.h"
#include "roadmap_object.h"
#include "roadmap_gui.h"
#include "roadmap_math.h"
#include "roadmap_config.h"
#include "roadmap_dialog.h"
#include "roadmap_fileselection.h"
#include "roadmap_canvas.h"
#include "roadmap_sprite.h"
#include "roadmap_screen.h"
#include "roadmap_state.h"
#include "roadmap_message.h"
#include "roadmap_messagebox.h"
#include "roadmap_preferences.h"
#include "roadmap_display.h"
#include "roadmap_adjust.h"
#include "roadmap_res.h"
#include "roadmap_sunrise.h"
#include "roadmap_screen.h"
#include "editor/editor_screen.h"

#include "roadmap_trip.h"

static RoadMapConfigDescriptor RoadMapConfigTripName =
                        ROADMAP_CONFIG_ITEM("Trip", "Name");

static RoadMapConfigDescriptor RoadMapConfigTripRotate =
                        ROADMAP_CONFIG_ITEM("Display", "Rotate");

static RoadMapConfigDescriptor RoadMapConfigFocusName =
                        ROADMAP_CONFIG_ITEM("Focus", "Name");

static RoadMapConfigDescriptor RoadMapConfigFocusRotate =
                        ROADMAP_CONFIG_ITEM("Focus", "Rotate");

static RoadMapConfigDescriptor RoadMapConfigCarName =
                        ROADMAP_CONFIG_ITEM("Trip", "Car");



/* Default location is: Kikar Ha-Medina, Tel Aviv, Israel. */
#define ROADMAP_DEFAULT_POSITION "34794810, 32106010"
#define DEFAULT_CAR_NAME         "car_blue"

static int RoadMapTripRotate   = 1;
static int RoadMapTripModified = 0; /* List needs to be saved ? */
static int RoadMapTripRefresh  = 1; /* Screen needs to be refreshed ? */
static int RoadMapTripFocusChanged = 1;
static int RoadMapTripFocusMoved   = 1;

static RoadMapCallback RoadMapTripNextMessageUpdate;

typedef struct roadmap_trip_point {

    RoadMapListItem link;

    char *id;
    char *sprite;
    char *image;

    char predefined;
    char mobile;
    char in_trip;
    char has_value;

    RoadMapPosition    map;
    RoadMapGpsPosition gps;

    RoadMapConfigDescriptor config_position;
    RoadMapConfigDescriptor config_direction;

    int distance;   /* .. from the destination point. */

    int from_node_id;		// Start of the line segment
    int to_node_id;			// End of the line segment
    BOOL is_object;
} RoadMapTripPoint;


#define ROADMAP_TRIP_ITEM(id,sprite,image, mobile,in_trip, has_value) \
    {{NULL, NULL}, id, sprite, image, 1, mobile, in_trip, has_value, \
     {0, 0}, \
     ROADMAP_GPS_NULL_POSITION, \
     ROADMAP_CONFIG_ITEM(id,"Position"), \
     ROADMAP_CONFIG_ITEM(id,"Direction"), \
     0, \
     -1, -1, FALSE \
    }

RoadMapTripPoint RoadMapTripPredefined[] = {
    ROADMAP_TRIP_ITEM("GPS",         "GPS", NULL,         1, 0, 1),
    ROADMAP_TRIP_ITEM("Destination", "Destination", "Destination", 0, 1, 0),
    ROADMAP_TRIP_ITEM("Departure", "Departure", "Departure", 1, 1, 0),
    ROADMAP_TRIP_ITEM("Address",     NULL,    NULL,       0, 0, 1),
    ROADMAP_TRIP_ITEM("Selection",   "Selection",  "Selection",  0, 0, 1),
    ROADMAP_TRIP_ITEM("Hold",        NULL,    NULL,       1, 0, 0),
    ROADMAP_TRIP_ITEM("Location",   "Location",  "location",    0, 0, 1),
    ROADMAP_TRIP_ITEM("GPS_LOCATE",   "GPS_LOCATE",  NULL,    0, 0, 1),
    ROADMAP_TRIP_ITEM("ORIG_GPS",   "ORIG_GPS",  NULL,    0, 0, 1),
    ROADMAP_TRIP_ITEM("Marked_Location", "Selection",    "mark_location_pin",       0, 1, 0),
    ROADMAP_TRIP_ITEM("Alt-Routes",        NULL,    NULL,       1, 0, 0),
    ROADMAP_TRIP_ITEM(NULL, NULL, NULL, 0, 0, 0)
};


static RoadMapTripPoint *RoadMapTripGps = NULL;
static RoadMapTripPoint *RoadMapTripFocus = NULL;
static RoadMapTripPoint *RoadMapTripDeparture = NULL;
static RoadMapTripPoint *RoadMapTripDestination = NULL;
static RoadMapTripPoint *RoadMapTripNextWaypoint = NULL;

static RoadMapList RoadMapTripWaypoints;

static RoadMapPosition RoadMapTripLastPosition;


static void roadmap_trip_unfocus (void) {

    if (RoadMapTripFocus != NULL) {

        RoadMapTripLastPosition = RoadMapTripFocus->map;
        RoadMapTripFocus = NULL;
    }
    RoadMapTripDeparture = NULL;
    RoadMapTripDestination = NULL;
}


static RoadMapTripPoint *roadmap_trip_search (const char *name) {

    RoadMapListItem *item, *tmp;
    RoadMapTripPoint *trip;

    ROADMAP_LIST_FOR_EACH (&RoadMapTripWaypoints, item, tmp) {

       trip = (RoadMapTripPoint *)item;

       if (strcmp (trip->id, name) == 0) {
          return trip;
       }
    }
    return NULL;
}


static void roadmap_trip_coordinate (const RoadMapPosition *position, RoadMapGuiPoint *point) {

    if (roadmap_math_point_is_visible (position)) {
        roadmap_math_coordinate (position, point);
    } else {
        point->x = 32767;
        point->y = 32767;
    }
}

static void roadmap_trip_set_nodes( RoadMapTripPoint *trip_point, int from_node, int to_node )
{
	if ( trip_point )
	{
		trip_point->from_node_id = from_node;

		trip_point->to_node_id = to_node;
	}

}


static RoadMapTripPoint *roadmap_trip_update
                            (const char *name,
                             const RoadMapPosition *position,
                             const RoadMapGpsPosition *gps_position,
                             const char *sprite,
                             const char *image) {

    RoadMapTripPoint *result;


    if (strchr (name, ',') != NULL) {

        /* Because we use a 'simplified' CSV format, we cannot have
         * commas in the name.
         */
        char *to;
        char *from;
        char *cleaned = strdup (name);

        for (from = cleaned, to = cleaned; *from != 0; ++from) {
            if (*from != ',') {
                if (to != from) {
                    *to = *from;
                }
                ++to;
            }
        }
        *to = 0;

        result = roadmap_trip_update (cleaned, position, gps_position, sprite, image);
        free (cleaned);

        return result;
    }

    result = roadmap_trip_search (name);
    if (result && result->is_object){
       RoadMapDynamicString GUI_ID = roadmap_string_new( result->id);
       roadmap_object_remove(GUI_ID);
       roadmap_string_release( GUI_ID);
       result->is_object = FALSE;
    }

    if (result == NULL) {

        /* A new point: refresh is needed only if this point
         * is visible.
         */
        result = malloc (sizeof(RoadMapTripPoint));
        roadmap_check_allocated(result);

        result->id = strdup(name);
        roadmap_check_allocated(result->id);

        result->predefined = 0;
        result->in_trip = 1;
        result->sprite = strdup(sprite);
        result->mobile = (gps_position != NULL);
        if (image != NULL)
        	result->image = strdup(image);
        else
        	result->image = NULL;

        if (roadmap_math_point_is_visible (position)) {
            RoadMapTripRefresh = 1;
        }

        roadmap_list_append (&RoadMapTripWaypoints, &result->link);
        RoadMapTripModified = 1;
    } else {

        /* An existing point: refresh is needed only if the point
         * moved in a visible fashion.
         */
        RoadMapGuiPoint point1;
        RoadMapGuiPoint point2;

        if (result->map.latitude != position->latitude ||
            result->map.longitude != position->longitude) {
            roadmap_trip_coordinate (position, &point1);
            roadmap_trip_coordinate (&result->map, &point2);
            if (point1.x != point2.x || point1.y != point2.y) {
                RoadMapTripRefresh = 1;
            }
            RoadMapTripModified = 1;

            if (result == RoadMapTripFocus) {
                RoadMapTripFocusMoved = 1;
            }
            result->distance = 0;
        }
    }

    result->map = *position;
    result->has_value = 1;

    // Nodes should be updated explicitly
    roadmap_trip_set_nodes( result, -1, -1 );

    if (gps_position != NULL) {

       result->gps = *gps_position;

    } else {

       result->gps.speed = 0;
       result->gps.steering = 0;
    }

    if (result->predefined && (! result->in_trip)) {

       /* Save the new position in the session context. */

       if (result->mobile) {

          RoadMapPosition temporary;

          temporary.longitude = result->gps.longitude;
          temporary.latitude  = result->gps.latitude;

          roadmap_config_set_position
             (&result->config_position, &temporary);

          roadmap_config_set_integer
             (&result->config_direction, result->gps.steering);

       } else {

          roadmap_config_set_position
             (&result->config_position, &result->map);
       }
    }

    return result;
}

/* TODO remove the dialog stuff to another file */
#ifndef J2ME
static void roadmap_trip_dialog_cancel (const char *name, void *data) {

    roadmap_dialog_hide (name);
}


static void roadmap_trip_file_dialog_ok (const char *filename, const char *mode) {

    if (mode[0] == 'w') {
        roadmap_trip_save (filename);
    } else {
        roadmap_trip_load (filename, 0);
    }
}


static void roadmap_trip_file_dialog (const char *mode) {

    roadmap_fileselection_new ("RoadMap Trip",
                                NULL, /* no filter. */
                                roadmap_path_trips(),
                                mode,
                                roadmap_trip_file_dialog_ok);
}


static void roadmap_trip_set_dialog_ok (const char *name, void *data) {

    char *point_name = (char *) roadmap_dialog_get_data ("Name", "Name:");

    if (point_name[0] != 0) {
        roadmap_trip_set_point (point_name, (RoadMapPosition *)data);
        roadmap_dialog_hide (name);
    }
}


static void roadmap_trip_set_dialog (const RoadMapPosition *position) {

    static RoadMapPosition point_position;

    point_position = *position;

    if (roadmap_dialog_activate ("Add Waypoint", &point_position, 1)) {

        roadmap_dialog_new_entry  ("Name", "Name:", NULL);
        roadmap_dialog_add_button ("OK", roadmap_trip_set_dialog_ok);
        roadmap_dialog_add_button ("Cancel", roadmap_trip_dialog_cancel);

        roadmap_dialog_complete (roadmap_preferences_use_keyboard());
    }
}


static void roadmap_trip_remove_dialog_populate (int count);

static void roadmap_trip_remove_dialog_delete (const char *name, void *data) {

    int count;
    char *point_name = (char *) roadmap_dialog_get_data ("Names", ".Waypoints");

    if (point_name && (point_name[0] != 0)) {

        roadmap_trip_remove_point (point_name);

        count = roadmap_list_count (&RoadMapTripWaypoints);
        if (count > 0) {
            roadmap_trip_remove_dialog_populate (count);
        } else {
            roadmap_dialog_hide (name);
        }
    }
}


static void roadmap_trip_remove_dialog_selected (const char *name, void *data) {

   char *point_name = (char *) roadmap_dialog_get_data ("Names", ".Waypoints");

   if (point_name != NULL) {

      roadmap_trip_set_focus (point_name);
      roadmap_screen_refresh ();
   }
}


static void roadmap_trip_remove_dialog_populate (int count) {

    static char **Names = NULL;

    int i;
    RoadMapListItem *item, *tmp;
    RoadMapTripPoint *point;

    if (Names != NULL) {
       free (Names);
    }
    Names = calloc (count, sizeof(*Names));
    roadmap_check_allocated(Names);

    i=0;
    ROADMAP_LIST_FOR_EACH (&RoadMapTripWaypoints, item, tmp) {
        point = (RoadMapTripPoint *)item;
        if (! point->predefined) {
            Names[i++] = point->id;
        }
    }

    roadmap_dialog_show_list
        ("Names", ".Waypoints", i, Names, (void **)Names,
         roadmap_trip_remove_dialog_selected);
}


static void roadmap_trip_remove_dialog (void) {

    int count;

    count = roadmap_list_count (&RoadMapTripWaypoints);
    if (count <= 0) {
        return; /* Nothing to delete. */
    }

    if (roadmap_dialog_activate ("Delete Waypoints", NULL, 1)) {

        roadmap_dialog_new_list   ("Names", ".Waypoints");
        roadmap_dialog_add_button ("Delete", roadmap_trip_remove_dialog_delete);
        roadmap_dialog_add_button ("Done", roadmap_trip_dialog_cancel);

        roadmap_dialog_complete (0); /* No need for a keyboard. */
    }

    roadmap_trip_remove_dialog_populate (count);
}


static FILE *roadmap_trip_fopen (const char *name, const char *mode) {

    FILE *file;

    if (name == NULL) {
        roadmap_trip_file_dialog (mode);
        return NULL;
    }

    if (roadmap_path_is_full_path (name)) {
        file = roadmap_file_fopen (NULL, name, mode);
    } else {
        file = roadmap_file_fopen (roadmap_path_trips(), name, mode);
    }

    if (file != NULL) {
        roadmap_config_set (&RoadMapConfigTripName, name);
    }
    return file;
}
#endif


static void roadmap_trip_set_point_focus (RoadMapTripPoint *point) {

    int rotate;

    if (point == NULL) return;

    if (! point->mobile) {

        rotate = 0; /* Fixed point, no rotation no matter what. */
        RoadMapTripRefresh = 1;
        RoadMapTripFocusChanged = 1;

    } else {

       rotate = roadmap_config_match (&RoadMapConfigTripRotate, "yes");
    }


    if (RoadMapTripRotate != rotate) {
        roadmap_config_set_integer (&RoadMapConfigFocusRotate, rotate);
        RoadMapTripRotate = rotate;
        RoadMapTripRefresh = 1;
    }
    if (RoadMapTripFocus != point) {
        roadmap_config_set (&RoadMapConfigFocusName, point->id);
        RoadMapTripFocus = point;
        RoadMapTripRefresh = 1;
        RoadMapTripFocusChanged = 1;
        roadmap_display_page (point->id);
    }
    if (point != RoadMapTripGps && RoadMapTripDestination != NULL) {

        /* Force the refresh when we know the trip information
         * must disappears from the screen.
         */
        RoadMapTripRefresh = 1;
    }
}


static void roadmap_trip_activate (void) {

    RoadMapTripPoint *destination;
    RoadMapTripPoint *waypoint;
    RoadMapListItem *item, *tmp;

    destination = RoadMapTripDestination;
    if (destination == NULL) return;

    /* Compute the distances to the destination. */

    ROADMAP_LIST_FOR_EACH (&RoadMapTripWaypoints, item, tmp) {

       waypoint = (RoadMapTripPoint *)item;
       if (! waypoint->predefined) {

          waypoint->distance =
             roadmap_math_distance (&destination->map, &waypoint->map);

          roadmap_log (ROADMAP_DEBUG,
                "Waypoint %s: distance to destination = %d %s",
                waypoint->id,
                waypoint->distance,
                roadmap_math_distance_unit());
       }
    }
    destination->distance = 0;

    roadmap_trip_set_focus ("GPS");
    roadmap_screen_redraw ();
}


static void roadmap_trip_clear (void) {

    RoadMapTripPoint *point;
    RoadMapListItem *item, *tmp;

    ROADMAP_LIST_FOR_EACH (&RoadMapTripWaypoints, item, tmp) {

        point = (RoadMapTripPoint *)item;

        if (! point->predefined) {

            if (RoadMapTripFocus == point) {
                roadmap_trip_unfocus ();
            }
            if (roadmap_math_point_is_visible (&point->map)) {
                RoadMapTripRefresh = 1;
            }
            roadmap_list_remove (item);
            free (point->id);
            free (point->sprite);
            free(point);
            RoadMapTripModified = 1;

        } else {

            point->has_value = 0;
        }
    }

    if (RoadMapTripModified) {
        roadmap_config_set (&RoadMapConfigTripName, "default");
    }
}


static void roadmap_trip_format_messages (void) {

    int distance_to_destination;
    int distance_to_destination_far;
    RoadMapTripPoint *gps = RoadMapTripGps;
    RoadMapTripPoint *waypoint;
    RoadMapListItem *item, *tmp;


    if (RoadMapTripFocus == gps &&
        RoadMapTripDestination != NULL &&
        RoadMapTripDestination->has_value) {

        time_t now = time(NULL);
        time_t sun;

        roadmap_message_set ('T', roadmap_time_get_hours_minutes(now));

        distance_to_destination =
            roadmap_math_distance (&gps->map, &RoadMapTripDestination->map);

        roadmap_log (ROADMAP_DEBUG,
                        "GPS: distance to destination = %d %s",
                        distance_to_destination,
                        roadmap_math_distance_unit());

        distance_to_destination_far =
           roadmap_math_to_trip_distance(distance_to_destination);

        if (distance_to_destination_far > 0) {
           roadmap_message_set ('D', "%d %s",
                                distance_to_destination_far,
                                roadmap_math_trip_unit());
        } else {
           roadmap_message_set ('D', "%d %s",
                                roadmap_math_distance_to_current(distance_to_destination),
                                roadmap_math_distance_unit());
        };


        RoadMapTripNextWaypoint = RoadMapTripDestination;

        ROADMAP_LIST_FOR_EACH (&RoadMapTripWaypoints, item, tmp) {

            waypoint = (RoadMapTripPoint *)item;

            if (waypoint->in_trip && waypoint != RoadMapTripDeparture) {
                if ((waypoint->distance < distance_to_destination) &&
                    (waypoint->distance > RoadMapTripNextWaypoint->distance)) {
                    RoadMapTripNextWaypoint = waypoint;
                }
            }
        }

        if (RoadMapTripNextWaypoint != RoadMapTripDestination) {

            int distance_to_waypoint =
                    roadmap_math_distance (&gps->map,
                                           &RoadMapTripNextWaypoint->map);

            roadmap_log (ROADMAP_DEBUG,
                            "GPS: distance to next waypoint %s = %d %s",
                            RoadMapTripNextWaypoint->id,
                            distance_to_waypoint,
                            roadmap_math_distance_unit());

            roadmap_message_set ('W', "%d %s",
                                 roadmap_math_to_trip_distance
                                            (distance_to_waypoint),
                                 roadmap_math_trip_unit());

        } else {
            roadmap_message_unset ('W');
        }

        roadmap_message_set ('S', "%3d %s",
                             roadmap_math_to_speed_unit(gps->gps.speed),
                             roadmap_math_speed_unit());

        roadmap_message_set ('H', "%d %s",
                             gps->gps.altitude,
                             roadmap_math_distance_unit());

        sun = roadmap_sunset (&gps->gps, time(NULL));
        if (sun > now) {

           roadmap_message_unset ('M');

           roadmap_message_set ('E', roadmap_time_get_hours_minutes(sun));

        } else {

           roadmap_message_unset ('E');

           sun = roadmap_sunrise (&gps->gps, time(NULL));
           roadmap_message_set ('M', roadmap_time_get_hours_minutes(sun));
        }

    } else {

        RoadMapTripNextWaypoint = NULL;

        roadmap_message_unset ('D');
        roadmap_message_unset ('S');
        roadmap_message_unset ('T');
        roadmap_message_unset ('W');

        roadmap_message_unset ('M');
        roadmap_message_unset ('E');
    }

    (*RoadMapTripNextMessageUpdate) ();
}


int roadmap_trip_gps_state (void) {

   if (!RoadMapTripGps->has_value ||
         (roadmap_trip_get_focus_name () == RoadMapTripGps->id)) {

      return TRIP_FOCUS_GPS;
   }

   return TRIP_FOCUS_NO_GPS;
}


void roadmap_trip_set_point (const char *name,
                             const RoadMapPosition *position) {

    if (name == NULL) {
#ifndef J2ME
        roadmap_trip_set_dialog (position);
#endif
    } else {
        roadmap_trip_update (name, position, NULL, "Waypoint", NULL);
    }
}


void roadmap_trip_set_selection_as (const char *name) {

   static RoadMapTripPoint *Selection = NULL;

   if (Selection == NULL) {
      Selection = roadmap_trip_search ("Selection");
   }

   roadmap_trip_update (name, &Selection->map, NULL, "Waypoint", NULL);
}


void roadmap_trip_set_mobile (const char *name,
                              const RoadMapGpsPosition *gps_position) {

    RoadMapPosition position;

    roadmap_adjust_position (gps_position, &position);

    roadmap_trip_update (name, &position, gps_position, "Mobile", NULL);
}

void roadmap_trip_set_gps_position (const char *name, const char*sprite, const char* image,
                              const RoadMapGpsPosition *gps_position) {

    RoadMapPosition position;

    roadmap_adjust_position (gps_position, &position);

    roadmap_trip_update (name, &position, gps_position, sprite, image);
}


void roadmap_trip_set_gps_and_nodes_position (const char *name, const char*sprite, const char* image,
                              const RoadMapGpsPosition *gps_position, int from_node, int to_node ) {

    RoadMapPosition position;
    RoadMapTripPoint *trip_point;

    roadmap_adjust_position (gps_position, &position);

    trip_point = roadmap_trip_update (name, &position, gps_position, sprite, image);

    roadmap_trip_set_nodes( trip_point, from_node, to_node );

}

void roadmap_trip_set_animation(const char *name, int animation_type){
//#ifdef OPENGL
   RoadMapTripPoint *trip = roadmap_trip_search (name);

    if (trip && trip->image){
       RoadMapGpsPosition position;
       RoadMapGuiPoint      Offset ={0,0};
       RoadMapDynamicString Image = roadmap_string_new( trip->image);
       RoadMapDynamicString GUI_ID = roadmap_string_new( trip->id);
       RoadMapDynamicString Group = roadmap_string_new( "Trips");
       RoadMapDynamicString Name = roadmap_string_new( trip->id);

       position.latitude = trip->map.latitude;
       position.longitude = trip->map.longitude;

       if (roadmap_object_exists(GUI_ID))
         roadmap_object_remove(GUI_ID);
       trip->is_object = TRUE;
       roadmap_object_add_with_priority( Group, GUI_ID, Name, NULL, Image, &position, &Offset,
                         animation_type,
                          NULL, OBJECT_PRIORITY_HIGHEST);
       roadmap_string_release( Group);
       roadmap_string_release( Name);
       roadmap_string_release( Image);
       roadmap_string_release( GUI_ID);
   }
//#endif
}

void  roadmap_trip_copy_focus (const char *name) {

   RoadMapTripPoint *to = roadmap_trip_search (name);

   if (to != NULL &&
         RoadMapTripFocus != NULL && to != RoadMapTripFocus) {

      to->has_value = RoadMapTripFocus->has_value;

      to->map = RoadMapTripFocus->map;
      to->gps = RoadMapTripFocus->gps;

      if (to->predefined && (! to->in_trip)) {
         roadmap_config_set_position (&to->config_position, &to->map);
         roadmap_config_set_integer (&to->config_direction, to->gps.steering);
      }
   }
}


void roadmap_trip_remove_point (const char *name) {

    RoadMapTripPoint *result;

#ifndef J2ME
    if (name == NULL) {
        roadmap_trip_remove_dialog ();
        return;
    }
#endif

    result = roadmap_trip_search (name);

    if (result->is_object){
       RoadMapDynamicString GUI_ID = roadmap_string_new( result->id);
       roadmap_object_remove(GUI_ID);
       roadmap_string_release( GUI_ID);
    }

    if (result == NULL) {
        roadmap_log (ROADMAP_ERROR, "cannot delete: point %s not found", name);
        return;
    }
    if (result->predefined) {
        result->has_value = 0;
        result->is_object = 0;
        return;
    }

    if (RoadMapTripFocus == result ||
        RoadMapTripDeparture == result ||
        RoadMapTripDestination == result) {
        roadmap_trip_unfocus ();
    }

    if (roadmap_math_point_is_visible (&result->map)) {
        RoadMapTripRefresh = 1;
    }

    roadmap_list_remove (&result->link);
    free (result->id);
    free (result->sprite);
    free(result);

    RoadMapTripModified = 1;

    roadmap_screen_refresh();
}


void  roadmap_trip_restore_focus (void) {

    int i;
    RoadMapTripPoint *focus;
    RoadMapListItem *item, *tmp;
    RoadMapTripPoint *trip;

    ROADMAP_LIST_FOR_EACH (&RoadMapTripWaypoints, item, tmp) {

       trip = (RoadMapTripPoint *)item;

       if (trip->is_object) {
          RoadMapDynamicString GUI_ID = roadmap_string_new( trip->id);
          roadmap_object_remove(GUI_ID);
          roadmap_string_release( GUI_ID);
		   roadmap_screen_refresh();
       }
    }

    for (i = 0; RoadMapTripPredefined[i].id != NULL; ++i) {

        roadmap_list_append
            (&RoadMapTripWaypoints, &RoadMapTripPredefined[i].link);

        if (! RoadMapTripPredefined[i].in_trip) {

            roadmap_config_get_position
                (&RoadMapTripPredefined[i].config_position,
                 &RoadMapTripPredefined[i].map);

            if (RoadMapTripPredefined[i].mobile) {

                /* This is a mobile point: what was recorded was the GPS
                 * position, not the adjusted map position.
                 */
                RoadMapTripPredefined[i].gps.longitude =
                   RoadMapTripPredefined[i].map.longitude;

                RoadMapTripPredefined[i].gps.latitude =
                   RoadMapTripPredefined[i].map.latitude;

                RoadMapTripPredefined[i].gps.steering =
                    roadmap_config_get_integer
                        (&RoadMapTripPredefined[i].config_direction);

                roadmap_adjust_position (&RoadMapTripPredefined[i].gps,
                                         &RoadMapTripPredefined[i].map);
            }
        }
    }
    RoadMapTripGps = roadmap_trip_search ("GPS");

    focus = roadmap_trip_search (roadmap_config_get (&RoadMapConfigFocusName));
    if (focus == NULL) {
        focus = RoadMapTripGps;
    }

    roadmap_trip_set_point_focus (focus);
    RoadMapTripFocusChanged = 1;
}


void roadmap_trip_set_focus (const char *name) {

    RoadMapTripPoint *point = roadmap_trip_search (name);

    if (point == NULL) {
        roadmap_log
            (ROADMAP_ERROR, "cannot activate: point %s not found", name);
        return;
    }
//AVI
    //if (point->has_value)
    roadmap_trip_set_point_focus (point);
}

int roadmap_trip_is_focus_changed (void) {

    if (RoadMapTripFocusChanged) {

        RoadMapTripFocusChanged = 0;
        return 1;
    }
    return 0;
}

int roadmap_trip_is_focus_moved (void) {

    if (RoadMapTripFocusMoved) {

        RoadMapTripFocusMoved = 0;
        return 1;
    }
    return 0;
}

int roadmap_trip_is_refresh_needed (void) {

    if (RoadMapTripRefresh ||
        RoadMapTripFocusChanged || RoadMapTripFocusMoved) {

        RoadMapTripFocusChanged = 0;
        RoadMapTripFocusMoved = 0;
        RoadMapTripRefresh = 0;

        return 1;
    }
    return 0;
}


int roadmap_trip_get_orientation (void) {

    if (RoadMapTripRotate && (RoadMapTripFocus != NULL)) {
        return 360 - RoadMapTripFocus->gps.steering;
    }

    return 0;
}


const char *roadmap_trip_get_focus_name (void) {

    if (RoadMapTripFocus != NULL) {
        return RoadMapTripFocus->id;
    }

    return NULL;
}


const RoadMapPosition *roadmap_trip_get_focus_position (void) {

    if (RoadMapTripFocus != NULL) {
        return &RoadMapTripFocus->map;
    }

    return &RoadMapTripLastPosition;
}


const RoadMapPosition *roadmap_trip_get_position (const char *name) {

   RoadMapTripPoint *trip = NULL;

   trip = roadmap_trip_search (name);
   if (trip == NULL) return NULL;

   return &trip->map;
}

const RoadMapGpsPosition *roadmap_trip_get_gps_position(const char *name){

   RoadMapTripPoint *trip = NULL;

   trip = roadmap_trip_search (name);
   if (trip == NULL) return NULL;

   return &trip->gps;
}

void roadmap_trip_get_nodes(const char *name, int *from_node, int *to_node )
{

   RoadMapTripPoint *trip = NULL;
   *from_node = -1;
   *to_node = -1;

   trip = roadmap_trip_search (name);
   if (trip == NULL) return;

   *from_node = trip->from_node_id;
   *to_node = trip->to_node_id;
}


void  roadmap_trip_start (void) {

    RoadMapTripDestination = roadmap_trip_search ("Destination");

    if (RoadMapTripDestination != NULL) {

       roadmap_trip_set_point ("Departure", &RoadMapTripGps->map);
       RoadMapTripDeparture = roadmap_trip_search ("Departure");

       roadmap_trip_activate ();
    }
}


void roadmap_trip_resume (void) {

    RoadMapTripDeparture = roadmap_trip_search ("Departure");

    if (RoadMapTripDeparture != NULL) {

       RoadMapTripDestination = roadmap_trip_search ("Destination");

       if (RoadMapTripDestination != NULL) {
          roadmap_trip_activate ();
       }
    }
}


void roadmap_trip_reverse (void) {

    RoadMapTripDestination = roadmap_trip_search ("Departure");

    if (RoadMapTripDestination != NULL) {

       RoadMapTripDeparture = roadmap_trip_search ("Destination");

       if (RoadMapTripDeparture != NULL) {
          roadmap_trip_activate ();
       } else {
          RoadMapTripDestination = NULL;
       }
    }
}


void roadmap_trip_stop (void) {

    RoadMapTripDestination = NULL;
    RoadMapTripRefresh = 1;
    roadmap_screen_redraw();
}


void roadmap_trip_display (void) {

    int azymuth;
    RoadMapGuiPoint point;
    RoadMapTripPoint *gps = RoadMapTripGps;
    RoadMapTripPoint *waypoint;
    RoadMapListItem *item, *tmp;
    const char *focus = roadmap_trip_get_focus_name ();
   static BOOL no_3d = FALSE;

    ROADMAP_LIST_FOR_EACH (&RoadMapTripWaypoints, item, tmp) {
        waypoint = (RoadMapTripPoint *)item;

        if (waypoint->sprite == NULL) continue;
        if (! waypoint->has_value) continue;

        if (roadmap_math_point_is_visible (&waypoint->map)) {
            roadmap_math_coordinate (&waypoint->map, &point);
            roadmap_math_rotate_project_coordinate (&point);

            if ((focus != NULL) && ((!strcmp(waypoint->sprite,"GPS") &&
            		!strcmp (focus, "GPS") && (roadmap_screen_get_orientation_mode() != ORIENTATION_FIXED) ))) {
                RoadMapImage image;
                RoadMapGuiPoint screen_point;
                char *car_name;
                const char *config_car;

                roadmap_math_coordinate ((RoadMapPosition *)&waypoint->gps, &screen_point);

                roadmap_math_rotate_project_coordinate (&screen_point);
                config_car = roadmap_config_get (&RoadMapConfigCarName);

                if (config_car[0] != 0){
                	car_name = editor_screen_overide_car();

                	if (car_name == NULL){
                     char car_3d[70];
                	   car_name = roadmap_path_join("cars", config_car);
                     if (roadmap_screen_get_view_mode() == VIEW_MODE_3D &&
                         !no_3d) {
                        snprintf (car_3d, sizeof(car_3d), "%s_3D", car_name);
                        image = (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN, car_3d);
                        if (image == NULL)
                           no_3d = TRUE;// We will try only once in a session
                     } else {
                        image = NULL;
                     }
                     
                     if (!image)
                        image =  (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN, car_name);
                     
                    roadmap_path_free(car_name);
                	}
                	else
                     image =  (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN, car_name);

                    if (image){
                        screen_point.x -= roadmap_canvas_image_width(image)/2;
                        screen_point.y -= roadmap_canvas_image_height(image)/2;
                        roadmap_canvas_draw_image_angle (image, &screen_point,  0, roadmap_math_get_orientation() + waypoint->gps.steering ,IMAGE_NORMAL);
                    } else
                        roadmap_sprite_draw (waypoint->sprite, &point, waypoint->gps.steering);
                } else
                    roadmap_sprite_draw (waypoint->sprite, &point, waypoint->gps.steering);
            }else{
               BOOL is_location = !strcmp( waypoint->sprite,"Location" );
               if ((waypoint->image != NULL) && !(waypoint->is_object) && (  !is_location || ( is_location && focus && !strcmp( focus, "Location" ) ) )){
                    RoadMapImage image;
                    image =  (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN, waypoint->image);
                    if (image != NULL){

                        point.x -= roadmap_canvas_image_width(image)/2;
                        point.y -= roadmap_canvas_image_height(image)/2;
                        roadmap_canvas_draw_image (image, &point,  0, IMAGE_NORMAL);
                    }
                    else
                        roadmap_sprite_draw
                            (waypoint->sprite, &point, waypoint->gps.steering);
                }
                else
                {
                	if (  !is_location || ( is_location && focus && !strcmp( focus, "Location" ) ) )
                	{
                     if ((waypoint->image == NULL) && !(waypoint->is_object))
                        roadmap_sprite_draw
                           (waypoint->sprite, &point, waypoint->gps.steering);
                	}
                }
            }
        }
    }

    if (RoadMapTripNextWaypoint != NULL) {

        azymuth = roadmap_math_azymuth (&gps->map,
                                        &RoadMapTripNextWaypoint->map);
        roadmap_math_coordinate (&gps->map, &point);
        roadmap_math_rotate_project_coordinate (&point);
        roadmap_sprite_draw ("Direction", &point, azymuth);
    }
}


const char *roadmap_trip_current (void) {
    return roadmap_config_get (&RoadMapConfigTripName);
}


void roadmap_trip_new (void) {

    roadmap_trip_clear ();
    roadmap_config_set (&RoadMapConfigTripName, "default");
    RoadMapTripModified = 1;

    roadmap_screen_refresh();
}


void roadmap_trip_initialize (void) {

    int i;

    ROADMAP_LIST_INIT (&RoadMapTripWaypoints);

    for (i = 0; RoadMapTripPredefined[i].id != NULL; ++i) {

        if (! RoadMapTripPredefined[i].in_trip) {

            roadmap_config_declare
                ("session",
                 &RoadMapTripPredefined[i].config_position,
                 ROADMAP_DEFAULT_POSITION, NULL);

            if (RoadMapTripPredefined[i].mobile) {
                roadmap_config_declare
                    ("session",
                     &RoadMapTripPredefined[i].config_direction, "0", NULL);
            }
        }
        if (strcmp (RoadMapTripPredefined[i].id, "GPS") == 0) {
            RoadMapTripGps = &(RoadMapTripPredefined[i]);
        }
    }
    roadmap_config_declare
        ("session", &RoadMapConfigTripName, "default", NULL);
    roadmap_config_declare_enumeration
        ("preferences", &RoadMapConfigTripRotate, NULL, "yes", "no", NULL);
    roadmap_config_declare
        ("session", &RoadMapConfigFocusName, "GPS", NULL);
    roadmap_config_declare
        ("session", &RoadMapConfigFocusRotate, "1", NULL);
    roadmap_config_declare
    	("user", &RoadMapConfigCarName, DEFAULT_CAR_NAME, NULL);

    RoadMapTripNextMessageUpdate =
       roadmap_message_register (roadmap_trip_format_messages);

    roadmap_state_add ("GPS_focus", roadmap_trip_gps_state);
}


#ifndef J2ME
int roadmap_trip_load (const char *name, int silent) {

    FILE *file;
    int   i;
    char *p;
    char *argv[8];
    char  line[1024];

    RoadMapPosition position;


    /* Load waypoints from the user environment. */

    file = roadmap_trip_fopen (name, silent ? "sr" : "r");

    if (file == NULL) {
       if (name == NULL) {
          return 1; /* Delay the answer: file selection has been activated. */
       }
       if (!silent) {
          roadmap_messagebox ("Warning",
                "An error occurred when opening the selected trip");
       }
       return 0;
    }

    roadmap_trip_clear ();

    while (! feof(file)) {

       if (fgets (line, sizeof(line), file) == NULL) break;

       p = roadmap_config_extract_data (line, sizeof(line));

       if (p == NULL) continue;

       argv[0] = p;

       for (p = strchr (p, ','), i = 0; p != NULL && i < 8; p = strchr (p, ',')) {
           *p = 0;
           argv[++i] = ++p;
       }

       if (i != 3) {
           roadmap_log (ROADMAP_ERROR, "erroneous trip line (%d fields)", i);
           continue;
       }
       position.longitude = atoi(argv[2]);
       position.latitude  = atoi(argv[3]);
       roadmap_trip_update (argv[1], &position, NULL, argv[0], NULL);
    }

    fclose (file);
    RoadMapTripModified = 0;

    roadmap_screen_refresh();
    return 1;
}


static void roadmap_trip_printf (FILE *file, const RoadMapTripPoint *point) {

   fprintf (file, "%s,%s,%d,%d\n",
         point->sprite,
         point->id,
         point->map.longitude,
         point->map.latitude);
}

void roadmap_trip_save (const char *name) {

    RoadMapTripPoint *point;
    RoadMapListItem *item, *tmp;


    /* Save all existing points, if the list was ever modified, or if
     * the user wants to specify another name.
     */
    if (name == NULL) {
        RoadMapTripModified = 1;
    }

    if (RoadMapTripModified) {

        FILE *file;

        file = roadmap_trip_fopen (name, "w");

        if (file == NULL) {
            return;
        }


        ROADMAP_LIST_FOR_EACH (&RoadMapTripWaypoints, item, tmp) {
            point = (RoadMapTripPoint *)item;

            if (point->in_trip && point->has_value && (! point->mobile)) {
                roadmap_trip_printf (file, point);
            }
        }

        fclose (file);
        RoadMapTripModified = 0;
    }
}


void roadmap_trip_save_screenshot (void) {

   const char *extension = ".png";
   const char *trip_path = roadmap_path_trips();
   const char *trip_name = roadmap_trip_current();

   unsigned int total_length;
   unsigned int trip_length;
   char *dot;
   char *picture_name;

   trip_length = strlen(trip_name); /* Shorten it later. */

   dot = strrchr( trip_name, '.' );
   if (dot != NULL) {
      trip_length -= strlen(dot);
   }

   total_length = trip_length + strlen(trip_path) + strlen(extension) + 12;
   picture_name = malloc (total_length);
   roadmap_check_allocated(picture_name);

   memcpy (picture_name, trip_name, trip_length);
   sprintf (picture_name, "%s/%*.*s-%010d%s",
            trip_path,
            trip_length, trip_length, trip_name,
            (int)time(NULL),
            extension);

   roadmap_canvas_save_screenshot (picture_name);
   free (picture_name);
}
#endif


