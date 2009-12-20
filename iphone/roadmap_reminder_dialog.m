/* roadmap_reminder_dialog.m - iPhone dialog for add reminder
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi R
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
 */

#include "roadmap_types.h"
#include "roadmap_plugin.h"
#include "roadmap_main.h"
#include "roadmap_iphonemain.h"
#include "roadmap_lang.h"
#include "roadmap_iphoneimage.h"
#include "roadmap_lang.h"
#include "roadmap_device_events.h"
#include "roadmap_keyboard.h"
#include "roadmap_math.h"
#include "roadmap_messagebox.h"
#include "ssd/ssd_dialog.h"
#include "widgets/iphoneLabel.h"
#include "roadmap_reminder.h"
#include "roadmap_reminder_dialog.h"


#define REMINDER_DLG_TITLE    "New Geo-Reminder"
#define REMINDER_ROW_HEIGHT   25
#define REMINDER_X_MARG       10
#define REMINDER_Y_MARG       10

enum reminder_view_tags {
   VIEW_TAG_BG_FRAME = 1,
   VIEW_TAG_ICON,
   VIEW_TAG_PROPERTIES,
   VIEW_TAG_TITLE_LBL,
   VIEW_TAG_TITLE_EDT,
   VIEW_TAG_DESCRIPTION_LBL,
   VIEW_TAG_DESCRIPTION_EDT,
   VIEW_TAG_DISTANCE_LBL,
   VIEW_TAG_DISTANCE_SLD,
   VIEW_TAG_REPEAT_LBL,
   VIEW_TAG_REPEAT_OFF,
   VIEW_TAG_REPEAT_ON
};


typedef struct reminder_context_st{
   PluginStreetProperties properties;
   RoadMapPosition position;
}reminder_context;

static reminder_context gContext;


extern BOOL roadmap_horizontal_screen_orientation();




void reminder_add_dlg (PluginStreetProperties *properties, RoadMapPosition *position) {
   if (properties)
      gContext.properties = *properties;
   else{
      gContext.properties.address = "";
      gContext.properties.street = "";
      gContext.properties.city = "";
   }
   gContext.position = *position;
   
   RoadMapReminderView *reminderView = [[RoadMapReminderView alloc] init];
   [reminderView showWithProperties: properties andPosition:position];
}




//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

@implementation RoadMapReminderView


- (void) resizeViews
{
   UIScrollView *contentView = (UIScrollView *) self.view;
   UIView *view;
   CGRect bounds = contentView.bounds;
   CGRect rect;
   int yPos = REMINDER_Y_MARG;
   int xPos = REMINDER_X_MARG;
   
   //icon
   view = [contentView viewWithTag:VIEW_TAG_ICON];
   rect = view.frame;
   rect.origin = CGPointMake (xPos, yPos);
   view.frame = rect;
   xPos += view.bounds.size.width;
   
   //properties
   view = [contentView viewWithTag:VIEW_TAG_PROPERTIES];
   if (view) {
      rect = view.frame;
      rect.origin = CGPointMake (10 + xPos, yPos);
      rect.size = CGSizeMake (bounds.size.width - rect.origin.x - REMINDER_X_MARG, REMINDER_ROW_HEIGHT);
      view.frame = rect;
      
      yPos += view.bounds.size.height;
   }
   
   yPos += REMINDER_Y_MARG;
   xPos = REMINDER_X_MARG;
   
   //title
   view = [contentView viewWithTag:VIEW_TAG_TITLE_LBL];
   rect = view.frame;
   rect.origin = CGPointMake (xPos, yPos);
   rect.size = CGSizeMake (bounds.size.width - xPos - REMINDER_X_MARG, REMINDER_ROW_HEIGHT);
   view.frame = rect;
   
   yPos += view.bounds.size.height + REMINDER_Y_MARG/2;
   xPos = REMINDER_X_MARG;
   
   //title field
   view = [contentView viewWithTag:VIEW_TAG_TITLE_EDT];
   rect = view.frame;
   rect.origin = CGPointMake (xPos, yPos);
   rect.size = CGSizeMake (bounds.size.width - xPos - REMINDER_X_MARG, REMINDER_ROW_HEIGHT);
   view.frame = rect;
   
   yPos += view.bounds.size.height + REMINDER_Y_MARG;
   xPos = REMINDER_X_MARG;
   
   if (roadmap_reminder_feature_enabled()) {
      //description
      view = [contentView viewWithTag:VIEW_TAG_DESCRIPTION_LBL];
      rect = view.frame;
      rect.origin = CGPointMake (xPos, yPos);
      rect.size = CGSizeMake (bounds.size.width - xPos - REMINDER_X_MARG, REMINDER_ROW_HEIGHT);
      view.frame = rect;
      
      yPos += view.bounds.size.height + REMINDER_Y_MARG/2;
      xPos = REMINDER_X_MARG;
      
      //description field
      view = [contentView viewWithTag:VIEW_TAG_DESCRIPTION_EDT];
      rect = view.frame;
      rect.origin = CGPointMake (xPos, yPos);
      rect.size = CGSizeMake (bounds.size.width - xPos - REMINDER_X_MARG, REMINDER_ROW_HEIGHT);
      view.frame = rect;
      
      yPos += view.bounds.size.height + REMINDER_Y_MARG;
      xPos = REMINDER_X_MARG;
      
      //distance
      view = [contentView viewWithTag:VIEW_TAG_DISTANCE_LBL];
      rect = view.frame;
      rect.origin = CGPointMake (xPos, yPos);
      rect.size = CGSizeMake (bounds.size.width - xPos - REMINDER_X_MARG, REMINDER_ROW_HEIGHT);
      view.frame = rect;
      
      yPos += view.bounds.size.height + REMINDER_Y_MARG/2;
      xPos = REMINDER_X_MARG;
      
      //distance slider
      view = [contentView viewWithTag:VIEW_TAG_DISTANCE_SLD];
      rect = view.frame;
      rect.origin = CGPointMake (xPos + 10, yPos);
      rect.size = CGSizeMake (bounds.size.width - xPos - REMINDER_X_MARG - 10*2, REMINDER_ROW_HEIGHT);
      view.frame = rect;
      
      yPos += view.bounds.size.height + REMINDER_Y_MARG;
      xPos = REMINDER_X_MARG;
      
      //repeat
      view = [contentView viewWithTag:VIEW_TAG_REPEAT_LBL];
      rect = view.frame;
      rect.origin = CGPointMake (xPos, yPos);
      rect.size = CGSizeMake (bounds.size.width - xPos - REMINDER_X_MARG, REMINDER_ROW_HEIGHT);
      view.frame = rect;
      
      yPos += view.bounds.size.height + REMINDER_Y_MARG/2;
      xPos = REMINDER_X_MARG;
      
      //repeat button
      view = [contentView viewWithTag:VIEW_TAG_REPEAT_OFF];
      if (!view)
         view = [contentView viewWithTag:VIEW_TAG_REPEAT_ON];
      if (view) {
         rect = view.frame;
         rect.origin = CGPointMake ((bounds.size.width - view.bounds.size.width)/2, yPos);
         view.frame = rect;
      }
      
      yPos += view.bounds.size.height + REMINDER_Y_MARG;
      xPos = REMINDER_X_MARG;
   }
   
   
   // frame background
   view = [contentView viewWithTag:VIEW_TAG_BG_FRAME];
   rect.origin = CGPointMake (5, 5);
   rect.size = CGSizeMake (bounds.size.width - 10, yPos);
   view.frame = rect;
   
   
   contentView.contentSize = CGSizeMake (bounds.size.width, yPos + 10);
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
   return roadmap_main_should_rotate (interfaceOrientation);
}

- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation duration:(NSTimeInterval)duration {
   [self resizeViews];
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
   roadmap_device_event_notification( device_event_window_orientation_changed);
}

- (void)viewWillAppear:(BOOL)animated {
   [self resizeViews];
}

- (void) onSave
{
   const char *argv[reminder_hi__count];
   char  temp[15];
   UITextField *textField;
   UISlider *slider;
   UIView *view;
   
   static const char *repeat_values[2] = {"0", "1"};
   
   const char *title_txt = "";
   const char *description = "";
   int distance = 100;
   const char *repeat = repeat_values[0];
   
   //title
   textField = (UITextField *) [self.view viewWithTag:VIEW_TAG_TITLE_EDT];
   if (textField && textField.text)
      title_txt = [textField.text UTF8String];
   
   if (0) { //TODO: add reminder switch
      //description
      textField = (UITextField *) [self.view viewWithTag:VIEW_TAG_DESCRIPTION_EDT];
      if (textField && textField.text)
         description = [textField.text UTF8String];
      
      //distance
      slider = (UISlider *) [self.view viewWithTag:VIEW_TAG_DISTANCE_SLD];
      if (slider)
         distance = ceil(slider.value);
      
      //repeat
      view = [self.view viewWithTag:VIEW_TAG_REPEAT_ON];
      if (view)
         repeat = repeat_values[1];
   }
   
   
   argv[reminder_hi_house_number] = strdup(gContext.properties.address);
   argv[reminder_hi_street] = strdup(gContext.properties.street);
   argv[reminder_hi_city] = strdup(gContext.properties.city);
   argv[reminder_state] = ""; //state
   sprintf(temp, "%d", gContext.position.latitude);
   argv[reminder_hi_latitude] = strdup(temp);
   sprintf(temp, "%d", gContext.position.longitude);
   argv[reminder_hi_longtitude] = strdup(temp);
   argv[reminder_hi_title] = strdup(title_txt);
   if (0) { //TODO: add reminder switch
      argv[reminder_hi_add_reminder] = "1";
      sprintf(temp, "%d", distance);
      argv[reminder_hi_distance] = strdup(temp);
      argv[reminder_hi_description] = strdup(description);
      argv[reminder_hi_repeat] = strdup(repeat);
   } else {
      argv[reminder_hi_add_reminder] = "0";
      argv[reminder_hi_distance] = strdup("");
      argv[reminder_hi_description] = strdup("");
      argv[reminder_hi_repeat] = strdup("");
   }
   
   roadmap_reminder_add_entry (argv, FALSE);//TODO: add reminder switch
   
   free((void *)argv[reminder_hi_house_number]);
   free((void *)argv[reminder_hi_street]);
   free((void *)argv[reminder_hi_city]);
   free((void *)argv[reminder_hi_latitude]);
   free((void *)argv[reminder_hi_longtitude]);
   free((void *)argv[reminder_hi_distance]);
   free((void *)argv[reminder_hi_description]);
   free((void *)argv[reminder_hi_repeat]);
   
   
   ssd_dialog_hide_all(dec_close);
   roadmap_main_show_root(NO);
   //roadmap_messagebox_timeout("", "Reminder saved", 1);
}

- (void) setRepeat: (UIView *) buttonRepeatView
{
	UIButton *newButtonDirection = [UIButton buttonWithType:UIButtonTypeRoundedRect];
	[newButtonDirection setFrame:[buttonRepeatView bounds]];
	[newButtonDirection addTarget:self action:@selector(onRepeat:) forControlEvents:UIControlEventTouchUpInside];
   newButtonDirection.titleLabel.lineBreakMode = UILineBreakModeCharacterWrap;
   newButtonDirection.titleLabel.font = [UIFont boldSystemFontOfSize:16.0f];
	
	
	if (buttonRepeatView.tag == VIEW_TAG_REPEAT_OFF) {
		[newButtonDirection setTitle:[NSString stringWithUTF8String:roadmap_lang_get ("Once")] forState:UIControlStateNormal];
	} else {
		[newButtonDirection setTitle:[NSString stringWithUTF8String:roadmap_lang_get ("Every time")] forState:UIControlStateNormal];
	}
	
	if ([[buttonRepeatView subviews] count] > 0) {
		[UIView beginAnimations:NULL context:NULL];
		[UIView setAnimationDuration:0.5f];
		[UIView setAnimationCurve:UIViewAnimationCurveEaseInOut];
		
		if (buttonRepeatView.tag == VIEW_TAG_REPEAT_OFF)
			[UIView setAnimationTransition: UIViewAnimationTransitionFlipFromRight forView:buttonRepeatView cache:YES];
		else
			[UIView setAnimationTransition: UIViewAnimationTransitionFlipFromLeft forView:buttonRepeatView cache:YES];
		
		[[[buttonRepeatView subviews] objectAtIndex:0] removeFromSuperview];
		
		[buttonRepeatView addSubview:newButtonDirection];
		
		[UIView commitAnimations];
	} else
		[buttonRepeatView addSubview:newButtonDirection];
}

- (void) onRepeat: (id) obj {
   UIView *buttonRepeatView = [(UIButton *)obj superview];
   
   if (buttonRepeatView.tag == VIEW_TAG_REPEAT_OFF)
      buttonRepeatView.tag = VIEW_TAG_REPEAT_ON;
   else
      buttonRepeatView.tag = VIEW_TAG_REPEAT_OFF;

	[self setRepeat: buttonRepeatView];
   
}

- (void) onDistanceChange
{
   UIScrollView *contentView = (UIScrollView *) self.view;
   UILabel *distanceLabel = (UILabel *) [contentView viewWithTag:VIEW_TAG_DISTANCE_LBL];
   UISlider *distanceSliser = (UISlider *) [contentView viewWithTag:VIEW_TAG_DISTANCE_SLD];
   NSString *string;
   int rem_distance;
   char msg[250];
   
   
   rem_distance = roadmap_math_to_trip_distance_tenths(distanceSliser.value);
   sprintf(msg, "%s: %.1f %s",roadmap_lang_get("Distance to alert"), rem_distance/10.0, roadmap_lang_get(roadmap_math_trip_unit()) );
   
   string = [NSString stringWithUTF8String:msg];
   
   distanceLabel.text = string;
}

- (void) createViewWithProperties: (PluginStreetProperties *) properties andPosition: (RoadMapPosition *) position
{
   UIImage *image;
   UIImageView *imageView;
   iphoneLabel *label;
   UITextField *textField;
   UISlider *slider;
   UIView *view;
   char msg[256];
   
	UIScrollView *containerView = [[UIScrollView alloc] initWithFrame:CGRectZero];
   containerView.alwaysBounceVertical = YES;
	self.view = containerView;
	[containerView release]; // decrement retain count
   
   
   // frame image
   image = roadmap_iphoneimage_load("comments_alert");
	if (image) {
		UIImage *strechedImage = [image stretchableImageWithLeftCapWidth:20 topCapHeight:20];
		imageView = [[UIImageView alloc] initWithImage:strechedImage];
		[image release];
      imageView.tag = VIEW_TAG_BG_FRAME;
		[containerView addSubview:imageView];
		[imageView release];
	}
   
   // icon
   image = roadmap_iphoneimage_load("reminder");
   if (image) {
      imageView = [[UIImageView alloc] initWithImage:image];
      [image release];
      imageView.tag = VIEW_TAG_ICON;
      [containerView addSubview:imageView];
		[imageView release];
   }
   
   // properties
   if (properties){
      msg[0] = 0;
      snprintf(msg, sizeof(msg), "%s, %s", properties->street, properties->city);
      label = [[iphoneLabel alloc] initWithFrame:CGRectZero];
      label.text = [NSString stringWithUTF8String:msg];
      label.tag = VIEW_TAG_PROPERTIES;
      [containerView addSubview:label];
      [label release];
   }
   
   // title
   label = [[iphoneLabel alloc] initWithFrame:CGRectZero];
   label.text = [NSString stringWithUTF8String:roadmap_lang_get("Name")];
   label.tag = VIEW_TAG_TITLE_LBL;
   [containerView addSubview:label];
   [label release];
   textField = [[UITextField alloc] initWithFrame:CGRectZero];
   textField.text = @"";
   textField.borderStyle = UITextBorderStyleRoundedRect;
   textField.clearButtonMode = UITextFieldViewModeWhileEditing;
   textField.delegate = self;
   textField.tag = VIEW_TAG_TITLE_EDT;
   [containerView addSubview:textField];
   [textField release];
   
   if (roadmap_reminder_feature_enabled()) {
      // description
      label = [[iphoneLabel alloc] initWithFrame:CGRectZero];
      label.text = [NSString stringWithUTF8String:roadmap_lang_get("Description")];
      label.tag = VIEW_TAG_DESCRIPTION_LBL;
      [containerView addSubview:label];
      [label release];
      textField = [[UITextField alloc] initWithFrame:CGRectZero];
      textField.text = @"";
      textField.borderStyle = UITextBorderStyleRoundedRect;
      textField.clearButtonMode = UITextFieldViewModeWhileEditing;
      textField.delegate = self;
      textField.tag = VIEW_TAG_DESCRIPTION_EDT;
      [containerView addSubview:textField];
      [textField release];
      
      //distance
      label = [[iphoneLabel alloc] initWithFrame:CGRectZero];
      label.tag = VIEW_TAG_DISTANCE_LBL;
      [containerView addSubview:label];
      [label release];
      slider = [[UISlider alloc] initWithFrame:CGRectZero];
      if (roadmap_math_is_metric()) {
         slider.minimumValue = 100;
         slider.maximumValue = 20000;
      } else {
         slider.minimumValue = 161;
         slider.maximumValue = 16093;
      }
      [slider addTarget:self action:@selector(onDistanceChange) forControlEvents:UIControlEventValueChanged];
      slider.tag = VIEW_TAG_DISTANCE_SLD;
      [containerView addSubview:slider];
      [slider release];
      [self onDistanceChange];
      
      //repeat
      label = [[iphoneLabel alloc] initWithFrame:CGRectZero];
      label.text = [NSString stringWithUTF8String:roadmap_lang_get("Repeat reminder")];
      label.tag = VIEW_TAG_REPEAT_LBL;
      [containerView addSubview:label];
      [label release];
      view = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 100, REMINDER_ROW_HEIGHT)];
      view.tag = VIEW_TAG_REPEAT_OFF;
      [containerView addSubview:view];
      [view release];
      [self setRepeat:view];
   }
}

- (void) showWithProperties: (PluginStreetProperties *) properties andPosition: (RoadMapPosition *) position
{
   kbIsOn = FALSE;
   
   [self createViewWithProperties:properties andPosition:position];
   
   // Title
   if (0) //TODO: add reminder option
      self.title = [NSString stringWithUTF8String:roadmap_lang_get(REMINDER_DLG_TITLE)];
   else
      self.title = [NSString stringWithUTF8String:roadmap_lang_get("Save location")];
   
   // Save button
   UINavigationItem *navItem = [self navigationItem];
	UIBarButtonItem *button = [[UIBarButtonItem alloc] initWithTitle:[NSString stringWithUTF8String:roadmap_lang_get("Save")]
                                                              style:UIBarButtonItemStyleDone target:self action:@selector(onSave)];
	[navItem setRightBarButtonItem:button];
	[button release];
   

   roadmap_main_push_view(self);
}

- (void)dealloc
{
 
   
	[super dealloc];
}




//////////////////////////////////////////////
//Text field delegate

- (void)textFieldDidBeginEditing:(UITextField *)textField {
   UIScrollView *view = (UIScrollView *) self.view;
   CGRect rect = view.frame;
   
   if (!kbIsOn) {
      if (!roadmap_horizontal_screen_orientation())
         rect.size.height -= 215;
      else
         rect.size.height -= 162;
      
      view.frame = rect;
      kbIsOn = TRUE;
   }
   
   
   [view scrollRectToVisible:textField.frame animated:YES];
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
   [textField resignFirstResponder];
   
   UIScrollView *view = (UIScrollView *) self.view;
   CGRect rect = view.frame;
   
   if (kbIsOn) {
      if (!roadmap_horizontal_screen_orientation())
         rect.size.height += 215;
      else
         rect.size.height += 162;
      
      view.frame = rect;
      kbIsOn = FALSE;
   }
      
   [view scrollRectToVisible:textField.frame animated:YES];
   
	return NO;
}

- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string
{
   if (roadmap_keyboard_typing_locked(TRUE)) {
      [textField resignFirstResponder];
      kbIsOn = FALSE;
      return NO;
   } else
      return YES;
}


@end

