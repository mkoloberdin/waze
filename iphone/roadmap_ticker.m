/* roadmap_ticker.m
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi B.S
 *   Copyright 2009 Avi R.
 *   Copyright 2009, Waze Ltd
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License V2 as published by
 *   the Free Software Foundation.
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
 *
 */
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#include "roadmap.h"
#include "roadmap_main.h"
#include "roadmap_types.h"
#include "roadmap_gui.h"
#include "roadmap_math.h"
#include "roadmap_line.h"
#include "roadmap_street.h"
#include "roadmap_config.h"
#include "roadmap_canvas.h"
#include "roadmap_message.h"
#include "roadmap_sprite.h"
#include "roadmap_voice.h"
#include "roadmap_skin.h"
#include "roadmap_plugin.h"
#include "roadmap_square.h"
#include "roadmap_math.h"
#include "roadmap_res.h"
#include "roadmap_bar.h"
#include "roadmap_sound.h"
#include "roadmap_pointer.h"
#include "roadmap_ticker.h"
#include "roadmap_device.h"
#include "editor/editor_screen.h"
#include "editor/editor_points.h"
#include "Realtime/Realtime.h"
#include "roadmap_display.h"
#include "roadmap_lang.h"
#include "roadmap_device_events.h"
#include "roadmap_iphoneimage.h"
#include "roadmap_iphonemain.h"
#include "roadmap_iphonecanvas.h"
#include "ssd/ssd_dialog.h"
#include "roadmap_iphoneticker.h"

static RoadMapTickerView *gTickerView;
//static UIView *gCloseView;
static UILabel *gGainedPtLbl, *gGainedPtTtl, *gYourPtLbl, *gYourRankLbl;
static BOOL gTickerShown = FALSE;
static BOOL gTickerOn = FALSE;
static BOOL gTickerSupressHide = FALSE;
static BOOL gTickerHide = FALSE;
static BOOL gInitialized;
static int gLastUpdateEvent = default_event;

static RoadMapConfigDescriptor ShowTickerCfg =
                        ROADMAP_CONFIG_ITEM("User", "Show points ticker");

#define TICKER_GAINED_PTS_IMAGE			("gained_pt_box")

void roadmap_ticker_show_internal(void);
void roadmap_ticker_hide_internal(void);


////////////////////////////////////////////////////////////////////////////////
static void periodic_hide(void) {
	char text[256];
	
	if (!roadmap_message_format (text, sizeof(text), "%X|%*"))
		roadmap_ticker_hide_internal();
}

////////////////////////////////////////////////////////////////////////////////

int ticker_cfg_on (void) {
   return (roadmap_config_match(&ShowTickerCfg, "yes"));
}

void roadmap_ticker_off (void) {
	char text[256];
	
	//gTickerShown = FALSE;
	gTickerOn = FALSE;
	
	if (roadmap_message_format (text, sizeof(text), "%X")){
		roadmap_message_unset('X');
	}
   
   roadmap_ticker_hide_internal();
}

void roadmap_ticker_on (void) {	
	gTickerOn = TRUE;
}


////////////////////////////////////////////////////////////////////////////////
static void periodic_off(void) {
   /*
	if (!ticker_cfg_on()) {
		roadmap_ticker_off();
		return;
	}
    */
	
	//TODO: preserve shown status regardless to on/off. If was shown and ssd popped up, restore shown when ssd is closed.
	if (gTickerOn && ssd_dialog_is_currently_active()) {
		roadmap_ticker_off();
		return;
	}
	
	if (!gTickerOn && !ssd_dialog_is_currently_active()) {
		roadmap_ticker_on();
		return;
	}
}

////////////////////////////////////////////////////////////////////////////////
void roadmap_ticker_display() {
	int iMyTotalPoints;
	int iMyRanking;
	char text[256];
   const char * point_text;
   
   if (gTickerHide && (!roadmap_message_format (text, sizeof(text), "%X"))){
	 	return;
	}
	
	if (roadmap_message_format (text, sizeof(text), "%X|%*")) {
		if (!gTickerShown)
			roadmap_ticker_show_internal();
      
      if (strcmp(text, "0")) {
         [gGainedPtLbl setText:[NSString stringWithFormat:@"+%s", text]];
      } else {
         [gGainedPtLbl setText:@""];
      }
	}
   
   switch( gLastUpdateEvent ) {
      case default_event:
         point_text = roadmap_lang_get("New points");
         break;
      case report_event:
         point_text = roadmap_lang_get("Road report");
         break;
      case comment_event:
         point_text = roadmap_lang_get("Event comment");
         break;
      case confirm_event:
         point_text= roadmap_lang_get("Prompt response");
         break;
      case road_munching_event :
         point_text= roadmap_lang_get("Road munching");
         break;
      case user_contribution_event :
         point_text= roadmap_lang_get("Traffic detection");
         break;
      case bonus_points :
         point_text= roadmap_lang_get("Bonus points");
         break;
      default:
         point_text="";
         break;
	}
   [gGainedPtTtl setText:[NSString stringWithUTF8String:point_text]];
	
	
	iMyTotalPoints = editor_points_get_total_points();
	if (iMyTotalPoints != -1){
		if (iMyTotalPoints < 10000){
			[gYourPtLbl setText:[NSString stringWithFormat:@"%d", iMyTotalPoints]];
		} else {
			[gYourPtLbl setText:[NSString stringWithFormat:@"%dK", (int)iMyTotalPoints/1000]];			
		}
	} else {
		[gYourPtLbl setText:@""];
	}
	
	iMyRanking = RealTime_GetMyRanking();
	if (iMyRanking != -1)
		[gYourRankLbl setText:[NSString stringWithFormat:@"%d", iMyRanking]];
	else
		[gYourRankLbl setText:@""];
} 

////////////////////////////////////////////////////////////////////////////////
void roadmap_ticker_initialize(void){
	UIImage *image = NULL;
	UIImageView *imageView = NULL;
	UILabel *label = NULL;
	CGRect rect;
	gInitialized = FALSE;
	int posX = 10;
	
	if (!editor_screen_gray_scale())
		return;
		
	rect = CGRectZero;
	rect.size.width = roadmap_canvas_width();
	
	roadmap_config_declare_enumeration ("user", &ShowTickerCfg, NULL, "yes", "no", NULL);
	
	//Opened ticker view
	//Set background
	image = roadmap_iphoneimage_load("TickerBackground");
	if (image) { 
		rect.size.height = [image size].height;
		imageView = [[UIImageView alloc] initWithImage:[image stretchableImageWithLeftCapWidth:image.size.width/2
																				   topCapHeight:image.size.height]];
		[image release];
		gTickerView = [[RoadMapTickerView alloc] initWithFrame:rect];
      [gTickerView setAutoresizesSubviews: YES];
      [gTickerView setAutoresizingMask: UIViewAutoresizingFlexibleWidth];
		[imageView setFrame:rect];
      [imageView setAutoresizesSubviews: YES];
      [imageView setAutoresizingMask: UIViewAutoresizingFlexibleWidth];
		[gTickerView addSubview:imageView];
		[imageView release];
		
		rect.origin.x = 0;
		rect.origin.y =  -imageView.bounds.size.height;
		[gTickerView setFrame:rect];
		roadmap_canvas_show_view(gTickerView);
	} else {
		roadmap_log (ROADMAP_ERROR, "roadmap_ticker - cannot load %s image ", "TickerBackground");
		return;
	}
   
   // Your points general message
	rect = [gTickerView bounds];
   rect.size.height = 20;
	label = [[UILabel alloc] initWithFrame:rect];
	[label setText:[NSString stringWithUTF8String:roadmap_lang_get("Your Points (updated once a day)")]];
	[label setTextAlignment:UITextAlignmentCenter];
	[label setAdjustsFontSizeToFitWidth:YES];
	[label setFont:[UIFont systemFontOfSize:14]];
	[label setBackgroundColor:[UIColor clearColor]];
	[gTickerView addSubview:label];
	[label release];
	
	//Set gained box
	image = roadmap_iphoneimage_load("TickerSilverBox");
	if (image) {
		imageView = [[UIImageView alloc] initWithImage:[image stretchableImageWithLeftCapWidth:image.size.width/2
                                                                                topCapHeight:image.size.height]];
		[image release];
		rect = [imageView frame];
		rect.origin.x = posX;
		rect.origin.y = ([gTickerView frame].size.height - [imageView frame].size.height) - 5;
      rect.size.width = 100;
		[imageView setFrame:rect];
		posX += [imageView frame].size.width + 10;
		[gTickerView addSubview:imageView];
		
		rect = [imageView frame];
		[imageView release];
      
		//Gained points title
		rect.origin.y = 25;
		rect.size.height = 20;
		gGainedPtTtl = [[UILabel alloc] initWithFrame:rect];
		[gGainedPtTtl setTextAlignment:UITextAlignmentCenter];
		[gGainedPtTtl setFont:[UIFont systemFontOfSize:14]];
      [label setAdjustsFontSizeToFitWidth:YES];
		[gGainedPtTtl setBackgroundColor:[UIColor clearColor]];;
		[gTickerView addSubview:gGainedPtTtl];
		[gGainedPtTtl release];
		
		//Gained points label
      rect = [imageView frame];
		gGainedPtLbl = [[UILabel alloc] initWithFrame:rect];
		[gGainedPtLbl setTextAlignment:UITextAlignmentCenter];
		[gGainedPtLbl setBackgroundColor:[UIColor clearColor]];
		[gGainedPtLbl setAdjustsFontSizeToFitWidth:YES];
		[gGainedPtLbl setFont:[UIFont systemFontOfSize:20]];
		[gTickerView addSubview:gGainedPtLbl];
	} else {
		roadmap_log (ROADMAP_ERROR, "roadmap_ticker - cannot load %s image ", "TickerSilverBox");
		return;
	}
	
	image = roadmap_iphoneimage_load(TICKER_DIVIDER);
	if (image) {
		imageView = [[UIImageView alloc] initWithImage:image];
		rect = [imageView frame];
		rect.origin.x = posX;
		rect.origin.y = [gTickerView frame].size.height - imageView.bounds.size.height;
		[imageView setFrame:rect];
		[gTickerView addSubview:imageView];
		[imageView release];
		imageView = [[UIImageView alloc] initWithImage:image];
		rect.origin.x = posX + (roadmap_canvas_width() - posX) /2;
		[imageView setFrame:rect];
		[gTickerView addSubview:imageView];
		[imageView release];
		
		[image release];
		
	} else {
		roadmap_log (ROADMAP_ERROR, "roadmap_ticker - cannot load %s image ", TICKER_DIVIDER);
		return;
	}
	
	// Your points title
	rect = [gTickerView bounds];
	rect.origin.x = posX + 5;
	rect.origin.y = 25;
	rect.size.width = (rect.size.width - posX) /2 - 10;
	rect.size.height = 20;
	label = [[UILabel alloc] initWithFrame:rect];
	[label setText:[NSString stringWithUTF8String:roadmap_lang_get("Total")]];
	[label setTextAlignment:UITextAlignmentCenter];
	[label setAdjustsFontSizeToFitWidth:YES];
	[label setFont:[UIFont systemFontOfSize:14]];
	[label setBackgroundColor:[UIColor clearColor]];
	[gTickerView addSubview:label];
	[label release];
	
	// Your points label
	rect.origin.y += rect.size.height + 2;
	gYourPtLbl = [[UILabel alloc] initWithFrame:rect];
	[gYourPtLbl setTextAlignment:UITextAlignmentCenter];
	[gYourPtLbl setAdjustsFontSizeToFitWidth:YES];
	[gYourPtLbl setFont:[UIFont systemFontOfSize:20]];
	[gYourPtLbl setBackgroundColor:[UIColor clearColor]];
	[gTickerView addSubview:gYourPtLbl];
	
	// Your rank title
	rect = [gTickerView bounds];
	rect.origin.x = posX + 5 + (roadmap_canvas_width() - posX) /2;
	rect.origin.y = 25;
	rect.size.width = (roadmap_canvas_width() - posX) /2 - 10;
	rect.size.height = 20;
	label = [[UILabel alloc] initWithFrame:rect];
	[label setText:[NSString stringWithUTF8String:roadmap_lang_get("Rank")]];
	[label setTextAlignment:UITextAlignmentCenter];
	[label setAdjustsFontSizeToFitWidth:YES];
	[label setFont:[UIFont systemFontOfSize:14]];
	[label setBackgroundColor:[UIColor clearColor]];
	[gTickerView addSubview:label];
	[label release];
	
	// Your rank label
	rect.origin.y += rect.size.height + 2;
	gYourRankLbl = [[UILabel alloc] initWithFrame:rect];
	[gYourRankLbl setTextAlignment:UITextAlignmentCenter];
	[gYourRankLbl setAdjustsFontSizeToFitWidth:YES];
	[gYourRankLbl setFont:[UIFont systemFontOfSize:20]];
	[gYourRankLbl setBackgroundColor:[UIColor clearColor]];
	[gTickerView addSubview:gYourRankLbl];
	
   //[gTickerView setAlpha:0.3f];
	
	roadmap_main_set_periodic (250, periodic_off);
    
   gInitialized = TRUE;
}

////////////////////////////////////////////////////////////////////////////////
int roadmap_ticker_height(){
	if (!editor_screen_gray_scale())
		 return 0;
    else if (!gTickerShown)
    	 return 0;
    else
       return [gTickerView frame].size.height;
}

////////////////////////////////////////////////////////////////////////////////
void roadmap_ticker_hide_internal(){
	if (!gInitialized || 
       !ticker_cfg_on() ||
       !gTickerShown)
		return ;
   
	roadmap_main_remove_periodic (periodic_hide);
   
	[gTickerView setHide];
	gTickerShown = FALSE;
   
   roadmap_bar_draw_top_bar(0);
}

////////////////////////////////////////////////////////////////////////////////
void roadmap_ticker_hide(){
	if (!gInitialized || 
		!gTickerShown)
		return ;
   
	roadmap_main_remove_periodic (periodic_hide);
   
   gTickerSupressHide = FALSE;
   gTickerHide = TRUE;
   
	[gTickerView setHide];
	gTickerShown = FALSE;
   
   roadmap_bar_draw_top_bar(0);
}

////////////////////////////////////////////////////////////////////////////////
void roadmap_ticker_show_internal(){
	
	if (!gInitialized ||
		!ticker_cfg_on() ||
		gTickerShown)
		return;
	
	if (!gTickerSupressHide)
		roadmap_main_set_periodic (500, periodic_hide);
	
	[gTickerView setShow];
	gTickerShown = TRUE;
	
	roadmap_ticker_display();
   
   roadmap_bar_draw_top_bar(0);
}

////////////////////////////////////////////////////////////////////////////////
void roadmap_ticker_show(){
	
	if (!gInitialized ||
       gTickerShown)
		return;
   
   gTickerSupressHide = TRUE;
   gTickerHide = FALSE;
   
   if (!gTickerSupressHide)
		roadmap_main_set_periodic (500, periodic_hide);
   
   [gTickerView setShow];
   gTickerShown = TRUE;
	
	roadmap_ticker_display();
   
   roadmap_bar_draw_top_bar(0);
}
////////////////////////////////////////////////////////////////////////////////
int roadmap_ticker_state() {
   if (gTickerShown)
      return 1;
   else
      return 0;
}

////////////////////////////////////////////////////////////////////////////////
void roadmap_ticker_set_last_event(int event){
	gLastUpdateEvent = event;
}

@implementation RoadMapTickerView

- (void) setShow {
	CGRect rect;

	rect = [gTickerView frame];
   
   int topbarHeight = 0;
   
   if (roadmap_top_bar_shown() && roadmap_bar_top_height() > 0)
      topbarHeight = roadmap_bar_top_height();
	
	//Animate ticker into screen
	[UIView beginAnimations:NULL context:NULL];
	rect.origin.y = topbarHeight;
	[UIView setAnimationDuration:0.3f];
	[UIView setAnimationCurve:UIViewAnimationCurveEaseInOut];
	[gTickerView setFrame:rect];
   //[gTickerView setAlpha:1.0f];
	[UIView commitAnimations];
}

- (void) setHide {
	CGRect rect;
	
	rect = [gTickerView frame];
	
	//Animate ticker out of screen
	[UIView beginAnimations:NULL context:NULL];
	rect.origin.y =  - gTickerView.bounds.size.height;
	[UIView setAnimationDuration:0.3f];
	[UIView setAnimationCurve:UIViewAnimationCurveEaseInOut];
	[gTickerView setFrame:rect];
   //[gTickerView setAlpha:0.3f];
	[UIView commitAnimations];
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	if (gTickerShown)
      roadmap_ticker_hide();
}

@end