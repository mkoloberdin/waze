/* roadmap_polygon.c - Manage the tiger polygons.
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
 *   int  roadmap_polygon_count (void);
 *   int  roadmap_polygon_category (int polygon);
 *   void roadmap_polygon_edges (int polygon, RoadMapArea *edges);
 *   int  roadmap_polygon_points (int polygon, int *list, int size);
 *
 * These functions are used to retrieve the polygons to draw.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "roadmap.h"
#include "roadmap_dbread.h"
#include "roadmap_tile_model.h"
#include "roadmap_dictionary.h"
#include "roadmap_db_polygon.h"
#include "roadmap_square.h"


static char *RoadMapPolygonType = "RoadMapPolygonContext";

typedef struct {

   char *type;

   RoadMapPolygon *Polygon;
   int             PolygonCount;

   RoadMapPolygonPoint *PolygonPoint;
   int                  PolygonPointCount;

   RoadMapDictionary DictionaryLandmark;
} RoadMapPolygonContext;

static RoadMapPolygonContext *RoadMapPolygonActive = NULL;


static void *roadmap_polygon_map (const roadmap_db_data_file *file) {

   RoadMapPolygonContext *context;

   context = malloc (sizeof(RoadMapPolygonContext));
   roadmap_check_allocated(context);

   context->type = RoadMapPolygonType;

   if (!roadmap_db_get_data (file,
   								  model__tile_polygon_head,
   								  sizeof (RoadMapPolygon),
   								  (void**)&(context->Polygon),
   								  &(context->PolygonCount))) {
      roadmap_log (ROADMAP_FATAL, "invalid polygon/head structure");
   }

   if (!roadmap_db_get_data (file,
   								  model__tile_polygon_point,
   								  sizeof (RoadMapPolygonPoint),
   								  (void**)&(context->PolygonPoint),
   								  &(context->PolygonPointCount))) {
      roadmap_log (ROADMAP_FATAL, "invalid polygon/point structure");
   }

   context->DictionaryLandmark = NULL;

   return context;
}

static void roadmap_polygon_activate (void *context) {

   RoadMapPolygonContext *polygon_context = (RoadMapPolygonContext *) context;

   if ((polygon_context != NULL) &&
       (polygon_context->type != RoadMapPolygonType)) {
      roadmap_log (ROADMAP_FATAL, "cannot activate (invalid context type)");
   }

   if (polygon_context != NULL &&
   	 polygon_context->DictionaryLandmark == NULL) {
      polygon_context->DictionaryLandmark = roadmap_dictionary_open ("landmark");
   }

   RoadMapPolygonActive = polygon_context;
}

static void roadmap_polygon_unmap (void *context) {

   RoadMapPolygonContext *polygon_context = (RoadMapPolygonContext *) context;

   if (polygon_context->type != RoadMapPolygonType) {
      roadmap_log (ROADMAP_FATAL, "cannot activate (invalid context type)");
   }
   if (RoadMapPolygonActive == polygon_context) {
      RoadMapPolygonActive = NULL;
   }
   free (polygon_context);
}

roadmap_db_handler RoadMapPolygonHandler = {
   "polygon",
   roadmap_polygon_map,
   roadmap_polygon_activate,
   roadmap_polygon_unmap
};



int  roadmap_polygon_count (void) {

   if (RoadMapPolygonActive == NULL) return 0;

   return RoadMapPolygonActive->PolygonCount;
}

int  roadmap_polygon_category (int polygon) {

   return RoadMapPolygonActive->Polygon[polygon].cfcc;
}


const char *roadmap_polygon_name (int polygon) {

   return roadmap_dictionary_get (RoadMapPolygonActive->DictionaryLandmark,
   				   RoadMapPolygonActive->Polygon[polygon].name);
}


void roadmap_polygon_edges (int polygon, RoadMapArea *edges) {

   RoadMapPolygon *this_polygon = RoadMapPolygonActive->Polygon + polygon;
   RoadMapPosition pos_square;
   int scale_factor;
   
   roadmap_square_min (roadmap_square_active (), &pos_square);
   scale_factor = roadmap_square_current_scale_factor ();

   edges->west = pos_square.longitude + this_polygon->west * scale_factor;
   edges->east = pos_square.longitude + this_polygon->east * scale_factor;
   edges->north = pos_square.latitude + this_polygon->north * scale_factor;
   edges->south = pos_square.latitude + this_polygon->south * scale_factor;
}


int  roadmap_polygon_points (int polygon, int *list, int size) {

   int i;
   RoadMapPolygon      *this_polygon;
   RoadMapPolygonPoint *this_point;

   this_polygon = RoadMapPolygonActive->Polygon + polygon;
   this_point   = RoadMapPolygonActive->PolygonPoint + this_polygon->first;

   if (size < this_polygon->count) {
      return -1; /* Not enough space. */
   }

   for (i = this_polygon->count; i > 0; --i, ++list, ++this_point) {
      *list = this_point->point;
   }

   return this_polygon->count;
}

