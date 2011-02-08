/* roadmap_iphonerecorder.h - iPhone recorder view
 *
 * LICENSE:
 *
 *   Copyright 2010 Avi R.
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

#ifndef __ROADMAP_IPHONERECORDER__H
#define __ROADMAP_IPHONERECORDER__H


#include "roadmap_recorder.h"


@interface RoadMapRecorderView : UIView {
   UIView                  *recorderView;
   UIImageView             *backgroundImage;
   UILabel                 *textLabel;
   UILabel                 *notificationLabel;
   UIImageView             *timerBg;
   UILabel                 *timerDigits;
   UIProgressView          *timerProgress;
   UIButton                *recordButton;
   UIButton                *cancelButton;
   UIButton                *doneButton;
   NSTimer                 *refreshTimer;
   NSTimer                 *countdownTimer;
   NSString                *gFileName;
   int                     gTimeout;
   int                     gState;
   uint32_t                gStartTime;
   int                     gCountdownCounter;
   
}


- (void) showWithText: (const char*) text
           andTimeout: (int) timeout
         andAutostart: (int) autoStart
              andPath: (const char*) path
          andFileName: (const char*) file_name;

@end


#endif // __ROADMAP_IPHONERECORDER__H