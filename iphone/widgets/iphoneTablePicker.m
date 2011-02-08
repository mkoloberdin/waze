/* iphoneTablePicker.m - iPhone table style picker
 *
 * LICENSE:
 *
 *   Copyright 2010 Avi R
 *   Copyright 2010, Waze Ltd
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
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
#include "roadmap_lang.h"
#include "roadmap_res.h"
#include "iphoneCell.h"
#include "iphoneTablePicker.h"

@implementation iphoneTablePicker
@synthesize gTableView;
@synthesize gDelegate;
@synthesize dataArray;

- (id)initWithFrame:(CGRect)aRect
{   
	self = [super initWithFrame:aRect];
   
   self.backgroundColor = [UIColor colorWithWhite:0.2f alpha:0.7f];
   self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
   self.autoresizesSubviews = YES;
   
   if (self) {
      gTableView = [[UITableView alloc] initWithFrame:aRect style:UITableViewStyleGrouped];
      //gTableView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
      //gTableView.autoresizesSubviews = YES;
      gTableView.delegate = self;
      gTableView.dataSource = self;
      
      [self addSubview:gTableView];
      [gTableView release];
	}
   
   self.contentMode = UIViewContentModeCenter;
	
	return self;
}

- (void)layoutSubviews
{
   CGRect rect;
   if (gTableView && dataArray) {
      rect = gTableView.frame;
      rect.size.height = gTableView.rowHeight*([dataArray count] +1);
      if (rect.size.height > self.bounds.size.height)
         rect.size.height = self.bounds.size.height;
      rect.origin.x = (self.bounds.size.width - rect.size.width)/2;
      rect.origin.y = (self.bounds.size.height - rect.size.height)/2;
      gTableView.frame = rect;
   }
}

- (void) populateWithData:(NSArray *)array
              andCallback:(ChecklistCallback)callback
                andHeight:(int)height
                 andWidth:(int)width
              andDelegate:(id)delegate
{
   CGRect rect;
   
   gSelectedIndex = -1;
   
	dataArray = [array retain];
   
   gDelegate = delegate;
   gTableView.rowHeight = height;
   
   rect.size.width = width;
   rect.size.height = height*([array count] +1);
   if (rect.size.height > self.bounds.size.height)
      rect.size.height = self.bounds.size.height;
   rect.origin.x = (self.bounds.size.width - rect.size.width)/2;
   rect.origin.y = (self.bounds.size.height - rect.size.height)/2;
   gTableView.frame = rect;
   
   [gTableView reloadData];
   
   [gTableView setBackgroundColor:[UIColor clearColor]];
   if ([UITableView instancesRespondToSelector:@selector(setBackgroundView:)]) {
      UIView *dummyView = [[UIView alloc] initWithFrame:CGRectZero];
      [dummyView setBackgroundColor:[UIColor clearColor]];
      [(id)(gTableView) setBackgroundView:dummyView];
      [dummyView release];
   }
}

- (int) getSelectedIndex
{
   return gSelectedIndex;
}

- (void)dealloc
{
	[dataArray release];
   
   gTableView.delegate = nil;
   gTableView.dataSource = nil;
   
	[super dealloc];
}


//////////////////////////////////////////////////////////
//Table view delegate
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	return [dataArray count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	iphoneCell *cell = (iphoneCell *) [tableView dequeueReusableCellWithIdentifier:@"checklistCell"];
	UIImage *image;
	UIImageView *imageView;
	
	NSDictionary *dict = (NSDictionary *)[dataArray objectAtIndex:indexPath.row];
	
   if (cell == nil) {
      cell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"checklistCell"] autorelease];
   }
	
	if ([dict objectForKey:@"text"]){
		cell.textLabel.text = (NSString *)[dict objectForKey:@"text"];
      cell.textLabel.numberOfLines = 0;
      cell.textLabel.textAlignment = UITextAlignmentCenter;
      cell.textLabel.font = [UIFont systemFontOfSize:20];
      cell.textLabel.textColor = [UIColor colorWithRed:0.29 green:0.29 blue:0.29 alpha:1.0];
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
				image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "v");
				if (image) {
					imageView = [[UIImageView alloc] initWithImage:image];
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
      //cell.textLabel.font = [UIFont boldSystemFontOfSize:17];
      cell.accessoryType = UITableViewCellAccessoryNone;
      cell.accessoryView = NULL;
   }
   
	return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
   
	[tableView deselectRowAtIndexPath:[tableView indexPathForSelectedRow] animated:NO];
	
   gSelectedIndex = indexPath.row;
   if (gDelegate)
      [gDelegate performSelector:@selector(onTablePickerChange:) withObject:self];
}


@end
