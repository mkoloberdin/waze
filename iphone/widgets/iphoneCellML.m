/* iphoneCellML.m - Create multiline cell for iPhone table
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

#include "iphoneCellML.h"

#define X_MARGIN 10.0f
#define Y_MARGIN 5.0f
#define MAX_IMAGE_HEIGHT 50.0f
#define MAX_IMAGE_WIDTH 60.0f


@implementation iphoneCellML
@synthesize bgView;
@synthesize imageView;
@synthesize labelView;

- (id)initWithFrame:(CGRect)frame reuseIdentifier:(NSString *)reuseIdentifier
{
	self = [super initWithFrame:frame reuseIdentifier:reuseIdentifier];
	
	bgView = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 5, 5)];
	
	labelView = [[iphoneLabel alloc] initWithFrame:frame];
	[labelView setFont:[UIFont boldSystemFontOfSize:18]];
	[labelView setNumberOfLines:2];
	[[self contentView] addSubview:labelView];
	
	imageView = NULL; //This one may be supported in the future
	
	return self;
}

- (void) layoutSubviews {
	[super layoutSubviews];
	
	if (!imageView && !labelView)
		return;
	
	CGRect frame = [[self contentView] bounds];
	CGRect rect;
	
	rect = CGRectMake(X_MARGIN, Y_MARGIN, (frame.size.width/2) - X_MARGIN*1.5, frame.size.height - Y_MARGIN*2);
	if (imageView) {
		[imageView setFrame:rect];
	}
	
	rect = CGRectMake(X_MARGIN + MAX_IMAGE_WIDTH, Y_MARGIN, frame.size.width - X_MARGIN*2 - MAX_IMAGE_WIDTH, frame.size.height - Y_MARGIN*2);	
	if (labelView) {
		[labelView setFrame:rect];
	}
}

- (void)setSelected:(BOOL)selected animated:(BOOL)animated
{
	[super setSelected:selected animated:animated];
	
	bgView.backgroundColor = [UIColor colorWithRed:0.647 green:0.847 blue:0 alpha:1];
	[self setSelectedBackgroundView:bgView];
	[self setSelectedTextColor:[UIColor blackColor]];
}

- (void)setText:(NSString *) text
{
	labelView.text = [NSString stringWithString: text];
}

- (void)dealloc
{
	[bgView release];
	
	[super dealloc];
}

@end
