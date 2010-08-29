/* editor_track.c - street databse layer
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
 * NOTE:
 * This file implements all the "dynamic" editor functionality.
 * The code here is experimental and needs to be reorganized.
 *
 * SYNOPSYS:
 *
 *   See editor_track.h
 */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "roadmap.h"
#include "roadmap_math.h"
#include "roadmap_gps.h"
#include "roadmap_fuzzy.h"
#include "roadmap_navigate.h"
#include "roadmap_start.h"
#include "roadmap_state.h"
#include "roadmap_layer.h"
#include "roadmap_screen.h"
#include "roadmap_square.h"
#include "roadmap_line.h"
#include "roadmap_line_route.h"
#include "roadmap_shape.h"
#include "roadmap_message.h"
#include "roadmap_ticker.h"
#include "../editor_main.h"
#include "../editor_points.h"
#include "../db/editor_db.h"
#include "../db/editor_trkseg.h"
#include "../db/editor_point.h"
#include "../db/editor_route.h"
#include "../db/editor_line.h"
#include "../db/editor_override.h"
#include "../db/editor_street.h"
#include "../editor_bar.h"
#include "../editor_log.h"
#include "editor_track_filter.h"
#include "editor_track_util.h"
#include "editor_track_known.h"
#include "editor_track_unknown.h"
#include "editor_track_compress.h"
#include "../static/editor_street_bar.h"
#include "../editor_screen.h"
#include "editor_track_report.h"
#include "roadmap_messagebox.h"
#include "Realtime/Realtime.h"
#include "roadmap_config.h"
#include "navigate/navigate_instr.h"
#include "Realtime/RealtimeDefs.h"
#include "editor_track_main.h"

#define GPS_POINTS_DISTANCE "10m"
#define MAX_POINTS_IN_SEGMENT 10000



#ifdef IPHONE
#define GPS_TIME_GAP 8
#else
#define GPS_TIME_GAP 4 /* seconds */
#endif

#define MAX_UPDATE_TOGGLES		40

typedef struct {

   RoadMapGpsPosition gps_point;
   time_t time;
   int status;
   int ordinal;

} TrackPoint;

static TrackPoint TrackPoints[MAX_POINTS_IN_SEGMENT];

static int cur_ordinal = 1;
static int points_count = 0;
static int points_start = 0;
static int cur_active_line = 0;
static RoadMapGpsPosition TrackLastPosition;
static time_t LastGpsUpdate = 0;

static RoadMapTracking  TrackConfirmedStreet = ROADMAP_TRACKING_NULL;
static RoadMapNeighbour TrackPreviousLine = ROADMAP_NEIGHBOUR_NULL;
static RoadMapNeighbour TrackConfirmedLine = ROADMAP_NEIGHBOUR_NULL;

static NodeNeighbour cur_node = NODE_NEIGHBOUR_NULL;

static int is_new_track = 1;
static int EditorAllowNewRoads = 0;

static int 		NumUpdateToggles = 0;
static time_t 	UpdateToggleTimes[MAX_UPDATE_TOGGLES];

static struct GPSFilter *TrackFilter = NULL;


static RoadMapConfigDescriptor RoadMapConfigStartWithRoadRoller =
                  ROADMAP_CONFIG_ITEM("Editor", "Start with Road Roller");

static RoadMapConfigDescriptor RoadMapConfigStartShortCuts =
                  ROADMAP_CONFIG_ITEM("Editor", "Editor shortcut");

static RoadMapConfigDescriptor RoadMapConfigEditorGrid =
                  ROADMAP_CONFIG_ITEM("Editor", "Editor grid");


RoadMapPosition* track_point_pos (int index) {

   return (RoadMapPosition*) &TrackPoints[index].gps_point;
}

RoadMapGpsPosition* track_point_gps (int index) {

   return &TrackPoints[index].gps_point;
}

time_t track_point_time (int index) {

   return TrackPoints[index].time;
}

int *track_point_status (int index) {

	return &TrackPoints[index].status;
}

int track_point_ordinal (int index) {

	return TrackPoints[index].ordinal;
}

RoadMapPosition* export_track_point_pos (int index) {

	assert (index + points_start < points_count);
   return (RoadMapPosition*) &TrackPoints[index + points_start].gps_point;
}

RoadMapGpsPosition* export_track_point_gps (int index) {

	assert (index + points_start < points_count);
   return &TrackPoints[index + points_start].gps_point;
}

time_t export_track_point_time (int index) {

	assert (index + points_start < points_count);
   return TrackPoints[index + points_start].time;
}

int *export_track_point_status (int index) {

	assert (index + points_start < points_count);
	return &TrackPoints[index + points_start].status;
}

int export_track_point_ordinal (int index) {

	assert (index + points_start < points_count);
	return TrackPoints[index + points_start].ordinal;
}

int export_track_is_new (void) {

	return is_new_track;
}


int export_track_num_points (void) {

	return points_count - points_start;
}

void export_track_reset_points (void) {

	points_start = points_count;
}

static void track_reset_points (int last_point_id) {

   if (last_point_id == 0) return;

	assert (editor_track_known_num_candidates() <= 1);

   if (last_point_id == -1) {
      points_count = 0;
      points_start = 0;
      return;
   }

   points_count -= last_point_id;

   assert(points_count >= 0);

   if (points_count < 0) {
   	points_count = 0;
   }

   if (points_count > 0) {
      memmove (TrackPoints, TrackPoints + last_point_id,
            sizeof(TrackPoints[0]) * points_count);
   }

   points_start -= last_point_id;
   if (points_start < 0) points_start = 0;
}


static int create_node(int point_id, NodeNeighbour *node) {

   int new_node;

   if (node && (node->id != -1)) {

      if (node->plugin_id == ROADMAP_PLUGIN_ID) {

         return editor_track_util_roadmap_node_to_editor (node);
      }

      return node->id;
   }


   new_node = editor_point_add (track_point_pos (point_id), -1);

   return new_node;
}


static int create_new_line (int gps_first_point,
                            int gps_last_point,
                            int from_point,
                            int to_point,
                            int cfcc) {

   int p_from;
   int p_to;
   int line_id;

   if (editor_track_util_create_db (track_point_pos (gps_last_point)) == -1) {

      editor_log (ROADMAP_ERROR, "create_new_line: can't create db.");
      return -1;
   }

   if (from_point != -1) {
      p_from = from_point;

   } else {

      p_from = create_node (gps_first_point, &cur_node);
      if (p_from == -1) {
         return -1;
      }
   }

   if (to_point != -1) {
      p_to = to_point;
   } else {

      RoadMapPosition start_pos;
      editor_point_position (p_from, &start_pos);

      /* check if first and last point are the same */
      if (!roadmap_math_compare_points
            (&start_pos, track_point_pos (gps_last_point))) {
         p_to = p_from;

      } else {
         p_to = editor_point_add (track_point_pos (gps_last_point), -1);

         if (p_to == -1) {
            return -1;
         }
      }
   }

   cur_node.id = p_to;
   cur_node.plugin_id = EditorPluginID;

   line_id = editor_track_util_create_line
               (gps_first_point, gps_last_point,
                p_from, p_to, cfcc, is_new_track);

   is_new_track = 0;

   return line_id;
}

#if 0
static int add_road_connection (int point_id,
                                RoadMapTracking *new_street,
                                RoadMapNeighbour *new_line) {

   int from_point;
   int end_point;
   int line_id;
   NodeNeighbour end_node = NODE_NEIGHBOUR_NULL;
   int road_type = 0;

   editor_log_push ("add_road_connection");

   from_point =
      editor_track_util_new_road_start
      (&TrackConfirmedLine,
       track_point_pos (point_id),
       point_id,
       TrackConfirmedStreet.line_direction,
       &cur_node);

   assert (from_point != -1);

   if (from_point == -1) {
      return -1;
   }

   if (editor_track_known_end_segment
         (&TrackPreviousLine.line, from_point,
          &TrackConfirmedLine.line,
          &TrackConfirmedStreet,
          is_new_track)) {

      is_new_track = 0;
   }

   track_reset_points (from_point);

   /*FIXME the whole previous line thing is not used and broken */
   TrackPreviousLine = TrackConfirmedLine;

   TrackConfirmedLine = *new_line;
   TrackConfirmedStreet = *new_street;

   end_point =
      editor_track_util_new_road_end
      (&TrackConfirmedLine,
       track_point_pos (points_count - 1),
       points_count - 1,
       TrackConfirmedStreet.line_direction,
       &end_node);

   if ((cur_node.plugin_id == ROADMAP_PLUGIN_ID) &&
       (end_node.plugin_id == ROADMAP_PLUGIN_ID)) {

      /* This a known connection road */
      road_type = 0;
   } else {
      road_type = ED_LINE_CONNECTION;
   }

   if (!EditorAllowNewRoads) {
      road_type = ED_LINE_CONNECTION;
   }

   if (end_node.plugin_id == ROADMAP_PLUGIN_ID) {
      end_node.id = editor_track_util_roadmap_node_to_editor (&end_node);
      end_node.plugin_id = EditorPluginID;
   }

   line_id = create_new_line (0, end_point, -1, end_node.id,
                              ROADMAP_ROAD_STREET);

   if (road_type == ED_LINE_CONNECTION) {

      if (line_id != -1) {

         editor_line_set_flag (line_id, ED_LINE_CONNECTION);
      }

   }

   track_reset_points (end_point);

   return from_point + end_point;
}
#endif

static void editor_track_set_fuzzy (void) {
   roadmap_fuzzy_set_cycle_params (40, 150);
}

static int handle_new_direction (const PluginLine *line, void *context, int extend_flags) {
	roadmap_square_set_current(line->square);

	editor_override_line_set_flag (line->line_id, line->square, ED_LINE_NEW_DIRECTION);
	return 0;
}

static int end_known_segment (int point_id,
                              RoadMapTracking *new_street,
                              RoadMapNeighbour *new_line) {

   int fips;
   int current_points_count = points_count;

   if (!TrackConfirmedStreet.valid) {
      TrackConfirmedLine = *new_line;
      TrackConfirmedStreet = *new_street;
      return point_id;
   }

   fips = roadmap_plugin_get_fips (&TrackConfirmedLine.line);

   if (editor_db_activate (fips) == -1) {

      editor_db_create (fips);
      if (editor_db_activate (fips) == -1) {
         roadmap_log (ROADMAP_ERROR, "Can't end known segment.");

         track_reset_points (-1);
         TrackConfirmedLine = *new_line;
         TrackConfirmedStreet = *new_street;
         return current_points_count - 1;
      }
   }

   if (new_street->valid) {
	  int split_point;
      /* we have just switched from one known street to another */

	  if (TrackConfirmedStreet.valid && (TrackConfirmedLine.line.plugin_id == ROADMAP_PLUGIN_ID) && editor_screen_gray_scale()){
    	 int low_weight;
    	 roadmap_square_set_current(TrackConfirmedLine.line.square);
		 low_weight = roadmap_line_route_is_low_weight (TrackConfirmedLine.line.line_id);
		 if (low_weight){
			int length;
			RoadMapPosition line_from;
    		RoadMapPosition line_to;
    		int flags;
    		if (editor_override_line_get_flags(TrackConfirmedLine.line.line_id, TrackConfirmedLine.line.square, &flags) == -1){
    		   length = roadmap_line_length (TrackConfirmedLine.line.line_id);
    		   if (length > 0){
    		      editor_points_add(length);

    		      roadmap_line_from (TrackConfirmedLine.line.line_id, &line_from);
    		      roadmap_line_to (TrackConfirmedLine.line.line_id, &line_to);
    		      roadmap_square_set_current (TrackConfirmedLine.line.square);
    		      editor_override_line_set_flag (TrackConfirmedLine.line.line_id, TrackConfirmedLine.line.square,  ED_LINE_NEW_DIRECTION);
    		   }
    		   //roadmap_street_extend_line_ends (&TrackConfirmedLine.line, &line_from, &line_to, FLAG_EXTEND_BOTH, handle_new_direction, NULL);
    		}
		 }
		 else{
		    editor_points_hide();
		 }
      }

      split_point = editor_track_util_connect_roads
                           (&TrackConfirmedLine.line,
                            &new_line->line,
                            TrackConfirmedStreet.line_direction,
                            new_street->line_direction,
                            track_point_gps (point_id),
                            point_id);

      if (split_point != -1) {

         if (editor_track_known_end_segment
               (&TrackPreviousLine.line, split_point,
                &TrackConfirmedLine.line,
                &TrackConfirmedStreet, is_new_track)) {

            is_new_track = 0;

            /*FIXME the whole previous line thing is not used and broken */
            TrackPreviousLine = TrackConfirmedLine;
         }

      } else {

         /* We can't just connect the two roads.
          * We need to create a new road in between.
          */

#if 0
         split_point = add_road_connection (point_id, new_street, new_line);
#else
			split_point = -1;
#endif
         if (split_point == -1) {
            TrackConfirmedLine = *new_line;
            TrackConfirmedStreet = *new_street;
            editor_track_util_create_trkseg (-1, -1, ROADMAP_PLUGIN_ID, 0, point_id, ED_TRKSEG_IGNORE);
            track_reset_points (point_id);
            //track_reset_points (-1);
            editor_track_known_reset_resolve ();
            return current_points_count - 1;
         }

         is_new_track = 0;
         TrackPreviousLine = TrackConfirmedLine;
         return split_point;
      }

      track_reset_points (split_point);

      TrackConfirmedLine = *new_line;
      TrackConfirmedStreet = *new_street;

      return split_point;
   }


   /* we are not on a known road */

   if (TrackConfirmedStreet.valid) {

      int split_point;

      /* we just lost a known road, let's find the best point
         to start this new road from */

      split_point =
         editor_track_util_new_road_start
                     (&TrackConfirmedLine,
                      track_point_pos (point_id),
                      point_id,
                      TrackConfirmedStreet.line_direction,
                      &cur_node);

      if (split_point != -1) {
         if (editor_track_known_end_segment
               (&TrackPreviousLine.line, split_point,
                &TrackConfirmedLine.line,
                &TrackConfirmedStreet,
                is_new_track)) {

            is_new_track = 0;
            TrackPreviousLine = TrackConfirmedLine;
         }
      }

      track_reset_points (split_point);

      TrackConfirmedLine = *new_line;
      TrackConfirmedStreet = *new_street;

      return split_point;
   }

	assert(0);
	return point_id;
}


static void end_unknown_segments (TrackNewSegment *new_segments, int count) {

   int i;
   int start_point = 0;
   NodeNeighbour end_node = NODE_NEIGHBOUR_NULL;

   editor_track_util_create_db (track_point_pos (start_point));

   for (i=0; i<count; i++) {

      int type = new_segments[i].type;
      int end_point = new_segments[i].point_id;
      int end_node_id = -1;

      switch (type) {

         case TRACK_ROAD_TURN:

            if (editor_track_util_length (start_point, end_point) <
                  (editor_track_point_distance () * 2)) {

               RoadMapPosition pos;
               RoadMapPosition *pos1;
               RoadMapPosition *pos2;

               pos1 = track_point_pos (start_point);
               pos2 = track_point_pos (end_point);
               pos.longitude = (pos1->longitude + pos2->longitude) / 2;
               pos.latitude = (pos1->latitude + pos2->latitude) / 2;

               if (cur_node.plugin_id == ROADMAP_PLUGIN_ID) {
                  cur_node.id = editor_track_util_roadmap_node_to_editor (&cur_node);
                  cur_node.plugin_id = EditorPluginID;
               } else {
	               editor_point_set_pos (cur_node.id, &pos);
               }

               start_point = end_point;
               continue;
            }

            break;

         case TRACK_ROAD_ROUNDABOUT:

            if (cur_node.plugin_id == ROADMAP_PLUGIN_ID) {
               cur_node.id = editor_track_util_roadmap_node_to_editor (&cur_node);
               cur_node.plugin_id = EditorPluginID;
            }

            create_new_line (start_point, end_point, cur_node.id, cur_node.id,
                             ROADMAP_ROAD_STREET);

            start_point = end_point;
            continue;

            break;
      }

      if ((i == (count - 1)) && (TrackConfirmedStreet.valid)) {

         /* we are connecting to a known road */
         end_point =
            editor_track_util_new_road_end
                     (&TrackConfirmedLine,
                      track_point_pos (end_point),
                      end_point,
                      TrackConfirmedStreet.line_direction,
                      &end_node);

         if (end_node.plugin_id == ROADMAP_PLUGIN_ID) {
            end_node.id = editor_track_util_roadmap_node_to_editor (&end_node);
            end_node.plugin_id = EditorPluginID;
         }

         end_node_id = end_node.id;
      }

      if ((i < (count -1)) || (start_point != (end_point -1))) {
         int line_id = create_new_line (start_point, end_point, -1,
                                        end_node_id, ROADMAP_ROAD_STREET);
         if ((line_id != -1) &&
               ((type == TRACK_ROAD_CONNECTION) || !EditorAllowNewRoads)) {

				editor_line_set_flag (line_id, ED_LINE_CONNECTION);
         }
      }

      start_point = end_point;
   }

   track_reset_points (start_point);
}


static void track_rec_locate_point(int point_id, int point_type) {

   int i;
   int count;
   TrackNewSegment new_segments[10];

   assert (!(point_type & POINT_UNKNOWN) || cur_active_line);

	//track_verify_square (point_id);


   if (!cur_active_line) {

      RoadMapTracking new_street;
      RoadMapNeighbour new_line;

      do {
         int prev_square = -1;
         int prev_line = -1;

         if (TrackConfirmedStreet.valid) {
	         prev_square = TrackConfirmedLine.line.square;
	         prev_line = TrackConfirmedLine.line.line_id;
         }

         count = editor_track_known_locate_point
            (point_id,
             &TrackLastPosition,
             &TrackConfirmedStreet,
             &TrackConfirmedLine,
             &new_street,
             &new_line);

         if (count) {

            if (count == -1) {

               /* Error, graceful end track */

               TrackConfirmedLine = new_line;
               TrackConfirmedStreet = new_street;

               cur_node.id = -1;
               is_new_track = 1;
            } else {

	            int consumed = end_known_segment (count, &new_street, &new_line);
	            if (new_street.valid && (new_line.line.plugin_id == ROADMAP_PLUGIN_ID) && editor_screen_gray_scale()){

		  	    		int points;

    	    			roadmap_square_set_current (TrackConfirmedLine.line.square);
	      	    	points = roadmap_line_length (TrackConfirmedLine.line.line_id);
                  if ((!roadmap_line_route_is_low_weight (TrackConfirmedLine.line.line_id)) || editor_override_exists(TrackConfirmedLine.line.line_id, TrackConfirmedLine.line.square )){
                     editor_points_hide();
                  }
	         	}
	         	else{
	         	   editor_points_hide();
	         	}

               if (!new_street.valid) {
                  /* the current point does not belong to a known street */
                  assert(!editor_track_known_num_candidates());
                  editor_track_known_reset_resolve();
                  cur_active_line = 1;

                  for (i=0; i<points_count; i++) {

                     track_rec_locate_point (i, POINT_UNKNOWN|point_type);
                  }

                  break;
               }

               if (count > consumed) {
                  /* In case of a resolver, the resolver already assumes
                   * about the number of points which match each segment.
                   * So here we make it be.
                   */
                  track_reset_points(count - consumed);
               } else if (consumed > count) {
                  assert(!editor_track_known_num_candidates());
                  count = consumed;
               }

               point_id -= count;
            }
         } else if ((prev_square != -1) && (prev_square != TrackConfirmedLine.line.square)) {

               /*printf ("crossed from tile %d line %d to tile %d line %d\n",
                        prev_square, prev_line,
                        TrackConfirmedLine.line.square,
                        TrackConfirmedLine.line.line_id); */
               if (TrackConfirmedStreet.valid && (TrackConfirmedLine.line.plugin_id == ROADMAP_PLUGIN_ID) && editor_screen_gray_scale()){
                 int low_weight;
                 roadmap_square_set_current(prev_square);
                 low_weight = roadmap_line_route_is_low_weight (prev_line);
                 if (low_weight){
                   int length;
                   RoadMapPosition line_from;
                   RoadMapPosition line_to;
                   int flags;
                   if (editor_override_line_get_flags(prev_line, prev_square, &flags) == -1){
                      length = roadmap_line_length (prev_line);
                      if (length > 0){
                         editor_points_add(length);

                         roadmap_line_from (prev_line, &line_from);
                         roadmap_line_to (prev_line, &line_to);
                         roadmap_square_set_current (prev_square);
                         editor_override_line_set_flag (prev_line, prev_square,  ED_LINE_NEW_DIRECTION);
                         roadmap_street_extend_line_ends (&TrackConfirmedLine.line, &line_from, &line_to, FLAG_EXTEND_BOTH, handle_new_direction, NULL);
                      }
                   }
                 }
                }


         }
      } while (points_count && count && editor_track_known_resolve());

   } else {
      count = editor_track_unknown_locate_point
               (point_id,
                &TrackLastPosition,
                &TrackConfirmedStreet,
                &TrackConfirmedLine,
                new_segments,
                sizeof(new_segments) / sizeof(new_segments[0]),
                point_type);

      if (count) {

         int num_points;

         if ((point_id == 0) && TrackConfirmedStreet.valid) {
            cur_active_line = 0;
            return;
         }

         assert (point_id > 0);
         end_unknown_segments (new_segments, count);

         if (TrackConfirmedStreet.valid) {

            /* the current point is a known street */
            cur_active_line = 0;
         }

         /* After creating a new line, we need to check if the current
          * point_is still unknown.
          */

         num_points = points_count;
         for (i=0; i<points_count; i++) {

            track_rec_locate_point (i, point_type);
            if (points_count != num_points) {
               // The inner call has created a new line and further processed
               // all existing points.
               break;
            }
         }
      }
   }
}


static void track_filter_init (void) {

   if (TrackFilter == NULL) {

      TrackFilter = editor_track_filter_new
         (roadmap_math_distance_convert ("1000m", NULL),
          600, /* 10 minutes */
          GPS_TIME_GAP, /* used to be 60 */
          roadmap_math_distance_convert ("10m", NULL));
   }

}



const struct GPSFilter *editor_track_get_gps_filter (void) {

	track_filter_init ();
	return TrackFilter;

}



void editor_track_toggle_new_roads (void) {
#ifdef IPHONE
	roadmap_main_show_root(0);
#endif //IPHONE
   if ( Realtime_is_random_user() && (!roadmap_config_match(&RoadMapConfigStartWithRoadRoller, "Always"))){
      roadmap_messagebox ("Error", "Sorry, only registered users can record new roads. Go to Settings > Profile to register.");
      return;
   }

   if (EditorAllowNewRoads) EditorAllowNewRoads = 0;
   else EditorAllowNewRoads = 1;

   ssd_dialog_hide_all(dec_close);


   if (EditorAllowNewRoads) {
       editor_track_set_fuzzy ();
       roadmap_messagebox_timeout("New roads", "New Road Recording is ON. Please turn it off once you've finished recording a new road.",8);
       editor_bar_show();
   } else {
      if (editor_line_get_count () > 0)
         roadmap_messagebox_timeout ("New roads", "Road successfully recorded.  A link to edit it will be available on your Dashboard at www.waze.com. (May take 24 hrs.)", 8);
      else
         roadmap_messagebox_timeout("New roads", "New Road Recording is now OFF",5);

       editor_bar_hide();
       roadmap_fuzzy_reset_cycle ();
   }

   if (NumUpdateToggles == MAX_UPDATE_TOGGLES) {

   	NumUpdateToggles--;
   	memmove (UpdateToggleTimes, UpdateToggleTimes + 1, NumUpdateToggles * sizeof (UpdateToggleTimes[0]));
   }
   UpdateToggleTimes[NumUpdateToggles++] = LastGpsUpdate;

   roadmap_screen_redraw ();
}

int editor_track_get_num_update_toggles (void) {

	return NumUpdateToggles;
}

const time_t *editor_track_get_update_toggle_times (void) {

	return UpdateToggleTimes;
}

int editor_track_get_update_toggle_state (int index) {

	return EditorAllowNewRoads ^ ((NumUpdateToggles - 1 - index) % 2);
}

void editor_track_reset_update_toggles (void) {

	NumUpdateToggles = 0;
}

static int editor_new_roads_state(void){
#ifdef TOUCH_SCREEN
   if ((EditorAllowNewRoads) && !editor_bar_feature_enabled())
      return 1;
   else
      return roadmap_screen_show_icons_only_when_touched();
#else
   if (EditorAllowNewRoads)
      return 0;
   else
      return -1;
#endif
}

int editor_ignore_new_roads (void) {

   return !EditorAllowNewRoads;
}

int editor_track_point_distance (void) {

   static int distance = -1;

   if (distance == -1) {

      distance = roadmap_math_distance_convert (GPS_POINTS_DISTANCE, NULL);
   }

   return distance;
}


static void editor_track_force_reset (void) {

   if (points_count > 1) {

      if (cur_active_line) {

         TrackNewSegment segment;

         segment.point_id = points_count - 1;
         segment.type = TRACK_ROAD_REG;

         end_unknown_segments (&segment, 1);
         cur_active_line = 0;
      } else {

         if (TrackConfirmedStreet.valid) {
            editor_track_known_end_segment
               (&TrackPreviousLine.line,
                points_count - 1,
                &TrackConfirmedLine.line,
                &TrackConfirmedStreet,
                is_new_track);
         }
      }
   }
   TrackPreviousLine = TrackConfirmedLine;
   TrackConfirmedStreet.valid = 0;
   editor_track_known_reset_resolve ();
   track_reset_points (points_count);
   cur_node.id = -1;
}


static int editor_track_add_point(RoadMapGpsPosition *point, time_t time) {

   if (points_count == MAX_POINTS_IN_SEGMENT) {

   	roadmap_log (ROADMAP_WARNING, "Reached maximum number of track points (%d)", points_count);

		editor_track_force_reset ();

		if (points_count == MAX_POINTS_IN_SEGMENT) {

			/* this really should not happen */
   		roadmap_log (ROADMAP_ERROR, "Still has maximum number of track points (%d)", points_count);
			points_count--;
		}
   }

   TrackPoints[points_count].gps_point = *point;
   TrackPoints[points_count].time = time;
   TrackPoints[points_count].status = POINT_STATUS_IGNORE;
   TrackPoints[points_count].ordinal = cur_ordinal++;

   return points_count++;
}


static void track_rec_locate(time_t gps_time,
                             const RoadMapGpsPrecision *dilution,
                             const RoadMapGpsPosition* gps_position) {

   const RoadMapGpsPosition *filtered_gps_point;
   RoadMapPosition context_save_pos;
   int context_save_zoom;
   int point_id;
   int res;

   LastGpsUpdate = gps_time;

   track_filter_init ();

   roadmap_math_get_context (&context_save_pos, &context_save_zoom);
   roadmap_math_set_context ((RoadMapPosition *)gps_position, 20);
   editor_track_util_set_focus ((RoadMapPosition *)gps_position);

   res = editor_track_filter_add (TrackFilter, gps_time, dilution, gps_position);

   if (res == ED_TRACK_END) {

      /* The current point is too far from the previous point, or
       * the time from the previous point is too long.
       * This is probably a new GPS track.
       */

      editor_track_end ();
   }

   if (!EditorAllowNewRoads) editor_track_set_fuzzy ();

   while ((filtered_gps_point = editor_track_filter_get (TrackFilter)) != NULL) {

      TrackLastPosition = *filtered_gps_point;

      point_id = editor_track_add_point(&TrackLastPosition, gps_time);

      roadmap_fuzzy_set_cycle_params (40, 150);

      track_rec_locate_point (point_id, 0);
   }

   if (!EditorAllowNewRoads) roadmap_fuzzy_reset_cycle ();
   editor_track_util_release_focus ();
   roadmap_math_set_context (&context_save_pos, context_save_zoom);
}

#ifdef IPHONE_NATIVE
static void check_street_name (const RoadMapGpsPosition *gps_position) {
   static BOOL isTracking = FALSE;
   RoadMapStreetProperties properties;
   const char *street_name = "skip";
   static int last_line = -1;
   static int last_square = -1;

   if (TrackConfirmedStreet.valid) {
      if (last_line == TrackConfirmedLine.line.line_id &&
          last_square == TrackConfirmedLine.line.square)
         return;

      last_line = TrackConfirmedLine.line.line_id;
      last_square = TrackConfirmedLine.line.square;

      if (TrackConfirmedLine.line.plugin_id == EditorPluginID) {
         if (editor_db_activate (TrackConfirmedLine.line.fips) != -1) {
            int street_id = -1;
            editor_line_get_street (TrackConfirmedLine.line.line_id, &street_id);
            street_name = editor_street_get_street_fename (street_id);
         }
      } else {
         roadmap_square_set_current (TrackConfirmedLine.line.square);
         roadmap_street_get_properties(TrackConfirmedLine.line.line_id, &properties);
         street_name = roadmap_street_get_street_name(&properties);
      }
      if (!street_name || street_name[0] == 0) {
         editor_street_bar_track(&TrackConfirmedLine.line, gps_position, FALSE);
         isTracking = TRUE;
         return;
      }
   }

   if (isTracking) {
      editor_street_bar_stop();
      isTracking = FALSE;
   }
}
#endif

static void
editor_gps_update (time_t gps_time,
                   const RoadMapGpsPrecision *dilution,
                   const RoadMapGpsPosition *gps_position) {

   if (editor_is_enabled()) {
      track_rec_locate(gps_time, dilution, gps_position);
#ifdef IPHONE_NATIVE
      check_street_name(gps_position);
#endif
   }
}



void editor_track_initialize (void) {

   roadmap_start_add_action ("togglenewroads",TRACK_RECORD_ROADS_LABEL , NULL, NULL,
      "Allow / prevent automatic creation of new roads",
      editor_track_toggle_new_roads);

   roadmap_state_add ("new_roads", &editor_new_roads_state);

   roadmap_gps_register_listener(editor_gps_update);

   editor_track_report_init ();

   roadmap_config_declare_enumeration
       ("preferences", &RoadMapConfigStartWithRoadRoller, NULL,  "Never", "Always", "Non-random", NULL);

   roadmap_config_declare_enumeration
       ("preferences", &RoadMapConfigStartShortCuts, NULL,  "map-updates", "record", NULL);

   roadmap_config_declare_enumeration
      ("preferences", &RoadMapConfigEditorGrid, NULL,  "no", "yes", NULL);

   /*
    * Start recording if
    * 1. config is set to always
    * 2. config is set to non-random, and this is both not a first time login (has user name) and not a random user.
    * - D.F.
    */
   if ((roadmap_config_match(&RoadMapConfigStartWithRoadRoller, "Always"))||
   (roadmap_config_match(&RoadMapConfigStartWithRoadRoller, "Non-random")&&(!Realtime_is_random_user())&&
   (strcmp(RealTime_GetUserName(),RT_CFG_PRM_NAME_Default))))
   {
   	    EditorAllowNewRoads =1;
   	    editor_track_set_fuzzy ();
   		editor_bar_show();
   }


}


int editor_track_shortcut(void){
   if (navigate_main_alt_routes_display())
      return 0;

   if (roadmap_config_match(&RoadMapConfigEditorGrid, "yes"))
      return 3;
   else if (roadmap_config_match(&RoadMapConfigStartShortCuts, "record"))
      return 2;
   else
      return 1;
}

void editor_track_end (void) {

	editor_track_force_reset ();
   is_new_track = 1;
}


void editor_track_reset (void) {

   TrackConfirmedStreet.valid = 0;
   track_reset_points (points_count);
   cur_node.id = -1;
   is_new_track = 1;
}


static void editor_track_shape_position (int shape, RoadMapPosition *position) {

   assert (shape < points_count);

   *position = *track_point_pos (shape);
}


int editor_track_is_new_road(){
   if (!editor_ignore_new_roads() || editor_screen_show_candidates())
      if ((points_count > 2) && (cur_active_line))
         return TRUE;

   return FALSE;

}

int editor_track_draw_current (RoadMapPen pen) {

   RoadMapPosition *from;
   RoadMapPosition *to;
   int first_shape = -1;
   int last_shape = -2;
   int distance;

   if (points_count < 2) return 0;

   if (pen == NULL) return 0;

   if (!cur_active_line) return 0;

   from = track_point_pos (0);
   to = track_point_pos (points_count-1);

   if (points_count > 2) {

      first_shape = 1;
      last_shape = points_count - 2;
   }
   distance = roadmap_math_distance (from, to);
   editor_bar_set_temp_length(distance);

   roadmap_screen_draw_one_line
               (from, to, 0, from, first_shape, last_shape,
                editor_track_shape_position, &pen, 1, -1, 0, 0 ,0);
   return 1;
}


int editor_track_draw_current_new_direction_road(){
  	RoadMapPen layer_pens[LAYER_PROJ_AREAS];
 	NavigateSegment segment;
 	RoadMapPosition intersection;
 	int distance;
 	int i;

  	if (points_count < 1) return 0;

  	roadmap_square_set_current(TrackConfirmedLine.line.square);
  	if (!roadmap_line_route_is_low_weight (TrackConfirmedLine.line.line_id)) {
  	   return 0;
	}

	segment.square = TrackConfirmedLine.line.square;
	segment.line = TrackConfirmedLine.line.line_id;
	roadmap_line_from(TrackConfirmedLine.line.line_id, &segment.from_pos);
	roadmap_line_to(TrackConfirmedLine.line.line_id, &segment.to_pos);

	roadmap_line_shapes (TrackConfirmedLine.line.line_id, &segment.first_shape, &segment.last_shape);
	segment.shape_initial_pos = segment.from_pos;

   	for (i = 0; i < LAYER_PROJ_AREAS; i++) {
      			layer_pens[i] = roadmap_layer_get_pen (roadmap_line_cfcc(TrackConfirmedLine.line.line_id),1, i);
	}

	navigate_instr_fix_line_end(track_point_pos(points_count - 1), &segment,
							TrackConfirmedStreet.line_direction == ROUTE_DIRECTION_WITH_LINE ? LINE_END : LINE_START);

   	if (!editor_override_exists(TrackConfirmedLine.line.line_id, TrackConfirmedLine.line.square)){
   		distance =
            roadmap_math_get_distance_from_segment
            (track_point_pos(points_count - 1), &segment.from_pos, &segment.to_pos, &intersection, NULL);


		if (TrackConfirmedStreet.line_direction == ROUTE_DIRECTION_WITH_LINE)
   			distance = roadmap_math_distance (&intersection, &segment.from_pos);
   		else
   			distance = roadmap_math_distance (&intersection, &segment.to_pos);
	    roadmap_ticker_set_last_event(road_munching_event);
   		editor_points_display(distance);
   	}
   roadmap_screen_draw_one_line
               (&segment.from_pos, &segment.to_pos, 0, &segment.shape_initial_pos, segment.first_shape, segment.last_shape,
                NULL, layer_pens, LAYER_PROJ_AREAS, -1, 0, 0, 0);

	return 0;
}

int editor_track_draw_new_direction_roads () {
  int k;
  int count = editor_override_get_count();

  if (TrackConfirmedStreet.valid)
     editor_track_draw_current_new_direction_road();
   	 for (k=0; k<count; k++) {
   		int line_id, square, flags, direction;

   		if (!editor_override_get(k, &line_id, &square, &direction, &flags)) continue;

   		if (flags & ED_LINE_NEW_DIRECTION){
			   RoadMapPosition from;
         	RoadMapPosition to;
         	int first_shape;
         	int last_shape;
         	RoadMapPen layer_pens[LAYER_PROJ_AREAS];
         	int i;
         	if (!roadmap_square_set_current(square))
         	   continue;

         	if (line_id >= roadmap_line_count() || line_id < 0)
         	   continue;

         	roadmap_line_shapes (line_id, &first_shape, &last_shape);


            roadmap_line_from (line_id, &from);
            roadmap_line_to (line_id, &to);


            if (!roadmap_layer_is_visible (roadmap_line_cfcc(line_id), 0))
            	continue;

            if (!roadmap_math_line_is_visible (&from, &to))
            	continue;

         	for (i = 0; i < LAYER_PROJ_AREAS; i++) {
      			layer_pens[i] = roadmap_layer_get_pen (roadmap_line_cfcc(line_id),1, i);
   			}

            roadmap_screen_draw_one_line
               (&from, &to, 0,
                &from, first_shape, last_shape,
                NULL, layer_pens, LAYER_PROJ_AREAS, -1, 0, 0, 0);

   	    }
   	 }
   return 0;
}

int editor_track_deflate (void) {

	int i;
	int j;

	if (points_count <= points_start) return 0;

	editor_track_compress_track (points_start, points_count - 1);

	for (i = points_start, j = 0; i < points_count; i++) {
		if (TrackPoints[i].status == POINT_STATUS_SAVE) j++;
	}

	return j;
}

int editor_track_is_new_direction_roads (){
   PluginLine line;
	static int last_state;

	if (!TrackConfirmedStreet.valid){
		last_state = FALSE;
		return FALSE;
	}

	if (editor_override_exists(TrackConfirmedLine.line.line_id, TrackConfirmedLine.line.square)) {
		last_state = FALSE;
		return FALSE;
	}else{
		 int direction;
		 roadmap_square_set_current(TrackConfirmedLine.line.square);

   	 if ((roadmap_navigate_get_current(NULL, &line, &direction) == -1) ||
					(!roadmap_plugin_same_line(&line, &TrackConfirmedLine.line)))
				  return last_state;

		 if (!roadmap_line_route_is_low_weight (TrackConfirmedLine.line.line_id)) {
		 	 last_state = FALSE;
          return FALSE;
		 }

		 last_state = TRUE;
		 return TRUE;
	}
}

char * editor_track_main_getRecordRoadsLabel(){
	if (EditorAllowNewRoads)
		return TRACK_DONT_RECORD_ROADS_LABEL;
	else
		return TRACK_RECORD_ROADS_LABEL;
}
