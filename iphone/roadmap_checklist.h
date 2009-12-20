/* roadmap_checklist.h - iPhone checklist view
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

#ifndef __ROADMAP_CHECKLIST__H
#define __ROADMAP_CHECKLIST__H

typedef void (*ChecklistCallback) (int value, int group);

@interface RoadMapChecklist : UITableViewController {
	ChecklistCallback    gCallback;
	NSString             *gTitle;
	NSArray              *dataArray;
   NSArray              *headersArray;
}

@property (nonatomic) ChecklistCallback	gCallback;
@property (nonatomic, retain) NSArray     *dataArray;
@property (nonatomic, retain) NSArray     *headersArray;
@property (nonatomic, retain) NSString    *gTitle;



- (id) activateWithTitle:(NSString *)title
                 andData:(NSArray *)array
              andHeaders:(NSArray *)headers
             andCallback:(ChecklistCallback)callback;

- (void) show;

- (int) valueForGroup: (NSNumber *)group;

@end


#endif // __ROADMAP_CHECKLIST__H