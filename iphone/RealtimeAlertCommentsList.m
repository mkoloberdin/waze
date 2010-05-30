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
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_confirm_dialog.h"
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
#define ABUSE_TAG 1



///////////////////////////////////////////////////////////////////////////////////////////
static void report_abuse_confirm_dlg_callback(int exit_code, void *context){
   CommentsListView *commentsView;
   
   if (exit_code == dec_yes){
      commentsView = (CommentsListView *)context;
      [commentsView onReportAbuseConfirmed];
   }
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



///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
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

- (int) getCommentFromCount: (int) count
{
   RTAlert *alert;
   RTAlertCommentsEntry *CommentEntry;
   int i;
   
   alert = RTAlerts_Get_By_ID([alertId intValue]);
   CommentEntry = alert->Comment;
   
   for (i = 0; i < count; i++) {
      CommentEntry = CommentEntry->next;
      if (!CommentEntry)
         return -1;
   }
   
   return CommentEntry->comment.iID;
}


- (void) onReportAbuseConfirmed
{
   UIButton *abuseBtn;
   if (abusedComment > 0)
      Realtime_ReportAbuse([alertId intValue], abusedComment);
   
   abusedComment = -1;
   abuseBtn = (UIButton *)[self.view viewWithTag:ABUSE_TAG];
   abuseBtn.hidden = YES;
}

- (void) onReportAbuse
{
   ssd_confirm_dialog_custom (roadmap_lang_get("Report abuse"), roadmap_lang_get("Reports by several users will disable this user from reporting"), FALSE,report_abuse_confirm_dlg_callback, self, roadmap_lang_get("Report"), roadmap_lang_get("Cancel")) ;
}

- (void) onToggleAbuse: (id) commentButton
{
   CGRect rect, btnRect;
   UIButton *abuseBtn;
   int commentId = ((UIView *)commentButton).tag;   
   
   abuseBtn = (UIButton *)[self.view viewWithTag:ABUSE_TAG];
   
   if (commentId == abusedComment) {
      if (abuseBtn.hidden == NO)
         abuseBtn.hidden = YES;
      else
         abuseBtn.hidden = NO;
   } else {
      abusedComment = commentId;
      rect = ((UIView *)commentButton).frame;
      btnRect = abuseBtn.frame;
      btnRect.origin.y = (rect.origin.y + (rect.size.height - btnRect.size.height) /2);
      btnRect.origin.x = rect.origin.x + rect.size.width + 10;
      abuseBtn.frame = btnRect;
      abuseBtn.hidden = NO;
   }
}

- (void)drawContent
{
   UIScrollView *scrollView = (UIScrollView *) self.view;
   int viewCount = [[scrollView subviews] count];
   for (int i = 0; i < viewCount; ++i) {
      [[[scrollView subviews] objectAtIndex:0] removeFromSuperview];
   }
   
   int SavedZoom = -1;
   RoadMapPosition SavedPosition;
   UIImage *image = NULL;
   UIImage *leftImage = NULL;
   UIImage *rightImage = NULL;
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
   
   //Load left and right comment bubbles
   image = roadmap_iphoneimage_load("comments_left");
   leftImage = [image stretchableImageWithLeftCapWidth:50 topCapHeight:50];
   [image release];
   image = roadmap_iphoneimage_load("comments_right");
   rightImage = [image stretchableImageWithLeftCapWidth:50 topCapHeight:10];
   [image release];
   
   //abuse button
   button = [UIButton buttonWithType:UIButtonTypeCustom];
   button.tag = ABUSE_TAG;
   abusedComment = -1;
   button.hidden = YES;
   image = roadmap_iphoneimage_load("button_report_abuse");
   [button setBackgroundImage:image forState:UIControlStateNormal];
   [image release];
   image = roadmap_iphoneimage_load("button_report_abuse_down");
   [button setBackgroundImage:image forState:UIControlStateHighlighted];
   [image release];
   [button sizeToFit];
   [button addTarget:self action:@selector(onReportAbuse) forControlEvents:UIControlEventTouchUpInside];
   [scrollView addSubview:button];
   
   
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
   roadmap_math_get_context(&SavedPosition, &SavedZoom);
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
   roadmap_math_set_context(&SavedPosition, SavedZoom);
   
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
   
   iCount = 0;
   
   while (iCount < MAX_COMMENTS_ENTRIES && (CommentEntry != NULL))
   {
      CommentStr[0] = 0;
      
      if (!CommentEntry->comment.bCommentByMe) {
         image = leftImage;
      } else {
         image = rightImage;
      }
      
      
      //imageView = [[UIImageView alloc] initWithImage:image];
      button = [UIButton buttonWithType:UIButtonTypeCustom];
      
      [button setBackgroundImage:image
                        forState:UIControlStateNormal];
      
      rect.size = image.size;
      rect.origin.y = viewPosY;
      rect.origin.x = 5;
      if (!CommentEntry->comment.bCommentByMe) {
         //left side bubble
         commentRect = rect;
         commentRect.origin.x += 25.0f;
         commentRect.size.height -= 10;
         
         [button addTarget:self action:@selector(onToggleAbuse:) forControlEvents:UIControlEventTouchUpInside];
         button.tag = CommentEntry->comment.iID;
      } else {
         //right side bubble
         rect.origin.x = scrollView.frame.size.width - image.size.width -5.0f;
         commentRect = rect;
         commentRect.origin.x += 5.0f;
         commentRect.size.height -= 10;
      }
      commentRect.origin.y += 5;
      commentRect.size.width -= 30;
      
      //[imageView setFrame:rect];
      //[scrollView addSubview:imageView];
      [button setFrame:rect];
      [scrollView addSubview:button];
      //[imageView release];
      
      
      //viewPosY += imageView.frame.size.height + 5;
      viewPosY += button.frame.size.height + 5;
      
      
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
   image = rightImage;
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
   //[self drawContent];
}


- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation duration:(NSTimeInterval)duration {
   [self drawContent];
}


- (void)viewWillAppear:(BOOL)animated
{
	[self drawContent];
   
   [super viewWillAppear:animated];
}


- (void) showWithAlert: (int)iAlertId
{
   UIScrollView *scrollView = [[UIScrollView alloc] initWithFrame:CGRectZero];
   [scrollView setBackgroundColor:roadmap_main_table_color()];
   
   self.view = scrollView;
   [scrollView release];
   
   [self setTitle: [NSString stringWithUTF8String:roadmap_lang_get("Comments")]];
   
   alertId = [[NSNumber alloc] initWithInt:iAlertId];
   abusedComment = -1;
   
   roadmap_main_push_view(self);
}

- (void)dealloc
{	
   [alertId release];
   
   
   [super dealloc];
}



@end


