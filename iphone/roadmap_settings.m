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
#include "roadmap_factory.h"
#include "roadmap_start.h"
#include "roadmap_screen.h"
#include "roadmap_skin.h"
#include "roadmap_iphoneimage.h"
#include "roadmap_iphonesettings.h"
#include "roadmap_bar.h"
#include "roadmap_mood.h"
#include "roadmap_map_download.h"
#include "roadmap_device_events.h"

#include "roadmap_analytics.h"


static const char*   title = "Settings menu";

extern RoadMapConfigDescriptor NavigateConfigNavigationGuidance;

enum IDs {
	ID_MUTE = 1,
	ID_DISPLAY,
	ID_LIGHT,
	ID_TRAFFIC,
	ID_LOGIN,
	ID_PRIVACY,
	ID_GENERAL,
	ID_HELP_MENU,
   ID_DEBUG_INFO,
   ID_MEDIA_PLAYER,
   ID_DOWNLOAD,
   ID_MAP
};

#define MAX_IDS 25

static const char *id_actions[MAX_IDS];
static int isSettingsShown = 0;

// Settings events
static const char* ANALYTICS_EVENT_SETTINGS_NAME          = "SETTINGS";
static const char* ANALYTICS_EVENT_MUTE_NAME              = "TOGGLE_MUTE";
static const char* ANALYTICS_EVENT_MUTE_INFO              = "CHANGED_TO";
static const char* ANALYTICS_EVENT_MUTE_ON                = "ON";
static const char* ANALYTICS_EVENT_MUTE_OFF               = "OFF";
static const char* ANALYTICS_EVENT_VIEWMODESET_NAME       = "TOGGLE_VIEW";
static const char* ANALYTICS_EVENT_VIEWMODESET_INFO       = "NEW_MODE";
static const char* ANALYTICS_EVENT_VIEWMODESET_2D         = "2D";
static const char* ANALYTICS_EVENT_VIEWMODESET_3D         = "3D";
static const char* ANALYTICS_EVENT_DAYNIGHTSET_NAME       = "TOGGLE_DAY_NIGHT";
static const char* ANALYTICS_EVENT_DAYNIGHTSET_INFO       = "NEW_MODE";
static const char* ANALYTICS_EVENT_DAYNIGHTSET_DAY        = "DAY";
static const char* ANALYTICS_EVENT_DAYNIGHTSET_NIGHT      = "NIGHT";
                        

void roadmap_settings(void) {
   SettingsDialog *dialog;
   
	if (isSettingsShown) {
		//roadmap_main_pop_view(YES);
		return;
	}
   
   roadmap_analytics_log_event(ANALYTICS_EVENT_SETTINGS_NAME, NULL, NULL);
	
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
	
   [tableView setBackgroundColor:roadmap_main_table_color()];
   if ([UITableView instancesRespondToSelector:@selector(setBackgroundView:)])
      [self.tableView setBackgroundView:nil];
   tableView.rowHeight = 50;
   
   if (headersArray) {
      for (i = 0; i < [headersArray count]; ++i) {
         header = [headersArray objectAtIndex:i];
         [header layoutIfNeeded];
      }
   }
}

- (void)viewWillAppear:(BOOL)animated
{
   [super viewWillAppear:animated];
   /*static*/ BOOL first_time = TRUE;
   UITableView *tableView = [self tableView];
   
   if (first_time) {
      first_time = FALSE;
      [tableView reloadData];
   }
}

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
	iphoneCellSwitch *swCell = NULL;
	iphoneCellSelect *selCell = NULL;
	UIImage *img = NULL;
   iphoneTableHeader *header = NULL;
	char *icon_name;
	const RoadMapAction *this_action;
	
	
	//first group
	groupArray = [NSMutableArray arrayWithCapacity:1];
   
   header = [[iphoneTableHeader alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT)];
   [header setText:"Quick actions"];
   [headersArray addObject:header];
   [header release];
	
	//Mute
	swCell = [[[iphoneCellSwitch alloc] initWithFrame:CGRectZero reuseIdentifier:@"switchCell"] autorelease];
	[swCell setTag:ID_MUTE];
	[swCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get ("Navigation guidance")]];
	[swCell setDelegate:self];
	[swCell setState:roadmap_config_match(&NavigateConfigNavigationGuidance, "yes")];
	[groupArray addObject:swCell];
	
	//View 2D/3D
	selCell = [[[iphoneCellSelect alloc] initWithFrame:CGRectZero reuseIdentifier:@"selectCell"] autorelease];
	[selCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get ("Display")]];
	segmentsArray = [NSArray arrayWithObjects:@"2D", @"3D", NULL];
	[selCell setItems:segmentsArray];
	if (roadmap_screen_get_view_mode())
		[selCell setSelectedSegment:1];
	else
		[selCell setSelectedSegment:0];
	[selCell setTag:ID_DISPLAY];
	[selCell setDelegate:self];
	[groupArray addObject:selCell];
	
	
	//Light day/night
	selCell = [[[iphoneCellSelect alloc] initWithFrame:CGRectZero reuseIdentifier:@"selectCell"] autorelease];
	[selCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get ("Light")]];
	segmentsArray = [NSArray arrayWithObjects:@"Day", @"Night", NULL];
	[selCell setItems:segmentsArray];
	if (roadmap_skin_state())
		[selCell setSelectedSegment:1];
	else
		[selCell setSelectedSegment:0];
	[selCell setTag:ID_LIGHT];
	[selCell setDelegate:self];
	[groupArray addObject:selCell];
   
   [dataArray addObject:groupArray];
	
	
	//second group
	groupArray = [NSMutableArray arrayWithCapacity:1];
   
   header = [[iphoneTableHeader alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT)];
   [header setText:""];
   [headersArray addObject:header];
   [header release];
   
   //Media Player
	actionCell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"actionCell"] autorelease];
	icon_name = "media_player";
	img = roadmap_iphoneimage_load(icon_name);
	if (img) {
      actionCell.imageView.image = img;
		[img release];
	}
	
	[actionCell setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
	[actionCell setTag:ID_MEDIA_PLAYER];
	id_actions[ID_MEDIA_PLAYER] = "media_player";
	this_action =  roadmap_start_find_action (id_actions[ID_MEDIA_PLAYER]);
   actionCell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get 
                                (this_action->label_long)];
	[groupArray addObject:actionCell];

	[dataArray addObject:groupArray];
	
	
	//third group
	groupArray = [NSMutableArray arrayWithCapacity:1];
   
   header = [[iphoneTableHeader alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT)];
   [header setText:"Settings"];
   [headersArray addObject:header];
   [header release];
      
	//General settings
	actionCell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"actionCell"] autorelease];
	icon_name = "general_settings";
	img = roadmap_iphoneimage_load(icon_name);
	if (img) {
      actionCell.imageView.image = img;
		[img release];
	}
	
	[actionCell setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
	[actionCell setTag:ID_GENERAL];
	id_actions[ID_GENERAL] = "general_settings";
	this_action =  roadmap_start_find_action (id_actions[ID_GENERAL]);
   actionCell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get 
                                (this_action->label_long)];
	[groupArray addObject:actionCell];
	
	//Login Details
	actionCell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"actionCell"] autorelease];
	icon_name = "login_details";
	img = roadmap_iphoneimage_load(icon_name);
	if (img) {
      actionCell.imageView.image = img;
		[img release];
	}
	[actionCell setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
	[actionCell setTag:ID_LOGIN];
	id_actions[ID_LOGIN] = "login_details";
	this_action =  roadmap_start_find_action (id_actions[ID_LOGIN]);
   actionCell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get 
                                (this_action->label_long)];
	[groupArray addObject:actionCell];
   
   //Map
	actionCell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"actionCell"] autorelease];
	icon_name = "map_settings";
	img = roadmap_iphoneimage_load(icon_name);
	if (img) {
      actionCell.imageView.image = img;
		[img release];
	}
	[actionCell setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
	[actionCell setTag:ID_MAP];
	id_actions[ID_MAP] = "map_settings";
	this_action =  roadmap_start_find_action (id_actions[ID_MAP]);
   actionCell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get 
                                (this_action->label_long)];
	[groupArray addObject:actionCell];
   
   //Download
   actionCell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"actionCell"] autorelease];
   icon_name = "download_settings";
   img = roadmap_iphoneimage_load(icon_name);
   if (img) {
      actionCell.imageView.image = img;
      [img release];
   }
   [actionCell setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
   [actionCell setTag:ID_DOWNLOAD];
   id_actions[ID_DOWNLOAD] = "download_settings";
   this_action =  roadmap_start_find_action (id_actions[ID_DOWNLOAD]);
   actionCell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get 
                                (this_action->label_long)];
   [groupArray addObject:actionCell];
	
	//Traffic
	actionCell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"actionCell"] autorelease];
	icon_name = "traffic";
	img = roadmap_iphoneimage_load(icon_name);
	if (img) {
      actionCell.imageView.image = img;
		[img release];
	}
	[actionCell setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
	[actionCell setTag:ID_TRAFFIC];
	id_actions[ID_TRAFFIC] = "traffic";
	this_action =  roadmap_start_find_action (id_actions[ID_TRAFFIC]);
   actionCell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get 
                                (this_action->label_long)];
	[groupArray addObject:actionCell];
	
	//Privacy
	actionCell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"actionCell"] autorelease];
	icon_name = "privacy_settings";
	img = roadmap_iphoneimage_load(icon_name);
	if (img) {
      actionCell.imageView.image = img;
		[img release];
	}
	[actionCell setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
	[actionCell setTag:ID_PRIVACY];
	id_actions[ID_PRIVACY] = "privacy_settings";
	this_action =  roadmap_start_find_action (id_actions[ID_PRIVACY]);
   actionCell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get 
                                (this_action->label_long)];
	[groupArray addObject:actionCell];

	//Help / support
	actionCell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"actionCell"] autorelease];
	icon_name = "help_menu";
	img = roadmap_iphoneimage_load(icon_name);
	if (img) {
      actionCell.imageView.image = img;
		[img release];
	}
   [actionCell setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
	[actionCell setTag:ID_HELP_MENU];
	id_actions[ID_HELP_MENU] = "help_menu";
	this_action =  roadmap_start_find_action (id_actions[ID_HELP_MENU]);
   actionCell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get 
                                (this_action->label_long)];
	[groupArray addObject:actionCell];
	
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
	int tag = [[tableView cellForRowAtIndexPath:indexPath] tag];
	
	if (id_actions[tag]) {
		const RoadMapAction *this_action =  roadmap_start_find_action (id_actions[tag]);
		(*this_action->callback)();
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
//Switch delegate
- (void) switchToggle:(id)switchView {
	
	static const char *yesno[2];
	if (!yesno[0]) {
		yesno[0] = "Yes";
		yesno[1] = "No";
	}
	
	iphoneCellSwitch *view = (iphoneCellSwitch*)[[switchView superview] superview];
	int tag = [view tag];
	
	switch (tag) {
		case ID_MUTE:
			if ([view getState]) {
            roadmap_analytics_log_event(ANALYTICS_EVENT_MUTE_NAME, ANALYTICS_EVENT_MUTE_INFO, ANALYTICS_EVENT_MUTE_ON);

				roadmap_config_set (&NavigateConfigNavigationGuidance, yesno[0]);
			} else {
            roadmap_analytics_log_event(ANALYTICS_EVENT_MUTE_NAME, ANALYTICS_EVENT_MUTE_INFO, ANALYTICS_EVENT_MUTE_OFF);

				roadmap_config_set (&NavigateConfigNavigationGuidance, yesno[1]);
			}
			roadmap_bar_draw();
			break;
      default:
			break;
	}
	
}

//////////////////////////////////////////////////////////
//Segmented ctrl delegate
- (void) segmentToggle:(id)segmentView {
	iphoneCellSelect *view = (iphoneCellSelect*)[[segmentView superview] superview];
	int tag = [view tag];
	
	switch (tag) {
		case ID_DISPLAY:
			if ([view getItem] == 1) {
            roadmap_analytics_log_event(ANALYTICS_EVENT_VIEWMODESET_NAME, ANALYTICS_EVENT_VIEWMODESET_INFO, ANALYTICS_EVENT_VIEWMODESET_3D);

				roadmap_screen_set_view (VIEW_MODE_3D);
			} else {
            roadmap_analytics_log_event(ANALYTICS_EVENT_VIEWMODESET_NAME, ANALYTICS_EVENT_VIEWMODESET_INFO, ANALYTICS_EVENT_VIEWMODESET_2D);

				roadmap_screen_set_view (VIEW_MODE_2D);
         }
			break;
		case ID_LIGHT:
			if ([view getItem] == 1) {
            roadmap_analytics_log_event(ANALYTICS_EVENT_DAYNIGHTSET_NAME, ANALYTICS_EVENT_DAYNIGHTSET_INFO, ANALYTICS_EVENT_DAYNIGHTSET_NIGHT);

				roadmap_skin_set_subskin ("night");
			} else {
            roadmap_analytics_log_event(ANALYTICS_EVENT_DAYNIGHTSET_NAME, ANALYTICS_EVENT_DAYNIGHTSET_INFO, ANALYTICS_EVENT_DAYNIGHTSET_DAY);

				roadmap_skin_set_subskin ("day");
         }
			break;
		default:
			break;
	}
}


@end

