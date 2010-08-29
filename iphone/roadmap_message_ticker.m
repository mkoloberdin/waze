/* roadmap_message_ticker.m
 *
 * LICENSE:
 *
 *   Copyright 2010 Avi R.
 *   Copyright 2010, Waze Ltd
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
#include "roadmap_device.h"
#include "roadmap_res_download.h"
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
#include "editor/static/editor_street_bar.h"
#include "roadmap_iphonemessage_ticker.h"
#include "roadmap_message_ticker.h"

static RoadMapMessageTickerView *gTickerView;
static UILabel *gTitleLbl, *gContentLbl;
static UIImageView *gIconImg, *gXIcon;
static BOOL gTickerShown = FALSE;
static BOOL gInitialized;
static BOOL gPeriodicOn;
static BOOL gRefreshNeeded = FALSE;
static int  gTimer = 0;
static RoadMapCallback  gCallback = NULL;


static void roadmap_message_ticker_hide(void);


////////////////////////////////////////////////////////////////////////////////
static void periodic_hide(void) {
   roadmap_main_remove_periodic(periodic_hide);
   gPeriodicOn = FALSE;
   
	if (gTickerShown) {
		roadmap_message_ticker_hide();
	}
}

////////////////////////////////////////////////////////////////////////////////
void roadmap_message_ticker_display(void) {
   static int last_width = 0;
   CGRect rect;
   int width;
   
   if (last_width != gTickerView.bounds.size.width ||
       gRefreshNeeded) {
      // X icon pos
      rect.size = gXIcon.bounds.size;
      rect.origin = CGPointMake(gTickerView.bounds.size.width - 5 - gXIcon.bounds.size.width,
                                gTickerView.bounds.size.height - 5 - gXIcon.bounds.size.height);
      [gXIcon setFrame:rect];
      
      // message icon
      [gIconImg sizeToFit];
      rect = gIconImg.frame;
      rect.origin.y = 23 + (48 - rect.size.height) /2;
      gIconImg.frame = rect;
      
      //title
      rect = gTitleLbl.frame;
      rect.size.width = gTickerView.bounds.size.width - 20;
      gTitleLbl.frame = rect;
      
      //Content
      width = gTickerView.bounds.size.width - (gIconImg.frame.origin.x + gIconImg.frame.size.width + 5) - (gXIcon.bounds.size.width + 10);
      rect = CGRectMake(gIconImg.frame.origin.x + gIconImg.frame.size.width + 5, 25,
                        width, 46);
      gContentLbl.frame = rect;
      
      gRefreshNeeded = FALSE;
      last_width = gTickerView.bounds.size.width;
   }
}

////////////////////////////////////////////////////////////////////////////////
static void roadmap_message_ticker_refresh(const char *title, const char* text, const char* icon) {
   UIImage *image;
   
   image = roadmap_iphoneimage_load(icon);
	gIconImg.image = image;
   [image release];
   
   gTitleLbl.text = [NSString stringWithUTF8String:roadmap_lang_get(title)];
   gContentLbl.text = [NSString stringWithUTF8String:roadmap_lang_get(text)];
   
   gRefreshNeeded = TRUE;
} 

////////////////////////////////////////////////////////////////////////////////
void roadmap_message_ticker_initialize(void){
	UIImage *image = NULL;
	UIImageView *imageView = NULL;
	CGRect rect;
	gInitialized = FALSE;
		
	rect = CGRectZero;
	rect.size.width = roadmap_canvas_width();
	
	//Opened ticker view
	//Set background
	image = roadmap_iphoneimage_load("TickerBackground");
	if (image) { 
		rect.size.height = [image size].height;
		imageView = [[UIImageView alloc] initWithImage:[image stretchableImageWithLeftCapWidth:image.size.width/2
																				   topCapHeight:image.size.height]];
		[image release];
		gTickerView = [[RoadMapMessageTickerView alloc] initWithFrame:rect];
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
		roadmap_log (ROADMAP_ERROR, "roadmap_message_ticker - cannot load %s image ", "TickerBackground");
		return;
	}
   
   //X icon
   image = roadmap_iphoneimage_load("x_close");
	if (image) { 
		rect.size = image.size;
      rect.origin = CGPointMake(gTickerView.bounds.size.width - 5 - image.size.width,
                                gTickerView.bounds.size.height - 5 - image.size.height);
		gXIcon = [[UIImageView alloc] initWithImage:image];
		[image release];
      [gXIcon setFrame:rect];
		[gTickerView addSubview:gXIcon];
		[gXIcon release];
	} else {
		roadmap_log (ROADMAP_ERROR, "roadmap_message_ticker - cannot load %s image ", "x_close");
		return;
	}
   
   //message icon
   rect = CGRectMake(5, 25, 0, 0);
   gIconImg = [[UIImageView alloc] initWithFrame:rect];
   [gTickerView addSubview:gIconImg];
   [gIconImg release];
   
   // message title
   rect = CGRectMake(10, 1, gTickerView.bounds.size.width - 20, 19);
	gTitleLbl = [[UILabel alloc] initWithFrame:rect];
	[gTitleLbl setTextAlignment:UITextAlignmentCenter];
	[gTitleLbl setAdjustsFontSizeToFitWidth:YES];
	[gTitleLbl setFont:[UIFont boldSystemFontOfSize:16]];
	[gTitleLbl setBackgroundColor:[UIColor clearColor]];
	[gTickerView addSubview:gTitleLbl];
	[gTitleLbl release];
	
	// message content
	gContentLbl = [[UILabel alloc] initWithFrame:CGRectZero];
	[gContentLbl setTextAlignment:UITextAlignmentCenter];
	[gContentLbl setFont:[UIFont systemFontOfSize:15]];
	[gContentLbl setBackgroundColor:[UIColor clearColor]];
   [gContentLbl setLineBreakMode:UILineBreakModeWordWrap];
   [gContentLbl setNumberOfLines:2];
	[gTickerView addSubview:gContentLbl];
	[gContentLbl release];
	
   //[gTickerView setAlpha:0.3f];
    
   gInitialized = TRUE;
}

////////////////////////////////////////////////////////////////////////////////
int roadmap_message_ticker_height(void){
	if (!editor_screen_gray_scale())
		 return 0;
    else if (!gTickerShown)
    	 return 0;
    else
       return [gTickerView frame].size.height;
}

////////////////////////////////////////////////////////////////////////////////
static void roadmap_message_ticker_hide(void){
   RoadMapCallback callback;
   
	if (!gInitialized || 
		!gTickerShown)
		return ;
   
	roadmap_main_remove_periodic (periodic_hide);
   gPeriodicOn = FALSE;
   
	[gTickerView setHide];
	gTickerShown = FALSE;
   
   roadmap_bar_draw_top_bar(0);
   
   callback = gCallback;
   gCallback = NULL;
   if (callback)
      callback();
}

////////////////////////////////////////////////////////////////////////////////
static void on_resource_downloaded (const char* res_name, int status, void *context, char *last_modified){
   static RoadMapSoundList list;
   if (!list) {
      list = roadmap_sound_list_create (SOUND_LIST_NO_FREE);
      roadmap_sound_list_add (list, "message_ticker");
      roadmap_res_get (RES_SOUND, 0, "message_ticker");
   }
   roadmap_sound_play_list (list);
   
   if (!gTickerShown) {
      [gTickerView setShow];
      gTickerShown = TRUE;
      roadmap_bar_draw_top_bar(0);
   }

   if (gTimer > 0) {
      gPeriodicOn = TRUE;
      roadmap_main_set_periodic (gTimer * 1000, periodic_hide);
   }
}


////////////////////////////////////////////////////////////////////////////////
void roadmap_message_ticker_show_cb(const char *title, const char* text, const char* icon, int timer, RoadMapCallback callback) {   
   if (!gInitialized)
		return;
   
   if (gPeriodicOn) {
      roadmap_main_remove_periodic(periodic_hide);
      gPeriodicOn = FALSE;
   }
   
   roadmap_message_ticker_refresh(title, text, icon);
   
   if ((icon) && (roadmap_res_get(RES_BITMAP,RES_SKIN,icon) == NULL)){
      roadmap_res_download(RES_DOWNLOAD_IMAGE, icon,NULL, "", FALSE, 0, on_resource_downloaded, NULL );
      gTimer = timer;
      roadmap_message_ticker_hide();
   } else {      
      static RoadMapSoundList list;
      if (!list) {
         list = roadmap_sound_list_create (SOUND_LIST_NO_FREE);
         roadmap_sound_list_add (list, "message_ticker");
         roadmap_res_get (RES_SOUND, 0, "message_ticker");
      }
      roadmap_sound_play_list (list);
      
      if (!gTickerShown) {
         [gTickerView setShow];
         gTickerShown = TRUE;
         roadmap_bar_draw_top_bar(0);
      }
      
      if (timer > 0) {
         gPeriodicOn = TRUE;
         roadmap_main_set_periodic (timer * 1000, periodic_hide);
      }
   }
   
   gCallback = callback;
}

////////////////////////////////////////////////////////////////////////////////
void roadmap_message_ticker_show(const char *title, const char* text, const char* icon, int timer){
	roadmap_message_ticker_show_cb (title, text, icon, timer, NULL);	
}

////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_message_ticker_is_on(void){
   return (gTickerShown);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
@implementation RoadMapMessageTickerView

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
      roadmap_message_ticker_hide();
}

@end