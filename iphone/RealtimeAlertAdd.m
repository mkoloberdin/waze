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
#include "roadmap_iphoneimage.h"
#include "roadmap_iphonecanvas.h"
#include "roadmap_messagebox.h"
#include "roadmap_device_events.h"
#include "widgets/iphoneLabel.h"


static CGRect gRectNote = {25.0f, 15.0f, 250.0f, 17.0f};
static CGRect gRectIcon = {25.0f, 30.0f, 50.0f, 40.0f};
static CGRect gRectDescription = {75.0f, 35.0f, 200.0f, 35.0f};
static CGRect gRectEditbox = {25.0f, 80.0f, 270.0f, 40.0f};
static CGRect gRectDirection = {25.0f, 135.0f, 125.0f, 35.0f};
static CGRect gRectMood = {165.0f, 135.0f, 125.0f, 35.0f};
static CGRect gRectImage = {80.0f, 185.0f, 150.0f, 120.0f};
static CGRect gRectImageLs = {305.0f, 50.0f, 150.0f, 120.0f};
static CGRect gRectCancel = {39.0f, 335.0f, 100.0f, 28.0f};
static CGRect gRectCancelLs = {39.0f, 200.0f, 100.0f, 28.0f};
static CGRect gRectHide = {178.0f, 335.0f, 100.0f, 28.0f};
static CGRect gRectHideLs = {178.0f, 200.0f, 100.0f, 28.0f};

enum directions {
	MY_DIRECTION = 100,
	OPPOSITE_DIRECTION
};

#define ALERT_IMAGE_SCALE 0.3 //360 x 481

#define IMAGE_FILENAME "alert_capture.jpg"
#define HIDE_TIMEOUT 10

static AlertAddView *alertView;
static int viewIsShown = 0;
static int g_timeout;
static char gCurrentImageId[ROADMAP_IMAGE_ID_BUF_LEN] = "";

typedef struct tag_upload_context{
	int iType;
	char* desc;
	int iDirection;
}upload_Image_context;



static void continue_report_after_image_upload(void * context){
	int success;
	upload_Image_context * uploadContext = (upload_Image_context *)context;
   
	success = Realtime_Report_Alert(uploadContext->iType, uploadContext->desc,
                                   uploadContext->iDirection, gCurrentImageId,
                                   roadmap_twitter_is_sending_enabled() && roadmap_twitter_logged_in(),
                                   roadmap_facebook_is_sending_enabled() && roadmap_facebook_logged_in());
   
	RTAlerts_CloseProgressDlg();
   
   roadmap_main_show_root(NO);
   
	free(uploadContext->desc);
	free(uploadContext);
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
	
	alertView = [[AlertAddView alloc] init];
	[alertView showWithType:RT_ALERT_TYPE_CHIT_CHAT];
	viewIsShown = 1;
}





@implementation AlertScrollView

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	[alertView onCancelHide];
	
	[super touchesBegan:touches withEvent:event];
}


@end




@implementation AlertAddView
@synthesize commentEditbox;
@synthesize collapsedView;
@synthesize iconImage;
@synthesize minimizedImage;
@synthesize imageButton;
@synthesize moodButton;
@synthesize hideButton;
@synthesize cancelButton;
@synthesize animatedImageView;
@synthesize bgFrame;
@synthesize bgFrameButton;

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

- (void) setDirection
{
	UIImage *image;
	UIButton *newButtonDirection = [UIButton buttonWithType:UIButtonTypeRoundedRect];
	[newButtonDirection setFrame:[buttonDirectionView bounds]];
	[newButtonDirection addTarget:self action:@selector(onDirection) forControlEvents:UIControlEventTouchUpInside];
   newButtonDirection.titleLabel.lineBreakMode = UILineBreakModeCharacterWrap;
   newButtonDirection.titleLabel.font = [UIFont boldSystemFontOfSize:12.0f];
	
	
	if (alertDirection == MY_DIRECTION) {
		[newButtonDirection setTitle:[NSString stringWithUTF8String:roadmap_lang_get ("My direction")] forState:UIControlStateNormal];
		image = roadmap_iphoneimage_load("mydirection");
	} else {
		[newButtonDirection setTitle:[NSString stringWithUTF8String:roadmap_lang_get ("Opposite direction")] forState:UIControlStateNormal];
		image = roadmap_iphoneimage_load("oppositedirection");
	}
	
	if (image) {
		[newButtonDirection setImage:image forState:UIControlStateNormal];
		[newButtonDirection setImageEdgeInsets:UIEdgeInsetsMake(5, 0, 5, 10)];
		[image release];
	}
	
	
	if ([[buttonDirectionView subviews] count] > 0) {
		[UIView beginAnimations:NULL context:NULL];
		[UIView setAnimationDuration:0.5f];
		[UIView setAnimationCurve:UIViewAnimationCurveEaseInOut];
		
		if (alertDirection == MY_DIRECTION)
			[UIView setAnimationTransition: UIViewAnimationTransitionFlipFromRight forView:buttonDirectionView cache:YES];
		else
			[UIView setAnimationTransition: UIViewAnimationTransitionFlipFromLeft forView:buttonDirectionView cache:YES];
		
		[[[buttonDirectionView subviews] objectAtIndex:0] removeFromSuperview];
		
		[buttonDirectionView addSubview:newButtonDirection];
		
		[UIView commitAnimations];
	} else
		[buttonDirectionView addSubview:newButtonDirection];
}

- (void) onDirection {
	[self onCancelHide];
	
	//TODO: use the global directions RT_ALERT_MY_DIRECTION RT_ALERT_OPPSOITE_DIRECTION;
	if (alertDirection == OPPOSITE_DIRECTION) {
		alertDirection = MY_DIRECTION;
	} else {
		alertDirection = OPPOSITE_DIRECTION;
	}
	
	[self setDirection];

}

- (void) setMood {
	[self onCancelHide];
	roadmap_mood_dialog(NULL);
}

- (void) setImage {
	[self onCancelHide];
	
	const char *path = roadmap_path_first ("config");
	const char *file_name = IMAGE_FILENAME;
	roadmap_file_remove(path, file_name);
	
	//Clear the old image
	UIImage *image = roadmap_iphoneimage_load("add_image_box");
	if (image) {
		[imageButton setBackgroundImage:image forState:UIControlStateNormal];
		[image release];
	}
	[imageButton setTitle:@"Add image" forState:UIControlStateNormal];
   if (!roadmap_horizontal_screen_orientation())
      [imageButton setFrame:gRectImage];
   else
      [imageButton setFrame:gRectImageLs];
	
	if (animatedImageView) {
		[animatedImageView removeFromSuperview];
		animatedImageView = NULL;
	}
	
	isTakingImage = 1;
	
	if (path != NULL)
		roadmap_camera_take_alert_picture(ALERT_IMAGE_SCALE, CFG_CAMERA_IMG_QUALITY_DEFAULT, path, file_name);
}

- (void) sendAlert {
	//NSLog(@"sendAlert");
	[self onCancelHide];
   
   isRestoreFocuse = TRUE;
	
	char *description;

	description = (char*)[[commentEditbox text] UTF8String];
   
   if (!description)
      description = "";
	
	int alert_type = alertType;
	int alert_direction;
	const char *path = roadmap_path_first ("config");
	const char *file_name = IMAGE_FILENAME;
	
	if (alertDirection == MY_DIRECTION)
		alert_direction = RT_ALERT_MY_DIRECTION;
	else
		alert_direction = RT_ALERT_OPPSOITE_DIRECTION;
   
   RTAlerts_ShowProgressDlg();
   upload_Image_context * uploadContext = (upload_Image_context *)malloc(sizeof(upload_Image_context));
   uploadContext->desc = strdup(description);
   uploadContext->iDirection = alert_direction;
   uploadContext->iType = alert_type;
	
	//Upload image
   gCurrentImageId[0] = 0;
   if (roadmap_file_exists(path, file_name)) {
      if (!roadmap_camera_image_upload( path, file_name,
                                       gCurrentImageId, continue_report_after_image_upload, (void *)uploadContext )){
         roadmap_log(ROADMAP_ERROR,"Error in uploading image alert");
         continue_report_after_image_upload( ( (void *)uploadContext)); // error in image upload, continue
      }
   } else {
      continue_report_after_image_upload( ( (void *)uploadContext));
   }
      	
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

- (void) onCancelHide {
	//TODO: do this only once
	roadmap_main_remove_periodic(periodic_hide);
	[hideButton setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Hide")] forState:UIControlStateNormal];
}

- (void) periodicHide {
	char hide_str[20];
	
	g_timeout--;
	if (g_timeout <= 0) {
		[self onHide];
	}
	else {
		sprintf(hide_str, "%s (%d)", roadmap_lang_get("Hide"), g_timeout);
		[hideButton setTitle:[NSString stringWithUTF8String:hide_str] forState:UIControlStateNormal];
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
	rect.origin.x = roadmap_canvas_width();
	rect.origin.y = roadmap_canvas_height() - 110;
	rect.size = button.frame.size;
	[collapsedView setFrame:rect];
	
	
   isHidden = TRUE;
	
	//Show collapsed view
	roadmap_canvas_show_view(collapsedView);
	
	[self retain];
	roadmap_main_show_root(1);
	
	//Animate into screen
	rect = [collapsedView frame];
	[UIView beginAnimations:NULL context:NULL];
	rect.origin.x = roadmap_canvas_width() - button.frame.size.width;
	[UIView setAnimationDelay:0.3f];
	[UIView setAnimationDuration:0.3f];
	[UIView setAnimationCurve:UIViewAnimationCurveEaseOut];
	[collapsedView setFrame:rect];
	[UIView commitAnimations];
	

	//Add a small repeating animation
	rect.origin.x = roadmap_canvas_width() - button.frame.size.width + 5;
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

- (void)getLocation:(char *)sLocationStr direction:(int)iDirection
{
	const char *street;
	const char *city;
	int iLineId;
	int	iSquare;
	int alert_direction;
	RoadMapPosition AlertPosition;
	const RoadMapGpsPosition   *TripLocation;
	
	if (iDirection == MY_DIRECTION)
		alert_direction = RT_ALERT_MY_DIRECTION;
	else
		alert_direction = RT_ALERT_OPPSOITE_DIRECTION;
	
	sLocationStr[0] = 0;
	
	TripLocation = roadmap_trip_get_gps_position("AlertSelection");
	if (!TripLocation)
		return;
	
	AlertPosition.latitude = TripLocation->latitude;
	AlertPosition.longitude = TripLocation->longitude;
	
	RTAlerts_Get_City_Street(AlertPosition, &city, &street, &iSquare, &iLineId, alert_direction);
	if (!((city == NULL) && (street == NULL)))
    {
		if ((city != NULL) && (strlen(city) == 0))
         	sprintf( sLocationStr,"%s", street);
		else if ((street != NULL) && (strlen(street) == 0))
            sprintf(sLocationStr,"%s", city);
		else
            sprintf(sLocationStr,"%s, %s", street, city);
    }
}

- (void) resizeViews
{
   UIScrollView *contentView = (UIScrollView *) self.view;
   CGRect bounds = contentView.bounds;
   CGRect rect;
   
   // image, cancel, hide buttons
   if (!roadmap_horizontal_screen_orientation()) {
      //image
      rect = imageButton.frame;
      rect.origin.x = gRectImage.origin.x + (gRectImage.size.width - imageButton.frame.size.width)/2;
      rect.origin.y = gRectImage.origin.y + (gRectImage.size.height - imageButton.frame.size.height)/2;
      imageButton.frame = rect;
      animatedImageView.frame = rect;
      //buttons
      cancelButton.frame = gRectCancel;
      hideButton.frame = gRectHide;
   } else {
      //image
      rect = imageButton.frame;
      rect.origin.x = gRectImageLs.origin.x + (gRectImageLs.size.width - imageButton.frame.size.width)/2;
      rect.origin.y = gRectImageLs.origin.y + (gRectImageLs.size.height - imageButton.frame.size.height)/2;
      imageButton.frame = rect;
      animatedImageView.frame = rect;
      //buttons
      cancelButton.frame = gRectCancelLs;
      hideButton.frame = gRectHideLs;
   }
   
   
   
   // frame background
   rect.origin = CGPointMake (5, 3);
   rect.size = CGSizeMake (bounds.size.width - 5, hideButton.frame.origin.y + hideButton.frame.size.height + 10);
   bgFrame.frame = rect;
   rect = bgFrameButton.frame;
   rect.origin.x = bounds.size.width - rect.size.width;
   rect.origin.y = 3;
   bgFrameButton.frame = rect;
   
   
   contentView.contentSize = CGSizeMake (bounds.size.width, bgFrame.bounds.size.height + 10);
}

- (void) setupView
{
	UIScrollView *scrollView = (UIScrollView *) self.view;

	UIImage *image = NULL;
	UIImageView *imageView = NULL;
	NSString *text;
	iphoneLabel *label;
	CGRect rect;
	char description[300];
	char report_icon[100];
	char minimized_icon[100];
	char sLocationStr[200];
	
	//background frame
	image = roadmap_iphoneimage_load("report_frame");
	if (image) {
		UIImage *strechedImage = [image stretchableImageWithLeftCapWidth:20 topCapHeight:60];
		bgFrame = [[UIImageView alloc] initWithImage:strechedImage];
		[image release];
		[scrollView addSubview:bgFrame];
		[bgFrame release];
	}
	
	//frame arrow (button)
	image = roadmap_iphoneimage_load("report_frame_arrow");
	if (image) {
		bgFrameButton = [UIButton buttonWithType:UIButtonTypeCustom];
		[bgFrameButton setBackgroundImage:image forState:UIControlStateNormal];
		rect = CGRectZero;
		rect.size = image.size;
		[image release];
		[bgFrameButton setFrame:rect];
		[bgFrameButton addTarget:self action:@selector(onHide) forControlEvents:UIControlEventTouchUpInside];
		[scrollView addSubview:bgFrameButton];
	}
	
	
	//note message
	text = [NSString stringWithUTF8String:roadmap_lang_get("Note: Location and time saved")];
	label = [[iphoneLabel alloc] initWithFrame:gRectNote];
	[label setText:text];
	[label setTextColor:[UIColor  colorWithRed:0.0f green:0.412f blue:0.584f alpha:1.0f]];
	[scrollView addSubview:label];
	[label release];
	
	//description of alert
	//We do not use full description now, only the location string
	description[0] = 0;
	if (alertType != RT_ALERT_TYPE_CHIT_CHAT) {
		snprintf(description + strlen(description), sizeof(description)
				 - strlen(description), "%s ", roadmap_lang_get("Reporting"));
		if (alertType == RT_ALERT_TYPE_ACCIDENT) {
			snprintf(description + strlen(description), sizeof(description)
					 - strlen(description), "%s", roadmap_lang_get("accident"));
			strcpy(report_icon, "reportaccident");
			strcpy(minimized_icon, "minimized_accident");
		} else if (alertType == RT_ALERT_TYPE_POLICE) {
			snprintf(description + strlen(description), sizeof(description)
					 - strlen(description), "%s", roadmap_lang_get("police"));
			strcpy(report_icon, "reportpolice");
			strcpy(minimized_icon, "minimized_police");
		} else if (alertType == RT_ALERT_TYPE_TRAFFIC_JAM) {
			snprintf(description + strlen(description), sizeof(description)
					 - strlen(description), "%s", roadmap_lang_get("traffic jam"));
			strcpy(report_icon, "reporttrafficjam");
			strcpy(minimized_icon, "minimized_trafficjam");
		} else if (alertType == RT_ALERT_TYPE_HAZARD) {
			snprintf(description + strlen(description), sizeof(description)
					 - strlen(description), "%s", roadmap_lang_get("hazard"));
			strcpy(report_icon, "reporthazard");
			strcpy(minimized_icon, "minimized_hazard");
		} else if (alertType == RT_ALERT_TYPE_OTHER) { //TODO: this does not exist
			strcpy(report_icon, "reportother");
			strcpy(minimized_icon, "minimized_other");
		} else if (alertType == RT_ALERT_TYPE_PARKING) {
			snprintf(description + strlen(description), sizeof(description)
					 - strlen(description), "%s", roadmap_lang_get("parking"));
			strcpy(report_icon, "reportparking");
			strcpy(minimized_icon, "minimized_parking");
		}
	} else {
		//TODO: add suggested strings
		snprintf(description + strlen(description), sizeof(description)
				 - strlen(description), "%s", roadmap_lang_get("Chit chat"));
		strcpy(report_icon, "reportincident");
		strcpy(minimized_icon, "minimized_chitchat");
	}
	
	[self getLocation: &sLocationStr[0] direction: alertDirection];
	
	snprintf(description + strlen(description), sizeof(description)
			 - strlen(description), " on %s at %s", sLocationStr, roadmap_time_get_hours_minutes(time(NULL)));
	
	label = [[iphoneLabel alloc] initWithFrame:gRectDescription];
	text = [NSString stringWithUTF8String:sLocationStr];
	[label setText:text];
	[label setNumberOfLines:0];
	[label setLineBreakMode:UILineBreakModeWordWrap];
	[label setAdjustsFontSizeToFitWidth:YES];
	[label setFont:[UIFont systemFontOfSize:16.0f]];
	[label setMinimumFontSize:10.0f];
	[scrollView addSubview:label];
	[label release];
	
	//icon
	if (iconImage)
		[iconImage release];
	iconImage = roadmap_iphoneimage_load(report_icon);
	if (iconImage) {
		imageView = [[UIImageView alloc] initWithImage:iconImage];
		rect = gRectIcon;
		rect.size = imageView.frame.size;
		CGPoint center = CGPointMake (gRectIcon.origin.x + gRectIcon.size.width/2, gRectIcon.origin.y + gRectIcon.size.height/2);
		[imageView setFrame:rect];
		[imageView setCenter:center];
		[scrollView addSubview:imageView];
		[imageView release];
	}
	
	//minimized icon
	if (minimizedImage)
		[minimizedImage release];
	minimizedImage = roadmap_iphoneimage_load(minimized_icon);
	
	//comment editbox
	commentEditbox = [[UITextField alloc] initWithFrame:gRectEditbox];
	[commentEditbox setBorderStyle:UITextBorderStyleRoundedRect];
	[commentEditbox setClearButtonMode:UITextFieldViewModeWhileEditing];
	[commentEditbox setReturnKeyType:UIReturnKeyDone];
	[commentEditbox setDelegate:self];
	[commentEditbox setPlaceholder:[NSString stringWithUTF8String:roadmap_lang_get("<Add description>")]];
	[scrollView addSubview:commentEditbox];
	[commentEditbox release];
	
	//direction button
	buttonDirectionView = [[UIView alloc] initWithFrame:gRectDirection];
	[scrollView addSubview:buttonDirectionView];
	[buttonDirectionView release];
	[self setDirection];
	
	//mood button
	moodButton = [UIButton buttonWithType:UIButtonTypeRoundedRect];
	[moodButton setFrame:gRectMood];
	[moodButton addTarget:self action:@selector(setMood) forControlEvents:UIControlEventTouchUpInside];
   moodButton.titleLabel.lineBreakMode = UILineBreakModeCharacterWrap;
   moodButton.titleLabel.font = [UIFont systemFontOfSize:14.0f];
	[scrollView addSubview:moodButton];
	
	//image button
	imageButton = [UIButton buttonWithType:UIButtonTypeCustom];
	
	image = roadmap_iphoneimage_load("add_image_box");
	if (image) {
		[imageButton setBackgroundImage:image forState:UIControlStateNormal];
		[image release];
	}
	[imageButton setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Add image")] forState:UIControlStateNormal];
	[imageButton setTitleColor:[UIColor blackColor] forState:UIControlStateNormal];
	[imageButton setTitleEdgeInsets:UIEdgeInsetsMake(20, 0, -20, 0)];
	[imageButton setFrame:gRectImage];
	[imageButton addTarget:self action:@selector(setImage) forControlEvents:UIControlEventTouchUpInside];
   imageButton.titleLabel.lineBreakMode = UILineBreakModeWordWrap;
	[scrollView addSubview:imageButton];
	
	//cancel button
	cancelButton = [UIButton buttonWithType:UIButtonTypeCustom];
	[cancelButton setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Cancel")] forState:UIControlStateNormal];
	image = roadmap_iphoneimage_load("button_up");
	if (image) {
		[cancelButton setBackgroundImage:image forState:UIControlStateNormal];
		[image release];
	}
	image = roadmap_iphoneimage_load("button_down");
	if (image) {
		[cancelButton setBackgroundImage:image forState:UIControlStateHighlighted];
		[image release];
	}
	[cancelButton setFrame:gRectCancel];
	[cancelButton addTarget:self action:@selector(onCancel) forControlEvents:UIControlEventTouchUpInside];
	[scrollView addSubview:cancelButton];
	
	//hide button
	hideButton = [UIButton buttonWithType:UIButtonTypeCustom];
	[hideButton setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Hide")] forState:UIControlStateNormal];
	image = roadmap_iphoneimage_load("button_up");
	if (image) {
		[hideButton setBackgroundImage:image forState:UIControlStateNormal];
		[image release];
	}
	image = roadmap_iphoneimage_load("button_down");
	if (image) {
		[hideButton setBackgroundImage:image forState:UIControlStateHighlighted];
		[image release];
	}
	[hideButton setFrame:gRectHide];
	[hideButton addTarget:self action:@selector(onHide) forControlEvents:UIControlEventTouchUpInside];
	[scrollView addSubview:hideButton];
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
	UIScrollView *scrollView = (UIScrollView *) self.view;
	UIImage *image;
	CGRect rect;
	CGRect finalRect;
	char hide_str[20];
   const char* mood_str;

	
	if (!initialized) {
		[self setupView];
		isTakingImage = 0;
		animatedImageView = NULL;
		initialized = TRUE;
		
		g_timeout = HIDE_TIMEOUT;
		roadmap_main_set_periodic (1000, periodic_hide);
		sprintf(hide_str, "%s (%d)", roadmap_lang_get("Hide"), g_timeout);
		[hideButton setTitle:[NSString stringWithUTF8String:hide_str] forState:UIControlStateNormal];
	}
	
	// Set image if was taken
	image = [self loadJpgImage:IMAGE_FILENAME];
	if (image) {
      if (!roadmap_horizontal_screen_orientation())
         finalRect = gRectImage;
      else
         finalRect = gRectImageLs;
      
		if (image.size.width > image.size.height) {
			finalRect.size.height = (gRectImage.size.width / image.size.width) * image.size.height;
			finalRect.origin.y = gRectImage.origin.y + gRectImage.size.height/2 - finalRect.size.height/2;
		} else {
			finalRect.size.width = (gRectImage.size.height / image.size.height) * image.size.width;
			finalRect.origin.x = gRectImage.origin.x + gRectImage.size.width/2 - finalRect.size.width/2;
		}
		[imageButton setFrame:finalRect];
		[imageButton setBackgroundImage:image forState:UIControlStateNormal];
		[image release];
		
		[imageButton setTitle:@"" forState:UIControlStateNormal];
		
		if (isTakingImage) {
			rect = scrollView.bounds;
			if (image.size.width > image.size.height) {
				rect.size.height = (scrollView.bounds.size.width * image.size.height / image.size.width)  ;
				rect.origin.y = scrollView.bounds.origin.y + scrollView.bounds.size.height/2 - rect.size.height/2;
			}
			
			animatedImageView = [[UIImageView alloc] initWithFrame:rect];
			animatedImageView.image = image;
			[scrollView addSubview:animatedImageView];
			[animatedImageView release];
			
			[UIView beginAnimations:NULL context:NULL];
			[UIView setAnimationDuration:0.5f];
			[UIView setAnimationDelay:2.0f];
			[UIView setAnimationCurve:UIViewAnimationCurveEaseInOut];
			animatedImageView.frame = finalRect;
			[UIView commitAnimations];
			isTakingImage = 0;
		} else {
			[imageButton setBackgroundImage:image forState:UIControlStateNormal];
		}
	}	
	
	// Set mood image and text
	[moodButton setTitle:[NSString stringWithUTF8String:roadmap_lang_get(roadmap_mood_get_name())] forState:UIControlStateNormal];
   mood_str = roadmap_path_join("moods", roadmap_mood_get_name());
	image = roadmap_iphoneimage_load(mood_str);
   roadmap_path_free(mood_str);
	if (image) {
		[moodButton setImage:image forState:UIControlStateNormal];
		[moodButton setImageEdgeInsets:UIEdgeInsetsMake(3, 0, 2, 5)];
		[image release];
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
	
	//Clean old image if exists
	const char *path = roadmap_path_first ("config");
	const char *file_name = IMAGE_FILENAME;
	roadmap_file_remove(path, file_name);
	
	
	AlertScrollView *scrollView = [[AlertScrollView alloc] initWithFrame:CGRectZero];
   scrollView.alwaysBounceVertical = YES;
   scrollView.backgroundColor = roadmap_main_table_color();
   self.view = scrollView;
   [scrollView release]; // decrement retain count
   [scrollView setDelegate:self];
   [self setTitle:[NSString stringWithUTF8String:RTAlerts_get_title(alert_type, -1)]];
   UINavigationItem *navItem = [self navigationItem];
   UIBarButtonItem *button = [[UIBarButtonItem alloc] initWithTitle:[NSString stringWithUTF8String:roadmap_lang_get("Send")]
                                                              style:UIBarButtonItemStyleDone target:self action:@selector(sendAlert)];
   
   [navItem setRightBarButtonItem:button];
   [button release];
   
   alertType = alert_type;
   alertDirection = MY_DIRECTION;
   
   roadmap_main_push_view(self);
}


- (void)dealloc
{
	roadmap_main_remove_periodic(periodic_hide);
	
	const char *path = roadmap_path_first ("config");
	const char *file_name = IMAGE_FILENAME;
	roadmap_file_remove(path, file_name);
   
   if (isHidden)
      [collapsedView removeFromSuperview];
	
   [iconImage release];
	
	if (minimizedImage)
		[minimizedImage release];
	
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


