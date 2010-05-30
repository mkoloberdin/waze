/* iphoneCellSelect.h - iPhone select type cell
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi R.
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

#ifndef __IPHONECELLSELECT__H
#define __IPHONECELLSELECT__H

#include "iphoneLabel.h"


@interface iphoneCellSelect : UITableViewCell {
	iphoneLabel *labelView;
	UISegmentedControl *segmentedView;
}

@property (nonatomic, retain) iphoneLabel *labelView;
@property (nonatomic, retain) UISegmentedControl *segmentedView;


- (void) setItems:(NSArray*) segmentsArray;
- (int) getItem;
- (void) setLabel:(NSString *) text;
- (void) setSelectedSegment:(int)index;
- (void) setDelegate:(id) delegate;

@end

#endif // __IPHONECELLSELECT__H