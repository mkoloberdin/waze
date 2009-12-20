/* ssd_progress_msg_dialog.c - Progress message dialog source
 *
 * LICENSE:
 *
 *   Copyright 2009, Waze Ltd
 *   Alex Agranovich
 *   Avi R.
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
 */

#include "roadmap.h"
#include "widgets/iphoneRoundedView.h"
#include "ssd_progress_msg_dialog.h"
#include "roadmap_main.h"
#include "roadmap_iphonemain.h"


#define SSD_PROGRESS_MSG_FONT_SIZE 		24
#define TAG_LABEL                      1
#define MINIMIZE_TIMEOUT               3

static iphoneRoundedView   *gProgressMsgDlg = NULL;



void minimize_timer (void) {
   UILabel *label;
   
   roadmap_main_remove_periodic(minimize_timer);
   
   [gProgressMsgDlg setRectColor: [UIColor clearColor]];
   [gProgressMsgDlg setNeedsDisplay];
   
   
   label = (UILabel *)[gProgressMsgDlg viewWithTag:TAG_LABEL];
   label.textColor = [UIColor clearColor];
}

void ssd_progress_msg_dialog_show( const char* dlg_text )
{
   UIActivityIndicatorView *activityIndicator;
   CGRect rect;
   CGRect appRect;
   UIColor *color;
   UILabel *label;
   
   roadmap_main_get_bounds(&appRect);
   
   ssd_progress_msg_dialog_hide(); // for now always create the dialog, do not reuse
   
	if ( !gProgressMsgDlg  )
	{
      // Create the dialog.
      rect = CGRectMake((appRect.size.width - 280)/2,
                        appRect.size.height/2 - 50, 
                        280,//appRect.size.width - 40,
                        150);
		gProgressMsgDlg = [[iphoneRoundedView alloc] initWithFrame: rect];
      [gProgressMsgDlg setAutoresizesSubviews:YES];
      [gProgressMsgDlg setAutoresizingMask:UIViewAutoresizingFlexibleTopMargin |
                                             UIViewAutoresizingFlexibleBottomMargin |
                                             UIViewAutoresizingFlexibleRightMargin | 
                                             UIViewAutoresizingFlexibleLeftMargin];
      [gProgressMsgDlg setContentMode:UIViewContentModeCenter];
		gProgressMsgDlg.opaque = NO;
		color = [UIColor colorWithWhite:0 alpha:0.7];
		[gProgressMsgDlg setBackgroundColor: [UIColor clearColor]];
		[gProgressMsgDlg setRectColor: color];
		activityIndicator = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleWhiteLarge];
		rect = activityIndicator.frame;
		rect.origin.x = gProgressMsgDlg.bounds.size.width/2 - activityIndicator.frame.size.width/2;
		rect.origin.y = 10;
		[activityIndicator setFrame:rect];
      [activityIndicator setAutoresizesSubviews:YES];
      [activityIndicator setContentMode:UIViewContentModeCenter];
		[gProgressMsgDlg addSubview:activityIndicator];
      [activityIndicator release];
      [activityIndicator startAnimating];
      rect.origin.y += activityIndicator.bounds.size.height + 10;
      rect.origin.x = 10;
      rect.size.height = gProgressMsgDlg.bounds.size.height - rect.origin.y - 10;
      rect.size.width = gProgressMsgDlg.bounds.size.width - 20;
      label = [[UILabel alloc] initWithFrame:rect];
      label.text = [NSString stringWithUTF8String:dlg_text];
      label.font = [UIFont boldSystemFontOfSize:SSD_PROGRESS_MSG_FONT_SIZE];
      label.backgroundColor = [UIColor clearColor];
      label.textColor = [UIColor whiteColor];
      label.tag = TAG_LABEL;
      label.numberOfLines = 5;
      label.textAlignment = UITextAlignmentCenter;
      label.lineBreakMode = UILineBreakModeWordWrap;
      [label setAutoresizesSubviews:YES];
      [label  setAutoresizingMask:UIViewAutoresizingFlexibleBottomMargin | UIViewAutoresizingFlexibleLeftMargin |
                              UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleTopMargin];
      [label setContentMode:UIViewContentModeCenter];
      [gProgressMsgDlg addSubview:label];
      [label release];
   } else {
      label = (UILabel *)[gProgressMsgDlg viewWithTag:TAG_LABEL];
      label.text = [NSString stringWithUTF8String:dlg_text];
      label.textColor = [UIColor whiteColor];
      color = [UIColor colorWithWhite:0 alpha:0.7];
      [gProgressMsgDlg setRectColor: color];
      [gProgressMsgDlg setNeedsLayout];
   }
   
   [gProgressMsgDlg setNeedsDisplay];
   
   roadmap_main_show_view(gProgressMsgDlg);
   roadmap_main_set_periodic(MINIMIZE_TIMEOUT *1000, minimize_timer);
}


void ssd_progress_msg_dialog_hide( void )
{
   roadmap_main_remove_periodic(minimize_timer);
   
   if (gProgressMsgDlg) {
      [gProgressMsgDlg removeFromSuperview];
      [gProgressMsgDlg release];
      gProgressMsgDlg = NULL;
   }
}

static void hide_timer(){
	ssd_progress_msg_dialog_hide();
	roadmap_main_remove_periodic (hide_timer);
}

void ssd_progress_msg_dialog_show_timed( const char* dlg_text , int seconds)
{
	ssd_progress_msg_dialog_show(dlg_text);
	roadmap_main_set_periodic (seconds * 1000, hide_timer);
}


