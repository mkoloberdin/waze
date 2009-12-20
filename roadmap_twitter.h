/* roadmap_twitter.h - Manages Twitter accont
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
 
#ifndef INCLUDE__ROADMAP_TWITTER__H
#define INCLUDE__ROADMAP_TWITTER__H


#define  TWITTER_USER_NAME_MAXSIZE          (128)
#define  TWITTER_USER_PASSWORD_MAXSIZE      (128)

#define  TWITTER_USER_NAME_MIN_SIZE     (2)
#define  TWITTER_PASSWORD_MIN_SIZE     (2)

#define  TWITTER_CONFIG_TYPE			    ("user")
#define  TWITTER_CONFIG_PREF_TYPE       ("preferences")
#define  TWITTER_CONFIG_TAB             ("Twitter")

//   Twitter User name
#define  TWITTER_CFG_PRM_NAME_Name          ("Name")
#define  TWITTER_CFG_PRM_NAME_Default       ("")

//   Twitter User password
#define  TWITTER_CFG_PRM_PASSWORD_Name      ("Password")
#define  TWITTER_CFG_PRM_PASSWORD_Default   ("")

//  Send reports to Twitter Enable / Disable:
#define  TWITTER_CFG_PRM_SEND_REPORTS_Name        ("Send Twitts")
#define  TWITTER_CFG_PRM_SEND_REPORTS_Enabled      ("Enabled")
#define  TWITTER_CFG_PRM_SEND_REPORTS_Disabled     ("Disabled")

//  Send destination to Twitter
#define  TWITTER_CFG_PRM_SEND_DESTINATION_Name       ("Send Destination")
#define  TWITTER_CFG_PRM_SEND_DESTINATION_Disabled   ("Disabled")
#define  TWITTER_CFG_PRM_SEND_DESTINATION_City       ("City")
#define  TWITTER_CFG_PRM_SEND_DESTINATION_Street     ("Street")
#define  TWITTER_CFG_PRM_SEND_DESTINATION_Full       ("Full")

//  Send road goodie munching to Twitter Enable / Disable:
#define  TWITTER_CFG_PRM_SEND_MUNCHING_Name       ("Send Munching")
#define  TWITTER_CFG_PRM_SEND_MUNCHING_Enabled    ("Enabled")
#define  TWITTER_CFG_PRM_SEND_MUNCHING_Disabled   ("Disabled")
#define  TWITTER_CFG_PRM_SHOW_MUNCHING_Name       ("Show Munching")
#define  TWITTER_CFG_PRM_SHOW_MUNCHING_No         ("no")
#define  TWITTER_CFG_PRM_SHOW_MUNCHING_Yes        ("yes")

// Logged in status
#define  TWITTER_CFG_PRM_LOGGED_IN_Name           ("Logged In")
#define  TWITTER_CFG_PRM_LOGGED_IN_No             ("no")
#define  TWITTER_CFG_PRM_LOGGED_IN_Yes            ("yes")


#define  TWITTER_DIALOG_NAME                      ("twitter_settings")
#define  TWITTER_TITTLE						           ("Twitter")

#define  ROADMAP_TWITTER_DESTINATION_MODE_DISABLED 0
#define  ROADMAP_TWITTER_DESTINATION_MODE_CITY     1
#define  ROADMAP_TWITTER_DESTINATION_MODE_STREET   2
#define  ROADMAP_TWITTER_DESTINATION_MODE_FULL     3
#define  ROADMAP_TWITTER_DESTINATION_MODES_COUNT   4

BOOL roadmap_twitter_initialize(void);

BOOL roadmap_twitter_is_sending_enabled(void);
void roadmap_twitter_enable_sending(void);
void roadmap_twitter_disable_sending(void);

const char *roadmap_twitter_get_username(void);
const char *roadmap_twitter_get_password(void);

void roadmap_twitter_set_username(const char *user_name);
void roadmap_twitter_set_password(const char *password);

void roadmap_twitter_setting_dialog(void);

int roadmap_twitter_destination_mode(void);
void roadmap_twitter_set_destination_mode(int mode);

BOOL roadmap_twitter_is_munching_enabled(void);
void roadmap_twitter_enable_munching();
void roadmap_twitter_disable_munching();
BOOL roadmap_twitter_is_show_munching(void);

void roadmap_twitter_login_failed(int status);
void roadmap_twitter_set_logged_in (BOOL is_logged_in);
BOOL roadmap_twitter_logged_in(void);


#endif /* INCLUDE__ROADMAP_TWITTER__H */
