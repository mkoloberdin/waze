/* privacy_settings.m
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi R.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License V2 as published by
 *   the Free Software Foundation.
 *
 *   RoadMap is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with RoadMap; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "roadmap.h"
#include "roadmap_config.h"
#include "roadmap_checklist.h"
#include "roadmap_lang.h"
#include "roadmap_path.h"
#include "privacy_settings.h"
#include "Realtime/RealtimePrivacy.h"
#include "Realtime/RealtimeDefs.h"
#include "Realtime/Realtime.h"
#include "roadmap_bar.h"
#include "widgets/iphoneTableHeader.h"

#define NUM_DRIVING 3
static const char *driving_label[NUM_DRIVING];
static const char *driving_value[NUM_DRIVING];
static const char *driving_icon[NUM_DRIVING];

#define NUM_REPORTING 2
static const char *reporting_label[NUM_REPORTING];
static const char *reporting_value[NUM_REPORTING];
static const char *reporting_icon[NUM_REPORTING];

#define NUM_GROUPS 3
static const char *header_label[NUM_GROUPS];

extern RoadMapConfigDescriptor RT_CFG_PRM_VISGRP_Var;
extern RoadMapConfigDescriptor RT_CFG_PRM_VISREP_Var;


void on_item_selected (int value, int group) {
	if (group == 1) {
		roadmap_config_set (&RT_CFG_PRM_VISGRP_Var, driving_value[value]);
	} else if (group == 2) {
		roadmap_config_set (&RT_CFG_PRM_VISREP_Var, reporting_value[value]);
	}
	
	roadmap_config_save(TRUE);
	
	OnSettingsChanged_VisabilityGroup();
	
	roadmap_bar_draw();
}

UIImage *privacy_load_skin_image (const char *name) {
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


void privacy_settings_show() {
	NSMutableArray *dataArray = [NSMutableArray arrayWithCapacity:1];
	NSMutableArray *groupArray = NULL;
   NSMutableArray *headersArray = [NSMutableArray arrayWithCapacity:1];
	NSMutableDictionary *dict = NULL;
	NSString *text = NULL;
	NSNumber *accessoryType = [NSNumber numberWithInt:UITableViewCellAccessoryCheckmark];
	UIImage *image = NULL;
   iphoneTableHeader *header;
	RoadMapChecklist *privacyView;
	int i;
	
	//Init
	if (!driving_value[0]) {
		header_label[0] = roadmap_lang_get ("Display my location on waze mobile and web maps as follows:");
 		header_label[1] = roadmap_lang_get ("When driving");
      header_label[2] = roadmap_lang_get ("When reporting");
      
		driving_value[0] = RT_CFG_PRM_VISGRP_Nickname;
		driving_value[1] = RT_CFG_PRM_VISGRP_Anonymous;
		driving_value[2] = RT_CFG_PRM_VISGRP_Invisible;
		driving_label[0] = roadmap_lang_get ("Nickname");
		driving_label[1] = roadmap_lang_get ("Anonymous");
		driving_label[2] = roadmap_lang_get ("Don't show me");
		driving_icon[0] = "privacy_nickname";
		driving_icon[1] = "privacy_anonymous";
		driving_icon[2] = "privacy_invisible";

		reporting_value[0] = RT_CFG_PRM_VISREP_Nickname;
		reporting_value[1] = RT_CFG_PRM_VISREP_Anonymous;
		reporting_label[0] = roadmap_lang_get ("Nickname");
		reporting_label[1] = roadmap_lang_get ("Anonymous");
		reporting_icon[0] = "On_map_nickname";
		reporting_icon[1] = "On_map_anonymous";
	}
	
	//title (group 0)
   text = @"     ";
	text = [text stringByAppendingString: [NSString stringWithUTF8String:header_label[0]]];
   header = [[iphoneTableHeader alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT) ];
   [header setText:header_label[0]];
   [headersArray addObject:header];
   [header release];
   
         //empty group (only header)
  	groupArray = [NSMutableArray arrayWithCapacity:0];
	[dataArray addObject:groupArray];
	
   
	//When driving (group 1)
   text = @"     ";
	text = [text stringByAppendingString: [NSString stringWithUTF8String:header_label[1]]];
   header = [[iphoneTableHeader alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT) ];
   [header setText:header_label[1]];
   [headersArray addObject:header];
   [header release];
   
	groupArray = [NSMutableArray arrayWithCapacity:NUM_DRIVING];
	for (i = 0; i < NUM_DRIVING; ++i) {
		dict = [NSMutableDictionary dictionaryWithCapacity:1];
		text = [NSString stringWithUTF8String:driving_label[i]];
		[dict setValue:text forKey:@"text"];
      if (roadmap_config_match(&RT_CFG_PRM_VISGRP_Var, driving_value[i])) {
         [dict setObject:accessoryType forKey:@"accessory"];
      }
      [dict setValue:[NSNumber numberWithInt:1] forKey:@"selectable"];
      image = privacy_load_skin_image(driving_icon[i]);
      if (image) {
         [dict setObject:image forKey:@"image"];
         [image release];
      }
		[groupArray addObject:dict];
	}
	[dataArray addObject:groupArray];
	
   
	//When reporting (group 2)
   text = @"     ";
	text = [text stringByAppendingString: [NSString stringWithUTF8String:header_label[2]]];
   header = [[iphoneTableHeader alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT) ];
   [header setText:header_label[2]];
   [headersArray addObject:header];
   [header release];
   
	groupArray = [NSMutableArray arrayWithCapacity:NUM_REPORTING];
	for (i = 0; i < NUM_REPORTING; ++i) {
		dict = [NSMutableDictionary dictionaryWithCapacity:1];
		text = [NSString stringWithUTF8String:reporting_label[i]];
		[dict setValue:text forKey:@"text"];
      if (roadmap_config_match(&RT_CFG_PRM_VISREP_Var, reporting_value[i])) {
         [dict setObject:accessoryType forKey:@"accessory"];
      }
      [dict setValue:[NSNumber numberWithInt:1] forKey:@"selectable"];
      image = privacy_load_skin_image(reporting_icon[i]);
      if (image) {
         [dict setObject:image forKey:@"image"];
         [image release];
      }
		[groupArray addObject:dict];
	}
	[dataArray addObject:groupArray];
	
	
	text = [NSString stringWithUTF8String:roadmap_lang_get (PRIVACY_TITLE)];
	
	privacyView = [[RoadMapChecklist alloc] activateWithTitle:text andData:dataArray andHeaders:headersArray andCallback:on_item_selected];
}