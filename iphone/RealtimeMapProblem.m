/* RealtimeMapProblem.m
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
#include "Realtime/Realtime.h"
#include "Realtime/RealtimeAlerts.h"
#include "roadmap_main.h"
#include "roadmap_lang.h"
#include "roadmap_messagebox.h"
#include "roadmap_trip.h"
#include "roadmap_navigate.h"
#include "roadmap_iphonemain.h"
#include "widgets/iphoneCell.h"
#include "widgets/iphoneCellEdit.h"
#include "roadmap_iphoneimage.h"
#include "RealtimeAlerts.h"
#include "roadmap_device_events.h"
#include "RealtimeMapProblem.h"


static const char*   title = "Map error";


enum IDs {
	ID_DESCRIPTION = 10
};

                        

static int gMapProblemsCount = 0 ;


void RTAlerts_report_map_problem() {
	RoadMapGpsPosition   *TripLocation;

	if (!roadmap_gps_have_reception ()) {
		roadmap_messagebox ("Error",
							"No GPS connection. Make sure you are outdoors. Currently showing approximate location");
		roadmap_main_show_root(YES);
		return;
	}

	TripLocation = ( RoadMapGpsPosition *)roadmap_trip_get_gps_position ("AlertSelection");
	if (TripLocation == NULL) {
		
		int login_state;
		PluginLine line;
		int direction;
		login_state = RealTimeLoginState();
		if (login_state == 0){
         roadmap_messagebox("Error","No network connection, unable to report");
			roadmap_main_show_root(YES);
         return;
		}
		
		
		TripLocation = malloc (sizeof (*TripLocation));
		if (roadmap_navigate_get_current (TripLocation, &line, &direction) == -1) {
			const RoadMapPosition *GpsPosition;
			GpsPosition = roadmap_trip_get_position ("GPS");
			if ( (GpsPosition != NULL)) {
				TripLocation->latitude = GpsPosition->latitude;
				TripLocation->longitude = GpsPosition->longitude;
				TripLocation->speed = 0;
				TripLocation->steering = 0;
			}
			else {
				free (TripLocation);
				roadmap_messagebox ("Error",
									"No GPS connection. Make sure you are outdoors. Currently showing approximate location");
				roadmap_main_show_root(YES);
				return;
			}
		}
		roadmap_trip_set_gps_position( "AlertSelection", "Selection", "new_alert_marker",TripLocation);
	}

	MapProblemDialog *dialog = [[MapProblemDialog alloc] initWithStyle:UITableViewStyleGrouped];
	[dialog showDialog];
}



@implementation MapProblemDialog
@synthesize dataArray;
@synthesize textCell;

- (void) sendMapProblem
{
	char type[3];
	BOOL success;
	const RoadMapGpsPosition   *TripLocation;
	int i;
	iphoneCell *cell = NULL;
   int *mapProblems;
   char **mapProblemsOption;
   gMapProblemsCount = RTAlertsGetMapProblems(&mapProblems, &mapProblemsOption);
	
	const char *desc = [[textCell getText] UTF8String];
	
	TripLocation = roadmap_trip_get_gps_position("AlertSelection");
	
	for (i = 0; i < gMapProblemsCount; ++i){
		cell = (iphoneCell *)[(NSArray *)[dataArray objectAtIndex:0] objectAtIndex:i];
		if (cell.accessoryView != NULL)
			snprintf(type,sizeof(type), "%d", mapProblems[i]+6);
	}
	
	success = Realtime_ReportMapProblem(type, desc, TripLocation);
	roadmap_trip_restore_focus();
	
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
	
	[tableView setBackgroundColor:[UIColor clearColor]];
   
   roadmap_main_reload_bg_image();
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
   return roadmap_main_should_rotate (interfaceOrientation);
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
   roadmap_device_event_notification( device_event_window_orientation_changed);
}


- (void) populateData
{
	NSMutableArray	*groupArray = NULL;
	iphoneCellEdit	*editCell = NULL;
	iphoneCell		*cell = NULL;
	int i;
	UIImage *image;
	UIImageView *imageView;
   int *mapProblems;
   char **mapProblemsOption;
   gMapProblemsCount = RTAlertsGetMapProblems(&mapProblems, &mapProblemsOption);
	
	//first group
	groupArray = [NSMutableArray arrayWithCapacity:1];
	
	for (i = 0; i < gMapProblemsCount; ++i) {
		cell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"simpleCell"] autorelease];
		cell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get(mapProblemsOption[mapProblems[i]])];
		cell.tag = i;
      
		if (i == gMapProblemsCount - 1) { // default selection
			image = roadmap_iphoneimage_load("v");
			if (image) {
				imageView = [[UIImageView alloc] initWithImage:image];
				[image release];
				cell.accessoryView = imageView;
				[imageView release];
			} else
				cell.accessoryType = UITableViewCellAccessoryCheckmark;
		}
       
		[groupArray addObject:cell];
	}
	
	editCell = [[[iphoneCellEdit alloc] initWithFrame:CGRectZero reuseIdentifier:@"editCell"] autorelease];
	[editCell setPlaceholder:[NSString stringWithUTF8String:roadmap_lang_get("<Add description>")]];
	[editCell setTag:ID_DESCRIPTION];
	[editCell setDelegate:self];
	[groupArray addObject:editCell];
	textCell = editCell;
	
	[dataArray addObject:groupArray];
}


- (void) showDialog
{	
	[self populateData];

	[self setTitle:[NSString stringWithUTF8String:roadmap_lang_get(title)]];
	
	UINavigationItem *navItem = [self navigationItem];
	UIBarButtonItem *button = [[UIBarButtonItem alloc] initWithTitle:[NSString stringWithUTF8String:roadmap_lang_get("Send")]
															   style:UIBarButtonItemStyleDone target:self action:@selector(sendMapProblem)];
	
	[navItem setRightBarButtonItem:button];
	[button release];
	
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
	int i;
	int tag = [[tableView cellForRowAtIndexPath:indexPath] tag];
	iphoneCell *cell = NULL;
	UIImage *image;
	UIImageView *imageView;
	
	[tableView deselectRowAtIndexPath:[tableView indexPathForSelectedRow] animated:NO];
	
	if (tag >= 0 && tag < gMapProblemsCount) {
		for (i = 0; i < gMapProblemsCount; ++i) {
			cell = (iphoneCell *)[(NSArray *)[dataArray objectAtIndex:indexPath.section] objectAtIndex:i];
			if (tag == i) {
				image = roadmap_iphoneimage_load("v");
				if (image) {
					imageView = [[UIImageView alloc] initWithImage:image];
					[image release];
					cell.accessoryView = imageView;
					[imageView release];
				} else
					cell.accessoryType = UITableViewCellAccessoryCheckmark;	
			} else {
				cell.accessoryType = UITableViewCellAccessoryNone;
				cell.accessoryView = NULL;
			}
				
		}
	}
}


//////////////////////////////////////////////////////////
//Text field delegate

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
	[textField resignFirstResponder];
	return NO;
}


@end

