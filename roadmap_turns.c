/* roadmap_turns.c - Manage turn restrictions db.
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
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
 *   int  roadmap_turns_in_square (int square, int *first, int *last);
 *   int  roadmap_turns_of_node   (int node, int begin, int end,
 *                                 int *first, int *last);
 *   int roadmap_turns_find_restriction (int node, int from_line, int to_line);
 *
 * These functions are used to retrieve the turn restrictions that belong to a node.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "roadmap.h"
#include "roadmap_dbread.h"
#include "roadmap_db_turns.h"

#include "roadmap_point.h"
#include "roadmap_turns.h"
#include "roadmap_square.h"


//static char *RoadMapTurnsType = "RoadMapTurnsContext";

typedef struct {

   char *type;

   RoadMapTurns *Turns;
   int           TurnsCount;

   RoadMapTurnsByNode *TurnsByNode;
   int                 TurnsByNodeCount;

   RoadMapTurnsBySquare *TurnsBySquare;
   int                   TurnsBySquareCount;

   int *turns_cache;
   int  turns_cache_size;  /* This is the size in bits ! */

} RoadMapTurnsContext;

static RoadMapTurnsContext *RoadMapTurnsActive = NULL;

static int RoadMapTurns2Mask[8*sizeof(int)] = {0};

#if 0
static void *roadmap_turns_map (roadmap_db *root) {

   unsigned i;
   RoadMapTurnsContext *context;

   roadmap_db *turns_table;
   roadmap_db *node_table;
   roadmap_db *square_table;


   for (i = 0; i < 8*sizeof(int); i++) {
      RoadMapTurns2Mask[i] = 1 << i;
   }


   context = malloc(sizeof(RoadMapTurnsContext));
   roadmap_check_allocated(context);

   context->type = RoadMapTurnsType;

   turns_table  = roadmap_db_get_subsection (root, "data");
   node_table   = roadmap_db_get_subsection (root, "bynode");
   square_table = roadmap_db_get_subsection (root, "bysquare");

   context->Turns = (RoadMapTurns *) roadmap_db_get_data (turns_table);
   context->TurnsCount = roadmap_db_get_count (turns_table);

   if (roadmap_db_get_size (turns_table) !=
       context->TurnsCount * sizeof(RoadMapTurns)) {
      roadmap_log (ROADMAP_FATAL, "invalid turns/data structure");
   }

   context->TurnsByNode =
      (RoadMapTurnsByNode *) roadmap_db_get_data (node_table);
   context->TurnsByNodeCount = roadmap_db_get_count (node_table);

   if (roadmap_db_get_size (node_table) !=
       context->TurnsByNodeCount * sizeof(RoadMapTurnsByNode)) {
      roadmap_log (ROADMAP_FATAL, "invalid turns/byline structure");
   }

   context->TurnsBySquare =
      (RoadMapTurnsBySquare *) roadmap_db_get_data (square_table);
   context->TurnsBySquareCount = roadmap_db_get_count (square_table);

   if (roadmap_db_get_size (square_table) !=
       context->TurnsBySquareCount * sizeof(RoadMapTurnsBySquare)) {
      roadmap_log (ROADMAP_FATAL, "invalid turns/bysquare structure");
   }

   context->turns_cache = NULL;
   context->turns_cache_size = 0;

   return context;
}

static void roadmap_turns_activate (void *context) {

   RoadMapTurnsContext *turns_context = (RoadMapTurnsContext *) context;

   if (turns_context != NULL) {

      if (turns_context->type != RoadMapTurnsType) {
         roadmap_log (ROADMAP_FATAL, "cannot activate turns (bad type)");
      }

      if (turns_context->turns_cache == NULL) {

         turns_context->turns_cache_size = roadmap_point_count();
         turns_context->turns_cache =
            calloc ((turns_context->turns_cache_size / (8 * sizeof(int))) + 1,
                  sizeof(int));
         roadmap_check_allocated(turns_context->turns_cache);
      }
   }

   RoadMapTurnsActive = turns_context;
}

static void roadmap_turns_unmap (void *context) {

   RoadMapTurnsContext *turns_context = (RoadMapTurnsContext *) context;

   if (turns_context->type != RoadMapTurnsType) {
      roadmap_log (ROADMAP_FATAL, "cannot unmap turns (bad type)");
   }
   if (RoadMapTurnsActive == turns_context) {
      RoadMapTurnsActive = NULL;
   }
   if (turns_context->turns_cache != NULL) {
      free (turns_context->turns_cache);
   }
   free(turns_context);
}

roadmap_db_handler RoadMapTurnsHandler = {
   "turns",
   roadmap_turns_map,
   roadmap_turns_activate,
   roadmap_turns_unmap
};
#endif

int  roadmap_turns_in_square (int square, int *first, int *last) {

   RoadMapTurnsBySquare *TurnsBySquare;

   if (RoadMapTurnsActive == NULL) return 0;

   //square = roadmap_square_index(square);

   if (square >= 0 && square < RoadMapTurnsActive->TurnsBySquareCount) {

      TurnsBySquare = RoadMapTurnsActive->TurnsBySquare;

      *first = TurnsBySquare[square].first;
      *last  = TurnsBySquare[square].first + TurnsBySquare[square].count - 1;

      return TurnsBySquare[square].count;
   }

   return 0;
}


int  roadmap_turns_of_node (int node, int begin, int end,
                            int *first, int *last) {

   int middle = 0;
   RoadMapTurnsByNode *turns_by_node;


   if (RoadMapTurnsActive == NULL) return 0;

   if (node >= 0 && node < RoadMapTurnsActive->turns_cache_size) {

      int mask = RoadMapTurnsActive->turns_cache[node / (8 * sizeof(int))];

      if (mask & RoadMapTurns2Mask[node & ((8*sizeof(int))-1)]) {
         return 0;
      }
   }

   turns_by_node = RoadMapTurnsActive->TurnsByNode;

   begin--;
   end++;

   while (end - begin > 1) {

      middle = (begin + end) / 2;

      if (node < turns_by_node[middle].node) {

         end = middle;

      } else if (node > turns_by_node[middle].node) {

         begin = middle;

      } else {

         end = middle;

         break;
      }
   }

   if (turns_by_node[end].node == node) {

      *first = turns_by_node[end].first;
      *last  = turns_by_node[end].first + turns_by_node[end].count - 1;

      return turns_by_node[end].count;
   }

   RoadMapTurnsActive->turns_cache[node / (8 * sizeof(int))] |=
      RoadMapTurns2Mask[node & ((8*sizeof(int))-1)];

   return 0;
}


int roadmap_turns_find_restriction (int node, int from_line, int to_line) {

   static int cache_square = -1;
   static int cache_first = -1;
   static int cache_last = -1;

   int square;
   int first_turn;
   int last_turn;
   int i;

   from_line = abs(from_line);
   to_line = abs(to_line);

   /* no U turns */
   if (from_line == to_line) return 1;
   
   square = roadmap_square_active ();

   if (square != cache_square) {
      cache_square = square;
      if (!roadmap_turns_in_square (cache_square, &cache_first, &cache_last)) {
         cache_first = cache_last = -1;
      }
   }

   if (cache_first < 0) return 0;
   
   i = roadmap_turns_of_node (node, cache_first, cache_last, &first_turn, &last_turn);

   if (!i) return 0;

   for (; first_turn <= last_turn; first_turn++) {

      if ((RoadMapTurnsActive->Turns[first_turn].from_line == from_line) &&
            RoadMapTurnsActive->Turns[first_turn].to_line == to_line) {

         return 1;
      }
   }

   return 0;
}

