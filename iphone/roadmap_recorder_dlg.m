/* roadmap_recorder_dlg.m - iphone recorder dialog.
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
 *
 * SYNOPSYS:
 *
 *   See roadmap_recorder.h
 */

#include <stdlib.h>
#include "roadmap.h"
#include "roadmap_main.h"
#include "roadmap_lang.h"
#include "roadmap_sound.h"
#include "roadmap_iphonemain.h"
#include "roadmap_res.h"
#include "ssd_dialog.h"
#include "roadmap_time.h"
#include "roadmap_recorder.h"
#include "roadmap_iphonerecorder.h"


#define MAX_RECORDER_TIME              (60 * 1000)
#define RECORDER_COUNTDOWN             3 //seconds

#define RECORDER_TEXT_HEIGHT           15.0f
#define RECORDER_BUTTON_HEIGHT         35.0f
#define RECORDER_BUTTON_RECORD_WIDTH   100.0f
#define RECORDER_BUTTON_CANCEL_WIDTH   85.0f
#define RECORDER_BUTTON_DONE_WIDTH     85.0f
#define RECORDER_LINE_SPACE            10.0f
#define RECORDER_MARGIN                10.0f

enum states {
   recorder_init,
   recorder_autostart,
   recorder_recording,
   recorder_stopped
};

static RoadMapRecorderView *gRecorderView = NULL;
static recorder_closed_cb  gRecorderCb = NULL;
static void                *gRecorderContext = NULL;


void roadmap_recorder (const char *text, int seconds, recorder_closed_cb on_recorder_closed, int auto_start, const char* path, const char* file_name, void *context) {
   if (gRecorderView) {
		return;
	}
   
   gRecorderCb = on_recorder_closed;
   gRecorderContext = context;
   
   gRecorderView = [[RoadMapRecorderView alloc] initWithFrame:CGRectZero];
   [gRecorderView showWithText:text andTimeout:seconds andAutostart:auto_start andPath:path andFileName:file_name];
}

static void recorder_closed (int exit_code) {
   gRecorderView = NULL;
   
   if (gRecorderCb) {
      gRecorderCb (exit_code, gRecorderContext);
      gRecorderCb = NULL;
   }
}


////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
@implementation RoadMapRecorderView

- (void) resizeViews
{
   CGRect rect;
   int maxHeight = self.bounds.size.height - 80;
   int maxWidth = 310;
   int textHeight = 0;
   int textSpace = 0;
   int buttonSpace;
   
   if (!recorderView || !backgroundImage)
      return;
   
   if (textLabel && [textLabel.text length] > 0) {
      textHeight = RECORDER_TEXT_HEIGHT;
      textSpace = RECORDER_LINE_SPACE;
   }
   
   rect = CGRectMake(20, 0, maxWidth, self.bounds.size.height);
   recorderView.frame = rect;
   
   //notification label size
   rect = CGRectMake(RECORDER_MARGIN, RECORDER_LINE_SPACE,
                     recorderView.bounds.size.width - 2* RECORDER_MARGIN, recorderView.bounds.size.height);
   notificationLabel.frame = rect;
   [notificationLabel sizeToFit];
   rect = notificationLabel.frame;
   if (notificationLabel.bounds.size.height + 3* RECORDER_LINE_SPACE +
       RECORDER_BUTTON_HEIGHT + textHeight + textSpace > maxHeight) {
      rect.size.height = maxHeight - 3* RECORDER_LINE_SPACE -
      RECORDER_BUTTON_HEIGHT - textHeight - textSpace;
   }
   //rect.origin.y = textSpace + textHeight + RECORDER_LINE_SPACE;
   rect.origin.x = (recorderView.bounds.size.width - rect.size.width)/2;
   notificationLabel.frame = rect;
   
   //text label size
   //if (textHeight > 0) {
      rect = CGRectMake(RECORDER_MARGIN, notificationLabel.frame.origin.y + notificationLabel.frame.size.height + textSpace,
                        recorderView.bounds.size.width - 2* RECORDER_MARGIN, textHeight);
      textLabel.frame = rect;
   //}
   
   //timer bg
   rect = CGRectMake((recorderView.bounds.size.width - timerBg.bounds.size.width)/2,
                     textLabel.frame.origin.y + textLabel.frame.size.height + RECORDER_LINE_SPACE,
                     timerBg.bounds.size.width, timerBg.bounds.size.height);
   timerBg.frame = rect;
   
   //timer digits
   rect = CGRectMake(0, 10,
                     timerBg.frame.size.width, timerBg.frame.size.height - 30);
   timerDigits.frame = rect;
   
   //timer progress bar
   rect = CGRectMake(20,
                     timerBg.frame.size.height - (20 + 2),
                     timerBg.frame.size.width - 40,
                     20);
   timerProgress.frame = rect;
   
   
   //final message size
   rect = CGRectMake(0, 0, maxWidth, 
                     notificationLabel.bounds.size.height + 5* RECORDER_LINE_SPACE +
                     RECORDER_BUTTON_HEIGHT + textHeight + textSpace +
                     timerBg.bounds.size.height);
   recorderView.bounds = rect;
   recorderView.center = self.center;
   
   
   buttonSpace = (recorderView.bounds.size.width -
                  RECORDER_BUTTON_RECORD_WIDTH - RECORDER_BUTTON_CANCEL_WIDTH - RECORDER_BUTTON_DONE_WIDTH) / 4;
   
   //Record button size
   rect = CGRectMake(buttonSpace, 
                     recorderView.bounds.size.height - RECORDER_LINE_SPACE*2 - RECORDER_BUTTON_HEIGHT,
                     RECORDER_BUTTON_RECORD_WIDTH,
                     RECORDER_BUTTON_HEIGHT);
   recordButton.frame = rect;
   //Cancel button size
   rect = CGRectMake(buttonSpace*2 + RECORDER_BUTTON_RECORD_WIDTH, 
                     recorderView.bounds.size.height - RECORDER_LINE_SPACE*2 - RECORDER_BUTTON_HEIGHT,
                     RECORDER_BUTTON_CANCEL_WIDTH,
                     RECORDER_BUTTON_HEIGHT);
   cancelButton.frame = rect;
   //Done button size
   rect = CGRectMake(buttonSpace*3 + RECORDER_BUTTON_RECORD_WIDTH + RECORDER_BUTTON_CANCEL_WIDTH, 
                     recorderView.bounds.size.height - RECORDER_LINE_SPACE*2 - RECORDER_BUTTON_HEIGHT,
                     RECORDER_BUTTON_DONE_WIDTH,
                     RECORDER_BUTTON_HEIGHT);
   doneButton.frame = rect;
   
   //background image size
   backgroundImage.frame = recorderView.bounds;
}

- (void) onCountdownTimeout
{
	if (--gCountdownCounter  ==  0) {
      [self removeCountdownTimer];
      if (gState == recorder_autostart) {
         [self performSelector:@selector(onRecord)];
      } else if (gState == recorder_stopped) {
         [self performSelector:@selector(onDone)];
      }
	} else {
		[self refreshTimerProgress];
	}
	
}

- (void) setCountdownTimer
{
   if (countdownTimer) return;

   if (gState == recorder_autostart)
      gCountdownCounter = RECORDER_COUNTDOWN;
   else
      gCountdownCounter = 1;
   
   countdownTimer = [NSTimer scheduledTimerWithTimeInterval: 1.0f
                                                     target: self
                                                   selector: @selector (onCountdownTimeout)
                                                   userInfo: nil
                                                    repeats: YES];
   
   [self refreshTimerProgress];
}

- (void) removeCountdownTimer
{
   if (countdownTimer) {
      [countdownTimer invalidate];
      countdownTimer = NULL;
   }
}

- (void) setRefreshTimer
{
   if (refreshTimer) return;
   
   refreshTimer = [NSTimer scheduledTimerWithTimeInterval: 0.1
                                                   target: self
                                                 selector: @selector (onRefreshTimeout)
                                                 userInfo: nil
                                                  repeats: YES];
}

- (void) removeRefreshTimer
{
   if (refreshTimer) {
      [refreshTimer invalidate];
      refreshTimer = NULL;
   }
}

- (void) onDone
{   
   if (gState == recorder_recording)
      [self onRecord];
   
   //kill timer if exists
	[self removeRefreshTimer];
   [self removeCountdownTimer];
   
   [self removeFromSuperview];
   recorder_closed (dec_yes);
	
	[self release];
}

- (void) onRecordToggle
{
   UIImage *image;
   RoadMapSoundList     list  = NULL;
   
   doneButton.enabled = YES;
   
   if (gState == recorder_recording) {
      [recordButton setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Record")] forState:UIControlStateNormal];
      roadmap_sound_stop_recording();
      gState = recorder_stopped;
      timerProgress.progress = 1.0f;
      [self removeRefreshTimer];
      [self setCountdownTimer];
      
      image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "recorder_record");
      
      list = roadmap_sound_list_create (SOUND_LIST_NO_FREE);
      roadmap_sound_list_add (list, "rec_end");
      roadmap_sound_play_list (list);
   } else {
      [self removeCountdownTimer];
      
      if (gState == recorder_stopped) {
         roadmap_file_remove(NULL, [gFileName UTF8String]);
      }
      [recordButton setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Stop")] forState:UIControlStateNormal];
      
      roadmap_sound_record([gFileName UTF8String], gTimeout);
      gStartTime = roadmap_time_get_millis();
      [self setRefreshTimer];
      
      gState = recorder_recording;
      
      [doneButton setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Use")] forState:UIControlStateNormal];
      image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "recorder_stop");
   }
   
   switch (gState) {
      case recorder_init:
         notificationLabel.text = [NSString stringWithUTF8String:roadmap_lang_get("Tap \"Record\" when ready")];
         break;
      case recorder_recording:
         notificationLabel.text = [NSString stringWithUTF8String:roadmap_lang_get("Recording...")];
         break;
      case recorder_stopped:
         notificationLabel.text = [NSString stringWithUTF8String:roadmap_lang_get("Recording complete")];
         break;
      default:
         break;
   }
   
   [recordButton setImage:image forState:UIControlStateNormal];
   
   [self resizeViews];
}

- (void) onRecord
{
    NSTimer *timer;
   RoadMapSoundList     list  = NULL;
   
   if (gState == recorder_recording) {
      [self onRecordToggle];
   } else {
      list = roadmap_sound_list_create (SOUND_LIST_NO_FREE);
      roadmap_sound_list_add (list, "rec_start");
      roadmap_sound_play_list (list);
      timer = [NSTimer scheduledTimerWithTimeInterval: 0.3f
                                               target: self
                                             selector: @selector (onRecordToggle)
                                             userInfo: nil
                                              repeats: NO];
   }

   
}

- (void) onCancel
{
   if (gState == recorder_recording)
      [self onRecord];
   
   //kill timer if exists
	[self removeRefreshTimer];
   [self removeCountdownTimer];
   
   if (gState == recorder_stopped) {
      roadmap_file_remove(NULL, [gFileName UTF8String]);
   }
   
   [self removeFromSuperview];
   recorder_closed (dec_no);
   
	[self release];
}

- (void) onTimeout
{
   if (gState == recorder_recording)
      [self onRecord];
}

- (void) refreshTimerProgress
{
   int elapsed_time, minutes, seconds;
   
   if (gState == recorder_stopped) {
      char text[64];
      snprintf(text, sizeof(text), "%s (%d)",
               roadmap_lang_get("Use"),
               gCountdownCounter);
      [doneButton setTitle:[NSString stringWithUTF8String:text] forState:UIControlStateNormal];
      return;
   }
   
   if (gState == recorder_autostart) {
      char text[128];
      snprintf(text, sizeof(text), "%s %d %s...",
               roadmap_lang_get("Recording in"),
               gCountdownCounter,
               roadmap_lang_get("seconds"));
      notificationLabel.text = [NSString stringWithUTF8String:text];
   }
   
   if (gState == recorder_init ||
       gState == recorder_autostart) {
      elapsed_time = minutes = seconds = 0;
   } else {
      elapsed_time = roadmap_time_get_millis() - gStartTime;
      minutes = elapsed_time / (60 * 1000);
      seconds = elapsed_time / 1000 - minutes * 60;
   }
   
   timerDigits.text = [NSString stringWithFormat:@"%02d:%02d", minutes, seconds];
   timerProgress.progress = 1.0f * elapsed_time / gTimeout;
}

- (void) onRefreshTimeout
{
	if (roadmap_time_get_millis() - gStartTime > gTimeout)
		[self performSelector:@selector(onTimeout)];
	else {
		[self refreshTimerProgress];
	}
	
}

- (void) layoutSubviews
{
   [self resizeViews];
}

- (UIButton *) newButton
{
   UIImage *image = NULL;
   UIButton *button = [UIButton buttonWithType: UIButtonTypeCustom];
   image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "button_small_up");
   if (image) {
      [button setBackgroundImage:[image stretchableImageWithLeftCapWidth:10 topCapHeight:10]
                          forState:UIControlStateNormal];
   }
   image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "button_small_down");
   if (image) {
      [button setBackgroundImage:[image stretchableImageWithLeftCapWidth:10 topCapHeight:10]
                          forState:UIControlStateHighlighted];
   }

   [button setTitleEdgeInsets:UIEdgeInsetsMake(5, 5, 5, 5)];
   button.titleLabel.numberOfLines = 2;
   [button setTitleColor:[UIColor colorWithRed:1.0f green:1.0f blue:1.0f alpha:1.0f] forState:UIControlStateNormal];
   button.titleLabel.font = [UIFont systemFontOfSize:16];
   button.titleLabel.lineBreakMode = UILineBreakModeWordWrap;
   button.titleLabel.textAlignment = UITextAlignmentCenter;
   
   return button;
}

- (void) createWithText: (const char*) text
{
   UIImage *image;
   CGRect rect;
   
   roadmap_main_get_bounds(&rect);
   [self setFrame:rect];
   //[self setBackgroundColor:[UIColor clearColor]];
   //[self setAutoresizesSubviews:YES];
   //[self setAutoresizingMask:UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth];
   self.backgroundColor = [UIColor colorWithWhite:0.2f alpha:0.7f];
   self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
   self.autoresizesSubviews = YES;
   
   //recorder view
   recorderView = [[UIView alloc] initWithFrame:CGRectZero];
   [self addSubview:recorderView];
   [recorderView release];
   
   //background image
   image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "message_box");
   if (image) {
      backgroundImage = [[UIImageView alloc] initWithImage:[image stretchableImageWithLeftCapWidth:30
                                                                                      topCapHeight:40]];
      [recorderView addSubview:backgroundImage];
      [backgroundImage release];
   }
   
   //notification
   notificationLabel = [[UILabel alloc] initWithFrame:CGRectZero];
   notificationLabel.text = [NSString stringWithUTF8String:roadmap_lang_get("Tap \"Record\" when ready")];
   [notificationLabel setBackgroundColor:[UIColor clearColor]];
   [notificationLabel setTextColor:[UIColor whiteColor]];
   [notificationLabel setFont:[UIFont boldSystemFontOfSize:20]];
   [notificationLabel setTextAlignment:UITextAlignmentCenter];
   [notificationLabel setAutoresizingMask:UIViewAutoresizingFlexibleBottomMargin];
   [recorderView addSubview:notificationLabel];
   [notificationLabel release];
   
   //text
   //if ((text) && strlen(text) > 0) {
      textLabel = [[UILabel alloc] initWithFrame:CGRectZero];
      if (!text)
         [textLabel setText:[NSString stringWithUTF8String:""]];
      else
         [textLabel setText:[NSString stringWithUTF8String:roadmap_lang_get(text)]];
      [textLabel setBackgroundColor:[UIColor clearColor]];
      [textLabel setTextColor:[UIColor whiteColor]];
      [textLabel setFont:[UIFont systemFontOfSize:15]];
      [textLabel setTextAlignment:UITextAlignmentCenter];
      [textLabel setAdjustsFontSizeToFitWidth:YES];
      [recorderView addSubview:textLabel];
      [textLabel release];
   //} else {
      //textLabel = NULL;
  //}
   
   //timer background
   image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "recorder_timer_bg");
   timerBg = [[UIImageView alloc] initWithImage:image];
   [recorderView addSubview:timerBg];
   [timerBg release];
   
   //timer digits
   timerDigits = [[UILabel alloc] initWithFrame:CGRectZero];
   [timerDigits setBackgroundColor:[UIColor clearColor]];
   [timerDigits setTextAlignment:UITextAlignmentCenter];
   //[timerDigits setAutoresizingMask:UIViewAutoresizingFlexibleBottomMargin];
   [timerDigits setFont:[UIFont boldSystemFontOfSize:30]];
   [timerBg addSubview:timerDigits];
   [timerDigits release];
    
    //timer progress bar
    timerProgress = [[UIProgressView alloc] initWithProgressViewStyle:UIProgressViewStyleDefault];
   [timerBg addSubview:timerProgress];
   [timerProgress release];    
   
   //record button
   recordButton = [self newButton];
   [recordButton addTarget:self action:@selector(onRecord) forControlEvents:UIControlEventTouchUpInside];
   [recordButton setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Record")] forState:UIControlStateNormal];
   image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "recorder_record");
   [recordButton setImage:image forState:UIControlStateNormal];
   recordButton.imageEdgeInsets = UIEdgeInsetsMake(0, 0, 0, 5);
   [recorderView addSubview: recordButton];
   recordButton.hidden = YES; //currently disabled
   
   //cancel button
   cancelButton = [self newButton];
   [cancelButton addTarget:self action:@selector(onCancel) forControlEvents:UIControlEventTouchUpInside];
   [cancelButton setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Cancel")] forState:UIControlStateNormal];
   [recorderView addSubview: cancelButton];
   
   //done button
   doneButton = [self newButton];
   [doneButton addTarget:self action:@selector(onDone) forControlEvents:UIControlEventTouchUpInside];
   [doneButton setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Use")] forState:UIControlStateNormal];
   [doneButton setTitleColor:[UIColor grayColor] forState:UIControlStateDisabled];
   doneButton.enabled = NO;
   [recorderView addSubview: doneButton];
   doneButton.hidden = YES; //currently disables
   
   [self resizeViews];
}

- (void) showWithText: (const char*) text
           andTimeout: (int) timeout
         andAutostart: (int) autoStart
              andPath: (const char*) path
          andFileName: (const char*) file_name
{
 	gTimeout = timeout*1000;
   gStartTime = 0;
   gState = recorder_init;
   refreshTimer = NULL;
   gFileName = [[NSString alloc] initWithFormat:@"%s%s", path, file_name];

   [self createWithText:text];

   if (gTimeout <= 0 ||
       gTimeout > MAX_RECORDER_TIME)
      gTimeout = MAX_RECORDER_TIME;

   [self refreshTimerProgress];
   
   if (autoStart) {
      gState = recorder_autostart;
      [self setCountdownTimer];
      [self refreshTimerProgress];
   }

	roadmap_main_show_view(self);
}

- (void)dealloc
{
	if (gFileName)
      [gFileName release];
   
	[super dealloc];
}


@end

