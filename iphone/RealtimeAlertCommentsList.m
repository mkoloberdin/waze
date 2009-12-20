/* RealtimeAlertsCommentsList.m - manage the Real Time Alerts Commemnts list display (iphone)
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi B.S
 *   Copyright 2008 Ehud Shabtai
 *   Copyright 2009 Avi R
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
 * SYNOPSYS:
 *
 *   See RealtimeAlertsCommentsList.h
 */

#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_history.h"
#include "roadmap_locator.h"
#include "roadmap_street.h"
#include "roadmap_lang.h"
#include "roadmap_messagebox.h"
#include "roadmap_geocode.h"
#include "roadmap_config.h"
#include "roadmap_trip.h"
#include "roadmap_county.h"
#include "roadmap_display.h"
#include "roadmap_math.h"
#include "roadmap_navigate.h"
#include "ssd/ssd_keyboard.h"
#include "ssd/ssd_generic_list_dialog.h"
#include "ssd/ssd_contextmenu.h"
#include "roadmap_mood.h"
#include "roadmap_path.h"
#include "roadmap_main.h"
#include "roadmap_iphonemain.h"
#include "roadmap_iphoneimage.h"
#include "roadmap_time.h"
#include "widgets/iphoneLabel.h"
#include "roadmap_device_events.h"
#include "RealtimeAlerts.h"
#include "Realtime.h"
#include "RealtimeAlertsList.h"
#include "RealtimeAlertCommentsList.h"
#include "iphoneRealtimeAlertCommentsList.h"

#define MAX_COMMENTS_ENTRIES 100
static   BOOL                 	g_context_menu_is_active= FALSE;

// Context menu:
static ssd_cm_item      context_menu_items[] = 
{
   SSD_CM_INIT_ITEM  ( "Show on map",  rtcl_cm_show),
   SSD_CM_INIT_ITEM  ( "Post Comment", rtcl_cm_add_comments),
   SSD_CM_INIT_ITEM  ( "Exit_key",     rtcl_cm_exit)
};
static ssd_contextmenu  context_menu = SSD_CM_INIT_MENU( context_menu_items);

typedef struct 
{

    const char *title;
    int alert_id;

} RoadMapRealTimeAlertListCommentsDialog;

typedef struct
{

    char *value;
    char *label;
} RoadMapRealTimeAlertCommentsList;

static int g_Alert_Id;

///////////////////////////////////////////////////////////////////////////////////////////
static void on_option_show(){
	 ssd_generic_list_dialog_hide_all();
    RTAlerts_Popup_By_Id(g_Alert_Id);
}

///////////////////////////////////////////////////////////////////////////////////////////
static void on_option_add_comment(){
	 real_time_post_alert_comment_by_id(g_Alert_Id);
}

///////////////////////////////////////////////////////////////////////////////////////////
static void on_option_selected(  BOOL              made_selection,
                                 ssd_cm_item_ptr   item,
                                 void*             context)                                  
{
   real_time_list_context_menu_items   selection;
  // int                                 exit_code = dec_ok;
   
   g_context_menu_is_active = FALSE;
   
   if( !made_selection)
      return;  
   
   selection = (real_time_list_context_menu_items)item->id;

   switch( selection)
   {
      case rtcl_cm_show:
      	on_option_show();
         break;
         
      case rtcl_cm_add_comments:
         on_option_add_comment();
         break;
         
         
      case rtcl_cm_exit:
         //exit_code = dec_cancel;
      	//ssd_dialog_hide_all( exit_code);   
      	//roadmap_screen_refresh ();
         break;

      default:
         break;
   }
}                           
/*
///////////////////////////////////////////////////////////////////////////////////////////

static int on_options(SsdWidget widget, const char *new_value, void *context)
{
	int   menu_x;

   if(g_context_menu_is_active)
   {
      ssd_dialog_hide_current(dec_cancel);
      g_context_menu_is_active = FALSE;
   }

   if  (ssd_widget_rtl (NULL))
	   menu_x = SSD_X_SCREEN_RIGHT;
	else
		menu_x = SSD_X_SCREEN_LEFT;
		
   ssd_context_menu_show(  menu_x,   // X
                           SSD_Y_SCREEN_BOTTOM, // Y
                           &context_menu,
                           on_option_selected,
                           NULL,
                           dir_default); 
 
   g_context_menu_is_active = TRUE;

   return 0;
}*/

///////////////////////////////////////////////////////////////////////////////////////////
static int RealtimeAlertCommetsListCallBack(SsdWidget widget,
        const char *new_value, const void *value, void *context)
{

    int alertId;

    if (!value) return 0;

    alertId = atoi((const char*)value);
    ssd_generic_list_dialog_hide_all();
    RTAlerts_Popup_By_Id(alertId);

    return 1;
}


///////////////////////////////////////////////////////////////////////////////////////////
int RealtimeAlertCommentsList(int iAlertId)
{
	int iNumberOfComments = RTAlerts_Get_Number_of_Comments(iAlertId);
    if (iNumberOfComments == 0)
    {
        return -1;
    }
	
	CommentsListView *commentsView = [[CommentsListView alloc] init];
	[commentsView showWithAlert:iAlertId];
	
	return 0;
}





@implementation CommentsListView
@synthesize alertId;




- (void) onShowAlert
{
   RTAlerts_Popup_By_Id([alertId intValue]);
	roadmap_main_show_root(NO);
}

- (void) onAddComment
{
	real_time_post_alert_comment_by_id([alertId intValue]);
}

- (void)drawContent
{
   UIScrollView *scrollView = (UIScrollView *) self.view;
   int viewCount = [[scrollView subviews] count];
   for (int i = 0; i < viewCount; ++i) {
      [[[scrollView subviews] objectAtIndex:0] removeFromSuperview];
   }
	
	UIImage *image = NULL;
	UIImageView *imageView = NULL;
	RTAlert *alert;
	NSString *title;
	NSString *content;
	iphoneLabel *label;
	UIButton *button;
	CGRect rect;
	CGRect commentRect;
	CGSize old_size;
	CGFloat viewPosY = 5.0f;
	char dist_str[100];
   char unit_str[20];
	char detail_str[300];
	char CommentStr[300];
	const char *mood_str;
	RoadMapGpsPosition CurrentPosition;
   RoadMapPosition position, current_pos;
   const RoadMapPosition *gps_pos;
	PluginLine line;
	int Direction;
	int distance = -1;
	int distance_far;
	int timeDiff;
	time_t now;
	time_t comment_time;
	int iCount;
	
	RTAlertCommentsEntry *CommentEntry;
	
	detail_str[0] = 0;
	
	/////////////
	//show alert
	alert = RTAlerts_Get_By_ID([alertId intValue]);
   position.longitude = alert->iLongitude;
   position.latitude = alert->iLatitude;
	
	
	//Set detail string
	
	//Add desc
	snprintf(detail_str + strlen(detail_str), sizeof(detail_str)
            - strlen(detail_str), "%s", alert->sDescription);
	
	//Show time of report
	now = time(NULL);
	timeDiff = (int)difftime(now, (time_t)alert->iReportTime);
	if (timeDiff <0)
		timeDiff = 0;
	
	if (alert->iType == RT_ALERT_TYPE_TRAFFIC_INFO)
		snprintf(detail_str + strlen(detail_str), sizeof(detail_str)
               - strlen(detail_str),"\n%s",
               roadmap_lang_get("Updated "));
	else
		snprintf(detail_str + strlen(detail_str), sizeof(detail_str)
               - strlen(detail_str),"\n%s",
               roadmap_lang_get("Reported "));
	
	
   if (timeDiff < 60)
      snprintf(detail_str + strlen(detail_str), sizeof(detail_str)
               - strlen(detail_str),
               roadmap_lang_get("%d seconds ago"), timeDiff);
   else if ((timeDiff > 60) && (timeDiff < 3600))
      snprintf(detail_str + strlen(detail_str), sizeof(detail_str)
               - strlen(detail_str),
               roadmap_lang_get("%d minutes ago"), timeDiff/60);
   else
      snprintf(detail_str + strlen(detail_str), sizeof(detail_str)
               - strlen(detail_str),
               roadmap_lang_get("%2.1f hours ago"), (float)timeDiff
               /3600);
	
   // Display who reported the alert
   if (alert->sReportedBy[0] != 0){
      snprintf(detail_str + strlen(detail_str), sizeof(detail_str) - strlen(detail_str),
               " %s %s", roadmap_lang_get("by"), alert->sReportedBy);
   }
	
	//Calc distance
   roadmap_math_set_context((RoadMapPosition *)&position, 20);
	
   if (roadmap_navigate_get_current(&CurrentPosition, &line, &Direction) == -1)
   {
      // check the distance to the alert
      gps_pos = roadmap_trip_get_position("GPS");
      if (gps_pos != NULL)
      {
         current_pos.latitude = gps_pos->latitude;
         current_pos.longitude = gps_pos->longitude;
      }
      else
      {
         current_pos.latitude = -1;
         current_pos.longitude = -1;
      }
   }
   else
   {
      current_pos.latitude = CurrentPosition.latitude;
      current_pos.longitude = CurrentPosition.longitude;
   }
   
   if ((current_pos.latitude != -1) && (current_pos.longitude != -1))
   {
      distance = roadmap_math_distance(&current_pos, &position);
      distance_far = roadmap_math_to_trip_distance(distance);
      
      if (distance_far > 0)
      {
         int tenths = roadmap_math_to_trip_distance_tenths(distance);
         snprintf(dist_str, sizeof(dist_str), "\n%d.%d", distance_far, tenths % 10);
         snprintf(unit_str, sizeof(unit_str), "%s",
                  roadmap_lang_get(roadmap_math_trip_unit()));
      }
      else
      {
         snprintf(dist_str, sizeof(dist_str), "\n%d", distance);
         snprintf(unit_str, sizeof(unit_str), "%s",
                  roadmap_lang_get(roadmap_math_distance_unit()));
      }
   }
	
   //Display the distance of alert
   snprintf(detail_str + strlen(detail_str), sizeof(detail_str)
            - strlen(detail_str), roadmap_lang_get("%s %s Away"), dist_str, roadmap_lang_get(unit_str));
   
   //Create the report frame
   image = roadmap_iphoneimage_load("comments_alert");
   if (image) {
      rect = CGRectMake((scrollView.bounds.size.width - image.size.width)/2, viewPosY,
                        [image size].width, [image size].height);
      button = [UIButton buttonWithType:UIButtonTypeCustom];
      [button setFrame:rect];
      [button setBackgroundImage:[image stretchableImageWithLeftCapWidth:15 topCapHeight:15]
                        forState:UIControlStateNormal];
      [button addTarget:self action:@selector(onShowAlert) forControlEvents:UIControlEventTouchUpInside];
      [image release];
      [scrollView addSubview:button];
   }
   
   //Set title	
   title = [NSString stringWithUTF8String:(" %s",alert->sLocationStr)];
   rect = button.frame;
   rect.size.width -= 10;
   rect.size.height = (rect.size.height - 10) /3;
   rect.origin.x += 5;
   rect.origin.y += 5;
   label = [[iphoneLabel alloc] initWithFrame:rect];
   [label setText:title];
   [label setTextColor:[UIColor redColor]];
   [scrollView addSubview:label];
   [label release];
   
   //Set details
   content = [NSString stringWithUTF8String:detail_str];
   rect = button.frame;
   rect.size.width -= 10;
   rect.size.height = (button.frame.size.height - 10)* 2/3;
   rect.origin.x += 5;
   rect.origin.y += 5 + (button.frame.size.height - 10) /3;
   label = [[iphoneLabel alloc] initWithFrame:rect];
   [label setText:content];
   [label setNumberOfLines:0];
   [label setLineBreakMode:UILineBreakModeWordWrap];
   [label setAutoresizingMask:UIViewAutoresizingFlexibleBottomMargin];
   old_size = [label frame].size;
   [label sizeToFit];
   if (old_size.width > [label frame].size.width) {
      rect = [label frame];
      rect.size.width = old_size.width;
      [label setFrame:rect];
   }
   rect = [button frame];
   rect.size.height += [label bounds].size.height - old_size.height;
   [button setFrame:rect];
   [scrollView addSubview:label];
   [label release];
   
   viewPosY += button.frame.origin.y + button.frame.size.height + 5;
   
   /////////////
   //show comments
   CommentEntry = alert->Comment;
   
   //myUserName = RealTime_Get_UserName();
   iCount = 0;
   
   while (iCount < MAX_COMMENTS_ENTRIES && (CommentEntry != NULL))
   {
      CommentStr[0] = 0;
      
      if (!CommentEntry->comment.bCommentByMe)
         image = roadmap_iphoneimage_load("comments_left");
      else
         image = roadmap_iphoneimage_load("comments_right");
      if (image) {
         imageView = [[UIImageView alloc] initWithImage:image];
         [image release];
         rect = imageView.frame;
         rect.origin.y = viewPosY;
         rect.origin.x += 5;
         if (!CommentEntry->comment.bCommentByMe) {
            //left side bubble
            commentRect = rect;
            commentRect.origin.x += 25.0f;
            commentRect.size.height -= 10;
         } else {
            //right side bubble
            rect.origin.x = scrollView.frame.size.width - imageView.frame.size.width -5.0f;
            commentRect = rect;
            commentRect.origin.x += 5.0f;
            commentRect.size.height -= 10;
         }
         commentRect.origin.y += 5;
         commentRect.size.width -= 30;
         
         [imageView setFrame:rect];
         [scrollView addSubview:imageView];
         [imageView release];
      }
      
      viewPosY += imageView.frame.size.height + 5;
      
      
      //comment description
      snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
               - strlen(CommentStr), "%s%s",
               CommentEntry->comment.sDescription, "\n");
      
      comment_time = (time_t)CommentEntry->comment.i64ReportTime;
      //add time stamp
      snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
               - strlen(CommentStr), "%s ",
               roadmap_time_get_hours_minutes(comment_time));
      
      snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
               - strlen(CommentStr), " %s %s", roadmap_lang_get("by"),
               CommentEntry->comment.sPostedBy);
      
      
      content = [NSString stringWithUTF8String:CommentStr];
      label = [[iphoneLabel alloc] initWithFrame:commentRect];
      [label setText:content];
      [label setNumberOfLines:2];
      [label setLineBreakMode:UILineBreakModeWordWrap];
      [label setAdjustsFontSizeToFitWidth:YES];
      [label setMinimumFontSize:6.0f];
      [label setBackgroundColor:[UIColor clearColor]];
      [scrollView addSubview:label];
      [label release];
      
      //show mood
      mood_str = roadmap_mood_to_string(CommentEntry->comment.iMood);
      image = roadmap_iphoneimage_load(mood_str);
      roadmap_path_free(mood_str);
      if (image) {
         imageView = [[UIImageView alloc] initWithImage:image];
         [image release];
         rect = imageView.frame;
         rect.origin.y = commentRect.origin.y + commentRect.size.height - 1 - rect.size.height;
         if (!roadmap_lang_rtl())
            rect.origin.x = commentRect.origin.x + commentRect.size.width - 1 - rect.size.width;
         else
            rect.origin.x = commentRect.origin.x + 1;
         [imageView setFrame:rect];
         [scrollView addSubview:imageView];
         [imageView release];
      }
      
      
      iCount++;
      CommentEntry = CommentEntry->next;
   }
   
   
   //Add comment button
   image = roadmap_iphoneimage_load("comments_right");
   if (image) {
      CommentStr[0] = 0;
      rect.size = image.size;
      rect.origin.y = viewPosY;
      rect.origin.x = 5;
      
      //right side bubble
      rect.origin.x = scrollView.frame.size.width - rect.size.width -5.0f;
      commentRect = rect;
      commentRect.origin.x += 5.0f;
      commentRect.size.height -= 10;
      commentRect.origin.y += 5;
      commentRect.size.width -= 30;
      
      button = [UIButton buttonWithType:UIButtonTypeCustom];
      [button setFrame:rect];
      [button setBackgroundImage:image
                        forState:UIControlStateNormal];
      [button addTarget:self action:@selector(onAddComment) forControlEvents:UIControlEventTouchUpInside];
      [image release];
      [scrollView addSubview:button];
      
      viewPosY += button.frame.size.height + 5;
      
      //comment description
      snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
               - strlen(CommentStr), "%s",
               roadmap_lang_get("Add a comment"));
      
      content = [NSString stringWithUTF8String:CommentStr];
      
      label = [[iphoneLabel alloc] initWithFrame:commentRect];
      [label setText:content];
      [label setNumberOfLines:2];
      [label setLineBreakMode:UILineBreakModeWordWrap];
      [label setAdjustsFontSizeToFitWidth:YES];
      [label setMinimumFontSize:6.0f];
      [label setBackgroundColor:[UIColor clearColor]];
      [scrollView addSubview:label];
      [label release];
   }
   
   
   UINavigationBar *nav = [[self navigationController] navigationBar];
   if (viewPosY < roadmap_main_get_mainbox_height() - [nav bounds].size.height + 1)
      viewPosY = roadmap_main_get_mainbox_height() - [nav bounds].size.height + 1;
   [scrollView setContentSize:CGSizeMake(scrollView.frame.size.width, viewPosY)];
}


- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
   return roadmap_main_should_rotate (interfaceOrientation);
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
   roadmap_device_event_notification( device_event_window_orientation_changed);
   [self drawContent];
}





- (void)viewWillAppear:(BOOL)animated
{
	[self drawContent];
   
   roadmap_main_reload_bg_image();
   
   [super viewWillAppear:animated];
}
/*
- (void) viewDidLoad
{   
   roadmap_main_reload_bg_image();
}

*/

- (void) showWithAlert: (int)iAlertId
{
	UIScrollView *scrollView = [[UIScrollView alloc] initWithFrame:CGRectZero];

	self.view = scrollView;
	[scrollView release];
	
	[self setTitle: [NSString stringWithUTF8String:roadmap_lang_get("Comments")]];
	
	alertId = [[NSNumber alloc] initWithInt:iAlertId];
	
	roadmap_main_push_view(self);
}

- (void)dealloc
{	
	[alertId release];
	
	
	[super dealloc];
}



@end


