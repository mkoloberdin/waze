/* editor_track_report.c - prepare track data for export
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
 *   See editor_track_report.h
 */

#include <stdlib.h>
#include <string.h>
#include "time.h"

#include "roadmap.h"
#include "roadmap_plugin.h"
#include "roadmap_point.h"
#include "roadmap_line.h"
#include "roadmap_square.h"
#include "roadmap_navigate.h"
#include "roadmap_line_route.h"

#include "../editor_log.h"
#include "../editor_main.h"

#include "../db/editor_point.h"
#include "../db/editor_shape.h"
#include "../db/editor_line.h"
#include "../db/editor_trkseg.h"

#include "editor_track_main.h"
#include "editor_track_util.h"
#include "editor_track_report.h"

static int EditorReportTrksegsId = -1;
static int EditorReportTrksegsInProgress = 0;
static int LastReportedNode = -1;
static int PendingLastNode = -1;
static int LastOrdinal = 0;
static int PendingOrdinal = 0;

static RTPathInfo		PathInfo;

void editor_track_report_init (void) {

	PathInfo.max_nodes = 0;
	PathInfo.max_points = 0;
   PathInfo.max_user_points = 0;
	PathInfo.nodes = NULL;
	PathInfo.points = NULL;
	PathInfo.num_nodes = 0;
	PathInfo.num_points = 0;
   PathInfo.user_points = NULL;
   PathInfo.num_user_points = 0;
}


void editor_track_report_reset (void) {
	
	LastReportedNode = -1;
	PendingLastNode = -1;
	LastOrdinal = 0;
	PendingOrdinal = 0;
}

static int editor_track_report_prepare_export (int offline, int *num_nodes, int *num_points, int *num_toggles, BOOL *has_user_points) {

	int i;
	int nodes = 0;
	int points = 0;
	int toggles = 0;

	if (EditorReportTrksegsInProgress) {
		return 0;
	}
   
   *has_user_points = FALSE;

	EditorReportTrksegsId = editor_trkseg_begin_commit();
	if (editor_trkseg_items_pending ()) {

		int first_shape;
		int last_shape;
		int flags;
		int last_node = LastReportedNode;
		int count = editor_trkseg_get_count ();
		int from_id;
		int to_id;
		int recording_mode = 0;
      int user_points;
      int square_ver;

		for (i = 0; i < count; i++) {

			if (editor_trkseg_item_committed (i)) continue;

			editor_trkseg_get (i, NULL, &first_shape, &last_shape, &flags);
			if (!(flags & ED_TRKSEG_FAKE)) {

				if (first_shape >= 0) {
					points += (last_shape - first_shape + 1);
				}

				if (flags & ED_TRKSEG_NEW_TRACK) {
					points ++;
				}

				if ((flags & ED_TRKSEG_RECORDING_ON) != recording_mode) {
					recording_mode = flags & ED_TRKSEG_RECORDING_ON;
					if (offline) {
						toggles++;
					}
				}
			}

			if (!(flags & (ED_TRKSEG_FAKE | ED_TRKSEG_IGNORE))) {

				editor_trkseg_get_points (i, &from_id, &to_id, &user_points, &square_ver);
            
            if (user_points > 0)
               *has_user_points = TRUE;
																	 
				nodes ++;
				if (!(flags & (ED_TRKSEG_NEW_TRACK | ED_TRKSEG_LOW_CONFID)) &&
					 from_id != last_node) {
					nodes ++;
				}
				if (flags & ED_TRKSEG_LOW_CONFID)
               to_id = -1;
				last_node = to_id;
			}
		}
	}

	if (export_track_is_new ()) {
		points += 1;
	}

	points += editor_track_deflate ();

	/* add space for optional last gps point */
	points++;

	if (!offline) {
		toggles = editor_track_get_num_update_toggles ();
	}

	*num_nodes = nodes;
	*num_points = points;
	*num_toggles = toggles;

	return nodes + points + toggles;
}


static void editor_track_report_get_points (int offline,
														  LPNodeInTime nodes,
														  int *num_nodes,
														  LPGPSPointInTime points,
														  int *num_points,
														  time_t *toggle_times,
														  int *num_toggles,
														  int *first_toggle,
                                            BOOL has_user_points,
                                            LPUserPointsVer user_points,
                                            int *num_user_points) {

	/* should be called immediately after editor_export_prepare_track() */
	int i;
	int j;
	int node = 0;
	int point = 0;
	int max_nodes = *num_nodes;
	int max_points = *num_points;
	int max_toggles = *num_toggles;
	int num_extra;
	int count;
	RoadMapPosition last_gps_pos;
	time_t last_gps_time;
	int ordinal;
	int toggle = 0;
	int recording_mode = 0;

	PendingLastNode = LastReportedNode;
	PendingOrdinal = LastOrdinal;

	/* loop over all trksegs */
	if (editor_trkseg_items_pending ()) {

		int first_shape;
		int last_shape;
		int flags;
		int from;
		RoadMapPosition from_pos;
		int from_id;
		int to_id;
      int seg_user_points;
      int square_ver;
		time_t start_time;
		time_t shape_time;
		time_t end_time;
      int shape_altitude;

		count = editor_trkseg_get_count ();
		for (i = 0; i < count; i++) {

			if (editor_trkseg_item_committed (i)) continue;

			/* extract details of trkseg */

			editor_trkseg_get (i, &from, &first_shape, &last_shape, &flags);
			if (flags & ED_TRKSEG_FAKE) continue;

         editor_point_position (from, &from_pos);
         editor_trkseg_get_time (i, &start_time, &end_time);

			/* add all points */
			if (first_shape < 0)
				last_shape = first_shape - 1;

			shape_time = start_time;
			for (j = first_shape; j <= last_shape; j++) {

				ordinal = editor_shape_ordinal (j);
				editor_shape_position (j, &from_pos);
				editor_shape_time (j, &shape_time);
            shape_altitude = editor_shape_altitude(j);

				if (ordinal > PendingOrdinal) {
			      /* on new track, add a null point */
			      if (j == first_shape && (flags & ED_TRKSEG_NEW_TRACK)) {

			      	if (point >= max_points) {
		         		editor_log (ROADMAP_FATAL, "not enough space (%d) for GPSPath", max_points);
			      	}
			      	points[point].Position.longitude = INVALID_COORDINATE;
			      	points[point].Position.latitude = INVALID_COORDINATE;
                  points[point].altitude = INVALID_COORDINATE;
			      	points[point].GPS_time = 0;
			      	point++;
			      }

	      		if (offline && j == first_shape &&
	      			 recording_mode != (flags & ED_TRKSEG_RECORDING_ON)) {

	      			recording_mode = flags & ED_TRKSEG_RECORDING_ON;
	      			if (toggle >= max_toggles) {
	      				editor_log (ROADMAP_FATAL, "not enough space (%d) for recording togles", max_toggles);
	      			}
	      			if (toggle == 0) {
	      				*first_toggle = recording_mode ? 1 : 0;
	      			}
	      			toggle_times[toggle++] = start_time;
	      		}

		      	if (point >= max_points) {
	         		editor_log (ROADMAP_FATAL, "not enough space (%d) for GPSPath", max_points);
		      	}
					points[point].Position = from_pos;
               points[point].altitude = shape_altitude;
					points[point].GPS_time = shape_time;
					point++;
					PendingOrdinal = ordinal;
				}
			}

			if (flags & ED_TRKSEG_IGNORE) continue;

			editor_trkseg_get_points (i, &from_id, &to_id, &seg_user_points, &square_ver); 
				      
	      if ((flags & (ED_TRKSEG_NEW_TRACK | ED_TRKSEG_LOW_CONFID)) == 0 &&
	      	 from_id != PendingLastNode) {

	      	/* add from node */

	      	if (node >= max_nodes) {
         		editor_log (ROADMAP_FATAL, "not enough space (%d) for NodePath", max_nodes);
	      	}

	      	nodes[node].node = from_id;
	      	nodes[node].GPS_time = start_time;
	      	
            if (has_user_points) {
               user_points[node].points = 0;
               user_points[node].version = 0;
            }
            
            node++;
	      }

			/* add to node */
      	if (node >= max_nodes) {
      		editor_log (ROADMAP_FATAL, "not enough space (%d) for NodePath", max_nodes);
      	}
      	if (flags & ED_TRKSEG_LOW_CONFID)
      	   to_id = -1;
      	nodes[node].node = to_id;
      	nodes[node].GPS_time = end_time;
         
         if (has_user_points) {
            user_points[node].points = seg_user_points;
            user_points[node].version = square_ver;
         }
            
      	node++;

			PendingLastNode = to_id;


		}
	}

	/* add points that are not yet in trkseg */

	num_extra = export_track_num_points ();

	if (export_track_is_new () && num_extra > 0) {

		if (point >= max_points) {
			editor_log (ROADMAP_FATAL, "not enough space (%d) for GPSPath", max_points);
		}
   	points[point].Position.longitude = INVALID_COORDINATE;
   	points[point].Position.latitude = INVALID_COORDINATE;
      points[point].altitude = INVALID_COORDINATE;
   	points[point].GPS_time = 0;
   	point++;
	}

	for (i = 0; i < num_extra; i++) {

		ordinal = export_track_point_ordinal (i);
		if (ordinal > PendingOrdinal) {

			if (*export_track_point_status (i) == POINT_STATUS_SAVE) {
				if (point >= max_points) {
					editor_log (ROADMAP_FATAL, "not enough space (%d) for GPSPath", max_points);
				}
				points[point].Position = *export_track_point_pos (i);
            points[point].altitude = export_track_point_gps (i) -> altitude;
				points[point].GPS_time = export_track_point_time (i);
				point++;
			}
			PendingOrdinal = ordinal;
		}
	}
	export_track_reset_points ();

	/* add optional last gps point */

	if (editor_track_filter_get_current (editor_track_get_gps_filter (), &last_gps_pos, &last_gps_time)) {

		if (point == 0 ||
			 last_gps_pos.longitude != points[point - 1].Position.longitude ||
			 last_gps_pos.latitude != points[point - 1].Position.latitude ||
			 last_gps_time != points[point - 1].GPS_time) {

			if (export_track_is_new () && num_extra == 0) {

				if (point >= max_points) {
					editor_log (ROADMAP_FATAL, "not enough space (%d) for GPSPath", max_points);
				}
		   	points[point].Position.longitude = INVALID_COORDINATE;
		   	points[point].Position.latitude = INVALID_COORDINATE;
		   	points[point].GPS_time = 0;
		   	point++;
			}

			if (point >= max_points) {
				editor_log (ROADMAP_FATAL, "not enough space (%d) for GPSPath", max_points);
			}
			points[point].Position = last_gps_pos;
			points[point].GPS_time = last_gps_time;
			point++;
		}
	}

	/* add real-time recording mode toggles */

	if (!offline) {
		toggle = editor_track_get_num_update_toggles ();
		if (toggle > max_toggles) {
	      editor_log (ROADMAP_FATAL, "not enough space (%d) for recording togles", max_toggles);
		}
		memcpy (toggle_times, editor_track_get_update_toggle_times (),
				  toggle * sizeof (time_t));
		*first_toggle = editor_track_get_update_toggle_state (0);
	}

	*num_nodes = node;
	*num_points = point;
	*num_toggles = toggle;
   if (has_user_points)
      *num_user_points = node;
   else
      *num_user_points = 0;
}

RTPathInfo *editor_track_report_begin_export (int offline) {

	int need_points;
	int need_nodes;
	int need_toggles;
	int has_data;
   BOOL has_user_points;

	has_data = editor_track_report_prepare_export (offline, &need_nodes, &need_points, &need_toggles, &has_user_points);

	if (need_points > PathInfo.max_points) {
		if (PathInfo.points) free (PathInfo.points);
		PathInfo.points = malloc (need_points * sizeof (PathInfo.points[0]));
		if (!PathInfo.points) {
			roadmap_log (ROADMAP_FATAL, "Cannot allocate space for %d track points", need_points);
		}
		PathInfo.max_points = need_points;
	}
	if (need_nodes > PathInfo.max_nodes) {
		if (PathInfo.nodes) free (PathInfo.nodes);
		PathInfo.nodes = malloc (need_nodes * sizeof (PathInfo.nodes[0]));
		if (!PathInfo.nodes) {
			roadmap_log (ROADMAP_FATAL, "Cannot allocate space for %d track nodes", need_nodes);
		}
		PathInfo.max_nodes = need_nodes;
	}
   if (has_user_points && need_nodes > PathInfo.max_user_points) {
      if (PathInfo.user_points) free (PathInfo.user_points);
      PathInfo.user_points = malloc (need_nodes * sizeof (PathInfo.user_points[0]));
      if (!PathInfo.user_points) {
         roadmap_log (ROADMAP_FATAL, "Cannot allocate space for %d user points", need_nodes);
      }
      PathInfo.max_user_points = need_nodes;
   }
	if (need_toggles > PathInfo.max_toggles) {
		if (PathInfo.update_toggle_times) free (PathInfo.update_toggle_times);
		PathInfo.update_toggle_times = malloc (need_toggles * sizeof (time_t));
		if (!PathInfo.update_toggle_times) {
			roadmap_log (ROADMAP_FATAL, "Cannot allocate space for %d recording toggles", need_toggles);
		}
		PathInfo.max_toggles = need_toggles;
	}

	if (has_data) {

		PathInfo.num_nodes = PathInfo.max_nodes;
		PathInfo.num_points = PathInfo.max_points;
		PathInfo.num_update_toggles = PathInfo.max_toggles;

		editor_track_report_get_points (offline,
												  PathInfo.nodes,
												  &PathInfo.num_nodes,
												  PathInfo.points,
												  &PathInfo.num_points,
												  PathInfo.update_toggle_times,
												  &PathInfo.num_update_toggles,
												  &PathInfo.first_update_toggle_state,
                                      has_user_points,
                                      PathInfo.user_points,
                                      &PathInfo.num_user_points);
	} else {

		PathInfo.num_nodes = 0;
		PathInfo.num_points = 0;
		PathInfo.num_update_toggles = 0;
      PathInfo.num_user_points = 0;
	}

	return &PathInfo;
}


void editor_track_report_conclude_export (int success) {

	EditorReportTrksegsInProgress = 0;
	if (success) {
		editor_trkseg_confirm_commit (EditorReportTrksegsId);
		editor_track_reset_update_toggles ();
		if (PendingOrdinal > LastOrdinal) {
			LastOrdinal = PendingOrdinal;
		}
		LastReportedNode = PendingLastNode;
	}
}


int editor_track_report_get_current_position (RoadMapGpsPosition*  GPS_position,
                               			 		 int*                 from_node,
                               			 		 int*                 to_node,
                               			 		 int*                 direction) {

   PluginLine  line;

	//TODO: change implementation to use internal trkseg info

   (*from_node) = (*to_node) = -1;

   if( -1 == roadmap_navigate_get_current( GPS_position, &line, direction))
   {
      // This is not an error:
      roadmap_log( ROADMAP_DEBUG, "editor_track_report_get_current_position() - 'roadmap_navigate_get_current()' failed");
      return 0;
   }

	editor_track_util_get_line_point_ids (&line,
 													  ROUTE_DIRECTION_AGAINST_LINE == *direction,
													  from_node,
													  to_node);

   if( (-1 == (*from_node)) || (-1 == (*to_node)))
   {
      roadmap_log( ROADMAP_WARNING, "editor_track_report_get_current_position() - 'editor_track_util_get_line_point_ids()' returned (from:%d; to:%d) for (line %d",
            (*from_node), (*to_node), line.line_id);
      (*from_node) = (*to_node) = -1;
      return 0;
   }

   return 1;
}

