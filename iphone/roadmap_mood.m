/* roadmap_mood.m - Manage mood selection (iphone)
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
 *   Copyright 2009 Avi R.
 *   Copyright 2009, Waze Ltd
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
 *   See roadmap_mood.h
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
#include "Realtime.h"
#include "roadmap_mood.h"
#include "roadmap_res.h"
#include "widgets/iphoneTableHeader.h"
#include "roadmap_checklist.h"
#include "roadmap_analytics.h"


#define MAX_MOOD_ENTRIES 50

static char *mood_value[MAX_MOOD_ENTRIES];

static RoadMapConfigDescriptor MoodCfg =
	ROADMAP_CONFIG_ITEM("User", "Mood");

//Mood set event
static const char* ANALYTICS_EVENT_MOODSET_NAME = "TOGGLE_MOOD";
static const char* ANALYTICS_EVENT_MOODSET_INFO = "CHANGED_TO";


///////////////////////////////////////////////////////////////////////////////////////////
void roadmap_mood_call_back (int value, int group) {
   if (group == 1)
      value += 3;
   
   roadmap_analytics_log_event(ANALYTICS_EVENT_MOODSET_NAME, ANALYTICS_EVENT_MOODSET_INFO, mood_value[value]);

	roadmap_mood_set (mood_value[value]);
	
	roadmap_main_pop_view(YES);
}

///////////////////////////////////////////////////////////////////////////////////////////
UIImage *loadMoodImage (const char *name) {
	const char *cursor;
	UIImage *img;
	for (cursor = roadmap_path_first ("skin");
		 cursor != NULL;
		 cursor = roadmap_path_next ("skin", cursor)){
		
		NSString *fileName = [NSString stringWithFormat:@"%s/%s.png", cursor, name];
		
		img = [[UIImage alloc] initWithContentsOfFile: fileName];
		
		if (img) break; 
	}
	return (img);
}

///////////////////////////////////////////////////////////////////////////////////////////
void roadmap_mood_dialog (RoadMapCallback callback) {

   char **files;
   const char *cursor;
   char **cursor2;
   char *directory;
   int count = 0; 		
   char *icon;
   char str[512];
   NSMutableArray *dataArray = [NSMutableArray arrayWithCapacity:1];
   NSMutableArray *groupArray = NULL;
   NSMutableArray *headersArray = [NSMutableArray arrayWithCapacity:1];
   NSMutableDictionary *dict = NULL;
   NSString *text = NULL;
   NSNumber *accessoryType = [NSNumber numberWithInt:UITableViewCellAccessoryCheckmark];
   UIImage *image = NULL;
   iphoneTableHeader *header;
   RoadMapChecklist *moodView;
   BOOL isNewbie = FALSE;
   
   roadmap_config_declare ("user", &MoodCfg, "happy", NULL);
   
   //Newbie
   if (Realtime_IsNewbie()) {
      char str2[128];
      header = [[iphoneTableHeader alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT) ];
      snprintf (str2, sizeof(str2), roadmap_lang_get("(Gotta drive %d+ %s to access other moods)"), 
                roadmap_mood_get_number_of_newbie_miles(), roadmap_lang_get(roadmap_math_trip_unit()));
      snprintf(str, sizeof(str), "%s %s", 
               roadmap_lang_get("Waze newbie"),
               str2);
      [header setText:str];
      [headersArray addObject:header];
      [header release];
      groupArray = [NSMutableArray arrayWithCapacity:1];
      dict = [NSMutableDictionary dictionaryWithCapacity:1];
      mood_value[count] = strdup("wazer_baby");
      text = [NSString stringWithUTF8String:roadmap_lang_get("Waze newbie")];
      [dict setValue:text forKey:@"text"];
      image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "wazer_baby");
      if (image) {
         [dict setValue:image forKey:@"image"];
      }
      [dict setObject:accessoryType forKey:@"accessory"];
      [dict setValue:[NSNumber numberWithInt:1] forKey:@"selectable"];
      [groupArray addObject:dict];
      count++;
      
      [dataArray addObject:groupArray];
      isNewbie = TRUE;
   }
   
   //Exclusive moods
   header = [[iphoneTableHeader alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT) ];
   snprintf(str, sizeof(str), "%s %s", 
            roadmap_lang_get("Exclusive moods"),
            roadmap_lang_get("(Available only to top weekly scoring wazers)"));
   [header setText:str];
   [headersArray addObject:header];
   [header release];
   groupArray = [NSMutableArray arrayWithCapacity:1];
   //gold
   dict = [NSMutableDictionary dictionaryWithCapacity:1];
   mood_value[count] = strdup("wazer_gold");
   text = [NSString stringWithUTF8String:roadmap_lang_get(mood_value[count])];
   [dict setValue:text forKey:@"text"];
   image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "wazer_gold");
   if (image) {
      [dict setValue:image forKey:@"image"];
   }
   if (roadmap_config_match(&MoodCfg,mood_value[count])) {
      [dict setObject:accessoryType forKey:@"accessory"];
   }
   if (roadmap_mood_get_exclusive_moods() < 3 || isNewbie) {
      [dict setValue:[UIColor grayColor] forKey:@"color"];
   } else {
      [dict setValue:[NSNumber numberWithInt:1] forKey:@"selectable"];
   }
   [groupArray addObject:dict];
   count++;
   //silver
   dict = [NSMutableDictionary dictionaryWithCapacity:1];
   mood_value[count] = strdup("wazer_silver");
   text = [NSString stringWithUTF8String:roadmap_lang_get(mood_value[count])];
   [dict setValue:text forKey:@"text"];
   image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "wazer_silver");
   if (image) {
      [dict setValue:image forKey:@"image"];
   }
   if (roadmap_config_match(&MoodCfg,mood_value[count])) {
      [dict setObject:accessoryType forKey:@"accessory"];
   }
   if (roadmap_mood_get_exclusive_moods() < 2 || isNewbie) {
      [dict setValue:[UIColor grayColor] forKey:@"color"];
   } else {
      [dict setValue:[NSNumber numberWithInt:1] forKey:@"selectable"];
   }
   [groupArray addObject:dict];
   count++;
   //bronze
   dict = [NSMutableDictionary dictionaryWithCapacity:1];
   mood_value[count] = strdup("wazer_bronze");
   text = [NSString stringWithUTF8String:roadmap_lang_get(mood_value[count])];
   [dict setValue:text forKey:@"text"];
   image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "wazer_bronze");
   if (image) {
      [dict setValue:image forKey:@"image"];
   }
   if (roadmap_config_match(&MoodCfg,mood_value[count])) {
      [dict setObject:accessoryType forKey:@"accessory"];
   }
   if (roadmap_mood_get_exclusive_moods() < 1 || isNewbie) {
      [dict setValue:[UIColor grayColor] forKey:@"color"];
   } else {
      [dict setValue:[NSNumber numberWithInt:1] forKey:@"selectable"];
   }
   [groupArray addObject:dict];
   count++;
   
   [dataArray addObject:groupArray];
   
   //Everyday moods
   header = [[iphoneTableHeader alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT) ];
   snprintf(str, sizeof(str), "%s %s", 
            roadmap_lang_get("Everyday moods"),
            roadmap_lang_get("(Available to all)"));
   [header setText:str];
   [headersArray addObject:header];
   [header release];
   groupArray = [NSMutableArray arrayWithCapacity:1];
   for (cursor = roadmap_path_first ("skin");
        cursor != NULL;
        cursor = roadmap_path_next ("skin", cursor)) {
      
      directory = roadmap_path_join (cursor, "moods");
      
      files = roadmap_path_list (directory, ".png");
      
      roadmap_path_free(directory);
      
      for (cursor2 = files; *cursor2 != NULL; ++cursor2) {
         //set text
         dict = [NSMutableDictionary dictionaryWithCapacity:1];
         mood_value[count] = strdup(strtok(*cursor2,"."));
         text = [NSString stringWithUTF8String:roadmap_lang_get(mood_value[count])];
         [dict setValue:text forKey:@"text"];
         
         //set icon
         icon = roadmap_path_join("moods", *cursor2);
         image = loadMoodImage(icon);
         roadmap_path_free(icon);
         if (image) {
            [dict setValue:image forKey:@"image"];
            [image release];
         }
         
         if (roadmap_config_match(&MoodCfg,*cursor2)) {
            [dict setObject:accessoryType forKey:@"accessory"];
         }
         if (isNewbie) {
            [dict setValue:[UIColor grayColor] forKey:@"color"];
         } else {
            [dict setValue:[NSNumber numberWithInt:1] forKey:@"selectable"];
         }
         [groupArray addObject:dict];
         count++;
      }
      
      roadmap_path_list_free(files);
   } 
   [dataArray addObject:groupArray];
   
   text = [NSString stringWithUTF8String:roadmap_lang_get ("Select your mood")];
   moodView = [[RoadMapChecklist alloc] 
               activateWithTitle:text andData:dataArray andHeaders:headersArray 
               andCallback:roadmap_mood_call_back andHeight:40 andFlags:CHECKLIST_GLOBAL];
}

///////////////////////////////////////////////////////////////////////////////////////////
void roadmap_mood(void){
	static int initialized = 0;
	int i;
	
	if (!initialized) {
		for (i = 0; i < MAX_MOOD_ENTRIES; ++i)
			mood_value[i] = NULL;
		
		initialized = 1;
	} else {
		for (i = 0; i < MAX_MOOD_ENTRIES; ++i) {
			if (mood_value[i]) {
				free (mood_value[i]);
				mood_value[i] = NULL;
			}
		}
	}
	
	roadmap_mood_dialog(NULL);
}

