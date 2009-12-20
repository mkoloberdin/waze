/* roadmap_shape.c - Manage the tiger shape points.
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
 *   int  roadmap_shape_in_square (int square, int *first, int *last);
 *   int  roadmap_shape_of_line   (int line, int begin, int end,
 *                                 int *first, int *last);
 *   void roadmap_shape_get_position (int shape, RoadMapPosition *position);
 *
 * These functions are used to retrieve the shape points that belong to a line.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define DECLARE_ROADMAP_SHAPE

#include "roadmap.h"
#include "roadmap_dbread.h"
#include "roadmap_tile_model.h"

#include "roadmap_line.h"
#include "roadmap_shape.h"
#include "roadmap_square.h"


static char *RoadMapShapeType = "RoadMapShapeContext";

RoadMapShapeContext *RoadMapShapeActive = NULL;

int shape_cache_square = -1;
int shape_cache_scale_factor = 1;

static void *roadmap_shape_map (const roadmap_db_data_file *file) {

   RoadMapShapeContext *context;

   context = malloc(sizeof(RoadMapShapeContext));
   roadmap_check_allocated(context);

   context->type = RoadMapShapeType;

   if (!roadmap_db_get_data (file,
   								  model__tile_shape_data,
   								  sizeof (RoadMapShape),
   								  (void**)&(context->Shape),
   								  &(context->ShapeCount))) {
      roadmap_log (ROADMAP_FATAL, "invalid shape/data structure");
   }

   return context;
}

static void roadmap_shape_activate (void *context) {

   RoadMapShapeContext *shape_context = (RoadMapShapeContext *) context;

   if (shape_context != NULL) {

      if (shape_context->type != RoadMapShapeType) {
         roadmap_log (ROADMAP_FATAL, "cannot activate shape (bad type)");
      }
   }

   RoadMapShapeActive = shape_context;
}

static void roadmap_shape_unmap (void *context) {

   RoadMapShapeContext *shape_context = (RoadMapShapeContext *) context;

   if (shape_context->type != RoadMapShapeType) {
      roadmap_log (ROADMAP_FATAL, "cannot activate shape (bad type)");
   }
   if (RoadMapShapeActive == shape_context) {
      RoadMapShapeActive = NULL;
   }
   free(shape_context);
}

roadmap_db_handler RoadMapShapeHandler = {
   "shape",
   roadmap_shape_map,
   roadmap_shape_activate,
   roadmap_shape_unmap
};


