/* iphoneCellSelect.m - Create select cell for iPhone table
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi R
 *   Copyright 2009, Waze Ltd
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

#include "iphoneCellSelect.h"

#define X_MARGIN 10.0f
#define Y_MARGIN 5.0f
#define MAX_CONTROL_HEIGHT 28.0f
#define MAX_CONTROL_WIDTH  130.0f


@implementation iphoneCellSelect
@synthesize labelView;
@synthesize segmentedView;

- (id)initWithFrame:(CGRect)frame reuseIdentifier:(NSString *)reuseIdentifier {
	self = [super initWithFrame:frame reuseIdentifier:reuseIdentifier];
	
	labelView = [[iphoneLabel alloc] initWithFrame:frame];
	[labelView setFont:[UIFont boldSystemFontOfSize:17]];
	[labelView setNumberOfLines:2];
	[[self contentView] addSubview:labelView];
	
	[self setSelectionStyle:UITableViewCellSelectionStyleNone];
   [self setBackgroundColor:[UIColor whiteColor]];
	
	return self;
}

- (void) layoutSubviews {
	[super layoutSubviews];
	
	if (!segmentedView || !labelView)
		return;
	
	CGRect frame = [[self contentView] bounds];
	CGRect rect;
	CGFloat height = frame.size.height - Y_MARGIN*2;
   if (height > MAX_CONTROL_HEIGHT)
      height = MAX_CONTROL_HEIGHT;
   
   CGFloat width = (frame.size.width/2) - X_MARGIN*1.5;
   if (width > MAX_CONTROL_WIDTH)
      width = MAX_CONTROL_WIDTH;
   
	rect = CGRectMake(X_MARGIN,
                     Y_MARGIN,
                     frame.size.width - width - X_MARGIN*3,
                     frame.size.height - Y_MARGIN*2);
	[labelView setFrame:rect];
	
   rect = CGRectMake(labelView.frame.origin.x + labelView.frame.size.width + X_MARGIN,
                     (frame.size.height - height)/2,
                     width,
                     height);
	[segmentedView setFrame:rect];
}

- (void) setItems:(NSArray*) segmentsArray {
	if (!segmentedView) {
		segmentedView = [[UISegmentedControl alloc] initWithItems:segmentsArray];
		[[self contentView] addSubview:segmentedView];
	}
}


- (void) setLabel:(NSString *) text {
	[labelView setText:text];
}

- (int) getItem {
	return [segmentedView selectedSegmentIndex];;
}

- (void) setDelegate:(id) delegate {
	[segmentedView addTarget:delegate action:@selector(segmentToggle:) forControlEvents:UIControlEventValueChanged];
}

- (void)setSelectedSegment:(int)index {
	[segmentedView setSelectedSegmentIndex:index];
}

- (void)dealloc
{
	[labelView release];
	[segmentedView release];
	
	[super dealloc];
}


@end
