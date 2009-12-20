/* iphoneRealtimeAlertAdd.h - iPhone new alert view
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

#ifndef __IPHONEREALTIMEALERTADD__H
#define __IPHONEREALTIMEALERTADD__H

@interface AlertScrollView : UIScrollView {
	
}


@end


@interface AlertAddView : UIViewController <UIScrollViewDelegate, UITextFieldDelegate> {
	int            initialized;
	int            alertType;
	int            alertDirection;
	int            isTakingImage;
   int            isHidden;
	UIView         *buttonDirectionView;
	UITextField    *commentEditbox;
	UIView         *collapsedView;
	UIImage        *iconImage;
	UIImage        *minimizedImage;
	UIButton       *imageButton;
	UIButton       *moodButton;
	UIButton       *hideButton;
   UIButton       *cancelButton;
	UIImageView    *animatedImageView;
   UIImageView    *bgFrame;
   UIButton       *bgFrameButton;
}

@property (nonatomic, retain) UITextField    *commentEditbox;
@property (nonatomic, retain) UIView         *collapsedView;
@property (nonatomic, retain) UIImage        *iconImage;
@property (nonatomic, retain) UIImage        *minimizedImage;
@property (nonatomic, retain) UIButton       *imageButton;
@property (nonatomic, retain) UIButton       *moodButton;
@property (nonatomic, retain) UIButton       *hideButton;
@property (nonatomic, retain) UIButton       *cancelButton;
@property (nonatomic, retain) UIImageView    *animatedImageView;
@property (nonatomic, retain) UIImageView    *bgFrame;
@property (nonatomic, retain) UIButton       *bgFrameButton;

- (void) onCancelHide;
- (void) periodicHide;
- (void) onHide;
- (void) showWithType: (int)alert_type;


@end


#endif // __IPHONEREALTIMEALERTADD__H