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

#include "widgets/iphoneSelect.h"
#include "widgets/iphoneLabel.h"

@interface AlertScrollView : UIScrollView {
	
}


@end


@interface AlertAddView : UIViewController <UIScrollViewDelegate, UITextFieldDelegate> {
	int            initialized;
	int            alertType;
	int            alertDirection;
   int            alertSubType;
	int            isTakingImage;
   int            isHidden;
   int            isRestoreFocuse;
   iphoneSelect   *subtypeControl;
   iphoneSelect   *directionControl;
	UITextField    *commentEditbox;
	UIView         *collapsedView;
	UIImage        *iconImage;
	UIImage        *minimizedImage;
	UIButton       *imageButton;
   UIButton       *recordButton;
   iphoneLabel    *groupNote;
	UIButton       *groupButton;
   UIButton       *sendButton;
	UIImageView    *animatedImageView;
   UIImageView    *bgFrame;
   UIButton       *bgFrameButton;
   iphoneLabel    *categoryLabel;
   iphoneLabel    *cameraText1;
   iphoneLabel    *cameraText2;
}

@property (nonatomic, retain) iphoneSelect   *subtypeControl;
@property (nonatomic, retain) iphoneSelect   *directionControl;
@property (nonatomic, retain) UITextField    *commentEditbox;
@property (nonatomic, retain) UIView         *collapsedView;
@property (nonatomic, retain) UIImage        *iconImage;
@property (nonatomic, retain) UIImage        *minimizedImage;
@property (nonatomic, retain) UIButton       *imageButton;
@property (nonatomic, retain) iphoneLabel    *groupNote;
@property (nonatomic, retain) UIButton       *groupButton;
@property (nonatomic, retain) UIButton       *sendButton;
@property (nonatomic, retain) UIImageView    *animatedImageView;
@property (nonatomic, retain) UIImageView    *bgFrame;
@property (nonatomic, retain) UIButton       *bgFrameButton;
@property (nonatomic, retain) iphoneLabel    *categoryLabel;
@property (nonatomic, retain) iphoneLabel    *cameraText1;
@property (nonatomic, retain) iphoneLabel    *cameraText2;

- (void) onCancelHide;
- (void) onRecorderClosed;
- (void) periodicHide;
- (void) onHide;
- (void) showWithType: (int)alert_type;
- (void) showCameraDialog;


@end


#endif // __IPHONEREALTIMEALERTADD__H