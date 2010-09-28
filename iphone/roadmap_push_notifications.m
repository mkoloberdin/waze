/* roadmap_push_notifications.m - iPhone push notifications
 *
 * LICENSE:
 *
 *   Copyright 2010 Avi R.
 *   Copyright 2010, Waze Ltd
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
#include "roadmap_config.h"
#include "roadmap_lang.h"
#include "roadmap_device_events.h"
#include "roadmap_main.h"
#include "roadmap_iphonemain.h"
#include "widgets/iphoneCellSwitch.h"
#include "roadmap_analytics.h"
#include "roadmap_push_notifications.h"
#include "roadmap_iphonepush_notifications.h"


static const char*   title = "Notifications";
static char RoadMapPushNotificationToken[64] = "";
static BOOL isPending = FALSE;

static RoadMapConfigDescriptor RoadMapConfigPushScore =
      ROADMAP_CONFIG_ITEM("Push Notifications", "Score");
static RoadMapConfigDescriptor RoadMapConfigPushUpdates =
      ROADMAP_CONFIG_ITEM("Push Notifications", "Updates");
static RoadMapConfigDescriptor RoadMapConfigPushFriends =
      ROADMAP_CONFIG_ITEM("Push Notifications", "Friends");

extern RoadMapConfigDescriptor NavigateConfigNavigationGuidance;

enum IDs {
	ID_SCORE,
	ID_UPDATES,
   ID_FRIENDS
};

// Notifications events
static const char* ANALYTICS_EVENT_NOTIF_NAME         = "NOTIFICATIONS_SETTINGS";
static const char* ANALYTICS_EVENT_NOTIF_ON           = "ON";
static const char* ANALYTICS_EVENT_NOTIF_OFF          = "OFF";
static const char* ANALYTICS_EVENT_NOTIF_INFO         = "CHANGED_TO";
static const char* ANALYTICS_EVENT_NOTIF_SCORE_NAME   = "TOGGLE_NOTIFICATIONS_SCORE";
static const char* ANALYTICS_EVENT_NOTIF_UPDATES_NAME = "TOGGLE_NOTIFICATIONS_UPDATE";
static const char* ANALYTICS_EVENT_NOTIF_FRIENDS_NAME = "TOGGLE_NOTIFICATIONS_FRIENDS";



//////////////////////////////////////////////////////////////////////////////////////////////////
static void push_notifications_init (void) {
   static int initialized = 0;
	
	if (!initialized) {
		initialized = 1;
		
		roadmap_config_declare_enumeration
      ("user", &RoadMapConfigPushScore, NULL, "yes", "no", NULL);
      roadmap_config_declare_enumeration
      ("user", &RoadMapConfigPushUpdates, NULL, "yes", "no", NULL);
      roadmap_config_declare_enumeration
      ("user", &RoadMapConfigPushFriends, NULL, "yes", "no", NULL);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
const char* roadmap_push_notifications_token(void) {
   return RoadMapPushNotificationToken;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_push_notifications_set_token(const char* token) {
   strncpy_safe (RoadMapPushNotificationToken, token, sizeof(RoadMapPushNotificationToken));
   roadmap_push_notifications_set_pending(TRUE);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_push_notifications_score(void) {
   push_notifications_init();
   return roadmap_config_match (&RoadMapConfigPushScore, "yes");
}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_push_notifications_updates(void) {
   push_notifications_init();
   return roadmap_config_match (&RoadMapConfigPushUpdates, "yes");
}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_push_notifications_friends(void) {
   push_notifications_init();
   return roadmap_config_match (&RoadMapConfigPushFriends, "yes");
}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_push_notifications_pending(void) {
   return isPending;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_push_notifications_set_pending(BOOL value) {
   isPending = value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_push_notifications_settings(void) {
   PushNotificationsDialog *dialog;
   
   roadmap_analytics_log_event(ANALYTICS_EVENT_NOTIF_NAME, NULL, NULL);
	
	dialog = [[PushNotificationsDialog alloc] initWithStyle:UITableViewStyleGrouped];
	[dialog show];
}



//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
@implementation PushNotificationsDialog
@synthesize dataArray;

- (void) onClose
{
   roadmap_main_show_root(NO);
}

- (id)initWithStyle:(UITableViewStyle)style
{
	self = [super initWithStyle:style];
	
	dataArray = [[NSMutableArray arrayWithCapacity:1] retain];
	
	return self;
}

- (void) viewDidLoad
{
	UITableView *tableView = [self tableView];
	
   roadmap_main_set_table_color(tableView);
   tableView.rowHeight = 50;
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
	iphoneCellSwitch *swCell = NULL;
	
	//first group
	groupArray = [NSMutableArray arrayWithCapacity:1];
	
	//Score and rank
	swCell = [[[iphoneCellSwitch alloc] initWithFrame:CGRectZero reuseIdentifier:@"switchCell"] autorelease];
	[swCell setTag:ID_SCORE];
	[swCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get ("Your score and rank")]];
	[swCell setDelegate:self];
	[swCell setState:roadmap_config_match (&RoadMapConfigPushScore, "yes")];
	[groupArray addObject:swCell];
   
   //Contests and updates
	swCell = [[[iphoneCellSwitch alloc] initWithFrame:CGRectZero reuseIdentifier:@"switchCell"] autorelease];
	[swCell setTag:ID_UPDATES];
	[swCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get ("Contests and updates")]];
	[swCell setDelegate:self];
	[swCell setState:roadmap_config_match (&RoadMapConfigPushUpdates, "yes")];
	[groupArray addObject:swCell];
   
   //Friends
	swCell = [[[iphoneCellSwitch alloc] initWithFrame:CGRectZero reuseIdentifier:@"switchCell"] autorelease];
	[swCell setTag:ID_FRIENDS];
	[swCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get ("Friends")]];
	[swCell setDelegate:self];
	[swCell setState:roadmap_config_match (&RoadMapConfigPushFriends, "yes")];
	[groupArray addObject:swCell];
	
	[dataArray addObject:groupArray];
}

- (void) show
{
   push_notifications_init();
   
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
	roadmap_config_save(FALSE);

	[dataArray release];

	
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
		case ID_SCORE:
			if ([view getState]) {
            roadmap_analytics_log_event(ANALYTICS_EVENT_NOTIF_SCORE_NAME, ANALYTICS_EVENT_NOTIF_INFO, ANALYTICS_EVENT_NOTIF_ON);

				roadmap_config_set (&RoadMapConfigPushScore, yesno[0]);
			} else {
            roadmap_analytics_log_event(ANALYTICS_EVENT_NOTIF_SCORE_NAME, ANALYTICS_EVENT_NOTIF_INFO, ANALYTICS_EVENT_NOTIF_OFF);

				roadmap_config_set (&RoadMapConfigPushScore, yesno[1]);
			}
         
         roadmap_push_notifications_set_pending(TRUE);
			break;
      case ID_UPDATES:
			if ([view getState]) {
            roadmap_analytics_log_event(ANALYTICS_EVENT_NOTIF_UPDATES_NAME, ANALYTICS_EVENT_NOTIF_INFO, ANALYTICS_EVENT_NOTIF_ON);
            
				roadmap_config_set (&RoadMapConfigPushUpdates, yesno[0]);
			} else {
            roadmap_analytics_log_event(ANALYTICS_EVENT_NOTIF_UPDATES_NAME, ANALYTICS_EVENT_NOTIF_INFO, ANALYTICS_EVENT_NOTIF_OFF);
            
				roadmap_config_set (&RoadMapConfigPushUpdates, yesno[1]);
			}
         
         roadmap_push_notifications_set_pending(TRUE);
			break;
      case ID_FRIENDS:
			if ([view getState]) {
            roadmap_analytics_log_event(ANALYTICS_EVENT_NOTIF_FRIENDS_NAME, ANALYTICS_EVENT_NOTIF_INFO, ANALYTICS_EVENT_NOTIF_ON);
            
				roadmap_config_set (&RoadMapConfigPushFriends, yesno[0]);
			} else {
            roadmap_analytics_log_event(ANALYTICS_EVENT_NOTIF_FRIENDS_NAME, ANALYTICS_EVENT_NOTIF_INFO, ANALYTICS_EVENT_NOTIF_OFF);
            
				roadmap_config_set (&RoadMapConfigPushFriends, yesno[1]);
			}
         
         roadmap_push_notifications_set_pending(TRUE);
			break;
      default:
			break;
	}
	
}




@end

