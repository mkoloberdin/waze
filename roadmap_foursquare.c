/* roadmap_foursquare.c - Manages Foursquare account
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
 *
 */

#include <stdlib.h>
#include <string.h>

#include "roadmap_main.h"
#include "roadmap_config.h"
#include "roadmap_foursquare.h"
#include "roadmap_dialog.h"
#include "roadmap_messagebox.h"
#include "roadmap_trip.h"
#include "roadmap_screen.h"
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_choice.h"
#include "ssd/ssd_button.h"
#include "ssd/ssd_entry.h"
#include "ssd/ssd_entry_label.h"
#include "ssd/ssd_checkbox.h"
#include "ssd/ssd_separator.h"
#include "ssd/ssd_bitmap.h"
#include "ssd/ssd_list.h"
#include "ssd/ssd_generic_list_dialog.h"
#include "ssd/ssd_progress_msg_dialog.h"

#ifdef IPHONE
#include "roadmap_list_menu.h"
#endif //IPHONE

#include "Realtime/Realtime.h"
#include "Realtime/RealtimeDefs.h"

static const char *yesno[2];
static BOOL       gsCheckInOnLogin = FALSE;

typedef enum {
   ROADMAP_FOURSQUARE_NONE,
   ROADMAP_FOURSQUARE_LOGIN,
   ROADMAP_FOURSQUARE_SEARCH,
   ROADMAP_FOURSQUARE_CHECKIN
} RoadmapFoursquareRequestType;

static RoadmapFoursquareRequestType gsRequestType = ROADMAP_FOURSQUARE_NONE;

#define     ROADMAP_FOURSQUARE_MAX_VENUE_COUNT        50
#define     FOURSQUARE_VENUES_LIST_NAME               "VenuesList"
#define     FOURSQUARE_REQUEST_TIMEOUT                15000


static FoursquareVenue        gsVenuesList[ROADMAP_FOURSQUARE_MAX_VENUE_COUNT];
static int                    gsVenuesCount = 0;



static FoursquareCheckin      gsCheckInInfo;

//   User name
RoadMapConfigDescriptor FOURSQUARE_CFG_PRM_NAME_Var = ROADMAP_CONFIG_ITEM(
      FOURSQUARE_CONFIG_TAB,
      FOURSQUARE_CFG_PRM_NAME_Name);

//   Password
RoadMapConfigDescriptor FOURSQUARE_CFG_PRM_PASSWORD_Var = ROADMAP_CONFIG_ITEM(
      FOURSQUARE_CONFIG_TAB,
      FOURSQUARE_CFG_PRM_PASSWORD_Name);

//   Enable / Disable Tweeting Foursquare login
RoadMapConfigDescriptor FOURSQUARE_CFG_PRM_TWEET_LOGIN_Var = ROADMAP_CONFIG_ITEM(
      FOURSQUARE_CONFIG_TAB,
      FOURSQUARE_CFG_PRM_TWEET_LOGIN_Name);

//   Enable / Disable Tweeting Foursquare badge unlock
RoadMapConfigDescriptor FOURSQUARE_CFG_PRM_TWEET_BADGE_Var = ROADMAP_CONFIG_ITEM(
      FOURSQUARE_CONFIG_TAB,
      FOURSQUARE_CFG_PRM_TWEET_BADGE_Name);

//   Feature activated
RoadMapConfigDescriptor FOURSQUARE_CFG_PRM_ACTIVATED_Var = ROADMAP_CONFIG_ITEM(
      FOURSQUARE_CONFIG_TAB,
      FOURSQUARE_CFG_PRM_ACTIVATED_Name);

//    Logged in status
RoadMapConfigDescriptor FOURSQUARE_CFG_PRM_LOGGED_IN_Var = ROADMAP_CONFIG_ITEM(
      FOURSQUARE_CONFIG_TAB,
      FOURSQUARE_CFG_PRM_LOGGED_IN_Name);

/////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_foursquare_initialize(void) {

   // Name
   roadmap_config_declare(FOURSQUARE_CONFIG_TYPE, &FOURSQUARE_CFG_PRM_NAME_Var,
         FOURSQUARE_CFG_PRM_NAME_Default, NULL);

   // Password
   roadmap_config_declare_password(FOURSQUARE_CONFIG_TYPE,
         &FOURSQUARE_CFG_PRM_PASSWORD_Var, FOURSQUARE_CFG_PRM_PASSWORD_Default);

   // Tweet login
   roadmap_config_declare_enumeration(FOURSQUARE_CONFIG_TYPE,
         &FOURSQUARE_CFG_PRM_TWEET_LOGIN_Var, NULL,
         FOURSQUARE_CFG_PRM_TWEET_LOGIN_Enabled, FOURSQUARE_CFG_PRM_TWEET_LOGIN_Disabled, NULL);

   // Tweet badge unlock
   roadmap_config_declare_enumeration(FOURSQUARE_CONFIG_TYPE,
         &FOURSQUARE_CFG_PRM_TWEET_BADGE_Var, NULL,
         FOURSQUARE_CFG_PRM_TWEET_BADGE_Enabled, FOURSQUARE_CFG_PRM_TWEET_BADGE_Disabled, NULL);

   // Feature activated
   roadmap_config_declare_enumeration(FOURSQUARE_CONFIG_PREF_TYPE,
            &FOURSQUARE_CFG_PRM_ACTIVATED_Var, NULL,
            FOURSQUARE_CFG_PRM_ACTIVATED_No, FOURSQUARE_CFG_PRM_ACTIVATED_Yes, NULL);

   // Logged in status
   roadmap_config_declare_enumeration(FOURSQUARE_CONFIG_TYPE,
            &FOURSQUARE_CFG_PRM_LOGGED_IN_Var, NULL,
            FOURSQUARE_CFG_PRM_LOGGED_IN_No, FOURSQUARE_CFG_PRM_LOGGED_IN_Yes, NULL);

   yesno[0] = "Yes";
   yesno[1] = "No";

   return TRUE;

}

static void request_time_out (void){
   roadmap_main_remove_periodic(request_time_out);
   ssd_progress_msg_dialog_hide();
   roadmap_messagebox("Oops", "Could not connect with Foursquare. Try again later.");
}

static void delayed_show_progress (void){
   roadmap_main_remove_periodic(delayed_show_progress);
   ssd_progress_msg_dialog_show( roadmap_lang_get( "Connecting Foursquare . . ." ) );
}



void roadmap_foursquare_set_logged_in (BOOL is_logged_in) {
   if (is_logged_in)
      roadmap_config_set(&FOURSQUARE_CFG_PRM_LOGGED_IN_Var, FOURSQUARE_CFG_PRM_LOGGED_IN_Yes);
   else
      roadmap_config_set(&FOURSQUARE_CFG_PRM_LOGGED_IN_Var, FOURSQUARE_CFG_PRM_LOGGED_IN_No);
   roadmap_config_save(TRUE);
}

/////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_foursquare_logged_in(void) {
   if (0 == strcmp(roadmap_config_get(&FOURSQUARE_CFG_PRM_LOGGED_IN_Var),
         FOURSQUARE_CFG_PRM_LOGGED_IN_Yes))
      return TRUE;
   return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////
static void roadmap_foursquare_login_failed(int status) {
   if (status == 701)
      roadmap_messagebox("Oops", "Updating your foursquare account details failed. Please ensure you entered the correct user name and password");
   else
      roadmap_messagebox("Oops", "Could not connect with Foursquare. Try again later.");
   roadmap_foursquare_set_logged_in (FALSE);
}

/////////////////////////////////////////////////////////////////////////////////////
const char *roadmap_foursquare_get_username(void) {
   return roadmap_config_get(&FOURSQUARE_CFG_PRM_NAME_Var);
}

/////////////////////////////////////////////////////////////////////////////////////
const char *roadmap_foursquare_get_password(void) {
   return roadmap_config_get(&FOURSQUARE_CFG_PRM_PASSWORD_Var);
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_foursquare_set_username(const char *user_name) {
   roadmap_config_set(&FOURSQUARE_CFG_PRM_NAME_Var, user_name); //AR: we save only username
   roadmap_config_save(TRUE);
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_foursquare_set_password(const char *password) {
   roadmap_config_set(&FOURSQUARE_CFG_PRM_PASSWORD_Var, FOURSQUARE_CFG_PRM_PASSWORD_Default/*password*/); //AR: we don't want to save login details
   roadmap_config_save(TRUE);
}

/////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_foursquare_is_enabled(void) {
   if (0 == strcmp(roadmap_config_get(&FOURSQUARE_CFG_PRM_ACTIVATED_Var),
         FOURSQUARE_CFG_PRM_ACTIVATED_Yes))
      return TRUE;
   return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_foursquare_is_tweet_login_enabled(void) {
   if (0 == strcmp(roadmap_config_get(&FOURSQUARE_CFG_PRM_TWEET_LOGIN_Var),
         FOURSQUARE_CFG_PRM_TWEET_LOGIN_Enabled))
      return TRUE;
   return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_foursquare_enable_tweet_login() {
   roadmap_config_set(&FOURSQUARE_CFG_PRM_TWEET_LOGIN_Var,
         FOURSQUARE_CFG_PRM_TWEET_LOGIN_Enabled);
   roadmap_config_save(TRUE);
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_foursquare_disable_tweet_login() {
   roadmap_config_set(&FOURSQUARE_CFG_PRM_TWEET_LOGIN_Var,
         FOURSQUARE_CFG_PRM_TWEET_LOGIN_Disabled);
   roadmap_config_save(TRUE);
}

/////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_foursquare_is_tweet_badge_enabled(void) {
   if (0 == strcmp(roadmap_config_get(&FOURSQUARE_CFG_PRM_TWEET_BADGE_Var),
            FOURSQUARE_CFG_PRM_TWEET_BADGE_Enabled))
         return TRUE;
      return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_foursquare_enable_tweet_badge() {
   roadmap_config_set(&FOURSQUARE_CFG_PRM_TWEET_BADGE_Var,
         FOURSQUARE_CFG_PRM_TWEET_BADGE_Enabled);
   roadmap_config_save(TRUE);
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_foursquare_disable_tweet_badge() {
   roadmap_config_set(&FOURSQUARE_CFG_PRM_TWEET_BADGE_Var,
         FOURSQUARE_CFG_PRM_TWEET_BADGE_Disabled);
   roadmap_config_save(TRUE);
}

/////////////////////////////////////////////////////////////////////////////////////
static void foursquare_un_empty(void){
   roadmap_main_remove_periodic (foursquare_un_empty);
   roadmap_messagebox("Error", "Foursquare user name is empty. You are not logged in");
}

static void foursquare_pw_empty(void){
   roadmap_main_remove_periodic (foursquare_pw_empty);
   roadmap_messagebox("Error", "Foursquare password is empty. You are not logged in");
}

/////////////////////////////////////////////////////////////////////////////////////
static void foursquare_network_error(void){
   roadmap_main_remove_periodic (foursquare_network_error);
   roadmap_messagebox("Oops", roadmap_lang_get("There is no network connection. Updating your Foursquare account details failed."));
   roadmap_foursquare_set_logged_in (FALSE);
   //roadmap_foursquare_login_dialog();
}

/////////////////////////////////////////////////////////////////////////////////////
static int on_checkin_ok_btn(SsdWidget widget, const char *new_value) {
   ssd_dialog_hide_all(dec_close);
   return 1;
}

void roadmap_foursquare_login (const char *user_name, const char *password) {
   BOOL success;

   gsRequestType = ROADMAP_FOURSQUARE_LOGIN;
#ifdef IPHONE
   delayed_show_progress();
#else
   roadmap_main_set_periodic(100, delayed_show_progress);
#endif //IPHONE
   roadmap_main_set_periodic(FOURSQUARE_REQUEST_TIMEOUT, request_time_out);

   success = Realtime_FoursquareConnect(user_name, password,
         roadmap_foursquare_is_tweet_login_enabled() && roadmap_twitter_logged_in());
   if (!success) {
      roadmap_main_remove_periodic(request_time_out);
      ssd_progress_msg_dialog_hide();
      gsRequestType = ROADMAP_FOURSQUARE_NONE;
      roadmap_main_set_periodic (100, foursquare_network_error);
   }
}

/////////////////////////////////////////////////////////////////////////////////////
static int on_login_ok() {
   BOOL logged_in;

   const char * user_name = ssd_dialog_get_value("FoursquareUserName");
   const char * password = ssd_dialog_get_value("FoursquarePassword");

   logged_in = roadmap_foursquare_logged_in();

   if (!strcasecmp((const char*) ssd_dialog_get_data("FoursquareSendLogin"),
        yesno[0]))
      roadmap_foursquare_enable_tweet_login();
   else
      roadmap_foursquare_disable_tweet_login();

   if (!strcasecmp((const char*) ssd_dialog_get_data("FoursquareSendBadgeUnlock"),
        yesno[0]))
      roadmap_foursquare_enable_tweet_badge();
   else
      roadmap_foursquare_disable_tweet_badge();


   if (user_name[0] != 0 && password[0] != 0) {
      roadmap_foursquare_set_username(user_name);
      roadmap_foursquare_set_password(password);
      roadmap_foursquare_login (user_name, password);
      return 1;
   } else if (!logged_in){
      if (user_name[0] == 0) {
         roadmap_main_set_periodic (100, foursquare_un_empty);
         return 1;
      }

      if (password[0] == 0) {
         roadmap_main_set_periodic (100, foursquare_pw_empty);
         return 1;
      }
   }

   return 0;
}


/////////////////////////////////////////////////////////////////////////////////////
static void on_login_dlg_close(int exit_code, void* context) {
   if (exit_code == dec_ok)
      on_login_ok();
}


#ifndef TOUCH_SCREEN
/////////////////////////////////////////////////////////////////////////////////////
static int on_ok_softkey(SsdWidget widget, const char *new_value, void *context) {
   ssd_dialog_hide_current(dec_ok);
   return 0;
}
#endif
/////////////////////////////////////////////////////////////////////
static SsdWidget space(int height) {
   SsdWidget space;
   space = ssd_container_new("spacer", NULL, SSD_MAX_SIZE, height,
         SSD_WIDGET_SPACE | SSD_END_ROW);
   ssd_widget_set_color(space, NULL, NULL);
   return space;
}


/////////////////////////////////////////////////////////////////////////////////////
static void create_checkedin_dialog(void) {

   SsdWidget dialog;
   SsdWidget group;
   SsdWidget box;
   SsdWidget bitmap;
   int width;
   char text[256];

   width = roadmap_canvas_width()/2;

   dialog = ssd_dialog_new(FOURSQUARE_CHECKEDIN_DIALOG_NAME,
         roadmap_lang_get(FOURSQUARE_CHECKEDIN_TITLE), NULL, SSD_CONTAINER_TITLE);

#ifdef TOUCH_SCREEN
   ssd_widget_add (dialog, space(5));
#endif

   box = ssd_container_new("Checkin_result", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
         SSD_WIDGET_SPACE | SSD_END_ROW | SSD_ROUNDED_CORNERS
               | SSD_ROUNDED_WHITE | SSD_POINTER_NONE | SSD_CONTAINER_BORDER);

   //Logo header
   group = ssd_container_new("Foursquare logo", NULL, SSD_MAX_SIZE,SSD_MIN_SIZE,
         SSD_WIDGET_SPACE | SSD_END_ROW);
   ssd_widget_set_color(group, "#000000", "#ffffff");
   bitmap = ssd_bitmap_new ("foursquare_icon", "foursquare_logo",SSD_WS_TABSTOP);
   ssd_widget_add(group, bitmap);
   ssd_widget_add(group, space(5));
   ssd_widget_add (group, ssd_separator_new ("separator", SSD_ALIGN_BOTTOM));
   ssd_widget_add(group, space(5));
   ssd_widget_add(box, group);
   //Checkin message
   group = ssd_container_new("Foursquare message group", NULL, SSD_MAX_SIZE,SSD_MIN_SIZE,
         SSD_WIDGET_SPACE | SSD_END_ROW);
   ssd_widget_set_color(group, "#000000", "#ffffff");
   ssd_widget_add(group, ssd_text_new("Checkin message lablel", gsCheckInInfo.sCheckinMessage,
            16, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER));
   ssd_widget_add(box, group);
   //Address
   group = ssd_container_new("Foursquare address group", NULL, SSD_MAX_SIZE,SSD_MIN_SIZE,
         SSD_WIDGET_SPACE | SSD_END_ROW);
   ssd_widget_set_color(group, "#000000", "#ffffff");
   ssd_widget_add(group, ssd_text_new("Checkin address", gsCheckInInfo.sAddress,
            16, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER));
   ssd_widget_add(box, group);
   //Points
   group = ssd_container_new("Foursquare points group", NULL, SSD_MAX_SIZE,SSD_MIN_SIZE,
         SSD_WIDGET_SPACE | SSD_END_ROW);
   ssd_widget_set_color(group, "#000000", "#ffffff");
   snprintf(text, sizeof(text), "%s %s", roadmap_lang_get("Points:"), gsCheckInInfo.sScorePoints);
   ssd_widget_add(group, ssd_text_new("Checkin points", text,
            16, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER));
   ssd_widget_add(group, space(5));
   ssd_widget_add (group, ssd_separator_new ("separator", SSD_ALIGN_BOTTOM));
   ssd_widget_add(group, space(5));
   ssd_widget_add(box, group);

   //More details
   group = ssd_container_new("Foursquare more details group", NULL, SSD_MAX_SIZE,SSD_MIN_SIZE,
         SSD_WIDGET_SPACE | SSD_END_ROW);
   ssd_widget_set_color(group, "#000000", "#ffffff");
   ssd_widget_add(group, ssd_text_new("more details label", roadmap_lang_get("Full details on this check-in are available for you on foursquare.com."),
            16, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER));
   ssd_widget_add(group, space(5));
   ssd_widget_add(box, group);


   //OK button
   ssd_widget_add (box,
         ssd_button_label ("confirm", roadmap_lang_get ("Ok"),
                     SSD_ALIGN_CENTER|SSD_START_NEW_ROW|SSD_WS_DEFWIDGET|
                     SSD_WS_TABSTOP,
                     on_checkin_ok_btn));

   ssd_widget_add (dialog, box);


#ifndef TOUCH_SCREEN
   ssd_widget_set_left_softkey_text ( dialog, roadmap_lang_get("Ok"));
   ssd_widget_set_left_softkey_callback ( dialog, on_ok_softkey);
#endif
}

/////////////////////////////////////////////////////////////////////////////////////
static void create_login_dialog(void) {

   SsdWidget dialog;
   SsdWidget group, entry_label;
   SsdWidget box, box2;
   SsdWidget bitmap;
   int width;
   int tab_flag = SSD_WS_TABSTOP;
   int row_height = ssd_container_get_row_height();
   int total_width = ssd_container_get_width();
   const char * notesColor = "#383838";
   SsdWidget text;

   width = total_width/2;

   dialog = ssd_dialog_new(FOURSQUARE_LOGIN_DIALOG_NAME,
         roadmap_lang_get(FOURSQUARE_TITLE), on_login_dlg_close, SSD_CONTAINER_TITLE);

#ifdef TOUCH_SCREEN
   ssd_widget_add (dialog, space(5));
#endif

   box = ssd_container_new("UN/PW group", NULL, total_width, SSD_MIN_SIZE,
         SSD_WIDGET_SPACE | SSD_END_ROW | SSD_ROUNDED_CORNERS
               | SSD_ROUNDED_WHITE | SSD_POINTER_NONE | SSD_CONTAINER_BORDER | SSD_ALIGN_CENTER);

   //Accound details header
   group = ssd_container_new("Foursquare Account Header group", NULL, SSD_MAX_SIZE, row_height,
         SSD_WIDGET_SPACE | SSD_END_ROW);
   ssd_widget_set_color(group, "#000000", "#ffffff");
   ssd_widget_add(group, ssd_text_new("Label", roadmap_lang_get("Account details"),
         SSD_MAIN_TEXT_SIZE, SSD_TEXT_NORMAL_FONT | SSD_TEXT_LABEL | SSD_ALIGN_VCENTER));
   bitmap = ssd_bitmap_new ("foursquare_icon", "foursquare_logo",SSD_ALIGN_RIGHT);
   ssd_widget_add(group, bitmap);
   ssd_widget_add(box, group);
   ssd_widget_add (box, ssd_separator_new ("separator", SSD_END_ROW));
   //Accound login status
   group = ssd_container_new("Foursquare Account Login group", NULL, SSD_MAX_SIZE, row_height,
         SSD_WIDGET_SPACE | SSD_END_ROW);
   ssd_widget_set_color(group, "#000000", "#ffffff");
   ssd_widget_add(group, ssd_text_new("Login Status Label", "",
         SSD_MAIN_TEXT_SIZE, SSD_TEXT_NORMAL_FONT | SSD_TEXT_LABEL | SSD_ALIGN_VCENTER));
   ssd_widget_add(box, group);
   ssd_widget_add (box, ssd_separator_new ("separator", SSD_END_ROW));
   //User name
   group = ssd_container_new("Foursquare Name group", NULL, SSD_MAX_SIZE, row_height,
         SSD_WIDGET_SPACE | SSD_END_ROW);
   ssd_widget_set_color(group, "#000000", "#ffffff");

   entry_label = ssd_entry_label_new( "FoursquareUserName", roadmap_lang_get("Email/phone"), SSD_MAIN_TEXT_SIZE, 160 /* Half of the SD canvas width. Scaled internally */,
                                             SSD_ROW_HEIGHT/2, SSD_ALIGN_VCENTER | SSD_WS_TABSTOP, roadmap_lang_get("User name") );
   ssd_widget_add( group, entry_label );
   ssd_widget_add( box, group );

   ssd_widget_add (box, ssd_separator_new ("separator", SSD_END_ROW));

   //Password
   group = ssd_container_new("Foursquare PW group", NULL, SSD_MAX_SIZE, row_height,
         SSD_WIDGET_SPACE | SSD_END_ROW);
   ssd_widget_set_color(group, "#000000", "#ffffff");
   entry_label = ssd_entry_label_new( "FoursquarePassword", roadmap_lang_get("Password"), SSD_MAIN_TEXT_SIZE, 160 /* Half of the SD canvas width. Scaled internally */,
                                             SSD_ROW_HEIGHT/2, SSD_ALIGN_VCENTER | SSD_WS_TABSTOP, roadmap_lang_get("Password") );
   ssd_entry_label_set_text_flags( entry_label, SSD_TEXT_PASSWORD );
   ssd_widget_add( group, entry_label );

   ssd_widget_add(box, group);

   ssd_widget_add(dialog, box);


   //tweet settings header
   ssd_widget_add(dialog, space(5));
   box = ssd_container_new ("Foursquare auto settings header group", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE,
            SSD_WIDGET_SPACE | SSD_END_ROW);

   ssd_widget_add (box, ssd_text_new ("foursquare_auto_settings_header",
         roadmap_lang_get ("Automatically tweet to my followers that I:"), SSD_HEADER_TEXT_SIZE, SSD_TEXT_NORMAL_FONT | SSD_TEXT_LABEL | SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE));
   ssd_widget_set_color (box, NULL, NULL);
   ssd_widget_add (dialog, box);
   ssd_widget_add(dialog, space(5));

   //Tweets
   box = ssd_container_new("Tweet toggles group", NULL, total_width, SSD_MIN_SIZE,
            SSD_WIDGET_SPACE | SSD_END_ROW | SSD_ROUNDED_CORNERS
                  | SSD_ROUNDED_WHITE | SSD_POINTER_NONE | SSD_CONTAINER_BORDER | SSD_ALIGN_CENTER);

   group = ssd_checkbox_row_new("FoursquareSendLogin", roadmap_lang_get ("Am checking out this integration"),
                                       TRUE, NULL,NULL,NULL,CHECKBOX_STYLE_ON_OFF);
   ssd_widget_add(box, group);
   ssd_widget_add (box, ssd_separator_new ("separator", SSD_END_ROW));



   group = ssd_checkbox_row_new("FoursquareSendBadgeUnlock", roadmap_lang_get ("Have unlocked the Roadwarrior Badge"),
                                          TRUE, NULL,NULL,NULL,CHECKBOX_STYLE_ON_OFF);
   ssd_widget_add(box, group);

   ssd_widget_add(dialog, box);


   //Foursquare comments
   ssd_widget_add(dialog, space(5));
   box = ssd_container_new ("Comments", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE,
            SSD_WIDGET_SPACE | SSD_END_ROW);

   text = ssd_text_new ("comment footer",
         roadmap_lang_get ("We've partnered with Foursquare to give you quick access to check-in to nearby venues."), SSD_FOOTER_TEXT_SIZE, SSD_TEXT_NORMAL_FONT | SSD_TEXT_LABEL | SSD_WIDGET_SPACE);
   ssd_text_set_color(text,notesColor);
   ssd_widget_add (box, text);

   text = ssd_text_new ("comment footer",
         roadmap_lang_get ("What is Foursquare?"), SSD_FOOTER_TEXT_SIZE, SSD_TEXT_NORMAL_FONT | SSD_TEXT_LABEL | SSD_WIDGET_SPACE);
   ssd_text_set_color(text,notesColor);
   ssd_widget_add (box, text);

   text = ssd_text_new ("comment footer",
        roadmap_lang_get ("It's a cool way to discover and promote cool places in your city and be rewarded for doing so."), SSD_FOOTER_TEXT_SIZE, SSD_TEXT_NORMAL_FONT | SSD_TEXT_LABEL | SSD_WIDGET_SPACE);
   ssd_text_set_color(text,notesColor);
   ssd_widget_add (box, text);

   text = ssd_text_new ("comment footer",
        roadmap_lang_get ("Don't have an account? Sign up on:"), SSD_FOOTER_TEXT_SIZE, SSD_TEXT_NORMAL_FONT | SSD_TEXT_LABEL | SSD_WIDGET_SPACE);
   ssd_text_set_color(text,notesColor);
   ssd_widget_add (box, text);

   text = ssd_text_new ("comment footer",
           roadmap_lang_get ("www.foursquare.com"), SSD_FOOTER_TEXT_SIZE, SSD_TEXT_NORMAL_FONT | SSD_TEXT_LABEL | SSD_WIDGET_SPACE|SSD_WS_TABSTOP);
   ssd_text_set_color(text,notesColor);
   ssd_widget_add (box, text);

   ssd_widget_set_color (box, NULL, NULL);
   ssd_widget_add (dialog, box);


#ifndef TOUCH_SCREEN
   ssd_widget_set_left_softkey_text ( dialog, roadmap_lang_get("Ok"));
   ssd_widget_set_left_softkey_callback ( dialog, on_ok_softkey);
#endif

}

static void create_address (FoursquareVenue*   venue, FoursquareCheckin* checkin) {
   sprintf(checkin->sAddress, "%s, %s",venue->sAddress, venue->sCity);
}

static int on_venue_item_selected(SsdWidget widget, const char* selection,const void *value, void* context)
{
   int index = (int)value;
#ifdef IPHONE_NATIVE
   index -= 10;
#endif //IPHONE_NATIVE

   gsRequestType = ROADMAP_FOURSQUARE_CHECKIN;

   roadmap_main_set_periodic(FOURSQUARE_REQUEST_TIMEOUT, request_time_out);
#ifdef IPHONE
   delayed_show_progress();
#else
   roadmap_main_set_periodic(100, delayed_show_progress);
#endif //IPHONE

   create_address (&gsVenuesList[index], &gsCheckInInfo);

   ssd_dialog_hide_all(dec_close);

   Realtime_FoursquareCheckin(gsVenuesList[index].sId,
         roadmap_foursquare_is_tweet_badge_enabled() && roadmap_twitter_logged_in());

   return 1;
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_foursquare_venues_list (void) {
   static   const char* results[ROADMAP_FOURSQUARE_MAX_VENUE_COUNT];
   static   void*       indexes[ROADMAP_FOURSQUARE_MAX_VENUE_COUNT];
   static   const char* icons[ROADMAP_FOURSQUARE_MAX_VENUE_COUNT];
   int i;

   roadmap_main_remove_periodic(roadmap_foursquare_venues_list);


   for (i = 0; i < gsVenuesCount; i++) {
      results[i] = gsVenuesList[i].sDescription;
#ifdef IPHONE_NATIVE
      indexes[i] = (void*)(i+10);
#else
      indexes[i] = (void*)i;
#endif
      icons[i] = "foursquare_checkin";
   }

#ifdef IPHONE_NATIVE
   roadmap_list_menu_generic(roadmap_lang_get(FOURSQUARE_TITLE),
                             gsVenuesCount,
                             results,
                             (const void **)indexes,
                             icons,
                             NULL,
                             NULL,
                             on_venue_item_selected,
                             NULL,
                             NULL,
                             NULL,
                             60,
                             0,
                             NULL);
#else
   ssd_generic_icon_list_dialog_show(roadmap_lang_get(FOURSQUARE_VENUES_TITLE),
                                       gsVenuesCount,
                                       results,
                                       (const void **)indexes,
                                       icons,
                                       0,
                                       on_venue_item_selected,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL,
                                       60,
                                       0,
                                       FALSE);

#endif //IPHONE_NATIVE
}

static void create_description (FoursquareVenue*   venue) {
   if (strlen(venue->sZip) <= 3) //filter non-valid zip codes like '-1', ' ' etc
      snprintf(venue->sDescription, sizeof(venue->sDescription), "%s\n%s, %s", venue->sName, venue->sAddress, venue->sCity);
   else
      snprintf(venue->sDescription, sizeof(venue->sDescription), "%s\n%s, %s, %s", venue->sName, venue->sAddress, venue->sCity, venue->sZip);
}


static const char* parse_checkin_results(roadmap_result* rc, int NumParams, const char*  pData) {
   //Expected data:
   // CheckinResult,<checkin_message>,<score_points>

   char CommandName[128];
   int iBufferSize;

   iBufferSize =  128;

   if (NumParams == 0)
      return pData;

   if ((NumParams - 1) % ROADMAP_FOURSQUARE_CHECKIN_ENTRIES != 0) {
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   pData       = ExtractNetworkString(
                       pData,             // [in]     Source string
                       CommandName,//   [out]   Output buffer
                       &iBufferSize,      // [in,out] Buffer size / Size of extracted string
                       ",",          //   [in]   Array of chars to terminate the copy operation
                       1);   // [in]     Remove additional termination chars

   if (strcmp(CommandName, "CheckinResult") != 0) {
      roadmap_log(ROADMAP_ERROR, "Foursquare - parse_search_results(): could not find command: CheckinResult (received: '%s')", CommandName);
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   //   1.   checkin message
   iBufferSize = ROADMAP_FOURSQUARE_MESSAGE_MAX_SIZE;
   pData       = ExtractNetworkString(
                  pData,               // [in]     Source string
                  gsCheckInInfo.sCheckinMessage,         // [out,opt]Output buffer
                  &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                  ",",                 // [in]     Array of chars to terminate the copy operation
                  1);                  // [in]     Remove additional termination chars

   if( !pData || !(*pData))
   {
      roadmap_log( ROADMAP_ERROR, "Foursquare - parse_checkin_results(): Failed to read checkin message=%s", gsCheckInInfo.sCheckinMessage);
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   //   2.   name
   iBufferSize = ROADMAP_FOURSQUARE_SCORE_PT_MAX_SIZE;
   pData       = ExtractNetworkString(
                  pData,               // [in]     Source string
                  gsCheckInInfo.sScorePoints,         // [out,opt]Output buffer
                  &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                  ",\r\n",                 // [in]     Array of chars to terminate the copy operation
                  1);                  // [in]     Remove additional termination chars

   if( !pData)
   {
      roadmap_log( ROADMAP_ERROR, "Foursquare - parse_checkin_results(): Failed to read score points=%s", gsCheckInInfo.sScorePoints);
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   roadmap_main_set_periodic(100, roadmap_foursquare_checkedin_dialog);

   return pData;
}

static const char* parse_search_results(roadmap_result* rc, int NumParams, const char*  pData) {
   //Expected data:
   // VenueList,<id>,<name>,<address>,<crossstreet>,<city>,<state>,<zip>,<geolat>,<geolong>,<phone>,<distance>[,<id>,.....]

   FoursquareVenue   venue;
   int i;
   char CommandName[128];
   int iBufferSize;
   double dValue;
   int count;

   iBufferSize =  128;

   if (NumParams == 0)
      return pData;

   if ((NumParams - 1) % ROADMAP_FOURSQUARE_VENUE_ENTRIES != 0) {
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   pData       = ExtractNetworkString(
                       pData,             // [in]     Source string
                       CommandName,//   [out]   Output buffer
                       &iBufferSize,      // [in,out] Buffer size / Size of extracted string
                       ",\r\n",          //   [in]   Array of chars to terminate the copy operation
                       1);   // [in]     Remove additional termination chars

   if (strcmp(CommandName, "VenueList") != 0) {
      roadmap_log(ROADMAP_ERROR, "Foursquare - parse_search_results(): could not find command: VenueList (received: '%s')", CommandName);
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   count = (NumParams - 1) / ROADMAP_FOURSQUARE_VENUE_ENTRIES;

   if (!(*pData) || count == 0) {
      roadmap_log(ROADMAP_DEBUG, "Foursquare - received empty venues list");
      ssd_dialog_hide_all(dec_close);
#ifdef IPHONE_NATIVE
      roadmap_main_show_root(1);
#endif //IPHONE_NATIVE
      roadmap_messagebox_timeout("Foursquare", "We can't find anything nearby.", 5);
      return pData;
   }

   for (i = 0; i < count; ++i){
      //   1.   id
      iBufferSize = ROADMAP_FOURSQUARE_ID_MAX_SIZE;
      pData       = ExtractNetworkString(
                     pData,               // [in]     Source string
                     venue.sId,           // [out,opt]Output buffer
                     &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                     ",",                 // [in]     Array of chars to terminate the copy operation
                     1);                  // [in]     Remove additional termination chars

      if( !pData || !(*pData))
      {
         roadmap_log( ROADMAP_ERROR, "Foursquare - parse_search_results(): Failed to read venue id=%s", venue.sId);
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }

      //   2.   name
      iBufferSize = ROADMAP_FOURSQUARE_NAME_MAX_SIZE;
      pData       = ExtractNetworkString(
                     pData,               // [in]     Source string
                     venue.sName,         // [out,opt]Output buffer
                     &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                     ",",                 // [in]     Array of chars to terminate the copy operation
                     1);                  // [in]     Remove additional termination chars

      if( !pData || !(*pData))
      {
         roadmap_log( ROADMAP_ERROR, "Foursquare - parse_search_results(): Failed to read venue name=%s", venue.sName);
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }

      //   3.   address
      iBufferSize = ROADMAP_FOURSQUARE_ADDRESS_MAX_SIZE;
      pData       = ExtractNetworkString(
                     pData,               // [in]     Source string
                     venue.sAddress,      // [out,opt]Output buffer
                     &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                     ",",                 // [in]     Array of chars to terminate the copy operation
                     1);                  // [in]     Remove additional termination chars

      if( !pData || !(*pData))
      {
         roadmap_log( ROADMAP_ERROR, "Foursquare - parse_search_results(): Failed to read venue address=%s", venue.sAddress);
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }

      //   4.   crossstreet
      iBufferSize = ROADMAP_FOURSQUARE_CROSS_STREET_MAX_SIZE;
      pData       = ExtractNetworkString(
                     pData,               // [in]     Source string
                     venue.sCrossStreet,  // [out,opt]Output buffer
                     &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                     ",",                 // [in]     Array of chars to terminate the copy operation
                     1);                  // [in]     Remove additional termination chars

      if( !pData || !(*pData))
      {
         roadmap_log( ROADMAP_ERROR, "Foursquare - parse_search_results(): Failed to read venue crossname=%s", venue.sCrossStreet);
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }

      //   5.   city
      iBufferSize = ROADMAP_FOURSQUARE_CITY_MAX_SIZE;
      pData       = ExtractNetworkString(
                     pData,               // [in]     Source string
                     venue.sCity,         // [out,opt]Output buffer
                     &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                     ",",                 // [in]     Array of chars to terminate the copy operation
                     1);                  // [in]     Remove additional termination chars

      if( !pData || !(*pData))
      {
         roadmap_log( ROADMAP_ERROR, "Foursquare - parse_search_results(): Failed to read venue city=%s", venue.sCity);
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }

      //   6.   state
      iBufferSize = ROADMAP_FOURSQUARE_STATE_MAX_SIZE;
      pData       = ExtractNetworkString(
                     pData,               // [in]     Source string
                     venue.sState,        // [out,opt]Output buffer
                     &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                     ",",                 // [in]     Array of chars to terminate the copy operation
                     1);                  // [in]     Remove additional termination chars

      if( !pData || !(*pData))
      {
         roadmap_log( ROADMAP_ERROR, "Foursquare - parse_search_results(): Failed to read venue state=%s", venue.sState);
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }

      //   7.   zip
      iBufferSize = ROADMAP_FOURSQUARE_ZIP_MAX_SIZE;
      pData       = ExtractNetworkString(
                     pData,               // [in]     Source string
                     venue.sZip,          // [out,opt]Output buffer
                     &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                     ",",                 // [in]     Array of chars to terminate the copy operation
                     1);                  // [in]     Remove additional termination chars

      if( !pData || !(*pData))
      {
         roadmap_log( ROADMAP_ERROR, "Foursquare - parse_search_results(): Failed to read venue zip=%s", venue.sZip);
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }

      //   8.   lat
      pData = ReadDoubleFromString(
                           pData,         //   [in]      Source string
                           ",",           //   [in,opt]  Value termination
                           NULL,          //   [in,opt]  Allowed padding
                           &dValue,       //   [out]     Output value
                           1);            //   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

      venue.iLatitude = (int) (dValue * 1000000);
      if( !pData || !(*pData))
      {
         roadmap_log( ROADMAP_ERROR, "Foursquare - parse_search_results(): Failed to read venue lat=%d", venue.iLatitude);
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }

      //   9.   lon
      pData = ReadDoubleFromString(
                           pData,         //   [in]      Source string
                           ",",           //   [in,opt]  Value termination
                           NULL,          //   [in,opt]  Allowed padding
                           &dValue,       //   [out]     Output value
                           1);            //   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

      venue.iLongitude = (int) (dValue * 1000000);
      if( !pData || !(*pData))
      {
         roadmap_log( ROADMAP_ERROR, "Foursquare - parse_search_results(): Failed to read venue lon=%d", venue.iLongitude);
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }

      //   10.   phone
      iBufferSize = ROADMAP_FOURSQUARE_PHONE_MAX_SIZE;
      pData       = ExtractNetworkString(
                     pData,               // [in]     Source string
                     venue.sPhone,        // [out,opt]Output buffer
                     &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                     ",",                 // [in]     Array of chars to terminate the copy operation
                     1);                  // [in]     Remove additional termination chars

      if( !pData || !(*pData))
      {
         roadmap_log( ROADMAP_ERROR, "Foursquare - parse_search_results(): Failed to read venue phone=%s", venue.sPhone);
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }

      //   11.   distance
      pData = ReadIntFromString(
                     pData,            //   [in]      Source string
                     ",\r\n",              //   [in,opt]   Value termination
                     NULL,             //   [in,opt]   Allowed padding
                     &venue.iDistance,    //   [out]      Put it here
                     1);               //   [in]      Remove additional termination CHARS

      if( !pData || (!(*pData) && i < count-1))
      {
         roadmap_log( ROADMAP_ERROR, "Foursquare - parse_search_results(): Failed to read venue distance=%d", venue.iDistance);
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }


      if (gsVenuesCount < ROADMAP_FOURSQUARE_MAX_VENUE_COUNT) { //skip if more venues received
         create_description (&venue);
         gsVenuesList[gsVenuesCount++] = venue;
      }
   }

   roadmap_main_set_periodic(100,roadmap_foursquare_venues_list);

   return pData;
}

const char* roadmap_foursquare_response(int status, roadmap_result* rc, int NumParams, const char*  pData){

   roadmap_main_remove_periodic(request_time_out);
   ssd_progress_msg_dialog_hide();

   if (status != 200){
      switch (gsRequestType) {
         case ROADMAP_FOURSQUARE_LOGIN:
            roadmap_log( ROADMAP_ERROR, "roadmap_foursquare_response (login) - Command failed (status= %d)", status );
            roadmap_foursquare_login_failed(status);
            break;
         case ROADMAP_FOURSQUARE_SEARCH:
            roadmap_log( ROADMAP_ERROR, "roadmap_foursquare_response (search) - Command failed (status= %d)", status );
            roadmap_messagebox ("Oops", "Could not connect with Foursquare. Try again later.");
            break;
         case ROADMAP_FOURSQUARE_CHECKIN:
            roadmap_log( ROADMAP_ERROR, "roadmap_foursquare_response (checkin) - Command failed (status= %d)", status );
            roadmap_messagebox ("Oops", "Could not connect with Foursquare. Try again later.");
           break;
         default:
            roadmap_log( ROADMAP_ERROR, "roadmap_foursquare_response (unknown) - Command failed (status= %d)", status );
      }
      gsRequestType = ROADMAP_FOURSQUARE_NONE;
      return pData;
   }

   switch (gsRequestType) {
      case ROADMAP_FOURSQUARE_LOGIN:
         roadmap_log( ROADMAP_DEBUG, "roadmap_foursquare_response (login) - successful");
         roadmap_foursquare_set_logged_in(TRUE);
         gsRequestType = ROADMAP_FOURSQUARE_NONE;
         if (gsCheckInOnLogin)
            roadmap_foursquare_checkin();
         break;
      case ROADMAP_FOURSQUARE_SEARCH:
         roadmap_log( ROADMAP_DEBUG, "roadmap_foursquare_response (search) - successful");
         gsRequestType = ROADMAP_FOURSQUARE_NONE;
         return parse_search_results(rc, NumParams, pData);
         break;
      case ROADMAP_FOURSQUARE_CHECKIN:
         roadmap_log( ROADMAP_DEBUG, "roadmap_foursquare_response (checkin) - successful");
         gsRequestType = ROADMAP_FOURSQUARE_NONE;
         return parse_checkin_results(rc, NumParams, pData);
        break;
      default:
         roadmap_log( ROADMAP_DEBUG, "roadmap_foursquare_response (unknown) - - successful");
   }

   return pData;
}

void roadmap_foursquare_request_failed (roadmap_result status) {
   roadmap_main_remove_periodic(request_time_out);
   ssd_progress_msg_dialog_hide();

   switch (gsRequestType) {
      case ROADMAP_FOURSQUARE_LOGIN:
         roadmap_log( ROADMAP_ERROR, "roadmap_foursquare_response (login) - network failed (status= %d)", status );
         roadmap_foursquare_login_failed(status);
         break;
      case ROADMAP_FOURSQUARE_SEARCH:
         roadmap_log( ROADMAP_ERROR, "roadmap_foursquare_response (search) - network failed failed (status= %d)", status );
         roadmap_messagebox ("Oops", "Could not connect with Foursquare. Try again later.");
         break;
      case ROADMAP_FOURSQUARE_CHECKIN:
         roadmap_log( ROADMAP_ERROR, "roadmap_foursquare_response (checkin) - network failed (status= %d)", status );
         roadmap_messagebox ("Oops", "Could not connect with Foursquare. Try again later.");
        break;
      default:
         roadmap_log( ROADMAP_ERROR, "roadmap_foursquare_response (unknown) - network failed (status= %d)", status );
   }
   gsRequestType = ROADMAP_FOURSQUARE_NONE;
}

#ifndef IPHONE_NATIVE
/////////////////////////////////////////////////////////////////////////////////////
void roadmap_foursquare_checkedin_dialog(void) {
   char text[256];

   roadmap_main_remove_periodic(roadmap_foursquare_checkedin_dialog);

   if (!ssd_dialog_activate(FOURSQUARE_CHECKEDIN_DIALOG_NAME, NULL)) {
      create_checkedin_dialog();
      ssd_dialog_activate(FOURSQUARE_CHECKEDIN_DIALOG_NAME, NULL);
   }

   ssd_dialog_set_value("Checkin message lablel", gsCheckInInfo.sCheckinMessage);
   ssd_dialog_set_value("Checkin address", gsCheckInInfo.sAddress);
   snprintf(text, sizeof(text), "%s %s", roadmap_lang_get("Points:"), gsCheckInInfo.sScorePoints);
   ssd_dialog_set_value("Checkin points", text);

   roadmap_screen_redraw();
}
/////////////////////////////////////////////////////////////////////////////////////
void roadmap_foursquare_login_dialog(void) {
   const char *pVal;

   if (!ssd_dialog_activate(FOURSQUARE_LOGIN_DIALOG_NAME, NULL)) {
      create_login_dialog();
      ssd_dialog_activate(FOURSQUARE_LOGIN_DIALOG_NAME, NULL);
   }

   if (roadmap_foursquare_logged_in())
      ssd_dialog_set_value("Login Status Label", roadmap_lang_get("Status: logged in"));
   else
      ssd_dialog_set_value("Login Status Label", roadmap_lang_get("Status: not logged in"));

   ssd_dialog_set_value("FoursquareUserName", roadmap_foursquare_get_username());
   ssd_dialog_set_value("FoursquarePassword", roadmap_foursquare_get_password());

   if (roadmap_foursquare_is_tweet_login_enabled())
      pVal = yesno[0];
   else
      pVal = yesno[1];
   ssd_dialog_set_data("FoursquareSendLogin", (void *) pVal);

   if (roadmap_foursquare_is_tweet_badge_enabled())
      pVal = yesno[0];
   else
      pVal = yesno[1];
   ssd_dialog_set_data("FoursquareSendBadgeUnlock", (void *) pVal);
}
#endif //IPHONE_NATIVE

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_foursquare_get_checkin_info (FoursquareCheckin *outCheckInInfo) {
   *outCheckInInfo = gsCheckInInfo;
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_foursquare_checkin(void) {
  const RoadMapPosition *coordinates;

   if (!roadmap_foursquare_logged_in()) {
      gsCheckInOnLogin = TRUE;
      roadmap_foursquare_login_dialog();
      return;
   }

   gsCheckInOnLogin = FALSE;
   gsRequestType = ROADMAP_FOURSQUARE_SEARCH;
   gsVenuesCount = 0;

   roadmap_main_set_periodic(FOURSQUARE_REQUEST_TIMEOUT, request_time_out);
#ifdef IPHONE
   delayed_show_progress();
#else
   roadmap_main_set_periodic(100, delayed_show_progress);
#endif //IPHONE
   coordinates = roadmap_trip_get_position( "Location" );
   Realtime_FoursquareSearch((RoadMapPosition *)coordinates);
}

/////////////////////////////////////////////////////////////////////////////////////
static int on_foursquare_checkin( SsdWidget this, const char *new_value){
   roadmap_foursquare_checkin();
   return 0;
}

/////////////////////////////////////////////////////////////////////////////////////
SsdWidget roadmap_foursquare_create_alert_menu(void) {
   SsdWidget foursquare_container, container, box, text;
   char *icon[2];
   int height = ssd_container_get_row_height();
   int width = ssd_container_get_width();

   if (!roadmap_foursquare_is_enabled())
      return NULL;

   //Foursquare Container
   foursquare_container = ssd_container_new ("__foursuqare_settings", NULL, width, SSD_MIN_SIZE,
                 SSD_ALIGN_CENTER|SSD_WIDGET_SPACE|SSD_END_ROW|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE|SSD_POINTER_NONE|SSD_CONTAINER_BORDER);

   box = ssd_container_new ("Foursquare checkin group", NULL,
                           SSD_MAX_SIZE,height,
                       SSD_WIDGET_SPACE|SSD_END_ROW|SSD_WS_TABSTOP);
   ssd_widget_set_color (box, NULL, NULL);
   box->callback = on_foursquare_checkin;
   icon[0] = "foursquare_checkin";
   icon[1] = NULL;
   container =  ssd_container_new ("button container", NULL, 80, height,0);
   ssd_widget_set_color(container, NULL, NULL);

   ssd_widget_add (container,
         ssd_button_new ("foursquare_checkin", "foursquare_checkin", (const char **)&icon[0], 1,
                        SSD_ALIGN_VCENTER|SSD_ALIGN_CENTER,
                        on_foursquare_checkin));
   ssd_widget_add(box, container);
   container = ssd_container_new ("text_box", NULL,
                                               SSD_MIN_SIZE,
                                               SSD_MIN_SIZE,
                                               SSD_ALIGN_VCENTER|SSD_END_ROW);

   ssd_widget_set_color (container, "#000000", NULL);

   text = ssd_text_new ("label_long",
                       roadmap_lang_get ("Check-in with Foursquare"),
                       SSD_MAIN_TEXT_SIZE, SSD_TEXT_NORMAL_FONT | SSD_ALIGN_VCENTER|SSD_END_ROW);
   ssd_widget_add (container, text);
   ssd_widget_add (box, container);


   ssd_widget_add (foursquare_container, box);

   return (foursquare_container);
}
