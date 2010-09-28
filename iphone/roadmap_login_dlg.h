/* roadmap_login_dlg.h - iPhone login class
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

#ifndef __ROADMAP_LOGIN_DLG__H
#define __ROADMAP_LOGIN_DLG__H

#include "widgets/iphoneCellEdit.h"

@interface NewExistingDialog : UIViewController <UIScrollViewDelegate> {
}

- (void) show;

@end


@interface LoginDialog : UITableViewController <UITextFieldDelegate> {
	NSMutableArray				*dataArray;
   iphoneCellEdit          *gUsernameEditCell;
   iphoneCellEdit          *gPasswordEditCell;
   iphoneCellEdit          *gNicknameEditCell;
}

@property (nonatomic, retain) NSMutableArray	*dataArray;
@property (nonatomic, retain) iphoneCellEdit	*gUsernameEditCell;
@property (nonatomic, retain) iphoneCellEdit	*gPasswordEditCell;
@property (nonatomic, retain) iphoneCellEdit	*gNicknameEditCell;

- (void) showLogin;
- (void) showUnPw;

@end


@interface LoginUpdateDialog : UIViewController <UIScrollViewDelegate, UITextFieldDelegate, UISearchBarDelegate> {
   BOOL  kbIsOn;
}

- (void) showUpdateLogin;

@end


#endif // __ROADMAP_LOGIN_DLG__H