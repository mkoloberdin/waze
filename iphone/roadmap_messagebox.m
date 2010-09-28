/* ssd_messagebox.m - iphone messagebox.
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
 *
 * SYNOPSYS:
 *
 *   See roadmap_messagebox.h
 */

#include <stdlib.h>
#include "roadmap.h"
#include "roadmap_lang.h"
#include "roadmap_main.h"
#include "roadmap_path.h"
#include "roadmap_bar.h"
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_confirm_dialog.h"

#include "roadmap_iphonemain.h"
#include "roadmap_res.h"
#include "roadmap_messagebox.h"
#include "roadmap_iphonemessagebox.h"


#define MESSAGE_BOX_IMAGE "message_box"
#define MESSAGE_BOX_TITLE_HEIGHT 25.0f
#define MESSAGE_BOX_BUTTON_HEIGHT 35.0f
#define MESSAGE_BOX_BUTTON_WIDTH 120.0f
#define MESSAGE_BOX_LINE_SPACE 15.0f
#define MESSAGE_BOX_MARGIN    10.0f

static RoadMapMessageBoxView *gMessageBox = NULL;
static RoadMapMessageBoxView *gConfirmBox = NULL;

////////////////////////////////////////////////////////
// roadmap_messagebox

static void roadmap_messagebox_timeout_cb (const char *title, const char *text, int seconds, messagebox_closed callback) {
	if (gMessageBox) {
		[gMessageBox close];
	}
   
   gMessageBox = [[RoadMapMessageBoxView alloc] initWithFrame:CGRectZero];
   [gMessageBox showWithTitle:title andContent:text andTimeout:seconds andCallback:callback];
}

void roadmap_messagebox_cb(const char *title, const char *message,
         messagebox_closed on_messagebox_closed)
{
   roadmap_messagebox_timeout_cb(title, message, 0, on_messagebox_closed);
}


void roadmap_messagebox (const char *title, const char *text) {
	roadmap_messagebox_timeout_cb(title, text, 0, NULL);
}


void roadmap_messagebox_timeout (const char *title, const char *text, int seconds) {
	roadmap_messagebox_timeout_cb(title, text, seconds, NULL);
}

static void messagebox_removed (void) {
   gMessageBox = NULL;
}

////////////////////////////////////////////////////////
// ssd_confirm_dialog

void ssd_confirm_dialog_custom_timeout (const char *title, const char *text, BOOL default_yes, 
                                        ConfirmDialogCallback callback, void *context,
                                        const char *textYes, const char *textNo, int seconds) {
   if (gConfirmBox)
      [gConfirmBox close];
   
   gConfirmBox = [[RoadMapMessageBoxView alloc] initWithFrame:CGRectZero];
   [gConfirmBox showConfirmWithTitle:title
                          andContent:text
                       andDefaultYes:default_yes
                         andCallback:callback
                          andContext:context
                          andTextYes:textYes
                           andTextNo:textNo
                          andTimeout:seconds];
}

void ssd_confirm_dialog_custom (const char *title, const char *text, BOOL default_yes, ConfirmDialogCallback callback, void *context,const char *textYes, const char *textNo) {
   ssd_confirm_dialog_custom_timeout (title, text, default_yes, callback, context, textYes, textNo, 0);
}


void ssd_confirm_dialog (const char *title, const char *text, BOOL default_yes, ConfirmDialogCallback callback, void *context) {
	ssd_confirm_dialog_custom(title, text, default_yes, callback, context, roadmap_lang_get("Yes"),roadmap_lang_get("No"));
}


void ssd_confirm_dialog_timeout (const char *title, const char *text, BOOL default_yes, ConfirmDialogCallback callback, void *context, int seconds) {
   ssd_confirm_dialog_custom_timeout (title, text, default_yes, callback, context, roadmap_lang_get("Yes"),roadmap_lang_get("No"), seconds);
}

void ssd_confirm_dialog_close (void) {
   if (gConfirmBox)
      [gConfirmBox close];
}

static void confirm_removed (void) {
   gConfirmBox = NULL;
}


////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
@implementation RoadMapMessageBoxView
@synthesize messageView;
@synthesize backgroundImage;
@synthesize titleLabel;
@synthesize contentLabel;
@synthesize okButton;
@synthesize yesButton;
@synthesize noButton;
@synthesize okTimer;


- (void) animateIn_3
{   
   [UIView beginAnimations:NULL context:NULL];
   [UIView setAnimationDuration:0.15f];
   self.transform = CGAffineTransformScale(CGAffineTransformIdentity, 1, 1);
   [UIView commitAnimations];
}

- (void) animateIn_2
{
   [UIView beginAnimations:NULL context:NULL];
   [UIView setAnimationDuration:0.15f];
   [UIView setAnimationDelegate:self];
   [UIView setAnimationDidStopSelector:@selector(animateIn_3)];
   self.transform = CGAffineTransformScale(CGAffineTransformIdentity, 0.9, 0.9);
   [UIView commitAnimations];
}

- (void) animateIn
{
   if (!contentLabel || !messageView || !backgroundImage)
      return;
      
   [UIView beginAnimations:NULL context:NULL];
   [UIView setAnimationDuration:0.15f];
   [UIView setAnimationDelegate:self];
   [UIView setAnimationDidStopSelector:@selector(animateIn_2)];
   self.transform = CGAffineTransformScale(CGAffineTransformIdentity, 1.1, 1.1);
   [UIView commitAnimations];
}

- (void) refreshTimerButton
{
   char str[40];
   
   sprintf(str, "%s (%d)", timerText, gTimeout);
   [timerButton setTitle:[NSString stringWithUTF8String:str] forState:UIControlStateNormal];
}

- (void) onTimerTimeout
{
	gTimeout--;
	if (gTimeout <= 0)
		[self performSelector:timerSelector];
	else {
		[self refreshTimerButton];
	}
	
}

- (void) setTimer
{
   if (okTimer) return;
   
   okTimer = [NSTimer scheduledTimerWithTimeInterval: 1.0
                                              target: self
                                            selector: @selector (onTimerTimeout)
                                            userInfo: nil
                                             repeats: YES];
}

- (void) removeTimer
{
   if (okTimer) {
      [okTimer invalidate];
      okTimer = NULL;
   }
}


- (void) onOkClicked
{
	//kill timer if exists
	[self removeTimer];
   
   [self removeFromSuperview];
   messagebox_removed();
   
   if ( MessageBoxClosedCallback )
	{
		MessageBoxClosedCallback( 0 );
	}
	
	[self release];
}

- (void) onYesClicked
{
	//kill timer if exists
	[self removeTimer];
   
   [self removeFromSuperview];
   confirm_removed();
   
   if ( ConfirmCallback )
	{
		(*ConfirmCallback)(dec_yes, ConfirmContext);
	}
	
	[self release];
}

- (void) onNoClicked
{
	//kill timer if exists
	[self removeTimer];
   
   [self removeFromSuperview];
   confirm_removed();
   
   if ( ConfirmCallback )
	{
		(*ConfirmCallback)(dec_no, ConfirmContext);
	}
	
	[self release];
}

- (void) close
{
   if (!ConfirmCallback)
      [self onOkClicked];
   else
      [self performSelector:timerSelector];
}

- (void) resizeViews
{
   CGRect rect;
   int maxHeight = self.bounds.size.height - 80;
   int maxWidth = self.bounds.size.width - 40;
   int titleHeight = 0;
   int titleSpace = 0;
   
   
   if (!contentLabel || !messageView || !backgroundImage)
      return;
   
   if (maxWidth > 400)
      maxWidth = 400;
   
   if (titleLabel) {
      titleHeight = MESSAGE_BOX_TITLE_HEIGHT;
      titleSpace = MESSAGE_BOX_LINE_SPACE;
   }
   
   rect = CGRectMake(20, 0, maxWidth, self.bounds.size.height);
   messageView.frame = rect;
   
   //Content label size
   rect = CGRectMake(MESSAGE_BOX_MARGIN, 0,
                     messageView.bounds.size.width - 2* MESSAGE_BOX_MARGIN, messageView.bounds.size.height);
   contentLabel.frame = rect;
   [contentLabel sizeToFit];
   rect = contentLabel.frame;
   if (contentLabel.bounds.size.height + 3* MESSAGE_BOX_LINE_SPACE +
       MESSAGE_BOX_BUTTON_HEIGHT + titleHeight + titleSpace > maxHeight) {
      rect.size.height = maxHeight - 3* MESSAGE_BOX_LINE_SPACE -
                           MESSAGE_BOX_BUTTON_HEIGHT - titleHeight - titleSpace;
   }
   rect.origin.y = titleSpace + titleHeight + MESSAGE_BOX_LINE_SPACE;
   rect.origin.x = (messageView.bounds.size.width - rect.size.width)/2;
   contentLabel.frame = rect;
   
   //title label size
   if (titleHeight > 0) {
      rect = CGRectMake(MESSAGE_BOX_MARGIN, titleSpace,
                        messageView.bounds.size.width - 2* MESSAGE_BOX_MARGIN, titleHeight);
      titleLabel.frame = rect;
   }
   
   //final message size
   rect = CGRectMake(0, 0, maxWidth, 
                     contentLabel.bounds.size.height + 3* MESSAGE_BOX_LINE_SPACE +
                     MESSAGE_BOX_BUTTON_HEIGHT + titleHeight + titleSpace);
   messageView.bounds = rect;
   messageView.center = self.center;
   
   if (!ConfirmCallback) {
      //OK button size
      rect = CGRectMake((messageView.bounds.size.width - MESSAGE_BOX_BUTTON_WIDTH)/2, 
                        messageView.bounds.size.height - MESSAGE_BOX_LINE_SPACE - MESSAGE_BOX_BUTTON_HEIGHT,
                        MESSAGE_BOX_BUTTON_WIDTH,
                        MESSAGE_BOX_BUTTON_HEIGHT);
      okButton.frame = rect;
   } else {
      //Yes button size
      rect = CGRectMake((messageView.bounds.size.width - MESSAGE_BOX_BUTTON_WIDTH*2)/3, 
                        messageView.bounds.size.height - MESSAGE_BOX_LINE_SPACE - MESSAGE_BOX_BUTTON_HEIGHT,
                        MESSAGE_BOX_BUTTON_WIDTH,
                        MESSAGE_BOX_BUTTON_HEIGHT);
      if (!roadmap_lang_rtl())
         yesButton.frame = rect;
      else
         noButton.frame = rect;
      
      //No button size
      rect = CGRectMake((messageView.bounds.size.width - MESSAGE_BOX_BUTTON_WIDTH*2)*2/3 + MESSAGE_BOX_BUTTON_WIDTH, 
                        messageView.bounds.size.height - MESSAGE_BOX_LINE_SPACE - MESSAGE_BOX_BUTTON_HEIGHT,
                        MESSAGE_BOX_BUTTON_WIDTH,
                        MESSAGE_BOX_BUTTON_HEIGHT);
      if (!roadmap_lang_rtl())
         noButton.frame = rect;
      else
         yesButton.frame = rect;
   }
   
   //background image size
   backgroundImage.frame = messageView.bounds;
}

- (void) layoutSubviews
{
   [self resizeViews];
}

- (UIButton *) newButton
{
   UIImage *image = NULL;
   UIButton *button = [UIButton buttonWithType: UIButtonTypeCustom];
   image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "button_up");
   if (image) {
      [button setBackgroundImage:[image stretchableImageWithLeftCapWidth:10 topCapHeight:10]
                          forState:UIControlStateNormal];
   }
   image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "button_down");
   if (image) {
      [button setBackgroundImage:[image stretchableImageWithLeftCapWidth:10 topCapHeight:10]
                          forState:UIControlStateHighlighted];
   }

   [button setTitleEdgeInsets:UIEdgeInsetsMake(5, 5, 5, 5)];
   button.titleLabel.numberOfLines = 2;
   [button setTitleColor:[UIColor colorWithRed:1.0f green:1.0f blue:1.0f alpha:1.0f] forState:UIControlStateNormal];
   button.titleLabel.font = [UIFont systemFontOfSize:17];
   button.titleLabel.lineBreakMode = UILineBreakModeWordWrap;
   button.titleLabel.textAlignment = UITextAlignmentCenter;
   
   return button;
}

- (void) createWithTitle: (const char*) title
              andContent: (const char*) content
              andYesText: (const char*) yesText
               andNoText: (const char*) noText
{
   UIImage *image;
   CGRect rect;
   
   roadmap_main_get_bounds(&rect);
   [self setFrame:rect];
   [self setBackgroundColor:[UIColor clearColor]];
   [self setAutoresizesSubviews:YES];
   [self setAutoresizingMask:UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth];
   
   //messagebox view
   messageView = [[UIView alloc] initWithFrame:CGRectZero];
   [self addSubview:messageView];
   [messageView release];
   
   //background image
   image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, MESSAGE_BOX_IMAGE);
   if (image) {
      minHeight = image.size.height;
      minWidth = image.size.width;
      backgroundImage = [[UIImageView alloc] initWithImage:[image stretchableImageWithLeftCapWidth:30
                                                                                      topCapHeight:40]];
      [messageView addSubview:backgroundImage];
      [backgroundImage release];
   }
   
   //title
   if ((title) && strlen(title) > 0) {
      titleLabel = [[UILabel alloc] initWithFrame:CGRectZero];
      [titleLabel setText:[NSString stringWithUTF8String:roadmap_lang_get(title)]];
      [titleLabel setBackgroundColor:[UIColor clearColor]];
      [titleLabel setTextColor:[UIColor whiteColor]];
      [titleLabel setFont:[UIFont boldSystemFontOfSize:20]];
      [titleLabel setTextAlignment:UITextAlignmentCenter];
      [titleLabel setAdjustsFontSizeToFitWidth:YES];
      [messageView addSubview:titleLabel];
      [titleLabel release];
   } else {
      titleLabel = NULL;
   }
   
   //Content
   contentLabel = [[UILabel alloc] initWithFrame:CGRectZero];
   [contentLabel setText:[NSString stringWithUTF8String:roadmap_lang_get(content)]];
   [contentLabel setBackgroundColor:[UIColor clearColor]];
   [contentLabel setTextColor:[UIColor whiteColor]];
   [contentLabel setNumberOfLines:0];
   [contentLabel setTextAlignment:UITextAlignmentCenter];
   [contentLabel setAutoresizingMask:UIViewAutoresizingFlexibleBottomMargin];
   [messageView addSubview:contentLabel];
   [contentLabel release];
   
   if (!ConfirmCallback) {
      //OK button
      okButton = [self newButton];
      [okButton addTarget:self action:@selector(onOkClicked) forControlEvents:UIControlEventTouchUpInside];
      [messageView addSubview: okButton];
   } else {
      //Yes button
      yesButton = [self newButton];
      [yesButton addTarget:self action:@selector(onYesClicked) forControlEvents:UIControlEventTouchUpInside];
      [yesButton setTitle:[NSString stringWithUTF8String:roadmap_lang_get(yesText)] forState:UIControlStateNormal];
      [messageView addSubview: yesButton];
      
      //No button
      noButton = [self newButton];
      [noButton addTarget:self action:@selector(onNoClicked) forControlEvents:UIControlEventTouchUpInside];
      [noButton setTitle:[NSString stringWithUTF8String:roadmap_lang_get(noText)] forState:UIControlStateNormal];
      [messageView addSubview: noButton];
   }
   
   [self resizeViews];
}

- (void) showWithTitle: (const char*) aTitleUtf8
            andContent: (const char*) aTextUtf8
            andTimeout: (int) timeout
           andCallback: (messagebox_closed) callback
{
 	gTimeout = timeout;
   MessageBoxClosedCallback = callback;
   ConfirmCallback = NULL;
   
   [self createWithTitle:aTitleUtf8 andContent:aTextUtf8 andYesText:NULL andNoText:NULL];
   
   if (gTimeout > 0) {
      strncpy_safe(timerText, roadmap_lang_get("Ok"), sizeof (timerText));
      timerButton = okButton;
      timerSelector = @selector(onOkClicked);
      
      [self setTimer];
      [self refreshTimerButton];
	} else
		[okButton setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Ok")] forState:UIControlStateNormal];
	
	roadmap_main_show_view(self);
   
//   [self animateIn];
}

- (void) showConfirmWithTitle: (const char*) aTitleUtf8
                   andContent: (const char*) aTextUtf8
                andDefaultYes: (BOOL) default_yes
                  andCallback: (ConfirmDialogCallback) callback
                   andContext: (void *) context
                   andTextYes: (const char*) aTextYes
                    andTextNo: (const char*) aTextNo
                   andTimeout: (int) timeout
{
   gTimeout = timeout;
   MessageBoxClosedCallback = NULL;
   ConfirmCallback = callback;
   ConfirmContext = context;
   
   [self createWithTitle:aTitleUtf8 andContent:aTextUtf8 andYesText:aTextYes andNoText:aTextNo];
   
   if (default_yes) {
      strncpy_safe(timerText, roadmap_lang_get(aTextYes), sizeof (timerText));
      timerButton = yesButton;
      timerSelector = @selector(onYesClicked);
      
   } else {
      strncpy_safe(timerText, roadmap_lang_get(aTextNo), sizeof (timerText));
      timerButton = noButton;
      timerSelector = @selector(onNoClicked);
   }
   
   if (gTimeout > 0) {      
      [self setTimer];
      [self refreshTimerButton];
	}
   
	roadmap_main_show_view(self);
   
//   [self animateIn];
}

@end

