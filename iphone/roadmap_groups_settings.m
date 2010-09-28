/* roadmap_groups_settings.m
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

#include "roadmap.h"
#include "roadmap_main.h"
#include "roadmap_iphonemain.h"
#include "roadmap_device_events.h"
#include "roadmap_config.h"
#include "roadmap_lang.h"
#include "widgets/iphoneCell.h"
#include "roadmap_checklist.h"
#include "roadmap_groups.h"
#include "roadmap_iphonegroups_settings.h"


//////////////////////////////////////////////////////////////////////////////////////////////////
static const char*   title = "Groups";

//////////////////////////////////////////////////////////////////////////////////////////////////
static const char *popup_labels[3];
static const char *popup_values[3];
static const char *wazer_labels[3];
static const char *wazer_values[3];

enum IDs {
   ID_POPUP_CHOICE = 1,
	ID_WAZER_CHOICE
};

#define MAX_IDS 25

static RoadMapCallback id_callbacks[MAX_IDS];




static void popup_callback (int value, int group) {
   roadmap_groups_set_popup_config(popup_values[value]);
   roadmap_main_pop_view(YES);
}

static void show_popup_choice() {
   NSMutableArray *dataArray = [NSMutableArray arrayWithCapacity:1];
	NSMutableArray *groupArray = NULL;
   NSMutableDictionary *dict = NULL;
   NSNumber *accessoryType = [NSNumber numberWithInt:UITableViewCellAccessoryCheckmark];
   RoadMapChecklist *choiceView;
   NSString *text;
   int i;
   
   groupArray = [NSMutableArray arrayWithCapacity:1];
   
   for (i = 0; i < 3; ++i) {
      dict = [NSMutableDictionary dictionaryWithCapacity:1];
      text = [NSString stringWithUTF8String:roadmap_lang_get(popup_labels[i])];
      [dict setValue:text forKey:@"text"];
      if (i == roadmap_groups_get_popup_config()) {
         [dict setObject:accessoryType forKey:@"accessory"];
      }
      [dict setValue:[NSNumber numberWithInt:1] forKey:@"selectable"];
      [groupArray addObject:dict];
   }
   [dataArray addObject:groupArray];
   
   text = [NSString stringWithUTF8String:roadmap_lang_get ("Pop-up reports")];
   
   choiceView = [[RoadMapChecklist alloc] 
                 activateWithTitle:text andData:dataArray andHeaders:NULL
                 andCallback:popup_callback andHeight:60 andFlags:0];
   
}

static void wazer_callback (int value, int group) {
   roadmap_groups_set_show_wazer_config(wazer_values[value]);
   roadmap_main_pop_view(YES);
}

static void show_wazer_choice() {
   NSMutableArray *dataArray = [NSMutableArray arrayWithCapacity:1];
	NSMutableArray *groupArray = NULL;
   NSMutableDictionary *dict = NULL;
   NSNumber *accessoryType = [NSNumber numberWithInt:UITableViewCellAccessoryCheckmark];
   RoadMapChecklist *choiceView;
   NSString *text;
   int i;
   
   groupArray = [NSMutableArray arrayWithCapacity:1];
   
   for (i = 0; i < 3; ++i) {
      dict = [NSMutableDictionary dictionaryWithCapacity:1];
      text = [NSString stringWithUTF8String:roadmap_lang_get(wazer_labels[i])];
      [dict setValue:text forKey:@"text"];
      if (i == roadmap_groups_get_show_wazer_config()) {
         [dict setObject:accessoryType forKey:@"accessory"];
      }
      [dict setValue:[NSNumber numberWithInt:1] forKey:@"selectable"];
      [groupArray addObject:dict];
   }
   [dataArray addObject:groupArray];
   
   text = [NSString stringWithUTF8String:roadmap_lang_get ("Waze group icons")];
   
   choiceView = [[RoadMapChecklist alloc] 
                 activateWithTitle:text andData:dataArray andHeaders:NULL
                 andCallback:wazer_callback andHeight:60 andFlags:0];
   
}

static void init(void){
   static BOOL initialized = FALSE;
   
   if (initialized)
      return;
   
   popup_values[POPUP_REPORT_ONLY_MAIN_GROUP] = POPUP_REPORT_VAL_ONLY_MAIN_GROUP;
   popup_values[POPUP_REPORT_FOLLOWING_GROUPS] = POPUP_REPORT_VAL_FOLLOWING_GROUPS;
   popup_values[POPUP_REPORT_NONE] = POPUP_REPORT_VAL_NONE;
   popup_labels[POPUP_REPORT_ONLY_MAIN_GROUP] = roadmap_lang_get ("From my main group");
   popup_labels[POPUP_REPORT_FOLLOWING_GROUPS] = roadmap_lang_get ("From all groups I follow");
   popup_labels[POPUP_REPORT_NONE] = roadmap_lang_get ("None");
   
   wazer_values[SHOW_WAZER_GROUP_MAIN] = SHOW_WAZER_GROUP_VAL_MAIN;
   wazer_values[SHOW_WAZER_GROUP_FOLLOWING] = SHOW_WAZER_GROUP_VAL_FOLLOWING;
   wazer_values[SHOW_WAZER_GROUP_ALL] = SHOW_WAZER_GROUP_VAL_ALL;
   wazer_labels[SHOW_WAZER_GROUP_MAIN] = roadmap_lang_get ("My main group only");
   wazer_labels[SHOW_WAZER_GROUP_FOLLOWING] = roadmap_lang_get ("The groups I follow");
   wazer_labels[SHOW_WAZER_GROUP_ALL] = roadmap_lang_get ("All groups");
   
   initialized = TRUE;
}


void roadmap_groups_settings (void) {
   init();
   
   GroupsSetingsDialog *dialog = [[GroupsSetingsDialog alloc] initWithStyle:UITableViewStyleGrouped];
   [dialog show];
}



@implementation GroupsSetingsDialog
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

- (void)viewWillAppear:(BOOL)animated
{
   /*
   UITableView *tableView = [self tableView];
   iphoneCell *cell;
   
   cell = (iphoneCell *)[tableView viewWithTag:ID_POPUP_CHOICE];
   if (cell){
      cell.rightLabel.text = [NSString stringWithUTF8String:popup_labels[roadmap_groups_get_popup_config()]];
      [cell setNeedsLayout];
   }
   
   cell = (iphoneCell *)[tableView viewWithTag:ID_WAZER_CHOICE];
   if (cell){
      cell.rightLabel.text = [NSString stringWithUTF8String:wazer_labels[roadmap_groups_get_show_wazer_config()]];
      [cell setNeedsLayout];
   }
    */
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
   roadmap_main_show_root(0);
}

- (void) populateData
{
	NSMutableArray *groupArray = NULL;
   iphoneCell *callbackCell = NULL;
	
   
   //Group #1
   groupArray = [NSMutableArray arrayWithCapacity:1];
   
   //Popup
   callbackCell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"actionCell"] autorelease];
   [callbackCell setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
   [callbackCell setTag:ID_POPUP_CHOICE];
   id_callbacks[ID_POPUP_CHOICE] = show_popup_choice;
   callbackCell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get ("Pop-up reports")];
   //callbackCell.rightLabel.text = [NSString stringWithUTF8String:popup_labels[roadmap_groups_get_popup_config()]];
   [groupArray addObject:callbackCell];
   
   //Wazers show
   callbackCell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"actionCell"] autorelease];
   [callbackCell setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
   [callbackCell setTag:ID_WAZER_CHOICE];
   id_callbacks[ID_WAZER_CHOICE] = show_wazer_choice;
   callbackCell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get ("Wazers group icons")];
   //callbackCell.rightLabel.text = [NSString stringWithUTF8String:wazer_labels[roadmap_groups_get_show_wazer_config()]];
   [groupArray addObject:callbackCell];
   
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
	
	iphoneCell *cell = (iphoneCell *)[(NSArray *)[dataArray objectAtIndex:indexPath.section] objectAtIndex:indexPath.row];
	
	return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	int tag = [[tableView cellForRowAtIndexPath:indexPath] tag];
	
	if (id_callbacks[tag]) {
		(*id_callbacks[tag])();
	}
}

@end

