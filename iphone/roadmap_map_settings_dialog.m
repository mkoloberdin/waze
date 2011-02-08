/*  roadmap_map_settings_dialog.m - iPhone specific dialog
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
#include "roadmap_main.h"
#include "roadmap_iphonemain.h"
#include "roadmap_start.h"
#include "roadmap_device_events.h"
#include "roadmap_config.h"
#include "roadmap_lang.h"
#include "roadmap_map_settings.h"
#include "widgets/iphoneCell.h"
#include "widgets/iphoneCellSwitch.h"
#include "widgets/iphoneTableHeader.h"
#include "roadmap_checklist.h"
#include "roadmap_res.h"
#include "roadmap_skin.h"
#include "Realtime.h"
#include "RealtimeAlerts.h"
#include "RealtimeBonus.h"
#include "editor/track/editor_track_main.h"
#include "roadmap_map_settings.h"
#include "roadmap_map_settings_dialog.h"
#include "roadmap_iphonemap_settings_dialog.h"

#include "roadmap_analytics.h"


extern RoadMapConfigDescriptor RoadMapConfigAutoNightMode;
extern RoadMapConfigDescriptor RoadMapConfigShowScreenIconsOnTap;
extern RoadMapConfigDescriptor RoadMapConfigShowTopBarOnTap;
extern RoadMapConfigDescriptor RoadMapConfigAutoShowStreetBar;
extern RoadMapConfigDescriptor RoadMapConfigShowWazers;
extern RoadMapConfigDescriptor RoadMapConfigColorRoads;
extern RoadMapConfigDescriptor RoadMapConfigShowSpeedCams;
extern RoadMapConfigDescriptor RoadMapConfigEnableToggleConstruction;
extern RoadMapConfigDescriptor RoadMapConfigReportDontShow;
extern RoadMapConfigDescriptor RoadMapConfigRoadGoodies;
extern RoadMapConfigDescriptor RoadMapConfigShowSpeedometer;


enum IDs {
   ID_AUTO_NIGHT_MODE = 1,
   ID_SHOW_ICONS,
   ID_SHOW_TOPBAR,
   ID_SHOW_STREET_BAR,
   ID_COLOR_SCHEME,
   ID_SHOW_SPEEDOMETER,
   ID_SHOW_WAZERS,
   ID_SHOW_COLOR_ROADS,
   ID_SHOW_SPEED_CAMS,
   ID_SHOW_GOODIES
};

#define MAX_IDS 25
#define MAX_ALERTS 25
#define ALERTS_PREFIX 1000

#define MAP_SCHEMES 9
static char *scheme_labels[MAP_SCHEMES] = {"Vitamin C", "The Blues", "Mochaccino", "Snow Day", "Twilight", "Tutti-fruiti", "Rosebud", "Electrolytes", "Map editors"};
static void *scheme_values[MAP_SCHEMES]= {"", "1", "2", "3", "4", "5", "6", "7", "8"};
static void *scheme_icons[MAP_SCHEMES] = {"schema", "schema1", "schema2", "schema3", "schema4", "schema5", "schema6", "schema7", "schema8"};

static RoadMapCallback id_callbacks[MAX_IDS];


static const char * title = "Map";

static char *AlertString[MAX_ALERTS];
static int  alertsUserCanToggle[MAX_ALERTS];
static int  countAlertsUserCanToggle;



///////////////////////////////////////////////////////////////////////////////////////////
static void map_scheme_callback (int value, int group)  {
   roadmap_analytics_log_event(ANALYTICS_EVENT_MAPSCHEME, ANALYTICS_EVENT_INFO_CHANGED_TO, scheme_labels[value]);
   
   roadmap_skin_set_scheme(scheme_values[value]);
   roadmap_main_pop_view(YES);
}

static void show_map_scheme_dialog () {
   int i;
   
   NSMutableArray *dataArray = [NSMutableArray arrayWithCapacity:1];
	NSMutableArray *groupArray = NULL;
   NSMutableDictionary *dict = NULL;
   NSString *text;
   NSNumber *accessoryType = [NSNumber numberWithInt:UITableViewCellAccessoryCheckmark];
   RoadMapChecklist *schemeView;
   UIImage *image;
   
   groupArray = [NSMutableArray arrayWithCapacity:1];
   
   for (i = 0; i < MAP_SCHEMES; i++){
      dict = [NSMutableDictionary dictionaryWithCapacity:1];
      text = [NSString stringWithUTF8String:roadmap_lang_get(scheme_labels[i])];
      [dict setValue:text forKey:@"text"];
      image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, scheme_icons[i]);
      if (image) {
         [dict setValue:image forKey:@"image"];
      }
      if (i == roadmap_skin_get_scheme()) {
         [dict setObject:accessoryType forKey:@"accessory"];
      }
      [dict setValue:[NSNumber numberWithInt:1] forKey:@"selectable"];
      [groupArray addObject:dict];
   }
   [dataArray addObject:groupArray];
   
   text = [NSString stringWithUTF8String:roadmap_lang_get ("Map color scheme")];
	schemeView = [[RoadMapChecklist alloc] 
                 activateWithTitle:text andData:dataArray andHeaders:NULL
                 andCallback:map_scheme_callback andHeight:60 andFlags:0];
}
                           

void roadmap_map_settings_dialog_show(void){
   MapSettingsDialog *dialog;
   
   roadmap_analytics_log_event(ANALYTICS_EVENT_MAPSETTINGS, NULL, NULL);
   
	dialog = [[MapSettingsDialog alloc] initWithStyle:UITableViewStyleGrouped];
	[dialog show];
}




///////////////////////////////////////////////////////////
@implementation MapSettingsDialog
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

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
   return roadmap_main_should_rotate (interfaceOrientation);
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
   roadmap_device_event_notification( device_event_window_orientation_changed);
}

- (void) populateSettingsData
{
	NSMutableArray *groupArray = NULL;
   iphoneCell *callbackCell = NULL;
	iphoneCellSwitch *swCell = NULL;
   iphoneTableHeader *header = NULL;
   int i;
	
	
	//first group
	groupArray = [NSMutableArray arrayWithCapacity:1];
   
   header = [[iphoneTableHeader alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT)];
   [header setText:roadmap_lang_get("General map settings")];
   [headersArray addObject:header];
   [header release];
   
   //Auto night mode
   if (roadmap_skin_auto_night_feature_enabled()){
      swCell = [[[iphoneCellSwitch alloc] initWithFrame:CGRectZero reuseIdentifier:@"switchCell"] autorelease];
      [swCell setTag:ID_AUTO_NIGHT_MODE];
      [swCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get 
                        ("Automatic night mode")]];
      [swCell setDelegate:self];
      [swCell setState: roadmap_config_match(&RoadMapConfigAutoNightMode, "yes")];
      [groupArray addObject:swCell];
   }
   
   //Show icons
	swCell = [[[iphoneCellSwitch alloc] initWithFrame:CGRectZero reuseIdentifier:@"switchCell"] autorelease];
	[swCell setTag:ID_SHOW_ICONS];
	[swCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get 
                     ("Display map controls on tap")]];
	[swCell setDelegate:self];
	[swCell setState: roadmap_config_match(&RoadMapConfigShowScreenIconsOnTap, "yes")];
	[groupArray addObject:swCell];
   
   //Show top bar
   swCell = [[[iphoneCellSwitch alloc] initWithFrame:CGRectZero reuseIdentifier:@"switchCell"] autorelease];
   [swCell setTag:ID_SHOW_TOPBAR];
   [swCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get 
                     ("Display top bar on tap")]];
   [swCell setDelegate:self];
   [swCell setState: roadmap_config_match(&RoadMapConfigShowTopBarOnTap, "yes")];
   [groupArray addObject:swCell];
   
   //Auto show street bar
   if (editor_track_shortcut() == 3) {
      swCell = [[[iphoneCellSwitch alloc] initWithFrame:CGRectZero reuseIdentifier:@"switchCell"] autorelease];
      [swCell setTag:ID_SHOW_STREET_BAR];
      [swCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get 
                        ("Prompt me to edit road info")]];
      [swCell setDelegate:self];
      [swCell setState: roadmap_config_match(&RoadMapConfigAutoShowStreetBar, "yes")];
      [groupArray addObject:swCell];
   }
   
   //Speedometer
	swCell = [[[iphoneCellSwitch alloc] initWithFrame:CGRectZero reuseIdentifier:@"switchCell"] autorelease];
	[swCell setTag:ID_SHOW_SPEEDOMETER];
	[swCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get 
                     ("Speedometer")]];
	[swCell setDelegate:self];
	[swCell setState: roadmap_config_match(&RoadMapConfigShowSpeedometer, "yes")];
	[groupArray addObject:swCell];
   
   //Map color scheme
   callbackCell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"actionCell"] autorelease];
   [callbackCell setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
   [callbackCell setTag:ID_COLOR_SCHEME];
   id_callbacks[ID_COLOR_SCHEME] = show_map_scheme_dialog;
   callbackCell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get ("Map color scheme")];
   [groupArray addObject:callbackCell];
   
   [dataArray addObject:groupArray];
   
   
   //second group - map layers
	groupArray = [NSMutableArray arrayWithCapacity:1];
   
   header = [[iphoneTableHeader alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT)];
   [header setText:roadmap_lang_get("Show on map")];
   [headersArray addObject:header];
   [header release];

	//Show wazers
	swCell = [[[iphoneCellSwitch alloc] initWithFrame:CGRectZero reuseIdentifier:@"switchCell"] autorelease];
	[swCell setTag:ID_SHOW_WAZERS];
	[swCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get 
                     ("Wazers")]];
	[swCell setDelegate:self];
	[swCell setState: roadmap_config_match(&RoadMapConfigShowWazers, "yes")];
	[groupArray addObject:swCell];
   
   //Show color roads
	swCell = [[[iphoneCellSwitch alloc] initWithFrame:CGRectZero reuseIdentifier:@"switchCell"] autorelease];
	[swCell setTag:ID_SHOW_COLOR_ROADS];
	[swCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get 
                     ("Traffic layer (color coded)")]];
	[swCell setDelegate:self];
	[swCell setState: roadmap_config_match(&RoadMapConfigColorRoads, "yes")];
	[groupArray addObject:swCell];
   
   //Show speed cams
	swCell = [[[iphoneCellSwitch alloc] initWithFrame:CGRectZero reuseIdentifier:@"switchCell"] autorelease];
	[swCell setTag:ID_SHOW_SPEED_CAMS];
	[swCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get 
                     ("Speed cams")]];
	[swCell setDelegate:self];
	[swCell setState: roadmap_config_match(&RoadMapConfigShowSpeedCams, "yes")];
	[groupArray addObject:swCell];
   
   //Show alerts
   countAlertsUserCanToggle = roadmap_map_settings_allowed_alerts(alertsUserCanToggle);
   roadmap_map_settings_alert_string(AlertString);
   for (i = 0; i < countAlertsUserCanToggle; i++){
      if (!roadmap_config_match(&RoadMapConfigEnableToggleConstruction, "yes"))
         if ( alertsUserCanToggle[i] == RT_ALERT_TYPE_CONSTRUCTION ) // don't show construction
            continue;
      swCell = [[[iphoneCellSwitch alloc] initWithFrame:CGRectZero reuseIdentifier:@"switchCell"] autorelease];
      [swCell setTag:ALERTS_PREFIX + alertsUserCanToggle[i]];
      [swCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get 
                        (AlertString[alertsUserCanToggle[i]])]];
      [swCell setDelegate:self];
      [swCell setState: roadmap_map_settings_show_report(alertsUserCanToggle[i])];
      [groupArray addObject:swCell];
   }
   
   //Show road goodies
	swCell = [[[iphoneCellSwitch alloc] initWithFrame:CGRectZero reuseIdentifier:@"switchCell"] autorelease];
	[swCell setTag:ID_SHOW_GOODIES];
	[swCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get 
                     ("Road goodies")]];
	[swCell setDelegate:self];
	[swCell setState: roadmap_config_match(&RoadMapConfigRoadGoodies, "yes")];
	[groupArray addObject:swCell];

   
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

- (void)setDisabledAlerts
{
	char newDescriptorData[100];
	char num[5];
	BOOL firstReport = TRUE;
	int i;
   iphoneCellSwitch *cellSwitch;
   
   UITableView *tableView = [self tableView];
   
	strcpy(newDescriptorData,"");
   
	for (i = 0; i < countAlertsUserCanToggle; i++){
		
		if (!roadmap_config_match(&RoadMapConfigEnableToggleConstruction, "yes"))
         if ( alertsUserCanToggle[i] == RT_ALERT_TYPE_CONSTRUCTION ) // don't show construction
            continue;
      
      cellSwitch = (iphoneCellSwitch *)[tableView viewWithTag:ALERTS_PREFIX + alertsUserCanToggle[i]];
		if (cellSwitch && ![cellSwitch getState]){ // user decided not to show report
         
         if (firstReport){ 	
		    	firstReport = FALSE;
         }else{
		    	strcat(newDescriptorData,"-");
         }
         snprintf(num , 5, "%d",alertsUserCanToggle[i]);
         strcat( newDescriptorData, num );
         
		}	 
	}
	roadmap_config_set(&RoadMapConfigReportDontShow,newDescriptorData);
   RTAlerts_RefreshOnMap();
}

- (void)dealloc
{	
   [self setDisabledAlerts];
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
	
	if (id_callbacks[tag]) {
		(*id_callbacks[tag])();
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
		case ID_AUTO_NIGHT_MODE:
			if ([(iphoneCellSwitch *) view getState]) {
            roadmap_analytics_log_event(ANALYTICS_EVENT_NIGHTMODESET, ANALYTICS_EVENT_INFO_CHANGED_TO, ANALYTICS_EVENT_ON);

				roadmap_config_set (&RoadMapConfigAutoNightMode, yesno[0]);
				roadmap_skin_auto_night_mode();
			} else {
            roadmap_analytics_log_event(ANALYTICS_EVENT_NIGHTMODESET, ANALYTICS_EVENT_INFO_CHANGED_TO, ANALYTICS_EVENT_OFF);

				roadmap_config_set (&RoadMapConfigAutoNightMode, yesno[1]);
				roadmap_skin_auto_night_mode_kill_timer();
			}
			break;
      case ID_SHOW_ICONS:
			if ([(iphoneCellSwitch *) view getState]) {
            roadmap_analytics_log_event(ANALYTICS_EVENT_MAPCONTROLSSET, ANALYTICS_EVENT_INFO_CHANGED_TO, ANALYTICS_EVENT_ON);

				roadmap_config_set (&RoadMapConfigShowScreenIconsOnTap, yesno[0]);
			} else {
            roadmap_analytics_log_event(ANALYTICS_EVENT_MAPCONTROLSSET, ANALYTICS_EVENT_INFO_CHANGED_TO, ANALYTICS_EVENT_OFF);

				roadmap_config_set (&RoadMapConfigShowScreenIconsOnTap, yesno[1]);
         }
			break;
      case ID_SHOW_TOPBAR:
			if ([(iphoneCellSwitch *) view getState])
				roadmap_config_set (&RoadMapConfigShowTopBarOnTap, yesno[0]);
			else
				roadmap_config_set (&RoadMapConfigShowTopBarOnTap, yesno[1]);
			break;
		case ID_SHOW_SPEEDOMETER:
         if ([(iphoneCellSwitch *) view getState])
            roadmap_config_set (&RoadMapConfigShowSpeedometer, yesno[0]);
         else
            roadmap_config_set (&RoadMapConfigShowSpeedometer, yesno[1]);
			OnSettingsChanged_VisabilityGroup();
         break;
      case ID_SHOW_STREET_BAR:
			if ([(iphoneCellSwitch *) view getState])
				roadmap_config_set (&RoadMapConfigAutoShowStreetBar, yesno[0]);
			else
				roadmap_config_set (&RoadMapConfigAutoShowStreetBar, yesno[1]);
			break;
      case ID_SHOW_WAZERS:
			if ([(iphoneCellSwitch *) view getState])
				roadmap_config_set (&RoadMapConfigShowWazers, yesno[0]);
			else
				roadmap_config_set (&RoadMapConfigShowWazers, yesno[1]);
         OnSettingsChanged_VisabilityGroup();
			break;
      case ID_SHOW_COLOR_ROADS:
			if ([(iphoneCellSwitch *) view getState])
				roadmap_config_set (&RoadMapConfigColorRoads, yesno[0]);
			else
				roadmap_config_set (&RoadMapConfigColorRoads, yesno[1]);
			break;
      case ID_SHOW_SPEED_CAMS:
			if ([(iphoneCellSwitch *) view getState])
				roadmap_config_set (&RoadMapConfigShowSpeedCams, yesno[0]);
			else
				roadmap_config_set (&RoadMapConfigShowSpeedCams, yesno[1]);
			break;
      case ID_SHOW_GOODIES:
			if ([(iphoneCellSwitch *) view getState]) {
				roadmap_config_set (&RoadMapConfigRoadGoodies, yesno[0]);
         } else {
				roadmap_config_set (&RoadMapConfigRoadGoodies, yesno[1]);
            RealtimeBonus_Term();
         }
			break;
		default:
			break;
	}	
}

@end

