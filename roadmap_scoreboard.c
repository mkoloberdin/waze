/* roadmap_scoreboard.c
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi R.
 *   Copyright 2009, Waze Ltd
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


#include "roadmap.h"
#include "roadmap_config.h"
#include "roadmap_bar.h"
#include "roadmap_messagebox.h"
#include "Realtime/Realtime.h"
#include "roadmap_browser.h"
#include "roadmap_canvas.h"
#include "roadmap_social.h"
#include "roadmap_lang.h"
#include "roadmap_scoreboard.h"
#include "roadmap_analytics.h"
#include "roadmap_start.h"



static RoadMapConfigDescriptor RoadMapConfigScoreboardFeatureEnabled =
   ROADMAP_CONFIG_ITEM("Scoreboard", "Feature enabled");

static RoadMapConfigDescriptor RoadMapConfigScoreboardUrl =
   ROADMAP_CONFIG_ITEM("Scoreboard", "Url");


static char gs_url[1024];

//Scoreboard event
static const char* ANALYTICS_EVENT_SCOREBOARD_NAME = "SCOREBOARD";


///////////////////////////////////////////////////////////////
void roadmap_scoreboard_request_failed (roadmap_result rc) {
   //Not implemented for web scoreboard
}

///////////////////////////////////////////////////////////////
const char* roadmap_scoreboard_response(int status, roadmap_result* rc, int NumParams, const char*  pData){
   //Not implemented for web scoreboard
   return pData;
}

///////////////////////////////////////////////////////////////
BOOL roadmap_scoreboard_feature_enabled (void) {
   if (0 == strcmp (roadmap_config_get (&RoadMapConfigScoreboardFeatureEnabled), "yes")){
      return TRUE;
   }

   return FALSE;
}

///////////////////////////////////////////////////////////////
static void create_url(void) {
   snprintf(gs_url, sizeof(gs_url),"%s?sessionid=%d&cookie=%s&deviceid=%d&width=%d&height=%d&lang=%s&client_version=%s&web_version=%s",
            roadmap_config_get(&RoadMapConfigScoreboardUrl),
            Realtime_GetServerId(),
            Realtime_GetServerCookie(),
            RT_DEVICE_ID,
            roadmap_canvas_width(),
            roadmap_canvas_height() - roadmap_bar_bottom_height(),
            roadmap_lang_get_system_lang(),
            roadmap_start_version(),
            BROWSER_WEB_VERSION);
}

///////////////////////////////////////////////////////////////
static void roadmap_scoreboard_init(void) {
   roadmap_config_declare_enumeration ("preferences", &RoadMapConfigScoreboardFeatureEnabled, NULL, "no",
                                       "yes", NULL);

   roadmap_config_declare_enumeration ("preferences", &RoadMapConfigScoreboardUrl, NULL,
                                       "http://www.waze.com/WAS/mvc/scoreboard", NULL);

   gs_url[0] = 0;
}

///////////////////////////////////////////////////////////////
void roadmap_scoreboard(void) {
   static BOOL initialized = FALSE;

   if (!initialized) {
      roadmap_scoreboard_init();
      initialized = TRUE;
   }

   if (!roadmap_scoreboard_feature_enabled()) {
      roadmap_messagebox_timeout("Info", "Scoreboard is currently not available in your area", 5);
      return;
   }

   roadmap_analytics_log_event(ANALYTICS_EVENT_SCOREBOARD_NAME, NULL, NULL);

   create_url();

   roadmap_browser_show( "Scoreboard", gs_url, roadmap_facebook_check_login, BROWSER_BAR_NORMAL );
}
