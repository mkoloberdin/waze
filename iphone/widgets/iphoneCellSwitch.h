/* iphoneCellSwitch.h - iPhone switch type cell
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

#ifndef __IPHONECELLSWITCH__H
#define __IPHONECELLSWITCH__H

#include "iphoneLabel.h"


@interface iphoneCellSwitch : UITableViewCell {
	iphoneLabel *labelView;
	UISwitch *switchView;
}

@property (nonatomic, retain) iphoneLabel *labelView;
@property (nonatomic, retain) UISwitch *switchView;


- (void) setState:(BOOL) enabled;
- (void) setState:(BOOL) enabled animated:(BOOL) animated;
- (BOOL) getState;
- (void) setLabel:(NSString *) text;
- (void) setDelegate:(id) delegate;

@end

#endif // __IPHONECELLSWITCH__H