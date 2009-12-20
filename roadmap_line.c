/* roadmap_line.c - Manage the tiger lines.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
 *   Copyright 2008 Ehud Shabtai
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
 *   int  roadmap_line_in_square (int square, int cfcc, int *first, int *last);
 *   void roadmap_line_from (int line, RoadMapPosition *position);
 *   void roadmap_line_to   (int line, RoadMapPosition *position);
 *
 *   int  roadmap_line_count (void);
 *
 * These functions are used to retrieve the points that make the lines.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#define DECLARE_ROADMAP_LINE

#include "roadmap.h"
#include "roadmap_dbread.h"
#include "roadmap_tile_model.h"
#include "roadmap_db_line.h"

#include "roadmap_point.h"
#include "roadmap_math.h"
#include "roadmap_shape.h"
#include "roadmap_square.h"
#include "roadmap_layer.h"
#include "roadmap_locator.h"
#include "roadmap_range.h"

#include "roadmap_line.h"

static char *RoadMapLineType = "RoadMapLineContext";

RoadMapLineContext *RoadMapLineActive = NULL;


static void *roadmap_line_map (const roadmap_db_data_file *file) {

   RoadMapLineContext *context;

   context = (RoadMapLineContext *) malloc (sizeof(RoadMapLineContext));
   if (context == NULL) {
      roadmap_log (ROADMAP_ERROR, "no more memory");
      return NULL;
   }
   context->type = RoadMapLineType;

   if (!roadmap_db_get_data (file,
   								  model__tile_line_data,
   								  sizeof (RoadMapLine),
   								  (void**)&(context->Line),
   								  &(context->LineCount))) {
      roadmap_log (ROADMAP_ERROR, "invalid line/data structure");
	   goto roadmap_line_map_abort;
   }

   if (!roadmap_db_get_data (file,
   								  model__tile_line_bysquare1,
   								  sizeof (RoadMapLineBySquare),
   								  (void**)&(context->LineBySquare1),
   								  &(context->LineBySquare1Count))) {
      roadmap_log (ROADMAP_ERROR, "invalid line/bysquare1 structure");
	   goto roadmap_line_map_abort;
   }

	if (context->LineBySquare1->first_broken[ROADMAP_DIRECTION_COUNT * 2] > 0) {
	   
	   if (!roadmap_db_get_data (file,
	   								  model__tile_line_broken,
	   								  sizeof (unsigned short),
	   								  (void**)&(context->BrokenLine),
	   								  &(context->BrokenLineCount))) {
	      roadmap_log (ROADMAP_ERROR, "invalid line/broken structure");
	      goto roadmap_line_map_abort;
	   }
	   
	   if (context->LineBySquare1->first_broken[ROADMAP_DIRECTION_COUNT * 2] != context->BrokenLineCount) {
	   	roadmap_log (ROADMAP_ERROR, "broken count mismatch");
	      goto roadmap_line_map_abort;
	   }
	} else {
	   context->BrokenLine = NULL;
	   context->BrokenLineCount = 0;
	}

	if (context->LineBySquare1->num_roundabout > 0) {
	   
	   if (!roadmap_db_get_data (file,
	   								  model__tile_line_roundabout,
	   								  sizeof (unsigned short),
	   								  (void**)&(context->RoundaboutLine),
	   								  &(context->RoundaboutLineCount))) {
	      roadmap_log (ROADMAP_ERROR, "invalid line/roundabout structure");
	      goto roadmap_line_map_abort;
	   }
	   
	   if (context->LineBySquare1->num_roundabout != context->RoundaboutLineCount) {
	   	roadmap_log (ROADMAP_ERROR, "roundabout count mismatch");
	      goto roadmap_line_map_abort;
	   }
	} else {
	   context->RoundaboutLine = NULL;
	   context->RoundaboutLineCount = 0;
	}

   return context;

roadmap_line_map_abort:

   free(context);
   return NULL;
}

static void roadmap_line_activate (void *context) {

   RoadMapLineContext *line_context = (RoadMapLineContext *) context;
   
   VERIFY_THREAD("roadmap_line_activate")

   if ((line_context != NULL) &&
       (line_context->type != RoadMapLineType)) {
      roadmap_log (ROADMAP_FATAL, "invalid line context activated");
   }
   RoadMapLineActive = line_context;
}

static void roadmap_line_unmap (void *context) {

   RoadMapLineContext *line_context = (RoadMapLineContext *) context;

   if (line_context->type != RoadMapLineType) {
      roadmap_log (ROADMAP_FATAL, "unmapping invalid line context");
   }
   free (line_context);
}

roadmap_db_handler RoadMapLineHandler = {
   "dictionary",
   roadmap_line_map,
   roadmap_line_activate,
   roadmap_line_unmap
};


int  roadmap_line_from_is_fake (int line) {

	return RoadMapLineActive->Line[line].from & POINT_FAKE_FLAG;
}


int  roadmap_line_to_is_fake (int line) {

	return RoadMapLineActive->Line[line].to & POINT_FAKE_FLAG;
}


int  roadmap_line_count (void) {

   if (RoadMapLineActive == NULL) return 0; /* No line. */

   return RoadMapLineActive->LineCount;
}

int roadmap_line_length (int line) {

   RoadMapPosition p1;
   RoadMapPosition p2;
   int length = 0;

   int first_shape;
   int last_shape;
   int i;

   roadmap_line_from (line, &p1);

   if (roadmap_line_shapes (line, &first_shape, &last_shape) > 0) {

      p2 = p1;
      for (i = first_shape; i <= last_shape; i++) {

         roadmap_shape_get_position (i, &p2);
         length += roadmap_math_distance (&p1, &p2);
         p1 = p2;
      }
   }

   roadmap_line_to (line, &p2);
   length += roadmap_math_distance (&p1, &p2);

   return length;
}


int roadmap_line_get_range (int line) {
   
   int range;

	if (RoadMapLineActive == NULL) return -1; /* No line. */
	
   VERIFY_THREAD("roadmap_line_get_range")

#ifdef DEBUG
   if (line < 0 || line >= RoadMapLineActive->LineCount) {
      roadmap_log (ROADMAP_FATAL, "illegal line index %d", line);
   }
#endif

	range = RoadMapLineActive->Line[line].range;
	if (range == ROADMAP_LINE_NO_RANGE) return -1;
	return range;
}


int roadmap_line_get_street (int line) {
   
   return roadmap_range_get_street (roadmap_line_get_range (line));
}


void roadmap_line_points (int line, int *from, int *to) {

   VERIFY_THREAD("roadmap_line_points")

#ifdef DEBUG
   if (line < 0 || line >= RoadMapLineActive->LineCount) {
      roadmap_log (ROADMAP_FATAL, "illegal line index %d", line);
   }
#endif

   *from = RoadMapLineActive->Line[line].from & POINT_REAL_MASK;
   *to = RoadMapLineActive->Line[line].to & POINT_REAL_MASK;
}


void roadmap_line_from_point (int line, int *from) {

   VERIFY_THREAD("roadmap_line_from_point")

#ifdef DEBUG
   if (line < 0 || line >= RoadMapLineActive->LineCount) {
      roadmap_log (ROADMAP_FATAL, "illegal line index %d", line);
   }
#endif

   *from = RoadMapLineActive->Line[line].from & POINT_REAL_MASK;
}


void roadmap_line_to_point (int line, int *to) {

   VERIFY_THREAD("roadmap_line_to_point")

#ifdef DEBUG
   if (line < 0 || line >= RoadMapLineActive->LineCount) {
      roadmap_log (ROADMAP_FATAL, "illegal line index %d", line);
   }
#endif

   *to = RoadMapLineActive->Line[line].to & POINT_REAL_MASK;
}


void roadmap_line_point_ids (int line, int *id_from, int *id_to) {

   VERIFY_THREAD("roadmap_line_point_ids")

#ifdef DEBUG
   if (line < 0 || line >= RoadMapLineActive->LineCount) {
      roadmap_log (ROADMAP_FATAL, "illegal line index %d", line);
   }
#endif

   *id_from = roadmap_point_db_id (RoadMapLineActive->Line[line].from & POINT_REAL_MASK);
   *id_to = roadmap_point_db_id (RoadMapLineActive->Line[line].to & POINT_REAL_MASK);
}


int roadmap_line_cfcc (int line_id) {

   unsigned short *index;
   int cfcc;

   if (RoadMapLineActive == NULL) return 0; /* No line. */

   index = RoadMapLineActive->LineBySquare1[0].next;

	if (index[ROADMAP_ROAD_FIRST - 1] > line_id) return 0; // not a road
	
	for (cfcc = ROADMAP_ROAD_FIRST; cfcc <= ROADMAP_ROAD_LAST; cfcc++) {
		if (index[cfcc] > line_id) return cfcc;
	}
	
	return 0;
}


int roadmap_line_broken_range (int direction, int broken_to, int *first, int *last) {
	
	unsigned short *pos;
   if (RoadMapLineActive == NULL) return 0; /* No line. */
   
   pos = RoadMapLineActive->LineBySquare1->first_broken + (direction * 2 + broken_to); 
   *first = *pos++;
   *last = (*pos) - 1;
   
   return (*last) >= (*first);
}


int roadmap_line_get_broken (int broken_id) {

	return RoadMapLineActive->BrokenLine[broken_id];	
}


SegmentContext roadmap_line_context (int line_id) { 

	int i;
	
	for (i = 0; i < RoadMapLineActive->RoundaboutLineCount; i++) {
		if (line_id == RoadMapLineActive->RoundaboutLine[i]) return SEG_ROUNDABOUT;
	}
	return SEG_ROAD;
}


