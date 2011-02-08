/* iphoneSelect.h - iPhone select custom control
 *
 * LICENSE:
 *
 *   Copyright 20109 Avi R.
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

#ifndef __IPHONESELECT__H
#define __IPHONESELECT__H

#define IPHONE_SELECT_TYPE_TEXT        0
#define IPHONE_SELECT_TYPE_IMAGE       1
#define IPHONE_SELECT_TYPE_TEXT_IMAGE  2

@interface iphoneSelect : UIView {
	id    gDelegate;
   int   gIndex;
}

- (void) createWithItems:(NSArray*) segmentsArray selectedSegment:(int)index delegate:(id) delegate type:(int)type;
- (int) getSelectedSegment;
- (int) setSelectedSegment:(int)index;

@end

#endif // __IPHONESELECT__H