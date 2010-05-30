/* roadmap_view.c - controls the different view modes
 *
 * LICENSE:
 *
 *   Copyright 2008 Ehud Shabtai
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; version 2 of the License.
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
 * SYNOPSYS:
 *
 *   See roadmap_view.h
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "roadmap.h"
#include "roadmap_trip.h"
#include "roadmap_math.h"
#include "roadmap_screen.h"
#include "roadmap_main.h"
#include "roadmap_alternative_routes.h"
#include "navigate/navigate_main.h"
#include "navigate/navigate_zoom.h"

#include "roadmap_view.h"

#define VIEW_COMMUTE     1
#define VIEW_NAVIGATION  2

#define COMMUTE_SCALE 7000

#define AUTO_ZOOM_SUSPEND_PERIOD 30
#define SHOW_ROUTE_TIME 8

typedef enum {
   VIEW_STATE_NORMAL,
   VIEW_STATE_SHOW_ROUTE,
   VIEW_STATE_SHOW_ALT_ROUTE,
   VIEW_STATE_NAVIGATE
} ViewState;

static RoadMapPosition RoadMapViewWayPoint;
static int RoadMapViewLastOrientation = -1;
static int RoadMapViewDistance;
static int RoadMapViewAzymuth;
static int RoadMapViewMode = VIEW_NAVIGATION;
static int AutoZoomSuspended = 0;
static int RoadMapViewIsGpsFocus = 0;
static ViewState RoadMapViewState = VIEW_STATE_NORMAL;
static time_t RoadMapViewStateTime;

static void set_state (ViewState state) {
   if (RoadMapViewState != state) {
      RoadMapViewState = state;
      RoadMapViewStateTime = time(NULL);
   }
}

void roadmap_view_refresh (void) {
   const RoadMapPosition *pos;

   const char *focus = roadmap_trip_get_focus_name();
   RoadMapViewIsGpsFocus = focus && !strcmp(focus, "GPS");

   if (!navigate_main_alt_routes_display()){
      if (!navigate_track_enabled() && !navigate_offtrack())  {
         set_state(VIEW_STATE_NORMAL);
         return;
      }

      if (navigate_offtrack()) return;

      if (!navigate_is_auto_zoom()) {
         return;
      }
   }

   if (RoadMapViewState == VIEW_STATE_NORMAL) {
      roadmap_view_reset();
      if (navigate_main_alt_routes_display())
         set_state(VIEW_STATE_SHOW_ALT_ROUTE);
      else
         set_state(VIEW_STATE_SHOW_ROUTE);
      roadmap_screen_set_orientation_fixed();
   } else if (RoadMapViewState == VIEW_STATE_SHOW_ROUTE)  {
      if (navigate_main_alt_routes_display()){
               set_state(VIEW_STATE_SHOW_ALT_ROUTE);
               roadmap_screen_set_orientation_fixed();
      }
      else if (navigate_main_is_alt_routes()){
         set_state(VIEW_STATE_NAVIGATE);
         roadmap_screen_set_orientation_dynamic();
      }
      else if ((time(NULL) - RoadMapViewStateTime) > SHOW_ROUTE_TIME) {
         set_state(VIEW_STATE_NAVIGATE);
         roadmap_screen_set_orientation_dynamic();
      }
   } else if (RoadMapViewState == VIEW_STATE_SHOW_ALT_ROUTE)  {
      if (!navigate_main_alt_routes_display()){
         set_state(VIEW_STATE_NORMAL);
         roadmap_screen_set_orientation_dynamic();
      }
      roadmap_screen_set_orientation_fixed();
   }


   pos = roadmap_trip_get_focus_position ();

   if (RoadMapViewState == VIEW_STATE_SHOW_ROUTE) {

      navigate_get_waypoint (-1, &RoadMapViewWayPoint);
      RoadMapViewDistance =
         roadmap_math_distance(pos, &RoadMapViewWayPoint) * 9 / 4;
      RoadMapViewAzymuth = 360 - roadmap_math_azymuth(pos, &RoadMapViewWayPoint);
		RoadMapViewWayPoint.longitude = (RoadMapViewWayPoint.longitude + pos->longitude) / 2;
		RoadMapViewWayPoint.latitude = (RoadMapViewWayPoint.latitude + pos->latitude) / 2;
      roadmap_screen_move_center (100);
		roadmap_screen_update_center(&RoadMapViewWayPoint);
      return;
   }
   if (RoadMapViewState == VIEW_STATE_SHOW_ALT_ROUTE) {
      roadmap_alternative_route_get_src((RoadMapPosition *)pos);
      roadmap_alternative_route_get_waypoint (-1, &RoadMapViewWayPoint);
         RoadMapViewDistance =

            roadmap_math_distance(pos, &RoadMapViewWayPoint) * 7 / 4;
       RoadMapViewAzymuth = 360 - roadmap_math_azymuth(pos, &RoadMapViewWayPoint);
       RoadMapViewWayPoint.longitude = (RoadMapViewWayPoint.longitude + pos->longitude) / 2;
       RoadMapViewWayPoint.latitude = (RoadMapViewWayPoint.latitude + pos->latitude) / 2;
       roadmap_screen_move_center (roadmap_canvas_height()/3);
       roadmap_screen_move_center (50);
       roadmap_screen_update_center(&RoadMapViewWayPoint);
       return;
   }

   if (RoadMapViewMode == VIEW_COMMUTE) return;

   if (navigate_main_alt_routes_display())
      roadmap_alternative_route_get_waypoint (-1, &RoadMapViewWayPoint);
   else
      navigate_get_waypoint (25000, &RoadMapViewWayPoint);

   RoadMapViewDistance = roadmap_math_distance(pos, &RoadMapViewWayPoint);

   RoadMapViewAzymuth = 360 - roadmap_math_azymuth(pos, &RoadMapViewWayPoint);
}

int roadmap_view_get_orientation (void) {

   if (!RoadMapViewIsGpsFocus) {
      return roadmap_math_get_orientation();

   } else if (RoadMapViewMode != VIEW_COMMUTE) {
      return roadmap_trip_get_orientation();
   }

#if 0
   if (RoadMapViewState == VIEW_STATE_SHOW_ROUTE) {
      return 0;
   }
#endif

   if (!navigate_track_enabled()) {
      int current = roadmap_trip_get_orientation();

      if ((RoadMapViewLastOrientation == -1) ||
            (abs(current - RoadMapViewLastOrientation) > 10)) {
         RoadMapViewLastOrientation = current;
      }

      return RoadMapViewLastOrientation;
   } else {
      return RoadMapViewAzymuth;
   }
}

int roadmap_view_get_scale (void) {

   if (navigate_main_alt_routes_display()){
      return RoadMapViewDistance;
   }

   if (navigate_track_enabled() && navigate_offtrack()) return -1;

   if (!RoadMapViewIsGpsFocus || AutoZoomSuspended ||
         (RoadMapViewState == VIEW_STATE_NORMAL)) {
      return -1;
   }

   if (!navigate_is_auto_zoom()) {
      return -1;
   }

   if (RoadMapViewState == VIEW_STATE_SHOW_ROUTE) {
      return RoadMapViewDistance;
   }

   if (RoadMapViewMode == VIEW_NAVIGATION) {
      return navigate_zoom_get_scale();
   }

   if (!navigate_track_enabled()) {
      return COMMUTE_SCALE;
   } else {
      if ((RoadMapViewDistance > 2000) && (RoadMapViewDistance < 7000)) {
         return COMMUTE_SCALE;
      } else {
         return RoadMapViewDistance < 3000 ? 3000 : RoadMapViewDistance;
      }
   }
}

int roadmap_view_show_labels (int cfcc, RoadMapPen *pens, int num_projs) {
   int i;
   int label_max_proj = -1;
   int min_thickness = 4;

   switch (cfcc) {
      case ROADMAP_ROAD_FREEWAY:
         label_max_proj = num_projs - 1;
         break;

      case ROADMAP_ROAD_PRIMARY:
         label_max_proj = num_projs - 2;
         break;

      case ROADMAP_ROAD_SECONDARY:
         label_max_proj = num_projs - 2;
         break;

      default:

	 if (roadmap_screen_is_hd_screen()) min_thickness = 7;
         if (roadmap_canvas_get_thickness(pens[0]) < min_thickness) return -1;

         for (i=0; i<num_projs; i++) {
            if (pens[i] && (roadmap_canvas_get_thickness(pens[i]) > 1))
               label_max_proj = i;
         }
   }

   return label_max_proj;
}

static void roadmap_view_auto_zoom_restore (void) {
   AutoZoomSuspended = 0;
   roadmap_main_remove_periodic (roadmap_view_auto_zoom_restore);
}

void roadmap_view_auto_zoom_suspend (void) {

   if (!RoadMapViewIsGpsFocus) return;

   if (navigate_is_auto_zoom()) {
      if (AutoZoomSuspended){
         roadmap_main_remove_periodic (roadmap_view_auto_zoom_restore);
      }
      AutoZoomSuspended = 1;
      roadmap_main_set_periodic
         (AUTO_ZOOM_SUSPEND_PERIOD * 1000, roadmap_view_auto_zoom_restore);
   }
}

void roadmap_view_reset (void) {
   if (AutoZoomSuspended) {
      roadmap_view_auto_zoom_restore();
   }
}

void roadmap_view_menu (void) {
}

void roadmap_view_commute (void) {
   RoadMapViewMode = VIEW_COMMUTE;
   roadmap_view_reset();
}

void roadmap_view_navigation (void) {
   roadmap_view_reset();
   RoadMapViewMode = VIEW_NAVIGATION;
}

int roadmap_view_hold (void) {
   return ((RoadMapViewState == VIEW_STATE_SHOW_ROUTE) || (RoadMapViewState == VIEW_STATE_SHOW_ALT_ROUTE));
}

