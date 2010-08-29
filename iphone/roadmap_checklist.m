/* roadmap_checklist.m - iPhone checklist dialog
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
#include "cost_preferences.h"
#include "roadmap_checklist.h"
#include "roadmap_main.h"
#include "roadmap_lang.h"
#include "roadmap_iphonemain.h"
#include "roadmap_iphoneimage.h"
#include "roadmap_device_events.h"
#include "widgets/iphoneCell.h"
#include "widgets/iphoneTableHeader.h"


@implementation RoadMapChecklist
@synthesize dataArray;
@synthesize headersArray;
@synthesize gTitle;
@synthesize gCallback;

- (void) viewDidLoad
{
   int i;
   iphoneTableHeader *header;
   
	UITableView *tableView = [self tableView];
	
   [tableView setBackgroundColor:roadmap_main_table_color()];
   if ([UITableView instancesRespondToSelector:@selector(setBackgroundView:)])
      [(id)(self.tableView) setBackgroundView:nil];
   tableView.rowHeight = gRowHeight;
   
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


- (void) onClose
{
   roadmap_main_show_root(NO);
}

- (id) activateWithTitle:(NSString *)title
                 andData:(NSArray *)array
              andHeaders:(NSArray *)headers
             andCallback:(ChecklistCallback)callback
               andHeight:(int)height
                andFlags:(int)flags
{
	dataArray = [array retain];
   if (headers)
      headersArray = [headers retain];
   else
      headersArray = NULL;
   
   gEnableBack = !(flags & CHECKLIST_DISABLE_BACK);
   gEnableClose =  !(flags & CHECKLIST_DISABLE_CLOSE);
   gGlobalChecklist = flags & CHECKLIST_GLOBAL;
	gCallback = callback;
   gRowHeight = height;
	gTitle = [title retain];
	self = [super initWithStyle:UITableViewStyleGrouped];
	
	[self show];
	
	return self;
}

- (void) show
{
	[self setTitle:gTitle];
   
   //set right button
	UINavigationItem *navItem = [self navigationItem];
   if (gEnableBack && gEnableClose) {
      UIBarButtonItem *barButton = [[UIBarButtonItem alloc] initWithTitle:[NSString stringWithUTF8String:roadmap_lang_get("Close")]
                                                                    style:UIBarButtonItemStyleDone target:self action:@selector(onClose)];
      [navItem setRightBarButtonItem:barButton];
   } else if (!gEnableBack) {
      navItem.hidesBackButton = TRUE;
   }
	
	roadmap_main_push_view (self);
}

- (int) valueForGroup: (NSNumber *)group
{
	NSMutableArray *groupArray = [dataArray objectAtIndex:[group integerValue]];
	NSDictionary *dict = NULL;
	
	int value = -1;
	int i;
	
	for (i = 0; i < [groupArray count]; ++i) {
		dict = (NSDictionary *)[groupArray objectAtIndex:i];
		if ([dict objectForKey:@"accessory"]) 
			if ([(NSNumber *)[dict objectForKey:@"accessory"] integerValue] == UITableViewCellAccessoryCheckmark){
				value = i;
				break;
			}
	}
	
	return value;
}

- (void)dealloc
{
	//NSLog(@" dealloc checklist dialog");
	[dataArray release];
	[gTitle release];
   
   if (headersArray)
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
	iphoneCell *cell = (iphoneCell *) [tableView dequeueReusableCellWithIdentifier:@"checklistCell"];
	UIImage *image;
	UIImageView *imageView;
	
	NSMutableArray *groupArray = [dataArray objectAtIndex:indexPath.section];
	
	NSDictionary *dict = (NSDictionary *)[groupArray objectAtIndex:indexPath.row];
	
   if (cell == nil) {
      cell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"checklistCell"] autorelease];
   }
	
	if ([dict objectForKey:@"text"]){
		cell.textLabel.text = (NSString *)[dict objectForKey:@"text"];
      cell.textLabel.numberOfLines = 0;
	} else {
      cell.textLabel.text = @"";
   }
	
	if ([dict objectForKey:@"image"]){
		cell.imageView.image = (UIImage *)[dict objectForKey:@"image"];
	} else {
      cell.imageView.image = NULL;
   }
   
   if ([dict objectForKey:@"color"]){
		cell.textLabel.textColor = (UIColor *)[dict objectForKey:@"color"];
	} else {
      cell.textLabel.textColor = [UIColor blackColor];
   }
	
	if ([dict objectForKey:@"selectable"]){
		if ([dict objectForKey:@"accessory"]){
			NSNumber *accessoryType = (NSNumber *)[dict objectForKey:@"accessory"];
			if ([accessoryType integerValue] == UITableViewCellAccessoryCheckmark) {
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
		} else {
			cell.accessoryType = UITableViewCellAccessoryNone;
			cell.accessoryView = NULL;
		}
	} else {
      cell.textLabel.font = [UIFont boldSystemFontOfSize:17];
      cell.accessoryType = UITableViewCellAccessoryNone;
      cell.accessoryView = NULL;
   }
   
	return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	int i, section;
	NSMutableArray *groupArray = [dataArray objectAtIndex:indexPath.section];
	NSMutableDictionary *dict = (NSMutableDictionary *)[groupArray objectAtIndex:indexPath.row];
	NSNumber *accessoryType = NULL;
   NSMutableArray *reloadPaths = [NSMutableArray arrayWithCapacity:2];

	[tableView deselectRowAtIndexPath:[tableView indexPathForSelectedRow] animated:NO];
	
	if ([dict objectForKey:@"selectable"]) {
      for (section = 0; section < [dataArray count]; section++) {
         if (!gGlobalChecklist && section != indexPath.section)
            continue;
         groupArray = [dataArray objectAtIndex:section];
         for (i = 0; i < [groupArray count]; i++) {
            dict = (NSMutableDictionary *)[groupArray objectAtIndex:i];
            accessoryType = (NSNumber *)[dict objectForKey:@"accessory"];
            if (accessoryType &&
                [accessoryType intValue] != UITableViewCellAccessoryNone &&
                (i != indexPath.row || section != indexPath.section)) {
               accessoryType = [NSNumber numberWithInt:UITableViewCellAccessoryNone];
               [dict setObject:accessoryType forKey:@"accessory"];
               [reloadPaths addObject:[NSIndexPath indexPathForRow:i inSection:section]];
            }
         }
      }
      groupArray = [dataArray objectAtIndex:indexPath.section];
		dict = (NSMutableDictionary *)[groupArray objectAtIndex:indexPath.row];
		accessoryType = [NSNumber numberWithInt:UITableViewCellAccessoryCheckmark];
		[dict setObject:accessoryType forKey:@"accessory"];
      
      [reloadPaths addObject:indexPath];
      [tableView reloadRowsAtIndexPaths:reloadPaths withRowAnimation:UITableViewRowAnimationNone];
		
		(*gCallback)(indexPath.row, indexPath.section);
	}
}


- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {
   if (headersArray)
      return [headersArray objectAtIndex:section];
   else
      return NULL;
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


@end

