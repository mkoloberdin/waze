/* roadmap_urlscheme.c - URL scheme handling
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi R
 *   Copyright 2009, Waze Ltd
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

#include <math.h>
#include "roadmap.h"
#include "roadmap_screen.h"
#include "roadmap_math.h"
#include "roadmap_start.h"
#include "roadmap_layer.h"
#include "address_search/single_search_dlg.h"
#include "address_search/address_search_dlg.h"
#include "Realtime/Realtime.h"
#include "roadmap_street.h"
#include "roadmap_trip.h"
#include "roadmap_lang.h"
#include "ssd/ssd_popup.h"
#include "roadmap_search.h"
#include "navigate/navigate_route.h"
#include "roadmap_urlscheme.h"

#define URL_APP_PREFIX              "waze://?"

#define URL_QUERY_STR               "q="
#define URL_MAP_CENTER_STR          "ll="
#define URL_ZOOM_STR                "z="
#define URL_FAVORITE_STR            "favorite="
#define URL_ACTION_STR              "a="
#define URL_START_NAVIGATE_STR      "navigate=yes"

typedef struct  {

   char              query[URL_MAX_LENGTH];
   char              search_token[URL_MAX_LENGTH];		// Note: can be less 
   char              favorite_token[URL_MAX_LENGTH];
   char              action[URL_MAX_LENGTH];
   RoadMapPosition   map_center;
   RoadMapPosition   business_center;
   int               zoom;
   RoadMapPosition   saddr;
   RoadMapPosition   daddr;
   BOOL              start_navigate;
} urlQuery_s;

static urlQuery_s    gs_Query;
static RoadMapCallback UrlNextLoginCb = NULL;






///////////////////////////////////////////////////////////////////////
static void init_query (void) {
   gs_Query.query[0] = 0;
   gs_Query.search_token[0] = 0;
   gs_Query.favorite_token[0] = 0;
   gs_Query.action[0]= 0;
   gs_Query.map_center.latitude = -1;
   gs_Query.map_center.longitude = -1;
   gs_Query.business_center.latitude = -1;
   gs_Query.business_center.longitude = -1;
   gs_Query.zoom = -1;
   gs_Query.saddr.latitude = -1;
   gs_Query.saddr.longitude = -1;
   gs_Query.daddr.latitude = -1;
   gs_Query.daddr.longitude = -1;
   gs_Query.start_navigate = FALSE;
}

///////////////////////////////////////////////////////////////////////
static void decode (char *txt) {
   char p[URL_MAX_LENGTH];

   strncpy_safe(p, txt, sizeof(p));

   roadmap_log(ROADMAP_DEBUG, "parsing: %s", p);

   //address query
   if (strstr(p, URL_QUERY_STR) == p) { //<q=query>
      char *query = p + strlen(URL_QUERY_STR);
      strncpy_safe(gs_Query.search_token, query, sizeof(gs_Query.search_token));
      return;
   }

   //Action
   if (strstr(p, URL_ACTION_STR) == p) { //<a=action>
      char *action = p + strlen(URL_ACTION_STR);
      strncpy_safe(gs_Query.action, action, sizeof(gs_Query.action));
      return;
   }

   //map center
   if (strstr(p, URL_MAP_CENTER_STR) == p) { //<ll=lat,lon>
      char *lat;
      char *lon;
      int comma_pos = strcspn(p, ",");
      if (comma_pos < strlen(p)) {
         p[comma_pos] = 0;
         lat = p + strlen(URL_MAP_CENTER_STR);
         if (lat) {
            roadmap_log(ROADMAP_DEBUG, "decode: valid lat %s", lat);
            lon = lat + strlen(lat) + 1;
            if (lon) {
               roadmap_log(ROADMAP_DEBUG, "decode: valid lon %s", lon);
               gs_Query.map_center.latitude = floor(atof(lat) * 1000000);
               gs_Query.map_center.longitude = floor(atof(lon) * 1000000);
            }
         }
      }
      return;
   }

   //zoom
   if (strstr(p, URL_ZOOM_STR) == p) { //<z=zoom>
      char *zoom;
      zoom = p + strlen(URL_ZOOM_STR);
      if (zoom)
         roadmap_log(ROADMAP_DEBUG, "decode: valid zoom %s", zoom);
         gs_Query.zoom = atoi(zoom);
      return;
   }

   // favorite
   if ( strstr( p, URL_FAVORITE_STR ) == p)
   {
      char *query = p + strlen(URL_FAVORITE_STR);
      strncpy_safe( gs_Query.favorite_token, query, sizeof(gs_Query.favorite_token));
      return;
   }
   
   // start navigation
   if (strstr(p, URL_START_NAVIGATE_STR) == p) { //navigate=yes
      gs_Query.start_navigate = TRUE;
   }
}


///////////////////////////////////////////////////////////////////////
static void parse_query ( void ) {
   char *p;

   char * const query = strdup( gs_Query.query );

   p = strtok( query, "&" );

   while (p != NULL) {
      decode (p);
      p = strtok (NULL, "&");
   }

   free( query );
}

///////////////////////////////////////////////////////////////////////
static char const *PopupMenuItems[] = {
   
   "setasdestination",
   "setasdeparture",
   NULL,
};

//////////////////////////////////////////////////////////////////////////////////////////////////
static int on_next (SsdWidget widget, const char *new_value){
   ssd_dialog_hide_current(dec_close);
   roadmap_start_popup_menu ("Show External Pos menu", PopupMenuItems,
                             NULL, NULL, DIALOG_ANIMATION_FROM_RIGHT);
   
   return 1;
}

///////////////////////////////////////////////////////////////////////
static void show_position_popup (void) {
   int number;
   int layers[128];
   int layers_count;
   RoadMapNeighbour neighbours[2];
   RoadMapPosition context_save_pos;
   PluginStreetProperties properties;
   char title[250];
   zoom_t context_save_zoom;
   
   roadmap_main_remove_periodic(show_position_popup);
   
   roadmap_trip_set_point ("Selection", &gs_Query.map_center);
   
   layers_count = roadmap_layer_all_roads(layers, 128);
   roadmap_math_get_context (&context_save_pos, &context_save_zoom);
   roadmap_math_set_context(&gs_Query.map_center, 20);
   number = roadmap_street_get_closest(&gs_Query.map_center, 0, layers, layers_count,
                                       1, &neighbours[0], 1);
   roadmap_math_set_context (&context_save_pos, context_save_zoom);
   if (number <= 0) {
      strncpy_safe (title, roadmap_lang_get("External point"), sizeof(title));
      properties.city = "";
   } else {
      roadmap_plugin_get_street_properties (&neighbours[0].line, &properties, 0);
      
      if (ssd_widget_rtl(NULL)){
         snprintf (title, sizeof(title), "%s %s", properties.street, properties.address);
      }else{
         snprintf (title, sizeof(title), "%s %s", properties.address, properties.street);
      }
   }
   
   ssd_popup_show_float("SelectDlg",
                        properties.city,
                        NULL,
                        title,
                        NULL,
                        "marked_location",
                        &gs_Query.map_center,
                        ADJ_SCALE(-18),
                        NULL,
                        on_next,
                        "Options",
                        NULL);
}

///////////////////////////////////////////////////////////////////////
static void move_screen (void) {
   roadmap_main_remove_periodic(move_screen);
   
   roadmap_screen_hold();
   roadmap_screen_update_center_animated(&gs_Query.map_center, 800, 0);
   
   roadmap_main_set_periodic(1000, show_position_popup);
   
}

///////////////////////////////////////////////////////////////////////
static void execute_query (void) {
   BOOL needs_redraw = FALSE;

   ssd_dialog_hide_all(dec_cancel);
#ifdef IPHONE_NATIVE
   roadmap_main_show_root(0);
#endif
   
   if (gs_Query.zoom >= 0) {
      roadmap_log(ROADMAP_DEBUG, "Executing zoom:%d", gs_Query.zoom);
      roadmap_math_zoom_set (gs_Query.zoom);
      roadmap_layer_adjust ();
      needs_redraw = TRUE;
   }

   if (gs_Query.map_center.latitude != -1 &&
       gs_Query.map_center.longitude != -1) {
      if (!gs_Query.start_navigate) {
         roadmap_log(ROADMAP_DEBUG, "Executing map center to lat:%d & lon:%d", gs_Query.map_center.latitude, gs_Query.map_center.longitude);
         
         roadmap_main_set_periodic(2000, move_screen);
         
         needs_redraw = TRUE;
      } else {
         roadmap_log(ROADMAP_DEBUG, "Executing navigation to lat:%d & lon:%d", gs_Query.map_center.latitude, gs_Query.map_center.longitude);
         roadmap_trip_set_point ("Destination", &gs_Query.map_center );
         navigate_main_route ( NAV_ROUTE_FLAGS_NONE );
      }

   }

   if (needs_redraw)
      roadmap_screen_redraw();

   if (gs_Query.search_token[0]!=0) {
      roadmap_log(ROADMAP_DEBUG, "Executing query: %s", gs_Query.search_token);
      if (!gs_Query.start_navigate) {
         single_search_auto_search(gs_Query.search_token);
      } else {
         address_search_auto_nav(gs_Query.search_token);
      }

      return;
   }
   if (gs_Query.action[0]!=0) {
        RoadMapAction * action;
        roadmap_log(ROADMAP_DEBUG, "Executing action: %s", gs_Query.action);
        action = roadmap_start_find_action_un_const(gs_Query.action);
        if (action){
              action->callback();
        }
        else{
              roadmap_log (ROADMAP_ERROR, "Invalid action query %s", gs_Query.action);
        }
        return;
   }

   if ( gs_Query.favorite_token[0] != 0 ) {
      roadmap_log( ROADMAP_DEBUG, "Navigating to favorite: %s", gs_Query.favorite_token );
      roadmap_search_navigate_to_favorite( gs_Query.favorite_token );
   }
}

///////////////////////////////////////////////////////////////////////
static void next_callback (void) {
   if (UrlNextLoginCb) {
      UrlNextLoginCb ();
      UrlNextLoginCb = NULL;
   }
}

///////////////////////////////////////////////////////////////////////
// Checks if the initial url is a valid waze url
//
BOOL roadmap_urlscheme_valid ( const char* raw_url )
{
	return !strncmp( raw_url, URL_APP_PREFIX, strlen( URL_APP_PREFIX ) );
}

///////////////////////////////////////////////////////////////////////
// Checks if the initial url is a valid waze url
//
void roadmap_urlscheme_remove_prefix( char* dst_url, const char *src_url )
{
	int trgt_len = strlen( src_url ) - strlen( URL_APP_PREFIX ) + 1;
	strncpy_safe( dst_url, src_url + strlen( URL_APP_PREFIX ), trgt_len );
}

///////////////////////////////////////////////////////////////////////
// NOTE:: !!! url must be with the decoded escapes !!!!
//
// Use platforms API-s to accomplish this operation
//
//
BOOL roadmap_urlscheme_init ( const char* url_decoded_escapes )
{
	BOOL res = FALSE;
	roadmap_log( ROADMAP_WARNING, "Application is initialized with the URL: %s", url_decoded_escapes );

	init_query();

	if ( url_decoded_escapes )
	{
	   strncpy_safe( gs_Query.query, url_decoded_escapes, strlen( url_decoded_escapes ) + 1 );
		UrlNextLoginCb = Realtime_NotifyOnLogin ( roadmap_urlscheme );
		res = TRUE;
	}
	else
	{
		roadmap_log( ROADMAP_WARNING, "Invalid URL" );
	}
	return res;
}

///////////////////////////////////////////////////////////////////////
BOOL roadmap_urlscheme_pending (void) {
   return (gs_Query.query[0] != 0 );
}

///////////////////////////////////////////////////////////////////////
void roadmap_urlscheme_reset (void) {
   init_query();
}

///////////////////////////////////////////////////////////////////////
void roadmap_urlscheme (void)
{
      parse_query();
      execute_query();
      next_callback();
}


