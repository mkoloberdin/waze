/* roadmap_crossing.c - manage the roadmap Intersection dialogs.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
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
 *   See roadmap_crossing.h
 */

#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_math.h"
#include "roadmap_config.h"
#include "roadmap_gui.h"
#include "roadmap_street.h"
#include "roadmap_county.h"
#include "roadmap_locator.h"
#include "roadmap_trip.h"
#include "roadmap_screen.h"
#include "roadmap_messagebox.h"
#include "roadmap_dialog.h"
#include "roadmap_display.h"
#include "roadmap_history.h"
#include "roadmap_preferences.h"

#include "roadmap_crossing.h"


#define ROADMAP_MAX_STREETS  256


typedef struct {

    int line1;
    int line2;
    RoadMapPosition position;

} RoadMapCrossingSelection;

static RoadMapStreetIntersection *RoadMapCrossingSelections = NULL;

static void *RoadMapCrossingHistory;


static void roadmap_crossing_done (RoadMapStreetIntersection *selected) {

    if (roadmap_locator_activate (selected->fips) == ROADMAP_US_OK) {

        PluginLine line;
        PluginStreet street;

        roadmap_plugin_set_line
           (&line, ROADMAP_PLUGIN_ID, selected->line1, -1, selected->square, selected->fips);
        roadmap_display_activate
           ("Selected Street", &line, &selected->position, &street);
        roadmap_trip_set_point ("Selection", &selected->position);
        roadmap_trip_set_point ("Address", &selected->position);
        roadmap_trip_set_focus ("Address");
        roadmap_screen_refresh ();
    }
}


static void roadmap_crossing_selected (const char *name, void *data) {

   RoadMapStreetIntersection *selected;

   selected =
      (RoadMapStreetIntersection *)
           roadmap_dialog_get_data ("List", ".Intersections");

   if (selected != NULL) {
      roadmap_crossing_done (selected);
   }
}


static void roadmap_crossing_selection_ok (const char *name, void *data) {

   roadmap_dialog_hide (name);
   roadmap_crossing_selected (name, data);

   if (RoadMapCrossingSelections != NULL) {
      free (RoadMapCrossingSelections);
      RoadMapCrossingSelections = NULL;
   }
}


static void roadmap_crossing_set (void) {

   char *argv[3];

   roadmap_history_get ('I', RoadMapCrossingHistory, argv);

   roadmap_dialog_set_data ("Crossing", "Street 1:", argv[0]);
   roadmap_dialog_set_data ("Crossing", "Street 2:", argv[1]);
   roadmap_dialog_set_data ("Crossing", "State:", argv[2]);
}


static void roadmap_crossing_before (const char *name, void *data) {

   RoadMapCrossingHistory =
      roadmap_history_before ('I', RoadMapCrossingHistory);

   roadmap_crossing_set ();
}


static void roadmap_crossing_after (const char *name, void *data) {

   RoadMapCrossingHistory =
      roadmap_history_after ('I', RoadMapCrossingHistory);

   roadmap_crossing_set ();
}


static void roadmap_crossing_ok (const char *name, void *data) {

   int i;
   int count;

   void *list[ROADMAP_MAX_STREETS];
   char *names[ROADMAP_MAX_STREETS];

   char *street1_name;
   char *street2_name;
   char *state_name;

   const char *argv[3];

   //RoadMapString state;


   street1_name   = (char *) roadmap_dialog_get_data ("Crossing", "Street 1:");
   street2_name   = (char *) roadmap_dialog_get_data ("Crossing", "Street 2:");
   state_name     = (char *) roadmap_dialog_get_data ("Crossing", "State:");

/*
   state = roadmap_locator_get_state (state_name);
   if (state == ROADMAP_INVALID_STRING) {
       roadmap_messagebox ("Warning", "unknown state");
       return;
   }
*/

   if (RoadMapCrossingSelections == NULL) {
       RoadMapCrossingSelections = (RoadMapStreetIntersection *)
          calloc (ROADMAP_MAX_STREETS, sizeof(RoadMapStreetIntersection));
   }

   count = roadmap_street_intersection (state_name, street1_name, street2_name,
                                        RoadMapCrossingSelections,
                                        ROADMAP_MAX_STREETS);

   if (count <= 0) {
       roadmap_messagebox ("Warning", "Could not find any intersection");
       return;
   }

   argv[0] = street1_name;
   argv[1] = street2_name;
   argv[2] = state_name;

   roadmap_history_add ('I', argv);

   roadmap_dialog_hide (name);


   if (count == 1) {
       roadmap_crossing_done (RoadMapCrossingSelections);
       return;
   }


   /* We found several solutions: tell the user to choose one. */

   for (i = count-1; i >= 0; --i) {

       char buffer[128];
       RoadMapStreetProperties properties;
       RoadMapStreetIntersection *intersection;


       intersection = RoadMapCrossingSelections + i;

       if (roadmap_locator_activate (intersection->fips) != ROADMAP_US_OK) {
           continue;
       }
       roadmap_street_get_properties (intersection->line1, &properties);

       sprintf (buffer, "%s (%s County)",
                roadmap_street_get_city_name (&properties),
                roadmap_county_get_name (intersection->fips));

       list[i] = intersection;
       names[i] = strdup(buffer);
   }

   if (roadmap_dialog_activate ("Choose Intersection", NULL, 1)) {

      roadmap_dialog_new_list ("List", ".Intersections");

      roadmap_dialog_add_button ("OK", roadmap_crossing_selection_ok);

      roadmap_dialog_complete (0); /* No need for a keyboard. */
   }

   roadmap_dialog_show_list
      ("List", ".Intersections", count, names, list, roadmap_crossing_selected);

   for (i = count-1; i >= 0; --i) {
      free (names[i]);
   }
}


static void roadmap_crossing_cancel (const char *name, void *data) {

   roadmap_dialog_hide (name);
}


void roadmap_crossing_dialog (void) {

   if (roadmap_dialog_activate ("Intersection", NULL, 1)) {

      roadmap_dialog_new_entry ("Crossing", "Street 1:", NULL);
      roadmap_dialog_new_entry ("Crossing", "Street 2:", NULL);
      roadmap_dialog_new_entry ("Crossing", "State:", NULL);

      roadmap_dialog_add_button ("Back", roadmap_crossing_before);
      roadmap_dialog_add_button ("Next", roadmap_crossing_after);
      roadmap_dialog_add_button ("OK", roadmap_crossing_ok);
      roadmap_dialog_add_button ("Cancel", roadmap_crossing_cancel);

      roadmap_dialog_complete (roadmap_preferences_use_keyboard());

      roadmap_history_declare ('I', 3);
   }

   RoadMapCrossingHistory = roadmap_history_latest ('I');

   roadmap_crossing_set ();
}

