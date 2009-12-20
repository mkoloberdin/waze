/* roadmap_foursquare_dialog.h - iPhone foursquare settings dialog
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

#ifndef __ROADMAP_FOURSQUARE_DIALOG__H
#define __ROADMAP_FOURSQUARE_DIALOG__H


@interface FoursquareCheckedDialog : UIViewController <UIScrollViewDelegate> {
   
}

- (void) showFoursquareCheckedin;


@end

@interface FoursquareLoginDialog : UITableViewController <UITextFieldDelegate> {
	NSMutableArray				*dataArray;
   NSMutableArray				*headersArray;
   NSMutableArray				*footersArray;
}

@property (nonatomic, retain) NSMutableArray	*dataArray;
@property (nonatomic, retain) NSMutableArray	*headersArray;
@property (nonatomic, retain) NSMutableArray	*footersArray;

- (void) showFoursquareLogin;

@end


#endif // __ROADMAP_FOURSQUARE_DIALOG__H