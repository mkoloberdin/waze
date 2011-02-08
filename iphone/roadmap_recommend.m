/* roadmap_recommend.m - Recommend waze
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

#include "roadmap_main.h"
#include "roadmap_iphonemain.h"
#include "roadmap_messagebox.h"
#include "roadmap_lang.h"
#include "Realtime.h"
#include "roadmap_list_menu.h"
#ifdef CHOMP
#include "chomp-connect-iphoneos/ChompDialog.h"
#endif //CHOMP
#include "roadmap_social.h"
#include "roadmap_recommend.h"
#include "ssd/ssd_confirm_dialog.h"
#include "roadmap_res.h"
#include "roadmap_device_events.h"
#include "roadmap_iphonerecommend.h"

#include "roadmap_analytics.h"



extern RoadMapAction          RoadMapStartActions[];

#ifdef CHOMP
extern const char *CHOMP_API_KEY;
#endif



enum Tags {
   ID_TAG_IMAGE = 1,
   ID_TAG_TITLE,
   ID_TAG_TEXT,
   ID_TAG_BUTTON
};

#define RECOMMEND_DLG_TXT_LINE_1          "Can we assume that the feeling's mutual? If you've been enjoyning our app, we'd really appreciate it if you could leave us a nice review in the %s."
#define RECOMMEND_DLG_TXT_LINE_2          "It'll really help us grow :)"
#define RECOMMEND_DLG_TXT_NEW_VER_LINE_1  "If you like this version and wanna help get more users on-board - rate this version, too."
#define RECOMMEND_DLG_TXT_NEW_VER_LINE_2  "Every rating helps!"
#define RECOMMEND_DLG_TXT_FB_SHARE_LINE_1 "It only takes a handful of wazers driving in your area to significantly improve the real-time traffic & road info you need for a better commute."
#define RECOMMEND_DLG_TXT_FB_SHARE_LINE_2 "So tell your friends and help build the waze driving community in your 'hood'!"

#define RECOMMEND_DLG_APP_STORE_NAME     "Appstore"

void roadmap_recommend_email() {
   char str[1024];
   if (![RoadMapEmailView canSendMail]) {
      roadmap_messagebox("Info", "Could not send email");
      return;
   }

   roadmap_analytics_log_event(ANALYTICS_EVENT_RECOMMENDEMAIL, NULL, NULL);
   
   NSString *body_message;
   RoadMapEmailView *emailView = [[RoadMapEmailView alloc] init];
   emailView.mailComposeDelegate = emailView;
   [emailView setSubject:[NSString stringWithUTF8String:roadmap_lang_get("I found a fun app called waze")]];
   snprintf(str, sizeof(str), "<p>%s</p>"
            "<p>%s</p>"
            "<p>%s <a href=\"http://itunes.apple.com/us/app/id323229106?mt=8&uo=6\">%s</a>, <a href=\"http://www.waze.com/download/\">%s</a></p>"
            "<p>%s %s</p>",
            roadmap_lang_get("Hey,"),
            roadmap_lang_get("Wanted to tell you about a free social GPS app called waze. Waze has all kinds of fun social elements that give drivers the ability to actively update one another on traffic, police traps, construction, speed cams and a whole lot more..."),
            roadmap_lang_get("Download for"), roadmap_lang_get ("iPhone"), roadmap_lang_get("Android, Windows Mobile or Symbian"),
            roadmap_lang_get("My waze username is:"), RealTime_GetUserName());
   body_message = [NSString stringWithUTF8String: str];
   [emailView setMessageBody:body_message isHTML:YES];
   
   //roadmap_main_show_root(FALSE);
   roadmap_main_present_modal(emailView);
   [emailView release];
}

void roadmap_recommend_chomp () {
#ifdef CHOMP
   roadmap_analytics_log_event(ANALYTICS_EVENT_RECOMMENDCHOMP, NULL, NULL);
   
   // Open the Chomp Connect dialog
   
   // Use the apps bundle identifier
   NSDictionary *infoDictionary = [[NSBundle mainBundle] infoDictionary];
   NSString *appID = [infoDictionary valueForKey:@"CFBundleIdentifier"];
   
   ChompDialog *cDialog = [[[ChompDialog alloc] initWithApiKey:[NSString stringWithUTF8String:CHOMP_API_KEY] forApp:appID] autorelease];
   
   RoadMapChompDelegate *delegate = [[RoadMapChompDelegate alloc] init];
   cDialog.delegate = delegate;
   //[delegate release];
   
   roadmap_main_show_root(FALSE);
   [cDialog show];
#endif //CHOMP
}

static void recommend_appstore_cb(int exit_code, void *data){
   
   if( dec_yes != exit_code)
      return;
   
   roadmap_analytics_log_event(ANALYTICS_EVENT_RECOMMENDAPPSTORE, NULL, NULL);
   
   roadmap_main_open_url("http://itunes.apple.com/us/app/id323229106?mt=8&uo=6");
}   

void roadmap_recommend_appstore () {
   roadmap_main_show_root(FALSE);
   ssd_confirm_dialog("Exit Application", "Waze will now close and you'll be redirected to the Appstore", TRUE, recommend_appstore_cb, NULL);
}

void roadmap_recommend_rate_us(RoadMapCallback on_close, int type) {
   RoadMapRateUsView *dialog;
   
   dialog = [[RoadMapRateUsView alloc] init];
   [dialog show:on_close type:type];
}

void roadmap_recommend() {
   roadmap_analytics_log_event(ANALYTICS_EVENT_RECOMMEND, NULL, NULL);
   
   roadmap_list_menu_simple ("Spread the word", "recommend", NULL, NULL, 
                             NULL, NULL, NULL,
                             RoadMapStartActions, 0);
}


//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

@implementation RoadMapEmailView

/////////////////////////////
// MFMailComposeViewControllerDelegate delegate


- (void)mailComposeController:(MFMailComposeViewController*)controller didFinishWithResult:(MFMailComposeResult)result error:(NSError*)error
{
   if (result == MFMailComposeResultSaved ||
       result == MFMailComposeResultSent) {
      roadmap_messagebox_timeout("", "Thanks for reviewing Waze", 3);
   } else if (result == MFMailComposeResultFailed) {
      roadmap_messagebox("Error", "Could not send email");
   }
   roadmap_main_dismiss_modal();
   roadmap_main_show_root(FALSE);
}


@end


#ifdef CHOMP
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

@implementation RoadMapChompDelegate


- (void)dialogDidCancel:(ChompDialog*)dialog {
   [self release];
}

- (void)dialogDidSucceed:(ChompDialog*)dialog {
   roadmap_messagebox_timeout("", "Thanks for reviewing waze", 3);
   
   [self release];
}

@end
#endif //CHOMP


//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

@implementation RoadMapRateUsView

- (void) resizeViews
{
   int viewPosY = 0;
   CGRect rect;
   UIImageView *imageView;
   UILabel *label;
   UIButton *button;
   UIScrollView *scrollView = (UIScrollView *)self.view;
   
   //Set image pos
   imageView = (UIImageView *)[scrollView viewWithTag:ID_TAG_IMAGE];
   if (!imageView)
      return;
   rect = imageView.frame;
   rect.origin.x = (scrollView.bounds.size.width - imageView.bounds.size.width) /2;
   rect.origin.y = viewPosY;
   imageView.frame = rect;
   viewPosY += imageView.frame.size.height;
   
   //Set title size/pos
   label = (UILabel *)[scrollView viewWithTag:ID_TAG_TITLE];
   if (!label)
      return;
   [label sizeToFit];
   rect = label.frame;
   if (rect.size.width > 300) {
      rect.size.width = 300;
      label.frame = rect;
      label.autoresizingMask = UIViewAutoresizingFlexibleHeight;
      [label sizeToFit];
      rect = label.frame;
   }
   rect.origin.x = (scrollView.bounds.size.width - label.bounds.size.width)/2;
   rect.origin.y = viewPosY;
   label.frame = rect;
   viewPosY += label.bounds.size.height + 5;
   
   //Set text size/pos
   label = (UILabel *)[scrollView viewWithTag:ID_TAG_TEXT];
   if (!label)
      return;
   rect = label.frame;
   rect.size.width = scrollView.bounds.size.width - 40;
   label.frame = rect;
   [label sizeToFit];
   rect = label.frame;
   rect.origin.x = (scrollView.bounds.size.width - label.bounds.size.width)/2;
   rect.origin.y = viewPosY;
   label.frame = rect;
   viewPosY += label.bounds.size.height + 5;
   
   //Set button pos
   button = (UIButton *)[scrollView  viewWithTag:ID_TAG_BUTTON];
   if (!button)
      return;
   rect.size = button.bounds.size;
   rect.origin.x = (scrollView.bounds.size.width - button.bounds.size.width) / 2;
   rect.origin.y = viewPosY;
	[button setFrame:rect];
   viewPosY += button.bounds.size.height + 10;
   
   [scrollView setContentSize:CGSizeMake(scrollView.frame.size.width, viewPosY)];
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

- (void)viewDidAppear:(BOOL)animated {
   [self resizeViews];
}

- (void) onRate
{
   roadmap_main_pop_view(NO);
   recommend_appstore_cb(dec_yes, NULL);
   if (onCloseCallback)
      onCloseCallback();
}

- (void) onFBShare
{
   roadmap_main_pop_view(NO);
   roadmap_facebook_share();
   if (onCloseCallback)
      onCloseCallback();
}

- (void) onSkip
{
   roadmap_main_pop_view(YES);
   if (onCloseCallback)
      onCloseCallback();
}

- (void)show: (RoadMapCallback)onClose type:(int)type
{
	CGRect rect;
	UIImage *image;
	UIImageView *imageView;
	NSString *text;
	UILabel *label;
	UIButton *button;
	rect = [[UIScreen mainScreen] applicationFrame];
	rect.origin.x = 0;
	rect.origin.y = 0;
	rect.size.height = roadmap_main_get_mainbox_height();
	UIScrollView *scrollView = [[UIScrollView alloc] initWithFrame:rect];
	self.view = scrollView;
	[scrollView release]; // decrement retain count
	[scrollView setDelegate:self];
   
   roadmap_log( ROADMAP_WARNING, "roadmap_recommend_rate_us is shown" );
	
	[self setTitle:[NSString stringWithUTF8String: roadmap_lang_get ("Rate us")]];
   onCloseCallback = onClose;
	
	//hide left button
	UINavigationItem *navItem = [self navigationItem];
	[navItem setHidesBackButton:YES];
   UIBarButtonItem *barButton = [[UIBarButtonItem alloc] initWithTitle:[NSString stringWithUTF8String:roadmap_lang_get("Skip")]
                                                                 style:UIBarButtonItemStyleDone target:self action:@selector(onSkip)];
   [navItem setRightBarButtonItem:barButton];
	[barButton release];
	
	//set UI elements
	
	//top image
   switch (type) {
      case RATE_SCREEN_TYPE_FIRST_TIME:
         image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "rate_us_bg");
         break;
      case RATE_SCREEN_TYPE_NEW_VER:
         image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "rate_us_new_ver_bg");
         break;
      case RATE_SCREEN_TYPE_FB_SHARE:
         image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "rate_us_fb_share_bg");
         break;
      default:
         image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "rate_us_bg");
         break;
   }
	if (image) {
		imageView = [[UIImageView alloc] initWithImage:image];
      imageView.tag = ID_TAG_IMAGE;
		[scrollView addSubview:imageView];
		[imageView release];
	}
	
	//title
   switch (type) {
      case RATE_SCREEN_TYPE_FIRST_TIME:
         text = [NSString stringWithUTF8String:roadmap_lang_get("we love you!")];
         break;
      case RATE_SCREEN_TYPE_NEW_VER:
         text = [NSString stringWithUTF8String:roadmap_lang_get("Appstore ratings start from scratch with every new version.")];
         break;
      case RATE_SCREEN_TYPE_FB_SHARE:
         text = [NSString stringWithUTF8String:roadmap_lang_get("Share with friends & colleagues!")];
         break;
      default:
         text = [NSString stringWithUTF8String:roadmap_lang_get("we love you!")];
         break;
   }
   label = [[UILabel alloc] initWithFrame:CGRectZero];
	[label setText:text];
   [label setNumberOfLines:0];
	[label setTextAlignment:UITextAlignmentCenter];
	[label setFont:[UIFont boldSystemFontOfSize:18]];
   [label setAutoresizingMask:UIViewAutoresizingFlexibleHeight];
   [label setBackgroundColor:[UIColor clearColor]];
   [label setTextColor:[UIColor whiteColor]];
   label.tag = ID_TAG_TITLE;
	[scrollView addSubview:label];
	[label release];

   //Message
   char text_str[512];
   // Format text string
   switch (type) {
      case RATE_SCREEN_TYPE_FIRST_TIME:
         snprintf( text_str, 512, roadmap_lang_get( RECOMMEND_DLG_TXT_LINE_1 ), roadmap_lang_get( RECOMMEND_DLG_APP_STORE_NAME ) );
         snprintf( text_str + strlen( text_str ), 512 - strlen( text_str ), "\n%s", roadmap_lang_get( RECOMMEND_DLG_TXT_LINE_2 ) );
         break;
      case RATE_SCREEN_TYPE_NEW_VER:
         strncpy_safe( text_str, roadmap_lang_get( RECOMMEND_DLG_TXT_NEW_VER_LINE_1 ), 512);
         snprintf( text_str + strlen( text_str ), 512 - strlen( text_str ), "\n%s", roadmap_lang_get( RECOMMEND_DLG_TXT_NEW_VER_LINE_2 ) );
         break;
      case RATE_SCREEN_TYPE_FB_SHARE:
         strncpy_safe( text_str, roadmap_lang_get( RECOMMEND_DLG_TXT_FB_SHARE_LINE_1 ), 512);
         snprintf( text_str + strlen( text_str ), 512 - strlen( text_str ), "\n%s", roadmap_lang_get( RECOMMEND_DLG_TXT_FB_SHARE_LINE_2 ) );
         break;
      default:
         text = [NSString stringWithUTF8String:roadmap_lang_get("we love you!")];
         break;
   }
   
   text = [NSString stringWithUTF8String:text_str];
   label = [[UILabel alloc] initWithFrame:CGRectZero];
	[label setText:text];
	[label setNumberOfLines:0];
   [label setLineBreakMode:UILineBreakModeWordWrap];
	[label setTextAlignment:UITextAlignmentCenter];
	[label setFont:[UIFont systemFontOfSize:18]];
   [label setAutoresizingMask:UIViewAutoresizingFlexibleHeight];
   [label setBackgroundColor:[UIColor clearColor]];
   [label setTextColor:[UIColor whiteColor]];
   label.tag = ID_TAG_TEXT;
	[scrollView addSubview:label];
	[label release];
   
	//Rate button
	button = [UIButton buttonWithType:UIButtonTypeCustom];
   if (type != RATE_SCREEN_TYPE_FB_SHARE) {
      [button addTarget:self action:@selector(onRate) forControlEvents:UIControlEventTouchUpInside];
      [button setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Rate")] forState:UIControlStateNormal];
   } else {
      [button addTarget:self action:@selector(onFBShare) forControlEvents:UIControlEventTouchUpInside];
      [button setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Share on Facebook")] forState:UIControlStateNormal];
   }

	image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "welcome_btn");
	if (image) {
		[button setBackgroundImage:image forState:UIControlStateNormal];
	}
	image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "welcome_btn_h");
	if (image) {
		[button setBackgroundImage:image forState:UIControlStateHighlighted];
	}
   button.titleLabel.adjustsFontSizeToFitWidth = YES;
   button.titleEdgeInsets = UIEdgeInsetsMake(0, 10, 0, 10);
   [button sizeToFit];
   button.tag = ID_TAG_BUTTON;
	[scrollView addSubview:button];

   [self resizeViews];
	
	roadmap_main_push_view(self);
}

@end