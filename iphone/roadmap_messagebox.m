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

#include "roadmap_iphonemain.h"
#include "roadmap_iphoneimage.h"
#include "roadmap_messagebox.h"
#include "roadmap_iphonemessagebox.h"

static RoadMapMessageBoxView *gMessageBox = NULL;

#define MESSAGE_BOX_IMAGE "message_box"
#define MESSAGE_BOX_TITLE_HEIGHT 25.0f
#define MESSAGE_BOX_BUTTON_HEIGHT 32.0f
#define MESSAGE_BOX_BUTTON_WIDTH 100.0f
#define MESSAGE_BOX_LINE_SPACE 15.0f
#define MESSAGE_BOX_MARGIN    10.0f

static messagebox_closed MessageBoxClosedCallback = NULL;



void roadmap_messagebox_cb(const char *title, const char *message,
         messagebox_closed on_messagebox_closed)
{
   MessageBoxClosedCallback = on_messagebox_closed;
   roadmap_messagebox( title, message);
}


void roadmap_messagebox (const char *title, const char *text) {
	if (!gMessageBox) {
		gMessageBox = [[RoadMapMessageBoxView alloc] initWithFrame:CGRectZero];
		[gMessageBox showWithTitle:title andContent:text andTimeout:0];
	}
}


void roadmap_messagebox_timeout (const char *title, const char *text, int seconds) {
	if (!gMessageBox) {
		gMessageBox = [[RoadMapMessageBoxView alloc] initWithFrame:CGRectZero];
		[gMessageBox showWithTitle:title andContent:text andTimeout:seconds];
	}
}



@implementation RoadMapMessageBoxView
@synthesize messageView;
@synthesize backgroundImage;
@synthesize titleLabel;
@synthesize contentLabel;
@synthesize okButton;
@synthesize okTimer;


- (void) onTimerTimeout
{
	char ok_str[20];
	
	gTimeout--;
	if (gTimeout <= 0)
		[self onOkClicked];
	else {
		sprintf(ok_str, "%s (%d)", roadmap_lang_get("Ok"), gTimeout);
		[okButton setTitle:[NSString stringWithUTF8String:ok_str] forState:UIControlStateNormal];
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
   gMessageBox = NULL;
   
   if ( MessageBoxClosedCallback )
	{
		messagebox_closed temp = MessageBoxClosedCallback;  
	   MessageBoxClosedCallback = NULL;
		temp( 0 );
	}
	
	[self release];
}

- (void) resizeViews
{
   CGRect rect;
   int maxHeight = self.bounds.size.height - 80;
   int maxWidth = self.bounds.size.width - 40;
   int titleHeight = 0;
   int titleSpace = 0;
   
   
   if (!contentLabel || !messageView || !backgroundImage || !okButton)
      return;
   
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
   
   //button size
   rect = CGRectMake((messageView.bounds.size.width - MESSAGE_BOX_BUTTON_WIDTH)/2, 
                     messageView.bounds.size.height - MESSAGE_BOX_LINE_SPACE - MESSAGE_BOX_BUTTON_HEIGHT,
                     MESSAGE_BOX_BUTTON_WIDTH,
                     MESSAGE_BOX_BUTTON_HEIGHT);
   okButton.frame = rect;
   
   //background image size
   backgroundImage.frame = messageView.bounds;
}

- (void) layoutSubviews
{
   [self resizeViews];
}

- (void) showWithTitle: (const char*) aTitleUtf8
			andContent: (const char*) aTextUtf8
			andTimeout: (int) timeout
{
	UIImage *image;
	CGRect rect;
	char ok_str[20];
	
	gTimeout = timeout;
	
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
	image = roadmap_iphoneimage_load(MESSAGE_BOX_IMAGE);
	if (image) {
      minHeight = image.size.height;
      minWidth = image.size.width;
		backgroundImage = [[UIImageView alloc] initWithImage:[image stretchableImageWithLeftCapWidth:30
																				  topCapHeight:40]];
		[image release];
      [messageView addSubview:backgroundImage];
      [backgroundImage release];
	}
	
    //title
   if ((aTitleUtf8) && strcmp(roadmap_lang_get(aTitleUtf8), "") != 0) {
      titleLabel = [[UILabel alloc] initWithFrame:CGRectZero];
      [titleLabel setText:[NSString stringWithUTF8String:roadmap_lang_get(aTitleUtf8)]];
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
	[contentLabel setText:[NSString stringWithUTF8String:roadmap_lang_get(aTextUtf8)]];
	[contentLabel setBackgroundColor:[UIColor clearColor]];
   [contentLabel setTextColor:[UIColor whiteColor]];
	[contentLabel setNumberOfLines:0];
	[contentLabel setTextAlignment:UITextAlignmentCenter];
	[contentLabel setAutoresizingMask:UIViewAutoresizingFlexibleBottomMargin];
	[messageView addSubview:contentLabel];
	[contentLabel release];
	
	//OK button
	okButton = [UIButton buttonWithType: UIButtonTypeCustom];
	[okButton addTarget:self action:@selector(onOkClicked) forControlEvents:UIControlEventTouchUpInside];
	image = roadmap_iphoneimage_load("button_up");
	if (image) {
		[okButton setBackgroundImage:[image stretchableImageWithLeftCapWidth:10 topCapHeight:10]
							forState:UIControlStateNormal];
		[image release];
	}
	image = roadmap_iphoneimage_load("button_down");
	if (image) {
		[okButton setBackgroundImage:[image stretchableImageWithLeftCapWidth:10 topCapHeight:10]
							forState:UIControlStateHighlighted];
		[image release];
	}
   [messageView addSubview: okButton];
   
   [self resizeViews];
	
	if (gTimeout > 0) {
      [self setTimer];
		sprintf(ok_str, "%s (%d)", roadmap_lang_get("Ok"), gTimeout);
		[okButton setTitle:[NSString stringWithUTF8String:ok_str] forState:UIControlStateNormal];
	} else
		[okButton setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Ok")] forState:UIControlStateNormal];
	
	roadmap_main_show_view(self);
}

@end

