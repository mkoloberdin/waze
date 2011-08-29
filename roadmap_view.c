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
#include "roadmap_navigate.h"
#include "roadmap_config.h"
#include "roadmap_bar.h"
#include "roadmap_alternative_routes.h"
#include "navigate/navigate_main.h"
#include "navigate/navigate_zoom.h"
#include "navigate/navigate_bar.h"

#include "roadmap_view.h"

#define VIEW_COMMUTE     1
#define VIEW_NAVIGATION  2

#define COMMUTE_SCALE 7000

#define AUTO_ZOOM_SUSPEND_PERIOD 30
#define SHOW_ROUTE_TIME 8
#define BOTTOM_OFFSET       35


typedef enum {
   VIEW_STATE_NORMAL,
   VIEW_STATE_SHOW_ROUTE,
   VIEW_STATE_SHOW_ALT_ROUTE,
   VIEW_STATE_NAVIGATE
} ViewState;

static RoadMapPosition RoadMapViewWayPoint;
static int RoadMapViewLastOrientation = -1;
static long RoadMapViewDistance;
static int RoadMapViewAzymuth;
static int RoadMapViewMode = VIEW_NAVIGATION;
static int AutoZoomSuspended = 0;
static int RoadMapViewIsGpsFocus = 0;
static int RoadMapViewIsLocationFocus = 0;
static ViewState RoadMapViewState = VIEW_STATE_NORMAL;
static time_t RoadMapViewStateTime;

static RoadMapConfigDescriptor ConfigAutoZoomSpeedFactor =
                        ROADMAP_CONFIG_ITEM("AutoZoom", "Speed Factor");
static RoadMapConfigDescriptor ConfigAutoZoomScaleFactor =
                        ROADMAP_CONFIG_ITEM("AutoZoom", "Scale Factor");
static RoadMapConfigDescriptor ConfigAutoZoomMaxScale =
                        ROADMAP_CONFIG_ITEM("AutoZoom", "Max Scale");
static RoadMapConfigDescriptor ConfigAutoZoomMinScale =
                        ROADMAP_CONFIG_ITEM("AutoZoom", "Min Scale");
static RoadMapConfigDescriptor ConfigAutoZoomThreshold =
                        ROADMAP_CONFIG_ITEM("AutoZoom", "Threshold");
static RoadMapConfigDescriptor ConfigAutoZoomGradientFactor =
                        ROADMAP_CONFIG_ITEM("AutoZoom", "Gradient Factor");
static RoadMapConfigDescriptor ConfigAutoZoomGradientSpeedThreshold =
                        ROADMAP_CONFIG_ITEM("AutoZoom", "Gradient Speed Threshold");

static void set_state (ViewState state) {
   if (RoadMapViewState != state) {
      RoadMapViewState = state;
      RoadMapViewStateTime = time(NULL);
   }
}

BOOL roadmap_view_is_autozomm(){
   return navigate_is_auto_zoom()  || navigate_is_speed_auto_zoom();
}


void roadmap_view_refresh (void) {
   int should_focus_on_me = 0;
   const RoadMapPosition *pos;

   const char *focus = roadmap_trip_get_focus_name();
   
   RoadMapViewIsGpsFocus = focus && !strcmp(focus, "GPS");
   RoadMapViewIsLocationFocus = focus && !strcmp(focus, "Location");

   if (RoadMapViewIsGpsFocus &&
       roadmap_screen_get_orientation_mode() != ORIENTATION_FIXED &&
       roadmap_screen_get_nonogl_view_mode() != VIEW_MODE_3D) {
         if ( roadmap_screen_get_view_mode() == VIEW_MODE_2D ) {
            roadmap_screen_move_center (-roadmap_screen_height()/6);
         }
         else {
            int bottom_offset =  roadmap_canvas_height()/5;
            if ( !navigate_bar_is_hidden() )
               bottom_offset =  navigate_bar_get_height() + roadmap_bar_bottom_height() + ADJ_SCALE( BOTTOM_OFFSET );
            roadmap_screen_move_center ( bottom_offset - roadmap_canvas_height()/2 );
         }
   } else {
      roadmap_screen_move_center (0);
   }

   if (!navigate_main_alt_routes_display()){
      if (!navigate_track_enabled() && !navigate_offtrack())  {
         if (RoadMapViewState != VIEW_STATE_NORMAL)
            roadmap_screen_reset_view_mode();
         set_state(VIEW_STATE_NORMAL);
         return;
      }

      if (navigate_offtrack()) return;

      if (!roadmap_view_is_autozomm()) {
         return;
      }
   }

   if (RoadMapViewState == VIEW_STATE_NORMAL) {
      roadmap_view_reset();
      if (navigate_main_alt_routes_display()){
         set_state(VIEW_STATE_SHOW_ALT_ROUTE);
         roadmap_screen_set_orientation_fixed();
         roadmap_screen_override_view_mode (VIEW_MODE_2D);
      }else
         set_state(VIEW_STATE_SHOW_ROUTE);
   } else if (RoadMapViewState == VIEW_STATE_SHOW_ROUTE)  {
      if (navigate_main_alt_routes_display()){
               set_state(VIEW_STATE_SHOW_ALT_ROUTE);
         roadmap_screen_set_orientation_fixed();
         roadmap_screen_override_view_mode (VIEW_MODE_2D);
      }
      else if (navigate_main_is_alt_routes()){
         set_state(VIEW_STATE_NAVIGATE);
         roadmap_screen_set_orientation_dynamic();
         roadmap_screen_reset_view_mode();
      }
      else  {
         set_state(VIEW_STATE_NAVIGATE);
         roadmap_screen_set_orientation_dynamic();
         roadmap_screen_reset_view_mode();
         //roadmap_screen_update_center_animated(roadmap_trip_get_focus_position(), 800, 0);
         //focus_on_me();
         should_focus_on_me = 1;
      }

   } else if (RoadMapViewState == VIEW_STATE_SHOW_ALT_ROUTE)  {
      if (!navigate_main_alt_routes_display()){
         set_state(VIEW_STATE_NORMAL);
         roadmap_screen_set_orientation_dynamic();
         roadmap_screen_reset_view_mode();
      }
      //roadmap_screen_set_orientation_fixed();
      //roadmap_screen_override_view_mode (VIEW_MODE_2D);
   } else if (RoadMapViewState == VIEW_STATE_NAVIGATE && navigate_main_alt_routes_display()) {
      set_state(VIEW_STATE_SHOW_ALT_ROUTE);
      roadmap_screen_set_orientation_fixed();
      roadmap_screen_override_view_mode (VIEW_MODE_2D);
   }


   pos = roadmap_trip_get_focus_position ();

   if (RoadMapViewState == VIEW_STATE_SHOW_ROUTE) {

//      navigate_get_waypoint (-1, &RoadMapViewWayPoint);
//      if ((RoadMapViewWayPoint.latitude == 0) && (RoadMapViewWayPoint.longitude == 0))
//         return;
//      RoadMapViewDistance =
//         roadmap_math_distance(pos, &RoadMapViewWayPoint) * 9 / 4;
//      RoadMapViewAzymuth = 360 - roadmap_math_azymuth(pos, &RoadMapViewWayPoint);
//      RoadMapViewWayPoint.longitude = (RoadMapViewWayPoint.longitude + pos->longitude) / 2;
//      RoadMapViewWayPoint.latitude = (RoadMapViewWayPoint.latitude + pos->latitude) / 2;
//      //roadmap_screen_move_center (100);
//      roadmap_screen_move_center (0);
//#ifdef OPENGL
//     roadmap_screen_update_center_animated(&RoadMapViewWayPoint, 600, 0);
//#endif
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
       //roadmap_screen_move_center (roadmap_canvas_height()/3);
       //roadmap_screen_move_center (50);
      roadmap_screen_move_center (20);
#ifdef OPENGL
       roadmap_screen_update_center_animated(&RoadMapViewWayPoint, 600, 0);
#endif
       return;
   }

   if (RoadMapViewMode == VIEW_COMMUTE) return;

   if (RoadMapViewIsGpsFocus &&
       roadmap_screen_get_orientation_mode() != ORIENTATION_FIXED &&
       roadmap_screen_get_nonogl_view_mode() != VIEW_MODE_3D) {

         if ( roadmap_screen_get_view_mode() == VIEW_MODE_2D ) {
            roadmap_screen_move_center (-roadmap_screen_height()/6);
         }
         else {
            int bottom_offset =  roadmap_canvas_height()/5;
            if ( !navigate_bar_is_hidden() )
               bottom_offset =  navigate_bar_get_height() + roadmap_bar_bottom_height() + ADJ_SCALE( BOTTOM_OFFSET );

            roadmap_screen_move_center ( bottom_offset - roadmap_canvas_height()/2 );
         }
   } else {
      roadmap_screen_move_center (0);
   }

   if (navigate_main_alt_routes_display())
      roadmap_alternative_route_get_waypoint (-1, &RoadMapViewWayPoint);
   else
      navigate_get_waypoint (25000, &RoadMapViewWayPoint);

   RoadMapViewDistance = roadmap_math_distance(pos, &RoadMapViewWayPoint);

   RoadMapViewAzymuth = 360 - roadmap_math_azymuth(pos, &RoadMapViewWayPoint);
   
   //if (should_focus_on_me)
//      focus_on_me();
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

static void int_speed_params(void){
   roadmap_config_declare
      ("preferences", &ConfigAutoZoomSpeedFactor, "15", NULL);
   roadmap_config_declare
      ("preferences", &ConfigAutoZoomScaleFactor, "250", NULL);
   roadmap_config_declare
      ("preferences", &ConfigAutoZoomMaxScale, "2000", NULL);
   roadmap_config_declare
      ("preferences", &ConfigAutoZoomMinScale, "250", NULL);
   roadmap_config_declare
      ("preferences", &ConfigAutoZoomThreshold, "5", NULL);
   roadmap_config_declare
      ("preferences", &ConfigAutoZoomGradientFactor, "150", NULL);
   roadmap_config_declare
      ("preferences", &ConfigAutoZoomGradientSpeedThreshold, "60", NULL);


}

static int get_autozoom_speed_factor(void){
   return roadmap_config_get_integer( &ConfigAutoZoomSpeedFactor );
}

static int get_autozoom_threshold(void){
   return roadmap_config_get_integer( &ConfigAutoZoomThreshold );
}

static int get_autozoom_scale_factor(void){
   if (roadmap_screen_get_view_mode() == VIEW_MODE_3D)
      return roadmap_config_get_integer( &ConfigAutoZoomScaleFactor );
   else
      return roadmap_config_get_integer( &ConfigAutoZoomScaleFactor )*2;
}

static int get_autozoom_max_scale(void){
   if (roadmap_screen_get_view_mode() == VIEW_MODE_3D)
      return roadmap_config_get_integer( &ConfigAutoZoomMaxScale );
   else
      return roadmap_config_get_integer( &ConfigAutoZoomMaxScale )*2;
}

static int get_autozoom_min_scale(void){
   return roadmap_config_get_integer( &ConfigAutoZoomMinScale );
}
static int get_autozoom_gradient_factor(void){
   return roadmap_config_get_integer( &ConfigAutoZoomGradientFactor );
}
static int get_autozoom_gradient_speed_threshold(void){
   return roadmap_config_get_integer( &ConfigAutoZoomGradientSpeedThreshold );
}

static long get_speed_dependant_scale(){
   RoadMapGpsPosition pos;
   BOOL gps_active;
   int gps_state;
   int speed;
   BOOL initialzed = FALSE;
   long scale;
   static int last_speed = -1;
   static long last_scale = 0;
   int gradient_factor_norm = 100;
   int max_scale;

   if (!initialzed)
      int_speed_params();

   gps_state = roadmap_gps_reception_state();
   gps_active = (gps_state != GPS_RECEPTION_NA) && (gps_state != GPS_RECEPTION_NONE);


   if ((RoadMapViewState == VIEW_STATE_NAVIGATE) && navigate_track_enabled()) {
       long nav_zoom = navigate_zoom_get_scale();
       if ((nav_zoom <= 500) && (nav_zoom))
          return nav_zoom;
   }

   if (!gps_active)
      return -1;


   roadmap_navigate_get_current(&pos, NULL, NULL);

   if (pos.speed < 0)
      return -1;

   speed = roadmap_math_to_speed_unit(pos.speed);
   if (!roadmap_math_is_metric())
      speed *= 1.60934;

   if (last_speed == -1)
      last_speed = speed;
   else{

      if (abs(speed -last_speed) < get_autozoom_threshold()){
            return last_scale;
      }
      else
         last_speed = speed;
   }

   if ( speed < get_autozoom_gradient_speed_threshold() ) {
      scale = get_autozoom_min_scale() + (speed/get_autozoom_speed_factor())*get_autozoom_scale_factor();
   }
   else   {
      /*
       * For the higher speeds use increased gradient. Controlled by gradient factor.
       * Factorized result should be normalized by 100.
       */
      int normalized_speed = speed/get_autozoom_speed_factor();
      scale = get_autozoom_min_scale() + ( normalized_speed*get_autozoom_scale_factor()*get_autozoom_gradient_factor() ) / gradient_factor_norm;
   }

   max_scale = ( get_autozoom_max_scale()*get_autozoom_gradient_factor() )/gradient_factor_norm;
   if ( scale > max_scale )
      scale = max_scale;
   //printf ("get_speed_dependant_scale - speed = %d, scale =%d\n", speed, scale);

   last_scale = scale;
   return scale;
}

long roadmap_view_get_scale (void) {
   RoadMapGpsPosition gps_position;

   if (navigate_main_alt_routes_display()){
      return RoadMapViewDistance;
   }

   if (navigate_track_enabled() && navigate_offtrack()) return -1;


   if (roadmap_view_is_autozomm() &&
       RoadMapViewIsGpsFocus&&
       !AutoZoomSuspended &&
       RoadMapViewState == VIEW_STATE_NAVIGATE &&
       roadmap_navigate_get_current(&gps_position, NULL, NULL) != 0) {
      RoadMapPosition pos;
      pos.latitude = gps_position.latitude;
      pos.longitude = gps_position.longitude;
      return navigate_main_visible_route_scale(&pos);
   }
      
   if (RoadMapViewIsGpsFocus && !AutoZoomSuspended && ( (RoadMapViewState == VIEW_STATE_NORMAL) || (RoadMapViewState == VIEW_STATE_NAVIGATE) ) && navigate_is_speed_auto_zoom()) {
      return get_speed_dependant_scale();
   }

   if (!roadmap_view_is_autozomm()) {
       return -1;
   }

   if ((!RoadMapViewIsGpsFocus && !RoadMapViewIsLocationFocus) || AutoZoomSuspended ||
         (RoadMapViewState == VIEW_STATE_NORMAL)) {
      return -1;
   }

   if (RoadMapViewState == VIEW_STATE_SHOW_ROUTE) {
      return -1;
   }
   
   if (RoadMapViewIsLocationFocus) {
      return -1;
   }

   if ((RoadMapViewMode == VIEW_NAVIGATION) && navigate_is_auto_zoom()) {
      return (long)navigate_zoom_get_scale();
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

   if (navigate_is_auto_zoom() || navigate_is_speed_auto_zoom()) {
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
   return ((RoadMapViewState == VIEW_STATE_SHOW_ALT_ROUTE));
}

int roadmap_view_reset_hold (void) {
   if (RoadMapViewState == VIEW_STATE_SHOW_ROUTE) {
      RoadMapViewStateTime = 0;
      return 1;
   } else {
      return 0;
   }

}

int roadmap_view_should_refresh (void) {
   if (RoadMapViewState == VIEW_STATE_SHOW_ROUTE)
      return 1;
   else
      return 0;
}
