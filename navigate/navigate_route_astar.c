/* navigate_route_astar.c - astar route calculation
 *
 * LICENSE:
 *
 *   Copyright 2007 Ehud Shabtai
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
 *   See navigate_route.h
 */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "roadmap.h"
#include "roadmap_point.h"
#include "roadmap_line.h"
#include "roadmap_locator.h"
#include "roadmap_layer.h"
#include "roadmap_square.h"
#include "roadmap_math.h"
#include "roadmap_turns.h"
#include "roadmap_main.h"
#include "roadmap_line_route.h"
#include "roadmap_hash.h"
#include "roadmap_navigate.h"

#ifdef SSD
#include "ssd/ssd_dialog.h"
#else
#include "roadmap_dialog.h"
#endif

#include "navigate_main.h"
#include "navigate_traffic.h"
#include "navigate_graph.h"
#include "navigate_cost.h"

#include "fib-1.1/fib.h"
#include "navigate_route.h"

#define LOCKED_ROUTE (1 << 7)

#define UNREACHED ((PrevItem)-1)
#define IN_LAST_ROUTE ((PrevItem)-2)

#define HU_SPEED 28 /* Meters per second */
#define MAX_SUCCESSORS 100

#define NUM_IGNORE_SEGMENTS 10
#define MIN_COST_FACTOR 0.25
#define COST_FACTOR_UPDATE 0.98

#define HASH_BLOCK_SIZE 4096
#define HASH_MAX_BLOCKS 40
#define MAX_ASTAR_LINES (HASH_BLOCK_SIZE * HASH_MAX_BLOCKS)
static int RouteNumNodes = 0;

#define MAX_REROUTE_ATTEMPS	100

static RoadMapHash *RouteGraph;
static RoadMapPosition GoalPos;

typedef struct {
	int					line_square;
	int					prev_square;
	unsigned short		line_id;
	unsigned short		prev_id;
} NavItem;
static NavItem *NavNode[HASH_MAX_BLOCKS];

typedef struct {
	int square;
	int last_line;
	int node;
	RoadMapPosition position;
	int goal_square;
	int goal_node;
} DestNode;

#define MAX_REPLACE_DISTANCE 300
#define MAX_DEAD_END_SEGMENTS	10

#ifdef J2ME
#define MAX_NAV_SEGEMENTS 50
#else
#define MAX_NAV_SEGEMENTS 2500
#endif

static NavigateSegment NavigateSegments[MAX_NAV_SEGEMENTS];

int navigate_route_reload_data (void) {

   return 0;
}

int navigate_route_load_data (void) {
   return 0;
}


static int hash_key (int square, int line, int reversed) {

	return (square << 16) + line * 2 + (reversed != 0);
}


static NavItem *make_path (int square_id, int line_id, int line_reversed,
							  		int prev_square, int prev_line, int prev_reversed) {

   NavItem *item;

	if (RouteNumNodes >= MAX_ASTAR_LINES) {
		roadmap_log (ROADMAP_ERROR, "Too many nodes in route calculation");
		return NULL;
	}

	if (RouteNumNodes % HASH_BLOCK_SIZE == 0) {
		if (RouteNumNodes) roadmap_hash_resize (RouteGraph, RouteNumNodes + HASH_BLOCK_SIZE);
		NavNode[RouteNumNodes / HASH_BLOCK_SIZE] = (NavItem *)malloc (HASH_BLOCK_SIZE * sizeof (NavItem));
	}

	item = NavNode[RouteNumNodes / HASH_BLOCK_SIZE] + (RouteNumNodes % HASH_BLOCK_SIZE);
	item->prev_square = prev_square | (prev_reversed ? REVERSED : 0);
	item->prev_id = prev_line;
	item->line_square = square_id | (line_reversed ? REVERSED : 0);
	item->line_id = line_id;

	//printf ("Adding path (%d/%d)%s -> (%d/%d)%s\n",
	//			item->prev_square & ~REVERSED, item->prev_id, item->prev_square & REVERSED ? "'" : "",
	//			item->line_square & ~REVERSED, item->line_id, item->line_square & REVERSED ? "'" : "");

	roadmap_hash_add (RouteGraph, hash_key (square_id, line_id, line_reversed), RouteNumNodes);
	RouteNumNodes++;

	return item;
}


static NavItem *find_prev (int square_id, int line_id, int line_reversed) {

	int key = hash_key (square_id, line_id, line_reversed);
	int index = roadmap_hash_get_first (RouteGraph, key);

	if (line_reversed) {
		square_id = square_id | REVERSED;
	}

	while (index >= 0) {
		NavItem *item = NavNode[index / HASH_BLOCK_SIZE] + (index % HASH_BLOCK_SIZE);
		if (item->line_square == square_id &&
			 item->line_id == line_id) {

			return item;
		}
		index = roadmap_hash_get_next (RouteGraph, index);
	}

	return NULL;
}


static void get_to_node (int square, int line_id, int reversed, int *node, RoadMapPosition *position) {

	roadmap_square_set_current (square);
	if (reversed) {
		roadmap_line_from_point (line_id, node);
	} else {
		roadmap_line_to_point (line_id, node);
	}
	roadmap_point_position (*node, position);
}



struct fibheap * make_queue (int square, int line_id, int reversed) {

   struct fibheap *fh;
   NavItem *item = make_path (square, line_id, reversed, square, line_id, reversed);
   fh = fh_makekeyheap();

   fh_insertkey (fh, 0, item);

   return fh;
}

static void update_progress (int progress) {
   progress = progress * 9 / 10;

#ifdef SSD
   {
      char progress_str[10];

      snprintf (progress_str, sizeof(progress_str), "%d", progress);

      ssd_dialog_set_value ("Progress", progress_str);
      ssd_dialog_draw ();
   }
#else
   roadmap_dialog_set_progress ("Calculating", "Progress", progress);
#endif

   roadmap_main_flush ();
}

static int prepare_prev_list (const NavigateSegment *prev_route, int num_prev) {

   int i;

   RouteGraph = roadmap_hash_new ("astar", HASH_BLOCK_SIZE);
   RouteNumNodes = 0;

   for (i = 0; i < num_prev; i++) {
   	if (prev_route[i].context != SEG_ROUNDABOUT &&
   		 (i == 0 || prev_route[i - 1].context != SEG_ROUNDABOUT)) {
   		// making sure roundabout is not split between old and new route segments
	   	make_path (prev_route[i].square,
	   				  prev_route[i].line,
	   				  prev_route[i].line_direction != ROUTE_DIRECTION_WITH_LINE,
	   				  -1,
	   				  i,
	   				  0);
   	}
   }

   return 0;
}

static void free_prev_list(void) {

   int i;

   if (RouteGraph) {
	   roadmap_hash_free (RouteGraph);
	   RouteGraph = NULL;
   }
   if (RouteNumNodes) {
   	for (i = (RouteNumNodes - 1) / HASH_BLOCK_SIZE; i >= 0; i--) {
   		free (NavNode[i]);
   	}
   	RouteNumNodes = 0;
   }
}


static int astar(int *start_square, int start_node, int *start_segment, int *start_reversed,
                 PluginLine *goal, int *goal_node, int *route_total_cost, int *flags,
                 int *first_prev_segment, int *last_is_reversed)
{
   int last_square;
   int last_line;
   int last_line_reversed;
   int node;
   int no_successors;
   int i;
   struct successor successors[MAX_SUCCESSORS];
   NavItem *item;
   int cur_cost;
   int prev_cost;
   float goal_distance;
   int cur_max_progress;
   int progress;
   int num_heap_gets;
   RoadMapPosition position;
   int recalc = (*flags) & RECALC_ROUTE;
   int goal_square = goal->square;
   int goal_line = goal->line_id;
   int best_distance = MAX_REPLACE_DISTANCE + 1;
   int best_square = 0;
   int best_line = 0;
   int best_reversed = 0;
   int best_node = 0;
   int max_dead_end_attempts = ((*flags) & ALLOW_ALTERNATE_SOURCE) ? 5 : 0;
   int attempt;
   int dead_end_count = 0;
   int dead_lines[MAX_DEAD_END_SEGMENTS];
   int dead_squares[MAX_DEAD_END_SEGMENTS];
   int dead_reversed[MAX_DEAD_END_SEGMENTS];
   RoadMapNeighbour departure_alternatives[MAX_DEAD_END_SEGMENTS];
   int count;
   RoadMapPosition start_position;
   int out_of_memory;

   struct fibheap *q;
   NavigateCostFn cost_fn = navigate_cost_get ();
   int navigate_type = navigate_cost_type ();

	*first_prev_segment = -1;
	roadmap_square_set_current (goal_square);
   roadmap_point_position (*goal_node, &GoalPos);
   roadmap_square_set_current (*start_square);
   roadmap_point_position (start_node, &position);
   goal_distance = (float)roadmap_math_distance (&position, &GoalPos);
   start_position = position;
   cur_max_progress = 0;

	for (attempt = 0; attempt <= max_dead_end_attempts; attempt++) {

		if (attempt > 0) {
			int reversed = 0;

			if (dead_end_count >= MAX_DEAD_END_SEGMENTS) break;

			count = roadmap_navigate_get_neighbours
              (&start_position, 0, MAX_REPLACE_DISTANCE, 1,
               departure_alternatives,
               dead_end_count + 1,
               LAYER_ALL_ROADS);

         for (i = 0; i < count; i++) {

         	if (departure_alternatives[i].line.plugin_id == ROADMAP_PLUGIN_ID) {

         		int j;
         		int match = 0;

         		for (j = 0; j < dead_end_count; j++) {
         			if (departure_alternatives[i].line.square == dead_squares[j] &&
         				 departure_alternatives[i].line.line_id == dead_lines[j]) {
         				match++;
         				if (match == 2) break;
         				reversed = !dead_reversed[j];
         			}
         		}
         		if (j == dead_end_count) {
         			break;
         		}
         	}
         }

         if (i < count) {

         	*start_square = departure_alternatives[i].line.square;
         	*start_segment = departure_alternatives[i].line.line_id;
         	*start_reversed = reversed ? REVERSED : 0;
         	*flags |= CHANGED_DEPARTURE;
         }

		}

	   last_square = *start_square;
	   last_line = *start_segment;
	   last_line_reversed = *start_reversed;

	   q = make_queue (last_square, last_line, last_line_reversed);
		num_heap_gets = 0;

		out_of_memory = 0;
	   while (fh_min(q) != NULL && !out_of_memory) {

			if (((*flags) & USE_LAST_RESULTS) &&
				 num_heap_gets >= MAX_REROUTE_ATTEMPS) {

				break;
			}
	      num_heap_gets++;

	      cur_cost = fh_minkey(q);
	      item = (NavItem *)fh_extractmin(q);
	      last_square = item->line_square & ~REVERSED;
	      last_line = item->line_id;
	      last_line_reversed = item->line_square & REVERSED;

	      get_to_node (last_square, last_line, last_line_reversed, &node, &position);

	      prev_cost = cur_cost;
	      if (cur_cost) {
	         int dis = roadmap_math_distance (&position, &GoalPos);
	         if (navigate_type == COST_FASTEST) dis = (dis / HU_SPEED);
	         cur_cost -= dis;
	      }

	      if (last_square == goal_square &&
	      	 last_line == goal_line) {
	         *route_total_cost = cur_cost;
	         fh_deleteheap(q);
	         //printf("Total no. of heap gets in this search: %d\n", num_heap_gets);
	         //printf ("Final cost for track is %d\n", cur_cost);
	         *last_is_reversed = last_line_reversed;
	         return 0;
	      }

			if (dead_end_count < MAX_DEAD_END_SEGMENTS) {
				for (i = 0; i < dead_end_count; i++) {
					if (dead_squares[i] == last_square &&
						 dead_lines[i] == last_line &&
						 dead_reversed[i] == last_line_reversed) break;
				}
				if (i == dead_end_count) {
					dead_squares[dead_end_count] = last_square;
					dead_lines[dead_end_count] = last_line;
					dead_reversed[dead_end_count] = last_line_reversed;
					dead_end_count++;
				}
			}

	      no_successors = get_connected_segments (last_square, last_line, last_line_reversed, node,
	                                    			 successors, MAX_SUCCESSORS, 1, 1);
	      if (!no_successors) {
	         continue;
	      }

	      for(i = 0; i < no_successors; i++) {
	         int path_cost;
	         int square;
	         int segment;
	         int segment_cost;
	         int distance_to_goal;
	         int cost_to_goal;
	         int total_cost;
	         int is_reversed;
	         NavItem* prev_ptr;
	         RoadMapPosition to_pos;

				square = successors[i].square_id;
	         segment = successors[i].line_id;
	         is_reversed = successors[i].reversed;

				prev_ptr = find_prev (square, segment, is_reversed);
				if (prev_ptr != NULL) {
					if (prev_ptr->prev_square == -1) {
						*first_prev_segment = prev_ptr->prev_id;
						prev_ptr->prev_square = last_square | (last_line_reversed ? REVERSED : 0);
						prev_ptr->prev_id = last_line;
						fh_deleteheap(q);
						return 0;
					}
					continue;
				}

				roadmap_square_set_current (square);
	         segment_cost = cost_fn (segment, is_reversed, cur_cost,
	         								last_line, last_line_reversed,
	         								square == last_square ? node : -1);

	         if (segment_cost < 0) continue;

	         path_cost = segment_cost + cur_cost;
	         roadmap_point_position (successors[i].to_point, &to_pos);
	         distance_to_goal = roadmap_math_distance (&to_pos, &GoalPos);

	         if (distance_to_goal < best_distance) {

	         	best_square = square;
	         	best_line = segment;
	         	best_reversed = is_reversed;
	         	best_distance = distance_to_goal;
	         	best_node = successors[i].to_point;
	         }

	         //printf ("Sq %d seg %d node %d\n"
	         //		  " pt %d.%d dist %d\n",
	         //		  square, segment, successors[i].to_point,
	         //		  to_pos.longitude, to_pos.latitude, distance_to_goal);

	         if (navigate_type == COST_FASTEST) {
	            cost_to_goal = (distance_to_goal / HU_SPEED);
	         } else {
	            cost_to_goal = distance_to_goal;
	         }

	         total_cost = path_cost + cost_to_goal + 1;
	         if (total_cost < prev_cost) {
	            total_cost = prev_cost;
	         }

	         prev_ptr = make_path (square, segment, is_reversed,
	         				  	   	 last_square, last_line, last_line_reversed);

				if (!prev_ptr) {

					out_of_memory = 1;
					break;
				}

	         fh_insertkey(q, total_cost, prev_ptr);

				progress = (int)(100 * (1 - sqrt ((float)distance_to_goal / goal_distance)));
	         if ((progress >> 2 ) > (cur_max_progress >> 2)) {
	            cur_max_progress = progress;
	            if (!recalc) update_progress (cur_max_progress);
	         }

	      }
	   }
	   fh_deleteheap(q);
	}

	if (((*flags) & ALLOW_DESTINATION_CHANGE) &&
		 best_distance <= MAX_REPLACE_DISTANCE) {

		goal->square = best_square;
		goal->line_id = best_line;
		*flags |= CHANGED_DESTINATION;
		*goal_node = best_node;
		*last_is_reversed = best_reversed;
		return 0;
	}

   return -1;
}


static int navigate_route_calc_segments (PluginLine *from_line,
                                         int from_point,
                                         PluginLine *to_line,
                                         int *to_point,
                                         NavigateSegment **segments,
                                         int *num_total,
                                         int *num_new,
                                         int *flags,
                                         const NavigateSegment *prev_segments,
                                         int num_prev_segments) {

   int i;
   int total_cost;
   int line_from_point;
   int line_to_point;
   int line;
   int square;
   int line_reversed;
   int start_line_reversed;
   int start_square = from_line->square;
   int start_line = from_line->line_id;
   int first_prev_segment;
   int rc;
   NavItem *prev_item;
   NavigateSegment *curr_segment;

   //FIXME add plugin support
   roadmap_square_set_current (start_square);
   roadmap_line_points (start_line, &line_from_point, &line_to_point);
   if (from_point == line_to_point) start_line_reversed = 0;
   else if (from_point == line_from_point) start_line_reversed = REVERSED;
   else start_line_reversed = 0;

   rc = astar (&start_square, from_point, &start_line, &start_line_reversed,
   				  to_line, to_point, &total_cost, flags, &first_prev_segment, &line_reversed);

   if (rc == -1) {
      return -1;
   }

   i = 0;
   curr_segment = NavigateSegments + MAX_NAV_SEGEMENTS;


   //printf("Route: ");
   square = to_line->square;
   line = to_line->line_id;

   if (first_prev_segment >= 0) {

   	const NavigateSegment *prev_segment = prev_segments + first_prev_segment;

		prev_item = find_prev (prev_segment->square,
									  prev_segment->line,
									  prev_segment->line_direction != ROUTE_DIRECTION_WITH_LINE);
		if (!prev_item) {
			roadmap_log (ROADMAP_ERROR, "Inconsistency in route calculation");
			return -1;
		}

		square = prev_item->prev_square & ~REVERSED;
		line = prev_item->prev_id;
		line_reversed = prev_item->prev_square & REVERSED;
   }

   while (1) {
      int prev_node;

      i++;
      curr_segment--;

		roadmap_square_set_current (square);
		curr_segment->is_instrumented = 0;
      curr_segment->square = square;
      curr_segment->line = line;
      //printf("%d, ", line);
      curr_segment->cfcc = roadmap_line_cfcc (line);
      curr_segment->line_direction = line_reversed ? ROUTE_DIRECTION_AGAINST_LINE : ROUTE_DIRECTION_WITH_LINE;
      //printf ("Segment %d: (%d/%d)%s\n", i,
      //			segments[*size - i].line.square,
      //			segments[*size - i].line.line_id,
      //			segments[*size - i].line_direction == ROUTE_DIRECTION_AGAINST_LINE ? "'" : "");

      if (line_reversed) {
         roadmap_line_to_point (line, &prev_node);
      } else {
         roadmap_line_from_point (line, &prev_node);
      }

      /* Check if we got to the first line */
      if ((line == start_line) &&
      	 (square == start_square) &&
          (line_reversed == start_line_reversed)) break;

		prev_item = find_prev (square, line, line_reversed);
		if (!prev_item) {
			roadmap_log (ROADMAP_ERROR, "Inconsistency in route calculation");
			return -1;
		}

		square = prev_item->prev_square & ~REVERSED;
		line = prev_item->prev_id;
		line_reversed = prev_item->prev_square & REVERSED;

      if (i >= MAX_NAV_SEGEMENTS) {
      	if ((*flags) & CHANGED_DEPARTURE) break;
         return -1;
      }
   }
   //printf("\n");

   /* add starting line */
   if (((*flags) & CHANGED_DEPARTURE) == 0 &&
   	 (curr_segment->line != start_line ||
   	  curr_segment->square != start_square ||
   	  curr_segment->line_direction !=
   	  	(start_line_reversed ? ROUTE_DIRECTION_AGAINST_LINE : ROUTE_DIRECTION_WITH_LINE))) {

      int tmp_from;
      int tmp_to;

      i++;
      curr_segment--;

      if (i > MAX_NAV_SEGEMENTS) {
         return -1;
      }

      roadmap_square_set_current (from_line->square);
      roadmap_line_points (from_line->line_id, &tmp_from, &tmp_to);

		curr_segment->is_instrumented = 0;
      curr_segment->square = from_line->square;
      curr_segment->line = from_line->line_id;
      curr_segment->cfcc = from_line->cfcc;

      if (from_point == tmp_from) {
         curr_segment->line_direction = ROUTE_DIRECTION_AGAINST_LINE;
      } else {
         curr_segment->line_direction = ROUTE_DIRECTION_WITH_LINE;
      }
   }

   assert(curr_segment >= NavigateSegments);

	*segments = curr_segment;

	*num_new = i;
   if (first_prev_segment >= 0) {
   	*num_total = i + (num_prev_segments - first_prev_segment);
   } else {
   	*num_total = i;
   }

   return total_cost + 1;
}


int navigate_route_get_segments (PluginLine *from_line,
                                 int from_point,
                                 PluginLine *to_line,
                                 int *to_point,
                                 NavigateSegment **segments,
                                 int *num_total,
                                 int *num_new,
                                 int *flags,
                                 const NavigateSegment *prev_segments,
                                 int num_prev_segments) {

   static int inside_route = 0;
   int reuse = (*flags & USE_LAST_RESULTS);
   int rc;
   int prev_scale = roadmap_square_get_screen_scale ();

   if (inside_route) {
      roadmap_log (ROADMAP_ERROR, "re-entering navigate_route_get_segments");
      return -1;
   }
   inside_route = 1;

   if (prepare_prev_list (prev_segments, reuse ? num_prev_segments : 0)) {
      inside_route = 0;
      return -1;
   }

	roadmap_square_set_screen_scale (0);
   rc = navigate_route_calc_segments(from_line, from_point, to_line, to_point, segments,
   											 num_total, num_new, flags,
   											 prev_segments, num_prev_segments);
   if (rc > 0)
   	roadmap_log (ROADMAP_INFO, "Found route: %d segments (%d new)", *num_total, *num_new);

   roadmap_square_set_screen_scale (prev_scale);

   free_prev_list();

   inside_route = 0;
   return rc;
}

