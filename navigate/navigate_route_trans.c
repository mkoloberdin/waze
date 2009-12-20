/* navigate_route_trans.c - handle routing transactions
 *
 * LICENSE:
 *
 *   Copyright 2009 Israel Disatnik
 *
 *   This file is part of Waze.
 *
 *   Waze is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   Waze is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Waze; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <string.h>
#include "navigate_route_trans.h"
#include "navigate_instr.h"
#include "navigate_cost.h"
#include "navigate_route.h"
#include "roadmap_line_route.h"
#include "roadmap_street.h"
#include "roadmap_tile_manager.h"
#include "roadmap_tile_status.h"
#include "roadmap_messagebox.h"
#include "roadmap_twitter.h"
#include "Realtime/Realtime.h"

#define	MAX_RESULTS		10

typedef struct {

	int									route_id;
	int									rc;
	int									num_results;
	int									num_received;
	int									num_geometries;
	int									next_result;
	int									flags;
	int									alt_id;
	NavigateRouteRC					route_rc;
	NavigateRouteResult				result[MAX_RESULTS];
	NavigateRouteSegments			route;
	RoadMapPosition					pos_src;
	RoadMapPosition					pos_dst;
	const NavigateRouteCallbacks	*callbacks;
} NavigateRoutingContext;


static NavigateRoutingContext		RoutingContext;

static int 								TileCbRegistered = 0;
static RoadMapTileCallback 		TileCbNext = NULL;

enum RoutingOptions {

	AVOID_PRIMARIES							=	1,
	AVOID_TRAILS								=	2,
	AVOID_LONG_TRAILS							=	3,
	PREFER_SAME_STREET						=	4,
	IGNORE_REALTIME_INFO						=	5,
	ALLOW_UNKNOWN_DIRECTIONS				= 	6,
	ALLOW_NEAREST_REACHABLE_DESTINATION =	7,
	ALLOW_NEAREST_CONNECTED_ORIGIN      =	8,
	IGNORE_SEGMNET_INFO                 =  9,
	AVOID_TOLL_ROADS                    =  10,
   IGNORE_PENALTIES                    =  11,
   PREFER_UNKNOWN_DIRECTIONS           =  12,
    AVOID_PALESTINIAN_ROADS				= 13,
	MAX_ROUTING_OPTIONS						=	13
};

enum RoutingTypes {

	ROUTE_SHORTEST_DISTANCE					=	1,
	ROUTE_SHORTEST_TIME						=	2,
	ROUTE_SHORTEST_HISTORIC_TIME			=	3
};

enum RoutingAttributes {

	ATTR_CHANGED_DEPARTURE 					=	1,
	ATTR_CHANGED_DESTINATION				=	2,
	ATTR_IGNORED_TURN_RESTRICTIONS		=	3
};

static void instrument_segments_cb (int tile_id);


static void free_result (int iresult)
{
	if (RoutingContext.result[iresult].geometry.points) {
		free (RoutingContext.result[iresult].geometry.points);
	}
	if (RoutingContext.result[iresult].description) {
		free (RoutingContext.result[iresult].description);
	}
	RoutingContext.result[iresult].alt_id = 0;
}

static void navigate_route_free_context (void)
{
	int i;

	if (RoutingContext.route.segments) free (RoutingContext.route.segments);
	for (i = 0; i < MAX_RESULTS; i++) {
		free_result (i);
	}
}

static void navigate_route_init_context (void)
{
	static int last_route_id = 0;

	if (last_route_id > 0) {
		navigate_route_free_context ();
	}

	memset (&RoutingContext, 0, sizeof (RoutingContext));
	RoutingContext.route_id = ++last_route_id;
	roadmap_log(ROADMAP_DEBUG, "Increasing route id to:%d", RoutingContext.route_id );
}

static void navigate_route_clear_context (void)
{
	int last_route_id = RoutingContext.route_id;

	if (last_route_id > 0) {
		navigate_route_free_context ();
	}

	memset (&RoutingContext, 0, sizeof (RoutingContext));
	RoutingContext.route_id = last_route_id;
}

static void assign_group_ids (void)
{
	int i;
	int group;

	if (RoutingContext.route.num_valid !=
		 RoutingContext.route.num_segments) {

		roadmap_log (ROADMAP_ERROR, "assign_group_ids() - route incomplete");
		return;
	}

	for (i = 0, group = 0; i < RoutingContext.route.num_valid; i++) {

		RoutingContext.route.segments[i].group_id = group;
		if (RoutingContext.route.segments[i].instruction != CONTINUE) {
			group++;
		}
	}

	roadmap_log (ROADMAP_DEBUG, "Found %d segment groups", group);
}


static void request_tile (int tile_id, int urgent)
{
	int *tile_status;

	if (!TileCbRegistered) {
		TileCbNext = roadmap_tile_register_callback (instrument_segments_cb);
		TileCbRegistered = 1;
	}

	tile_status = roadmap_tile_status_get (tile_id);
	(*tile_status) = ((*tile_status) | ROADMAP_TILE_STATUS_FLAG_CALLBACK | ROADMAP_TILE_STATUS_FLAG_ROUTE) &
	      ~ROADMAP_TILE_STATUS_FLAG_UPTODATE;
	if (urgent) {
		roadmap_log (ROADMAP_DEBUG, "Requesting tile %d", tile_id);
		roadmap_tile_request (tile_id, ROADMAP_TILE_STATUS_PRIORITY_NAV_RESOLVE, 1, NULL);
	}
}

static void request_first_tiles (void)
{
	int i;

	for (i = 0; i < RoutingContext.route.num_valid; i++) {

		NavigateSegment *segment = RoutingContext.route.segments + i;
		if (i == 0 ||
			 (segment->distance == 0 &&
			  segment->square !=
			  (segment - 1)->square)) {

			if ((time_t)roadmap_square_timestamp (segment->square) < (time_t)segment->update_time) {
				request_tile (segment->square, 1);
			}
		} else {
			break;
		}
	}

	RoutingContext.route.num_first = i;
	if (RoutingContext.route.num_first > 1) {
		RoutingContext.route.segments[RoutingContext.route.num_first - 1].distance = RoutingContext.route.segments[0].distance;
		RoutingContext.route.segments[RoutingContext.route.num_first - 1].cross_time = RoutingContext.route.segments[0].cross_time;
		RoutingContext.route.segments[0].distance = 0;
		RoutingContext.route.segments[0].cross_time = 0;
	}

	if (RoutingContext.route.num_segments == RoutingContext.route.num_valid) {
		for (i = 1; i <= RoutingContext.route.num_valid; i++) {

			NavigateSegment *segment = RoutingContext.route.segments + RoutingContext.route.num_valid - i;
			if (segment->distance > 0) break;
		}
	}
	RoutingContext.route.num_last = i;
}

static void instrument_segments (int initial)
{
	int prev_done = RoutingContext.route.num_instrumented;
	int max_instrumented;
	int i;
	BOOL is_version_mismatch = FALSE;

	if (initial) {
		request_first_tiles ();
	}
	// we cannot instrument the last valid segment
	// before we have the next one, unless
	// we have all segments
	if (RoutingContext.route.num_valid ==
		 RoutingContext.route.num_segments) {

		max_instrumented = RoutingContext.route.num_valid;
	} else {

		max_instrumented = RoutingContext.route.num_valid - 1;
	}

	for (i = RoutingContext.route.next_to_instrument; i < max_instrumented; i++) {

		NavigateSegment *segment;
		NavigateSegment *next_segment;
		NavigateSegment *prev_segment;
		int first_shape;
		int last_shape;
		int from_id;
		int to_id;
		int distance;
		int cross_time;
		time_t square_time;
		time_t update_time;

		if (i < RoutingContext.route.num_first) {
			segment = RoutingContext.route.segments + RoutingContext.route.num_first - 1 - i;
			if (i == RoutingContext.route.num_first - 1) {
				next_segment = NULL;
			} else {
				next_segment = segment - 1;
			}
			if (i > 0 && i < RoutingContext.route.num_valid - 1) {
				prev_segment = segment + 1;
			} else {
				prev_segment = NULL;
			}
		} else {
			segment = RoutingContext.route.segments + i;
			if (i < RoutingContext.route.num_valid - 1) {
				next_segment = segment + 1;
			} else {
				next_segment = NULL;
			}
			prev_segment = segment - 1;
		}

		if (segment->is_instrumented) continue;

		square_time = roadmap_square_timestamp (segment->square);
		update_time = segment->update_time;
		if (square_time < update_time) {

			//roadmap_log (ROADMAP_DEBUG, "total %d/%d instrumented, requesting tile %d(%08x)", RoutingContext.route.num_instrumented, max_instrumented, segment->square, segment->square);
			//roadmap_tile_request (segment->square, 0, instrument_segments_cb);
			if (initial) {
				roadmap_log (ROADMAP_DEBUG, "Requesting tile %d", segment->square);
				roadmap_log (ROADMAP_DEBUG, " has time %d", square_time);
				roadmap_log (ROADMAP_DEBUG, " need time %d",update_time);
				request_tile (segment->square, 0);
			}
			continue;
		}

		if (roadmap_square_version (segment->square) > (int)update_time + 60 * 60) {

			char msg[200];
			snprintf (msg, 200, "Square version mismatch! tile = %d expected version = %d current version = %d",
							segment->square, (int)update_time, roadmap_square_version (segment->square));
			roadmap_log (ROADMAP_ERROR, msg);
			/*
			if (roadmap_verbosity () <= ROADMAP_MESSAGE_DEBUG) {
				roadmap_messagebox ("Debug", msg);
			}
			*/
			is_version_mismatch = TRUE;
			continue;
		}

		// do not instrument segments split to tiles before their predecessors
		if (prev_segment != NULL && segment->square != prev_segment->square && !prev_segment->is_instrumented) {
			roadmap_log (ROADMAP_DEBUG, "Cannot instrument segment %d/%d before instrumentation of segment %d/%d",
								segment->square, segment->line, prev_segment->square, prev_segment->line);
			continue;
		}

		// check for valid line id
		if (segment->line < 0 || segment->line >= roadmap_line_count ()) {

			roadmap_log (ROADMAP_ERROR, "Invalid route line %d square %d (update_time=%d, sq.timestamp=%d, range=%d)",
							 segment->line, segment->square, segment->update_time,
							 (int)roadmap_square_timestamp (segment->square),
							 roadmap_line_count ());
			if (!RoutingContext.route_rc)
				RoutingContext.route_rc = route_inconsistent;
			continue;
		}

		// fill segment data
		segment->cfcc = roadmap_line_cfcc (segment->line);
	   roadmap_line_shapes (segment->line, &first_shape, &last_shape);
	   segment->first_shape = first_shape;
	   segment->last_shape = last_shape;
	   roadmap_line_from (segment->line, &segment->from_pos);
	   roadmap_line_to (segment->line, &segment->to_pos);
	   segment->shape_initial_pos.longitude = segment->from_pos.longitude;
	   segment->shape_initial_pos.latitude = segment->from_pos.latitude;

      segment->street = roadmap_line_get_street (segment->line);
      segment->context = roadmap_line_context (segment->line);

		// find segment direction
		roadmap_line_point_ids (segment->line, &from_id, &to_id);
		if (from_id == segment->from_node_id) {
			segment->line_direction = ROUTE_DIRECTION_WITH_LINE;
		} else {
			segment->line_direction = ROUTE_DIRECTION_AGAINST_LINE;
		}
		roadmap_log (ROADMAP_DEBUG, "Segment %d: group %d from %d to %d track %d direction %s",
					i,
					segment->group_id,
					from_id, to_id, segment->from_node_id,
					segment->line_direction == ROUTE_DIRECTION_WITH_LINE ? "-->" : "<--");

		// compensate for calculated length and time
		if (next_segment != NULL &&
			 next_segment->distance == 0 &&
			 next_segment->square != segment->square) {

         distance = roadmap_line_length (segment->line);

			if (distance < segment->distance) {
				cross_time = segment->cross_time * distance / segment->distance;

				next_segment->distance = segment->distance - distance;
				next_segment->cross_time = segment->cross_time - cross_time;
				segment->distance = distance;
				segment->cross_time = cross_time;
			}
		}

		// fix shapes of end points
		if (i < RoutingContext.route.num_first && segment->distance > 0 &&
			 (next_segment == NULL || next_segment->distance == 0)) {

		   if (segment->line_direction == ROUTE_DIRECTION_WITH_LINE) {
		      navigate_instr_fix_line_end (&RoutingContext.pos_src, segment, LINE_START);
		   } else {
		      navigate_instr_fix_line_end (&RoutingContext.pos_src, segment, LINE_END);
		   }
		}

		if (i >= RoutingContext.route.num_segments - RoutingContext.route.num_last - 1 &&
			 (next_segment == NULL || next_segment->distance == 0)) {

		   if (segment->line_direction == ROUTE_DIRECTION_WITH_LINE) {
		      navigate_instr_fix_line_end (&RoutingContext.pos_dst, segment, LINE_END);
		   } else {
		      navigate_instr_fix_line_end (&RoutingContext.pos_dst, segment, LINE_START);
		   }
		}

		segment->is_instrumented = 1;
		RoutingContext.route.num_instrumented++;

		roadmap_log (ROADMAP_DEBUG, "Finished instrumentation of segment no. %d (%d/%d)",
						 i, segment->square, segment->line);

		if (i == RoutingContext.route.next_to_instrument) {
			RoutingContext.route.next_to_instrument++;
		}
	}
	roadmap_log (ROADMAP_INFO, "done: total %d/%d instrumented", RoutingContext.route.num_instrumented, RoutingContext.route.num_segments);

	if (RoutingContext.route.num_instrumented > prev_done) {
		if (RoutingContext.callbacks->on_instrumented && !initial) {
			RoutingContext.callbacks->on_instrumented (RoutingContext.route.num_instrumented);
		}
	}

	if (is_version_mismatch && RoutingContext.callbacks->on_square_ver_mismatch)
	   RoutingContext.callbacks->on_square_ver_mismatch();

}

static void instrument_segments_cb (int tile_id)
{
	roadmap_log (ROADMAP_INFO, "Tile callback on %d", tile_id);

	if ( *roadmap_tile_status_get( tile_id ) & ROADMAP_TILE_STATUS_FLAG_ROUTE )
	{
		instrument_segments	 (0);
	}
	if (TileCbNext) {
		TileCbNext (tile_id);
	}
}


static void on_route_complete (void)
{
	if (RoutingContext.route.num_segments == 0) {

		if (RoutingContext.route_rc == route_succeeded)
			RoutingContext.route_rc = route_inconsistent;
	}

	if (RoutingContext.route.num_received == RoutingContext.route.num_segments) {

		assign_group_ids ();
		instrument_segments (1);
		if (RoutingContext.callbacks->on_segments) {
			RoutingContext.callbacks->on_segments (RoutingContext.route_rc, RoutingContext.result, &RoutingContext.route);
			RoutingContext.route_rc = route_succeeded;
		}
	}
}


static int process_attribute (NavigateRouteResult		*result,
										int							attribute,
										int							enabled)
{
	switch (attribute) {

		case ATTR_CHANGED_DEPARTURE:
			if (enabled) result->flags = result->flags | CHANGED_DEPARTURE;
			break;
		case ATTR_CHANGED_DESTINATION:
			if (enabled) result->flags = result->flags | CHANGED_DESTINATION;
			break;
		case ATTR_IGNORED_TURN_RESTRICTIONS:
			if (enabled) result->flags = result->flags | GRAPH_IGNORE_TURNS;
			break;
		default:
			roadmap_log (ROADMAP_ERROR, "process_attribute() -  Invalid attribute pair %d,%s",
							 attribute, enabled? "T" : "F");
			return 0;
	}

	return 1;
}

static int verify_route_id (const char **data, roadmap_result *rc)
{
   int							route_id;

   *data = ReadIntFromString (*data,         //   [in]      Source string
                              ",",           //   [in,opt]  Value termination
                              NULL,          //   [in,opt]  Allowed padding
                              &route_id,		//   [out]     Output value
                              1);            //   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

   if (!*data) {
      roadmap_log (ROADMAP_ERROR, "verify_route_id() - Failed to read 'route_id'");
      return 0;
   }

   if (route_id != RoutingContext.route_id) {
      int length = 2000;

      roadmap_log (ROADMAP_ERROR, "verify_route_id() - route_id mismatch: expected %d received %d",
      				 RoutingContext.route_id, route_id);
      // ignore rest of line
      *data = ExtractString (*data, NULL, &length, "\r\n", TRIM_ALL_CHARS);
      *rc = succeeded;
      return 0;
   }

	return 1;
}


static int find_alt_id (int alt_id)
{
   int	i;

   for (i = 0; i < RoutingContext.num_received && RoutingContext.result[i].alt_id != alt_id; i++)
   	;

   if (i == RoutingContext.num_received) {
      roadmap_log (ROADMAP_ERROR, "find_alt_id() - unknown alt_id %d", alt_id);
      return -1;
   }

   return i;
}

static int verify_alt_id (const char **data, roadmap_result *rc)
{
   int							alt_id;
   int							i;

   *data = ReadIntFromString (*data,         //   [in]      Source string
                              ",",           //   [in,opt]  Value termination
                              NULL,          //   [in,opt]  Allowed padding
                              &alt_id,			//   [out]     Output value
                              1);            //   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

   if (!*data) {
      roadmap_log (ROADMAP_ERROR, "verify_alt_id() - Failed to read 'alt_id'");
      return -1;
   }

   i = find_alt_id (alt_id);

   if (i == -1) {
      // ignore rest of line
      int length = 2000;
      *data = ExtractString (*data, NULL, &length, "\r\n", TRIM_ALL_CHARS);
      *rc = succeeded;
   }

	return i;
}


static void routing_error (const char *description)
{
	roadmap_messagebox_timeout("Error", description, 5);

	if (RoutingContext.callbacks->on_results) {
		RoutingContext.callbacks->on_results (RoutingContext.route_rc, 0, NULL);
	}
	else if (RoutingContext.callbacks->on_segments) {
		RoutingContext.callbacks->on_segments (RoutingContext.route_rc, NULL, NULL);
	}
}


const char *on_routing_response_code (/* IN  */   const char*       data,
                          		   	  /* IN  */   void*             context,
                                 	  /* OUT */   BOOL*             more_data_needed,
                                 	  /* OUT */   roadmap_result*   rc)
{
	char description[1000];
	int size = sizeof (description);

   // Default error for early exit:
   (*rc) = err_parser_unexpected_data;

   // Expected data:
   //    <route_id>,<num_responses>,<rc>,<description>\n

   // route_id
   if (!verify_route_id (&data, rc)) return data;

 	roadmap_log (ROADMAP_DEBUG, "RoutingResonseCode: %s", data);

 	// num_responses
   data = ReadIntFromString(  data,          					//   [in]      Source string
                              ",",           					//   [in,opt]  Value termination
                              NULL,          					//   [in,opt]  Allowed padding
                              &RoutingContext.num_results,	//   [out]     Output value
                              1);            					//   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

   if (!data) {
      roadmap_log (ROADMAP_ERROR, "on_routing_response_code() - Failed to read 'num_responses'");
      return NULL;
   }

 	// rc
   data = ReadIntFromString(  data,          		//   [in]      Source string
                              ",",           		//   [in,opt]  Value termination
                              NULL,          		//   [in,opt]  Allowed padding
                              &RoutingContext.rc,	//   [out]     Output value
                              1);            		//   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

   if (!data) {
      roadmap_log (ROADMAP_ERROR, "on_routing_response_code() - Failed to read 'rc'");
      return NULL;
   }

 	// description
 	data = ExtractString (data, description, &size, "\r\n", TRIM_ALL_CHARS);
   if (!data) {
      roadmap_log (ROADMAP_ERROR, "on_routing_response_code() - Failed to read 'description'");
      return NULL;
   }

 	roadmap_log (ROADMAP_DEBUG, "RoutingResponseCode for id %d is %d,%s",
 					 RoutingContext.route_id, RoutingContext.rc, description);

	if (RoutingContext.rc != 200) {
		if (RoutingContext.route_rc == route_succeeded)
			RoutingContext.route_rc = route_server_error;
	}

	if (RoutingContext.callbacks->on_rc) {
		RoutingContext.callbacks->on_rc (RoutingContext.route_rc, RoutingContext.rc, description);
	}

	if (RoutingContext.rc != 200) {
		//TODO move all error handling to RoutingContext.callbacks->on_rc
		routing_error (description);
	}

   // Fix [out] param:
   (*rc) = succeeded;

   return data;
}

const char *on_routing_response (/* IN  */   const char*       data,
                          		   /* IN  */   void*             context,
                                 /* OUT */   BOOL*             more_data_needed,
                                 /* OUT */   roadmap_result*   rc)
{
   int							nattrs;
   int							status;
   char							alt_description[1000];
   int							size_description = sizeof (alt_description);
   NavigateRouteResult		*curr_result;

   // Default error for early exit:
   (*rc) = err_parser_unexpected_data;

   // Expected data:
   //    <route_id>,<status>,<alt_id>,<alt_description>,<total_length>,<total_time>,<nsegments>,<nattrs*2>,<attr1>,<val1>...\n

   // route_id
   if (!verify_route_id (&data, rc)) return data;

   // status
   data = ReadIntFromString(  data,          		//   [in]      Source string
                              ",",           		//   [in,opt]  Value termination
                              NULL,          		//   [in,opt]  Allowed padding
                              &status,					//   [out]     Output value
                              1);            		//   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

   if (!data) {
      roadmap_log (ROADMAP_ERROR, "on_routing_response() - Failed to read 'status'");
      return NULL;
   }

   switch (status) {
   	case ROUTE_ORIGINAL:
   		// receive next result
   		RoutingContext.next_result = RoutingContext.num_received++;
   		break;
   	case ROUTE_UPDATE:
   		// override original result
   		RoutingContext.next_result = 0;
   		RoutingContext.num_received = 1;
   		break;
   	case ROUTE_ALTERNATIVE:
   		// create second alternative
   		RoutingContext.next_result = 1;
   		RoutingContext.num_received = 2;
   		RoutingContext.num_geometries = 1;
   		break;
   	default:
	   	roadmap_log (ROADMAP_ERROR, "on_routing_response() - does not support status %d", status);
	   	return NULL;
   }
   assert(RoutingContext.next_result < MAX_RESULTS);

   curr_result = &RoutingContext.result[RoutingContext.next_result];
   curr_result->route_status = status;
   curr_result->flags = RoutingContext.flags;

   // alt_id
   data = ReadIntFromString(  data,          				//   [in]      Source string
                              ",",           				//   [in,opt]  Value termination
                              NULL,          				//   [in,opt]  Allowed padding
                              &curr_result->alt_id,		//   [out]     Output value
                              1);            				//   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

   if (!data) {
      roadmap_log (ROADMAP_ERROR, "on_routing_response() - Failed to read 'alt_id'");
      return NULL;
   }

 	// alt_description
 	data = ExtractNetworkString(data, alt_description, &size_description, ",", 1);
   if (!data) {
      roadmap_log (ROADMAP_ERROR, "on_routing_response() - Failed to read 'alt_description'");
      return NULL;
   }
   curr_result->description = strdup (alt_description);

   // total_length
   data = ReadIntFromString(  data,          		//   [in]      Source string
                              ",",           		//   [in,opt]  Value termination
                              NULL,          		//   [in,opt]  Allowed padding
                              &curr_result->total_length,	//   [out]     Output value
                              1);            		//   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

   if (!data) {
      roadmap_log (ROADMAP_ERROR, "on_routing_response() - Failed to read 'total_length'");
      return NULL;
   }

   // total_time
   data = ReadIntFromString(  data,          		//   [in]      Source string
                              ",",           		//   [in,opt]  Value termination
                              NULL,          		//   [in,opt]  Allowed padding
                              &curr_result->total_time,		//   [out]     Output value
                              1);            		//   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

   if (!data) {
      roadmap_log (ROADMAP_ERROR, "on_routing_response() - Failed to read 'total_time'");
      return NULL;
   }

   // nsegments
   data = ReadIntFromString(  data,          				//   [in]      Source string
                              ",",           				//   [in,opt]  Value termination
                              NULL,          				//   [in,opt]  Allowed padding
                              &curr_result->num_segments,	//   [out]     Output value
                              1);            				//   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

   if (!data) {
      roadmap_log (ROADMAP_ERROR, "on_routing_response() - Failed to read 'nsegments'");
      return NULL;
   }

   // support for partial update
   if (status == ROUTE_UPDATE) {
   	RoutingContext.route.num_received = RoutingContext.route.num_segments - curr_result->num_segments;
   }

   // nattrs
   data = ReadIntFromString(  data,          				//   [in]      Source string
                              ",\n",           				//   [in,opt]  Value termination
                              NULL,          				//   [in,opt]  Allowed padding
                              &nattrs,							//   [out]     Output value
                              1);    				         //   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

   if (!data) {
      roadmap_log (ROADMAP_ERROR, "on_routing_response() - Failed to read 'nattrs'");
      return NULL;
   }

   while (nattrs > 0) {

   	int	attribute;
   	char	bool_string[2];
   	int	bool_size;

	   data = ReadIntFromString(  data,          				//   [in]      Source string
	                              ",",           				//   [in,opt]  Value termination
	                              NULL,          				//   [in,opt]  Allowed padding
	                              &attribute,						//   [out]     Output value
	                              1);    							//   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

	   if (!data) {
	      roadmap_log (ROADMAP_ERROR, "on_routing_response() - Failed to read attribute");
	      return NULL;
   	}

   	nattrs--;

   	if (nattrs == 0) {
	      roadmap_log (ROADMAP_ERROR, "on_routing_response() - invalid number of attributes");
	      return NULL;
   	}

   	bool_size = sizeof (bool_string);
   	data = ExtractString (data, bool_string, &bool_size, ",\n", 1);
	   if (!data) {
	      roadmap_log (ROADMAP_ERROR, "on_routing_response() - Failed to read attribute value");
	      return NULL;
	   }

   	if (!process_attribute (curr_result, attribute, (bool_string[0] == 'T'))) {
   		return NULL;
   	}

   	nattrs--;
   }

   data = EatChars (data, "\r\n", TRIM_ALL_CHARS);
   if (!data) {
      roadmap_log (ROADMAP_ERROR, "on_routing_response() - illegal characters at end of line");
      return NULL;
   }

   // Fix [out] param:
   (*rc) = succeeded;

   roadmap_log (ROADMAP_INFO, "Receiving route: %d segments", curr_result->num_segments);
   return data;
}


const char *on_route_points (/* IN  */   const char*       data,
                             /* IN  */   void*             context,
                             /* OUT */   BOOL*             more_data_needed,
                             /* OUT */   roadmap_result*   rc)
{
   int							num_total_points;
   int							start_index;
   int							num_point_numbers;
   int							iresult;
   NavigateRouteGeometry	*geometry;

   // Default error for early exit:
   (*rc) = err_parser_unexpected_data;

   // Expected data:
   //    <route_id>,<alt_id>,<num_total_points>,<start_index>,<npoints*2>,<longitude1>,<latitude1>...\n

   // route_id
   if (!verify_route_id (&data, rc)) return data;

   // alt_id
   iresult = verify_alt_id (&data, rc);
   if (iresult < 0) return data;
   assert(iresult < MAX_RESULTS);
   geometry = &RoutingContext.result[iresult].geometry;

   // num_total_points
   data = ReadIntFromString(  data,          		//   [in]      Source string
                              ",",           		//   [in,opt]  Value termination
                              NULL,          		//   [in,opt]  Allowed padding
                              &num_total_points,	//   [out]     Output value
                              1);            		//   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

   if (!data) {
      roadmap_log (ROADMAP_ERROR, "on_route_points() - Failed to read 'num_total_points'");
      return NULL;
   }

   // start_index
   data = ReadIntFromString(  data,          //   [in]      Source string
                              ",",           //   [in,opt]  Value termination
                              NULL,          //   [in,opt]  Allowed padding
                              &start_index,	//   [out]     Output value
                              1);            //   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

   if (!data) {
      roadmap_log (ROADMAP_ERROR, "on_route_points() - Failed to read 'start_index'");
      return NULL;
   }

   // num_point_numbers
   data = ReadIntFromString(  data,          		//   [in]      Source string
                              ",",           		//   [in,opt]  Value termination
                              NULL,          		//   [in,opt]  Allowed padding
                              &num_point_numbers,	//   [out]     Output value
                              1);            		//   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

   if (!data) {
      roadmap_log (ROADMAP_ERROR, "on_route_points() - Failed to read 'num_point_numbers'");
      return NULL;
   }

	if (geometry->num_points == 0) {

		// allocate point space
		geometry->num_points = num_total_points;
		geometry->valid_points = 0;
		geometry->points = malloc (num_total_points * sizeof (RoadMapPosition));
	} else {

		if (geometry->num_points != num_total_points) {
	      roadmap_log (ROADMAP_ERROR, "on_route_points() - num_total_points mismatch: expected %d found %d",
	      				 geometry->num_points, num_total_points);
	      return NULL;
		}
	}

	if (geometry->valid_points != start_index) {

      roadmap_log (ROADMAP_ERROR, "on_route_points() - start_index mismatch: expected %d found %d",
      				 geometry->valid_points, start_index);
      return NULL;
	}

	while (num_point_numbers > 0) {

		if (geometry->valid_points == geometry->num_points) {

	      roadmap_log (ROADMAP_ERROR, "on_route_points() - too many points");
	      return NULL;
		}

	   data = ReadIntFromString(  data,          		//   [in]      Source string
	                              ",",           		//   [in,opt]  Value termination
	                              NULL,          		//   [in,opt]  Allowed padding
	                              &geometry->points[geometry->valid_points].longitude,
	                              							//   [out]     Output value
	                              1);            		//   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

	   if (!data) {
	      roadmap_log (ROADMAP_ERROR, "on_route_points() - Failed to read longitude");
	      return NULL;
	   }
		num_point_numbers--;

		if (num_point_numbers == 0) {
	      roadmap_log (ROADMAP_ERROR, "on_route_points() - Odd number of point values");
	      return NULL;
		}

	   data = ReadIntFromString(  data,          		//   [in]      Source string
	                              num_point_numbers > 1 ? "," : "\r\n",
	                              							//   [in,opt]  Value termination
	                              NULL,          		//   [in,opt]  Allowed padding
	                              &geometry->points[geometry->valid_points].latitude,
	                              							//   [out]     Output value
	                              1);            		//   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

	   if (!data) {
	      roadmap_log (ROADMAP_ERROR, "on_route_points() - Failed to read latitude");
	      return NULL;
	   }
		num_point_numbers--;

		geometry->valid_points++;
	}

   data = EatChars (data, "\r\n", TRIM_ALL_CHARS);
   if (!data) {
      roadmap_log (ROADMAP_ERROR, "on_route_points() - illegal characters at end of line");
      return NULL;
   }

	if (geometry->valid_points == geometry->num_points) {
		RoutingContext.num_geometries++;
		if (RoutingContext.num_geometries == RoutingContext.num_results &&
			 RoutingContext.callbacks->on_results != NULL) {
			RoutingContext.callbacks->on_results (RoutingContext.route_rc, RoutingContext.num_results, RoutingContext.result);
		}
	}

   // Fix [out] param:
   (*rc) = succeeded;

   return data;
}


static void adjust_segment_times (int i_segment, int length, int cross_time)
{
	NavigateSegment *segment;
	int direction = 1;

	roadmap_log (ROADMAP_DEBUG, "Adjusting times from segment %d", i_segment);

	if (i_segment == 0) {
		i_segment = RoutingContext.route.num_first - 1;
		direction = -1;
	}

	while (length > 0) {
		if (i_segment < 0 ||
			 i_segment >= RoutingContext.route.num_segments) {
			roadmap_log (ROADMAP_ERROR, "Inconsistency in new route times adjustment");
			break;
		}
		segment = RoutingContext.route.segments + i_segment;
		if (segment->distance == 0) {
			roadmap_log (ROADMAP_ERROR, "Inconsistency in new route times adjustment");
			break;
		}
		segment->cross_time = cross_time * segment->distance / length;
		length -= segment->distance;
		cross_time -= segment->cross_time;
		roadmap_log (ROADMAP_DEBUG, "Set cross time of segment %d to %d", i_segment, segment->cross_time);
		i_segment += direction;
	}
}


const char *on_route_segments (/* IN  */   const char*       data,
                               /* IN  */   void*             context,
                               /* OUT */   BOOL*             more_data_needed,
                               /* OUT */   roadmap_result*   rc)
{
   int							square;
   int							timestamp;
   int							num_args;
   int							iresult;

   // Default error for early exit:
   (*rc) = err_parser_unexpected_data;

   // Expected data:
   //    <route_id>,<alt_id>,<tile_id>,<timestamp>,<nsegments*6>,<line_id>,<from_node_id>,<length>,<cross_time>,<instruction>,<exit_no>...\n

   // route_id
   if (!verify_route_id (&data, rc)) return data;

   // alt_id
   iresult = verify_alt_id (&data, rc);
   if (iresult < 0) return data;

   // tile_id
   data = ReadIntFromString(  data,    //   [in]      Source string
                              ",",     //   [in,opt]  Value termination
                              NULL,    //   [in,opt]  Allowed padding
                              &square,	//   [out]     Output value
                              1);      //   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

   if (!data) {
      roadmap_log (ROADMAP_ERROR, "on_route_segments() - Failed to read 'tile_id'");
      return NULL;
   }

   // timestamp
   data = ReadIntFromString(  data,    	//   [in]      Source string
                              ",",     	//   [in,opt]  Value termination
                              NULL,    	//   [in,opt]  Allowed padding
                              &timestamp,	//   [out]     Output value
                              1);      	//   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

   if (!data) {
      roadmap_log (ROADMAP_ERROR, "on_route_segments() - Failed to read 'timestamp'");
      return NULL;
   }

   // nsegments
   data = ReadIntFromString(  data,    	//   [in]      Source string
                              ",",     	//   [in,opt]  Value termination
                              NULL,    	//   [in,opt]  Allowed padding
                              &num_args,	//   [out]     Output value
                              1);      	//   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

   if (!data) {
      roadmap_log (ROADMAP_ERROR, "on_route_segments() - Failed to read 'nsegments'");
      return NULL;
   }

   if (RoutingContext.route.segments == NULL) {
   	RoutingContext.route.num_segments = RoutingContext.result[iresult].num_segments;
   	RoutingContext.route.segments = calloc (RoutingContext.route.num_segments, sizeof (NavigateSegment));
   }

   while (num_args > 0) {

   	int 	arg;
   	int	ignore_this;
   	int	pending_length = 0;
   	int	pending_time = 0;

   	NavigateSegment *segment = RoutingContext.route.segments + RoutingContext.route.num_received;

   	if (RoutingContext.route.num_received >= RoutingContext.route.num_segments) {

	      roadmap_log (ROADMAP_ERROR, "on_route_segments() - too many segments");
	      return NULL;
   	}

   	ignore_this = 0;

	   if (RoutingContext.result[iresult].route_status != ROUTE_UPDATE) {
   		segment->square = square;
	   } else if (segment->square != square) {
	   	roadmap_log (ROADMAP_ERROR, "on_route_segments() - square %d changed on update to %d",
	   					 segment->square, square);
	   	ignore_this = 1;
	   }
	   if (RoutingContext.result[iresult].route_status != ROUTE_UPDATE) {
   		segment->update_time = timestamp;
	   } else if (segment->update_time != timestamp && !ignore_this) {
	   	roadmap_log (ROADMAP_ERROR, "on_route_segments() - timestamp %d changed on update to %d",
	   					 segment->update_time, timestamp);
	   }

	   // segment_id
	   data = ReadIntFromString(  data,    	//   [in]      Source string
	                              ",",     	//   [in,opt]  Value termination
	                              NULL,    	//   [in,opt]  Allowed padding
	                              &arg,			//   [out]     Output value
	                              1);      	//   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

	   if (!data) {
	      roadmap_log (ROADMAP_ERROR, "on_route_segments() - Failed to read 'segment_id'");
	      return NULL;
	   }
	   if (RoutingContext.result[iresult].route_status != ROUTE_UPDATE) {
		   segment->line = arg;
	   } else if (arg != segment->line && !ignore_this) {
	   	roadmap_log (ROADMAP_ERROR, "on_route_segments() - line %d changed on update to %d",
	   					 segment->line, arg);
	   	ignore_this = 1;
	   }
	   num_args--;

	   if (num_args == 0) {
	      roadmap_log (ROADMAP_ERROR, "on_route_segments() - invalid number of arguments");
	      return NULL;
	   }

	   // from_node_id
	   data = ReadIntFromString(  data,    	//   [in]      Source string
	                              ",",     	//   [in,opt]  Value termination
	                              NULL,    	//   [in,opt]  Allowed padding
	                              &arg,			//   [out]     Output value
	                              1);      	//   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

	   if (!data) {
	      roadmap_log (ROADMAP_ERROR, "on_route_segments() - Failed to read 'from_node_id'");
	      return NULL;
	   }
	   if (RoutingContext.result[iresult].route_status != ROUTE_UPDATE) {
		   segment->from_node_id = arg;
	   } else if (arg != segment->from_node_id && !ignore_this) {
	   	roadmap_log (ROADMAP_ERROR, "on_route_segments() - from node %d changed on update to %d",
	   					 segment->from_node_id, arg);
	   	ignore_this = 1;
	   }
	   num_args--;

	   if (num_args == 0) {
	      roadmap_log (ROADMAP_ERROR, "on_route_segments() - invalid number of arguments");
	      return NULL;
	   }

	   // length
	   data = ReadIntFromString(  data,    	//   [in]      Source string
	                              ",",     	//   [in,opt]  Value termination
	                              NULL,    	//   [in,opt]  Allowed padding
	                              &arg,			//   [out]     Output value
	                              1);      	//   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

	   if (!data) {
	      roadmap_log (ROADMAP_ERROR, "on_route_segments() - Failed to read 'length'");
	      return NULL;
	   }
	   if (RoutingContext.result[iresult].route_status != ROUTE_UPDATE) {
		   segment->distance = arg;
	   } else if (!ignore_this) {
		   pending_length = arg;
	   }
	   num_args--;

	   if (num_args == 0) {
	      roadmap_log (ROADMAP_ERROR, "on_route_segments() - invalid number of arguments");
	      return NULL;
	   }

	   // cross_time
	   data = ReadIntFromString(  data,    	//   [in]      Source string
	                              ",",     	//   [in,opt]  Value termination
	                              NULL,    	//   [in,opt]  Allowed padding
	                              &arg,			//   [out]     Output value
	                              1);      	//   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

	   if (!data) {
	      roadmap_log (ROADMAP_ERROR, "on_route_segments() - Failed to read 'cross_time'");
	      return NULL;
	   }
	   if (RoutingContext.result[iresult].route_status != ROUTE_UPDATE) {
		   segment->cross_time = arg;
	   } else if (!ignore_this) {
		   pending_time = arg;
	   }
	   num_args--;

	   if (num_args == 0) {
	      roadmap_log (ROADMAP_ERROR, "on_route_segments() - invalid number of arguments");
	      return NULL;
	   }

	   // instruction
	   data = ReadIntFromString(  data,    	//   [in]      Source string
	                              ",",     	//   [in,opt]  Value termination
	                              NULL,    	//   [in,opt]  Allowed padding
	                              &arg,			//   [out]     Output value
	                              1);      	//   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

	   if (!data) {
	      roadmap_log (ROADMAP_ERROR, "on_route_segments() - Failed to read 'instruction'");
	      return NULL;
	   }
	   if (arg < 0 || arg > LAST_DIRECTION) {
	      roadmap_log (ROADMAP_ERROR, "on_route_segments() - invalid instruction value %d", arg);
	      return NULL;
	   }

	   if (arg > 0) {
	   	arg -= 1;
	   } else {
	   	arg = CONTINUE;
	   }

	   if (RoutingContext.result[iresult].route_status != ROUTE_UPDATE) {
	   	segment->instruction = arg;
		} else if (segment->instruction != arg && !ignore_this) {
	   	roadmap_log (ROADMAP_DEBUG, "on_route_segments() - instruction %d changed on update to %d",
	   					 segment->instruction, arg);
	   }
	   num_args--;

	   if (num_args == 0) {
	      roadmap_log (ROADMAP_ERROR, "on_route_segments() - invalid number of arguments");
	      return NULL;
	   }

	   // exit_no
	   data = ReadIntFromString(  data,    	//   [in]      Source string
	                              num_args > 1 ? "," : "\r\n",
	                              				//   [in,opt]  Value termination
	                              NULL,    	//   [in,opt]  Allowed padding
	                              &arg,			//   [out]     Output value
	                              1);      	//   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

	   if (!data) {
	      roadmap_log (ROADMAP_ERROR, "on_route_segments() - Failed to read 'exit_no'");
	      return NULL;
	   }
	   if (RoutingContext.result[iresult].route_status != ROUTE_UPDATE) {
	   	segment->exit_no = arg;
	   } else if (segment->exit_no != arg && !ignore_this) {
	   	roadmap_log (ROADMAP_DEBUG, "on_route_segments() - exit %d changed on update to %d",
	   					 segment->exit_no, arg);
	   }
	   roadmap_log (ROADMAP_DEBUG, "Tile %d update %d segment %d from %d dist %d time %d inst %d exit %d",
	   			square,
	   			segment->update_time,
					segment->line,
	   			segment->from_node_id,
					segment->distance,
					segment->cross_time,
					segment->instruction,
					segment->exit_no);

		if (pending_time > 0) {
			adjust_segment_times (RoutingContext.route.num_received, pending_length, pending_time);
		}

		RoutingContext.route.num_received++;
		if (RoutingContext.result[iresult].route_status != ROUTE_UPDATE) {
		   RoutingContext.route.num_valid++;
		}

	   num_args--;
   }

	if (RoutingContext.route.num_received == RoutingContext.route.num_segments) {

		int l = 0;
		int t = 0;
		int i;

		for (i = 0; i < RoutingContext.route.num_received; i++) {
			l += RoutingContext.route.segments[i].distance;
			t += RoutingContext.route.segments[i].cross_time;
		}

		roadmap_log (ROADMAP_DEBUG, "Route statistics: %d segments, length %d time %d", i, l, t);
	}

   data = EatChars (data, "\r\n", TRIM_ALL_CHARS);
   if (!data) {
      roadmap_log (ROADMAP_ERROR, "on_route_points() - illegal characters at end of line");
      return NULL;
   }

	on_route_complete ();

   // Fix [out] param:
   (*rc) = succeeded;

   return data;
}

void navigate_route_request (const PluginLine *from_line,
                             int from_point,
                             const PluginLine *to_line,
                             const RoadMapPosition *from_pos,
                             const RoadMapPosition *to_pos,
                             const char *to_street,
                             const char *to_street_number,
                             const char *to_city,
                             const char *to_state,
                             int   twitter_mode,
                             int flags,
                             int trip_id,
                             int max_routes,
                             const NavigateRouteCallbacks *cb)
{
	int line_from_point;
	int line_to_point;
	int from_point_ids[2];
	int to_point_ids[2];
	const char *from_street;
	RoadMapStreetProperties street_props;
	int option_types[MAX_ROUTING_OPTIONS];
	BOOL option_values[MAX_ROUTING_OPTIONS];
	int num_options = 0;
	int route_type;
	BOOL bRes;

	if (flags & RECALC_ROUTE) {

		if (RoutingContext.rc == 0) {

			// still waiting for response
			return;
		}

		navigate_route_clear_context ();
	} else {

		roadmap_log(ROADMAP_DEBUG, "calling navigate_route_init_context from navigate_route_request()");
		navigate_route_init_context ();
	}

	RoutingContext.callbacks = cb;
	RoutingContext.pos_src = *from_pos;
	RoutingContext.pos_dst = *to_pos;

	if (from_line){
	   roadmap_square_set_current (from_line->square);
	   roadmap_line_points (from_line->line_id, &line_from_point, &line_to_point);

	   if (line_to_point == from_point) {
	      roadmap_line_point_ids (from_line->line_id, &from_point_ids[0], &from_point_ids[1]);
	   } else {
	      roadmap_line_point_ids (from_line->line_id, &from_point_ids[1], &from_point_ids[0]);
	   }

	   roadmap_street_get_properties (from_line->line_id, &street_props);
	   from_street = roadmap_street_get_street_fename (&street_props);

	}
	else{
	   from_point_ids[0] = -1;
	   from_point_ids[1] = -1;
	   from_street = "";
	}
	roadmap_log (ROADMAP_DEBUG, "Fr: %d - %d (%s)", from_point_ids[0], from_point_ids[1], from_street);

	if (to_line){
	   if (to_line->plugin_id == ROADMAP_PLUGIN_ID) {
	      roadmap_square_set_current (to_line->square);
	      roadmap_line_point_ids (to_line->line_id, &to_point_ids[0], &to_point_ids[1]);
	      roadmap_street_get_properties (to_line->line_id, &street_props);
	      to_street = roadmap_street_get_street_fename (&street_props);
	   } else {
	      to_point_ids[0] = -1;
	      to_point_ids[1] = -1;
	      if (!to_street) {
	         to_street = "";
	      }
	   }
	}
	else{
      to_point_ids[0] = -1;
      to_point_ids[1] = -1;
      if (!to_street)
      	to_street = "";
	}
	roadmap_log (ROADMAP_DEBUG, "To: %d - %d (%s)", to_point_ids[0], to_point_ids[1], to_street);

	// enumerate options
	option_types[num_options] = AVOID_PRIMARIES;
	option_values[num_options] = navigate_cost_avoid_primaries () ? TRUE : FALSE;
	num_options++;

	option_types[num_options] = AVOID_TRAILS;
	option_values[num_options] = (navigate_cost_avoid_trails () == AVOID_ALL_DIRT_ROADS) ? TRUE : FALSE;
	num_options++;

	option_types[num_options] = AVOID_LONG_TRAILS;
	option_values[num_options] = (navigate_cost_avoid_trails () == AVOID_LONG_DIRT_ROADS) ? TRUE : FALSE;
	num_options++;

	option_types[num_options] = PREFER_SAME_STREET;
	option_values[num_options] = navigate_cost_prefer_same_street () ? TRUE : FALSE;
	num_options++;

	option_types[num_options] = IGNORE_REALTIME_INFO;
	option_values[num_options] = navigate_cost_use_traffic () ? FALSE : TRUE;
	num_options++;

	option_types[num_options] = ALLOW_UNKNOWN_DIRECTIONS;
	option_values[num_options] = navigate_cost_allow_unknowns () ? TRUE : FALSE;
	num_options++;

	if (flags & ALLOW_DESTINATION_CHANGE) {

		option_types[num_options] = ALLOW_NEAREST_REACHABLE_DESTINATION;
		option_values[num_options] = TRUE;
		num_options++;
	}

	if (flags & ALLOW_ALTERNATE_SOURCE) {

		option_types[num_options] = ALLOW_NEAREST_CONNECTED_ORIGIN;
		option_values[num_options] = TRUE;
		num_options++;
	}

   option_types[num_options] = AVOID_TOLL_ROADS;
   option_values[num_options] = navigate_cost_avoid_toll_roads() ? TRUE : FALSE;
   num_options++;

   option_types[num_options] = PREFER_UNKNOWN_DIRECTIONS;
   option_values[num_options] = navigate_cost_prefer_unknown_directions() ? TRUE : FALSE;
   num_options++;


   // this option will be true iff palestinian road routing options are enabled, (first condition)
   // and user decided to avoid them ( second condition) - D.F.
   option_types[num_options] = AVOID_PALESTINIAN_ROADS;
   option_values[num_options] = (navigate_cost_isPalestinianOptionEnabled()&&
   								 navigate_cost_avoid_palestinian_roads())        ?
                                 TRUE : FALSE;
   num_options++;



	// route type
   if (navigate_cost_type () == COST_FASTEST) {
      route_type = ROUTE_SHORTEST_HISTORIC_TIME;
   } else {
      route_type = ROUTE_SHORTEST_DISTANCE;
   }

   if (max_routes >= MAX_RESULTS) {
   	max_routes = MAX_RESULTS - 1;
   }

   RoutingContext.flags = flags;
   bRes = Realtime_RequestRoute (RoutingContext.route_id,	 //iRoute
								  route_type,						       //iType
								  trip_id,							       //iTripId
								  RoutingContext.alt_id,		       //iAltId
								  max_routes,						       //nMaxRoutes
								  -1,									       //nMaxSegments
								  1000,								       //nMaxPoints
								  *from_pos,						       //posFrom
								  -1,									       //iFrSegmentId
								  from_point_ids,					       //iFrNodeId[2]
								  from_street,						       //szFrStreet
								  FALSE,								       //bFrAllowBidi
								  *to_pos,							       //posTo
								  -1,									       //iToSegmentId
								  to_point_ids,					       //iToNodeId[2]
								  to_street,						       //szToStreet
								  to_street_number,                  //szToStreetNumber
								  to_city,                           //szToCity
								  to_state,                          //szToState
								  TRUE,								       //bToAllowBidi
								  num_options,						       //nOptions
								  option_types,				          //iOptionNumeral
								  option_values,					       //bOptionValue
								  twitter_mode,                      //iTwitterLevel
	//SRUL: TEMP PATCH: DON'T REROUTE
									FALSE);
								  //(flags & RECALC_ROUTE) ? TRUE : FALSE);
	//END PATCH

	if (!bRes) {
		RoutingContext.rc = 500;
		RoutingContext.route_rc = route_server_error;
		routing_error ("Failed to Communicate with Routing Server");
	}
}

void navigate_route_select (int alt_id)
{
	int iresult = find_alt_id (alt_id);
	int i;

	if (iresult < 0) {

		roadmap_log (ROADMAP_ERROR, "navigate_route_select() : invalid alt_id %d", alt_id);
		return;
	}

	// copy result to first slot, and free all other results
	for (i = 0; i < MAX_RESULTS; i++) {
		if (i != iresult) {
			free_result (i);
		}
	}

	if (iresult > 0) {
		RoutingContext.result[0] = RoutingContext.result[iresult];
	}

	// send selection to server
	Realtime_SelectRoute (RoutingContext.route_id, alt_id);
}

void navigate_route_cancel_request (void)
{
	roadmap_log(ROADMAP_DEBUG, "calling navigate_route_init_context from navigate_route_cancel_request()");
	navigate_route_init_context ();
}


const char *on_suggest_reroute (/* IN  */   const char*       data,
                          		   	  /* IN  */   void*             context,
                                 	  /* OUT */   BOOL*             more_data_needed,
                                 	  /* OUT */   roadmap_result*   rc)
{
	int reroute_segment;
	int time_before;
	int time_after;

   // Default error for early exit:
   (*rc) = err_parser_unexpected_data;

   // Expected data:
   //    <route_id>,<num_responses>,<rc>,<description>\n

   // route_id
   if (!verify_route_id (&data, rc)) return data;

 	roadmap_log (ROADMAP_DEBUG, "SuggestReroute: %s", data);

 	// reroute segment
   data = ReadIntFromString(  data,          					//   [in]      Source string
                              ",",           					//   [in,opt]  Value termination
                              NULL,          					//   [in,opt]  Allowed padding
                              &reroute_segment,					//   [out]     Output value
                              1);            					//   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

   if (!data) {
      roadmap_log (ROADMAP_ERROR, "on_suggest_reroute() - Failed to read 'reroute_segment'");
      return NULL;
   }

 	// time_before
   data = ReadIntFromString(  data,          					//   [in]      Source string
                              ",",           					//   [in,opt]  Value termination
                              NULL,          					//   [in,opt]  Allowed padding
                              &time_before,					//   [out]     Output value
                              1);            					//   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

   if (!data) {
      roadmap_log (ROADMAP_ERROR, "on_suggest_reroute() - Failed to read 'time_before'");
      return NULL;
   }

 	// time_after
   data = ReadIntFromString(  data,          					//   [in]      Source string
                              ",\r\n",           					//   [in,opt]  Value termination
                              NULL,          					//   [in,opt]  Allowed padding
                              &time_after,					//   [out]     Output value
                              TRIM_ALL_CHARS);            					//   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

   if (!data) {
      roadmap_log (ROADMAP_ERROR, "on_suggest_reroute() - Failed to read 'time_after'");
      return NULL;
   }

 	roadmap_log (ROADMAP_DEBUG, "SuggestReroute for id %d segment %d, time improves from %d to %d",
 					 RoutingContext.route_id, reroute_segment, time_before, time_after);

 	if (RoutingContext.callbacks->on_reroute) {
 		RoutingContext.callbacks->on_reroute (reroute_segment, time_before, time_after);
 	}

   // Fix [out] param:
   (*rc) = succeeded;

   return data;
}

