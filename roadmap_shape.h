/* roadmap_shape.h - Manage the tiger shape points.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
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

#ifndef _ROADMAP_SHAPE__H_
#define _ROADMAP_SHAPE__H_

#include "roadmap_types.h"
#include "roadmap_dbread.h"
#include "roadmap_db_shape.h"

typedef struct {

   char *type;

   RoadMapShape *Shape;
   int           ShapeCount;

} RoadMapShapeContext;

extern RoadMapShapeContext *RoadMapShapeActive;

extern int shape_cache_square;
extern int shape_cache_scale_factor;

#if defined(FORCE_INLINE) || defined(DECLARE_ROADMAP_SHAPE)
#if !defined(INLINE_DEC)
#define INLINE_DEC
#endif

INLINE_DEC void roadmap_shape_get_position (int shape, RoadMapPosition *position) {

   int square = roadmap_square_active();

   if (square != shape_cache_square) {
      shape_cache_square = square;
      shape_cache_scale_factor = roadmap_square_current_scale_factor ();
   }

   position->longitude += (int)RoadMapShapeActive->Shape[shape].delta_longitude * shape_cache_scale_factor;
   position->latitude  += (int)RoadMapShapeActive->Shape[shape].delta_latitude * shape_cache_scale_factor;
}


INLINE_DEC int roadmap_shape_get_count (int shape) {

   return RoadMapShapeActive->Shape[shape].delta_latitude;
}


INLINE_DEC int roadmap_shape_count (void) {

   if (RoadMapShapeActive == NULL) return 0; /* No line. */

   return RoadMapShapeActive->ShapeCount;
}
#endif // inline

void roadmap_shape_get_position (int shape, RoadMapPosition *position);
int roadmap_shape_get_count (int shape);
int roadmap_shape_count (void);

extern roadmap_db_handler RoadMapShapeHandler;

#endif // _ROADMAP_SHAPE__H_
