/* roadmap_line.h - Manage the tiger lines.
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
 */

#ifndef _ROADMAP_LINE__H_
#define _ROADMAP_LINE__H_

#include <assert.h>
#include "roadmap_types.h"
#include "roadmap_dbread.h"
#include "roadmap_db_line.h"
#include "roadmap_point.h"
#include "roadmap_shape.h"

#ifdef   WIN32_DEBUG
static DWORD s_dwThreadID = INVALID_THREAD_ID;
#define VERIFY_THREAD(_method_)                                                              \
   if( INVALID_THREAD_ID == s_dwThreadID)                                                    \
      s_dwThreadID = GetCurrentThreadId();                                                   \
   else                                                                                      \
   {                                                                                         \
      DWORD dwThisThread = GetCurrentThreadId();                                             \
                                                                                             \
      if( s_dwThreadID != dwThisThread)                                                      \
      {                                                                                      \
         roadmap_log(ROADMAP_FATAL,                                                          \
                     "roadmap_line.c::%s() - WRONG THREAD - Expected 0x%08X, Got 0x%08X",    \
                     _method_, s_dwThreadID, dwThisThread);                                  \
      }                                                                                      \
   }
   
#else
#define  VERIFY_THREAD(_method_)
   
#endif   // WIN32_DEBUG

typedef enum {
	SEG_ROAD,
	SEG_ROUNDABOUT
} SegmentContext;

typedef struct {

   char *type;

   RoadMapLine *Line;
   int          LineCount;

   RoadMapLineBySquare *LineBySquare1;
   int                  LineBySquare1Count;

	unsigned short		  *RoundaboutLine;
	int						RoundaboutLineCount;
	
	unsigned short		  *BrokenLine;
	int						BrokenLineCount;
	
} RoadMapLineContext;

extern RoadMapLineContext *RoadMapLineActive;

#if defined(FORCE_INLINE) || defined(DECLARE_ROADMAP_LINE)
#if !defined(INLINE_DEC)
#define INLINE_DEC
#endif

INLINE_DEC void roadmap_line_from (int line, RoadMapPosition *position) {

   VERIFY_THREAD("roadmap_line_from")

#ifdef DEBUG
   if (line < 0 || line >= RoadMapLineActive->LineCount) {
      roadmap_log (ROADMAP_FATAL, "illegal line index %d", line);
   }
#endif
   roadmap_point_position (RoadMapLineActive->Line[line].from & POINT_REAL_MASK, position);
}


INLINE_DEC void roadmap_line_to   (int line, RoadMapPosition *position) {

   VERIFY_THREAD("roadmap_line_to")

#ifdef DEBUG
   if (line < 0 || line >= RoadMapLineActive->LineCount) {
      roadmap_log (ROADMAP_FATAL, "illegal line index %d", line);
   }
#endif
   roadmap_point_position (RoadMapLineActive->Line[line].to & POINT_REAL_MASK, position);
}


INLINE_DEC int roadmap_line_shapes (int line, int *first_shape, int *last_shape) {

   int shape;
   int count;

#ifndef J2ME
   VERIFY_THREAD("roadmap_line_shapes")

#ifdef DEBUG
   if (line < 0 || line >= RoadMapLineActive->LineCount) {
      roadmap_log (ROADMAP_FATAL, "illegal line index %d", line);
   }
#endif
#endif

   *first_shape = *last_shape = -1;

   shape = RoadMapLineActive->Line[line].first_shape;
   if (shape == ROADMAP_LINE_NO_SHAPES) return -1;

   /* the first shape is actually the count */
   count = roadmap_shape_get_count (shape);

   *first_shape = shape + 1;
   *last_shape = *first_shape + count - 1;

   return count;
}


INLINE_DEC int roadmap_line_in_square (int square, int cfcc, int *first, int *last) {

   assert (cfcc > 0 && cfcc <= ROADMAP_CATEGORY_RANGE);

   if (square < 0) {
      return 0;   /* This square is empty. */
   }

   roadmap_square_set_current(square);

   if (RoadMapLineActive == NULL) return 0; /* No line. */

	*first = RoadMapLineActive->LineBySquare1->next[cfcc - 1];
	*last = RoadMapLineActive->LineBySquare1->next[cfcc] - 1;

   return (*last) >= (*first);
}

#endif // inline


int  roadmap_line_in_square (int square, int cfcc, int *first, int *last);

int roadmap_line_shapes (int line, int *first_shape, int *last_shape);
int roadmap_line_get_range (int line);
int roadmap_line_get_street (int line);

void roadmap_line_from (int line, RoadMapPosition *position);
void roadmap_line_to   (int line, RoadMapPosition *position);

int  roadmap_line_from_is_fake (int line);
int  roadmap_line_to_is_fake (int line);

void roadmap_line_points     (int line, int *from, int *to);
void roadmap_line_from_point (int line, int *from);
void roadmap_line_to_point   (int line, int *from);
void roadmap_line_point_ids  (int line, int *id_from, int *id_to);

int roadmap_line_length (int line);

int  roadmap_line_count (void);

int roadmap_line_cfcc (int line_id);
int roadmap_line_broken_range (int direction, int broken_to, int *first, int *last);
int roadmap_line_get_broken (int broken_id);

SegmentContext roadmap_line_context (int line_id);

extern roadmap_db_handler RoadMapLineHandler;

#endif // _ROADMAP_LINE__H_
