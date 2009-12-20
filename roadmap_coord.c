/* roadmap_coord.c - manage the roadmap Coordinates dialog.
 *
 * LICENSE:
 *
 *   Copyright 2003 Latchesar Ionkov
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
 *   See roadmap_coord.h
 */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

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

#include "roadmap_coord.h"


static void *RoadMapCoordHistory;

static const char RoadMapLongitudeLabel[] = "Longitude (ISO 6709 format):";
static const char RoadMapLatitudeLabel[]  = "Latitude (ISO 6709 format):";


static int roadmap_coord_fraction (const char *image) {

   int length;
   char normalized[16];

   if (image[0] == 0) return 0;

   strncpy (normalized, image, 6);
   normalized[6] = 0;

   length = strlen(normalized);
   if (isalpha(normalized[length-1])) {
      normalized[length-1] = 0;
   }

   strncat (normalized, "000000", 7);
   normalized[6] = 0;

   return atoi(normalized);
}

static int roadmap_coord_to_binary (const char *image, int is_longitude) {

   int value;
   int sign = 1;
   int length;

   const char *p;


   /* This function supports the following formats:
    *
    * Sdd[d].ddd     (ISO 6709-1983, decimal degree format).
    * Sdd[d]mm.mmm   (ISO 6709-1983, degree/minute format).
    * Sdd[d]mmss.sss (ISO 6709-1983, degree/minute/second format).
    *
    * RoadMap is more lenient than the ISO standard: the degrees
    * do not need a leading zero and 'E', 'W', 'S' and 'N' can be
    * used as a prefix or suffix in addition to '+' and '-'.
    */
   switch (*image)
   {
      case '-':
         sign = -1;
         ++image;
         break;
      case '+':
         ++image;
         break;
      case 'W':
      case 'w':
         if (! is_longitude) return 181000000; /* Invalid on purpose. */
         sign = -1;
         ++image;
         break;
      case 'E':
      case 'e':
         if (! is_longitude) return 181000000; /* Invalid on purpose. */
         ++image;
         break;
      case 'S':
      case 's':
         if (is_longitude) return 181000000; /* Invalid on purpose. */
         sign = -1;
         ++image;
         break;
      case 'N':
      case 'n':
         if (is_longitude) return 181000000; /* Invalid on purpose. */
         ++image;
         break;
   }
   switch (image[strlen(image)-1])
   {
      case 'W':
      case 'w':
         if (! is_longitude) return 181000000; /* Invalid on purpose. */
         sign = -1;
         break;
      case 'E':
      case 'e':
         if (! is_longitude) return 181000000; /* Invalid on purpose. */
         sign = 1;
         break;
      case 'S':
      case 's':
         if (is_longitude) return 181000000; /* Invalid on purpose. */
         sign = -1;
         break;
      case 'N':
      case 'n':
         if (is_longitude) return 181000000; /* Invalid on purpose. */
         sign = 1;
         break;
   }

   p = strchr (image, '.');
   if (p == NULL) {
      length = strlen(image);
      p = image + length;
   } else {
      length = p - image;
      ++p;
   }
   value = atoi(image);

   switch (length) {

      case 3:

         if (! is_longitude) return 181000000; /* Invalid on purpose. */
         /* Otherwise same as 1 & 2.. */

      case 1:
      case 2:

         value = (value * 1000000) + roadmap_coord_fraction(p);
         break;

      case 5:

         if (! is_longitude) return 181000000; /* Invalid on purpose. */
         /* Otherwise same as 4.. */

      case 4:

         value = ((value / 100) * 1000000)                     /* degrees */
                    + (((value % 100) * 1000000) / 60)         /* minutes */
                    + (roadmap_coord_fraction(p) / 60);
         break;

      case 7:

         if (! is_longitude) return 181000000; /* Invalid on purpose. */
         /* Otherwise same as 6.. */

      case 6:

         value = ((value / 10000) * 1000000)                   /* degrees */
                    + ((((value / 100) % 100) * 1000000) / 60) /* minutes */
                    + (((value % 100) * 1000000) / 3600)       /* Seconds */
                    + (roadmap_coord_fraction(p) / 3600);
         break;

      default: return 181000000; /* Invalid on purpose. */
   }

   return sign * value;
}


static void roadmap_coord_set (void) {

   char *argv[2];

   roadmap_history_get ('C', RoadMapCoordHistory, argv);

   roadmap_dialog_set_data ("Coordinates", RoadMapLongitudeLabel, argv[0]);
   roadmap_dialog_set_data ("Coordinates", RoadMapLatitudeLabel, argv[1]);
}


static void roadmap_coord_before (const char *name, void *data) {

   RoadMapCoordHistory = roadmap_history_before ('C', RoadMapCoordHistory);

   roadmap_coord_set ();
}


static void roadmap_coord_after (const char *name, void *data) {

   RoadMapCoordHistory = roadmap_history_after ('C', RoadMapCoordHistory);

   roadmap_coord_set ();
}



static void roadmap_coord_ok (const char *name, void *data) {

   const char *argv[2];

   RoadMapPosition position;


   argv[0] = (const char*) roadmap_dialog_get_data
                              ("Coordinates", RoadMapLongitudeLabel);
   position.longitude = roadmap_coord_to_binary (argv[0], 1);
   if (position.longitude > 180000000 || position.longitude < -180000000) {
      roadmap_messagebox("Warning", "Invalid longitude value");
      return;
   }

   argv[1] = (const char*) roadmap_dialog_get_data
                              ("Coordinates", RoadMapLatitudeLabel);
   position.latitude = roadmap_coord_to_binary (argv[1], 0);
   if (position.latitude > 90000000 || position.latitude < -90000000) {
      roadmap_messagebox("Warning", "Invalid latitude value");
      return;
   }

   roadmap_trip_set_point ("Selection", &position);
   roadmap_trip_set_point ("Address", &position);
   roadmap_trip_set_focus ("Address");
   roadmap_screen_refresh ();

   roadmap_history_add ('C', argv);
   roadmap_dialog_hide (name);
}


static void roadmap_coord_cancel (const char *name, void *data) {

   roadmap_dialog_hide (name);
}


void roadmap_coord_dialog (void) {

   if (roadmap_dialog_activate ("Position", NULL, 1)) {

      roadmap_dialog_new_entry ("Coordinates", RoadMapLongitudeLabel, NULL);
      roadmap_dialog_new_entry ("Coordinates", RoadMapLatitudeLabel, NULL);

      roadmap_dialog_add_button ("Back", roadmap_coord_before);
      roadmap_dialog_add_button ("Next", roadmap_coord_after);
      roadmap_dialog_add_button ("OK", roadmap_coord_ok);
      roadmap_dialog_add_button ("Cancel", roadmap_coord_cancel);

      roadmap_dialog_complete (roadmap_preferences_use_keyboard());

      roadmap_history_declare ('C', 2);
   }

   RoadMapCoordHistory = roadmap_history_latest ('C');

   roadmap_coord_set ();
}

