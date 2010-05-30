/* roadmap_social_dialog.h - iPhone social networks settings dialog
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

#ifndef __ROADMAP_SOCIAL_DIALOG__H
#define __ROADMAP_SOCIAL_DIALOG__H



@interface TwitterDialog : UITableViewController <UITextFieldDelegate> {
	NSMutableArray				*dataArray;
   NSMutableArray				*headersArray;
   NSMutableArray				*footersArray;
   BOOL                    isTwitter;
}

@property (nonatomic, retain) NSMutableArray	*dataArray;
@property (nonatomic, retain) NSMutableArray	*headersArray;
@property (nonatomic, retain) NSMutableArray	*footersArray;

- (void) refreshConnection;
- (void) showTwitter;
- (void) showFacebook;

@end


#endif // __ROADMAP_SOCIAL_DIALOG__H