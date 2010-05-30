/* iphoneCellSwitch.m - Create switch cell for iPhone table
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

#include "iphoneCellSwitch.h"

#define X_MARGIN 10.0f
#define Y_MARGIN 5.0f


@implementation iphoneCellSwitch
@synthesize labelView;
@synthesize switchView;

- (id)initWithFrame:(CGRect)frame reuseIdentifier:(NSString *)reuseIdentifier {
	self = [super initWithFrame:frame reuseIdentifier:reuseIdentifier];
	
	labelView = [[iphoneLabel alloc] initWithFrame:frame];
	[labelView setFont:[UIFont boldSystemFontOfSize:17]];
	[labelView setNumberOfLines:3];
	[[self contentView] addSubview:labelView];
	
	switchView = [[UISwitch alloc] initWithFrame: CGRectMake(200, 20, 0, 0)];
	[[self contentView] addSubview:switchView];
	
	[self setSelectionStyle:UITableViewCellSelectionStyleNone];
	
	return self;
}

- (void) layoutSubviews {
	[super layoutSubviews];
	
	if (!switchView || !labelView)
		return;
	
	CGRect frame = [[self contentView] bounds];
	CGRect rect;
	CGPoint point;
	CGFloat sw_width = [switchView frame].size.width;

	//Layout switch view
	point = CGPointMake (frame.size.width - X_MARGIN - sw_width/2,
								 frame.size.height/2);
	[switchView setCenter:point];
	
	//Layout label
	rect = CGRectMake(X_MARGIN, Y_MARGIN,
					  frame.size.width - X_MARGIN*3 - sw_width, frame.size.height-Y_MARGIN*2);
	[labelView setFrame:rect];
}

- (void) setState:(BOOL) enabled {
	[switchView setOn:enabled animated:YES];
}

- (void) setState:(BOOL) enabled animated:(BOOL) animated{
	[switchView setOn:enabled animated:animated];
}

- (void) setLabel:(NSString *) text {
	[labelView setText:text];
}

- (BOOL) getState {
	return [switchView isOn];
}

- (void) setDelegate:(id) delegate {
	[switchView addTarget:delegate action:@selector(switchToggle:) forControlEvents:UIControlEventValueChanged];
}

- (void)dealloc
{
	[labelView release];
	[switchView release];
	
	[super dealloc];
}


@end
