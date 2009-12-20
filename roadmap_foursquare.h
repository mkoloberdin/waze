/* roadmap_foursquare.h - Manages Foursquare account
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
 
#ifndef INCLUDE__ROADMAP_FOURSQUARE__H
#define INCLUDE__ROADMAP_FOURSQUARE__H

#include "ssd/ssd_widget.h"

#define  FOURSQUARE_USER_NAME_MAXSIZE           (128)
#define  FOURSQUARE_USER_PASSWORD_MAXSIZE       (128)

#define  FOURSQUARE_USER_NAME_MIN_SIZE          (2)
#define  FOURSQUARE_PASSWORD_MIN_SIZE           (2)

#define  FOURSQUARE_CONFIG_TYPE			         ("user")
#define  FOURSQUARE_CONFIG_PREF_TYPE            ("preferences")
#define  FOURSQUARE_CONFIG_TAB                  ("Foursquare")

//   Foursquare User name
#define  FOURSQUARE_CFG_PRM_NAME_Name           ("Name")
#define  FOURSQUARE_CFG_PRM_NAME_Default        ("")

//   Foursquare User password
#define  FOURSQUARE_CFG_PRM_PASSWORD_Name       ("Password")
#define  FOURSQUARE_CFG_PRM_PASSWORD_Default    ("")

// Logged in status
#define  FOURSQUARE_CFG_PRM_LOGGED_IN_Name      ("Logged In")
#define  FOURSQUARE_CFG_PRM_LOGGED_IN_No        ("no")
#define  FOURSQUARE_CFG_PRM_LOGGED_IN_Yes       ("yes")

// Foursquare activated
#define  FOURSQUARE_CFG_PRM_ACTIVATED_Name      ("Activated")
#define  FOURSQUARE_CFG_PRM_ACTIVATED_No        ("no")
#define  FOURSQUARE_CFG_PRM_ACTIVATED_Yes       ("yes")


#define  FOURSQUARE_LOGIN_DIALOG_NAME           ("foursquare_login")
#define  FOURSQUARE_TITLE						      ("Foursquare")
#define  FOURSQUARE_VENUES_TITLE                ("Foursquare - Nearby")
#define  FOURSQUARE_CHECKEDIN_DIALOG_NAME       ("foursquare_checkedin")
#define  FOURSQUARE_CHECKEDIN_TITLE             ("Foursquare - Check-in")



#define     ROADMAP_FOURSQUARE_ID_MAX_SIZE            10
#define     ROADMAP_FOURSQUARE_NAME_MAX_SIZE          100
#define     ROADMAP_FOURSQUARE_ADDRESS_MAX_SIZE       200
#define     ROADMAP_FOURSQUARE_CROSS_STREET_MAX_SIZE  100
#define     ROADMAP_FOURSQUARE_CITY_MAX_SIZE          100
#define     ROADMAP_FOURSQUARE_STATE_MAX_SIZE         50
#define     ROADMAP_FOURSQUARE_PHONE_MAX_SIZE         50
#define     ROADMAP_FOURSQUARE_MESSAGE_MAX_SIZE       400
#define     ROADMAP_FOURSQUARE_SCORE_PT_MAX_SIZE      10


#define     ROADMAP_FOURSQUARE_VENUE_ENTRIES          11
typedef struct tagFoursquareVenue {
   char                       sId[ROADMAP_FOURSQUARE_ID_MAX_SIZE +1];
   char                       sName[ROADMAP_FOURSQUARE_NAME_MAX_SIZE +1];
   char                       sAddress[ROADMAP_FOURSQUARE_ADDRESS_MAX_SIZE +1];
   char                       sCrossStreet[ROADMAP_FOURSQUARE_CROSS_STREET_MAX_SIZE +1];
   char                       sCity[ROADMAP_FOURSQUARE_CITY_MAX_SIZE +1];
   char                       sState[ROADMAP_FOURSQUARE_STATE_MAX_SIZE +1];
   int                        iZip;
   int                        iLatitude;
   int                        iLongitude;
   char                       sPhone[ROADMAP_FOURSQUARE_PHONE_MAX_SIZE +1];
   int                        iDistance;
   //Following description is created from the above entries
   char                       sDescription[ROADMAP_FOURSQUARE_NAME_MAX_SIZE + 1 + ROADMAP_FOURSQUARE_ADDRESS_MAX_SIZE + 1 + ROADMAP_FOURSQUARE_CITY_MAX_SIZE + 1];
}  FoursquareVenue;

#define     ROADMAP_FOURSQUARE_CHECKIN_ENTRIES        2
typedef struct tagFoursquareCheckin {
   char                       sCheckinMessage[ROADMAP_FOURSQUARE_MESSAGE_MAX_SIZE +1];
   char                       sScorePoints[ROADMAP_FOURSQUARE_SCORE_PT_MAX_SIZE +1];

   //Following address is created from the selected venue entries
   char                       sAddress[ROADMAP_FOURSQUARE_ADDRESS_MAX_SIZE + 1 + ROADMAP_FOURSQUARE_CITY_MAX_SIZE + 1];
}  FoursquareCheckin;


BOOL roadmap_foursquare_initialize(void);

const char *roadmap_foursquare_get_username(void);
const char *roadmap_foursquare_get_password(void);

void roadmap_foursquare_set_username(const char *user_name);
void roadmap_foursquare_set_password(const char *password);

void roadmap_foursquare_login_dialog(void);
void roadmap_foursquare_checkedin_dialog(void);

BOOL roadmap_foursquare_is_enabled(void);

void roadmap_foursquare_request_failed (roadmap_result status);
void roadmap_foursquare_set_logged_in (BOOL is_logged_in);
BOOL roadmap_foursquare_logged_in(void);
void roadmap_foursquare_checkin(void);
const char* roadmap_foursquare_response(int status, roadmap_result* rc, int NumParams, const char*  pData);
void roadmap_foursquare_login (const char *user_name, const char *password);
void roadmap_foursquare_get_checkin_info (FoursquareCheckin *outCheckInInfo);
SsdWidget roadmap_foursquare_create_alert_menu(void);


#endif /* INCLUDE__ROADMAP_FOURSQUARE__H */
