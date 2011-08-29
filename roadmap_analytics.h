/* roadmap_analytics.h
 *
 * LICENSE:
 *
 *   Copyright 2010 Avi R.
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
 */


#ifndef ROADMAP_ANALYTICS_H_
#define ROADMAP_ANALYTICS_H_


#define STATS_MAX_ATTRS         5
#define STATS_MAX_STRING_SIZE   100

//Events
#define ANALYTICS_EVENT_NAVIGATE             "NAVIGATE"
#define ANALYTICS_EVENT_REROUTE              "REROUTE"

//GPS Events
#define ANALYTICS_EVENT_GPS_REC              "GPS_REC"
#define ANALYTICS_EVENT_GPS_LOST             "GPS_LOST"

// Search menu events
#define ANALYTICS_EVENT_SEARCHFAV            "SEARCH_FAV"
#define ANALYTICS_EVENT_SEARCHHISTORY        "SEARCH_HISTORY"
#define ANALYTICS_EVENT_SEARCHMARKED         "SEARCH_MARKED"
#define ANALYTICS_EVENT_SEARCHQUICK          "SEARCH_QUICK"
#define ANALYTICS_EVENT_SEARCHAB             "SEARCH_ADDRESS_BOOK"
#define ANALYTICS_EVENT_SEARCHMENU           "SEARCH_MENU"
#define ANALYTICS_EVENT_SEARCHADDR           "SEARCH_ADDRESS"
#define ANALYTICS_EVENT_SEARCHLOCAL          "SEARCH_LOCAL"

#define ANALYTICS_EVENT_ADR_SEARCH_SELCTED   "ADR_SEARCH_SELECTED"
#define ANALYTICS_EVENT_LS_SEARCH_SELCTED    "LS_SEARCH_SELECTED"
#define ANALYTICS_EVENT_ADR_SEARCH_BACK      "ADR_SEARCH_BACK"

#define ANALYTICS_EVENT_SEARCH_FAILED        "SEARCH_FAILED"
#define ANALYTICS_EVENT_SEARCH_NORES         "SEARCH_NO_RESULTS"
#define ANALYTICS_EVENT_SEARCH_SUCCESS       "SEARCH_SUCCESS"

#define ANALYTICS_EVENT_ROUTE_ERROR          "ROUTING_ERROR"
#define ANALYTICS_EVENT_ROUTE_RESULT         "ROUTING_RESULT"

#define ANALYTICS_EVENT_RT_ERROR             "REALTIME_ERROR"

// Settings events
#define ANALYTICS_EVENT_SETTINGS             "SETTINGS"

//QuickSettings events
#define ANALYTICS_EVENT_NAVGUIDANCE          "NAV_GUIDANCE"
#define ANALYTICS_EVENT_VIEWMODESET          "TOGGLE_VIEW"
#define ANALYTICS_EVENT_DAYNIGHTSET          "TOGGLE_DAY_NIGHT"

#define ANALYTICS_EVENT_ORIENTATION          "TOGGLE_ORIENTATION"

#define ANALYTICS_EVENT_CAR                  "TOGGLE_CAR_AVATAR"
#define ANALYTICS_EVENT_MOOD                 "TOGGLE_MOOD"
#define ANALYTICS_EVENT_ALLOWPING            "TOGGLE_ALLOW_PING"
#define ANALYTICS_EVENT_CHANGE_VIEW          "CHANGE_VIEW"

//Map settings events
#define ANALYTICS_EVENT_MAPSETTINGS          "MAP_SETTINGS"
#define ANALYTICS_EVENT_NIGHTMODESET         "TOGGLE_NIGHT_MODE"
#define ANALYTICS_EVENT_MAPCONTROLSSET       "TOGGLE_MAP_CONTROLS_ON_TAP"
#define ANALYTICS_EVENT_MAPSCHEME            "TOGGLE_MAP_SCHEME"

//Map controls event
#define ANALYTICS_EVENT_MAPCONTROL           "MAP_CONTROL"

//New user Events
#define ANALYTICS_EVENT_FIRST_USE            "FIRST_USE"
#define ANALYTICS_EVENT_NEW_USER             "NEW_USER"
#define ANALYTICS_EVENT_NEW_USER_OPTION      "NEW_USER_OPTION"
#define ANALYTICS_EVENT_NEW_USER_LOGIN       "NEW_USER_LOGIN"
#define ANALYTICS_EVENT_NEW_USER_SIGNUP      "NEW_USER_SIGNUP"

//Alerts Events
#define ANALYTICS_EVENT_SEND_ALERT          "SEND_ALERT"
#define ANALYTICS_EVENT_IMAGE_ALERT         "ALERT_WITH_IMAGE"
#define ANALYTICS_EVENT_VOICE_ALERT         "ALERT_WITH_VOICE"

#define ANALYTICS_EVENT_DOWNLOAD_ATTACHMENT    "D_ATTACHMENT"

//Ping Event
#define ANALYTICS_EVENT_PING_NAME           "PING_A_WAZER"

//Groups event
#define ANALYTICS_EVENT_GROUPS              "GROUPS"

//Scoreboard event
#define ANALYTICS_EVENT_SCOREBOARD          "SCOREBOARD"

#define ANALYTICS_EVENT_ALT_ROUTES         "ALT_ROUTES_BUTTON"

// Recommend events
#define  ANALYTICS_EVENT_RECOMMEND         "RECOMMEND"
#define ANALYTICS_EVENT_RECOMMENDAPPSTORE  "RECOMMEND_APPSTORE"
#define  ANALYTICS_EVENT_RECOMMENDCHOMP    "RECOMMEND_CHOMP"
#define  ANALYTICS_EVENT_RECOMMENDEMAIL    "RECOMMEND_EMAIL"


// Info types
#define ANALYTICS_EVENT_INFO_TIME        "TIME"
#define ANALYTICS_EVENT_INFO_TYPE        "TYPE"
#define ANALYTICS_EVENT_INFO_ACTION      "ACTION"
#define ANALYTICS_EVENT_INFO_SOURCE      "SOURCE"
#define ANALYTICS_EVENT_INFO_VALUE       "VAUE"
#define ANALYTICS_EVENT_INFO_CHANGED_TO  "CHANGED_TO"
#define ANALYTICS_EVENT_INFO_ERROR       "ERROR"
#define ANALYTICS_EVENT_INFO_NEW_MODE    "NEW_MODE"

#define ANALYTICS_EVENT_ON      "ON"
#define ANALYTICS_EVENT_OFF     "OFF"
#define ANALYTICS_EVENT_DAY     "DAY"
#define ANALYTICS_EVENT_NIGHT   "NIGHT"
#define ANALYTICS_EVENT_2D      "2D"
#define ANALYTICS_EVENT_3D      "3D"

#define ANALYTICS_EVENT_PORTRATE  "PORTRATE"
#define ANALYTICS_EVENT_LANDSCAPE "LANDSCAPE"

#define ANALYTICS_EVENT_NAV_GUIDANCE_NATURAL      "NATURAL"
#define ANALYTICS_EVENT_NAV_GUIDANCE_TTS          "TTS"

void roadmap_analytics_log_event (const char *event_name, const char *info_name, const char *info_val);
void roadmap_analytics_log_int_event (const char *event_name, const char *info_name, int info_val);

void roadmap_analytics_init(void);
void roadmap_analytics_term(void);

void roadmap_analytics_clear(void);
int  roadmap_analytics_count(void);
BOOL roadmap_analytics_is_empty(void);

#endif //ROADMAP_ANALYTICS_H_
