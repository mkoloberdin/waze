/* roadmap_social.h - Manages social network (Twitter / Facebook) accounts
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
 
#ifndef INCLUDE__ROADMAP_SOCIAL__H
#define INCLUDE__ROADMAP_SOCIAL__H

/* the following seem to be used nowhere. If so we should remove. AR
#define  TWITTER_USER_NAME_MAXSIZE          (128)
#define  TWITTER_USER_PASSWORD_MAXSIZE      (128)

#define  TWITTER_USER_NAME_MIN_SIZE     (2)
#define  TWITTER_PASSWORD_MIN_SIZE     (2)
*/
#define  SOCIAL_CONFIG_TYPE               ("user")
#define  SOCIAL_CONFIG_PREF_TYPE          ("preferences")
#define  TWITTER_CONFIG_TAB               ("Twitter")
#define  FACEBOOK_CONFIG_TAB              ("Facebook")

//   Twitter User name
#define  SOCIAL_CFG_PRM_NAME_Name          ("Name")
#define  SOCIAL_CFG_PRM_NAME_Default       ("")

//   Twitter User password
#define  SOCIAL_CFG_PRM_PASSWORD_Name      ("Password")
#define  SOCIAL_CFG_PRM_PASSWORD_Default   ("")

//  Send reports to Twitter Enable / Disable:
#define  SOCIAL_CFG_PRM_SEND_REPORTS_Name        ("Send Twitts")
#define  SOCIAL_CFG_PRM_SEND_REPORTS_Enabled      ("Enabled")
#define  SOCIAL_CFG_PRM_SEND_REPORTS_Disabled     ("Disabled")

//  Send destination to Twitter
#define  SOCIAL_CFG_PRM_SEND_DESTINATION_Name       ("Send Destination")
#define  SOCIAL_CFG_PRM_SEND_DESTINATION_Disabled   ("Disabled")
#define  SOCIAL_CFG_PRM_SEND_DESTINATION_City       ("City")
#define  SOCIAL_CFG_PRM_SEND_DESTINATION_Street     ("Street")
#define  SOCIAL_CFG_PRM_SEND_DESTINATION_Full       ("Full")

//  Send road goodie munching to Twitter Enable / Disable:
#define  SOCIAL_CFG_PRM_SEND_MUNCHING_Name       ("Send Munching")
#define  SOCIAL_CFG_PRM_SEND_MUNCHING_Enabled    ("Enabled")
#define  SOCIAL_CFG_PRM_SEND_MUNCHING_Disabled   ("Disabled")
#define  SOCIAL_CFG_PRM_SHOW_MUNCHING_Name       ("Show Munching")
#define  SOCIAL_CFG_PRM_SHOW_MUNCHING_No         ("no")
#define  SOCIAL_CFG_PRM_SHOW_MUNCHING_Yes        ("yes")

//  Post about sign up Enable / Disable:
#define  SOCIAL_CFG_PRM_SEND_SIGNUP_Name        ("Publish Signup")
#define  SOCIAL_CFG_PRM_SEND_SIGNUP_Enabled     ("Enabled")
#define  SOCIAL_CFG_PRM_SEND_SIGNUP_Disabled    ("Disabled")

//  Show user name Enable / Disable:
#define  SOCIAL_CFG_PRM_SHOW_NAME_Name          ("Show Name")
#define  SOCIAL_CFG_PRM_SHOW_NAME_Enabled       ("Enabled")
#define  SOCIAL_CFG_PRM_SHOW_NAME_Friends       ("Friends")
#define  SOCIAL_CFG_PRM_SHOW_NAME_Disabled      ("Disabled")

//  Show user picture Enable / Disable:
#define  SOCIAL_CFG_PRM_SHOW_PICTURE_Name       ("Show Picture")
#define  SOCIAL_CFG_PRM_SHOW_PICTURE_Enabled    ("Enabled")
#define  SOCIAL_CFG_PRM_SHOW_PICTURE_Friends    ("Friends")
#define  SOCIAL_CFG_PRM_SHOW_PICTURE_Disabled   ("Disabled")

//  Show user picture Enable / Disable:
#define  SOCIAL_CFG_PRM_SHOW_PROFILE_Name       ("Show Profile")
#define  SOCIAL_CFG_PRM_SHOW_PROFILE_Enabled    ("Enabled")
#define  SOCIAL_CFG_PRM_SHOW_PROFILE_Friends    ("Friends")
#define  SOCIAL_CFG_PRM_SHOW_PROFILE_Disabled   ("Disabled")

// Logged in status
#define  SOCIAL_CFG_PRM_LOGGED_IN_Name          ("Logged In")
#define  SOCIAL_CFG_PRM_LOGGED_IN_No            ("no")
#define  SOCIAL_CFG_PRM_LOGGED_IN_Yes           ("yes")

// Facebook URLs
#define  FACEBOOK_CFG_PRM_URL_Name                 ("Url")
#define  FACEBOOK_CFG_PRM_URL_Default              ("")
#define  FACEBOOK_CONNECT_SUFFIX                   ("/facebook/connect") //TODO: change back to this
//#define  FACEBOOK_CONNECT_SUFFIX                   ("/facebook/oauth2_connect")
#define  FACEBOOK_DISCONNECT_SUFFIX                ("/WAS/facebook_disconnect") //TODO: change back to this
//#define  FACEBOOK_DISCONNECT_SUFFIX                ("/WAS/facebook_disconnect_oauth2")
#define  FACEBOOK_IS_CONNECTED_SUFFIX              ("/WAS/facebook_is_connected") //TODO: change back to this
//#define  FACEBOOK_IS_CONNECTED_SUFFIX              ("/WAS/facebook_is_connected_oauth2")
#define  FACEBOOK_SHARE_SUFFIX                     ("/facebook/share")


#define  TWITTER_DIALOG_NAME                      ("twitter_settings")
#define  TWITTER_TITTLE						           ("Twitter")
#define  FACEBOOK_DIALOG_NAME                     ("facebook_settings")
#define  FACEBOOK_TITTLE                          ("Facebook")

#define  ROADMAP_SOCIAL_DESTINATION_MODE_DISABLED 0
#define  ROADMAP_SOCIAL_DESTINATION_MODE_CITY     1
#define  ROADMAP_SOCIAL_DESTINATION_MODE_STREET   2
#define  ROADMAP_SOCIAL_DESTINATION_MODE_FULL     3
#define  ROADMAP_SOCIAL_DESTINATION_MODES_COUNT   4

#define  ROADMAP_SOCIAL_SHOW_DETAILS_MODE_DISABLED 0
#define  ROADMAP_SOCIAL_SHOW_DETAILS_MODE_FRIENDS  1
#define  ROADMAP_SOCIAL_SHOW_DETAILS_MODE_ENABLED  2
#define  ROADMAP_SOCIAL_SHOW_DETAILS_MODES_COUNT   3

BOOL roadmap_social_initialize(void);

BOOL roadmap_twitter_is_sending_enabled(void);
void roadmap_twitter_enable_sending(void);
void roadmap_twitter_disable_sending(void);
BOOL roadmap_facebook_is_sending_enabled(void);
void roadmap_facebook_enable_sending(void);
void roadmap_facebook_disable_sending(void);

const char *roadmap_twitter_get_username(void);
const char *roadmap_twitter_get_password(void);

void roadmap_twitter_set_username(const char *user_name);
void roadmap_twitter_set_password(const char *password);

void roadmap_twitter_setting_dialog(void);
void roadmap_facebook_setting_dialog(void);

int roadmap_twitter_destination_mode(void);
void roadmap_twitter_set_destination_mode(int mode);
int roadmap_facebook_destination_mode(void);
void roadmap_facebook_set_destination_mode(int mode);

BOOL roadmap_twitter_is_munching_enabled(void);
void roadmap_twitter_enable_munching();
void roadmap_twitter_disable_munching();
BOOL roadmap_twitter_is_show_munching(void);
BOOL roadmap_twitter_is_signup_enabled(void);
void roadmap_twitter_set_signup(BOOL enabled);
int roadmap_twitter_get_show_profile(void);
void roadmap_twitter_set_show_profile(int mode);
BOOL roadmap_facebook_is_munching_enabled(void);
void roadmap_facebook_enable_munching();
void roadmap_facebook_disable_munching();
int roadmap_facebook_get_show_name(void);
void roadmap_facebook_set_show_name(int mode);
int roadmap_facebook_get_show_picture(void);
void roadmap_facebook_set_show_picture(int mode);
int roadmap_facebook_get_show_profile(void);
void roadmap_facebook_set_show_profile(int mode);

void roadmap_twitter_login_failed(int status);
void roadmap_twitter_set_logged_in (BOOL is_logged_in);
BOOL roadmap_twitter_logged_in(void);
BOOL roadmap_facebook_logged_in(void);
void roadmap_facebook_check_login(void);
void roadmap_facebook_connect(BOOL preload);
void roadmap_facebook_disconnect(void);
void roadmap_facebook_refresh_connection (void);
void roadmap_facebook_invite(void);
void roadmap_social_send_permissions (void);


#endif /* INCLUDE__ROADMAP_SOCIAL__H */
