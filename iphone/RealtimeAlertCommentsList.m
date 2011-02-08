/* RealtimeAlertCommentsList.m - manage the Real Time Alerts Commemnts list display (iphone)
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
#include "roadmap_social_image.h"
#include "roadmap_path.h"
#include "roadmap_main.h"
#include "roadmap_iphonemain.h"
#include "roadmap_res.h"
#include "roadmap_time.h"
#include "widgets/iphoneLabel.h"
#include "roadmap_device_events.h"
#include "RealtimeAlerts.h"
#include "Realtime.h"
#include "RealtimeAlertsList.h"
#include "RealtimeAlertCommentsList.h"
#include "iphoneRealtimeAlertCommentsList.h"

#define MAX_COMMENTS_ENTRIES 100

enum tags {
   ABUSE_TAG = 1,
   CONTENT_TAG,
   MOOD_TAG,
   PICTURE_TAG,
   ALERT_TAG,
   THUMBS_TAG
};

typedef struct {
   void *delegate;
   int   alert;
   int   comment;
} comments_list_context;


static CommentsListView *gCommentsView = NULL;


///////////////////////////////////////////////////////////////////////////////////////////
static void image_download_cb ( void* context, int status, RoadMapImage image ){
   comments_list_context* list_context = (comments_list_context*)context;
   if (list_context->delegate &&
       list_context->delegate == (void *)gCommentsView &&
       status == 0) {
      [gCommentsView setImage:image alert:list_context->alert comment:list_context->comment];
      free(list_context);
   }
}

///////////////////////////////////////////////////////////////////////////////////////////
static void set_download_delegate (CommentsListView *commentsView) {
   if (gCommentsView != commentsView)
      gCommentsView = commentsView;
}

///////////////////////////////////////////////////////////////////////////////////////////
static void image_download (int alert, int comment, void* delegate) {
   comments_list_context* list_context;
   list_context = malloc( sizeof( comments_list_context ) );
   roadmap_check_allocated(list_context);
   list_context->delegate = delegate;
   list_context->alert = alert;
   list_context->comment = comment;
   
   set_download_delegate(delegate);
   roadmap_social_image_download( SOCIAL_IMAGE_SOURCE_FACEBOOK, SOCIAL_IMAGE_ID_TYPE_ALERT, alert, comment, SOCIAL_IMAGE_SIZE_SQUARE, list_context, image_download_cb );
}

   
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

- (void) resizeButton:(UIButton *)button byMe:(BOOL)by_me showPic:(BOOL)show_pic bSelected:(BOOL)selected yPos:(int)y_pos
{
   UIScrollView *scrollView = (UIScrollView *)self.view;
   iphoneLabel *label;
   UIImageView *imageView;
   UIView *view;
   CGRect rect;
   int x_margin;
   int height, width;
   int pic_width;
   UIButton *abuseBtn = (UIButton *)[scrollView viewWithTag:ABUSE_TAG];


   label = (iphoneLabel *)[button viewWithTag:CONTENT_TAG];
   
   if (!by_me) {
      //left side bubble
      x_margin = 25;
   } else {
      //right side bubble
      x_margin = 5;
   }
   if (show_pic) {
      pic_width = 50;
   } else {
      pic_width = 20;
   }
   
   if (selected)
      pic_width += abuseBtn.bounds.size.width + 10;

   rect = CGRectMake(x_margin, 5, scrollView.bounds.size.width - 10 - 25 - 20 - pic_width, minCommentHeight - 10);
   //label.backgroundColor = [UIColor redColor];
   label.frame = rect;
   
   [label sizeToFit];
   height = label.bounds.size.height + 10;
   if (height < minCommentHeight) {
      rect = label.frame;
      rect.size.height = minCommentHeight - 10;
      label.frame = rect;
      height = minCommentHeight;
   }
   
   width = 45 + label.bounds.size.width + pic_width;
   
   if (!by_me)
      rect = CGRectMake(5, y_pos, width, height);
   else
      rect = CGRectMake(scrollView.bounds.size.width - 5 - width,
                                                        y_pos, width, height);
   button.frame = rect;
   
   //show picture
   if (show_pic){
      view = (UIView *)[button viewWithTag:PICTURE_TAG];
      rect = CGRectMake(button.bounds.size.width - (30-x_margin) - pic_width - 2,
                        button.bounds.size.height - 8 - 50 - 2,
                        50 +2, 50 +2);
      view.frame = rect;
      view.hidden = NO;
   }
   
   //show mood
   imageView = (UIImageView *)[button viewWithTag:MOOD_TAG];
   rect = CGRectMake(button.bounds.size.width - (30-x_margin) - pic_width - 2 - 12,
                     button.bounds.size.height - 1 - imageView.bounds.size.height,
                     imageView.bounds.size.width, imageView.bounds.size.height);
   [imageView setFrame:rect];
}

- (void) resizeViews
{
   UIButton *button;
   iphoneLabel *label;
   UIImage *image;
   UIImage *leftImage = NULL;
   UIImage *leftImageSel = NULL;
   CGRect rect;
   CGFloat viewPosY;
   UIScrollView *scrollView = (UIScrollView *)self.view;
   int iCount = 0;
   RTAlert *alert;
   RTAlertCommentsEntry *CommentEntry;
   BOOL showAbuse = FALSE;
   UIButton *abuseBtn = (UIButton *)[scrollView viewWithTag:ABUSE_TAG];
   
   image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "comments_left");
   leftImage = [image stretchableImageWithLeftCapWidth:50 topCapHeight:50];
   image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "comments_left_sel");
   leftImageSel = [image stretchableImageWithLeftCapWidth:50 topCapHeight:50];
   
   viewPosY = 5;
   
   alert = RTAlerts_Get_By_ID(alertId);
   
   button = (UIButton *)[scrollView viewWithTag:ALERT_TAG];
   [self resizeButton:button byMe:alert->bAlertByMe showPic:alert->bShowFacebookPicture bSelected:FALSE yPos:viewPosY];
   viewPosY += button.bounds.size.height + 10;
   
   button = (UIButton *)[scrollView viewWithTag:THUMBS_TAG];
   if (button) {
      rect = CGRectMake(15, viewPosY, scrollView.bounds.size.width - 30, 30);
      button.frame = rect;
      [button sizeToFit];
      viewPosY += button.bounds.size.height + 10;
   }
  
   CommentEntry = alert->Comment;
   
   while (iCount < MAX_COMMENTS_ENTRIES && (CommentEntry != NULL))
   {
      button = (UIButton *)[scrollView viewWithTag:iCount + 100];

      [self resizeButton:button
                    byMe:CommentEntry->comment.bCommentByMe 
                  showPic:CommentEntry->comment.bShowFacebookPicture
               bSelected:(iCount == selectedComment)
                    yPos:viewPosY];
      
      if (selectedComment == iCount) {
         [button setBackgroundImage:leftImageSel forState:UIControlStateNormal];
         showAbuse = TRUE;
         rect = button.frame;
         CGRect btnRect = abuseBtn.frame;
         btnRect.origin.y = (rect.origin.y + (rect.size.height - btnRect.size.height) /2);
         btnRect.origin.x = rect.origin.x + rect.size.width - 10 - btnRect.size.width;
         if (btnRect.origin.x > scrollView.bounds.size.width - abuseBtn.bounds.size.width - 5)
            btnRect.origin.x = scrollView.bounds.size.width - abuseBtn.bounds.size.width - 5;
         abuseBtn.frame = btnRect;
      } else {
         if (!CommentEntry->comment.bCommentByMe)
            [button setBackgroundImage:leftImage forState:UIControlStateNormal];
      }
      
      viewPosY += button.bounds.size.height + 10;
      
      iCount++;
      CommentEntry = CommentEntry->next;
   }
   
   if (!alert->bArchive) {
      button = (UIButton *)[scrollView viewWithTag:iCount + 100];
      label = (iphoneLabel *)[button viewWithTag:CONTENT_TAG];
      
      //right side bubble
      rect = CGRectMake(5, 5, 170, minCommentHeight - 10);
      //label.backgroundColor = [UIColor redColor];
      label.frame = rect;
      rect = CGRectMake(scrollView.bounds.size.width - 205, viewPosY, 200, minCommentHeight);
      button.frame = rect;
      viewPosY += button.bounds.size.height + 10;
   }
   
   if (showAbuse) {
      
   } else {
   }
   
   scrollView.contentSize = CGSizeMake (scrollView.bounds.size.width, viewPosY + 10);
}

- (void) setImage:(RoadMapImage)image alert:(int)alert_id comment:(int)comment_id
{
   UIButton *button;
   UIImageView *pictureImage;
   unsigned char* image_buf;
   int buf_size;

   if (alert_id != alertId ||
       comment_id > RTAlerts_Get_By_ID(alertId)->iNumComments) {
      roadmap_log (ROADMAP_DEBUG, "Wrong image received (alert id: %d ; comment id: %d) current alert id: %d and comment count: %d",
                   alert_id, comment_id, alertId, RTAlerts_Get_By_ID(alertId)->iNumComments);
      return;
   }
   
   buf_size = roadmap_canvas_buf_from_image(image, &image_buf);
   if (!image_buf) {
      roadmap_log(ROADMAP_WARNING, "Empty image buffer");
      return;
   }
   
   int width = roadmap_canvas_image_width(image);
   int height = roadmap_canvas_image_height(image);
   
   // Copy the buffer to UIImage
   CGDataProviderRef provider = CGDataProviderCreateWithData(NULL, image_buf, buf_size, NULL);
   int bitsPerComponent = 8;
   int bitsPerPixel = 32;
   int bytesPerRow = 4*width;
   CGColorSpaceRef colorSpaceRef = CGColorSpaceCreateDeviceRGB();
   CGBitmapInfo bitmapInfo = kCGBitmapByteOrderDefault | kCGImageAlphaLast;
   CGColorRenderingIntent renderingIntent = kCGRenderingIntentDefault;
   CGImageRef imageRef = CGImageCreate(width, height, bitsPerComponent, bitsPerPixel, bytesPerRow, colorSpaceRef, bitmapInfo, provider, NULL, NO, renderingIntent);
   
   UIImage *img_uncached = [UIImage imageWithCGImage:imageRef];
   UIImage *img = [UIImage imageWithData: UIImagePNGRepresentation(img_uncached)]; //force cache
   
   UIScrollView *scrollView = (UIScrollView *)self.view;
   if (comment_id == -1)
      button = (UIButton *)[scrollView viewWithTag:ALERT_TAG];
   else
      button = (UIButton *)[scrollView viewWithTag:comment_id + 99];
   
   if (!button) {
      roadmap_log (ROADMAP_WARNING, "Could not find button to update");
      return;
   }
   
   pictureImage = (UIImageView *)[[[button viewWithTag:PICTURE_TAG] subviews] objectAtIndex:0];
   pictureImage.image = img;
   
   CGDataProviderRelease(provider);
   CGImageRelease(imageRef);
   free(image_buf);
}

- (void) onShowAlert
{
   RTAlerts_Popup_By_Id(alertId, FALSE);
	roadmap_main_show_root(NO);
}

- (void) onAddComment
{
	real_time_post_alert_comment_by_id(alertId);
}

- (int) getCommentFromCount: (int) count
{
   RTAlert *alert;
   RTAlertCommentsEntry *CommentEntry;
   int i;
   
   alert = RTAlerts_Get_By_ID(alertId);
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
   if (selectedComment > 0)
      Realtime_ReportAbuse(alertId, selectedComment);
   
   selectedComment = -1;
   abuseBtn = (UIButton *)[self.view viewWithTag:ABUSE_TAG];
   abuseBtn.hidden = YES;
}

- (void) onReportAbuse
{
   ssd_confirm_dialog_custom (roadmap_lang_get("Report abuse"), roadmap_lang_get("Reports by several users will disable this user from reporting"), FALSE,report_abuse_confirm_dlg_callback, self, roadmap_lang_get("Report"), roadmap_lang_get("Cancel")) ;
}

- (void) onSelectComment: (id) commentButtonId
{
   CGRect rect, btnRect;
   UIButton *abuseBtn;
   UIButton *commentBtn = (UIButton *)commentButtonId;
   int commentId = commentBtn.tag -100;   
   
   abuseBtn = (UIButton *)[self.view viewWithTag:ABUSE_TAG];
   
   if (commentId == selectedComment) {
         abuseBtn.hidden = YES;
      selectedComment = -1;
   } else {
      selectedComment = commentId;
      rect = commentBtn.frame;
      btnRect = abuseBtn.frame;
      btnRect.origin.y = (rect.origin.y + (rect.size.height - btnRect.size.height) /2);
      btnRect.origin.x = rect.origin.x + rect.size.width + 10;
      if (btnRect.origin.x > self.view.bounds.size.width - abuseBtn.bounds.size.width - 5)
         btnRect.origin.x = self.view.bounds.size.width - abuseBtn.bounds.size.width - 5;
      abuseBtn.frame = btnRect;
      abuseBtn.hidden = NO;
   }
   /*
   [UIView beginAnimations:NULL context:NULL];
   [UIView setAnimationDuration:0.5f];
   [UIView setAnimationCurve:UIViewAnimationCurveEaseInOut];
   */
   [self resizeViews];
   /*
   [UIView commitAnimations];
    */
}

- (void)createContent
{
   UIScrollView *scrollView = (UIScrollView *) self.view;
   zoom_t SavedZoom = -1;
   RoadMapPosition SavedPosition;
   UIImage *image = NULL;
   UIImage *leftImage = NULL;
   UIImage *leftImageSel = NULL;
   UIImage *rightImage = NULL;
   UIImageView *imageView = NULL;
   UIView *view;
   RTAlert *alert;
   NSString *content;
   iphoneLabel *label;
   UIButton *button;
   CGRect rect;
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
   image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "comments_left");
   leftImage = [image stretchableImageWithLeftCapWidth:50 topCapHeight:50];
   image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "comments_left_sel");
   leftImageSel = [image stretchableImageWithLeftCapWidth:50 topCapHeight:50];
   image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "comments_right");
   rightImage = [image stretchableImageWithLeftCapWidth:50 topCapHeight:50];
   minCommentHeight = leftImage.size.height;
   
   
   detail_str[0] = 0;
   
   /////////////
   //show alert
   alert = RTAlerts_Get_By_ID(alertId);
   position.longitude = alert->iLongitude;
   position.latitude = alert->iLatitude;
   
   
   //Set detail string
   
   //Add desc
   
   //snprintf(detail_str + strlen(detail_str), sizeof(detail_str)
   //         - strlen(detail_str), "%s - %s", alert->sAlertType, alert->sLocationStr);
   
   if (alert->sDescription && alert->sDescription[0])
   snprintf(detail_str + strlen(detail_str), sizeof(detail_str)
               - strlen(detail_str), "%s\n", alert->sDescription);
   
   //Show time of report
   now = time(NULL);
   timeDiff = (int)difftime(now, (time_t)alert->iReportTime);
   if (timeDiff <0)
      timeDiff = 0;
   
   if (alert->iType == RT_ALERT_TYPE_TRAFFIC_INFO)
      snprintf(detail_str + strlen(detail_str), sizeof(detail_str)
               - strlen(detail_str),"%s",
               roadmap_lang_get("Updated "));
   else
      snprintf(detail_str + strlen(detail_str), sizeof(detail_str)
               - strlen(detail_str),"%s",
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
      gps_pos = roadmap_trip_get_position("Location");
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
   
   
   
   button = [UIButton buttonWithType:UIButtonTypeCustom];
   
   if (!alert->bArchive)
      [button addTarget:self action:@selector(onShowAlert) forControlEvents:UIControlEventTouchUpInside];
   
   if (!alert->bAlertByMe) {
      image = leftImage;
   } else {
      image = rightImage;
   }
   
   [button setBackgroundImage:image
                     forState:UIControlStateNormal];
   button.tag = ALERT_TAG;
   [scrollView addSubview:button];
   
   content = [NSString stringWithUTF8String:detail_str];
   label = [[iphoneLabel alloc] initWithFrame:CGRectZero];
   [label setText:content];
   [label setNumberOfLines:0];
   [label setLineBreakMode:UILineBreakModeWordWrap];
   [label setBackgroundColor:[UIColor clearColor]];
   label.tag = CONTENT_TAG;
   label.autoresizesSubviews = YES;
   label.autoresizingMask = UIViewAutoresizingFlexibleBottomMargin;
   [button addSubview:label];
   [label release];
   
   //picture
   view = [[UIView alloc] initWithFrame:CGRectZero];
   view.tag = PICTURE_TAG;
   view.backgroundColor = [UIColor whiteColor];
   view.hidden = YES;
   [button addSubview:view];
   [view release];
   image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "facebook_default_image");
   imageView = [[UIImageView alloc] initWithImage:image];
   rect = imageView.frame;
   rect.origin.x = 1;
   rect.origin.y = 1;
   imageView.frame = rect;
   [view addSubview:imageView];
   [imageView release];
   if (alert->bShowFacebookPicture){
      image_download( alert->iID, -1, self);
   }
   
   //mood
   mood_str = roadmap_mood_to_string(alert->iMood);
   image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, mood_str);
   roadmap_path_free(mood_str);
   if (image) {
      imageView = [[UIImageView alloc] initWithImage:image];
      imageView.tag = MOOD_TAG;
      [button addSubview:imageView];
      [imageView release];
   }
   
   /////////////
   // Thumbs Up
   if (alert->iNumThumbsUp > 0){
      image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "thumbs_up");
      if (image) {
         char thumbs_text[256];
         if (alert->iNumThumbsUp == 1)
            snprintf(thumbs_text, sizeof(thumbs_text), "%s %s", roadmap_lang_get("Thanks from:"), roadmap_lang_get("one user") );
         else
            snprintf(thumbs_text, sizeof(thumbs_text), "%s %d %s", roadmap_lang_get("Thanks from:"), alert->iNumThumbsUp, roadmap_lang_get("users"));
         
         button = [UIButton buttonWithType:UIButtonTypeCustom];
         button.tag = THUMBS_TAG;
         button.contentHorizontalAlignment = UIControlContentHorizontalAlignmentLeft;
         button.autoresizingMask = UIViewAutoresizingFlexibleWidth;
         button.autoresizesSubviews = YES;
         button.titleLabel.font = [UIFont systemFontOfSize:14];
         button.contentEdgeInsets = UIEdgeInsetsMake(0, 10, 0, 10);
         [button setImage:image forState:UIControlStateNormal];
         image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "border_white");
         if (image) {
            [button setBackgroundImage:[image stretchableImageWithLeftCapWidth:image.size.width/2 topCapHeight:image.size.width/2]
                              forState:UIControlStateNormal];
            [button setTitle:[NSString stringWithUTF8String:thumbs_text] forState:UIControlStateNormal];
            [button setTitleColor:[UIColor blackColor] forState:UIControlStateNormal];
            [scrollView addSubview:button];
         }
      }
   }
   
   /////////////
   //show comments
   CommentEntry = alert->Comment;
   
   iCount = 0;
   
   while (iCount < MAX_COMMENTS_ENTRIES && (CommentEntry != NULL))
   {
      CommentStr[0] = 0;
      
      button = [UIButton buttonWithType:UIButtonTypeCustom];
      
      if (!CommentEntry->comment.bCommentByMe) {
         image = leftImage;
         [button addTarget:self action:@selector(onSelectComment:) forControlEvents:UIControlEventTouchUpInside];
      } else {
         image = rightImage;
      }
      
      [button setBackgroundImage:image
                        forState:UIControlStateNormal];
      button.tag = iCount + 100;
      [scrollView addSubview:button];
      
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
      label = [[iphoneLabel alloc] initWithFrame:CGRectZero];
      [label setText:content];
      [label setNumberOfLines:0];
      [label setLineBreakMode:UILineBreakModeWordWrap];
      [label setBackgroundColor:[UIColor clearColor]];
      label.tag = CONTENT_TAG;
      label.autoresizesSubviews = YES;
      label.autoresizingMask = UIViewAutoresizingFlexibleBottomMargin;
      [button addSubview:label];
      [label release];
      
      //picture
      view = [[UIView alloc] initWithFrame:CGRectZero];
      view.tag = PICTURE_TAG;
      view.backgroundColor = [UIColor whiteColor];
      view.hidden = YES;
      [button addSubview:view];
      [view release];
      image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "facebook_default_image");
      imageView = [[UIImageView alloc] initWithImage:image];
      rect = imageView.frame;
      rect.origin.x = 1;
      rect.origin.y = 1;
      imageView.frame = rect;
      [view addSubview:imageView];
      [imageView release];
      
      if (CommentEntry->comment.bShowFacebookPicture){
         image_download( alert->iID, CommentEntry->comment.iID, self);
      }
      
      //mood
      mood_str = roadmap_mood_to_string(CommentEntry->comment.iMood);
      image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, mood_str);
      roadmap_path_free(mood_str);
      if (image) {
         imageView = [[UIImageView alloc] initWithImage:image];
         imageView.tag = MOOD_TAG;
         [button addSubview:imageView];
         [imageView release];
      }
      
      iCount++;
      CommentEntry = CommentEntry->next;
   }
   
   
   //Add comment button
   if (!alert->bArchive) {
      image = rightImage;
      CommentStr[0] = 0;
      
      button = [UIButton buttonWithType:UIButtonTypeCustom];
      [button addTarget:self action:@selector(onAddComment) forControlEvents:UIControlEventTouchUpInside];
      [button setBackgroundImage:image
                        forState:UIControlStateNormal];
      button.tag = iCount + 100;
      [scrollView addSubview:button];
   }
   
   //comment description
   snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
            - strlen(CommentStr), "%s",
            roadmap_lang_get("Add a comment"));
   
   content = [NSString stringWithUTF8String:CommentStr];
   label = [[iphoneLabel alloc] initWithFrame:CGRectZero];
   [label setText:content];
   [label setNumberOfLines:0];
   [label setLineBreakMode:UILineBreakModeWordWrap];
   [label setBackgroundColor:[UIColor clearColor]];
   label.tag = CONTENT_TAG;
   [button addSubview:label];
   [label release];
   
   
   //abuse button
   button = [UIButton buttonWithType:UIButtonTypeCustom];
   button.tag = ABUSE_TAG;
   selectedComment = -1;
   button.hidden = YES;
   image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "button_report_abuse");
   [button setBackgroundImage:image forState:UIControlStateNormal];
   image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "button_report_abuse_down");
   [button setBackgroundImage:image forState:UIControlStateHighlighted];
   [button sizeToFit];
   [button addTarget:self action:@selector(onReportAbuse) forControlEvents:UIControlEventTouchUpInside];
   [scrollView addSubview:button];
}


- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
   return roadmap_main_should_rotate (interfaceOrientation);
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
   roadmap_device_event_notification( device_event_window_orientation_changed);
}


- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation duration:(NSTimeInterval)duration {
   [self resizeViews];
}


- (void)viewWillAppear:(BOOL)animated
{
	[self resizeViews];
   
   [super viewWillAppear:animated];
}


- (void) showWithAlert: (int)iAlertId
{
   UIScrollView *scrollView = [[UIScrollView alloc] initWithFrame:CGRectZero];
   [scrollView setBackgroundColor:roadmap_main_table_color()];
   scrollView.alwaysBounceVertical = YES;
   
   self.view = scrollView;
   [scrollView release];
   
   [self setTitle: [NSString stringWithUTF8String:roadmap_lang_get("Comments")]];
   
   alertId = iAlertId;
   selectedComment = -1;
   
   [self createContent];
   
   roadmap_main_push_view(self);
}

- (void)dealloc
{	
   set_download_delegate(NULL);
   [super dealloc];
}

@end


