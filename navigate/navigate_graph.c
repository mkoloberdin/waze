/* navigate_graph.c - Implements a graph for routing calculation
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
 *   See navigate_graph.h
 */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "roadmap.h"
#include "roadmap_point.h"
#include "roadmap_line.h"
#include "roadmap_locator.h"
#include "roadmap_layer.h"
#include "roadmap_square.h"
#include "roadmap_math.h"
#include "roadmap_turns.h"
#include "roadmap_dialog.h"
#include "roadmap_main.h"
#include "roadmap_line_route.h"
#include "roadmap_plugin.h"
#include "roadmap_navigate.h"

#include "navigate_graph.h"

#ifdef J2ME
#define MAX_GRAPH_CACHE 75
#define MAX_MEM_CACHE 150000
#else
#define MAX_GRAPH_CACHE 256
#define MAX_MEM_CACHE 500000
#endif

struct SquareGraphItem {
   int square_id;
   unsigned short lines_count;
   unsigned short nodes_count;
   unsigned short *nodes_index;
   int *lines;
   unsigned short *lines_index;
   int mem_size;
};

static struct SquareGraphItem *SquareGraphCache[MAX_GRAPH_CACHE];
static int SquareCacheSize = 0;
static int cache_total_mem;

static inline void add_graph_node(struct SquareGraphItem *cache,
                                  int line,
                                  int point_id,
                                  int cur_line,
                                  int reversed) {

   unsigned int l;

   l = cache->nodes_index[point_id];
	cache->nodes_index[point_id] = cur_line + 1;
	cache->lines_index[cur_line] = l;
/*
   if (l) {
      l--;
      while (cache->lines_index[l]) {
         l=cache->lines_index[l];
      }

      cache->lines_index[l] = cur_line;
   } else {
      cache->nodes_index[point_id] = cur_line + 1;
   }
*/
   cache->lines[cur_line] = line|reversed;
}


static void free_cache_slot (int slot) {
   free (SquareGraphCache[slot]->nodes_index);
   free (SquareGraphCache[slot]->lines);
   free (SquareGraphCache[slot]->lines_index);
   cache_total_mem -= SquareGraphCache[slot]->mem_size;
}


//TODO arrange as LRU list
static struct SquareGraphItem *get_square_graph (int square_id) {

   int i;
   int slot;
   int line;
   int lines1_count;
   int lines2_count;
   struct SquareGraphItem *cache;
   int cur_line = 0;
   int found = 0;

   for (slot = 0; slot < SquareCacheSize; slot++) {
     	if (SquareGraphCache[slot]->square_id == square_id) {
     		found = 1;
     		//printf ("get_square_graph: found square %d in slot %d\n", square_id, slot);
     		break;
     	}
   }

	if (!found) {
		if (SquareCacheSize == MAX_GRAPH_CACHE) {
			slot--;
			free_cache_slot (slot);
			cache = SquareGraphCache[slot];
		} else {
			cache = (struct SquareGraphItem *)malloc(sizeof(struct SquareGraphItem));
			SquareCacheSize++;
		}
	} else {
		cache = SquareGraphCache[slot];
	}

	for (i = slot; i > 0; i--) {
		SquareGraphCache[i] = SquareGraphCache[i - 1];
	}
	SquareGraphCache[0] = cache;

	if (found) return cache;

	//printf ("get_square_graph: adding square %d to slot %d\n", square_id, slot);
	
   cache->square_id = square_id;
   lines1_count = 0;
   lines2_count = 0;

   /* Count total lines */
   for (i = ROADMAP_ROAD_FIRST; i <= ROADMAP_ROAD_LAST; ++i) {

      int first_line;
      int last_line;

      if (roadmap_line_in_square
            (square_id, i, &first_line, &last_line) > 0) {

         lines1_count += (last_line - first_line + 1);
      }
   }

   cache->lines_count = lines1_count * 2 + lines2_count;

   /* assume that the number of nodes equals the number of lines */
   cache->nodes_count = roadmap_square_points_count (square_id);

   cache->mem_size = cache->lines_count * sizeof(int) +
                     cache->lines_count * sizeof(unsigned short) +
                     cache->nodes_count * sizeof(unsigned short);

   while (cache_total_mem &&
          ((cache_total_mem + cache->mem_size) > MAX_MEM_CACHE) &&
          SquareCacheSize > 1) {

		SquareCacheSize--;
      free_cache_slot(SquareCacheSize);
      free (SquareGraphCache[SquareCacheSize]);
      SquareGraphCache[SquareCacheSize] = NULL;
   }

   cache->lines = malloc(cache->lines_count * sizeof(int));
   cache->lines_index = calloc(cache->lines_count, sizeof(unsigned short));
   cache->nodes_index = calloc(cache->nodes_count, sizeof(unsigned short));

   cache_total_mem += cache->mem_size;

   for (i = ROADMAP_ROAD_LAST; i >= ROADMAP_ROAD_FIRST; --i) {

      int first_line;
      int last_line;

      if (roadmap_line_in_square
            (square_id, i, &first_line, &last_line) > 0) {

         for (line = last_line; line >= first_line; line--) {

            int from_point_id;
            int to_point_id;

            roadmap_line_points (line, &from_point_id, &to_point_id);
            from_point_id &= 0xffff;

            add_graph_node(cache, line, from_point_id, cur_line, 0);
            cur_line++;

            //if (roadmap_point_square(to_point_id) == square_id) {

               to_point_id &= 0xffff;

               add_graph_node(cache, line, to_point_id, cur_line, REVERSED);

               cur_line++;
            //}
         }
      }
   }

   assert(cur_line <= cache->lines_count);

   return cache;
}


static int extend_segment (const PluginLine *line, void *context, int extend_flags) {

	struct successor *successor = (struct successor *)context;
	
	successor->square_id = line->square;
	successor->line_id = line->line_id;
	
	roadmap_square_set_current (successor->square_id);
	if (extend_flags == FLAG_EXTEND_TO) {
		successor->reversed = 0;
		roadmap_line_to_point (successor->line_id, &successor->to_point);
	} else {
		successor->reversed = 1;
		roadmap_line_from_point (successor->line_id, &successor->to_point);
	}
	
	return 1;
}


static int find_segment_extension (int square,
									 		  int seg_line_id, int is_seg_reversed,
                            		  struct successor *successors) {

	PluginLine line;
	int flag = is_seg_reversed ? FLAG_EXTEND_FROM : FLAG_EXTEND_TO;
	
	line.square = square;
	line.line_id = seg_line_id;
	line.plugin_id = ROADMAP_PLUGIN_ID; 
	
	return roadmap_street_extend_line_ends (&line, NULL, NULL, flag, extend_segment, successors);
}


int get_connected_segments (int square,
									 int seg_line_id, int is_seg_reversed,
                            int node_id, struct successor *successors,
                            int max, int use_restrictions, int use_directions) {

   static int res_bits[8];
   int i;

   int count = 0;
   //int index = 0;
   int res_index = 0;
   int line;
   int line_reversed;
   int seg_res_bits = 0;
   struct SquareGraphItem *cache;

	if (max > 0 && 
		 find_segment_extension (square, seg_line_id, is_seg_reversed, successors)) {
		
		return 1;
	} 
	
   /* Init turn restrictions bits */
   if (!res_bits[0]) for (i=0; i<8; i++) res_bits[i] = 1<<i;

   roadmap_square_set_current (square);

   cache = get_square_graph (square);

   node_id &= 0xffff;

   i = cache->nodes_index[node_id];
   if (i <= 0) {
   	roadmap_log (ROADMAP_ERROR, "cannot find data for node %d square %d", node_id, square);
   }
   assert (i > 0);

   if (use_restrictions) {
      if (is_seg_reversed) {
         seg_res_bits = roadmap_line_route_get_restrictions (seg_line_id, 1);
      } else {
         seg_res_bits = roadmap_line_route_get_restrictions (seg_line_id, 0);
      }
   }

   while (i && count < max) {

      int to_point_id = -1;
      int line_direction_allowed;

      line = cache->lines[i - 1];
      i = cache->lines_index[i - 1];
      line_reversed = line & REVERSED;
      if (line_reversed) line = line & ~REVERSED;

      if (line == seg_line_id) {
         
         if (roadmap_line_route_get_direction
               (seg_line_id, ROUTE_CAR_ALLOWED) == ROUTE_DIRECTION_ANY) {
            res_index++;
         }
         continue;
      }

		if (use_directions) {
	      if (!line_reversed) {
	         roadmap_line_to_point (line, &to_point_id);
	         line_direction_allowed =
	            roadmap_line_route_get_direction (line, ROUTE_CAR_ALLOWED) &
	                  ROUTE_DIRECTION_WITH_LINE;
	      } else {
	         roadmap_line_from_point (line, &to_point_id);
	         line_direction_allowed =
	            roadmap_line_route_get_direction (line, ROUTE_CAR_ALLOWED) &
	                  ROUTE_DIRECTION_AGAINST_LINE;
	      }
	
	      if (!line_direction_allowed) {
	         continue;
	      }
		}

      if (!use_restrictions || (res_index >= 8) ||
          !(seg_res_bits & res_bits[res_index])) {
          	  successors[count].square_id = square;
              successors[count].line_id = line;
              successors[count].reversed = (line_reversed != 0);
              successors[count].to_point = to_point_id;
              count++;
      }
      res_index++;
   }

   return count;
}


int navigate_graph_get_line (int node, int line_no) {

   int square = roadmap_square_active (); //roadmap_point_square (node);
   struct SquareGraphItem *cache = get_square_graph (square);
   int i;
   int skip;

   node &= 0xffff;

   i = cache->nodes_index[node];
   assert (i > 0);
   i--;

   skip = line_no;
   while (skip) {
      i = cache->lines_index[i];
	   assert (i > 0);
	   i--;
      skip--;
   }

   return cache->lines[i];
}


static void navigate_graph_clear_all (void) {
	
	int slot;
	for (slot = SquareCacheSize - 1; slot >= 0; slot--) {
		
		free_cache_slot (slot);
	}
	SquareCacheSize = 0;	
}

void navigate_graph_clear (int square) {

	int slot;
	
	if (square == -1) {
		navigate_graph_clear_all ();
		return;
	}
	
	for (slot = SquareCacheSize - 1; slot >= 0 && SquareGraphCache[slot]->square_id != square; slot--)
		;
	
	if (slot >= 0) {
		
		SquareCacheSize--;
		free_cache_slot (slot);
		
		while (slot < SquareCacheSize) {
			SquareGraphCache[slot] = SquareGraphCache[slot + 1];
			slot++;	
		}
	}	
}
