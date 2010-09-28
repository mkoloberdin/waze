/*  roadmap_download_settings_dialog.m - iPhone specific dialog
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
#include "roadmap_res.h"
#include "roadmap_map_download.h"
#include "roadmap_net_mon.h"
#include "roadmap_download_settings.h"
#include "roadmap_download_settings_dialog.h"
#include "roadmap_iphonedownload_settings_dialog.h"


enum IDs {
   ID_DOWNLOAD_MAP = 1,
   ID_REFRESH_MAP,
   ID_DOWNLOAD_TRAFFIC,
   ID_NET_COMPRESSION,
   ID_NET_MONITOR
};

#define MAX_IDS 25

static RoadMapCallback id_callbacks[MAX_IDS];


static const char * title = "Data Usage";
static const char * NOTE1 = "* Changes won't affect routing. Your route is always calculated based on real-time traffic conditions.";         
static const char * NOTE2 = "* Traffic and updates will not be seen on the map if you disable their download.";                                             
         
                           



void map_download (void) {
   roadmap_map_download(NULL, NULL);
}

void map_refresh (void) {
   refresh_tiles_callback(NULL, NULL);
}

void roadmap_download_settings_dialog_show(void){
   DownloadSettingsDialog *dialog;
   
	dialog = [[DownloadSettingsDialog alloc] initWithStyle:UITableViewStyleGrouped];
	[dialog show];
}




///////////////////////////////////////////////////////////
@implementation DownloadSettingsDialog
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
   char str[512];
	
	
	//first group
	groupArray = [NSMutableArray arrayWithCapacity:1];
   
   header = [[iphoneTableHeader alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT)];
   [header setText:"Reduce data usage:"];
   [headersArray addObject:header];
   [header release];
   
   //download map
   if (roamdmap_map_download_enabled()) {
      callbackCell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"actionCell"] autorelease];
      callbackCell.imageView.image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "download_map");
      [callbackCell setTag:ID_DOWNLOAD_MAP];
      id_callbacks[ID_DOWNLOAD_MAP] = map_download;
      callbackCell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get ("Download map of my area")];
      [groupArray addObject:callbackCell];
   }
   
   //clear tiles cache
   callbackCell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"actionCell"] autorelease];
   callbackCell.imageView.image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "refresh_map");
   [callbackCell setTag:ID_REFRESH_MAP];
   id_callbacks[ID_REFRESH_MAP] = map_refresh;
   callbackCell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get ("Refresh map of my area")];
   [groupArray addObject:callbackCell];
   
   [dataArray addObject:groupArray];
   
   
   //second group
	groupArray = [NSMutableArray arrayWithCapacity:1];
   
   header = [[iphoneTableHeader alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT)];
   [header setText:""];
   [headersArray addObject:header];
   [header release];
   
   //net compression
   swCell = [[[iphoneCellSwitch alloc] initWithFrame:CGRectZero reuseIdentifier:@"switchCell"] autorelease];
	[swCell setTag:ID_NET_COMPRESSION];
	[swCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get ("Use data compression")]];
	[swCell setDelegate:self];
	[swCell setState:roadmap_net_get_compress_enabled()];
	[groupArray addObject:swCell];
   
   //net monitor
   swCell = [[[iphoneCellSwitch alloc] initWithFrame:CGRectZero reuseIdentifier:@"switchCell"] autorelease];
	[swCell setTag:ID_NET_MONITOR];
	[swCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get ("Display network monitor")]];
	[swCell setDelegate:self];
	[swCell setState:roadmap_net_mon_get_enabled()];
	[groupArray addObject:swCell];
   
   [dataArray addObject:groupArray];
   
   
   //third group
	groupArray = [NSMutableArray arrayWithCapacity:1];
   
   header = [[iphoneTableHeader alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT)];
   [header setText:""];
   [headersArray addObject:header];
   [header release];
   
	//download data
	swCell = [[[iphoneCellSwitch alloc] initWithFrame:CGRectZero reuseIdentifier:@"switchCell"] autorelease];
	[swCell setTag:ID_DOWNLOAD_TRAFFIC];
	[swCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get ("Download traffic info")]];
	[swCell setDelegate:self];
	[swCell setState:roadmap_download_settings_isDownloadTraffic()];
	[groupArray addObject:swCell];
	
   [dataArray addObject:groupArray];
   
   
   //third group
	groupArray = [NSMutableArray arrayWithCapacity:1];
   
   snprintf(str, sizeof(str), "%s\n%s", roadmap_lang_get(NOTE1), roadmap_lang_get(NOTE2));
   header = [[iphoneTableHeader alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT)];
   [header setText:str];
   [headersArray addObject:header];
   [header release];

	
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
	iphoneCellSwitch *view = (iphoneCellSwitch*)[[switchView superview] superview];
	int tag = [view tag];
	
	switch (tag) {
		case ID_DOWNLOAD_TRAFFIC:
         roadmap_download_settings_setDownloadTraffic ([view getState]);
			break;
      case ID_NET_COMPRESSION:
         roadmap_net_set_compress_enabled ([view getState]);
			break;
      case ID_NET_MONITOR:
         roadmap_net_mon_set_enabled ([view getState]);
			break;
		default:
			break;
	}	
}

@end

