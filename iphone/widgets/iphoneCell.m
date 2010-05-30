/* iphoneCell.m - Generic iPhone cell
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

#include "roadmap_lang.h"
#include "iphoneCell.h"

#define X_MARGIN 15.0f


@implementation iphoneCell
@synthesize rightLabel;
@synthesize leftLabel;


- (id)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier
{
   self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];
	
	rightLabel = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, 5, 5)];
   [rightLabel setBackgroundColor:[UIColor clearColor]];
   [rightLabel setTextColor:[UIColor colorWithRed:.22f green:.329f blue:.529f alpha:1.0f]];
   [rightLabel setFont:[UIFont boldSystemFontOfSize:16]];
   [self.contentView addSubview:rightLabel];
   
   leftLabel = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, 5, 5)];
   [leftLabel setBackgroundColor:[UIColor clearColor]];
   [leftLabel setTextColor:[UIColor clearColor]];
   [leftLabel setFont:[UIFont boldSystemFontOfSize:17]];
   [self.contentView addSubview:leftLabel];
	
   self.selectionStyle = UITableViewCellSelectionStyleNone;
	
	return self;
}

- (void)setHighlighted:(BOOL)highlighted animated:(BOOL)animated
{
	[super setHighlighted:highlighted animated:animated];
   
	if (highlighted) {
      self.backgroundColor = [UIColor colorWithRed:0.647 green:0.847 blue:0 alpha:1];
   } else
      self.backgroundColor = [UIColor whiteColor];
}

- (void) layoutSubviews {
	[super layoutSubviews];
	
	if (!rightLabel)
		return;
	
	CGRect frame = [[self contentView] bounds];
	CGRect rect;
   
	[rightLabel sizeToFit];
   rect = rightLabel.frame;
   rect.origin.x = frame.size.width - X_MARGIN - rightLabel.bounds.size.width;
   rect.origin.y = (frame.size.height - rightLabel.bounds.size.height)/2;
   
	[rightLabel setFrame:rect];
   [self.contentView bringSubviewToFront:rightLabel];
   
   if (rect.size.width > 0) {
      rect = self.textLabel.frame;
      rect.size.width -= rightLabel.bounds.size.width + 20;
      self.leftLabel.frame = rect;
      
      self.leftLabel.text = self.textLabel.text;
      self.leftLabel.textColor = self.textLabel.textColor;
      self.textLabel.hidden = TRUE;
      [self.contentView bringSubviewToFront:leftLabel];
   } else {
      self.leftLabel.hidden = TRUE;
   }
   
   if (roadmap_lang_rtl()) {
      if (self.textLabel.textAlignment != UITextAlignmentCenter) {
         self.textLabel.textAlignment = UITextAlignmentRight;
         self.leftLabel.textAlignment = UITextAlignmentRight;
      }
   }
    
}

- (void)dealloc
{
   if (rightLabel)
      [rightLabel release];
   
   if (leftLabel)
      [leftLabel release];
   
	[super dealloc];
}

@end
