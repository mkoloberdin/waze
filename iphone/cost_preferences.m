/* cost_preferences.m
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
#include "cost_preferences.h"
#include "iphonecost_preferences.h"
#include "widgets/iphoneCell.h"
#include "widgets/iphoneCellSwitch.h"
#include "roadmap_factory.h"
#include "roadmap_start.h"
#include "roadmap_lang.h"
#include "roadmap_main.h"
#include "roadmap_iphonemain.h"
#include "roadmap_checklist.h"
#include "roadmap_device_events.h"


static const char*   title = "Routing preferences";

static RoadMapConfigDescriptor CostTypeCfg =
   ROADMAP_CONFIG_ITEM("Routing", "Type");
static RoadMapConfigDescriptor PreferSameStreetCfg =
   ROADMAP_CONFIG_ITEM("Routing", "Prefer same street");
static RoadMapConfigDescriptor CostAvoidPrimaryCfg =
   ROADMAP_CONFIG_ITEM("Routing", "Avoid primaries");
static RoadMapConfigDescriptor CostAvoidTollRoadsCfg =
   ROADMAP_CONFIG_ITEM("Routing", "Avoid tolls");

static RoadMapConfigDescriptor CostPreferUnknownDirectionsCfg =
   ROADMAP_CONFIG_ITEM("Routing", "Prefer unknown directions");

static RoadMapConfigDescriptor CostAvoidTrailCfg =
   ROADMAP_CONFIG_ITEM("Routing", "Avoid trails");

static RoadMapConfigDescriptor TollRoadsCfg =
   ROADMAP_CONFIG_ITEM("Routing", "Tolls roads");

static RoadMapConfigDescriptor UnknownRoadsCfg =
   ROADMAP_CONFIG_ITEM("Routing", "Unknown roads");

static RoadMapConfigDescriptor CostAvoidPalestinianRoadsCfg =
   ROADMAP_CONFIG_ITEM("Routing", "Avoid Palestinian Roads");

// allow the user to avoid routing through palestinan roads only if
// this config will be true ( we will get it from server ).
static RoadMapConfigDescriptor PalestinianRoadsCfg =
   ROADMAP_CONFIG_ITEM("Routing", "Palestinian Roads");


enum IDs {
	ID_TYPE = 1,
	ID_AVOID_DIRT,
	ID_AVOID_PRIMARY,
	ID_AVOID_HIGHWAYS,
	ID_MIN_TURNS,
   ID_AVOID_TOLL,
   ID_AVOID_PALESTINIAN,
   ID_PREFER_UNKNOWN_DIR
};

#define MAX_IDS 25

static RoadMapCallback id_callbacks[MAX_IDS];

#define NUM_TRAIL_CFG 3
static const char *trails_label[NUM_TRAIL_CFG];
static const char *trails_value[NUM_TRAIL_CFG];
#define NUM_TYPE_CFG 2
static const char *type_label[NUM_TYPE_CFG];
static const char *type_value[NUM_TYPE_CFG];

                        
void on_avoid_trails_selected (int value, int group) {
	roadmap_config_set (&CostAvoidTrailCfg, trails_value[value]);
	roadmap_config_save(TRUE);
	
	roadmap_main_pop_view(YES);
}

void avoid_trails_show() {
	NSMutableArray *dataArray = [NSMutableArray arrayWithCapacity:NUM_TRAIL_CFG];
	NSMutableArray *groupArray = NULL;
	NSMutableDictionary *dict = NULL;
	NSString *text = NULL;
	NSNumber *accessoryType = [NSNumber numberWithInt:UITableViewCellAccessoryCheckmark];
	RoadMapChecklist *trailsView;
	int i;
	
	if (!trails_value[0]) {
		trails_value[0] = "Allow";
      trails_value[1] = "Don't allow";
      trails_value[2] = "Avoid long ones";
		trails_label[0] = roadmap_lang_get (trails_value[0]);
		trails_label[1] = roadmap_lang_get (trails_value[1]);
		trails_label[2] = roadmap_lang_get (trails_value[2]);
	}
	
	groupArray = [NSMutableArray arrayWithCapacity:1];
	for (i = 0; i < NUM_TRAIL_CFG; ++i) {
		dict = [NSMutableDictionary dictionaryWithCapacity:1];
		text = [NSString stringWithUTF8String:trails_label[i]];
		[dict setValue:text forKey:@"text"];
		if (roadmap_config_match(&CostAvoidTrailCfg, trails_value[i])) {
			[dict setObject:accessoryType forKey:@"accessory"];
		}
		[dict setValue:[NSNumber numberWithInt:1] forKey:@"selectable"];
		[groupArray addObject:dict];
	}
	[dataArray addObject:groupArray];
	
	text = [NSString stringWithUTF8String:roadmap_lang_get ("Dirt roads")];
	trailsView = [[RoadMapChecklist alloc] 
                 activateWithTitle:text andData:dataArray andHeaders:NULL
                 andCallback:on_avoid_trails_selected andHeight:60 andFlags:0];
}

void on_type_selected (int value, int group) {
	roadmap_config_set (&CostTypeCfg, type_value[value]);
	roadmap_config_save(TRUE);
	
	roadmap_main_pop_view(YES);
}

void type_show() {
	NSMutableArray *dataArray = [NSMutableArray arrayWithCapacity:NUM_TYPE_CFG];
	NSMutableArray *groupArray = NULL;
	NSMutableDictionary *dict = NULL;
	NSString *text = NULL;
	NSNumber *accessoryType = [NSNumber numberWithInt:UITableViewCellAccessoryCheckmark];
	RoadMapChecklist *typeView;
	int i;
	
	if (!type_value[0]) {
		type_value[0] = "Fastest";
		type_value[1] = "Shortest";
		type_label[0] = roadmap_lang_get (type_value[0]);
		type_label[1] = roadmap_lang_get (type_value[1]);
	}
	
	groupArray = [NSMutableArray arrayWithCapacity:1];
	for (i = 0; i < NUM_TYPE_CFG; ++i) {
		dict = [NSMutableDictionary dictionaryWithCapacity:1];
		text = [NSString stringWithUTF8String:type_label[i]];
		[dict setValue:text forKey:@"text"];
		if (roadmap_config_match(&CostTypeCfg, type_value[i])) {
			[dict setObject:accessoryType forKey:@"accessory"];
		}
		[dict setValue:[NSNumber numberWithInt:1] forKey:@"selectable"];
		[groupArray addObject:dict];
	}
	[dataArray addObject:groupArray];
	
	text = [NSString stringWithUTF8String:roadmap_lang_get ("Type")];
	typeView = [[RoadMapChecklist alloc] 
               activateWithTitle:text andData:dataArray andHeaders:NULL
               andCallback:on_type_selected andHeight:60 andFlags:0];
}

void cost_preferences_show (void) {
		CostDialog *dialog = [[CostDialog alloc] initWithStyle:UITableViewStyleGrouped];
		[dialog show];
}



@implementation CostDialog
@synthesize dataArray;

- (id)initWithStyle:(UITableViewStyle)style
{
	int i;
	static int initialized = 0;
	
	self = [super initWithStyle:style];
	
	dataArray = [[NSMutableArray arrayWithCapacity:1] retain];
	
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
	UITableView *tableView = [self tableView];
	
   roadmap_main_set_table_color(tableView);
   tableView.rowHeight = 50;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
   return roadmap_main_should_rotate (interfaceOrientation);
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
   roadmap_device_event_notification( device_event_window_orientation_changed);
}


- (void) onClose
{
   roadmap_main_show_root(NO);
}


- (void) populateData
{
	//NSMutableDictionary *dict = NULL;
	NSMutableArray *groupArray = NULL;
	iphoneCellSwitch *swCell = NULL;
	iphoneCell *callbackCell = NULL;

	
	//first group: route priority, avoid dirt roads
	groupArray = [NSMutableArray arrayWithCapacity:1];
	//Type
	callbackCell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"actionCell"] autorelease];
	[callbackCell setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
	[callbackCell setTag:ID_TYPE];
	id_callbacks[ID_TYPE] = type_show;
   callbackCell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get ("Type")];
	[groupArray addObject:callbackCell];
	
	//Avoid dirt roards
	callbackCell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"actionCell"] autorelease];
	[callbackCell setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
	[callbackCell setTag:ID_AVOID_DIRT];
	id_callbacks[ID_AVOID_DIRT] = avoid_trails_show;
   callbackCell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get ("Dirt roads")];
	[groupArray addObject:callbackCell];
	
	[dataArray addObject:groupArray];
	
	//Second group: Use traffic, Min turns, Avoid highways
	groupArray = [NSMutableArray arrayWithCapacity:1];
	
	//Min. turns
	swCell = [[[iphoneCellSwitch alloc] initWithFrame:CGRectZero reuseIdentifier:@"switchCell"] autorelease];
	[swCell setTag:ID_MIN_TURNS];
	[swCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get 
					  ("Minimize turns")]];
	[swCell setDelegate:self];
	[swCell setState: roadmap_config_match (&PreferSameStreetCfg, "yes")];
	[groupArray addObject:swCell];
	
	//Avoid highways
	swCell = [[[iphoneCellSwitch alloc] initWithFrame:CGRectZero reuseIdentifier:@"switchCell"] autorelease];
	[swCell setTag:ID_AVOID_HIGHWAYS];
	[swCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get 
						   ("Avoid highways")]];
	[swCell setDelegate:self];
	[swCell setState: roadmap_config_match (&CostAvoidPrimaryCfg, "yes")];
	[groupArray addObject:swCell];
   
   //Avoid toll roads
   if (roadmap_config_match(&TollRoadsCfg, "yes")){
      swCell = [[[iphoneCellSwitch alloc] initWithFrame:CGRectZero reuseIdentifier:@"switchCell"] autorelease];
      [swCell setTag:ID_AVOID_TOLL];
      [swCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get 
                        ("Avoid toll roads")]];
      [swCell setDelegate:self];
      [swCell setState: roadmap_config_match (&CostAvoidTollRoadsCfg, "yes")];
      [groupArray addObject:swCell];
   }
   
   //Avoid Palestinian roads
   if (roadmap_config_match(&PalestinianRoadsCfg, "yes")){
      swCell = [[[iphoneCellSwitch alloc] initWithFrame:CGRectZero reuseIdentifier:@"switchCell"] autorelease];
      [swCell setTag:ID_AVOID_PALESTINIAN];
      [swCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get 
                        ("Avoid areas under Palestinian authority supervision")]];
      [swCell setDelegate:self];
      [swCell setState: roadmap_config_match (&CostAvoidPalestinianRoadsCfg, "yes")];
      [groupArray addObject:swCell];
   }
   
   //Prefer unknown roads
   if (roadmap_config_match(&UnknownRoadsCfg, "yes")){
      swCell = [[[iphoneCellSwitch alloc] initWithFrame:CGRectZero reuseIdentifier:@"switchCell"] autorelease];
      [swCell setTag:ID_PREFER_UNKNOWN_DIR];
      [swCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get 
                        ("Prefer cookie munching")]];
      [swCell setDelegate:self];
      [swCell setState: roadmap_config_match (&CostPreferUnknownDirectionsCfg, "yes")];
      [groupArray addObject:swCell];
   }
	
	
	[dataArray addObject:groupArray];
}


- (void) show
{
	[self populateData];

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

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	int tag = [[tableView cellForRowAtIndexPath:indexPath] tag];
	
	if (id_callbacks[tag]) {
		(*id_callbacks[tag])();
	}
}


//////////////////////////////////////////////////////////
//Switch delegate
- (void) switchToggle:(id)switchView {
	static const char *yesno[2];
	if (!yesno[0]) {
		yesno[0] = "Yes";
		yesno[1] = "No";
	}
	
	UIView *view = [[switchView superview] superview];
	int tag = [view tag];
	
	switch (tag) {
		case ID_AVOID_HIGHWAYS:
			if ([(iphoneCellSwitch *) view getState])
				roadmap_config_set (&CostAvoidPrimaryCfg, yesno[0]);
			else
				roadmap_config_set (&CostAvoidPrimaryCfg, yesno[1]);
			break;
		case ID_MIN_TURNS:
			if ([(iphoneCellSwitch *) view getState])
				roadmap_config_set (&PreferSameStreetCfg, yesno[0]);
			else
				roadmap_config_set (&PreferSameStreetCfg, yesno[1]);
			break;
      case ID_AVOID_TOLL:
			if ([(iphoneCellSwitch *) view getState])
				roadmap_config_set (&CostAvoidTollRoadsCfg, yesno[0]);
			else
				roadmap_config_set (&CostAvoidTollRoadsCfg, yesno[1]);
			break;
      case ID_AVOID_PALESTINIAN:
			if ([(iphoneCellSwitch *) view getState])
				roadmap_config_set (&CostAvoidPalestinianRoadsCfg, yesno[0]);
			else
				roadmap_config_set (&CostAvoidPalestinianRoadsCfg, yesno[1]);
			break;
      case ID_PREFER_UNKNOWN_DIR:
			if ([(iphoneCellSwitch *) view getState])
				roadmap_config_set (&CostPreferUnknownDirectionsCfg, yesno[0]);
			else
				roadmap_config_set (&CostPreferUnknownDirectionsCfg, yesno[1]);
			break;
		default:
			break;
	}
}

@end

