/* iphoneTableFooter.m - iPhone table footer
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


#include "roadmap.h"
#include "roadmap_main.h"
#include "roadmap_iphonemain.h"
#include "iphoneLabel.h"
#include "roadmap_lang.h"
#include "iphoneTableFooter.h"

#define X_MARGIN 20
#define Y_MARGIN 5


@implementation iphoneTableFooter

- (id)initWithFrame:(CGRect)aRect
{
   iphoneLabel *label;
   CGRect      rect;
   
	self = [super initWithFrame:aRect];
   
   self.backgroundColor = [UIColor clearColor];
   
   if (self) {
      rect = aRect;
      rect.origin.x = X_MARGIN;
      rect.size.width -= X_MARGIN *2;
      rect.origin.y = Y_MARGIN;
      rect.size.height -= Y_MARGIN *2;
      label = [[iphoneLabel alloc] initWithFrame:rect];
      label.textColor = [UIColor colorWithRed:0.0 green:0.0 blue:0.0 alpha:0.65];
      label.backgroundColor = [UIColor clearColor];
      label.font = [UIFont boldSystemFontOfSize:14];
      label.shadowColor = [UIColor colorWithRed:1 green:1 blue:1 alpha:.35];
      label.shadowOffset = CGSizeMake(0, 1.0);
      label.numberOfLines = 0;
      label.lineBreakMode = UILineBreakModeWordWrap;
      label.autoresizesSubviews = YES;
      label.autoresizingMask = UIViewAutoresizingFlexibleBottomMargin;
      label.tag = 1;
      
      if (roadmap_lang_rtl())
         label.textAlignment = UITextAlignmentRight;
      
      [self addSubview:label];
      [label release];
	}
	
	return self;
}

- (void)layoutSubviews
{
   CGRect mainRect;
   CGRect viewRect = self.frame;
   CGRect rect = self.bounds;
   iphoneLabel *label = (iphoneLabel *)[self viewWithTag:1];
   
   roadmap_main_get_bounds(&mainRect);
   rect.origin.x = X_MARGIN;
   rect.size.width = mainRect.size.width - X_MARGIN *2;
   rect.origin.y = Y_MARGIN;
   label.frame = rect;
   [label sizeToFit];
   
   viewRect.size.height = label.bounds.size.height + Y_MARGIN *2;
   
   self.frame = viewRect;
}


- (void)setText:(const char *)text
{
   iphoneLabel *label = (iphoneLabel *)[self viewWithTag:1];
   label.text = [NSString stringWithUTF8String:roadmap_lang_get(text)];
}

- (const NSString *) getText
{
   iphoneLabel *label = (iphoneLabel *)[self viewWithTag:1];
   return label.text;
}

@end
