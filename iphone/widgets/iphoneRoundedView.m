/* ssd_progress_msg_dialog.m - rounded (corners) view for iPhone
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi R.
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
 *
 */

#include "iphoneRoundedView.h"


@implementation iphoneRoundedView
@synthesize rectColor;

-(void) CGContextMakeRoundedCorners: (CGContextRef) c rect: (CGRect) rect radius: (int) corner_radius
{
   int x_left = rect.origin.x;  
   int x_left_center = rect.origin.x + corner_radius;  
   int x_right_center = rect.origin.x + rect.size.width - corner_radius;  
   int x_right = rect.origin.x + rect.size.width;  
   int y_top = rect.origin.y;  
   int y_top_center = rect.origin.y + corner_radius;  
   int y_bottom_center = rect.origin.y + rect.size.height - corner_radius;  
   int y_bottom = rect.origin.y + rect.size.height;
   
   CGContextClearRect(c, rect);
	
   CGContextBeginPath(c);  
   CGContextMoveToPoint(c, x_left, y_top_center);  
	
   CGContextAddArcToPoint(c, x_left, y_top, x_left_center, y_top, corner_radius);  
   CGContextAddLineToPoint(c, x_right_center, y_top);
	
	CGContextAddArcToPoint(c, x_right, y_top, x_right, y_top_center, corner_radius);  
	
   CGContextAddArcToPoint(c, x_right, y_bottom, x_right_center, y_bottom, corner_radius);  
   CGContextAddLineToPoint(c, x_left_center, y_bottom);  
	
   CGContextAddArcToPoint(c, x_left, y_bottom, x_left, y_bottom_center, corner_radius);  
   CGContextAddLineToPoint(c, x_left, y_top_center);  
	
   CGContextClosePath(c);
	
	CGContextSetFillColorWithColor(c, [self rectColor].CGColor);
	CGContextDrawPath(c, kCGPathFill);
}

-(void) drawRect: (CGRect) rect
{
   CGContextRef c = UIGraphicsGetCurrentContext();
   [self CGContextMakeRoundedCorners:c rect:rect radius:10];
}

@end
