/* roadmap_car.c - Manage car selection
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
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
 *   See roadmap_car.h
 */

#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_history.h"
#include "roadmap_locator.h"
#include "roadmap_street.h"
#include "roadmap_lang.h"
#include "roadmap_geocode.h"
#include "roadmap_trip.h"
#include "roadmap_lang.h"
#include "roadmap_display.h"
#include "roadmap_config.h"
#include "roadmap_path.h"
#include "roadmap_navigate.h"
#include "ssd/ssd_keyboard_dialog.h"
#include "ssd/ssd_generic_list_dialog.h"


#define MAX_CAR_ENTRIES 20

typedef struct {

    const char *title;
    RoadMapCallback callback;

} roadmap_car_list_dialog;

typedef struct {

  char *value;
  char *label;
  char *icon;

} roadmap_car_list;

///////////////////////////////////////////////////////////////////////////////////////////
static int roadmap_car_call_back (SsdWidget widget, const char *new_value, const void *value, void *context) {

   roadmap_car_list_dialog *list_context = (roadmap_car_list_dialog *)context;
   RoadMapConfigDescriptor CarCfg =
                  ROADMAP_CONFIG_ITEM("Trip", "Car");

   roadmap_config_declare
        ("user", &CarCfg, "car_blue", NULL);
   roadmap_config_set (&CarCfg, value);
   ssd_generic_list_dialog_hide ();

   if (list_context->callback)
   		(*list_context->callback)();

   return 1;
}


///////////////////////////////////////////////////////////////////////////////////////////
void roadmap_car_dialog (RoadMapCallback callback) {

    char **files;
    const char *cursor;
    char **cursor2;
    char *directory = NULL;
    int count = 0;

    static roadmap_car_list_dialog context = {"roadmap_car", NULL};
   	static char *labels[MAX_CAR_ENTRIES] ;
	static void *values[MAX_CAR_ENTRIES] ;
	static void *icons[MAX_CAR_ENTRIES];


	context.callback = callback;
    for (cursor = roadmap_path_first ("skin");
            cursor != NULL;
            cursor = roadmap_path_next ("skin", cursor)) {

	    directory = roadmap_path_join (cursor, "cars");

    	files = roadmap_path_list (directory, ".png");
    	if ( *files == NULL )
    	{
    		// Try without extension
    		files = roadmap_path_list ( directory, NULL );
    	}
   		for (cursor2 = files; *cursor2 != NULL; ++cursor2) {
   	  			labels[count]  =   (char *)roadmap_lang_get(*cursor2);
   	  			values[count] =   strtok(*cursor2,".");
   	  			icons[count]   =   roadmap_path_join("cars", *cursor2);
      			count++;
   		}
   }

    free (directory);
    ssd_generic_icon_list_dialog_show (roadmap_lang_get ("Select your car"),
                  count,
                  (const char **)labels,
                  (const void **)values,
                  (const char **)icons,
                  NULL,
                  roadmap_car_call_back,
                  NULL,
                  &context,
                  NULL,
                  NULL,
                  70,
                  0,
                  FALSE);

}

void roadmap_car(void){

	roadmap_car_dialog(NULL);
}

