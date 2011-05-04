/* roadmap_social.c - Manages social network (Twitter / Facebook) accounts
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi B.S
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
#include "roadmap_social.h"
#include "roadmap_dialog.h"
#include "roadmap_messagebox.h"
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
#include "ssd/ssd_confirm_dialog.h"
#include "ssd/ssd_progress_msg_dialog.h"
#include "roadmap_start.h"

#include "Realtime/Realtime.h"
#include "Realtime/RealtimeDefs.h"

#include "roadmap_browser.h"

static const char *yesno[2];
static const char *privacy_values[3] = {"0", "1", "2"};

static SsdWidget CheckboxDestinationTwitter[ROADMAP_SOCIAL_DESTINATION_MODES_COUNT];
static SsdWidget CheckboxDestinationFacebook[ROADMAP_SOCIAL_DESTINATION_MODES_COUNT];

static wst_handle                s_websvc             = INVALID_WEBSVC_HANDLE;

extern const char* VerifyStatus( /* IN  */   const char*       pNext,
                                /* IN  */   void*             pContext,
                                /* OUT */   BOOL*             more_data_needed,
                                /* OUT */   roadmap_result*   rc);

static wst_parser data_parser[] =
{
   { "RC", VerifyStatus}
};


//   Twitter User name
RoadMapConfigDescriptor TWITTER_CFG_PRM_NAME_Var = ROADMAP_CONFIG_ITEM(
      TWITTER_CONFIG_TAB,
      SOCIAL_CFG_PRM_NAME_Name);

//   Twitter Password
RoadMapConfigDescriptor TWITTER_CFG_PRM_PASSWORD_Var = ROADMAP_CONFIG_ITEM(
      TWITTER_CONFIG_TAB,
      SOCIAL_CFG_PRM_PASSWORD_Name);

//   Enable / Disable Tweeting road reports
RoadMapConfigDescriptor TWITTER_CFG_PRM_SEND_REPORTS_Var = ROADMAP_CONFIG_ITEM(
      TWITTER_CONFIG_TAB,
      SOCIAL_CFG_PRM_SEND_REPORTS_Name);

//   Enable / Disable Tweeting destination
RoadMapConfigDescriptor TWITTER_CFG_PRM_SEND_DESTINATION_Var = ROADMAP_CONFIG_ITEM(
      TWITTER_CONFIG_TAB,
      SOCIAL_CFG_PRM_SEND_DESTINATION_Name);

//   Enable / Disable Road goodie munching destination
RoadMapConfigDescriptor TWITTER_CFG_PRM_SEND_MUNCHING_Var = ROADMAP_CONFIG_ITEM(
      TWITTER_CONFIG_TAB,
      SOCIAL_CFG_PRM_SEND_MUNCHING_Name);
RoadMapConfigDescriptor TWITTER_CFG_PRM_SHOW_MUNCHING_Var = ROADMAP_CONFIG_ITEM(
      TWITTER_CONFIG_TAB,
      SOCIAL_CFG_PRM_SHOW_MUNCHING_Name);

//   Enable / Disable Tweeting sign up
RoadMapConfigDescriptor TWITTER_CFG_PRM_SEND_SIGNUP_Var = ROADMAP_CONFIG_ITEM(
      TWITTER_CONFIG_TAB,
      SOCIAL_CFG_PRM_SEND_SIGNUP_Name);

//   Twitter profile permissions
RoadMapConfigDescriptor TWITTER_CFG_PRM_SHOW_PROFILE_Var = ROADMAP_CONFIG_ITEM(
      TWITTER_CONFIG_TAB,
      SOCIAL_CFG_PRM_SHOW_PROFILE_Name);

//    Logged in status
RoadMapConfigDescriptor TWITTER_CFG_PRM_LOGGED_IN_Var = ROADMAP_CONFIG_ITEM(
      TWITTER_CONFIG_TAB,
      SOCIAL_CFG_PRM_LOGGED_IN_Name);


//   Enable / Disable Facebook road reports
RoadMapConfigDescriptor FACEBOOK_CFG_PRM_SEND_REPORTS_Var = ROADMAP_CONFIG_ITEM(
      FACEBOOK_CONFIG_TAB,
      SOCIAL_CFG_PRM_SEND_REPORTS_Name);

//   Enable / Disable Facebook destination
RoadMapConfigDescriptor FACEBOOK_CFG_PRM_SEND_DESTINATION_Var = ROADMAP_CONFIG_ITEM(
      FACEBOOK_CONFIG_TAB,
      SOCIAL_CFG_PRM_SEND_DESTINATION_Name);

//   Enable / Disable Facebook Road goodie munching destination
RoadMapConfigDescriptor FACEBOOK_CFG_PRM_SEND_MUNCHING_Var = ROADMAP_CONFIG_ITEM(
      FACEBOOK_CONFIG_TAB,
      SOCIAL_CFG_PRM_SEND_MUNCHING_Name);
RoadMapConfigDescriptor FACEBOOK_CFG_PRM_SHOW_MUNCHING_Var = ROADMAP_CONFIG_ITEM(
      FACEBOOK_CONFIG_TAB,
      SOCIAL_CFG_PRM_SHOW_MUNCHING_Name);

//   Facebook name permissions
RoadMapConfigDescriptor FACEBOOK_CFG_PRM_SHOW_NAME_Var = ROADMAP_CONFIG_ITEM(
      FACEBOOK_CONFIG_TAB,
      SOCIAL_CFG_PRM_SHOW_NAME_Name);

//   Facebook picture permissions
RoadMapConfigDescriptor FACEBOOK_CFG_PRM_SHOW_PICTURE_Var = ROADMAP_CONFIG_ITEM(
      FACEBOOK_CONFIG_TAB,
      SOCIAL_CFG_PRM_SHOW_PICTURE_Name);

//   Facebook profile permissions
RoadMapConfigDescriptor FACEBOOK_CFG_PRM_SHOW_PROFILE_Var = ROADMAP_CONFIG_ITEM(
      FACEBOOK_CONFIG_TAB,
      SOCIAL_CFG_PRM_SHOW_PROFILE_Name);

//    Logged in status - Facebook
RoadMapConfigDescriptor FACEBOOK_CFG_PRM_LOGGED_IN_Var = ROADMAP_CONFIG_ITEM(
      FACEBOOK_CONFIG_TAB,
      SOCIAL_CFG_PRM_LOGGED_IN_Name);

//    URL - Facebook
RoadMapConfigDescriptor FACEBOOK_CFG_PRM_URL_Var = ROADMAP_CONFIG_ITEM(
      FACEBOOK_CONFIG_TAB,
      FACEBOOK_CFG_PRM_URL_Name);

//    URL - Facebook Share
RoadMapConfigDescriptor FACEBOOK_CFG_PRM_ShareURL_Var = ROADMAP_CONFIG_ITEM(
      FACEBOOK_CONFIG_TAB,
      FACEBOOK_CFG_PRM_ShareURL_Name);



enum {
   DLG_TYPE_TWITTER,
   DLG_TYPE_FACEBOOK
};

static RoadMapCallback SocialNextLoginCb = NULL;



/////////////////////////////////////////////////////////////////////////////////////
void roadmap_social_login_cb (void){
   roadmap_facebook_check_login();

   if (SocialNextLoginCb) {
      SocialNextLoginCb ();
      SocialNextLoginCb = NULL;
   }
}

/////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_social_initialize(void) {

   // Name
   roadmap_config_declare(SOCIAL_CONFIG_TYPE, &TWITTER_CFG_PRM_NAME_Var,
         SOCIAL_CFG_PRM_NAME_Default, NULL);

   // Password
   roadmap_config_declare_password(SOCIAL_CONFIG_TYPE,
         &TWITTER_CFG_PRM_PASSWORD_Var, SOCIAL_CFG_PRM_PASSWORD_Default);

   // Road reports - Twitter
   roadmap_config_declare_enumeration(SOCIAL_CONFIG_TYPE,
         &TWITTER_CFG_PRM_SEND_REPORTS_Var, NULL,
         SOCIAL_CFG_PRM_SEND_REPORTS_Enabled, SOCIAL_CFG_PRM_SEND_REPORTS_Disabled, NULL);

   // Destination - Twitter
   roadmap_config_declare_enumeration(SOCIAL_CONFIG_TYPE,
         &TWITTER_CFG_PRM_SEND_DESTINATION_Var, NULL,
         SOCIAL_CFG_PRM_SEND_DESTINATION_Disabled, SOCIAL_CFG_PRM_SEND_DESTINATION_City,
         SOCIAL_CFG_PRM_SEND_DESTINATION_Street, SOCIAL_CFG_PRM_SEND_DESTINATION_Full, NULL);

   // Road goodies munching - Twitter
   roadmap_config_declare_enumeration(SOCIAL_CONFIG_TYPE,
         &TWITTER_CFG_PRM_SEND_MUNCHING_Var, NULL,
         SOCIAL_CFG_PRM_SEND_MUNCHING_Enabled, SOCIAL_CFG_PRM_SEND_MUNCHING_Disabled, NULL);
   roadmap_config_declare_enumeration(SOCIAL_CONFIG_PREF_TYPE,
            &TWITTER_CFG_PRM_SHOW_MUNCHING_Var, NULL,
            SOCIAL_CFG_PRM_SHOW_MUNCHING_No, SOCIAL_CFG_PRM_SHOW_MUNCHING_Yes, NULL);

   // Show user profile - Twitter
   roadmap_config_declare_enumeration(SOCIAL_CONFIG_TYPE,
            &TWITTER_CFG_PRM_SHOW_PROFILE_Var, NULL,
            SOCIAL_CFG_PRM_SHOW_PROFILE_Disabled, SOCIAL_CFG_PRM_SHOW_PROFILE_Friends, SOCIAL_CFG_PRM_SHOW_PROFILE_Enabled, NULL);


   // Sign up - Twitter
   roadmap_config_declare_enumeration(SOCIAL_CONFIG_TYPE,
         &TWITTER_CFG_PRM_SEND_SIGNUP_Var, NULL,
         SOCIAL_CFG_PRM_SEND_SIGNUP_Enabled, SOCIAL_CFG_PRM_SEND_SIGNUP_Disabled, NULL);

   // Logged in status - Twitter
   roadmap_config_declare_enumeration(SOCIAL_CONFIG_TYPE,
            &TWITTER_CFG_PRM_LOGGED_IN_Var, NULL,
            SOCIAL_CFG_PRM_LOGGED_IN_No, SOCIAL_CFG_PRM_LOGGED_IN_Yes, NULL);

   // Show user name - Facebook
   roadmap_config_declare_enumeration(SOCIAL_CONFIG_TYPE,
            &FACEBOOK_CFG_PRM_SHOW_NAME_Var, NULL,
            SOCIAL_CFG_PRM_SHOW_NAME_Disabled, SOCIAL_CFG_PRM_SHOW_NAME_Friends, SOCIAL_CFG_PRM_SHOW_NAME_Enabled, NULL);

   // Show user picture - Facebook
   roadmap_config_declare_enumeration(SOCIAL_CONFIG_TYPE,
            &FACEBOOK_CFG_PRM_SHOW_PICTURE_Var, NULL,
            SOCIAL_CFG_PRM_SHOW_PICTURE_Disabled, SOCIAL_CFG_PRM_SHOW_PICTURE_Friends, SOCIAL_CFG_PRM_SHOW_PICTURE_Enabled, NULL);

   // Show user profile - Facebook
   roadmap_config_declare_enumeration(SOCIAL_CONFIG_TYPE,
            &FACEBOOK_CFG_PRM_SHOW_PROFILE_Var, NULL,
            SOCIAL_CFG_PRM_SHOW_PROFILE_Disabled, SOCIAL_CFG_PRM_SHOW_PROFILE_Friends, SOCIAL_CFG_PRM_SHOW_PROFILE_Enabled, NULL);

   // Road reports - Facebook
   roadmap_config_declare_enumeration(SOCIAL_CONFIG_TYPE,
         &FACEBOOK_CFG_PRM_SEND_REPORTS_Var, NULL,
         SOCIAL_CFG_PRM_SEND_REPORTS_Enabled, SOCIAL_CFG_PRM_SEND_REPORTS_Disabled, NULL);

   // Destination - Facebook
   roadmap_config_declare_enumeration(SOCIAL_CONFIG_TYPE,
         &FACEBOOK_CFG_PRM_SEND_DESTINATION_Var, NULL,
         SOCIAL_CFG_PRM_SEND_DESTINATION_Disabled, SOCIAL_CFG_PRM_SEND_DESTINATION_City,
         SOCIAL_CFG_PRM_SEND_DESTINATION_Street, SOCIAL_CFG_PRM_SEND_DESTINATION_Full, NULL);

   // Road goodies munching - Facebook
   roadmap_config_declare_enumeration(SOCIAL_CONFIG_TYPE,
         &FACEBOOK_CFG_PRM_SEND_MUNCHING_Var, NULL,
         SOCIAL_CFG_PRM_SEND_MUNCHING_Disabled, SOCIAL_CFG_PRM_SEND_MUNCHING_Enabled, NULL);
   roadmap_config_declare_enumeration(SOCIAL_CONFIG_PREF_TYPE,
            &FACEBOOK_CFG_PRM_SHOW_MUNCHING_Var, NULL,
            SOCIAL_CFG_PRM_SHOW_MUNCHING_No, SOCIAL_CFG_PRM_SHOW_MUNCHING_Yes, NULL);

   // Logged in status - Facebook
   roadmap_config_declare_enumeration(SOCIAL_CONFIG_TYPE,
            &FACEBOOK_CFG_PRM_LOGGED_IN_Var, NULL,
            SOCIAL_CFG_PRM_LOGGED_IN_No, SOCIAL_CFG_PRM_LOGGED_IN_Yes, NULL);

   // URL - Facebook
   roadmap_config_declare_enumeration(SOCIAL_CONFIG_PREF_TYPE,
            &FACEBOOK_CFG_PRM_URL_Var, NULL,
            FACEBOOK_CFG_PRM_URL_Default, NULL);
   
   // URL - Facebook Share
   roadmap_config_declare_enumeration(SOCIAL_CONFIG_PREF_TYPE,
            &FACEBOOK_CFG_PRM_ShareURL_Var, NULL,
            FACEBOOK_CFG_PRM_ShareURL_Default, NULL);



   yesno[0] = "Yes";
   yesno[1] = "No";

   SocialNextLoginCb = Realtime_NotifyOnLogin (roadmap_social_login_cb);

   return TRUE;
}


/////////////////////////////////////////////////////////////////////////////////////
void roadmap_twitter_set_logged_in (BOOL is_logged_in) {
   if (is_logged_in)
      roadmap_config_set(&TWITTER_CFG_PRM_LOGGED_IN_Var, SOCIAL_CFG_PRM_LOGGED_IN_Yes);
   else
      roadmap_config_set(&TWITTER_CFG_PRM_LOGGED_IN_Var, SOCIAL_CFG_PRM_LOGGED_IN_No);
   roadmap_config_save(FALSE);
}

/////////////////////////////////////////////////////////////////////////////////////
static BOOL roadmap_social_logged_in(RoadMapConfigDescriptor *config) {
   if (0 == strcmp(roadmap_config_get(config),
                   SOCIAL_CFG_PRM_LOGGED_IN_Yes))
      return TRUE;
   return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_twitter_logged_in(void) {
   return roadmap_social_logged_in(&TWITTER_CFG_PRM_LOGGED_IN_Var);
}

/////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_facebook_logged_in(void) {
   return roadmap_social_logged_in(&FACEBOOK_CFG_PRM_LOGGED_IN_Var);
}

static const char *roadmap_facebook_url (void) {
	return roadmap_config_get(&FACEBOOK_CFG_PRM_URL_Var);
}

/////////////////////////////////////////////////////////////////////////////////////
static void on_check_login_completed( void* ctx, roadmap_result res) {
   if (res == succeeded)
      roadmap_config_set(&FACEBOOK_CFG_PRM_LOGGED_IN_Var, SOCIAL_CFG_PRM_LOGGED_IN_Yes);
   else
      roadmap_config_set(&FACEBOOK_CFG_PRM_LOGGED_IN_Var, SOCIAL_CFG_PRM_LOGGED_IN_No);
   roadmap_config_save(FALSE);

   roadmap_facebook_refresh_connection();
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_facebook_check_login(void) {
   char url[256];
   char query[256];

   roadmap_log (ROADMAP_DEBUG, "check login");


   snprintf(url, sizeof(url), "%s%s", roadmap_facebook_url(), FACEBOOK_IS_CONNECTED_SUFFIX);
   snprintf(query, sizeof(query), "sessionid=%d&cookie=%s&deviceid=%d",
            Realtime_GetServerId(),
            Realtime_GetServerCookie(),
            RT_DEVICE_ID);

   if (INVALID_WEBSVC_HANDLE != s_websvc)
      wst_term (s_websvc);

   s_websvc = wst_init( url, NULL, NULL, "application/x-www-form-urlencoded; charset=utf-8");

   if (INVALID_WEBSVC_HANDLE == s_websvc) {
      roadmap_log (ROADMAP_ERROR, "roadmap_facebook_check_login() - invalid websvc handle");
      return;
   }

   wst_start_trans( s_websvc,
                    0,
                   "external_facebook",
                   data_parser,
                   sizeof(data_parser)/sizeof(wst_parser),
                   on_check_login_completed,
                   (void *)s_websvc, //TODO: consider non global
                   query);
}

/////////////////////////////////////////////////////////////////////////////////////
static void after_facebook_connect (void) {
   roadmap_facebook_check_login();
   roadmap_social_send_permissions();
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_facebook_connect(BOOL preload) {
   char url[256];

   snprintf(url, sizeof(url), "%s%s?sessionid=%d&cookie=%s&deviceid=%d&on_close=dialog_hide_current&client_version=%s&web_version=%s&lang=%s",
            roadmap_facebook_url(),
            FACEBOOK_CONNECT_SUFFIX,
            Realtime_GetServerId(),
            Realtime_GetServerCookie(),
            RT_DEVICE_ID,
            roadmap_start_version(),
            BROWSER_WEB_VERSION,
            roadmap_lang_get_system_lang());
   if (!preload)
      roadmap_browser_show("Connect to Facebook", url, after_facebook_connect, NULL, NULL, BROWSER_BAR_NORMAL);
#ifdef IPHONE
   else
      roadmap_browser_iphone_preload("Connect to Facebook", url, after_facebook_connect, NULL, NULL, BROWSER_BAR_NORMAL, 0);
#endif //IPHONE
}

static void on_disconnect_completed( void* ctx, roadmap_result res) {
   ssd_progress_msg_dialog_hide();

   roadmap_facebook_check_login();
}

/////////////////////////////////////////////////////////////////////////////////////
static void facebook_disconnect_confirmed_cb(int exit_code, void *context){
   char url[256];
   char query[256];

   if (exit_code != dec_yes)
      return;

   roadmap_log (ROADMAP_DEBUG, "Facebook logout");
   ssd_progress_msg_dialog_show("Disconnecting Facebook...");

   snprintf(url, sizeof(url), "%s%s", roadmap_facebook_url(), FACEBOOK_DISCONNECT_SUFFIX);
   snprintf(query, sizeof(query), "sessionid=%d&cookie=%s&deviceid=%d",
            Realtime_GetServerId(),
            Realtime_GetServerCookie(),
            RT_DEVICE_ID );

   if (INVALID_WEBSVC_HANDLE != s_websvc)
      wst_term (s_websvc);

   s_websvc = wst_init( url, NULL, NULL, "application/x-www-form-urlencoded; charset=utf-8");

   wst_start_trans( s_websvc,
                   0,
                   "external_facebook",
                   data_parser,
                   sizeof(data_parser)/sizeof(wst_parser),
                   on_disconnect_completed,
                   (void *)s_websvc, //TODO: consider non global
                   query);
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_facebook_disconnect(void) {
   ssd_confirm_dialog ("", "Disconnect from Facebook?", TRUE, facebook_disconnect_confirmed_cb , NULL);
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_facebook_invite(void) {
   //Not implemented server side
   char url[256];

   snprintf(url, sizeof(url), "%s%s?sessionid=%d&cookie=%s", roadmap_facebook_url(), FACEBOOK_SHARE_SUFFIX,
            Realtime_GetServerId(),
            Realtime_GetServerCookie());
   roadmap_browser_show("Invite friends", url, NULL, NULL, NULL, BROWSER_BAR_NORMAL);
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_facebook_share(void) {
   const char *url = roadmap_config_get(&FACEBOOK_CFG_PRM_ShareURL_Var);
   
   roadmap_browser_show("Share on Facebook", url, NULL, NULL, NULL, BROWSER_BAR_NORMAL);
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_twitter_login_failed(int status) {
   if (roadmap_twitter_logged_in()) {
      if (status == 701)
         roadmap_messagebox("Oops", "Updating your twitter account details failed. Please ensure you entered the correct user name and password");
      else
         roadmap_messagebox("Oops", "Twitter error");
      roadmap_twitter_set_logged_in (FALSE);
   }
   roadmap_log (ROADMAP_DEBUG, "Twitter status error (%d)", status);
}

/////////////////////////////////////////////////////////////////////////////////////
const char *roadmap_twitter_get_username(void) {
   return roadmap_config_get(&TWITTER_CFG_PRM_NAME_Var);
}

/////////////////////////////////////////////////////////////////////////////////////
const char *roadmap_twitter_get_password(void) {
   return roadmap_config_get(&TWITTER_CFG_PRM_PASSWORD_Var);
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_twitter_set_username(const char *user_name) {
   roadmap_config_set(&TWITTER_CFG_PRM_NAME_Var, user_name); //AR: we save only username
   roadmap_config_save(FALSE);
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_twitter_set_password(const char *password) {
   roadmap_config_set(&TWITTER_CFG_PRM_PASSWORD_Var, SOCIAL_CFG_PRM_PASSWORD_Default/*password*/); //AR: we don't want to save login details
   roadmap_config_save(FALSE);
}

/////////////////////////////////////////////////////////////////////////////////////
static BOOL roadmap_social_is_sending_enabled(RoadMapConfigDescriptor *config) {
   if (0 == strcmp(roadmap_config_get(config),
                   SOCIAL_CFG_PRM_SEND_REPORTS_Enabled))
      return TRUE;
   return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_twitter_is_sending_enabled(void) {
   return roadmap_social_is_sending_enabled(&TWITTER_CFG_PRM_SEND_REPORTS_Var);
}

/////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_facebook_is_sending_enabled(void) {
   return roadmap_social_is_sending_enabled(&FACEBOOK_CFG_PRM_SEND_REPORTS_Var);
}

/////////////////////////////////////////////////////////////////////////////////////
static void roadmap_social_set_sending(RoadMapConfigDescriptor *config, BOOL enabled) {
   if (enabled)
       roadmap_config_set(config, SOCIAL_CFG_PRM_SEND_REPORTS_Enabled);
   else
      roadmap_config_set(config, SOCIAL_CFG_PRM_SEND_REPORTS_Disabled);

   roadmap_config_save(FALSE);
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_twitter_enable_sending() {
   roadmap_social_set_sending(&TWITTER_CFG_PRM_SEND_REPORTS_Var, TRUE);
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_facebook_enable_sending() {
   roadmap_social_set_sending(&FACEBOOK_CFG_PRM_SEND_REPORTS_Var, TRUE);
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_twitter_disable_sending() {
   roadmap_social_set_sending(&TWITTER_CFG_PRM_SEND_REPORTS_Var, FALSE);
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_facebook_disable_sending() {
   roadmap_social_set_sending(&FACEBOOK_CFG_PRM_SEND_REPORTS_Var, FALSE);
}

/////////////////////////////////////////////////////////////////////////////////////
static int roadmap_social_destination_mode(RoadMapConfigDescriptor *config) {
   if (0 == strcmp(roadmap_config_get(config),
                   SOCIAL_CFG_PRM_SEND_DESTINATION_Full))
      return ROADMAP_SOCIAL_DESTINATION_MODE_FULL;
   else if (0 == strcmp(roadmap_config_get(config),
                        SOCIAL_CFG_PRM_SEND_DESTINATION_City))
      return ROADMAP_SOCIAL_DESTINATION_MODE_CITY;
   else
      return ROADMAP_SOCIAL_DESTINATION_MODE_DISABLED;
}

/////////////////////////////////////////////////////////////////////////////////////
int roadmap_twitter_destination_mode(void) {
   return roadmap_social_destination_mode(&TWITTER_CFG_PRM_SEND_DESTINATION_Var);
}

/////////////////////////////////////////////////////////////////////////////////////
int roadmap_facebook_destination_mode(void) {
   return roadmap_social_destination_mode(&FACEBOOK_CFG_PRM_SEND_DESTINATION_Var);
}

/////////////////////////////////////////////////////////////////////////////////////
static void roadmap_social_set_destination_mode(RoadMapConfigDescriptor *config, int mode) {
   switch (mode) {
      case ROADMAP_SOCIAL_DESTINATION_MODE_FULL:
         roadmap_config_set(config,
                            SOCIAL_CFG_PRM_SEND_DESTINATION_Full);
         break;
      case ROADMAP_SOCIAL_DESTINATION_MODE_CITY:
         roadmap_config_set(config,
                            SOCIAL_CFG_PRM_SEND_DESTINATION_City);
         break;
      default:
         roadmap_config_set(config,
                            SOCIAL_CFG_PRM_SEND_DESTINATION_Disabled);
         break;
   }

   roadmap_config_save(FALSE);
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_twitter_set_destination_mode(int mode) {
   roadmap_social_set_destination_mode(&TWITTER_CFG_PRM_SEND_DESTINATION_Var, mode);
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_facebook_set_destination_mode(int mode) {
   roadmap_social_set_destination_mode(&FACEBOOK_CFG_PRM_SEND_DESTINATION_Var, mode);
}

/////////////////////////////////////////////////////////////////////////////////////
static BOOL roadmap_social_is_munching_enabled(RoadMapConfigDescriptor *config) {
   if (0 == strcmp(roadmap_config_get(config),
                   SOCIAL_CFG_PRM_SEND_MUNCHING_Enabled))
      return TRUE;
   return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_twitter_is_munching_enabled(void) {
   return roadmap_social_is_munching_enabled(&TWITTER_CFG_PRM_SEND_MUNCHING_Var);
}

/////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_facebook_is_munching_enabled(void) {
   return roadmap_social_is_munching_enabled(&FACEBOOK_CFG_PRM_SEND_MUNCHING_Var);
}

/////////////////////////////////////////////////////////////////////////////////////
static void roadmap_social_set_munching(RoadMapConfigDescriptor *config, BOOL enabled) {
   if (enabled)
      roadmap_config_set(config, SOCIAL_CFG_PRM_SEND_MUNCHING_Enabled);
   else
      roadmap_config_set(config, SOCIAL_CFG_PRM_SEND_MUNCHING_Disabled);

   roadmap_config_save(FALSE);
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_twitter_enable_munching() {
   roadmap_social_set_munching(&TWITTER_CFG_PRM_SEND_MUNCHING_Var, TRUE);
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_facebook_enable_munching() {
   roadmap_social_set_munching(&FACEBOOK_CFG_PRM_SEND_MUNCHING_Var, TRUE);
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_twitter_disable_munching() {
   roadmap_social_set_munching(&TWITTER_CFG_PRM_SEND_MUNCHING_Var, FALSE);
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_facebook_disable_munching() {
   roadmap_social_set_munching(&FACEBOOK_CFG_PRM_SEND_MUNCHING_Var, FALSE);
}

/////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_twitter_is_show_munching(void) {
   if (0 == strcmp(roadmap_config_get(&TWITTER_CFG_PRM_SHOW_MUNCHING_Var),
         SOCIAL_CFG_PRM_SHOW_MUNCHING_Yes))
      return TRUE;
   return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////
static BOOL roadmap_social_is_signup_enabled(RoadMapConfigDescriptor *config) {
   if (0 == strcmp(roadmap_config_get(config),
                   SOCIAL_CFG_PRM_SEND_SIGNUP_Enabled))
      return TRUE;
   return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_twitter_is_signup_enabled(void) {
   return roadmap_social_is_signup_enabled(&TWITTER_CFG_PRM_SEND_SIGNUP_Var);
}

/////////////////////////////////////////////////////////////////////////////////////
static void roadmap_social_set_signup(RoadMapConfigDescriptor *config, BOOL enabled) {
   if (enabled)
      roadmap_config_set(config, SOCIAL_CFG_PRM_SEND_SIGNUP_Enabled);
   else
      roadmap_config_set(config, SOCIAL_CFG_PRM_SEND_SIGNUP_Disabled);

   roadmap_config_save(FALSE);
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_twitter_set_signup(BOOL enabled) {
   roadmap_social_set_signup(&TWITTER_CFG_PRM_SEND_SIGNUP_Var, enabled);
}

/////////////////////////////////////////////////////////////////////////////////////
static int roadmap_social_get_show_name(RoadMapConfigDescriptor *config) {
   if (0 == strcmp(roadmap_config_get(config),
                   SOCIAL_CFG_PRM_SHOW_NAME_Enabled))
      return ROADMAP_SOCIAL_SHOW_DETAILS_MODE_ENABLED;
   else if (0 == strcmp(roadmap_config_get(config),
                        SOCIAL_CFG_PRM_SHOW_NAME_Friends))
      return ROADMAP_SOCIAL_SHOW_DETAILS_MODE_FRIENDS;
   else
      return ROADMAP_SOCIAL_SHOW_DETAILS_MODE_DISABLED;
}

/////////////////////////////////////////////////////////////////////////////////////
int roadmap_facebook_get_show_name(void) {
   return roadmap_social_get_show_name(&FACEBOOK_CFG_PRM_SHOW_NAME_Var);
}

/////////////////////////////////////////////////////////////////////////////////////
static void roadmap_social_set_show_name(RoadMapConfigDescriptor *config, int mode) {
   switch (mode) {
      case ROADMAP_SOCIAL_SHOW_DETAILS_MODE_ENABLED:
         roadmap_config_set(config,
                            SOCIAL_CFG_PRM_SHOW_NAME_Enabled);
         break;
      case ROADMAP_SOCIAL_SHOW_DETAILS_MODE_FRIENDS:
         roadmap_config_set(config,
                            SOCIAL_CFG_PRM_SHOW_NAME_Friends);
         break;
      default:
         roadmap_config_set(config,
                            SOCIAL_CFG_PRM_SHOW_NAME_Disabled);
         break;
   }

   roadmap_config_save(FALSE);
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_facebook_set_show_name(int mode) {
   roadmap_social_set_show_name(&FACEBOOK_CFG_PRM_SHOW_NAME_Var, mode);
   OnSettingsChanged_VisabilityGroup(); // notify server of visibilaty settings change
}

/////////////////////////////////////////////////////////////////////////////////////
static int roadmap_social_get_show_picture(RoadMapConfigDescriptor *config) {
   if (0 == strcmp(roadmap_config_get(config),
                   SOCIAL_CFG_PRM_SHOW_PICTURE_Enabled))
      return ROADMAP_SOCIAL_SHOW_DETAILS_MODE_ENABLED;
   else if (0 == strcmp(roadmap_config_get(config),
                        SOCIAL_CFG_PRM_SHOW_PICTURE_Friends))
      return ROADMAP_SOCIAL_SHOW_DETAILS_MODE_FRIENDS;
   else
      return ROADMAP_SOCIAL_SHOW_DETAILS_MODE_DISABLED;
}

/////////////////////////////////////////////////////////////////////////////////////
int roadmap_facebook_get_show_picture(void) {
   return roadmap_social_get_show_picture(&FACEBOOK_CFG_PRM_SHOW_PICTURE_Var);
}

/////////////////////////////////////////////////////////////////////////////////////
static void roadmap_social_set_show_picture(RoadMapConfigDescriptor *config, int mode) {
   switch (mode) {
      case ROADMAP_SOCIAL_SHOW_DETAILS_MODE_ENABLED:
         roadmap_config_set(config,
                            SOCIAL_CFG_PRM_SHOW_PICTURE_Enabled);
         break;
      case ROADMAP_SOCIAL_SHOW_DETAILS_MODE_FRIENDS:
         roadmap_config_set(config,
                            SOCIAL_CFG_PRM_SHOW_PICTURE_Friends);
         break;
      default:
         roadmap_config_set(config,
                            SOCIAL_CFG_PRM_SHOW_PICTURE_Disabled);
         break;
   }

   roadmap_config_save(FALSE);
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_facebook_set_show_picture(int mode) {
   roadmap_social_set_show_picture(&FACEBOOK_CFG_PRM_SHOW_PICTURE_Var, mode);
   OnSettingsChanged_VisabilityGroup(); // notify server of visibilaty settings change
}

/////////////////////////////////////////////////////////////////////////////////////
static int roadmap_social_get_show_profile(RoadMapConfigDescriptor *config) {
   if (0 == strcmp(roadmap_config_get(config),
                   SOCIAL_CFG_PRM_SHOW_PROFILE_Enabled))
      return ROADMAP_SOCIAL_SHOW_DETAILS_MODE_ENABLED;
   else if (0 == strcmp(roadmap_config_get(config),
                        SOCIAL_CFG_PRM_SHOW_PROFILE_Friends))
      return ROADMAP_SOCIAL_SHOW_DETAILS_MODE_FRIENDS;
   else
      return ROADMAP_SOCIAL_SHOW_DETAILS_MODE_DISABLED;
}

/////////////////////////////////////////////////////////////////////////////////////
int roadmap_facebook_get_show_profile(void) {
   return roadmap_social_get_show_profile(&FACEBOOK_CFG_PRM_SHOW_PROFILE_Var);
}

/////////////////////////////////////////////////////////////////////////////////////
int roadmap_twitter_get_show_profile(void) {
   return roadmap_social_get_show_profile(&TWITTER_CFG_PRM_SHOW_PROFILE_Var);
}

/////////////////////////////////////////////////////////////////////////////////////
static void roadmap_social_set_show_profile(RoadMapConfigDescriptor *config, int mode) {
   switch (mode) {
      case ROADMAP_SOCIAL_SHOW_DETAILS_MODE_ENABLED:
         roadmap_config_set(config,
                            SOCIAL_CFG_PRM_SHOW_PROFILE_Enabled);
         break;
      case ROADMAP_SOCIAL_SHOW_DETAILS_MODE_FRIENDS:
         roadmap_config_set(config,
                            SOCIAL_CFG_PRM_SHOW_PROFILE_Friends);
         break;
      default:
         roadmap_config_set(config,
                            SOCIAL_CFG_PRM_SHOW_PROFILE_Disabled);
         break;
   }

   roadmap_config_save(FALSE);
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_facebook_set_show_profile(int mode) {
   roadmap_social_set_show_profile(&FACEBOOK_CFG_PRM_SHOW_PROFILE_Var, mode);
   OnSettingsChanged_VisabilityGroup(); // notify server of visibilaty settings change
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_twitter_set_show_profile(int mode) {
   roadmap_social_set_show_profile(&TWITTER_CFG_PRM_SHOW_PROFILE_Var, mode);
   OnSettingsChanged_VisabilityGroup(); // notify server of visibilaty settings change
}

/////////////////////////////////////////////////////////////////////////////////////
static void twitter_un_empty(void){
   roadmap_main_remove_periodic (twitter_un_empty);
   //roadmap_twitter_setting_dialog();
   roadmap_messagebox("Error", "Twitter user name is empty. You are not logged in");
}

static void twitter_pw_empty(void){
   roadmap_main_remove_periodic (twitter_pw_empty);
   //roadmap_twitter_setting_dialog();
   roadmap_messagebox("Error", "Twitter password is empty. You are not logged in");
}

/////////////////////////////////////////////////////////////////////////////////////
static void twitter_network_error(void){
   roadmap_main_remove_periodic (twitter_network_error);
   roadmap_messagebox("Oops", roadmap_lang_get("There is no network connection.Updating your twitter account details failed."));
   roadmap_twitter_set_logged_in (FALSE);
   roadmap_twitter_setting_dialog();
}

void roadmap_social_send_permissions (void) {
   if (Realtime_IsAnonymous() ||
       Realtime_is_random_user()) {
      Realtime_FacebookPermissions(ROADMAP_SOCIAL_SHOW_DETAILS_MODE_DISABLED,
                                   ROADMAP_SOCIAL_SHOW_DETAILS_MODE_DISABLED,
                                   ROADMAP_SOCIAL_SHOW_DETAILS_MODE_DISABLED,
                                   ROADMAP_SOCIAL_SHOW_DETAILS_MODE_DISABLED);
   } else {
      Realtime_FacebookPermissions(roadmap_facebook_get_show_name(),
                                   roadmap_facebook_get_show_picture(),
                                   roadmap_facebook_get_show_profile(),
                                   roadmap_twitter_get_show_profile());
   }
}
/////////////////////////////////////////////////////////////////////////////////////
static int on_ok(void *context) {
   BOOL success;
   BOOL send_reports = FALSE;
   int  destination_mode = 0;
   int i;
   BOOL send_munch = FALSE;
   const char *selected;
   BOOL logged_in;
   const char *user_name;
   const char *password;
   int dlg_type = (int)context;
   int show_name;
   int show_pic;
   const char* val;

   if (dlg_type == DLG_TYPE_TWITTER) {
      user_name = ssd_dialog_get_value("TwitterUserName");
      password = ssd_dialog_get_value("TwitterPassword");

      logged_in = roadmap_twitter_logged_in();
   } else {
      logged_in = roadmap_facebook_logged_in();

      val = (const char*) ssd_dialog_get_data("FacebookShowName");
      show_name = atoi(val);

      val = (const char*) ssd_dialog_get_data("FacebookShowPic");
      show_pic = atoi(val);
   }

   if (!strcasecmp((const char*) ssd_dialog_get_data("TwitterSendTwitts"),
         yesno[0]))
      send_reports = TRUE;

   for (i = 0; i < ROADMAP_SOCIAL_DESTINATION_MODES_COUNT; i++) {
      if (dlg_type == DLG_TYPE_TWITTER)
         selected = ssd_dialog_get_data (CheckboxDestinationTwitter[i]->name);
      else
         selected = ssd_dialog_get_data (CheckboxDestinationFacebook[i]->name);
      if (!strcmp (selected, "yes")) {
         break;
      }
   }
   if (i == ROADMAP_SOCIAL_DESTINATION_MODES_COUNT)
      i = 0;
   destination_mode = i;

   if (roadmap_twitter_is_show_munching())
      if (!strcasecmp((const char*) ssd_dialog_get_data("TwitterSendMunch"), yesno[0]))
         send_munch = TRUE;

   if (dlg_type == DLG_TYPE_TWITTER) {
      if (user_name[0] != 0 && password[0] != 0) {
         roadmap_twitter_set_username(user_name);
         roadmap_twitter_set_password(password);
         success = Realtime_TwitterConnect(user_name, password, roadmap_twitter_is_signup_enabled());
         if (!success) {
            roadmap_main_set_periodic (100, twitter_network_error);
            return 1;
         }
         roadmap_twitter_set_logged_in (TRUE);
      } else if (!logged_in && (send_reports || destination_mode > 0 || send_munch)){
         if (user_name[0] == 0) {
            roadmap_main_set_periodic (100, twitter_un_empty);
            return 1;
         }

         if (password[0] == 0) {
            roadmap_main_set_periodic (100, twitter_pw_empty);
            return 1;
         }
      }

      if (send_reports)
         roadmap_twitter_enable_sending();
      else
         roadmap_twitter_disable_sending();

      roadmap_twitter_set_destination_mode(destination_mode);

      if (send_munch)
         roadmap_twitter_enable_munching();
      else
         roadmap_twitter_disable_munching();
   } else { // Facebook
      if (show_name != roadmap_facebook_get_show_name())
         roadmap_facebook_set_show_name (show_name);

      if (show_pic != roadmap_facebook_get_show_picture())
         roadmap_facebook_set_show_picture (show_pic);

      roadmap_social_send_permissions();

      if (send_reports)
         roadmap_facebook_enable_sending();
      else
         roadmap_facebook_disable_sending();

      roadmap_facebook_set_destination_mode(destination_mode);

      if (send_munch)
         roadmap_facebook_enable_munching();
      else
         roadmap_facebook_disable_munching();
   }


   return 0;
}

/////////////////////////////////////////////////////////////////////////////////////
static void on_dlg_close(int exit_code, void* context) {
   if (exit_code == dec_ok) {
      on_ok(context);
   }
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
int dest_checkbox_callback_twitter (SsdWidget widget, const char *new_value) {
   int i;
   for (i = 0; i < ROADMAP_SOCIAL_DESTINATION_MODES_COUNT; i++) {
      if (CheckboxDestinationTwitter[i] &&
            strcmp (widget->parent->name, CheckboxDestinationTwitter[i]->name))
         CheckboxDestinationTwitter[i]->set_data (CheckboxDestinationTwitter[i], "no");
      else
         CheckboxDestinationTwitter[i]->set_data (CheckboxDestinationTwitter[i], "yes");
   }
   return 1;
}

/////////////////////////////////////////////////////////////////////////////////////
static int dest_checkbox_callback_facebook (SsdWidget widget, const char *new_value) {
   int i;
   for (i = 0; i < ROADMAP_SOCIAL_DESTINATION_MODES_COUNT; i++) {
      if (CheckboxDestinationFacebook[i] &&
            strcmp (widget->parent->name, CheckboxDestinationFacebook[i]->name))
         CheckboxDestinationFacebook[i]->set_data (CheckboxDestinationFacebook[i], "no");
      else
         CheckboxDestinationFacebook[i]->set_data (CheckboxDestinationFacebook[i], "yes");
   }
   return 1;
}

/////////////////////////////////////////////////////////////////////////////////////
static int login_button_callback_facebook( SsdWidget widget, const char *new_value){
   if (roadmap_facebook_logged_in())
      roadmap_facebook_disconnect();
   else
      roadmap_facebook_connect(0);
   return 0;
}

/////////////////////////////////////////////////////////////////////////////////////
static void create_dialog(int dlg_type) {

   SsdWidget dialog;
   SsdWidget group, group2;
   SsdWidget box;
   SsdWidget destination;
   SsdWidget entry_label;
   SsdWidget text;
   SsdWidget bitmap;
   SsdWidget *checkbox;
   SsdCallback dest_checkbox_cb;
   int width;
   int tab_flag = SSD_WS_TABSTOP;
   BOOL checked = FALSE;
   const char * notesColor = "#383838"; // some sort of gray
   BOOL isTwitter = (dlg_type == DLG_TYPE_TWITTER);
   int total_width = ssd_container_get_width();
   int row_height = ssd_container_get_row_height();

   width = total_width/2;

   if (isTwitter) {
      dialog = ssd_dialog_new(TWITTER_DIALOG_NAME,
         roadmap_lang_get(TWITTER_TITTLE), on_dlg_close, SSD_CONTAINER_TITLE);
      checkbox = CheckboxDestinationTwitter;
      dest_checkbox_cb = dest_checkbox_callback_twitter;
   } else {
      dialog = ssd_dialog_new(FACEBOOK_DIALOG_NAME,
         roadmap_lang_get(FACEBOOK_TITTLE), on_dlg_close, SSD_CONTAINER_TITLE);
      checkbox = CheckboxDestinationFacebook;
      dest_checkbox_cb = dest_checkbox_callback_facebook;
   }

#ifdef TOUCH_SCREEN
   ssd_widget_add (dialog, space(5));
#endif

   box = ssd_container_new("UN/PW group", NULL, total_width, SSD_MIN_SIZE,
         SSD_WIDGET_SPACE | SSD_END_ROW | SSD_ROUNDED_CORNERS
               | SSD_ROUNDED_WHITE | SSD_POINTER_NONE | SSD_CONTAINER_BORDER | SSD_ALIGN_CENTER);

   //Accound details header
   if (isTwitter) {
      group = ssd_container_new("Twitter Account Header group", NULL, SSD_MAX_SIZE,row_height,
                                SSD_WIDGET_SPACE | SSD_END_ROW);
      ssd_widget_set_color(group, "#000000", "#ffffff");
      ssd_widget_add(group, ssd_text_new("Label", roadmap_lang_get("Account details"),
                                         SSD_MAIN_TEXT_SIZE ,SSD_TEXT_NORMAL_FONT | SSD_TEXT_LABEL | SSD_ALIGN_VCENTER));
      bitmap = ssd_bitmap_new ("twitter_icon", "Tweeter-logo",SSD_ALIGN_RIGHT);
      ssd_widget_add(group, bitmap);
      ssd_widget_add(box, group);
      ssd_widget_add (box, ssd_separator_new ("separator", SSD_END_ROW));
   }
   //Accound login status
   if (isTwitter) {
      group = ssd_container_new("Twitter Account Login group", NULL, SSD_MAX_SIZE,row_height,
                                SSD_WIDGET_SPACE | SSD_END_ROW);
      ssd_widget_set_color(group, "#000000", "#ffffff");
   } else {
      group = ssd_container_new("Facebook Account Login group", NULL, SSD_MAX_SIZE,row_height,
                                SSD_WIDGET_SPACE | SSD_END_ROW| tab_flag);
      group->callback = login_button_callback_facebook;
      ssd_widget_set_color(group, "#000000", "#ffffff");

      bitmap = ssd_bitmap_new ("Login Status Icon", "facebook_connect",SSD_ALIGN_RIGHT | SSD_ALIGN_VCENTER);
      ssd_widget_add(group, bitmap);
   }
   ssd_widget_add(group, ssd_text_new("Login Status Label", "",
         SSD_MAIN_TEXT_SIZE ,SSD_TEXT_NORMAL_FONT | SSD_TEXT_LABEL | SSD_ALIGN_VCENTER));
   ssd_widget_add(box, group);
   if (isTwitter) {
      ssd_widget_add (box, ssd_separator_new ("separator", SSD_END_ROW));
      //User name
      group = ssd_container_new("Twitter Name group", NULL, SSD_MAX_SIZE, row_height,
            SSD_WIDGET_SPACE | SSD_END_ROW);
      entry_label = ssd_entry_label_new( "TwitterUserName", roadmap_lang_get("User name"), SSD_MAIN_TEXT_SIZE, 160 /* Half of the SD canvas width. Scaled internally */,
                                                SSD_ROW_HEIGHT/2, SSD_ALIGN_VCENTER | SSD_WS_TABSTOP, roadmap_lang_get("User name") );
      ssd_widget_add( group, entry_label );

      ssd_widget_add(box, group);
      ssd_widget_add (box, ssd_separator_new ("separator", SSD_END_ROW));

      //Password
      group = ssd_container_new("Twitter PW group", NULL, SSD_MAX_SIZE, row_height,
            SSD_WIDGET_SPACE | SSD_END_ROW);
      ssd_widget_set_color(group, "#000000", "#ffffff");
      entry_label = ssd_entry_label_new( "TwitterPassword", roadmap_lang_get("Password"), SSD_MAIN_TEXT_SIZE, 160 /* Half of the SD canvas width. Scaled internally */,
                                                SSD_ROW_HEIGHT/2, SSD_ALIGN_VCENTER | SSD_WS_TABSTOP, roadmap_lang_get("Password") );
      ssd_entry_label_set_text_flags( entry_label, SSD_TEXT_PASSWORD );
      ssd_widget_add( group, entry_label );
      ssd_widget_add(box, group);

   } else {

      static const char *privacy_labels[3];


      privacy_labels[0] = strdup(roadmap_lang_get("Don't show"));
      privacy_labels[1] = strdup(roadmap_lang_get("To friends only"));
      privacy_labels[2] = strdup(roadmap_lang_get("To everyone"));

      ssd_widget_add (box, ssd_separator_new ("separator", SSD_END_ROW));
      //Use FB name
      group = ssd_container_new("Show_name group", NULL, SSD_MAX_SIZE, row_height,
            SSD_START_NEW_ROW | SSD_WIDGET_SPACE | SSD_END_ROW | tab_flag);
      ssd_widget_set_color(group, "#000000", "#ffffff");

      group2 = ssd_container_new ("group2", NULL, 2*roadmap_canvas_width()/3, SSD_MIN_SIZE,
                                 SSD_ALIGN_VCENTER);
      ssd_widget_set_color(group2, NULL, NULL);
      ssd_widget_add(group2, ssd_text_new("Show_name_label", roadmap_lang_get(
                  "Show my Facebook name (on app & web)"), SSD_MAIN_TEXT_SIZE, SSD_TEXT_NORMAL_FONT | SSD_TEXT_LABEL
                  | SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE));
      ssd_widget_add(group, group2);

      ssd_widget_add (group,
            ssd_choice_new ("FacebookShowName", roadmap_lang_get ("Show my Facebook name (on app & web)"), 3,
                            (const char **)privacy_labels,
                            (const void **)privacy_values,
                            SSD_ALIGN_RIGHT, NULL));

      ssd_widget_add(box, group);

      ssd_widget_add (box, ssd_separator_new ("separator", SSD_END_ROW));

      //Use FB picture
      group = ssd_container_new("Show_pic group", NULL, SSD_MAX_SIZE, row_height,
            SSD_START_NEW_ROW | SSD_WIDGET_SPACE | SSD_END_ROW | tab_flag);
      ssd_widget_set_color(group, "#000000", "#ffffff");

      group2 = ssd_container_new ("group2", NULL, 2*roadmap_canvas_width()/3, SSD_MIN_SIZE,
                                 SSD_ALIGN_VCENTER);
      ssd_widget_set_color(group2, NULL, NULL);
      ssd_widget_add(group2, ssd_text_new("Show_pic_label", roadmap_lang_get(
               "Show my Facebook pic (on app & web)"), SSD_MAIN_TEXT_SIZE, SSD_TEXT_NORMAL_FONT | SSD_TEXT_LABEL
               | SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE));
      ssd_widget_add(group, group2);

      ssd_widget_add (group,
            ssd_choice_new ("FacebookShowPic", roadmap_lang_get ("Show my Facebook pic (on app & web)"), 3,
                            (const char **)privacy_labels,
                            (const void **)privacy_values,
                            SSD_ALIGN_RIGHT, NULL));

      ssd_widget_add(box, group);
   }

   ssd_widget_add(dialog, box);


   //tweet settings header
   ssd_widget_add(dialog, space(5));
   box = ssd_container_new ("Tweeter auto settings header group", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE,
            SSD_WIDGET_SPACE | SSD_END_ROW);

   if (isTwitter)
      ssd_widget_add (box, ssd_text_new ("tweeter_auto_settings_header",
            roadmap_lang_get ("Automatically tweet to my followers:"), SSD_HEADER_TEXT_SIZE, SSD_TEXT_NORMAL_FONT | SSD_TEXT_LABEL | SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE));
   else
      ssd_widget_add (box, ssd_text_new ("tweeter_auto_settings_header",
            roadmap_lang_get ("Automatically post to Facebook:"), SSD_HEADER_TEXT_SIZE, SSD_TEXT_NORMAL_FONT | SSD_TEXT_LABEL | SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE));
   ssd_widget_set_color (box, NULL, NULL);
   ssd_widget_add (dialog, box);
   ssd_widget_add(dialog, space(5));

   //Send Reports Yes/No
   box = ssd_container_new("Send_Reports box", NULL, total_width, SSD_MIN_SIZE,
         SSD_START_NEW_ROW | SSD_WIDGET_SPACE | SSD_END_ROW
                              | SSD_ROUNDED_CORNERS | SSD_ROUNDED_WHITE
                              | SSD_POINTER_NONE | SSD_CONTAINER_BORDER | SSD_ALIGN_CENTER);
   ssd_widget_set_color(box, "#000000", "#ffffff");

   group = ssd_checkbox_row_new("TwitterSendTwitts", roadmap_lang_get ("My road reports"),
                                 TRUE, NULL,NULL,NULL,CHECKBOX_STYLE_ON_OFF);
   ssd_widget_add(box, group);
   ssd_widget_add(dialog, box);

   //example
   box = ssd_container_new ("report_example group", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE,
      SSD_WIDGET_SPACE | SSD_END_ROW);
   text = ssd_text_new ("report_example_text_cont",
      roadmap_lang_get ("e.g:  Just reported a traffic jam on Geary St. SF, CA using @waze Social GPS."),
                           SSD_FOOTER_TEXT_SIZE, SSD_TEXT_NORMAL_FONT | SSD_TEXT_LABEL | SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE);
   ssd_text_set_color(text,notesColor);
   ssd_widget_add (box,text );
   ssd_widget_set_color (box, NULL, NULL);
   ssd_widget_add (dialog, box);

   //Send Destination
   ssd_widget_add(dialog, space(5));
   destination = ssd_container_new("Send_Destination group", NULL, total_width, SSD_MIN_SIZE,
            SSD_START_NEW_ROW | SSD_WIDGET_SPACE | SSD_END_ROW
                                 | SSD_ROUNDED_CORNERS | SSD_ROUNDED_WHITE
                                 | SSD_POINTER_NONE | SSD_CONTAINER_BORDER | SSD_ALIGN_CENTER);
   box = ssd_container_new ("Destination Heading group", NULL, SSD_MAX_SIZE,row_height,
         SSD_WIDGET_SPACE | SSD_END_ROW);
   ssd_widget_add (box, ssd_text_new ("destination_heading_label",
            roadmap_lang_get ("My destination and ETA"), SSD_MAIN_TEXT_SIZE, SSD_TEXT_NORMAL_FONT |SSD_TEXT_LABEL
                     | SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE | SSD_END_ROW));

   ssd_widget_add (box, ssd_separator_new ("separator", SSD_ALIGN_BOTTOM));
   ssd_widget_set_color (box, NULL, NULL);
   ssd_widget_add (destination, box);
   //Disabled
   box = ssd_container_new ("Destination disabled Group", NULL, SSD_MAX_SIZE, row_height,
         SSD_WIDGET_SPACE | SSD_END_ROW| tab_flag);
   if (roadmap_twitter_destination_mode() == ROADMAP_SOCIAL_DESTINATION_MODE_DISABLED)
      checked = TRUE;
   else
      checked = FALSE;

   checkbox[ROADMAP_SOCIAL_DESTINATION_MODE_DISABLED] = ssd_checkbox_new (SOCIAL_CFG_PRM_SEND_DESTINATION_Disabled,
               checked, SSD_ALIGN_VCENTER | SSD_ALIGN_RIGHT, dest_checkbox_cb, NULL, NULL,
               CHECKBOX_STYLE_V);
   ssd_widget_add (box, checkbox[ROADMAP_SOCIAL_DESTINATION_MODE_DISABLED]);

//   hor_space = ssd_container_new ("spacer1", NULL, 10, 14, 0);
//   ssd_widget_set_color (hor_space, NULL, NULL);
//   ssd_widget_add (box, hor_space);

   ssd_widget_add (box, ssd_text_new ("Destination disabled", roadmap_lang_get (
            "Disabled"), SSD_MAIN_TEXT_SIZE, SSD_TEXT_NORMAL_FONT |SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE
            | SSD_END_ROW));

   ssd_widget_set_color (box, NULL, NULL);
   ssd_widget_add (destination, box);
   ssd_widget_add (destination, ssd_separator_new ("separator", SSD_END_ROW));

   // State and City
   box = ssd_container_new ("Destination city Group", NULL, SSD_MAX_SIZE, row_height,
         SSD_WIDGET_SPACE | SSD_END_ROW| tab_flag);
   if (roadmap_twitter_destination_mode() == ROADMAP_SOCIAL_DESTINATION_MODE_CITY)
      checked = TRUE;
   else
      checked = FALSE;

   checkbox[ROADMAP_SOCIAL_DESTINATION_MODE_CITY] = ssd_checkbox_new (SOCIAL_CFG_PRM_SEND_DESTINATION_City,
               checked, SSD_ALIGN_VCENTER | SSD_ALIGN_RIGHT, dest_checkbox_cb, NULL, NULL,
               CHECKBOX_STYLE_V);
   ssd_widget_add (box, checkbox[ROADMAP_SOCIAL_DESTINATION_MODE_CITY]);

//   hor_space = ssd_container_new ("spacer1", NULL, 10, 14, 0);
//   ssd_widget_set_color (hor_space, NULL, NULL);
//   ssd_widget_add (box, hor_space);

   ssd_widget_add (box, ssd_text_new ("Destination city", roadmap_lang_get (
            "City & state only"), SSD_MAIN_TEXT_SIZE, SSD_TEXT_NORMAL_FONT |SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE
            | SSD_END_ROW));

   ssd_widget_set_color (box, NULL, NULL);
   ssd_widget_add (destination, box);
   ssd_widget_add (destination, ssd_separator_new ("separator", SSD_END_ROW));

   // Street, State and City (future option)
   box = ssd_container_new ("Destination street Group", NULL, SSD_MAX_SIZE, row_height,
         SSD_WIDGET_SPACE | SSD_END_ROW| tab_flag);
   if (roadmap_twitter_destination_mode() == ROADMAP_SOCIAL_DESTINATION_MODE_STREET)
      checked = TRUE;
   else
      checked = FALSE;

   checkbox[ROADMAP_SOCIAL_DESTINATION_MODE_STREET] = ssd_checkbox_new (SOCIAL_CFG_PRM_SEND_DESTINATION_Street,
               checked, SSD_ALIGN_VCENTER | SSD_ALIGN_RIGHT, dest_checkbox_cb, NULL, NULL,
               CHECKBOX_STYLE_V);

   ssd_widget_add (box, checkbox[ROADMAP_SOCIAL_DESTINATION_MODE_STREET]);

//   hor_space = ssd_container_new ("spacer1", NULL, 10, 14, 0);
//   ssd_widget_set_color (hor_space, NULL, NULL);
//   ssd_widget_add (box, hor_space);

   ssd_widget_add (box, ssd_text_new ("Destination street", roadmap_lang_get (
            "Street, City & State"), SSD_MAIN_TEXT_SIZE, SSD_TEXT_NORMAL_FONT | SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE
            | SSD_END_ROW));

   ssd_widget_add (box, ssd_separator_new ("separator", SSD_ALIGN_BOTTOM));
   ssd_widget_set_color (box, NULL, NULL);
   ssd_widget_add (destination, box);
   ssd_widget_hide(box);//future option


   // Full address
   box = ssd_container_new ("Destination full Group", NULL, SSD_MAX_SIZE, row_height,
         SSD_WIDGET_SPACE | SSD_END_ROW| tab_flag);
   if (roadmap_twitter_destination_mode() == ROADMAP_SOCIAL_DESTINATION_MODE_FULL)
      checked = TRUE;
   else
      checked = FALSE;

   checkbox[ROADMAP_SOCIAL_DESTINATION_MODE_FULL] = ssd_checkbox_new (SOCIAL_CFG_PRM_SEND_DESTINATION_Full,
               checked, SSD_ALIGN_VCENTER | SSD_ALIGN_RIGHT, dest_checkbox_cb, NULL, NULL,
               CHECKBOX_STYLE_V);
   ssd_widget_add (box, checkbox[ROADMAP_SOCIAL_DESTINATION_MODE_FULL]);

//   hor_space = ssd_container_new ("spacer1", NULL, 10, 14, 0);
//   ssd_widget_set_color (hor_space, NULL, NULL);
//   ssd_widget_add (box, hor_space);

   ssd_widget_add (box, ssd_text_new ("Destination full", roadmap_lang_get (
            "House #, Street, City, State"), SSD_MAIN_TEXT_SIZE, SSD_TEXT_NORMAL_FONT | SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE
            | SSD_END_ROW));

   ssd_widget_set_color (box, NULL, NULL);
   ssd_widget_add (destination, box);

   ssd_widget_add (dialog, destination);

   //example
   box = ssd_container_new ("destination_example group", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE,
      SSD_WIDGET_SPACE | SSD_END_ROW);
   text = ssd_text_new ("destination_example_text_cont",
      roadmap_lang_get ("e.g:  Driving to Greary St. SF, using @waze social GPS. ETA 2:32pm."),
                        SSD_FOOTER_TEXT_SIZE, SSD_TEXT_NORMAL_FONT | SSD_TEXT_LABEL | SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE);
   ssd_text_set_color(text,notesColor);
   ssd_widget_add (box,text );
   ssd_widget_set_color (box, NULL, NULL);
   ssd_widget_add (dialog, box);

   //Send road goodie munch Yes/No
   if (roadmap_twitter_is_show_munching()) {
      ssd_widget_add(dialog, space(5));

      box = ssd_container_new("Send_Reports box", NULL, total_width, SSD_MIN_SIZE,
                                 SSD_START_NEW_ROW | SSD_WIDGET_SPACE | SSD_END_ROW
                                 | SSD_ROUNDED_CORNERS | SSD_ROUNDED_WHITE
                                 | SSD_POINTER_NONE | SSD_CONTAINER_BORDER | SSD_ALIGN_CENTER);
      ssd_widget_set_color(box, "#000000", "#ffffff");

      group = ssd_checkbox_row_new("TwitterSendMunch", roadmap_lang_get ("My road munching"),
                                    TRUE, NULL,NULL,NULL,CHECKBOX_STYLE_ON_OFF);
      ssd_widget_add(box, group);
      ssd_widget_add(dialog, box);

      //example
      box = ssd_container_new ("munch_example group", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE,
         SSD_WIDGET_SPACE | SSD_END_ROW);
      text = ssd_text_new ("munch_example_text_cont",
         roadmap_lang_get ("e.g:  Just munched a 'waze road goodie' worth 200 points on Geary St. SF driving with @waze social GPS"),
                           SSD_FOOTER_TEXT_SIZE, SSD_TEXT_NORMAL_FONT | SSD_TEXT_LABEL | SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE| tab_flag);
      ssd_text_set_color(text,notesColor);
      ssd_widget_add (box,text );
      ssd_widget_set_color (box, NULL, NULL);
      ssd_widget_add (dialog, box);
   }

#ifndef TOUCH_SCREEN
   ssd_widget_set_left_softkey_text ( dialog, roadmap_lang_get("Ok"));
   ssd_widget_set_left_softkey_callback ( dialog, on_ok_softkey);
#endif

}

#ifndef IPHONE_NATIVE
/////////////////////////////////////////////////////////////////////////////////////
void roadmap_twitter_setting_dialog(void) {
   const char *pVal;

   if (!ssd_dialog_activate(TWITTER_DIALOG_NAME, (void *)DLG_TYPE_TWITTER)) {
      create_dialog(DLG_TYPE_TWITTER);
      ssd_dialog_activate(TWITTER_DIALOG_NAME, (void *)DLG_TYPE_TWITTER);
   }

   if (roadmap_twitter_logged_in())
      ssd_dialog_set_value("Login Status Label", roadmap_lang_get("Status: logged in"));
   else
      ssd_dialog_set_value("Login Status Label", roadmap_lang_get("Status: not logged in"));

   ssd_dialog_set_value("TwitterUserName", roadmap_twitter_get_username());
   ssd_dialog_set_value("TwitterPassword", roadmap_twitter_get_password());

   if (roadmap_twitter_is_sending_enabled())
      pVal = yesno[0];
   else
      pVal = yesno[1];
   ssd_dialog_set_data("TwitterSendTwitts", (void *) pVal);

   if (roadmap_twitter_is_munching_enabled())
      pVal = yesno[0];
   else
      pVal = yesno[1];
   ssd_dialog_set_data("TwitterSendMunch", (void *) pVal);
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_facebook_refresh_connection (void) {
   SsdWidget dialog;
   char *dialog_name = ssd_dialog_currently_active_name();
   if ((dialog_name != NULL) && (strcmp(dialog_name, FACEBOOK_DIALOG_NAME) == 0 )) {
      dialog = ssd_dialog_get_currently_active();
      if (roadmap_facebook_logged_in()) {
         ssd_dialog_set_value("Login Status Label", roadmap_lang_get("Status: logged in"));
         ssd_bitmap_update(ssd_widget_get(dialog, "Login Status Icon"), "facebook_disconnect");
      } else {
         ssd_dialog_set_value("Login Status Label", roadmap_lang_get("Status: not logged in"));
         ssd_bitmap_update(ssd_widget_get(dialog, "Login Status Icon"), "facebook_connect");
      }
   }
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_facebook_setting_dialog(void) {
   const char *pVal;
   int show_pic;
   int show_name;

   if (!ssd_dialog_activate(FACEBOOK_DIALOG_NAME, (void *)DLG_TYPE_FACEBOOK)) {
      create_dialog(DLG_TYPE_FACEBOOK);
      ssd_dialog_activate(FACEBOOK_DIALOG_NAME, (void *)DLG_TYPE_FACEBOOK);
   }

   roadmap_facebook_refresh_connection();


   show_pic = roadmap_facebook_get_show_picture();

   show_name = roadmap_facebook_get_show_name();

   ssd_dialog_set_data("FacebookShowName", (void *) privacy_values[show_name]);

   ssd_dialog_set_data("FacebookShowPic", (void *) privacy_values[show_pic]);

   if (roadmap_facebook_is_sending_enabled())
      pVal = yesno[0];
   else
      pVal = yesno[1];
   ssd_dialog_set_data("TwitterSendTwitts", (void *) pVal);

   if (roadmap_facebook_is_munching_enabled())
      pVal = yesno[0];
   else
      pVal = yesno[1];
   ssd_dialog_set_data("TwitterSendMunch", (void *) pVal);
}
#endif //IPHONE_NATIVE
