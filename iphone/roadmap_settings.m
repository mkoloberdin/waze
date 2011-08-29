/* roadmap_settings.m
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi R.
 *   Copyright 2009, Waze Ltd
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


#include <assert.h>
#include <string.h>
#include "roadmap.h"
#include "roadmap_keyboard.h"
#include "roadmap_config.h"
#include "roadmap_lang.h"
#include "roadmap_device.h"
#include "roadmap_path.h"

#include "roadmap_main.h"
#include "roadmap_iphonemain.h"
#include "widgets/iphoneCellEdit.h"
#include "widgets/iphoneCellSwitch.h"
#include "widgets/iphoneCellSelect.h"
#include "widgets/iphoneTableHeader.h"
#include "roadmap_checklist.h"
#include "roadmap_factory.h"
#include "roadmap_start.h"
#include "roadmap_screen.h"
#include "roadmap_skin.h"
#include "roadmap_res.h"
#include "roadmap_iphonesettings.h"
#include "roadmap_bar.h"
#include "roadmap_mood.h"
#include "roadmap_map_download.h"
#include "roadmap_device_events.h"
#include "Realtime/RealtimeExternalPoi.h"
#include "navigate/navigate_main.h"
#include "tts.h"

#include "roadmap_analytics.h"


static const char*   title = "Settings menu";

extern RoadMapConfigDescriptor NavigateConfigNavigationGuidanceType;
extern RoadMapConfigDescriptor NavigateConfigNavigationGuidanceOn;

#define FIRST_TAG  (1000)

enum IDs {
	ID_NAV_GUIDANCE = 1,
	ID_DISPLAY,
	ID_LIGHT,
	ID_TRAFFIC,
	ID_LOGIN,
	ID_PRIVACY,
   ID_RECOMMEND,
	ID_GENERAL,
	ID_GROUPS,
	ID_HELP_MENU,
   ID_MY_COUPONS,
   ID_DEBUG_INFO,
   ID_MEDIA_PLAYER,
   ID_DOWNLOAD,
   ID_MAP
};

#define MAX_IDS 25

static const char *id_actions[MAX_IDS];
static RoadMapCallback id_callbacks[MAX_IDS];
static int isSettingsShown = 0;


//static void guidance_callback (int value, int group) {
//   const char ** guidance_values = navigate_main_get_guidance_types();
//   
//   if (!strcmp( guidance_values[value], NAV_CFG_GUIDANCE_TYPE_NATURAL )) {
//      if ( !roadmap_config_match( &NavigateConfigNavigationGuidanceType, NAV_CFG_GUIDANCE_TYPE_NATURAL ) )
//         roadmap_analytics_log_event( ANALYTICS_EVENT_NAVGUIDANCE, ANALYTICS_EVENT_INFO_CHANGED_TO, ANALYTICS_EVENT_NAV_GUIDANCE_NATURAL );
//   } else if (!strcmp( guidance_values[value], NAV_CFG_GUIDANCE_TYPE_TTS )) {
//      if ( !roadmap_config_match( &NavigateConfigNavigationGuidanceType, NAV_CFG_GUIDANCE_TYPE_TTS ) )
//         roadmap_analytics_log_event( ANALYTICS_EVENT_NAVGUIDANCE, ANALYTICS_EVENT_INFO_CHANGED_TO, ANALYTICS_EVENT_NAV_GUIDANCE_TTS );
//   } else if (!strcmp( guidance_values[value], NAV_CFG_GUIDANCE_TYPE_NONE )) {
//      if ( !roadmap_config_match( &NavigateConfigNavigationGuidanceType, NAV_CFG_GUIDANCE_TYPE_NONE ) )
//         roadmap_analytics_log_event( ANALYTICS_EVENT_NAVGUIDANCE, ANALYTICS_EVENT_INFO_CHANGED_TO, ANALYTICS_EVENT_OFF );
//   }
//   
//   roadmap_config_set ( &NavigateConfigNavigationGuidanceType, guidance_values[value] );
//   if ( strcmp( guidance_values[value], NAV_CFG_GUIDANCE_TYPE_NONE ) )
//   {
//      roadmap_config_set ( &NavigateConfigNavigationGuidanceOn, "yes" );
//   }
//   
//   roadmap_config_save(FALSE);
//   roadmap_main_pop_view(YES);
//}
//
//static void show_guidance_type (void) {
//   const char ** guidance_values = navigate_main_get_guidance_types();
//   const char ** guidance_labels = navigate_main_get_guidance_labels();
//   int guidance_count = navigate_main_get_guidance_types_count();   
//   int i;
//   
//   NSMutableArray *dataArray = [NSMutableArray arrayWithCapacity:1];
//	NSMutableArray *groupArray = NULL;
//   NSMutableDictionary *dict = NULL;
//   NSString *text;
//   NSNumber *accessoryType = [NSNumber numberWithInt:UITableViewCellAccessoryCheckmark];
//   RoadMapChecklist *GuidanceView;
//   const char *current_value;
//   
//   if ( roadmap_config_match( &NavigateConfigNavigationGuidanceOn, "yes" ) )
//   {
//      current_value = navigate_main_get_guidance_type( roadmap_config_get( &NavigateConfigNavigationGuidanceType ) );
//   }
//   else
//   {
//      current_value = navigate_main_get_guidance_type( NAV_CFG_GUIDANCE_TYPE_NONE );
//   }
//   
//   groupArray = [NSMutableArray arrayWithCapacity:1];
//   
//   for (i = 0; i < guidance_count; ++i) {
//      dict = [NSMutableDictionary dictionaryWithCapacity:1];
//      text = [NSString stringWithUTF8String:roadmap_lang_get(guidance_labels[i])];
//      [dict setValue:text forKey:@"text"];
//      if (strcmp(guidance_values[i], current_value) == 0) {
//         [dict setObject:accessoryType forKey:@"accessory"];
//      }
//      [dict setValue:[NSNumber numberWithInt:1] forKey:@"selectable"];
//      [groupArray addObject:dict];
//   }
//   [dataArray addObject:groupArray];
//   
//   text = [NSString stringWithUTF8String:roadmap_lang_get ("Navigation guidance")];
//	GuidanceView = [[RoadMapChecklist alloc] 
//                  activateWithTitle:text andData:dataArray andHeaders:NULL
//                  andCallback:guidance_callback andHeight:60 andFlags:0];}

void roadmap_settings(void) {
   SettingsDialog *dialog;
   
	if (isSettingsShown) {
		//roadmap_main_pop_view(YES);
		return;
	}
   
   roadmap_analytics_log_event(ANALYTICS_EVENT_SETTINGS, NULL, NULL);
	
	isSettingsShown = 1;
	dialog = [[SettingsDialog alloc] initWithStyle:UITableViewStyleGrouped];
	[dialog show];
}



@implementation SettingsDialog
@synthesize dataArray;
@synthesize headersArray;

- (void) onClose
{
   roadmap_main_show_root(NO);
}

- (id)initWithStyle:(UITableViewStyle)style
{
	int i;
	static int initialized = 0;
	
	self = [super initWithStyle:style];
	
	dataArray = [[NSMutableArray arrayWithCapacity:1] retain];
   headersArray = [[NSMutableArray arrayWithCapacity:1] retain];
	
	if (!initialized) {
		for (i=0; i < MAX_IDS; ++i) {
			id_actions[i] = NULL;
         id_callbacks[i] = NULL;
		}
		initialized = 1;
	}
	
	return self;
}

- (void) viewDidLoad
{
   int i;
   iphoneTableHeader *header;
   
	UITableView *tableView = [self tableView];
	
   roadmap_main_set_table_color(tableView);
   tableView.rowHeight = 50;
   
   if (headersArray) {
      for (i = 0; i < [headersArray count]; ++i) {
         header = [headersArray objectAtIndex:i];
         [header layoutIfNeeded];
      }
   }
}

//- (void)viewWillAppear:(BOOL)animated
//{
//   [super viewWillAppear:animated];
//   UITableView *tableView = [self tableView];
//   
//   iphoneCell *cell = (iphoneCell *)[tableView viewWithTag:ID_NAV_GUIDANCE+FIRST_TAG];
//   if (cell){
//      const char *current_value;
//      if ( roadmap_config_match( &NavigateConfigNavigationGuidanceOn, "yes" ) )
//         current_value = navigate_main_get_guidance_type( roadmap_config_get( &NavigateConfigNavigationGuidanceType ) );
//      else
//         current_value = navigate_main_get_guidance_type( NAV_CFG_GUIDANCE_TYPE_NONE );
//      cell.rightLabel.text = [NSString stringWithUTF8String:roadmap_lang_get (navigate_main_get_guidance_label(current_value))];
//      
//      [cell setNeedsLayout];
//   }
//   
//   [tableView reloadData];
//}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
   return roadmap_main_should_rotate (interfaceOrientation);
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
   roadmap_device_event_notification( device_event_window_orientation_changed);
}

- (void) populateSettingsData
{
   NSMutableArray *groupArray = NULL;
   iphoneCell *actionCell = NULL;
   NSArray *segmentsArray = NULL;
   iphoneCell *callbackCell = NULL;
   iphoneCellSelect *selCell = NULL;
   UIImage *img = NULL;
   iphoneTableHeader *header = NULL;
   char *icon_name;
   const RoadMapAction *this_action;
   
   
   //first group
   groupArray = [NSMutableArray arrayWithCapacity:1];
   
   header = [[iphoneTableHeader alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT)];
   [header setText:"Navigation Guidance"];
   [headersArray addObject:header];
   [header release];
   
   //Navigation guidance type
   //callbackCell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"actionCell"] autorelease];
//   [callbackCell setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
//   [callbackCell setTag:ID_NAV_GUIDANCE+FIRST_TAG];
//   id_callbacks[ID_NAV_GUIDANCE] = show_guidance_type;
//   callbackCell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get ("Guidance")];
//   const char *current_value;
//   if ( roadmap_config_match( &NavigateConfigNavigationGuidanceOn, "yes" ) )
//      current_value = navigate_main_get_guidance_type( roadmap_config_get( &NavigateConfigNavigationGuidanceType ) );
//   else
//      current_value = navigate_main_get_guidance_type( NAV_CFG_GUIDANCE_TYPE_NONE );
//   callbackCell.rightLabel.text = [NSString stringWithUTF8String:roadmap_lang_get (navigate_main_get_guidance_label(current_value))];
//   [groupArray addObject:callbackCell];
   
   selCell = [[[iphoneCellSelect alloc] initWithFrame:CGRectZero reuseIdentifier:@"selectCell"] autorelease];
//   [selCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get ("Display")]];
   if (tts_feature_enabled()) {
      segmentsArray = [NSArray arrayWithObjects:
                       [NSString stringWithUTF8String:roadmap_lang_get(navigate_main_get_guidance_type( NAV_CFG_GUIDANCE_TYPE_TTS ))],
                       [NSString stringWithUTF8String:roadmap_lang_get(navigate_main_get_guidance_type( NAV_CFG_GUIDANCE_TYPE_NATURAL ))],
                       [NSString stringWithUTF8String:roadmap_lang_get(navigate_main_get_guidance_type( NAV_CFG_GUIDANCE_TYPE_NONE ))],
                       NULL];
      [selCell setItems:segmentsArray];
      
      const char *current_value;
      if ( roadmap_config_match( &NavigateConfigNavigationGuidanceOn, "yes" ) )
         current_value = roadmap_config_get( &NavigateConfigNavigationGuidanceType ) ;
      else
         current_value = ( NAV_CFG_GUIDANCE_TYPE_NONE );
      if (!strcmp(current_value, NAV_CFG_GUIDANCE_TYPE_NONE ))
         [selCell setSelectedSegment:2];
      else if (!strcmp(current_value, NAV_CFG_GUIDANCE_TYPE_NATURAL ))
         [selCell setSelectedSegment:1];
      else
         [selCell setSelectedSegment:0];
   } else {
      segmentsArray = [NSArray arrayWithObjects:
                       [NSString stringWithUTF8String:roadmap_lang_get("On")],
                       [NSString stringWithUTF8String:roadmap_lang_get("Off")],
                       NULL];
      [selCell setItems:segmentsArray];
      
      if ( roadmap_config_match( &NavigateConfigNavigationGuidanceOn, "yes" ) )
         [selCell setSelectedSegment:0];
      else
         [selCell setSelectedSegment:1];
      
      if (!roadmap_config_match( &NavigateConfigNavigationGuidanceType, NAV_CFG_GUIDANCE_TYPE_NATURAL))
         roadmap_config_set(&NavigateConfigNavigationGuidanceType, NAV_CFG_GUIDANCE_TYPE_NATURAL);
   }

   [selCell setTag:ID_NAV_GUIDANCE+FIRST_TAG];
   [selCell setDelegate:self];
   [groupArray addObject:selCell];
   
   [dataArray addObject:groupArray];
   
   //second group
   groupArray = [NSMutableArray arrayWithCapacity:1];
   
   header = [[iphoneTableHeader alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT)];
   [header setText:"Map"];
   [headersArray addObject:header];
   [header release];
   
   //View 2D/3D
   selCell = [[[iphoneCellSelect alloc] initWithFrame:CGRectZero reuseIdentifier:@"selectCell"] autorelease];
   [selCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get ("Display")]];
   segmentsArray = [NSArray arrayWithObjects:@"2D", @"3D", NULL];
   [selCell setItems:segmentsArray];
   if (roadmap_screen_get_view_mode())
      [selCell setSelectedSegment:1];
   else
      [selCell setSelectedSegment:0];
   [selCell setTag:ID_DISPLAY+FIRST_TAG];
   [selCell setDelegate:self];
   [groupArray addObject:selCell];
   
   
   //Light day/night
   selCell = [[[iphoneCellSelect alloc] initWithFrame:CGRectZero reuseIdentifier:@"selectCell"] autorelease];
   [selCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get ("Mode")]];
   segmentsArray = [NSArray arrayWithObjects:[NSString stringWithUTF8String:roadmap_lang_get("Day")],
                    [NSString stringWithUTF8String:roadmap_lang_get("Night")],
                    NULL];
   [selCell setItems:segmentsArray];
   if (roadmap_skin_state())
      [selCell setSelectedSegment:1];
   else
      [selCell setSelectedSegment:0];
   [selCell setTag:ID_LIGHT+FIRST_TAG];
   [selCell setDelegate:self];
   [groupArray addObject:selCell];
   
   [dataArray addObject:groupArray];
   
   
   
   
   //Media Player
   //actionCell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"actionCell"] autorelease];
//   icon_name = "media_player";
//   img = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, icon_name);
//   if (img) {
//      actionCell.imageView.image = img;
//   }
//   
//   [actionCell setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
//   [actionCell setTag:ID_MEDIA_PLAYER+FIRST_TAG];
//   id_actions[ID_MEDIA_PLAYER] = "media_player";
//   this_action =  roadmap_start_find_action (id_actions[ID_MEDIA_PLAYER]);
//   actionCell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get 
//                                (this_action->label_long)];
//   [groupArray addObject:actionCell];
//   
//   [dataArray addObject:groupArray];
   
   
   //third group
   groupArray = [NSMutableArray arrayWithCapacity:1];
   
   header = [[iphoneTableHeader alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT)];
   [header setText:"Settings"];
   [headersArray addObject:header];
   [header release];
   
   //General settings
   actionCell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"actionCell"] autorelease];
   icon_name = "general_settings";
   img = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, icon_name);
   if (img) {
      actionCell.imageView.image = img;
   }
   
   [actionCell setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
   [actionCell setTag:ID_GENERAL+FIRST_TAG];
   id_actions[ID_GENERAL] = "general_settings";
   this_action =  roadmap_start_find_action (id_actions[ID_GENERAL]);
   actionCell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get 
                                (this_action->label_long)];
   [groupArray addObject:actionCell];
   
   //Login Details
   actionCell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"actionCell"] autorelease];
   icon_name = "login_details";
   img = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, icon_name);
   if (img) {
      actionCell.imageView.image = img;
   }
   [actionCell setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
   [actionCell setTag:ID_LOGIN+FIRST_TAG];
   id_actions[ID_LOGIN] = "login_details";
   this_action =  roadmap_start_find_action (id_actions[ID_LOGIN]);
   actionCell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get 
                                (this_action->label_long)];
   [groupArray addObject:actionCell];
   
   //Map
   actionCell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"actionCell"] autorelease];
   icon_name = "map_settings";
   img = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, icon_name);
   if (img) {
      actionCell.imageView.image = img;
   }
   [actionCell setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
   [actionCell setTag:ID_MAP+FIRST_TAG];
   id_actions[ID_MAP] = "map_settings";
   this_action =  roadmap_start_find_action (id_actions[ID_MAP]);
   actionCell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get 
                                (this_action->label_long)];
   [groupArray addObject:actionCell];
   
   //Download
   actionCell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"actionCell"] autorelease];
   icon_name = "download_settings";
   img = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, icon_name);
   if (img) {
      actionCell.imageView.image = img;
   }
   [actionCell setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
   [actionCell setTag:ID_DOWNLOAD+FIRST_TAG];
   id_actions[ID_DOWNLOAD] = "download_settings";
   this_action =  roadmap_start_find_action (id_actions[ID_DOWNLOAD]);
   actionCell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get 
                                (this_action->label_long)];
   [groupArray addObject:actionCell];
   
   //Traffic
   actionCell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"actionCell"] autorelease];
   icon_name = "traffic";
   img = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, icon_name);
   if (img) {
      actionCell.imageView.image = img;
   }
   [actionCell setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
   [actionCell setTag:ID_TRAFFIC+FIRST_TAG];
   id_actions[ID_TRAFFIC] = "traffic";
   this_action =  roadmap_start_find_action (id_actions[ID_TRAFFIC]);
   actionCell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get 
                                (this_action->label_long)];
   [groupArray addObject:actionCell];
   /*
   //Privacy
   actionCell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"actionCell"] autorelease];
   icon_name = "privacy_settings";
   img = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, icon_name);
   if (img) {
      actionCell.imageView.image = img;
   }
   [actionCell setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
   [actionCell setTag:ID_PRIVACY+FIRST_TAG];
   id_actions[ID_PRIVACY] = "privacy_settings";
   this_action =  roadmap_start_find_action (id_actions[ID_PRIVACY]);
   actionCell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get 
                                (this_action->label_long)];
   [groupArray addObject:actionCell];
   */
   
   //Groups settings
   actionCell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"actionCell"] autorelease];
   icon_name = "group_settings";
   img = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, icon_name);
   if (img) {
      actionCell.imageView.image = img;
   }
   
   [actionCell setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
   [actionCell setTag:ID_GROUPS+FIRST_TAG];
   id_actions[ID_GROUPS] = "group_settings";
   this_action =  roadmap_start_find_action (id_actions[ID_GROUPS]);
   actionCell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get 
                                (this_action->label_long)];
   [groupArray addObject:actionCell];
   
   //Spread the word
   actionCell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"actionCell"] autorelease];
   icon_name = "recommend";
   img = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, icon_name);
   if (img) {
      actionCell.imageView.image = img;
   }
   [actionCell setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
   [actionCell setTag:ID_RECOMMEND+FIRST_TAG];
   id_actions[ID_RECOMMEND] = "recommend";
   this_action =  roadmap_start_find_action (id_actions[ID_RECOMMEND]);
   actionCell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get 
                                (this_action->label_long)];
   [groupArray addObject:actionCell];
   
   //Help / support
   actionCell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"actionCell"] autorelease];
   icon_name = "help_menu";
   img = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, icon_name);
   if (img) {
      actionCell.imageView.image = img;
   }
   [actionCell setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
   [actionCell setTag:ID_HELP_MENU+FIRST_TAG];
   id_actions[ID_HELP_MENU] = "help_menu";
   this_action =  roadmap_start_find_action (id_actions[ID_HELP_MENU]);
   actionCell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get 
                                (this_action->label_long)];
   [groupArray addObject:actionCell];
   
   //My coupons
   if (RealtimeExternalPoi_MyCouponsEnabled()) {
      [dataArray addObject:groupArray];
      
      
      //4th group
      groupArray = [NSMutableArray arrayWithCapacity:1];
      
      header = [[iphoneTableHeader alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT)];
      [header setText:""];
      [headersArray addObject:header];
      [header release];
      
      actionCell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"actionCell"] autorelease];
      icon_name = "my_coupons";
      img = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, icon_name);
      if (img) {
         actionCell.imageView.image = img;
      }
      [actionCell setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
      [actionCell setTag:ID_MY_COUPONS+FIRST_TAG];
      id_actions[ID_MY_COUPONS] = "my_coupons";
      this_action =  roadmap_start_find_action (id_actions[ID_MY_COUPONS]);
      actionCell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get 
                                   (this_action->label_long)];
      [groupArray addObject:actionCell];
   }
   
   [dataArray addObject:groupArray];
}

- (void) show
{
	[self populateSettingsData];

	[self setTitle:[NSString stringWithUTF8String:roadmap_lang_get(title)]];
   
   //set right button
	UINavigationItem *navItem = [self navigationItem];
   UIBarButtonItem *barButton = [[UIBarButtonItem alloc] initWithTitle:[NSString stringWithUTF8String:roadmap_lang_get("Close")]
                                                                 style:UIBarButtonItemStyleDone target:self action:@selector(onClose)];
   [navItem setRightBarButtonItem:barButton];
	
	roadmap_main_push_view (self);
   
}


- (void)dealloc
{	
	isSettingsShown = 0;
	roadmap_config_save(TRUE);

	[dataArray release];
   [headersArray release];

	
	[super dealloc];
}



//////////////////////////////////////////////////////////
//Table view delegate
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return [dataArray count];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	return [(NSArray *)[dataArray objectAtIndex:section] count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {

	UITableViewCell *cell = (UITableViewCell *)[(NSArray *)[dataArray objectAtIndex:indexPath.section] objectAtIndex:indexPath.row];
	
	return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	int tag = [[tableView cellForRowAtIndexPath:indexPath] tag] - FIRST_TAG;
	
	if (id_actions[tag]) {
		const RoadMapAction *this_action =  roadmap_start_find_action (id_actions[tag]);
		(*this_action->callback)();
	} else if (id_callbacks[tag]) {
		(*id_callbacks[tag])();
	}
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
   if (indexPath.section == 0 && indexPath.row == 0) {
      return 40;
   } else {
      return 50;
   }

}


- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {
   return [headersArray objectAtIndex:section];
}

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section {
   if (!headersArray)
      return 0;
   
   iphoneTableHeader *header = [headersArray objectAtIndex:section];
 
   [header layoutIfNeeded];
   
   if ([[header getText] isEqualToString:@""])
      return 0;
   else
      return header.bounds.size.height;      
}




//////////////////////////////////////////////////////////
//Segmented ctrl delegate
- (void) segmentToggle:(id)segmentView {
	iphoneCellSelect *view = (iphoneCellSelect*)[[segmentView superview] superview];
	int tag = [view tag]-FIRST_TAG;
	
	switch (tag) {
      case ID_NAV_GUIDANCE:
         if (tts_feature_enabled()) {
            switch ([view getItem]) {
               case 0:
                  roadmap_analytics_log_event( ANALYTICS_EVENT_NAVGUIDANCE, ANALYTICS_EVENT_INFO_CHANGED_TO, ANALYTICS_EVENT_NAV_GUIDANCE_TTS );
                  roadmap_config_set ( &NavigateConfigNavigationGuidanceType, NAV_CFG_GUIDANCE_TYPE_TTS );
                  roadmap_config_set ( &NavigateConfigNavigationGuidanceOn, "yes" );
                  break;
               case 1:
                  roadmap_analytics_log_event( ANALYTICS_EVENT_NAVGUIDANCE, ANALYTICS_EVENT_INFO_CHANGED_TO, ANALYTICS_EVENT_NAV_GUIDANCE_NATURAL );
                  roadmap_config_set ( &NavigateConfigNavigationGuidanceType, NAV_CFG_GUIDANCE_TYPE_NATURAL );
                  roadmap_config_set ( &NavigateConfigNavigationGuidanceOn, "yes" );
                  break;
               case 2:
                  roadmap_analytics_log_event( ANALYTICS_EVENT_NAVGUIDANCE, ANALYTICS_EVENT_INFO_CHANGED_TO, ANALYTICS_EVENT_OFF );
                  roadmap_config_set ( &NavigateConfigNavigationGuidanceType, NAV_CFG_GUIDANCE_TYPE_NONE );
               default:
                  break;
            }
         } else {
            if ([view getItem] == 0)
               roadmap_config_set ( &NavigateConfigNavigationGuidanceOn, "yes" );
            else
               roadmap_config_set ( &NavigateConfigNavigationGuidanceOn, "no" );
         }

         roadmap_config_save(FALSE);
         break;
		case ID_DISPLAY:
			if ([view getItem] == 1) {
            roadmap_analytics_log_event(ANALYTICS_EVENT_VIEWMODESET, ANALYTICS_EVENT_INFO_NEW_MODE, ANALYTICS_EVENT_3D);

				roadmap_screen_set_view (VIEW_MODE_3D);
			} else {
            roadmap_analytics_log_event(ANALYTICS_EVENT_VIEWMODESET, ANALYTICS_EVENT_INFO_NEW_MODE, ANALYTICS_EVENT_2D);

				roadmap_screen_set_view (VIEW_MODE_2D);
         }
			break;
		case ID_LIGHT:
			if ([view getItem] == 1) {
            roadmap_analytics_log_event(ANALYTICS_EVENT_DAYNIGHTSET, ANALYTICS_EVENT_INFO_NEW_MODE, ANALYTICS_EVENT_NIGHT);

				roadmap_skin_set_subskin ("night");
			} else {
            roadmap_analytics_log_event(ANALYTICS_EVENT_DAYNIGHTSET, ANALYTICS_EVENT_INFO_NEW_MODE, ANALYTICS_EVENT_DAY);

				roadmap_skin_set_subskin ("day");
         }
			break;
		default:
			break;
	}
}


@end

