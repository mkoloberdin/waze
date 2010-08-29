/* editor_screen.c - screen drawing
 *
 * LICENSE:
 *
 *   Copyright 2005 Ehud Shabtai
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
 *
 * SYNOPSYS:
 *
 *   See editor_screen.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "roadmap.h"
#include "roadmap_res.h"
#include "roadmap_canvas.h"
#include "roadmap_screen.h"
#include "roadmap_screen_obj.h"
#include "roadmap_trip.h"
#include "roadmap_display.h"
#include "roadmap_math.h"
#include "roadmap_navigate.h"
#include "roadmap_pointer.h"
#include "roadmap_line.h"
#include "roadmap_line_route.h"
#include "roadmap_shape.h"
#include "roadmap_square.h"
#include "roadmap_layer.h"
#include "roadmap_locator.h"
#include "roadmap_hash.h"
#include "roadmap_sprite.h"
#include "roadmap_lang.h"
#include "roadmap_start.h"
#include "roadmap_config.h"
#include "roadmap_main.h"
#include "roadmap_message.h"
#include "roadmap_reminder.h"
#include "roadmap_object.h"

#include "db/editor_db.h"
#include "db/editor_point.h"
#include "db/editor_marker.h"
#include "db/editor_shape.h"
#include "db/editor_line.h"
#include "db/editor_square.h"
#include "db/editor_street.h"
#include "db/editor_route.h"
#include "db/editor_override.h"
#include "db/editor_trkseg.h"
#include "track/editor_track_main.h"
#include "editor_main.h"

#include "Realtime/RealtimeAlerts.h"
#include "Realtime/RealtimeAlertCommentsList.h"
#include "Realtime/Realtime.h"

#include "static/editor_dialog.h"
#include "track/editor_track_main.h"

#include "editor_screen.h"

#define MIN_THICKNESS 3
#define MAX_LAYERS (ROADMAP_ROAD_LAST + 1)
#define MAX_LINE_SELECTIONS 100

#define MAX_PEN_LAYERS 2

#define MAX_ROAD_STATES 4
#define SELECTED_STATE 0
#define NO_DATA_STATE 1
#define MISSING_ROUTE_STATE 2
#define MISSING_NAME_STATE 3
#define NO_ROAD_STATE 4

typedef struct editor_pen_s {
   int in_use;
   int thickness;
   RoadMapPen pen;
} editor_pen;

static char *g_forced_car = NULL;

static int editor_screen_long_click (RoadMapGuiPoint *point);

static RoadMapConfigDescriptor ShowCandidateRoads =
                  ROADMAP_CONFIG_ITEM("Record", "Show unmatched roads");

static RoadMapConfigDescriptor GrayScale =
                  ROADMAP_CONFIG_ITEM("Editor", "Gray scale");

static RoadMapConfigDescriptor WazzyImage =
                  ROADMAP_CONFIG_ITEM("Editor", "wazzy");

static   RoadMapPosition position;

static editor_pen EditorPens[MAX_LAYERS][MAX_PEN_LAYERS][MAX_ROAD_STATES];
static editor_pen EditorTrackPens[MAX_PEN_LAYERS];

SelectedLine SelectedLines[MAX_LINE_SELECTIONS];

static int select_count;

#ifdef SSD
static char const *PopupMenuItems[] = {

#if EDITOR_ALLOW_LINE_DELETION
   "deleteroads",
#endif
   "setasdestination",
   "setasdeparture",
   "save_location",
#ifdef IPHONE
   "properties",
#endif
   NULL,
};
#endif


static RoadMapScreenSubscriber screen_prev_after_refresh = NULL;


static void editor_screen_update_segments (void) {

   if (select_count) {
#ifndef IPHONE_NATIVE
      editor_segments_properties (SelectedLines, select_count);
#else
      ssd_dialog_hide_all(dec_close);
      editor_street_bar_display (SelectedLines, select_count);
#endif //IPHONE_NATIVE
   }
}

static void editor_screen_delete_segments (void) {

   int i;

   for (i=0; i<select_count; i++) {

      SelectedLine *line = &SelectedLines[i];

      if (editor_db_activate (roadmap_plugin_get_fips(&line->line)) == -1) {

         editor_db_create (roadmap_plugin_get_fips(&line->line));

         if (editor_db_activate (roadmap_plugin_get_fips(&line->line)) == -1) {
            continue;
         }
      }

      if (roadmap_plugin_get_id (&line->line) == ROADMAP_PLUGIN_ID) {


         int line_id = roadmap_plugin_get_line_id (&line->line);
         int square = roadmap_plugin_get_square (&line->line);

			roadmap_square_set_current (square);
            editor_override_line_set_flag (line_id, line->line.square, ED_LINE_DELETED);

      } else if (roadmap_plugin_get_id (&line->line) == EditorPluginID) {

         editor_line_set_flag
            (roadmap_plugin_get_line_id (&line->line),
	          ED_LINE_DELETED);
      }
   }

   editor_screen_reset_selected ();

   return;

}


void report_accident_at_screen_point(void){

   RoadMapGpsPosition gpsPos;
   int distance;

   RoadMapPosition lineFrom, lineTo;
   PluginLine line;
   int iDirection = RT_ALERT_MY_DIRECTION;

   gpsPos.latitude = position.latitude;
   gpsPos.longitude = position.longitude;
   gpsPos.altitude = 1000;
   gpsPos.speed =60;

   gpsPos.steering = -1;

   roadmap_navigate_retrieve_line
         (&position, 0, 7, &line, &distance, LAYER_ALL_ROADS);
   roadmap_line_from (line.line_id, &lineFrom);
   roadmap_line_to (line.line_id, &lineTo);

   gpsPos.steering = roadmap_math_azymuth (&lineFrom, &lineTo);

   if (gpsPos.steering < 0) gpsPos.steering += 360;
   Realtime_Alert_ReportAtLocation(RT_ALERT_TYPE_ACCIDENT, "", iDirection,
                                   gpsPos,"" );
}


void report_accident_opposite_side_at_screen_point(void){

   RoadMapGpsPosition gpsPos;
   int distance;

   RoadMapPosition lineFrom, lineTo;
   PluginLine line;
   int iDirection = RT_ALERT_MY_DIRECTION;

   gpsPos.latitude = position.latitude;
   gpsPos.longitude = position.longitude;
   gpsPos.altitude = 1000;
   gpsPos.speed =60;

   gpsPos.steering = -1;

   roadmap_navigate_retrieve_line
      (&position, 0, 7, &line, &distance, LAYER_ALL_ROADS);
   roadmap_line_from (line.line_id, &lineFrom);
   roadmap_line_to (line.line_id, &lineTo);

   gpsPos.steering = roadmap_math_azymuth (&lineTo, &lineFrom);


   if (gpsPos.steering < 0) gpsPos.steering += 360;
   Realtime_Alert_ReportAtLocation(RT_ALERT_TYPE_ACCIDENT, "", iDirection,
                                   gpsPos,"" );

}




#ifdef SSD
static void popup_menu_callback (int ret, void *context) {
   roadmap_trip_remove_point("Selection");
   editor_screen_reset_selected ();
}
#endif


void editor_screen_select_line (const PluginLine *line) {

   int i;

   for (i=0; i<select_count; i++) {
      if (roadmap_plugin_same_line(&SelectedLines[i].line, line)) break;
   }

   if (i<select_count) {
      /* line was already selected, remove it */
      int j;
      for (j=i+1; j<select_count; j++) {
         SelectedLines[j-1] = SelectedLines[j];
      }

      select_count--;

   } else {

      if (select_count < MAX_LINE_SELECTIONS) {

         SelectedLines[select_count].line = *line;
         select_count++;
      }
   }
}


static int editor_screen_line_iter_cb (const PluginLine *line, void *context, int flags) {

  	editor_screen_select_line (line);
  	return 0;
}


static int editor_screen_short_click (RoadMapGuiPoint *point) {
/*
   int AlertId;
   int scale;

   if (!roadmap_object_short_ckick_enabled())
      return 0;

   roadmap_math_to_position (point, &position, 1);

   scale = roadmap_math_get_scale(0)/80;
   AlertId = RTAlerts_Alert_near_position(position, scale);
   if (AlertId != -1)
   {
   	if ((RTAlerts_State() == STATE_SCROLLING) && (RTAlerts_Get_Current_Alert_Id() == AlertId))
   		RealtimeAlertCommentsList(AlertId);
   	else
   		RTAlerts_Popup_By_Id_No_Center(AlertId);
   	return 1;
   }
*/
   return 0;
}

static int editor_screen_long_click (RoadMapGuiPoint *point) {
//   PluginStreet street;
   PluginLine line;
   int distance;
   int scale;
   int orig_square;
   RoadMapPosition from;
   RoadMapPosition to;

   roadmap_math_to_position (point, &position, 1);

   scale = roadmap_math_get_scale(0)/200;

   if (roadmap_navigate_retrieve_line
         (&position, 0, 7, &line, &distance, LAYER_ALL_ROADS) == -1) {

      roadmap_display_hide ("Selected Street");
      roadmap_trip_set_point ("Selection", &position);
#ifdef SSD
      roadmap_start_hide_menu ("Editor Menu");
#endif
      editor_screen_reset_selected ();
      roadmap_screen_redraw ();
      return 1;
   }

   roadmap_trip_set_point ("Selection", &position);

   //roadmap_display_activate ("Selected Street", &line, &position, &street);

   editor_screen_select_line (&line);

   if (line.plugin_id == ROADMAP_PLUGIN_ID) {
		orig_square = roadmap_square_active ();

		roadmap_street_extend_line_ends (&line, &from, &to, FLAG_EXTEND_BOTH, editor_screen_line_iter_cb, NULL);
		roadmap_display_update_points ("Selected Street", &from, &to);

#if 0
{
		int curr_scale;
   	RoadMapStreetProperties props;
   	char name[512];

		/*
		 * select the same line in other scales
		 */
		roadmap_square_set_current (orig_square);
	   roadmap_street_get_properties (line.line_id, &props);
	   strncpy_safe (name, roadmap_street_get_full_name (&props), 512);
		for (curr_scale = roadmap_square_get_num_scales () - 1; curr_scale > 0; curr_scale--) {

			if (roadmap_navigate_retrieve_line_force_name
				 (&position, curr_scale, 7, &line, &distance, LAYER_ALL_ROADS, orig_square, name,
				  roadmap_line_route_get_direction (line.line_id, ROUTE_DIRECTION_ANY),
				  0, 0, &from, &to) != -1) {

				editor_screen_select_line (&line);
				roadmap_street_extend_line_ends (&line, &from, &to, FLAG_EXTEND_BOTH, editor_screen_line_iter_cb, NULL);
			}
		}
}
#endif

	}
#ifdef SSD
//   if (!roadmap_reminder_feature_enabled())
//      PopupMenuItems[3] = NULL;
   roadmap_start_popup_menu ("Editor Menu", PopupMenuItems,
                             popup_menu_callback, point);
#endif
   roadmap_screen_redraw ();

   return 1;
}

/* TODO: this is a bad callback which is called from roadmap_layer_adjust().
 * This should be changed. Currently when the editor is enabled, an explicit
 * call to roadmap_layer_adjust() is called. When this is fixed, that call
 * should be removed.
 */
void editor_screen_adjust_layer (int layer, int thickness, int pen_count) {

   int i;
   int j;

   if (layer > ROADMAP_ROAD_LAST) return;
   if (!editor_is_enabled()) return;

   if (thickness < 1) thickness = 1;
   if ((pen_count > 1) && (thickness < 3)) {
      pen_count = 1;
   }

   for (i=0; i<MAX_PEN_LAYERS; i++)
      for (j=0; j<MAX_ROAD_STATES; j++) {

         editor_pen *pen = &EditorPens[layer][i][j];

         pen->in_use = i<pen_count;

         if (!pen->in_use) continue;

         roadmap_canvas_select_pen (pen->pen);

         if (i == 1) {
            pen->thickness = thickness-2;
         } else {
            pen->thickness = thickness;
         }

         roadmap_canvas_set_thickness (pen->thickness);
      }

   if (layer == ROADMAP_ROAD_STREET) {
      roadmap_canvas_select_pen (EditorTrackPens[0].pen);
      EditorTrackPens[0].thickness = thickness;
      EditorTrackPens[0].in_use = 1;
      roadmap_canvas_set_thickness (EditorTrackPens[0].thickness);
      if (pen_count == 1) {
         EditorTrackPens[1].in_use = 0;
      } else {
         roadmap_canvas_select_pen (EditorTrackPens[1].pen);
         EditorTrackPens[1].thickness = thickness - 2;
         EditorTrackPens[1].in_use = 1;
         roadmap_canvas_set_thickness (EditorTrackPens[1].thickness);
      }


   }
}


static int editor_screen_get_road_state (int line, int cfcc,
                                         int plugin_id, int fips) {

   int has_street = 0;
   int has_route = 0;

   if (plugin_id == ROADMAP_PLUGIN_ID) {

      if (roadmap_locator_activate (fips) >= 0) {

         RoadMapStreetProperties properties;

         roadmap_street_get_properties (line, &properties);

         if (properties.street != -1) {
            has_street = 1;
         }

         if (roadmap_line_route_get_direction
               (line, ROUTE_CAR_ALLOWED) != ROUTE_DIRECTION_NONE) {

            has_route = 1;

         } else if (editor_db_activate (fips) != -1) {

            int direction = ROUTE_DIRECTION_NONE;
            editor_override_line_get_direction (line, -1, &direction);

            if (direction != ROUTE_DIRECTION_NONE) {
               has_route = 1;
            }
         }
      }

   } else {

      if (editor_db_activate (fips) != -1) {

      	int street = -1;
      	int direction = ROUTE_DIRECTION_NONE;

         editor_line_get_street (line, &street);
         if (street != -1) {
            has_street = 1;
         }

         editor_line_get_direction (line, &direction);
         if (direction != ROUTE_DIRECTION_NONE) {
            has_route = 1;
         }
      }
   }

   if (has_street && has_route) return NO_ROAD_STATE;
   else if (!has_street && !has_route) return NO_DATA_STATE;
   else if (has_street) return MISSING_ROUTE_STATE;
   else return MISSING_NAME_STATE;
}


int editor_screen_override_pen (int line,
                                int cfcc,
                                int fips,
                                int pen_type,
                                RoadMapPen *override_pen) {

   int i;
   int ActiveDB;
   int road_state;
   int is_selected = 0;
   editor_pen *pen = NULL;
   int route = -1;
   int direction;

   if (cfcc > ROADMAP_ROAD_LAST) return 0;

   ActiveDB = editor_db_activate (fips);

   if (!editor_is_enabled()) return 0;

   for (i=0; i<select_count; i++) {
      if ((line == SelectedLines[i].line.line_id) &&
          (SelectedLines[i].line.plugin_id != EditorPluginID) &&
          (fips == SelectedLines[i].line.fips)) {
         is_selected = 1;
         break;
      }
   }

   if (is_selected) {

      road_state = SELECTED_STATE;
   } else {

      if (!roadmap_screen_fast_refresh() && (pen_type > 0)) {
         road_state = editor_screen_get_road_state (line, cfcc, 0, fips);
      } else {
         road_state = NO_ROAD_STATE;
      }
   }

   if (road_state != NO_ROAD_STATE) {

      if (pen_type > 1) return 0;

      pen = &EditorPens[cfcc][pen_type][road_state];
      if (!pen->in_use) {
         *override_pen = NULL;
         return 1;
      }
      *override_pen = pen->pen;
   } else {

      return 0;
   }

   if (ActiveDB != -1) {
      //TODO check roadmap route data
      route = editor_override_line_get_direction (line,-1, &direction);
   }

   if ((pen->thickness > 20) &&
         (roadmap_layer_get_pen (cfcc, pen_type+1, 0) == NULL) &&
         (route != -1) && (direction > 0)) {

      RoadMapPosition from;
      RoadMapPosition to;
      int first_shape = -1;
      int last_shape = -1;

      roadmap_line_from (line, &from);
      roadmap_line_to (line, &to);

      if (*override_pen == NULL) {
         *override_pen = roadmap_layer_get_pen (cfcc, pen_type, 0);
      }

      roadmap_line_shapes (line, &first_shape, &last_shape);

      roadmap_screen_draw_one_line
            (&from, &to, 0, &from, first_shape, last_shape,
             NULL, override_pen, 1, -1, NULL, NULL, NULL);

      roadmap_screen_draw_line_direction
            (&from, &to, &from, first_shape, last_shape,
             NULL,
             pen->thickness, direction, "#000000");

      return 1;
   }



   return 0;
}


static char *editor_screen_get_pen_color (int pen_type, int road_state) {

   if (pen_type == 0) {
      switch (road_state) {
      case SELECTED_STATE: return "yellow";
      default: return "black";
      }
   }


   switch (road_state) {

   case SELECTED_STATE:       return "black";
   case NO_DATA_STATE:        return "dark red";
   case MISSING_NAME_STATE:   return "red";
   case MISSING_ROUTE_STATE:  return "yellow";

   default:
                              assert (0);
                              return "black";
   }
}


static int editor_screen_draw_markers (void) {
   RoadMapArea screen;
   int count;
   int i;
   RoadMapPen pen;
   int steering;
   RoadMapPosition pos;
   RoadMapGuiPoint screen_point;
   RoadMapGuiPoint icon_screen_point;
   const char* icon;
   int drawn = 0;

   int fips = roadmap_locator_active ();
   if (editor_db_activate(fips) == -1) return 0;

   count = editor_marker_count ();

   roadmap_math_screen_edges (&screen);

   for (i=0; i<count; i++) {

		if (editor_marker_is_obsolete (i)) continue;
      editor_marker_position (i, &pos, &steering);
      if (!roadmap_math_point_is_visible (&pos)) continue;

      drawn++;
      roadmap_math_coordinate (&pos, &screen_point);
      roadmap_math_rotate_project_coordinate (&screen_point);

      pen = roadmap_layer_get_pen (ROADMAP_ROAD_MAIN,0 ,0);
      if (pen != NULL) {
         pen = roadmap_layer_get_pen (ROADMAP_ROAD_STREET,0 ,0);
         if (pen != NULL) {
            icon = edit_marker_icon(i);
            if (*icon) {
               RoadMapImage image =      (RoadMapImage) roadmap_res_get( RES_BITMAP, RES_SKIN, icon );
               if (image != NULL){
                  	icon_screen_point.x = screen_point.x - 8; //roadmap_canvas_image_width(image) ;
                  	icon_screen_point.y = screen_point.y - roadmap_canvas_image_height (image)  + 5;
               		roadmap_canvas_draw_image (image, &icon_screen_point,
                     									                      0, IMAGE_NORMAL);
               }
	            else
   	             roadmap_sprite_draw ("marker", &screen_point, steering);
            }
            else{
                roadmap_sprite_draw ("marker", &screen_point, steering);
            }

         }
         else{
                roadmap_sprite_draw ("marker", &screen_point, steering);
         }
      }
   }

   return drawn;
}


static void editor_screen_after_refresh (void) {

   if (editor_is_enabled()) {
   }

   if (screen_prev_after_refresh) {
      (*screen_prev_after_refresh) ();
   }
}


static int editor_screen_draw_lines
              (int fips, int min_cfcc, int pen_type) {

   int line;
   int count;
   int j;
   RoadMapPosition from;
   RoadMapPosition to;
   RoadMapPosition trk_from_pos;
   int first_shape;
   int last_shape;
   int cfcc;
   int flag;
   int fully_visible = 0;
   int road_state;
   RoadMapPen pen = NULL;
   int trkseg;
   int route;
   int drawn = 0;

   roadmap_log_push ("editor_screen_draw_lines");

   if (! (-1 << min_cfcc)) {
      roadmap_log_pop ();
      return 0;
   }

   count = editor_line_get_count ();

   for (line=0; line<count; line++) {

      editor_line_get (line, &from, &to, &trkseg, &cfcc, &flag);
      if (cfcc < min_cfcc) continue;

      if (flag & ED_LINE_DELETED) continue;

      if (editor_is_enabled () ) {

         int is_selected = 0;

         for (j=0; j<select_count; j++) {

            if ((line == SelectedLines[j].line.line_id) &&
                (SelectedLines[j].line.plugin_id == EditorPluginID)  &&
                (fips == SelectedLines[j].line.fips)) {

               is_selected = 1;
               break;
            }
         }

         if (is_selected) {

            road_state = SELECTED_STATE;
         } else if (flag & ED_LINE_CONNECTION) {

            road_state = MISSING_ROUTE_STATE;
         } else {

            road_state = editor_screen_get_road_state (line, cfcc, 1, fips);
         }

      } else {
         road_state = NO_ROAD_STATE;
      }

      if (road_state == NO_ROAD_STATE) {

         pen = roadmap_layer_get_pen (cfcc, pen_type, 0);
      } else {

         if (pen_type > 1) continue;
         if (!EditorPens[cfcc][pen_type][road_state].in_use) continue;

         pen = EditorPens[cfcc][pen_type][road_state].pen;
      }

      if (pen == NULL) continue;

      editor_trkseg_get (trkseg, &j, &first_shape, &last_shape, NULL);
      editor_point_position (j, &trk_from_pos);

      drawn += roadmap_screen_draw_one_line
               (&from, &to, fully_visible,
                &trk_from_pos, first_shape, last_shape,
                editor_shape_position, &pen, 1, -1, 0, 0, 0);

      if ((EditorPens[cfcc][pen_type][0].thickness > 20) &&
         (pen_type == 1)) {

         int direction;

         route = editor_line_get_direction (line, &direction);

         if (route != -1) {

            if (direction > 0) {
               roadmap_screen_draw_line_direction
                  (&from, &to, &trk_from_pos, first_shape, last_shape,
                   editor_shape_position,
                   EditorPens[cfcc][pen_type][0].thickness,
                   direction, "#000000");
            }
         }
      }
   }

   roadmap_log_pop ();
   return drawn;
}


static int editor_screen_repaint_lines (int fips, int pen_type) {

   int i;
   int count;
   int layers[256];
   int min_category = 256;
   int drawn;

   roadmap_log_push ("editor_screen_repaint_square");

   count = roadmap_layer_visible_lines (layers, 256, pen_type);

   for (i = 0; i < count; ++i) {
        if (min_category > layers[i]) min_category = layers[i];
   }

   drawn = editor_screen_draw_lines (fips, min_category, pen_type);

   roadmap_log_pop ();

   return drawn;
}


void editor_screen_repaint (int max_pen) {

   int k;
   int fips = roadmap_locator_active ();
   int drawn = 0;

   if (editor_db_activate(fips) != -1) {

      for (k = 0; k <= max_pen; k++) {

         drawn += editor_screen_repaint_lines (fips, k);

      }
   }

   if (!editor_ignore_new_roads() || editor_screen_show_candidates()) {

      for (k = 0; k <= max_pen; k++) {

         if (k < MAX_PEN_LAYERS) {
            if (EditorTrackPens[k].in_use) {
               editor_track_draw_current (EditorTrackPens[k].pen);
            }
         }
      }
   }

   for (k=0; k<select_count; k++) {

      if (SelectedLines[k].line.plugin_id == ROADMAP_PLUGIN_ID /*&&
      	 roadmap_square_at_current_scale (SelectedLines[k].line.square)*/) {

         RoadMapPosition from;
         RoadMapPosition to;
         int first_shape;
         int last_shape;
         RoadMapShapeItr shape_itr;
         RoadMapPen *pen;
         int i;

         roadmap_plugin_get_line_points (&SelectedLines[k].line,
                                         &from, &to,
                                         &first_shape, &last_shape,
                                         &shape_itr);

         for (i=0; i<1; i++) {
            pen = &EditorPens[SelectedLines[k].line.cfcc][i][SELECTED_STATE].pen;

            roadmap_screen_draw_one_line
               (&from, &to, 0,
                &from, first_shape, last_shape,
                shape_itr, pen, 1, -1, 0, 0, 0);
         }
      }
   }

   //draw gray scale
   if(editor_screen_gray_scale()){
		editor_track_draw_new_direction_roads ();
   }

   drawn += editor_screen_draw_markers ();

   if (drawn) {

   	// force update tiles with editor lines
   	roadmap_square_force_next_update ();
   	roadmap_square_view (NULL, 0);
   }

}


void editor_screen_set (int status) {

   if (status) {

      roadmap_pointer_register_short_click
            (editor_screen_short_click, POINTER_NORMAL);
      roadmap_pointer_register_long_click
            (editor_screen_long_click, POINTER_NORMAL);
      roadmap_pointer_register_enter_key_press
            (editor_screen_short_click, POINTER_HIGH);
      roadmap_pointer_register_enter_key_press
            (editor_screen_long_click, POINTER_NORMAL);

      roadmap_layer_adjust();
   } else {

      roadmap_pointer_unregister_short_click (editor_screen_short_click);
      roadmap_pointer_unregister_long_click (editor_screen_long_click);
   }
}


void editor_screen_reset_selected (void) {

   select_count = 0;
   roadmap_screen_redraw ();
}


void editor_screen_initialize (void) {

   int i;
   int j;
   int k;
   char name[80];

   roadmap_config_declare_enumeration
      ("preferences", &ShowCandidateRoads, NULL, "no", "yes", NULL);

   roadmap_config_declare_enumeration
      ("preferences", &GrayScale, NULL, "no", "yes", NULL);

   roadmap_config_declare
      ("preferences", &WazzyImage, "wazzy", NULL);
   /* FIXME should only create pens for road class */

   for (i=1; i<MAX_LAYERS; ++i)
      for (j=0; j<MAX_PEN_LAYERS; j++)
         for (k=0; k<MAX_ROAD_STATES; k++) {

            editor_pen *pen = &EditorPens[i][j][k];

            pen->in_use = 0;

            snprintf (name, sizeof(name), "EditorPen%d", i*100+j*10+k);
            pen->pen = roadmap_canvas_create_pen (name);
            roadmap_canvas_set_foreground (editor_screen_get_pen_color(j,k));
            roadmap_canvas_set_thickness (1);
         }

   EditorTrackPens[0].pen = roadmap_canvas_create_pen ("EditorTrack0");
   roadmap_canvas_set_foreground ("black");
   roadmap_canvas_set_thickness (1);
   EditorTrackPens[1].pen = roadmap_canvas_create_pen ("EditorTrack1");
   roadmap_canvas_set_foreground ("blue");
   roadmap_canvas_set_thickness (1);

   roadmap_start_add_action ("properties", "Properties",
      "Update road properties", NULL, "Update road properties",
      editor_screen_update_segments);

   roadmap_start_add_action ("deleteroads", "Delete", "Delete selected roads",
      NULL, "Delete selected roads",
      editor_screen_delete_segments);

   screen_prev_after_refresh =
      roadmap_screen_subscribe_after_refresh (editor_screen_after_refresh);
}


int editor_screen_show_candidates (void) {
   return roadmap_config_match (&ShowCandidateRoads, "yes");
}

int editor_screen_gray_scale(void){
   return roadmap_config_match (&GrayScale, "yes");
}

const char *editor_screen_wazzy_name(void){
   return roadmap_config_get(&WazzyImage);
}
void editor_screen_set_override_car(const char *name){
   if (g_forced_car)
      free (g_forced_car);
   if (name){
      g_forced_car = strdup(name);
   }
   else
      g_forced_car = NULL;

   if (!roadmap_screen_refresh()){
      roadmap_screen_redraw();
   }

}

char *editor_screen_overide_car(){
	static char car_name[50];
	static int pacman = -1;
	static int road_roller = 1;
   static BOOL center_set = FALSE;
	RoadMapGpsPosition pos;

	if (!editor_screen_gray_scale()){
      roadmap_screen_move_center(0);
      return NULL;
	}

	if (g_forced_car)
	   return g_forced_car;

   if (editor_track_is_new_road()) {
      if (road_roller == 3)
         road_roller = 1;
      else
         road_roller++;
      sprintf(car_name,"road_roller0%d", road_roller);
      return &car_name[0];
   }

	if (!editor_track_is_new_direction_roads()){
		// If we are just starting a new segment and we were pacman
		// stay as pacman
		if ((pacman == -1) || (export_track_num_points() > 1)) {
			pacman = -1;
         if (center_set) { //TODO: move center handling to roadmap_view
            roadmap_screen_move_center(0);
            center_set = FALSE;
         }
		   return NULL;
		}
	}

	if (pacman == -1) pacman = 3;

   roadmap_screen_move_center(40);
   center_set = TRUE;
	roadmap_navigate_get_current(&pos, NULL, NULL);
	if (pos.speed == 0)
		sprintf(car_name,"%s%d", editor_screen_wazzy_name(), 1);
	else
		sprintf(car_name,"%s%d", editor_screen_wazzy_name(), pacman);
	if (pacman == 4)
		pacman = 3;
	else
		pacman++;
	return &car_name[0];
}
