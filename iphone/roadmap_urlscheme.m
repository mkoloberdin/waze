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

#include "roadmap.h"
#include "roadmap_screen.h"
#include "roadmap_math.h"
#include "roadmap_layer.h"
#include "address_search_dlg.h"
#include "Realtime.h"
#include "roadmap_urlscheme.h"
#include "roadmap_iphoneurlscheme.h"



#define URL_MAX_LENGTH        256
#define URL_QUERY_STR         "q="
#define URL_MAP_CENTER_STR    "ll="
#define URL_ZOOM_STR          "z="


typedef struct  {
   char              query[URL_MAX_LENGTH];
   RoadMapPosition   map_center;
   RoadMapPosition   business_center;
   int               zoom;
   RoadMapPosition   saddr;
   RoadMapPosition   daddr;
} urlQuery_s;

static urlQuery_s    gs_Query;

static NSURL *RoadMapURL = NULL;
static RoadMapCallback UrlNextLoginCb = NULL;






///////////////////////////////////////////////////////////////////////
static void init_query (void) {
   gs_Query.query[0] = 0;
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
      strncpy_safe(gs_Query.query, query, sizeof(gs_Query.query));
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
static void parse_query (const char* url_query) {
   char *p;
   char *query = strdup(url_query);

   init_query();

   p = strtok(query, "&");
   
   while (p != NULL) {
      decode (p);
      p = strtok (NULL, "&");
   }
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
   
   if (gs_Query.query[0]!=0) {
      roadmap_log(ROADMAP_DEBUG, "Executing query: %s", gs_Query.query);
      address_search_auto_search (gs_Query.query);
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