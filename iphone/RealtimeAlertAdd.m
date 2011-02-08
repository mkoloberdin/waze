/* RealtimeAlertsAdd.m - Real Time Alerts Add new - single screen
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi R.
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
 *   See RealtimeAlerts.h
 */

#include "roadmap_main.h"
#include "roadmap_iphonemain.h"
#include "roadmap_path.h"
#include "roadmap_lang.h"
#include "roadmap_camera.h"
#include "Realtime.h"
#include "RealtimeAlerts.h"
#include "roadmap_list_menu.h"
#include "iphoneRealtimeAlertAdd.h"
#include "roadmap_trip.h"
#include "roadmap_mood.h"
#include "roadmap_social.h"
#include "ssd/ssd_widget.h"
#include "roadmap_iphonecamera.h"
#include "roadmap_camera_image.h"
#include "roadmap_config.h"
#include "roadmap_time.h"
#include "roadmap_groups.h"
#include "roadmap_res.h"
#include "roadmap_iphonecanvas.h"
#include "roadmap_messagebox.h"
#include "roadmap_device_events.h"
#include "editor/static/add_alert.h"
#include "widgets/iphoneLabel.h"
#include "widgets/iphoneSelect.h"
#include "widgets/iphoneTablePicker.h"
#include "roadmap_checklist.h"
#include "roadmap_recorder.h"
#include "roadmap_analytics.h"


static CGRect gRectCategory = {25.0f, 110.0f, 270.0f, 20.0f};
static CGRect gRectEditbox = {25.0f, 135.0f, 215.0f, 40.0f};
static CGRect gRectEditboxNoSelect = {25.0f, 25.0f, 215.0f, 145.0f};
static CGRect gRectImage = {15.0f, 245.0f, 290.0f, 40.0f};
static CGRect gRectImageLs = {320.0f, 20.0f, 145.0f, 40.0f};
static CGRect gRectGroupNote = {15.0f, 295.0f, 290.0f, 20.0f};
static CGRect gRectGroupNoteLs = {320.0f, 100.0f, 145.0f, 30.0f};
static CGRect gRectGroupSelect = {15.0f, 310.0f, 290.0f, 40.0f};
static CGRect gRectGroupSelectLs = {320.0f, 125.0f, 145.0f, 40.0f};
static CGRect gRectSend = {63.0f, 365.0f, 193.0f, 41.0f};
static CGRect gRectSendLs = {63.0f, 220.0f, 193.0f, 41.0f};
static CGRect gRectCameraText1 = {15.0f, 190.0f, 290.0f, 60.0f};
static CGRect gRectCameraText2 = {15.0f, 270.0f, 290.0f, 60.0f};
//static CGRect gRectCameraText1Ls = {300.0f, 10.0f, 160.0f, 135.0f};
//static CGRect gRectCameraText2Ls = {300.0f, 150.0f, 160.0f, 135.0f};

static float   gDirectionControlY = 180;
static float   gDirectionControlCameraY = 130;

#define ALERT_IMAGE_WIDTH     360
#define ALERT_IMAGE_HEIGHT    480

#define IMAGE_FILENAME "alert_capture.jpg"
#define VOICE_FILENAME "voice_capture.caf"
#define HIDE_TIMEOUT 10

#define SEPARATOR_TAG         1
#define SEPARATOR_LS_TAG      2
#define ALERT_TYPE_CAMERA  -1

enum camera_types {
   CAMERA_TYPE_SPEED = 0,
   CAMERA_TYPE_REDLIGHT,
   CAMERA_TYPE_DUMMY
};

static AlertAddView *alertView;
static int viewIsShown = 0;
static int g_timeout;
static char gCurrentImageId[ROADMAP_IMAGE_ID_BUF_LEN] = "";
static char gCurrentVoiceId[ROADMAP_VOICE_ID_BUF_LEN] = "";
static const char gEmptyStr[1] = "";

typedef struct tag_upload_context{
	int iType;
	char* desc;
	int iDirection;
   int iSubType;
   char *group;
}upload_Image_context;

//Voice record event
static const char* ANALYTICS_EVENT_VOICE_ALERT_NAME = "ALERT_WITH_VOICE";
//Took image event
static const char* ANALYTICS_EVENT_IMAGE_ALERT_NAME = "ALERT_WITH_IMAGE";





static void groups_callback (int value, int group) {
   int groups_count = roadmap_groups_get_num_following();
   int main_exists = 0;
   
   if (strlen(roadmap_groups_get_active_group_name()) > 0) {
      main_exists = 1;
   }
   
   if (value == groups_count) {
      roadmap_groups_set_selected_group_icon(gEmptyStr);
      roadmap_groups_set_selected_group_name(gEmptyStr);
   } else if (value == 0 && main_exists == 1) {
      roadmap_groups_set_selected_group_icon(roadmap_groups_get_active_group_icon());
      roadmap_groups_set_selected_group_name(roadmap_groups_get_active_group_name());
   } else {
      roadmap_groups_set_selected_group_icon(roadmap_groups_get_following_group_icon(value - main_exists));
      roadmap_groups_set_selected_group_name(roadmap_groups_get_following_group_name(value - main_exists));
   }
   
   roadmap_main_pop_view(YES);
}

static void show_groups() {
   int groups_count = roadmap_groups_get_num_following();   
   int i;
   
   NSMutableArray *dataArray = [NSMutableArray arrayWithCapacity:1];
	NSMutableArray *groupArray = NULL;
   NSMutableDictionary *dict = NULL;
   NSString *text;
   const char* icon;
   UIImage *image;
   NSNumber *accessoryType = [NSNumber numberWithInt:UITableViewCellAccessoryCheckmark];
   RoadMapChecklist *groupsView;
   int main_exists = 0;
   
   groupArray = [NSMutableArray arrayWithCapacity:1];
   
   //Add main group
   dict = [NSMutableDictionary dictionaryWithCapacity:1];
   if (strlen(roadmap_groups_get_active_group_name()) > 0) {
      text = [NSString stringWithUTF8String:roadmap_groups_get_active_group_name()];
      [dict setValue:text forKey:@"text"];
      icon = roadmap_groups_get_active_group_icon();
      if (icon && icon[0] != 0)
         image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, icon);
      else
         image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "groups_default_icons");
      
      [dict setValue:image forKey:@"image"];
      
      if (strcmp(roadmap_groups_get_active_group_name(), roadmap_groups_get_selected_group_name()) == 0) {
         [dict setObject:accessoryType forKey:@"accessory"];
      }
      [dict setValue:[NSNumber numberWithInt:1] forKey:@"selectable"];
      [groupArray addObject:dict];
      
      main_exists = 1;
   }
   
   //Add following groups
   for (i = 0; i < groups_count - main_exists; ++i) {
      dict = [NSMutableDictionary dictionaryWithCapacity:1];
      text = [NSString stringWithUTF8String:roadmap_groups_get_following_group_name(i)];
      [dict setValue:text forKey:@"text"];
      icon = roadmap_groups_get_following_group_icon(i);
      if (icon && icon[0] != 0)
         image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, icon);
      else
         image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "groups_default_icons");
      
      [dict setValue:image forKey:@"image"];
      
      if (strcmp(roadmap_groups_get_following_group_name(i), roadmap_groups_get_selected_group_name()) == 0) {
         [dict setObject:accessoryType forKey:@"accessory"];
      }
      [dict setValue:[NSNumber numberWithInt:1] forKey:@"selectable"];
      [groupArray addObject:dict];
   }
   
   //Add none option
      dict = [NSMutableDictionary dictionaryWithCapacity:1];
      text = [NSString stringWithUTF8String:roadmap_lang_get("No group")];
      [dict setValue:text forKey:@"text"];
      
      if (strlen(roadmap_groups_get_selected_group_name()) == 0) {
         [dict setObject:accessoryType forKey:@"accessory"];
      }
      [dict setValue:[NSNumber numberWithInt:1] forKey:@"selectable"];
      [groupArray addObject:dict];   
   
   [dataArray addObject:groupArray];
   
   text = [NSString stringWithUTF8String:roadmap_lang_get ("Groups")];
	groupsView = [[RoadMapChecklist alloc] 
                 activateWithTitle:text andData:dataArray andHeaders:NULL
                 andCallback:groups_callback andHeight:60 andFlags:CHECKLIST_DISABLE_CLOSE];
   
}


static void on_recorder_closed( int exit_code , void *context ) {
   if (viewIsShown) {
		[alertView onRecorderClosed];
	}
}


static void continue_report_after_audio_upload(void * context){
	int success;
	upload_Image_context * uploadContext = (upload_Image_context *)context;
   
	success = Realtime_Report_Alert(uploadContext->iType, uploadContext->iSubType, uploadContext->desc,
                                   uploadContext->iDirection, gCurrentImageId, gCurrentVoiceId,
                                   roadmap_twitter_is_sending_enabled() && roadmap_twitter_logged_in(),
                                   roadmap_facebook_is_sending_enabled() && roadmap_facebook_logged_in(),
                                   uploadContext->group);
   
	RTAlerts_CloseProgressDlg();
   
   roadmap_main_show_root(NO);
   
	free(uploadContext->desc);
	free(uploadContext);
}

static void continue_report_after_image_upload(void * context){
   const char *path = roadmap_path_first ("config");
	const char *file_name = VOICE_FILENAME;
   
	//Upload voice file
   gCurrentVoiceId[0] = 0;
   if (roadmap_file_exists(path, file_name)) {
      roadmap_analytics_log_event(ANALYTICS_EVENT_VOICE_ALERT, NULL, NULL);
      if (!roadmap_recorder_voice_upload( path, file_name,
                                       gCurrentVoiceId, continue_report_after_audio_upload, context )){
         roadmap_log(ROADMAP_ERROR,"Error in uploading voice alert");
         continue_report_after_audio_upload( ( context)); // error in image upload, continue
      }
   } else {
      continue_report_after_audio_upload( ( context));
   }
}

void periodic_hide ()
{
	
	[alertView periodicHide];
}

int valid_location()
{
	const RoadMapGpsPosition   *TripLocation;
	
   TripLocation = roadmap_trip_get_gps_position("AlertSelection");
	if ((TripLocation == NULL) /*|| (TripLocation->latitude <= 0) || (TripLocation->longitude <= 0)*/) {
   		roadmap_messagebox ("Error", "Can't find location.");
   		return 0;
    }
	
	return 1;
}

static int add_alert_request_valid () {
   if (!valid_location()) {
		roadmap_main_show_root(1);
		return FALSE;
	}
   
   if (viewIsShown) {
		[alertView release];
	}
   
   return TRUE;
}

void add_real_time_alert_for_police()
{
	if (!add_alert_request_valid()) {
		return;
	}
	
	alertView = [[AlertAddView alloc] init];
	[alertView showWithType:RT_ALERT_TYPE_POLICE];
	viewIsShown = 1;
}

void add_real_time_alert_for_accident()
{
	if (!add_alert_request_valid()) {
		return;
	}
	
	alertView = [[AlertAddView alloc] init];
	[alertView showWithType:RT_ALERT_TYPE_ACCIDENT];
	viewIsShown = 1;
}

void add_real_time_alert_for_traffic_jam()
{
	if (!add_alert_request_valid()) {
		return;
	}
	
	alertView = [[AlertAddView alloc] init];
	[alertView showWithType:RT_ALERT_TYPE_TRAFFIC_JAM];
	viewIsShown = 1;
}

void add_real_time_alert_for_hazard()
{
	if (!add_alert_request_valid()) {
		return;
	}
	
	alertView = [[AlertAddView alloc] init];
	[alertView showWithType:RT_ALERT_TYPE_HAZARD];
	viewIsShown = 1;
}

void add_real_time_alert_for_other()
{
	if (!add_alert_request_valid()) {
		return;
	}
	
	alertView = [[AlertAddView alloc] init];
	[alertView showWithType:RT_ALERT_TYPE_OTHER];
	viewIsShown = 1;
}

void add_real_time_alert_for_construction()
{
	if (!add_alert_request_valid()) {
		return;
	}
	
	alertView = [[AlertAddView alloc] init];
	[alertView showWithType:RT_ALERT_TYPE_CONSTRUCTION];
	viewIsShown = 1;
}

void add_real_time_alert_for_parking ()
{
	if (!add_alert_request_valid()) {
		return;
	}
	
	alertView = [[AlertAddView alloc] init];
	[alertView showWithType:RT_ALERT_TYPE_PARKING];
	viewIsShown = 1;
}

void add_real_time_chit_chat()
{
	if (!add_alert_request_valid()) {
		return;
	}
   
   if (Realtime_RandomUserMsg()) {
      return;
   }
   
   if (Realtime_AnonymousMsg()) {
      return;
   }
	
	alertView = [[AlertAddView alloc] init];
	[alertView showWithType:RT_ALERT_TYPE_CHIT_CHAT];
	viewIsShown = 1;
}

void add_cam_dlg(void){
   if (!add_alert_request_valid()) {
		return;
	}
	
	alertView = [[AlertAddView alloc] init];
	[alertView showCameraDialog];
	viewIsShown = 1;
}



///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
@implementation AlertScrollView

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	[alertView onCancelHide];
	
	[super touchesBegan:touches withEvent:event];
}


@end



///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
@implementation AlertAddView
@synthesize subtypeControl;
@synthesize directionControl;
@synthesize commentEditbox;
@synthesize collapsedView;
@synthesize iconImage;
@synthesize minimizedImage;
@synthesize imageButton;
@synthesize groupNote;
@synthesize groupButton;
@synthesize sendButton;
@synthesize animatedImageView;
@synthesize bgFrame;
@synthesize bgFrameButton;
@synthesize categoryLabel;
@synthesize cameraText1;
@synthesize cameraText2;

- (UIImage *)loadJpgImage: (const char *)name {
	const char *cursor;
	UIImage *img;
	for (cursor = roadmap_path_first ("config");
		 cursor != NULL;
		 cursor = roadmap_path_next ("config", cursor)){
		
		NSString *fileName = [NSString stringWithFormat:@"%s%s", cursor, name];
		
		img = [[UIImage alloc] initWithContentsOfFile: fileName];
		
		if (img) break; 
	}
	return (img);
}

- (void) setGroup {
	[self onCancelHide];
	show_groups();
}

- (void) setImage {
	[self onCancelHide];
	
	const char *path = roadmap_path_first ("config");
	const char *file_name = IMAGE_FILENAME;
	roadmap_file_remove(path, file_name);
		
	isTakingImage = 1;
	
   CGSize imageSize = CGSizeMake(ALERT_IMAGE_WIDTH, ALERT_IMAGE_HEIGHT);
	if (path != NULL)
		roadmap_camera_take_alert_picture(imageSize, CFG_CAMERA_IMG_QUALITY_DEFAULT, path, file_name);
}

- (void) onSendAlert {
   //do not allow another send click
   [sendButton removeTarget:self action:@selector(onSendAlert) forControlEvents:UIControlEventTouchUpInside];
   
	[self onCancelHide];
   
   isRestoreFocuse = TRUE;
	
	char *description;
   const char *alert_group;

	description = (char*)[[commentEditbox text] UTF8String];
   alert_group = roadmap_groups_get_selected_group_name();
   
   if (!description)
      description = "";
	
	const char *path = roadmap_path_first ("config");
	const char *file_name = IMAGE_FILENAME;
   
   RTAlerts_ShowProgressDlg();
   upload_Image_context * uploadContext = (upload_Image_context *)malloc(sizeof(upload_Image_context));
   uploadContext->desc = strdup(description);
   uploadContext->group = strdup(alert_group);
   uploadContext->iDirection = alertDirection;
   uploadContext->iSubType = alertSubType;
   uploadContext->iType = alertType;
	
	//Upload image
   gCurrentImageId[0] = 0;
   if (roadmap_file_exists(path, file_name)) {
      roadmap_analytics_log_event(ANALYTICS_EVENT_IMAGE_ALERT, NULL, NULL);
      if (!roadmap_camera_image_upload( path, file_name,
                                       gCurrentImageId, continue_report_after_image_upload, (void *)uploadContext )){
         roadmap_log(ROADMAP_ERROR,"Error in uploading image alert");
         continue_report_after_image_upload( ( (void *)uploadContext)); // error in image upload, continue
      }
   } else {
      continue_report_after_image_upload( ( (void *)uploadContext));
   }
}

- (void) onSendCamera {
   //do not allow another send click
   [sendButton removeTarget:self action:@selector(onSendCamera) forControlEvents:UIControlEventTouchUpInside];
   
   isRestoreFocuse = TRUE;
   
   switch (alertSubType) {
      case CAMERA_TYPE_SPEED:
         if (alertDirection == RT_ALERT_MY_DIRECTION)
            add_speed_cam_my_direction();
         else
            add_speed_cam_opposite_direction();
         break;
      case CAMERA_TYPE_REDLIGHT:
         if (alertDirection == RT_ALERT_MY_DIRECTION)
            add_red_light_cam_my_direction();
         else
            add_red_light_cam_opposite_direction();
         break;
      case CAMERA_TYPE_DUMMY:
         if (alertDirection == RT_ALERT_MY_DIRECTION)
            add_dummy_cam_my_direction();
         else
            add_dummy_cam_opposite_direction();
         break;
      default:
         return;
         break;
   }
   
   
   roadmap_main_show_root(NO);
}

- (void) onCancel {
	[self onCancelHide];
   
   isRestoreFocuse = TRUE;
	
	roadmap_main_show_root(NO);
}

- (void) onShow {
   isHidden = FALSE;
   
	[collapsedView removeFromSuperview];
	roadmap_main_push_view(self);
}

- (void) refreshRecordButton {
   // Set voice record status
   if (recordButton) {
      const char *path = roadmap_path_first ("config");
      const char *file_name = VOICE_FILENAME;
      UIImage *image;
      if (roadmap_file_exists(path, file_name)) {
         image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "button_record_taken");
      } else {
         image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "button_record");
      }
      [recordButton setImage:image forState:UIControlStateNormal];
   }
}

- (void) onRecorderClosed {
   [self refreshRecordButton];
}

- (void) onCancelHide {
	//TODO: do this only once
	roadmap_main_remove_periodic(periodic_hide);
   UINavigationItem *navItem = [self navigationItem];
   UIBarButtonItem *button = [navItem rightBarButtonItem];
   button.title = [NSString stringWithUTF8String:roadmap_lang_get("Hide")];
//	[hideButton setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Hide")] forState:UIControlStateNormal];
}

- (void) periodicHide {
	char hide_str[20];
	
	g_timeout--;
	if (g_timeout <= 0) {
		[self onHide];
	}
	else {
		sprintf(hide_str, "%s (%d)", roadmap_lang_get("Hide"), g_timeout);
      UINavigationItem *navItem = [self navigationItem];
      UIBarButtonItem *button = [navItem rightBarButtonItem];
      button.title = [NSString stringWithUTF8String:hide_str];
		//[hideButton setTitle:[NSString stringWithUTF8String:hide_str] forState:UIControlStateNormal];
	}
	
}

- (void) onHide {
	CGRect rect;
	
	[self onCancelHide];
	
	collapsedView = [[UIView alloc] initWithFrame:CGRectZero];
	UIButton *button = [UIButton buttonWithType:UIButtonTypeCustom];
	if (minimizedImage)
		[button setBackgroundImage:minimizedImage forState:UIControlStateNormal];
	
	[button sizeToFit];
	
	[button addTarget:self action:@selector(onShow) forControlEvents:UIControlEventTouchUpInside];
	[collapsedView addSubview:button];
	rect.origin.x = roadmap_canvas_width()*100/roadmap_screen_get_screen_scale();
	rect.origin.y = roadmap_canvas_height()*100/roadmap_screen_get_screen_scale() - 150;
	rect.size = button.frame.size;
	[collapsedView setFrame:rect];
	
	
   isHidden = TRUE;
	
	//Show collapsed view
	roadmap_canvas_show_view(collapsedView);
	
	[self retain];
	roadmap_main_show_root(1);
	
	//Animate into screen
   //TODO: this animation does not work since we have another animation after.
	rect = [collapsedView frame];
	[UIView beginAnimations:NULL context:NULL];
	rect.origin.x = roadmap_canvas_width()*100/roadmap_screen_get_screen_scale() - (button.frame.size.width);
	[UIView setAnimationDelay:0.3f];
	[UIView setAnimationDuration:0.3f];
	[UIView setAnimationCurve:UIViewAnimationCurveEaseOut];
	[collapsedView setFrame:rect];
	[UIView commitAnimations];
	
   
	//Add a small repeating animation
	rect.origin.x = roadmap_canvas_width()*100/roadmap_screen_get_screen_scale() - (button.frame.size.width - 5);
	[UIView beginAnimations:NULL context:NULL];
	[UIView setAnimationDelay:1.0f];
	[UIView setAnimationDuration:1.0f];
	[UIView setAnimationCurve:UIViewAnimationCurveLinear];
	[UIView setAnimationRepeatAutoreverses:YES];
	[UIView setAnimationRepeatCount:1e100f];
	[collapsedView setFrame:rect];
	[UIView commitAnimations];
	
	[collapsedView release];
}

- (void) resizeViews
{
   UIScrollView *contentView = (UIScrollView *) self.view;
   CGRect bounds = contentView.bounds;
   CGRect rect;
   UIView *view;
   int i;
   
   // image, cancel, hide buttons
   if (!roadmap_horizontal_screen_orientation()) {
      if (alertType != ALERT_TYPE_CAMERA) {
         //image
         imageButton.frame = gRectImage;
         
         //group
         groupNote.frame = gRectGroupNote;
         groupButton.frame = gRectGroupSelect;
      } else {
         cameraText1.hidden = NO;
         cameraText2.hidden = NO;
         cameraText1.frame = gRectCameraText1;
         cameraText2.frame = gRectCameraText2;
      }
      

      //buttons
      sendButton.frame = gRectSend;
      
      for (i = 0; i < [contentView.subviews count]; i++) {
         view = (UIView *)[contentView.subviews objectAtIndex:i];
         if (view.tag == SEPARATOR_TAG)
            view.hidden = NO;
         else if (view.tag == SEPARATOR_LS_TAG)
            view.hidden = YES;
      }
   } else {
      if (alertType != ALERT_TYPE_CAMERA) {
         //image
         imageButton.frame = gRectImageLs;
         
         //group
         groupNote.frame = gRectGroupNoteLs;
         groupButton.frame = gRectGroupSelectLs;
      } else {
         cameraText1.hidden = YES;
         cameraText2.hidden = YES;
//         cameraText1.frame = gRectCameraText1Ls;
//         cameraText2.frame = gRectCameraText2Ls;
      }
      
      
      //buttons
      sendButton.frame = gRectSendLs;
      
      for (i = 0; i < [contentView.subviews count]; i++) {
         view = (UIView *)[contentView.subviews objectAtIndex:i];
         for (i = 0; i < [contentView.subviews count]; i++) {
            view = (UIView *)[contentView.subviews objectAtIndex:i];
            if (view.tag == SEPARATOR_TAG)
               view.hidden = YES;
            else if (view.tag == SEPARATOR_LS_TAG)
               view.hidden = NO;
         }
      }
   }
   
   
   
   // frame background
   rect.origin = CGPointMake (5, 3);
   rect.size = CGSizeMake (bounds.size.width - 10, sendButton.frame.origin.y + sendButton.frame.size.height + 5);
   bgFrame.frame = rect;
   rect = bgFrameButton.frame;
   rect.origin.x = bounds.size.width - rect.size.width;
   rect.origin.y = 3;
   bgFrameButton.frame = rect;
   
   
   contentView.contentSize = CGSizeMake (bounds.size.width, bgFrame.bounds.size.height + 10);
}

- (void) onTablePickerChange: (id)object
{
   iphoneTablePicker *tablePicker = (iphoneTablePicker *)object;
   int index;
   
   index = [tablePicker getSelectedIndex];
   if (index < RTAlerts_get_num_categories(alertType, alertSubType)) {   
      alertSubType = RTAlerts_get_categories_subtype(alertType,alertSubType,index);
      //self.title = [NSString stringWithUTF8String:roadmap_lang_get(roadmap_lang_get(RTAlerts_get_title(NULL, alertType, alertSubType)))];
      categoryLabel.text = [NSString stringWithUTF8String:roadmap_lang_get(roadmap_lang_get(RTAlerts_get_title(NULL, alertType, alertSubType)))]; 
   }

   [tablePicker removeFromSuperview];
}

- (void) onSegmentChange: (id)object
{
   NSMutableArray *tableArray;
   NSMutableDictionary *dict;
   UIImage *image;
   iphoneTablePicker *tablePicker;
   iphoneSelect *selectControl = (iphoneSelect *)object;
   int num_categories, i;
   int new_subtype;
   
   [self onCancelHide];
   
   categoryLabel.text = NULL; 
   
   if (selectControl == directionControl) { //Direction changed
      if ([selectControl getSelectedSegment] == 0)
         alertDirection = RT_ALERT_MY_DIRECTION;
      else
         alertDirection = RT_ALERT_OPPSOITE_DIRECTION;
   } else if (selectControl == subtypeControl) { //Subtype changed
      new_subtype = [selectControl getSelectedSegment];
      if (alertType != ALERT_TYPE_CAMERA && new_subtype == alertSubType) {
         [selectControl setSelectedSegment:-1];
         alertSubType = RTAlerts_get_default_subtype(alertType);
      } else {
         alertSubType = new_subtype;
      }
      if (alertType != ALERT_TYPE_CAMERA) {
         self.title = [NSString stringWithUTF8String:roadmap_lang_get(roadmap_lang_get(roadmap_lang_get(RTAlerts_get_title(NULL, alertType, alertSubType))))];
         
         num_categories = RTAlerts_get_num_categories(alertType, alertSubType);
         if (num_categories > 0) {
            //show categiries table
            tableArray = [NSMutableArray arrayWithCapacity:3];
            for (i = 0; i < num_categories; i++) {
               dict = [NSMutableDictionary dictionaryWithCapacity:2];
               [dict setObject:[NSString 
                                stringWithUTF8String:roadmap_lang_get(RTAlerts_get_subtype_label(alertType, RTAlerts_get_categories_subtype(alertType,alertSubType,i)))]
                        forKey:@"text"];
               [tableArray addObject:dict];
            }
            
            //Add close option
            dict = [NSMutableDictionary dictionaryWithCapacity:2];
            [dict setObject:[NSString stringWithUTF8String:roadmap_lang_get("Cancel")]
                     forKey:@"text"];
            //image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "cancel");
//            if (image)
//               [dict setObject:image forKey:@"image"];
            [tableArray addObject:dict];
            
            CGRect rect;
            rect.size = ((UIScrollView *)self.view).contentSize;
            rect.origin = CGPointMake(0, 0);
            tablePicker = [[iphoneTablePicker alloc] initWithFrame:rect];
            [tablePicker populateWithData:tableArray andCallback:NULL andHeight:60 andWidth:240 andDelegate:self];
            [self.view addSubview:tablePicker];
            [tablePicker release];
         }
      } else {
         switch (alertSubType) {
            case CAMERA_TYPE_SPEED:
               self.title = [NSString stringWithUTF8String:roadmap_lang_get(roadmap_lang_get("Speed cam"))];
               break;
            case CAMERA_TYPE_REDLIGHT:
               self.title = [NSString stringWithUTF8String:roadmap_lang_get(roadmap_lang_get("Red light cam"))];
               break;
            case CAMERA_TYPE_DUMMY:
               self.title = [NSString stringWithUTF8String:roadmap_lang_get(roadmap_lang_get("Fake"))];
               break;
            default:
               break;
         }
         sendButton.enabled = YES;
      }
   }
}

- (void) onStartRecorder
{
   [self onCancelHide];
   
   const char *path = roadmap_path_first ("config");
	const char *file_name = VOICE_FILENAME;
	roadmap_file_remove(path, file_name);
   
	if (path != NULL)
		roadmap_recorder("", 10, on_recorder_closed, 1, path, file_name, NULL);
}

- (void) setupView
{
	UIScrollView *scrollView = (UIScrollView *) self.view;

	UIImage *image = NULL;
	UIImageView *imageView = NULL;
	NSString *text;
	CGRect rect;
	char report_icon[100];
	char minimized_icon[100];
   NSMutableArray *segmentsArray;
   NSMutableDictionary *dict;
   int num_sub_types;
	
	//background frame
	image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "border_white");
	if (image) {
		UIImage *strechedImage = [image stretchableImageWithLeftCapWidth:image.size.width/2 topCapHeight:image.size.height/2];
		bgFrame = [[UIImageView alloc] initWithImage:strechedImage];
		[scrollView addSubview:bgFrame];
		[bgFrame release];
	}
	
	//Alert type icon
	if (alertType != RT_ALERT_TYPE_CHIT_CHAT) {
		if (alertType == RT_ALERT_TYPE_ACCIDENT) {
			strcpy(report_icon, "reportaccident");
			strcpy(minimized_icon, "minimized_accident");
		} else if (alertType == RT_ALERT_TYPE_POLICE) {
			strcpy(report_icon, "reportpolice");
			strcpy(minimized_icon, "minimized_police");
		} else if (alertType == RT_ALERT_TYPE_TRAFFIC_JAM) {
			strcpy(report_icon, "reporttrafficjam");
			strcpy(minimized_icon, "minimized_trafficjam");
		} else if (alertType == RT_ALERT_TYPE_HAZARD) {
			strcpy(report_icon, "reporthazard");
			strcpy(minimized_icon, "minimized_hazard");
		} else if (alertType == RT_ALERT_TYPE_OTHER) { //TODO: this does not exist
			strcpy(report_icon, "reportother");
			strcpy(minimized_icon, "minimized_other");
		} else if (alertType == RT_ALERT_TYPE_PARKING) {
			strcpy(report_icon, "reportparking");
			strcpy(minimized_icon, "minimized_parking");
		}
	} else {
		//TODO: add suggested strings
		strcpy(report_icon, "reportincident");
		strcpy(minimized_icon, "minimized_chitchat");
	}
   
   //Sub types
   num_sub_types = RTAlerts_get_number_of_sub_types(alertType);
   alertSubType = RTAlerts_get_default_subtype(alertType);
   if (num_sub_types > 0) {
      int i;
      segmentsArray = [NSMutableArray arrayWithCapacity:num_sub_types];
      for (i = 0; i < num_sub_types; i++){
         dict = [NSMutableDictionary dictionaryWithCapacity:3];
         text = [NSString stringWithUTF8String:RTAlerts_get_subtype_label(alertType,  i)];
         [dict setObject:text forKey:@"text"];
         text = [NSString stringWithUTF8String:RTAlerts_get_subtype_icon(alertType,  i)];
         [dict setObject:text forKey:@"icon"];
         [dict setObject:text forKey:@"icon_sel"];
         [segmentsArray addObject:dict];
      }
      
      subtypeControl = [[iphoneSelect alloc] initWithFrame:CGRectZero];
      [subtypeControl createWithItems:segmentsArray selectedSegment:alertSubType delegate:self type:IPHONE_SELECT_TYPE_TEXT_IMAGE];
      rect = subtypeControl.bounds;
      rect.origin.y = 20;
      rect.origin.x = (320 - rect.size.width)/2;
      subtypeControl.frame = rect;
      [scrollView addSubview:subtypeControl];
      [subtypeControl release];
   } else {
      subtypeControl = NULL;
   }
   
   //Category
   categoryLabel = [[iphoneLabel alloc] initWithFrame:gRectCategory];
   categoryLabel.font = [UIFont systemFontOfSize:15];
   categoryLabel.textColor = [UIColor colorWithRed:.26 green:.52 blue:.72 alpha:1.0];
   [scrollView addSubview:categoryLabel];
   [categoryLabel release];
   
	//minimized icon
	if (minimizedImage)
		[minimizedImage release];
	minimizedImage = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, minimized_icon);
	
	//comment editbox
   if (subtypeControl)
      rect = gRectEditbox;
   else
      rect = gRectEditboxNoSelect;
	commentEditbox = [[UITextField alloc] initWithFrame:rect];
	[commentEditbox setBorderStyle:UITextBorderStyleRoundedRect];
	[commentEditbox setClearButtonMode:UITextFieldViewModeWhileEditing];
	[commentEditbox setReturnKeyType:UIReturnKeyDone];
	[commentEditbox setDelegate:self];
	[commentEditbox setPlaceholder:[NSString stringWithUTF8String:roadmap_lang_get("<Add description>")]];
	[scrollView addSubview:commentEditbox];
	[commentEditbox release];
   
   //record button
   recordButton = [UIButton buttonWithType:UIButtonTypeCustom];
   image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "button_record");
   [recordButton setBackgroundImage:image forState:UIControlStateNormal];
   [recordButton addTarget:self action:@selector(onStartRecorder) forControlEvents:UIControlEventTouchUpInside];
   rect = CGRectMake(commentEditbox.frame.origin.x + commentEditbox.frame.size.width + 10,
                     commentEditbox.frame.origin.y -1,
                     image.size.width, image.size.height);
   recordButton.frame = rect;
   [scrollView addSubview:recordButton];
   
   //direction button
   segmentsArray = [NSMutableArray arrayWithCapacity:num_sub_types];
   dict = [NSMutableDictionary dictionaryWithCapacity:1];
   text = [NSString stringWithUTF8String:roadmap_lang_get("My lane")];
   [dict setObject:text forKey:@"text"];
   [segmentsArray addObject:dict];
   dict = [NSMutableDictionary dictionaryWithCapacity:1];
   text = [NSString stringWithUTF8String:roadmap_lang_get("Other lane")];
   [dict setObject:text forKey:@"text"];
   [segmentsArray addObject:dict];
   
   directionControl = [[iphoneSelect alloc] initWithFrame:CGRectZero];
   [directionControl createWithItems:segmentsArray selectedSegment:0 delegate:self type:IPHONE_SELECT_TYPE_TEXT];
   rect = directionControl.bounds;
   rect.origin.y = gDirectionControlY;
   rect.origin.x = (320 - rect.size.width)/2;
   directionControl.frame = rect;
   [scrollView addSubview:directionControl];
   [directionControl release];

   //add separator
	image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "horizontal_separator");
   UIImage *separator = [image stretchableImageWithLeftCapWidth:1 topCapHeight:0];
   imageView = [[UIImageView alloc] initWithImage:separator];
   rect = imageView.bounds;
   rect.origin.x = 10;
   rect.origin.y = 235;
   rect.size.width = 300;
   imageView.frame = rect;
   imageView.tag = SEPARATOR_TAG;
   [scrollView addSubview:imageView];
   [imageView release];
   //landscape
   imageView = [[UIImageView alloc] initWithImage:separator];
   rect = imageView.bounds;
   rect.origin.x = 305;
   rect.origin.y = 78;
   rect.size.width = 165;
   imageView.frame = rect;
   imageView.tag = SEPARATOR_LS_TAG;
   [scrollView addSubview:imageView];
   [imageView release];
   
	//image button
	imageButton = [UIButton buttonWithType:UIButtonTypeCustom];
	
	image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "camera");
	if (image) {
      [imageButton setBackgroundImage:NULL forState:UIControlStateNormal];
		[imageButton setImage:image forState:UIControlStateNormal];
	}
	[imageButton setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Take a picture")] forState:UIControlStateNormal];
	[imageButton setTitleColor:[UIColor blackColor] forState:UIControlStateNormal];
	[imageButton setTitleEdgeInsets:UIEdgeInsetsMake(0, 10, 0, 10)];
//   [imageButton setImageEdgeInsets:UIEdgeInsetsMake(0, 15, 0, 0)];
   imageButton.contentHorizontalAlignment = UIControlContentHorizontalAlignmentLeft;
	[imageButton setFrame:gRectImage];
	[imageButton addTarget:self action:@selector(setImage) forControlEvents:UIControlEventTouchUpInside];
   imageButton.titleLabel.lineBreakMode = UILineBreakModeWordWrap;
   image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "button_arrow");
   if (image) {
      imageView = [[UIImageView alloc] initWithImage:image];
      rect = imageView.bounds;
      rect.origin.x = imageButton.bounds.size.width - imageView.bounds.size.width - 10;
      rect.origin.y = (imageButton.bounds.size.height - imageView.bounds.size.height)/2;
      imageView.frame = rect;
      [imageButton addSubview:imageView];
      [imageView release];
   }
	[scrollView addSubview:imageButton];
   
   //add separator
   imageView = [[UIImageView alloc] initWithImage:separator];
   rect = imageView.bounds;
   rect.origin.x = 10;
   rect.origin.y = 290;
   rect.size.width = 300;
   imageView.frame = rect;
   imageView.tag = SEPARATOR_TAG;
   [scrollView addSubview:imageView];
   [imageView release];
   //landscape
   imageView = [[UIImageView alloc] initWithImage:separator];
   rect = imageView.bounds;
   rect.origin.x = 305;
   rect.origin.y = 185;
   rect.size.width = 165;
   imageView.frame = rect;
   imageView.tag = SEPARATOR_LS_TAG;
   [scrollView addSubview:imageView];
   [imageView release];
   
   //group message
   if (roadmap_groups_get_num_following() > 0) {   
      text = [NSString stringWithUTF8String:roadmap_lang_get("Also send this report to:")];
      groupNote = [[iphoneLabel alloc] initWithFrame:gRectGroupNote];
      [groupNote setText:text];
      groupNote.lineBreakMode = UILineBreakModeWordWrap;
      groupNote.numberOfLines = 2;
      groupNote.font = [UIFont systemFontOfSize:13];
      groupNote.textColor = [UIColor grayColor];
      [scrollView addSubview:groupNote];
      [groupNote release];
      
      //group button
      groupButton = [UIButton buttonWithType:UIButtonTypeCustom];
      [groupButton setFrame:gRectGroupSelect];
      [groupButton setTitleColor:[UIColor blackColor] forState:UIControlStateNormal];
      [groupButton addTarget:self action:@selector(setGroup) forControlEvents:UIControlEventTouchUpInside];
      [groupButton setTitleEdgeInsets:UIEdgeInsetsMake(0, 10, 0, 10)];
//      [groupButton setImageEdgeInsets:UIEdgeInsetsMake(0, 150, 0, 0)];
      groupButton.contentHorizontalAlignment = UIControlContentHorizontalAlignmentLeft;
      groupButton.titleLabel.lineBreakMode = UILineBreakModeWordWrap;
      groupButton.titleLabel.numberOfLines = 2;
      image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "button_arrow");
      if (image) {
         imageView = [[UIImageView alloc] initWithImage:image];
         rect = imageView.bounds;
         rect.origin.x = imageButton.bounds.size.width - imageView.bounds.size.width - 10;
         rect.origin.y = (imageButton.bounds.size.height - imageView.bounds.size.height)/2;
         imageView.frame = rect;
         [groupButton addSubview:imageView];
         [imageView release];
      }
      [scrollView addSubview:groupButton];
      
      roadmap_groups_set_selected_group_name(roadmap_groups_get_active_group_name());
      roadmap_groups_set_selected_group_icon (roadmap_groups_get_active_group_icon());
      
      //add separator
      imageView = [[UIImageView alloc] initWithImage:separator];
      rect = imageView.bounds;
      rect.origin.x = 10;
      rect.origin.y = 350;
      rect.size.width = 300;
      imageView.frame = rect;
      imageView.tag = SEPARATOR_TAG;
      [scrollView addSubview:imageView];
      [imageView release];
   } else {
      groupButton = NULL;
      roadmap_groups_set_selected_group_name("");
      roadmap_groups_set_selected_group_icon ("");
   }
   
   //send button
	sendButton = [UIButton buttonWithType:UIButtonTypeCustom];
   sendButton.titleLabel.font = [UIFont boldSystemFontOfSize:20];
	[sendButton setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Send")] forState:UIControlStateNormal];
	image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "send_button");
	if (image) {
		[sendButton setBackgroundImage:image forState:UIControlStateNormal];
	}
	image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "send_button_s");
	if (image) {
		[sendButton setBackgroundImage:image forState:UIControlStateHighlighted];
	}
	[sendButton setFrame:gRectSend];
	[sendButton addTarget:self action:@selector(onSendAlert) forControlEvents:UIControlEventTouchUpInside];
	[scrollView addSubview:sendButton];
}

- (void) setupCameraView
{
	UIScrollView *scrollView = (UIScrollView *) self.view;
   
	UIImage *image = NULL;
	UIImageView *imageView = NULL;
	NSString *text;
	CGRect rect;
   NSMutableArray *segmentsArray;
   NSMutableDictionary *dict;
   int num_sub_types;
	
	//background frame
	image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "border_white");
	if (image) {
		UIImage *strechedImage = [image stretchableImageWithLeftCapWidth:image.size.width/2 topCapHeight:image.size.height/2];
		bgFrame = [[UIImageView alloc] initWithImage:strechedImage];
		[scrollView addSubview:bgFrame];
		[bgFrame release];
	}
   
   //Sub types
   num_sub_types = 3;
   alertSubType = -1;
   if (num_sub_types > 0) {
      int i;
      segmentsArray = [NSMutableArray arrayWithCapacity:num_sub_types];
      for (i = 0; i < num_sub_types; i++){
         switch (i) {
            case CAMERA_TYPE_SPEED:
               dict = [NSMutableDictionary dictionaryWithCapacity:3];
               text = [NSString stringWithUTF8String:roadmap_lang_get("Speed")];
               [dict setObject:text forKey:@"text"];
               text = [NSString stringWithUTF8String:"speedcam"];
               [dict setObject:text forKey:@"icon"];
               [dict setObject:text forKey:@"icon_sel"];
               [segmentsArray addObject:dict];
               break;
            case CAMERA_TYPE_REDLIGHT:
               dict = [NSMutableDictionary dictionaryWithCapacity:3];
               text = [NSString stringWithUTF8String:roadmap_lang_get("Red light")];
               [dict setObject:text forKey:@"text"];
               text = [NSString stringWithUTF8String:"redlightcam"];
               [dict setObject:text forKey:@"icon"];
               [dict setObject:text forKey:@"icon_sel"];
               [segmentsArray addObject:dict];
               break;
            case CAMERA_TYPE_DUMMY:
               dict = [NSMutableDictionary dictionaryWithCapacity:3];
               text = [NSString stringWithUTF8String:roadmap_lang_get("Fake")];
               [dict setObject:text forKey:@"text"];
               text = [NSString stringWithUTF8String:"dummy_cam"];
               [dict setObject:text forKey:@"icon"];
               [dict setObject:text forKey:@"icon_sel"];
               [segmentsArray addObject:dict];
               break;
            default:
               break;
         }
      }      
      
      subtypeControl = [[iphoneSelect alloc] initWithFrame:CGRectZero];
      [subtypeControl createWithItems:segmentsArray selectedSegment:alertSubType delegate:self type:IPHONE_SELECT_TYPE_TEXT_IMAGE];
      rect = subtypeControl.bounds;
      rect.origin.y = 20;
      rect.origin.x = (320 - rect.size.width)/2;
      subtypeControl.frame = rect;
      [scrollView addSubview:subtypeControl];
      [subtypeControl release];
   } else {
      subtypeControl = NULL;
   }
   
   //direction button
   segmentsArray = [NSMutableArray arrayWithCapacity:num_sub_types];
   dict = [NSMutableDictionary dictionaryWithCapacity:1];
   text = [NSString stringWithUTF8String:roadmap_lang_get("My lane")];
   [dict setObject:text forKey:@"text"];
   [segmentsArray addObject:dict];
   dict = [NSMutableDictionary dictionaryWithCapacity:1];
   text = [NSString stringWithUTF8String:roadmap_lang_get("Other lane")];
   [dict setObject:text forKey:@"text"];
   [segmentsArray addObject:dict];
   
   directionControl = [[iphoneSelect alloc] initWithFrame:CGRectZero];
   [directionControl createWithItems:segmentsArray selectedSegment:0 delegate:self type:IPHONE_SELECT_TYPE_TEXT];
   rect = directionControl.bounds;
   rect.origin.y = gDirectionControlCameraY;
   rect.origin.x = (320 - rect.size.width)/2;
   directionControl.frame = rect;
   [scrollView addSubview:directionControl];
   [directionControl release];
   
   
   //Add text 1
   cameraText1 = [[iphoneLabel alloc] initWithFrame:gRectCameraText1];
   cameraText1.text = [NSString stringWithUTF8String:roadmap_lang_get("Wazers are notified of speed cams only when approaching at an excessive speed.")];
   cameraText1.numberOfLines = 0;
   cameraText1.textColor = [UIColor colorWithRed:0.55 green:0.55 blue:0.55 alpha:1.0];
   [scrollView addSubview:cameraText1];
   [cameraText1 release];
   
   //add separator
	image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "horizontal_separator");
   UIImage *separator = [image stretchableImageWithLeftCapWidth:1 topCapHeight:0];
   imageView = [[UIImageView alloc] initWithImage:separator];
   rect = imageView.bounds;
   rect.origin.x = 10;
   rect.origin.y = 180;
   rect.size.width = 300;
   imageView.frame = rect;
   imageView.tag = SEPARATOR_TAG;
   [scrollView addSubview:imageView];
   [imageView release];
   

   //Add text 2
   cameraText2 = [[iphoneLabel alloc] initWithFrame:gRectCameraText2];
   cameraText2.text = [NSString stringWithUTF8String:roadmap_lang_get("Note: New speed cams need to be validated by community map editors. You can do it, too: www.waze.com")];
   cameraText2.numberOfLines = 0;
   cameraText2.textColor = [UIColor colorWithRed:0.55 green:0.55 blue:0.55 alpha:1.0];
   [scrollView addSubview:cameraText2];
   [cameraText2 release];
   
   //add separator
   //imageView = [[UIImageView alloc] initWithImage:separator];
//   rect = imageView.bounds;
//   rect.origin.x = 10;
//   rect.origin.y = 290;
//   rect.size.width = 300;
//   imageView.frame = rect;
//   imageView.tag = SEPARATOR_TAG;
//   [scrollView addSubview:imageView];
//   [imageView release];
   
   
   
   //send button
	sendButton = [UIButton buttonWithType:UIButtonTypeCustom];
   sendButton.titleLabel.font = [UIFont boldSystemFontOfSize:20];
	[sendButton setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Send")] forState:UIControlStateNormal];
	image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "send_button");
	if (image) {
		[sendButton setBackgroundImage:image forState:UIControlStateNormal];
	}
	image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "send_button_s");
	if (image) {
		[sendButton setBackgroundImage:image forState:UIControlStateHighlighted];
	}
	[sendButton setFrame:gRectSend];
	[sendButton addTarget:self action:@selector(onSendCamera) forControlEvents:UIControlEventTouchUpInside];
   sendButton.enabled = NO;
	[scrollView addSubview:sendButton];
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
	UIImage *image;
	char hide_str[20];
	
	if (!initialized) {
      if (alertType != ALERT_TYPE_CAMERA) {
         [self setupView];
         
         g_timeout = HIDE_TIMEOUT;
         roadmap_main_set_periodic (1000, periodic_hide);
         sprintf(hide_str, "%s (%d)", roadmap_lang_get("Hide"), g_timeout);
         UINavigationItem *navItem = [self navigationItem];
         UIBarButtonItem *button = [navItem rightBarButtonItem];
         button.title = [NSString stringWithUTF8String:hide_str];
      } else {
         [self setupCameraView];
      }
      
		isTakingImage = 0;
		animatedImageView = NULL;
		initialized = TRUE;
	}
	
	// Set image status
   if (imageButton) {
      const char *path = roadmap_path_first ("config");
      const char *file_name = IMAGE_FILENAME;
      if (roadmap_file_exists(path, file_name)) {
         image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "picture_added");
         [imageButton setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Picture added")] forState:UIControlStateNormal];
      } else {
         image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "camera");
         [imageButton setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Take a picture")] forState:UIControlStateNormal];
      }
      [imageButton setImage:image forState:UIControlStateNormal];
   }
   
   
   [self refreshRecordButton];
	
	// Set group image and text
   if (groupButton) {
      const char* group_name = roadmap_groups_get_selected_group_name();
      if (group_name && group_name[0]) {
         [groupButton setTitle:[NSString stringWithUTF8String:group_name]
                      forState:UIControlStateNormal];
         
         image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, roadmap_groups_get_selected_group_icon());
         if (!image) {
            image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "groups_default_icons");
         }
         if (image) {
            [groupButton setImage:image forState:UIControlStateNormal];
            [groupButton setImageEdgeInsets:UIEdgeInsetsMake(3, 0, 2, 10)];
         }
      } else {
         [groupButton setTitle:[NSString stringWithUTF8String:roadmap_lang_get("No group")]
                      forState:UIControlStateNormal];
         [groupButton setImage:NULL forState:UIControlStateNormal];
      }
      
	}

   [self resizeViews];
   
	[super viewWillAppear:animated];
}

- (void)scrollViewWillBeginDragging:(UIScrollView *)scrollView
{
	[self onCancelHide];
}


- (void) showWithType: (int)alert_type
{
   initialized = FALSE;
   isHidden = FALSE;
   isRestoreFocuse = FALSE;
	
	//Clean old image and voice if exist
	const char *path = roadmap_path_first ("config");
	const char *file_name = IMAGE_FILENAME;
	roadmap_file_remove(path, file_name);
   file_name = VOICE_FILENAME;
	roadmap_file_remove(path, file_name);
	
	
	AlertScrollView *scrollView = [[AlertScrollView alloc] initWithFrame:CGRectZero];
   scrollView.alwaysBounceVertical = YES;
	self.view = scrollView;
	[scrollView release]; // decrement retain count
	[scrollView setDelegate:self];
	[self setTitle:[NSString stringWithUTF8String:RTAlerts_get_title(NULL, alert_type, -1)]];
	UINavigationItem *navItem = [self navigationItem];
	UIBarButtonItem *button = [[UIBarButtonItem alloc] initWithTitle:[NSString stringWithUTF8String:roadmap_lang_get("Hide")]
															   style:UIBarButtonItemStylePlain target:self action:@selector(onHide)];

	[navItem setRightBarButtonItem:button];
	[button release];
	
	alertType = alert_type;
	alertDirection = RT_ALERT_MY_DIRECTION;
	
	roadmap_main_push_view(self);

}

- (void) showCameraDialog
{
   initialized = FALSE;
   isHidden = FALSE;
   isRestoreFocuse = FALSE;
   alertType = ALERT_TYPE_CAMERA;
	
	AlertScrollView *scrollView = [[AlertScrollView alloc] initWithFrame:CGRectZero];
   scrollView.alwaysBounceVertical = YES;
	self.view = scrollView;
	[scrollView release]; // decrement retain count
	[scrollView setDelegate:self];
	[self setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Speed cam")]];
	
	alertDirection = RT_ALERT_MY_DIRECTION;
	
	roadmap_main_push_view(self);
   
}


- (void)dealloc
{
	roadmap_main_remove_periodic(periodic_hide);
	
	const char *path = roadmap_path_first ("config");
	const char *file_name = IMAGE_FILENAME;
	roadmap_file_remove(path, file_name);
   file_name = VOICE_FILENAME;
	roadmap_file_remove(path, file_name);
   
   if (isHidden)
      [collapsedView removeFromSuperview];
	
   if (isRestoreFocuse)
      roadmap_trip_restore_focus();
	
	viewIsShown = 0;
	
	[super dealloc];
}

//////////////////////
//Text field delegate
- (void)textFieldDidEndEditing:(UITextField *)textField {

}

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
	[textField resignFirstResponder];
	return NO;
}

- (BOOL)textFieldShouldBeginEditing:(UITextField *)textField
{
	[self onCancelHide];
	
	return YES;
}

- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string
{
   if (roadmap_keyboard_typing_locked(TRUE)) {
      [textField resignFirstResponder];
      return NO;
   } else
      return YES;
}


@end