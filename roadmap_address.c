/* roadmap_address.c - manage the roadmap address dialogs.
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
 *   See roadmap_address.h
 */

#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_math.h"
#include "roadmap_config.h"
#include "roadmap_gui.h"
#include "roadmap_history.h"
#include "roadmap_locator.h"
#include "roadmap_street.h"
#include "roadmap_trip.h"
#include "roadmap_screen.h"
#include "roadmap_messagebox.h"
#include "roadmap_dialog.h"
#include "roadmap_display.h"
#include "roadmap_preferences.h"
#include "roadmap_geocode.h"
#include "roadmap_county.h"
#include "roadmap_main.h"

#include "roadmap_address.h"

#define MAX_NAMES 100
static const char *def_values[2] = {"", ""};
static int RoadMapAddressSearchCount;
static char *RoadMapAddressSearchNames[MAX_NAMES];
static RoadMapAddressNav RoadMapAddressNavigate;

typedef struct {

    const char *title;

    int   use_zip;
    int   navigate;

    RoadMapGeocode *selections;

    void *history;

} RoadMapAddressDialog;


typedef struct {

   RoadMapAddressSearchCB callback;
   void *data;
   const char *city;
} RoadMapAddressSearch;

static void roadmap_address_done (RoadMapGeocode *selected,
                                  address_info *ai,
                                  RoadMapAddressDialog *context) {

    PluginStreet street;
    PluginLine line;

    roadmap_locator_activate (selected->fips);

    roadmap_log (ROADMAP_DEBUG, "selected address at %d.%06d %c, %d.%06d %c",
                 abs(selected->position.longitude)/1000000,
                 abs(selected->position.longitude)%1000000,
                 selected->position.longitude >= 0 ? 'E' : 'W',
                 abs(selected->position.latitude)/1000000,
                 abs(selected->position.latitude)%1000000,
                 selected->position.latitude >= 0 ? 'N' : 'S');

    roadmap_plugin_set_line
       (&line, ROADMAP_PLUGIN_ID, selected->line, -1, selected->square, selected->fips);

    roadmap_trip_set_point ("Selection", &selected->position);
    roadmap_trip_set_point ("Address", &selected->position);

    if (!context->navigate || !RoadMapAddressNavigate) {

       roadmap_dialog_hide (context->title);

       roadmap_trip_set_focus ("Address");

       roadmap_display_activate
          ("Selected Street", &line, &selected->position, &street);

       roadmap_screen_refresh ();
    } else {
       if ((*RoadMapAddressNavigate) (&selected->position, &line, 0, ai) != -1) {
         roadmap_dialog_hide (context->title);
       }
    }
}


static void roadmap_address_selected (const char *name, void *data) {

   RoadMapGeocode *selected;

   selected =
      (RoadMapGeocode *) roadmap_dialog_get_data ("List", ".Streets");

   if (selected != NULL) {
      roadmap_address_done (selected, data);
   }
}


static void roadmap_address_selection_ok (const char *name, void *data) {

   RoadMapAddressDialog *context = (RoadMapAddressDialog *) data;

   roadmap_dialog_hide (name);
   roadmap_address_selected (name, data);

   if (context->selections != NULL) {
      free (context->selections);
      context->selections = NULL;
   }
}


static void roadmap_address_selection (void  *data,
                                       int    count,
                                       RoadMapGeocode *selections) {

   int i;
   char **names = calloc (count, sizeof(char *));
   void **list  = calloc (count, sizeof(void *));


   roadmap_check_allocated(list);
   roadmap_check_allocated(names);

   for (i = count-1; i >= 0; --i) {
      names[i] = selections[i].name;
      list[i] = selections + i;
   }


   if (roadmap_dialog_activate ("Street Select", data, 1)) {

      roadmap_dialog_new_list ("List", ".Streets");

      roadmap_dialog_add_button ("OK", roadmap_address_selection_ok);

      roadmap_dialog_complete (0); /* No need for a keyboard. */
   }

   roadmap_dialog_show_list
      ("List", ".Streets", count, names, list, roadmap_address_selected);

   free (names);
   free (list);
}


static void roadmap_address_set (RoadMapAddressDialog *context) {

   char *argv[4];

   roadmap_history_get ('A', context->history, argv);

   roadmap_dialog_set_data ("Address", "Number", argv[0]);
   roadmap_dialog_set_data ("Address", "Street", argv[1]);
   roadmap_dialog_set_data ("Address", "City",   argv[2]);
   roadmap_dialog_set_data ("Address", "State",  argv[3]);
}


static void roadmap_address_before (const char *name, void *data) {

   RoadMapAddressDialog *context = (RoadMapAddressDialog *) data;

   context->history = roadmap_history_before ('A', context->history);

   roadmap_address_set (context);
}


static void roadmap_address_after (const char *name, void *data) {

   RoadMapAddressDialog *context = (RoadMapAddressDialog *) data;

   context->history = roadmap_history_after ('A', context->history);

   roadmap_address_set (context);
}


static void roadmap_address_show (const char *name, void *data) {

   int i;
   int count;

   RoadMapGeocode *selections;

   char *street_number_image;
   char *street_name;
   char *city;
   char *state;
   const char *argv[4];

   RoadMapAddressDialog *context = (RoadMapAddressDialog *) data;


   street_number_image =
      (char *) roadmap_dialog_get_data ("Address", "Number");

   street_name   = (char *) roadmap_dialog_get_data ("Address", "Street");
   state         = (char *) roadmap_dialog_get_data ("Address", "State");

   if (context->use_zip) {
      return; /* TBD: how to select by ZIP ? Need one more table in usdir. */
   }

   city = (char *) roadmap_dialog_get_data ("Address", "City");

   count = roadmap_geocode_address (&selections,
                                    street_number_image,
                                    street_name,
                                    city,
                                    state);
   if (count <= 0) {
      roadmap_messagebox (roadmap_lang_get ("Warning"),
                          roadmap_geocode_last_error());
      return;
   }

   argv[0] = street_number_image;
   argv[1] = street_name;
   argv[2] = city;
   argv[3] = state;

   roadmap_history_add ('A', argv);
   roadmap_history_save();

   if (count > 1) {

      /* Open a selection dialog to let the user choose the right block. */

      roadmap_address_selection (context, count, selections);

      for (i = 0; i < count; ++i) {
         free (selections[i].name);
         selections[i].name = NULL;
      }

      context->selections = selections; /* Free these only when done. */

   } else {
      address_info   ai;
      ai.state = state;
      ai.country = NULL;
      ai.city = city;
      ai.street = street_name;
      ai.house = street_number_image;

      roadmap_address_done (selections, &ai, context);

      free (selections[0].name);
      free (selections);
   }
}


static void roadmap_address_navigate (const char *name, void *data) {

   RoadMapAddressDialog *context = (RoadMapAddressDialog *) data;

   context->navigate = 1;

   roadmap_address_show (name, data);
}


/*** city / street search dialogs ***/
static int roadmap_address_populate_list (RoadMapString index,
                                          const char *string,
                                          void *context) {

   if (RoadMapAddressSearchCount == MAX_NAMES) return 0;

   RoadMapAddressSearchNames[RoadMapAddressSearchCount++] = (char *)string;

   return 1;
}


static void roadmap_address_search_populate (const char *name, void *data) {

   const char *str;
   char count_str[10];
   RoadMapAddressSearch *context = (RoadMapAddressSearch *)data;

   str = roadmap_dialog_get_data (".search", "Name");

   RoadMapAddressSearchCount = 0;

   if (strlen(str)) {

      if (context->city) {
         roadmap_street_search (context->city,
                                str,
                                roadmap_address_populate_list,
                                NULL);
      } else {
         roadmap_locator_search_city (str, roadmap_address_populate_list, NULL);
      }
   }

   snprintf (count_str, sizeof(count_str), "%d", RoadMapAddressSearchCount);
   roadmap_dialog_set_data  (".search", "found", count_str);

   roadmap_dialog_show_list
      (".search", ".results",
       RoadMapAddressSearchCount,
       RoadMapAddressSearchNames, (void **)RoadMapAddressSearchNames,
       NULL);

   roadmap_main_flush();
}


static void roadmap_address_search_cancel (const char *name, void *data) {
   RoadMapAddressSearch *context = (RoadMapAddressSearch *)data;

   roadmap_dialog_hide ("Search Address");

   (*context->callback) (NULL, context->data);

   free (context);
}


static void roadmap_address_search_done (const char *name, void *data) {

   char *result_name = (char *) roadmap_dialog_get_data (".search", ".results");
   RoadMapAddressSearch *context = (RoadMapAddressSearch *)data;

   roadmap_dialog_hide ("Search Address");

   (*context->callback) (result_name, context->data);

   free (context);
}


static void roadmap_address_city_result (const char *result, void *data) {

   RoadMapAddressDialog *context = (RoadMapAddressDialog *)data;

   roadmap_dialog_activate (context->title, context, 1);

   if ((result == NULL) || !strlen (result)) return;

   roadmap_dialog_set_data ("Address", "City", result);
}


static void roadmap_address_street_result (const char *result, void *data) {

   RoadMapAddressDialog *context = (RoadMapAddressDialog *)data;
   char name[255];
   char *tmp;

   roadmap_dialog_activate (context->title, context, 1);

   if ((result == NULL) || !strlen (result)) return;

   strncpy_safe (name, result, sizeof(name));

   tmp = strrchr (name, ',');
   if (!tmp) return;

   *tmp = 0;
   tmp += 2;
   /* FIXME this is actually a memory leak.
    * The whole combo boxes mass must be fixed ASAP.
    */
   roadmap_dialog_set_data ("Address", "Street", strdup(name));
   roadmap_dialog_set_data ("Address", "City", strdup(tmp));
}


static void roadmap_address_other_cb (const char *name, void *context) {

   if (!strcmp(roadmap_dialog_get_data ("Address", "City"), def_values[1]) ){

      roadmap_dialog_set_data ("Address", "City", def_values[0]);
      roadmap_address_search_dialog
         (NULL, roadmap_address_city_result, context);
   } else if (!strcmp(roadmap_dialog_get_data ("Address", "Street"),
                      def_values[1])) {

      roadmap_dialog_set_data ("Address", "Street", def_values[0]);

      roadmap_address_search_dialog
         (roadmap_dialog_get_data ("Address", "City"),
          roadmap_address_street_result, context);
   }
}

static void roadmap_address_dialog (RoadMapAddressDialog *context) {

   if (!def_values[1][0]) {
      def_values[1] = roadmap_lang_get ("Search");
   }

   if (roadmap_dialog_activate (context->title, context, 1)) {

      if (context->use_zip) {
         roadmap_dialog_new_entry ("Address", "Zip", NULL);
      } else {
         roadmap_dialog_new_choice ("Address", "City",
               sizeof(def_values) / sizeof(char **),
               def_values, (void **)def_values, roadmap_address_other_cb);
      }
      roadmap_dialog_new_choice ("Address", "Street",
            sizeof(def_values) / sizeof(char **),
            def_values, (void **)def_values, roadmap_address_other_cb);
      roadmap_dialog_new_entry ("Address", "Number", NULL);
      roadmap_dialog_new_entry ("Address", "State", NULL);

      roadmap_dialog_add_button ("Back", roadmap_address_before);
      roadmap_dialog_add_button ("Next", roadmap_address_after);
      roadmap_dialog_add_button ("Show", roadmap_address_show);
      roadmap_dialog_add_button ("Navigate", roadmap_address_navigate);

      roadmap_dialog_complete (roadmap_preferences_use_keyboard());
      roadmap_dialog_set_data
            ("Address", "State",
             roadmap_county_get_name (roadmap_locator_active ()));

      roadmap_history_declare( ADDRESS_HISTORY_CATEGORY, ahi__count);

   }

   context->navigate = 0;

   context->history = roadmap_history_latest ('A');

   if (context->history) {
      void *history = context->history;

      while (history) {
         void *prev = context->history;

         roadmap_address_set (context);
         context->history = roadmap_history_before ('A', context->history);
         if (context->history == prev) break;
      }
      context->history = history;
      roadmap_address_set (context);
   }
}


void roadmap_address_search_dialog (const char *city,
                                    RoadMapAddressSearchCB callback,
                                    void *data) {

   RoadMapAddressSearch *context =
      (RoadMapAddressSearch *) malloc (sizeof(*context));

   context->callback = callback;
   context->data = data;
   context->city = city;

   if (roadmap_dialog_activate ("Search Address", context, 1)) {

      roadmap_dialog_new_entry  (".search", "Name",
                                 roadmap_address_search_populate);
      roadmap_dialog_new_label  (".search", "found");

      roadmap_dialog_new_list   (".search", ".results");

      roadmap_dialog_add_button ("Cancel", roadmap_address_search_cancel);
      roadmap_dialog_add_button ("Done", roadmap_address_search_done);

      roadmap_dialog_complete (roadmap_preferences_use_keyboard()); /* No need for a keyboard. */
   }

   roadmap_dialog_set_data  (".search", "Name", "");
   roadmap_dialog_set_data  (".search", "found", "");
   roadmap_dialog_set_focus (".search", "Name");

   roadmap_address_search_populate ("Search Address", context);
}


void roadmap_address_location_by_city (void) {

   static RoadMapAddressDialog context = {"Location", 0, 0, NULL, NULL};

   roadmap_address_dialog (&context);
}

void roadmap_address_location_by_zip (void) {

   static RoadMapAddressDialog context = {"Location by ZIP", 1, 0, NULL, NULL};

   roadmap_address_dialog (&context);
}

void roadmap_address_register_nav (RoadMapAddressNav navigate) {
   RoadMapAddressNavigate = navigate;
}

