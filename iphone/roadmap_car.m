/* roadmap_car.m - Manage car selection (iphone)
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
 *   Copyright 2009 Avi R.
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
#include "roadmap_main.h"
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
#include "ssd/ssd_keyboard.h"
#include "ssd/ssd_generic_list_dialog.h"
#include "roadmap_iphoneimage.h"
#include "roadmap_checklist.h"


#define MAX_CAR_ENTRIES 20

static char *car_value[MAX_CAR_ENTRIES];

RoadMapConfigDescriptor CarCfg =
			ROADMAP_CONFIG_ITEM("Trip", "Car");


///////////////////////////////////////////////////////////////////////////////////////////
void roadmap_car_call_back (int value, int group) {
	roadmap_config_set (&CarCfg, car_value[value]);
	roadmap_config_save(TRUE);
	
	roadmap_main_pop_view(YES);
}

///////////////////////////////////////////////////////////////////////////////////////////
void roadmap_car_dialog (RoadMapCallback callback) {
   
   char **files;
   const char *cursor;
   char **cursor2;
   char *directory;
   int count = 0; 		
	char *icon;
	NSMutableArray *dataArray = [NSMutableArray arrayWithCapacity:1];
	NSMutableArray *groupArray = NULL;
	NSMutableDictionary *dict = NULL;
	NSString *text = NULL;
	NSNumber *accessoryType = [NSNumber numberWithInt:UITableViewCellAccessoryCheckmark];
	UIImage *image = NULL;
	RoadMapChecklist *carView;
	
	roadmap_config_declare
   ("user", &CarCfg, "car_blue", NULL);
   
   
	groupArray = [NSMutableArray arrayWithCapacity:1];
   for (cursor = roadmap_path_first ("skin");
        cursor != NULL;
        cursor = roadmap_path_next ("skin", cursor)) {
      
      directory = roadmap_path_join (cursor, "cars");
    	
    	files = roadmap_path_list (directory, ".png");
		
		roadmap_path_free(directory);
      
      for (cursor2 = files; *cursor2 != NULL; ++cursor2) {
			//set text
			dict = [NSMutableDictionary dictionaryWithCapacity:1];
			text = [NSString stringWithUTF8String:roadmap_lang_get(*cursor2)];
			car_value[count] = strdup(strtok(*cursor2,"."));
			[dict setValue:text forKey:@"text"];
			
			//set icon
			icon = roadmap_path_join("cars", *cursor2);
			image = roadmap_iphoneimage_load(icon);
			roadmap_path_free(icon);
			if (image) {
				[dict setValue:image forKey:@"image"];
				[image release];
			}
			
			if (roadmap_config_match(&CarCfg,*cursor2)) {
				[dict setObject:accessoryType forKey:@"accessory"];
			}
			[dict setValue:[NSNumber numberWithInt:1] forKey:@"selectable"];
			[groupArray addObject:dict];
			count++;
      }
		
		roadmap_path_list_free(files);
   } 
	[dataArray addObject:groupArray];
	
	text = [NSString stringWithUTF8String:roadmap_lang_get ("Select your car")];
	carView = [[RoadMapChecklist alloc] activateWithTitle:text andData:dataArray andHeaders:NULL andCallback:roadmap_car_call_back];
}

///////////////////////////////////////////////////////////////////////////////////////////
void roadmap_car(void){
	static int initialized = 0;
	int i;
	
	if (!initialized) {
		for (i = 0; i < MAX_CAR_ENTRIES; ++i)
			car_value[i] = NULL;
		
		initialized = 1;
	} else {
		for (i = 0; i < MAX_CAR_ENTRIES; ++i) {
			if (car_value[i]) {
				free (car_value[i]);
				car_value[i] = NULL;
			}
		}
	}

	roadmap_car_dialog(NULL);
}

