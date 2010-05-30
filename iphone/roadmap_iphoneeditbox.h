/* roadmap_iphoneeditbox.h - iPhone editbox view
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

#ifndef __ROADMAP_IPHONEEDITBOX__H
#define __ROADMAP_IPHONEEDITBOX__H

#include "roadmap_editbox.h"

@interface EditBoxCell : UITableViewCell {
	
}

- (void) setText: (UITextView *) text;

@end


@interface RoadMapEditBox : UITableViewController <UITextViewDelegate> {
	UITextView *gTextView;
	SsdKeyboardCallback gCallback;
	void * gContext;
	TEditBoxType gBoxType;
   EditBoxCell *gCell;
	UIBarButtonItem *gDoneButton;
}

@property (nonatomic, retain) UITextView *gTextView;
@property (nonatomic) SsdKeyboardCallback gCallback;
@property (nonatomic) void * gContext;
@property (nonatomic) TEditBoxType gBoxType;
@property (nonatomic, retain) UIBarButtonItem *gDoneButton;
@property (nonatomic, retain) EditBoxCell *gCell;

- (void) showWithTitle: (const char*) aTitleUtf8
			   andText: (const char*) aTextUtf8
		   andCallback: (SsdKeyboardCallback) callback
			andContext: (void *)context
			   andType: (TEditBoxType) aBoxType;


@end


#endif // __ROADMAP_IPHONEEDITBOX__H