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
#include "roadmap_recommend.h"
#include "ssd/ssd_confirm_dialog.h"
#include "roadmap_iphonerecommend.h"

#include "roadmap_analytics.h"



extern RoadMapAction          RoadMapStartActions[];

#ifdef CHOMP
extern const char *CHOMP_API_KEY;
#endif

// Recommend events
static const char* ANALYTICS_EVENT_RECOMMEND_NAME         = "RECOMMEND";
static const char* ANALYTICS_EVENT_RECOMMENDAPPSTORE_NAME = "RECOMMEND_APPSTORE";
static const char* ANALYTICS_EVENT_RECOMMENDCHOMP_NAME    = "RECOMMEND_CHOMP";
static const char* ANALYTICS_EVENT_RECOMMENDEMAIL_NAME    = "RECOMMEND_EMAIL";


void roadmap_recommend_email() {
   char str[1024];
   if (![RoadMapEmailView canSendMail]) {
      roadmap_messagebox("Info", "Could not send email");
      return;
   }

   roadmap_analytics_log_event(ANALYTICS_EVENT_RECOMMENDEMAIL_NAME, NULL, NULL);
   
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
   roadmap_analytics_log_event(ANALYTICS_EVENT_RECOMMENDCHOMP_NAME, NULL, NULL);
   
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
   
   roadmap_analytics_log_event(ANALYTICS_EVENT_RECOMMENDAPPSTORE_NAME, NULL, NULL);
   
   roadmap_main_open_url("http://itunes.apple.com/us/app/id323229106?mt=8&uo=6");
}   

void roadmap_recommend_appstore () {
   roadmap_main_show_root(FALSE);
   ssd_confirm_dialog("Exit Application", "Waze will now close and you'll be redirected to the Appstore", TRUE, recommend_appstore_cb, NULL);
}

void roadmap_recommend() {
   roadmap_analytics_log_event(ANALYTICS_EVENT_RECOMMEND_NAME, NULL, NULL);
   
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
