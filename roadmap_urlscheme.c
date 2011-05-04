/* roadmap_urlscheme.m - iPhone URL scheme handling
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
#include "Realtime/Realtime.h"
#include "roadmap_urlscheme.h"
#ifdef IPHONE
#include "roadmap_iphoneurlscheme.h"
#endif // IPHONE

#define URL_APP_PREFIX  	  "waze://?"

#define URL_QUERY_STR         "q="
#define URL_MAP_CENTER_STR    "ll="
#define URL_ZOOM_STR          "z="
#define URL_ACTION_STR        "a="

typedef struct  {

   char              query[URL_MAX_LENGTH];
   char              search_token[URL_MAX_LENGTH];		// Note: can be less
   char              action[URL_MAX_LENGTH];
   RoadMapPosition   map_center;
   RoadMapPosition   business_center;
   int               zoom;
   RoadMapPosition   saddr;
   RoadMapPosition   daddr;
} urlQuery_s;

static urlQuery_s    gs_Query;

#ifdef IPHONE
static NSURL *RoadMapURL = NULL;
#endif // IPHONE
static RoadMapCallback UrlNextLoginCb = NULL;






///////////////////////////////////////////////////////////////////////
static void init_query (void) {
   gs_Query.query[0] = 0;
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
   if (strstr(p, URL_ACTION_STR) == p) { //<q=query>
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
static void execute_query (void) {
   BOOL needs_redraw = FALSE;
   
   if (gs_Query.zoom >= 0) {
      roadmap_log(ROADMAP_DEBUG, "Executing zoom:%d", gs_Query.zoom);
      roadmap_math_zoom_set (gs_Query.zoom);
      roadmap_layer_adjust ();
      needs_redraw = TRUE;
   }
   
   if (gs_Query.map_center.latitude != -1 &&
       gs_Query.map_center.longitude != -1) {
      roadmap_log(ROADMAP_DEBUG, "Executing map center to lat:%d & lon:%d", gs_Query.map_center.latitude, gs_Query.map_center.longitude);
      roadmap_screen_hold();
      roadmap_screen_update_center(&gs_Query.map_center);
      needs_redraw = TRUE;
   }
   
   if (needs_redraw)
      roadmap_screen_redraw();
   
   if (gs_Query.search_token[0]!=0) {
      roadmap_log(ROADMAP_DEBUG, "Executing query: %s", gs_Query.search_token);
      single_search_auto_search(gs_Query.search_token);
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
}

///////////////////////////////////////////////////////////////////////
static void next_callback (void) {
   if (UrlNextLoginCb) {
      UrlNextLoginCb ();
      UrlNextLoginCb = NULL;
   }
}

#ifdef IPHONE

///////////////////////////////////////////////////////////////////////
void roadmap_url_init( const char* url ) 
{
   RoadMapURL = [[NSURL alloc] initWithString:[url absoluteString]];
   if (UrlNextLoginCb) {
      UrlNextLoginCb ();
      UrlNextLoginCb = NULL;
   }
}

///////////////////////////////////////////////////////////////////////
void roadmap_urlscheme (void){
   NSString *query = NULL;
   
   if (RoadMapURL) {
      query = [RoadMapURL query];
      if (!query) {
         roadmap_log (ROADMAP_WARNING, "Invalid URL query");
         next_callback();
         return;
      }
      
      query = [query stringByReplacingPercentEscapesUsingEncoding:NSUTF8StringEncoding]; 
      if (!query) {
         roadmap_log (ROADMAP_WARNING, "Invalid URL query");
         next_callback();
         return;
      }
      
      roadmap_log (ROADMAP_DEBUG, "URL query is: %s", [query UTF8String]);
      parse_query([query UTF8String]);
      execute_query();
   }
   
   next_callback();
}

BOOL roadmap_urlscheme_pending (void) {
   return (RoadMapURL != NULL);
}

///////////////////////////////////////////////////////////////////////
BOOL roadmap_urlscheme_handle (NSURL *url) {
   
   if ([[url scheme] isEqualToString:@"waze"]) {
      RoadMapURL = [[NSURL alloc] initWithString:[url absoluteString]];
      UrlNextLoginCb = Realtime_NotifyOnLogin (roadmap_urlscheme);
      return YES;
   }
   return NO;
}
#else

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
#endif // IPHONE


