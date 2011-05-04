/* roadmap_label.c - Manage map labels.
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
 *   Label cache 2006, Paul Fox
 *
 *   This code was mostly taken from UMN Mapserver
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
 * TODO:
 *   - As the cache holds labels which are not drawn, we need a way to clean
 *     the cache by throwing out old entries. The age mechanism will keep
 *     the cache clean, but that may not be enough if we get many labels.
 *
 *   - New labels consume space from the label cache. It would be better if
 *     we could scan the old labels list when a label is added and just
 *     update its age. This will save us labels cache entries. It may
 *     require the usage of a hash table for doing the scan efficiently.
 *
 *   - Currently we keep in the cache labels and lines whose labels
 *     were not drawn, just in case they can be drawn next time.  These
 *     also consume space, though it's not wasted.  If we've hit the
 *     "cache full" condition, we should consider splicing the "undrawn"
 *     list onto the "spares" list rather than onto the cache, to free
 *     up cache entries.
 *
 * SYNOPSYS:
 *
 *   See roadmap_label.h.
 */

#include <stdlib.h>
#include <string.h>

#include "roadmap_config.h"
#include "roadmap_math.h"
#include "roadmap_plugin.h"
#include "roadmap_canvas.h"
#include "roadmap_canvas3d.h" // roadmap_canvas3_project()
#ifdef OGL_TILE
#include "roadmap_canvas_tile.h"
#endif

#ifdef ROADMAP_USE_LINEFONT
#include "roadmap_linefont.h"
#endif

#include "roadmap_skin.h"
#include "roadmap_layer.h"
#include "roadmap_list.h"
#include "roadmap_line.h"
#include "roadmap_label.h"
#include "roadmap_square.h"
#include "roadmap_tile.h"

#define CHECK_DIAGONAL_INTERSECTION 1
#define POLY_OUTLINE 0
#define LABEL_USING_LINEID 0
#define LABEL_USING_NODEID 0
#define LABEL_USING_TILEID 0

#define MIN(a, b) (a) < (b) ? (a) : (b)
#define MAX(a, b) (a) > (b) ? (a) : (b)

static RoadMapConfigDescriptor RoadMapConfigMinFeatureSize =
                        ROADMAP_CONFIG_ITEM("Labels", "MinFeatureSize");

RoadMapConfigDescriptor RoadMapConfigLabelsColor =
                        ROADMAP_CONFIG_ITEM("Labels", "Color");

RoadMapConfigDescriptor RoadMapConfigLabelsBgColor =
                        ROADMAP_CONFIG_ITEM("Labels", "BgColor");

/* this is fairly arbitrary */
#ifdef J2ME
#define MAX_LABELS 1024
#else
#define MAX_LABELS 2048
#endif
#define ROADMAP_LABEL_STREETLABEL_SIZE 14

#define MAX_NUM_TILES 20

//#define LABEL_FLAG_NOTEXT     0x01
#define LABEL_FLAG_PLACE      0x02
#define LABEL_FLAG_MULTI_ROW  0x04
#define LABEL_FLAG_DRAWN      0x08
#define LABEL_FLAG_NOT_CACHED 0x10

typedef struct {
   RoadMapListItem link;

   int featuresize_sq;

   PluginLine line;
   PluginStreet street;
   char *text;
   char *text2;

   RoadMapGuiPoint text_point; /* label point */
   RoadMapGuiPoint text2_point; /* label point (2nd line)*/
   RoadMapGuiPoint center_point; /* label point */
   RoadMapGuiRect bbox; /* label bounding box */
   RoadMapGuiPoint poly[4];

   RoadMapPosition center_pos;

   zoom_t zoom;
   short angle;  /* degrees */
   int gen;    /* combination "drawn" flag and generation marker */

   unsigned char flags;
   int           opacity;
   
#ifdef OGL_TILE
   int            tile_ids[MAX_NUM_TILES];
   int            num_tiles;
#endif

} roadmap_label;

static RoadMapList RoadMapLabelCache;
static RoadMapList RoadMapLabelSpares;
static RoadMapList RoadMapLabelNew;

static int RoadMapLabelCacheFull = 0;
static int RoadMapLabelCacheAlloced = 0;

static int RoadMapLabelGeneration = 1;
static int RoadMapLabelLastGeneration = 0;
#ifndef OGL_TILE
static int RoadMapLabelGenerationMaxAge = 36;
#else
static int RoadMapLabelGenerationMaxAge = 0;
#endif //OGL_TILE
static zoom_t RoadMapLabelLastZoom;
static int RoadMapLabelLastOrient;
static RoadMapPosition RoadMapLabelLastCenter;

static RoadMapPen RoadMapLabelPen;

static int RoadMapLabelMinFeatSizeSq;

static int MaxPlaceLabel;

static int rect_overlap (RoadMapGuiRect *a, RoadMapGuiRect *b) {
   int space_x = 0;
   int space_y = 0;

   if (roadmap_screen_get_view_mode() != VIEW_MODE_2D) {
      space_x = 20;
      space_y = 30;
   }

   if(a->minx > b->maxx + space_x) return 0;
   if(a->maxx < b->minx - space_x) return 0;
   if(a->miny > b->maxy + space_y) return 0;
   if(a->maxy < b->miny - space_y) return 0;

   return 1;
}

static int point_in_bbox( RoadMapGuiPoint *p, RoadMapGuiRect *bb) {

   if ((p->x <= bb->minx) || (p->x >= bb->maxx) ||
       (p->y >= bb->maxy) || (p->y <= bb->miny))
      return 0;

   return 1;
}

/* doesn't check for one completely inside the other -- just intersection */
static int poly_overlap (roadmap_label *c1, roadmap_label *c2) {

   RoadMapGuiPoint *a = c1->poly;
   RoadMapGuiPoint *b = c2->poly;
   RoadMapGuiPoint isect;
   int ai, bi;

//SRUL: Better to check intersection of diagonals first (?)
#if CHECK_DIAGONAL_INTERSECTION
   for (ai = 0; ai < 2; ai++) {
      for (bi = 0; bi < 2; bi++) {
         if (roadmap_math_screen_intersect( &a[ai], &a[ai+2],
                                &b[bi], &b[bi+2], &isect)) {
            if (point_in_bbox(&isect, &c1->bbox) &&
                point_in_bbox(&isect, &c2->bbox)) {
               return 1;
            }
         }
      }
   }
#endif
   for (ai = 0; ai < 4; ai++) {
      for (bi = 0; bi < 4; bi++) {
         if (roadmap_math_screen_intersect( &a[ai], &a[(ai+1)%4],
                                &b[bi], &b[(bi+1)%4], &isect)) {
            if (point_in_bbox(&isect, &c1->bbox) &&
                point_in_bbox(&isect, &c2->bbox)) {
               return 1;
            }
         }
      }
   }

   return 0;
}


static void compute_bbox(RoadMapGuiPoint *poly, RoadMapGuiRect *bbox) {

   int i;

   bbox->minx = bbox->maxx = poly[0].x;
   bbox->miny = bbox->maxy = poly[0].y;

   for(i=1; i<4; i++) {
      bbox->minx = MIN(bbox->minx, poly[i].x);
      bbox->maxx = MAX(bbox->maxx, poly[i].x);
      bbox->miny = MIN(bbox->miny, poly[i].y);
      bbox->maxy = MAX(bbox->maxy, poly[i].y);
   }
}


static RoadMapGuiPoint get_metrics(roadmap_label *c,
                                RoadMapGuiRect *rect, int centered_y) {
   RoadMapGuiPoint q;
   int x1=0, y1=0;
   RoadMapGuiPoint *poly = c->poly;
   int buffer = 1;
   int w, h;
   int angle = c->angle;
   RoadMapGuiPoint *p = &c->center_point;

   w = rect->maxx - rect->minx;
   h = rect->maxy - rect->miny;

   /* position CC */
   x1 = -(w/2);

   y1 = centered_y ? (h/2) : 0;

   q.x = x1 - rect->minx;
   q.y = rect->miny - y1;

   roadmap_math_rotate_point (&q, p, angle);

   poly[0].x = x1 - buffer; /* ll */
   poly[0].y = -(y1 + buffer);
   roadmap_math_rotate_point (&poly[0], p, angle);

   poly[1].x = x1 - buffer; /* ul */
   poly[1].y = -(y1 -h - buffer);
   roadmap_math_rotate_point (&poly[1], p, angle);

   poly[2].x = x1 + w + buffer; /* ur */
   poly[2].y = -(y1 - h - buffer);
   roadmap_math_rotate_point (&poly[2], p, angle);

   poly[3].x = x1 + w + buffer; /* lr */
   poly[3].y = -(y1 + buffer);
   roadmap_math_rotate_point (&poly[3], p, angle);

   compute_bbox(poly, &c->bbox);

   return q;
}

static int normalize_angle(int angle) {

   angle += roadmap_math_get_orientation ();

   while (angle > 360) angle -= 360;
   while (angle < 0) angle += 360;
   if (angle >= 180) angle -= 180;

   angle -= 90;

   return angle;
}

static int get_label_size(roadmap_label *cPtr){

   int size = ROADMAP_LABEL_STREETLABEL_SIZE;
   if ( !(cPtr->flags & LABEL_FLAG_PLACE)) {
      if ((cPtr->line.cfcc < ROADMAP_ROAD_RAMP))
         size += 1;
      else if ((cPtr->line.cfcc > ROADMAP_ROAD_EXIT))
         size -= 2;
   }
   else{
      if ((cPtr->line.cfcc == ROADMAP_AREA_CITY))
         size += 2;
      else if ((cPtr->line.cfcc >= ROADMAP_AREA_FIRST) && (cPtr->line.cfcc <= ROADMAP_WATER_LAST))
         size -= 3;
    }
    return size;
}

void roadmap_label_text_extents( const char *text, int size,
        int *width, int *ascent, int *descent,
        int *can_tilt, int *easy_reading)
{
#ifdef ROADMAP_USE_LINEFONT

   roadmap_linefont_extents
        (text, size, width, ascent, descent);

   /* The linefont font isn't pretty.  Reading it is hard with
    * a road running through it, so we don't center labels on
    * the road.
    */
   *easy_reading = 0;
   if (can_tilt) *can_tilt = 1;

#else

   roadmap_canvas_get_text_extents
      (text, size, width, ascent, descent, can_tilt);

   *easy_reading = 1;

#endif
}

static void roadmap_label_draw_text(const char *text,
        RoadMapGuiPoint *start, RoadMapGuiPoint *center,
        int doing_angles, int angle, int size, int font_type, int opacity)
{
#ifdef OGL_TILE
   if (opacity && opacity != 255) {
      roadmap_canvas_set_global_opacity (opacity);
   }
#endif
   
#ifdef ROADMAP_USE_LINEFONT
   roadmap_linefont_text
        (text, doing_angles ? ROADMAP_CANVAS_CENTER_BOTTOM :
                    ROADMAP_CANVAS_CENTER, center, size, angle);
#else
   if (doing_angles) {
      roadmap_canvas_draw_formated_string_angle (start, center, angle, size, font_type, text);
   } else {
      roadmap_canvas_draw_formated_string_size(start, ROADMAP_CANVAS_CENTERLEFT, size, font_type, text);
   }
#endif

#ifdef OGL_TILE
   if (opacity && opacity != 255) {
      roadmap_canvas_set_global_opacity (255);
   }
#endif
}

void roadmap_label_update_pos (void) {
   RoadMapListItem *item, *tmp;
   roadmap_label *cPtr = 0;
   RoadMapGuiPoint p0;
   int angle_dx = roadmap_math_get_orientation () - RoadMapLabelLastOrient;
   int dx, dy;

   ROADMAP_LIST_FOR_EACH
        (&RoadMapLabelCache,
             item, tmp) {

      cPtr = (roadmap_label *)item;

      roadmap_math_coordinate (&cPtr->center_pos, &p0);
      roadmap_math_rotate_project_coordinate (&p0);
      dx = p0.x - cPtr->center_point.x;
      dy = p0.y - cPtr->center_point.y;

      cPtr->center_point = p0;

      if (angle_dx ||
          cPtr->bbox.minx > cPtr->bbox.maxx) {
         cPtr->bbox.minx = 1;
         cPtr->bbox.maxx = -1;
         if (!(cPtr->flags & LABEL_FLAG_PLACE)) {
            cPtr->angle += angle_dx;
            if (cPtr->angle < -90) cPtr->angle += 360;
            if (cPtr->angle > 90) cPtr->angle -= 180;
            if (cPtr->angle > 90) cPtr->angle -= 180;
         }
      } else {

         if (dx != 0 || dy != 0) {
            int i;

            for (i = 0; i < 4; i++) {
               cPtr->poly[i].x += dx;
               cPtr->poly[i].y += dy;
            }
            cPtr->bbox.minx += dx;
            cPtr->bbox.maxx += dx;
            cPtr->bbox.miny += dy;
            cPtr->bbox.maxy += dy;

            cPtr->text_point.x += dx;
            cPtr->text_point.y += dy;

            if (cPtr->text2) {
               cPtr->text2_point.x += dx;
               cPtr->text2_point.y += dy;
            }
         }
      }
   }
}

/* called when a screen repaint is complete.  keeping track of
 * label "generations" involves keeping track of full refreshes,
 * not individual calls to roadmap_label_draw_cache().
 */
void roadmap_label_start (void) {
   
   /* Cheap cache invalidation:  if the previous center of screen
    * is still visible, assume that the cache is still more useful
    * than not.
    */
   if ( !roadmap_math_point_is_visible (&RoadMapLabelLastCenter) ) {
      ROADMAP_LIST_SPLICE (&RoadMapLabelSpares, &RoadMapLabelCache);
   }
   
   roadmap_math_get_context (&RoadMapLabelLastCenter, &RoadMapLabelLastZoom);
   
   MaxPlaceLabel = roadmap_canvas_height() / 75;
   
   RoadMapLabelGeneration++;
   RoadMapLabelLastOrient = roadmap_math_get_orientation ();
}

int roadmap_label_add (const RoadMapGuiPoint *point, int angle,
                       int featuresize_sq, const PluginLine *line) {

   roadmap_label *cPtr = 0;
   PluginStreetProperties properties;

   if (featuresize_sq < RoadMapLabelMinFeatSizeSq) {
      return -1;
   }
   
   roadmap_plugin_get_street_properties
      (line, &properties, PLUGIN_STREET_ONLY);
   
   if (!properties.street || !*properties.street) {
      return -1;
   }

   if (!ROADMAP_LIST_EMPTY(&RoadMapLabelSpares)) {
      cPtr = (roadmap_label *)roadmap_list_remove
                               (ROADMAP_LIST_FIRST(&RoadMapLabelSpares));
      if (cPtr->text && *cPtr->text)
      {
		 free( cPtr->text );
		 cPtr->text = NULL;
      }
      else
      {
    	  if ( cPtr->text == NULL )
    	  {
    		  roadmap_log( ROADMAP_WARNING, "Trying to free the deallocated memory!" );
    	  }
      }
      if (cPtr->text2 && *cPtr->text2)
      {
       free( cPtr->text2 );
       cPtr->text2 = NULL;
      }
   } else {
      if (RoadMapLabelCacheFull) return -1;

      if (RoadMapLabelCacheAlloced == MAX_LABELS) {
         roadmap_log (ROADMAP_WARNING, "Too many streets to label them all.");
         RoadMapLabelCacheFull = 1;
         return -1;
      }
      cPtr = malloc (sizeof (*cPtr));
      roadmap_check_allocated (cPtr);
      RoadMapLabelCacheAlloced++;
   }

   cPtr->flags = 0;
#ifndef OGL_TILE
   cPtr->opacity = 255;
#else
   cPtr->opacity = 0;
   cPtr->tile_ids[0] = roadmap_canvas_tile_get_id();
   cPtr->num_tiles = 1;
#endif
   
   cPtr->bbox.minx = 1;
   cPtr->bbox.maxx = -1;

   cPtr->line = *line;
   cPtr->featuresize_sq = featuresize_sq;
   
   cPtr->center_point = *point;
   
   /* The stored point is not screen oriented, rotate is needed */
#ifndef OGL_TILE
   cPtr->angle = normalize_angle(angle);
   roadmap_math_rotate_project_coordinate (&cPtr->center_point);
   
   roadmap_math_to_position (&cPtr->center_point, &cPtr->center_pos, 1);
#else
   cPtr->angle = angle;
   roadmap_math_rotate_coordinates (1, &cPtr->center_point);
   
   roadmap_math_to_position (&cPtr->center_point, &cPtr->center_pos, 0);
   
   roadmap_math_project(&cPtr->center_point);
#endif 

   /* the "generation" of a label is refreshed when it is re-added.
    * later, labels cache entries can be aged, and discarded.
    */
   cPtr->gen = RoadMapLabelGeneration;

   cPtr->zoom = RoadMapLabelLastZoom;

 {
#if LABEL_USING_LINEID
      char buf[40];
      sprintf(buf, "%d", cPtr->line.line_id);
      cPtr->text = strdup(buf);
#elif LABEL_USING_TILEID
      char buf[40];
      sprintf(buf, "%d:%d", cPtr->line.square, cPtr->line.line_id);
      cPtr->text = strdup(buf);
#elif LABEL_USING_NODEID
		if (cPtr->line.plugin_id == ROADMAP_PLUGIN_ID) {
			char buf[100];
			int from;
			int to;
			roadmap_square_set_current (cPtr->line.square);
			roadmap_line_point_ids (cPtr->line.line_id, &from, &to);
			sprintf (buf, "%d - %d", from, to);
			cPtr->text = strdup(buf);
		} else cPtr->text = strdup (properties.street);
#else
      cPtr->text = strdup(properties.street);
#endif
   }
   cPtr->text2 = NULL;
   cPtr->street = properties.plugin_street;

   roadmap_list_append(&RoadMapLabelNew, &cPtr->link);

   return 0;
}


int roadmap_label_add_place (const RoadMapGuiPoint *point, int angle,
                             int featuresize_sq, const char *name, int category,
                             BOOL multi_row) {

   roadmap_label *cPtr = 0;

   if (featuresize_sq < RoadMapLabelMinFeatSizeSq) {
      return -1;
   }
   
   if (!name || !*name) {
      return -1;
   }

   if (!ROADMAP_LIST_EMPTY(&RoadMapLabelSpares)) {
      cPtr = (roadmap_label *)roadmap_list_remove
                               (ROADMAP_LIST_FIRST(&RoadMapLabelSpares));
      if (cPtr->text && *cPtr->text)
      {
    		 free( cPtr->text );
    		 cPtr->text = NULL;
      }
      else
      {
    	  if ( cPtr->text == NULL )
    	  {
    		  roadmap_log( ROADMAP_WARNING, "Trying to free the deallocated memory!" );
    	  }
      }
      if (cPtr->text2 && *cPtr->text2)
      {
          free( cPtr->text2 );
          cPtr->text2 = NULL;
      }
   } else {
      if (RoadMapLabelCacheFull) return -1;

      if (RoadMapLabelCacheAlloced == MAX_LABELS) {
         roadmap_log (ROADMAP_WARNING, "Too many streets to label them all.");
         RoadMapLabelCacheFull = 1;
         return -1;
      }
      cPtr = malloc (sizeof (*cPtr));
      roadmap_check_allocated (cPtr);
      RoadMapLabelCacheAlloced++;
   }

   cPtr->flags = LABEL_FLAG_PLACE;
   if (multi_row)
      cPtr->flags |= LABEL_FLAG_MULTI_ROW;
   
#ifndef OGL_TILE
   cPtr->opacity = 255;
#else
   cPtr->opacity = 0;
   cPtr->tile_ids[0] = roadmap_canvas_tile_get_id();
   cPtr->num_tiles = 1;
#endif

   cPtr->text = strdup(name);

   cPtr->text2 = NULL;
   cPtr->bbox.minx = 1;
   cPtr->bbox.maxx = -1;
   cPtr->line.cfcc = category;
   cPtr->featuresize_sq = featuresize_sq;
   cPtr->angle = normalize_angle(angle);

   cPtr->center_point = *point;
   
   /* The stored point is not screen oriented, rotate is needed */
#ifndef OGL_TILE
   roadmap_math_rotate_project_coordinate (&cPtr->center_point);
   
   roadmap_math_to_position (&cPtr->center_point, &cPtr->center_pos, 1);
#else
   roadmap_math_rotate_coordinates (1, &cPtr->center_point);
   
   roadmap_math_to_position (&cPtr->center_point, &cPtr->center_pos, 0);
   
   roadmap_math_project(&cPtr->center_point);
#endif

   /* the "generation" of a label is refreshed when it is re-added.
    * later, labels cache entries can be aged, and discarded.
    */
   cPtr->gen = RoadMapLabelGeneration;

   cPtr->zoom = RoadMapLabelLastZoom;

   roadmap_list_append(&RoadMapLabelNew, &cPtr->link);

   return 0;
}

int roadmap_label_draw_cache (int angles, int full) {

   RoadMapPosition current_center;
   RoadMapListItem *item, *tmp;
   RoadMapListItem *item2, *tmp2;
   RoadMapListItem *end;
   RoadMapList undrawn_labels;
   int width, width2, ascent, descent;
   zoom_t current_zoom;
   int current_orient;
   RoadMapGuiRect r;
   RoadMapGuiPoint midpt;
   short aang;
   roadmap_label *cPtr, *ocPtr, *ncPtr;
   int whichlist;
#define OLDLIST 0
#define NEWLIST 1
   int label_center_y;
   int cannot_label;
   int place_label_count = 0;
   RoadMapPen currentLabelPen;
   int font_type;
   BOOL changed_zoom = FALSE;
   static uint32_t last_draw_time = 0;
   int draw = 1;
   int full_label_draw = 1;
   
   MaxPlaceLabel = roadmap_canvas_height() / 75;
   
   if (roadmap_time_get_millis() - 500 < last_draw_time && !full) {
      draw = 0;
   } else {
      last_draw_time = roadmap_time_get_millis();
   }

   //printf("=> spare: %d, cache: %d, new: %d\n", roadmap_list_count(&RoadMapLabelSpares), roadmap_list_count(&RoadMapLabelCache), roadmap_list_count(&RoadMapLabelNew));
   
//printf(">> draw cache <<\n");
   ROADMAP_LIST_INIT(&undrawn_labels);
   roadmap_canvas_select_pen (RoadMapLabelPen);

   roadmap_math_get_context (&current_center, &current_zoom);
   current_orient = roadmap_math_get_orientation();

   if ((current_orient != RoadMapLabelLastOrient) ||
       (current_zoom != RoadMapLabelLastZoom) ||
       (memcmp(&current_center, &RoadMapLabelLastCenter,
            sizeof(RoadMapPosition)) != 0)) {
      
      roadmap_label_update_pos ();
      RoadMapLabelLastCenter = current_center;
      RoadMapLabelLastOrient = current_orient;
      if (RoadMapLabelLastZoom != current_zoom) {
         changed_zoom = TRUE;
         RoadMapLabelLastZoom = current_zoom;
      }
   }


   /* We want to process the cache first, in order to render previously
    * rendered labels again.  Only after doing so (checking for updates
    * in the new list as we go), we'll process what's left of the new
    * list -- those are the truly new labels for this repaint.
    */
   for (whichlist = OLDLIST; whichlist <= NEWLIST; whichlist++)  {

      ROADMAP_LIST_FOR_EACH
           (whichlist == OLDLIST ? &RoadMapLabelCache : &RoadMapLabelNew,
                item, tmp) {
              BOOL remove_old_label = FALSE;
              int tile_cached = 1;
         cPtr = (roadmap_label *)item;

#ifdef OGL_TILE
              if (whichlist == NEWLIST) {
                 RoadMapGuiPoint center;
                 center.x = roadmap_canvas_width()/2;
                 center.y = roadmap_canvas_height()/2;
                 //roadmap_math_rotate_point(&cPtr->center_point, &center, RoadMapLabelLastOrient);
                 roadmap_math_coordinate(&cPtr->center_pos, &cPtr->center_point);
                 roadmap_math_rotate_project_coordinate (&cPtr->center_point);
                 if (!(cPtr->flags & LABEL_FLAG_PLACE))
                    cPtr->angle = normalize_angle(cPtr->angle);
              }
              
              if (draw && (cPtr->flags & LABEL_FLAG_NOT_CACHED)) {
                 roadmap_list_insert
                                        (&RoadMapLabelSpares, roadmap_list_remove(&cPtr->link));
                                        continue;
              }
              
              if (!changed_zoom && tile_cached) {
                 cPtr->gen = RoadMapLabelGeneration;
              }
#endif
              
         if ((RoadMapLabelGeneration - cPtr->gen) >
               RoadMapLabelGenerationMaxAge) {
            roadmap_list_insert
               (&RoadMapLabelSpares, roadmap_list_remove(&cPtr->link));
            continue;
         }

         /* If still working through previously rendered labels,
          * check for updates
          */
         if (whichlist == OLDLIST) {
            ROADMAP_LIST_FOR_EACH (&RoadMapLabelNew, item2, tmp2) {
               int exists = 0;

               ncPtr = (roadmap_label *)item2;
               
#ifndef OGL_TILE
               if (ncPtr->gen == 0) {
                   continue;
               }
#endif

               if (!(cPtr->flags & LABEL_FLAG_PLACE) &&
                    !(ncPtr->flags & LABEL_FLAG_PLACE) &&
                    roadmap_plugin_same_line (&cPtr->line, &ncPtr->line)) {
                  exists = 1;
               } else if ((cPtr->flags & LABEL_FLAG_PLACE) &&
                          (ncPtr->flags & LABEL_FLAG_PLACE) &&
                    !strncmp(cPtr->text, ncPtr->text, strlen(cPtr->text))) {

                  if (!cPtr->text2) exists = 1;
                  else {
                     /* old test is already split so need smarter cmp */
                     if (((strlen(cPtr->text) + 1 + strlen(cPtr->text2)) ==
                           strlen(ncPtr->text)) &&
                           !strcmp(cPtr->text2, ncPtr->text + strlen(cPtr->text) + 1)) {
                        exists = 1;
                     }
                  }
               }

               if (exists) {
                   /* Found a new version of this existing line */
                   int dx, dy;
#ifdef OGL_TILE
                  while (cPtr->num_tiles < MAX_NUM_TILES && ncPtr->num_tiles > 0) {
                     cPtr->tile_ids[cPtr->num_tiles++]= ncPtr->tile_ids[--ncPtr->num_tiles];
                  }
                  
                  roadmap_list_insert
                     (&RoadMapLabelSpares,
                   roadmap_list_remove(&ncPtr->link));
                  break;//TODO: handle this case instead of removing item
#endif
                   dx = ncPtr->center_point.x - cPtr->center_point.x;
                   dy = ncPtr->center_point.y - cPtr->center_point.y;

                   if ((cPtr->angle != ncPtr->angle) ||
                         (cPtr->zoom != ncPtr->zoom)) {
                       cPtr->bbox.minx = 1;
                       cPtr->bbox.maxx = -1;
                       cPtr->angle = ncPtr->angle;
                       cPtr->zoom = ncPtr->zoom;
                   } else {
                       /* Angle is unchanged -- simple movement only */

                       if ((abs(dx) > 1) || (abs(dy) > 1)) {
                          int i;

                          for (i = 0; i < 4; i++) {
                             cPtr->poly[i].x += dx;
                             cPtr->poly[i].y += dy;
                          }
                          cPtr->bbox.minx += dx;
                          cPtr->bbox.maxx += dx;
                          cPtr->bbox.miny += dy;
                          cPtr->bbox.maxy += dy;

                          cPtr->text_point.x += dx;
                          cPtr->text_point.y += dy;

                          if (cPtr->text2) {
                             cPtr->text2_point.x += dx;
                             cPtr->text2_point.y += dy;
                          }
                       }
                   }

                   if ((abs(dx) > 1) || (abs(dy) > 1)) {
                          cPtr->center_point = ncPtr->center_point;
                          cPtr->center_pos = ncPtr->center_pos;
                       }

                   cPtr->featuresize_sq = ncPtr->featuresize_sq;
                   cPtr->gen = ncPtr->gen;
                  cPtr->opacity = 0;

                   roadmap_list_insert
                      (&RoadMapLabelSpares, roadmap_list_remove(&ncPtr->link));
                   break;
               }
            }
            if (remove_old_label){
               roadmap_list_append
                  (&undrawn_labels, roadmap_list_remove(&cPtr->link));
               continue;
            }
         }              

         if (cPtr->gen != RoadMapLabelGeneration) {
            cPtr->opacity = 0;
            roadmap_list_append
               (&undrawn_labels, roadmap_list_remove(&cPtr->link));
            continue;
         }

         if (cPtr->bbox.minx > cPtr->bbox.maxx) {
            int can_tilt;
            roadmap_label_text_extents
                    (cPtr->text, get_label_size(cPtr),
                    &width, &ascent, &descent, &can_tilt, &label_center_y);
            angles = angles && can_tilt;

            /* text is too long for this feature */
            /* (4 times longer than feature) */
            if ((width * width / 16) > cPtr->featuresize_sq &&
                  !(cPtr->flags & LABEL_FLAG_MULTI_ROW)) {
               /* Keep this one in the cache as the feature size may change
                * in the next run.  Keeping it is cheaper than looking it
                * up again.
                */
               cPtr->opacity = 0;
               roadmap_list_append
                  (&undrawn_labels, roadmap_list_remove(&cPtr->link));
               continue;
            }

            if (!cPtr->text2 && cPtr->flags & LABEL_FLAG_MULTI_ROW &&
                  strlen(cPtr->text) > 10) {
               char *pos, *pos2;
               char *mid = cPtr->text + (strlen(cPtr->text)/2);

               pos = strchr(cPtr->text, ' ');
               pos2 = strchr(mid, ' ');

               if (pos) {
                  if (pos2 &&
                     (pos != pos2) &&
                     (pos2 - mid < mid - pos))
                     pos = pos2;

                  pos[0] = 0;
                  cPtr->text2 = strdup (pos + 1);

                  roadmap_label_text_extents
                                      (cPtr->text, get_label_size(cPtr) ,
                                      &width, &ascent, &descent, &can_tilt, &label_center_y);

                  roadmap_label_text_extents
                                      (cPtr->text2, get_label_size(cPtr) ,
                                      &width2, &ascent, &descent, &can_tilt, &label_center_y);
               }
            } else if (cPtr->text2) {
               roadmap_label_text_extents
                                (cPtr->text2, get_label_size(cPtr) ,
                                   &width2, &ascent, &descent, &can_tilt, &label_center_y);
            }


            r.minx = 0;
            r.maxx = width;
            r.miny = 0;
            r.maxy = ascent + descent;

            if (cPtr->text2) {
               r.maxy = r.maxy*2 + 2;
               if (width2 > width)
                  r.maxx = width2;
            }

            if (angles) {

               cPtr->text_point = get_metrics (cPtr, &r, label_center_y);

               if (cPtr->text2) {
                  int row_height = (cPtr->bbox.maxy - cPtr->bbox.miny)/2;
                  cPtr->text2_point = cPtr->text_point;
                  if (width2 > width)
                     cPtr->text_point.x += (width2 - width) /2;
                  else
                     cPtr->text2_point.x += (width - width2) /2;

                  cPtr->text_point.y -= row_height;
               }

            } else {
               /* Text will be horizontal, so bypass a lot of math.
                * (and compensate for eventual centering of text.)
                */
               cPtr->text_point.x = cPtr->center_point.x - (r.maxx - r.minx)/2;
               cPtr->text_point.y = cPtr->center_point.y;

               cPtr->bbox.minx =
					cPtr->text_point.x;// - (r.maxx - r.minx)/2;
               cPtr->bbox.maxx =
					cPtr->text_point.x + (r.maxx - r.minx);
               cPtr->bbox.miny =
					cPtr->text_point.y - (r.maxy - r.miny)/2 - 5;
               cPtr->bbox.maxy =
					cPtr->text_point.y + (r.maxy - r.miny)/2 + 5;

               if (cPtr->text2) {
                  int row_height = ascent + descent;
                  cPtr->text2_point = cPtr->text_point;
                  if (width2 > width)
                     cPtr->text_point.x += (width2 - width) /2;
                  else
                     cPtr->text2_point.x += (width - width2) /2;

                  cPtr->text_point.y -= row_height;
               }
#if POLY_OUTLINE
				cPtr->poly[0].x= cPtr->poly[1].x= cPtr->bbox.minx;
				cPtr->poly[2].x= cPtr->poly[3].x= cPtr->bbox.maxx;
				cPtr->poly[0].y= cPtr->poly[3].y= cPtr->bbox.miny;
				cPtr->poly[1].y= cPtr->poly[2].y= cPtr->bbox.maxy;
#endif// POLY_OUTLINE
            }
         }

         /* Bounding box midpoint */
         midpt.x = ( cPtr->bbox.maxx + cPtr->bbox.minx) / 2;
         midpt.y = ( cPtr->bbox.maxy + cPtr->bbox.miny) / 2;

         /* Too far over the edge of the screen? */
         if (midpt.x < 0 || midpt.y < 0 ||
               midpt.x > roadmap_canvas_width() ||
               midpt.y > roadmap_canvas_height()) {

            /* Keep this one in the cache -- it may move further on-screen
             * in the next run.  Keeping it is cheaper than looking it
             * up again.
             */
            cPtr->opacity = 0;
            roadmap_list_append
               (&undrawn_labels, roadmap_list_remove(&cPtr->link));
            continue;
         }

         cannot_label = 0;

         /* do not draw new labels that have better (longer lines) duplicates*/
         if (whichlist == NEWLIST) {
             ROADMAP_LIST_FOR_EACH_FROM_TO(cPtr->link.next, (RoadMapListItem *)&RoadMapLabelNew, item2, tmp2) {

                 ocPtr = (roadmap_label *)item2;

                 if (ocPtr->featuresize_sq >= cPtr->featuresize_sq &&
                 		(!strcmp(cPtr->text, ocPtr->text)) &&
                 		((cPtr->flags & LABEL_FLAG_PLACE) ==
                      (ocPtr->flags & LABEL_FLAG_PLACE))) {
                     cannot_label++;
                     break;
                 }
             }
         }

         /* compare against already rendered labels */
         if (!cannot_label) {
             if (whichlist == NEWLIST)
                end = (RoadMapListItem *)&RoadMapLabelCache;
             else
                end = item;

             ROADMAP_LIST_FOR_EACH_FROM_TO
                    ( RoadMapLabelCache.list_first, end, item2, tmp2) {

                ocPtr = (roadmap_label *)item2;

                if (ocPtr->gen != RoadMapLabelGeneration) {
                   continue;
                }

                if ((cPtr->flags & LABEL_FLAG_PLACE) ==
                    (ocPtr->flags & LABEL_FLAG_PLACE) &&
                    !strcmp(cPtr->text, ocPtr->text)) {

                   cannot_label++;  /* label is a duplicate */
                   break;
                }


                /* if bounding boxes don't overlap, we're clear */
                if (rect_overlap (&ocPtr->bbox, &cPtr->bbox)) {

                   /* cancel the poly check below for now.
                    * if rect overlap it probably means we have too many
                    * labels on screen.
                    */
                   cannot_label++;
                   break;

                   /* if labels are horizontal, bbox check is sufficient */
                   if(!angles) {
                      cannot_label++;
                      break;
                   }

                   /* if labels are "almost" horizontal, the bbox check is
                    * close enough.  (in addition, the line intersector
                    * has trouble with flat or steep lines.)
                    */
                   aang = abs(cPtr->angle);
                   if (aang < 4 || aang > 86) {
                      cannot_label++;
                      break;
                   }

                   aang = abs(ocPtr->angle);
                   if (aang < 4 || aang > 86) {
                      cannot_label++;
                      break;
                   }

                   /* otherwise we do the full poly check */
                   if (poly_overlap (ocPtr, cPtr)) {
                      cannot_label++;
                      break;
                   }
                }
             }
         }
              
              if (!cannot_label) {
                 RoadMapPen test_pen = roadmap_layer_get_pen (cPtr->line.cfcc, 0, 0);
                 if (!test_pen)
                    cannot_label++;
              }

         if(cannot_label) {
            /* Keep this one in the cache as we may need it for the next
             * run.  Keeping it is cheaper than looking it up again.
             */
            cPtr->opacity = 0;
            roadmap_list_append
               (&undrawn_labels, roadmap_list_remove(&cPtr->link));
            continue; /* next label */
         }

         //font_type = (cPtr->line.cfcc <= ROADMAP_ROAD_EXIT || cPtr->line.cfcc == ROADMAP_AREA_CITY);
         font_type = FONT_TYPE_BOLD | FONT_TYPE_OUTLINE;

         if ( (draw || !draw && cPtr->opacity)  && 
             (!(cPtr->flags & LABEL_FLAG_PLACE) ||
               (place_label_count++ < MaxPlaceLabel))) {
#if POLY_OUTLINE
			 {
				 int lines = 4;
				 roadmap_canvas_draw_multiple_lines(1, &lines, cPtr->poly, 1);
			 }
#endif			 
            int size = get_label_size(cPtr);

            currentLabelPen = roadmap_layer_get_label_pen (cPtr->line.cfcc);
         
                if (cPtr->opacity < 256) {
                   cPtr->opacity += 32;
                }
                
                if (full_label_draw && cPtr->opacity < 256) {
                   full_label_draw = 0;
                }
                
            if (currentLabelPen != NULL)
               roadmap_canvas_select_pen (currentLabelPen);

            roadmap_label_draw_text
               (cPtr->text, &cPtr->text_point, &cPtr->center_point,
                angles, angles ? cPtr->angle : 0,
                size, font_type, cPtr->opacity);

            if (cPtr->text2)
               roadmap_label_draw_text
                                 (cPtr->text2, &cPtr->text2_point, &cPtr->center_point,
                                  angles, angles ? cPtr->angle : 0,
                                  size, font_type, cPtr->opacity);

            if (currentLabelPen != NULL)
               roadmap_canvas_select_pen (RoadMapLabelPen);

            if (whichlist == NEWLIST) {
               /* move the rendered label to the cache */
               roadmap_list_append
                  (&RoadMapLabelCache, roadmap_list_remove(&cPtr->link));
            }
            
            //cPtr->flags |= LABEL_FLAG_DRAWN;
         } else {
            cPtr->opacity = 0;
            roadmap_list_append
               (&undrawn_labels, roadmap_list_remove(&cPtr->link));
         }

      } /* next label */
   } /* next list */

   if (RoadMapLabelLastGeneration == RoadMapLabelGeneration || !draw) {
      /* This is a fast draw. Keep everything in the cache. */
      ROADMAP_LIST_SPLICE (&RoadMapLabelCache, &undrawn_labels);
   } else {
      RoadMapLabelLastGeneration = RoadMapLabelGeneration;
      ROADMAP_LIST_SPLICE (&RoadMapLabelSpares, &undrawn_labels);
   }
   
   //printf("<= spare: %d, cache: %d, new: %d\n", roadmap_list_count(&RoadMapLabelSpares), roadmap_list_count(&RoadMapLabelCache), roadmap_list_count(&RoadMapLabelNew));
   return full_label_draw;
}

void roadmap_label_activate (void) {


   RoadMapLabelPen = roadmap_canvas_create_pen ("labels.main");

   roadmap_canvas_set_foreground
      (roadmap_config_get (&RoadMapConfigLabelsColor));

   roadmap_canvas_set_thickness (2);
   
   
   RoadMapLabelMinFeatSizeSq = ADJ_SCALE(roadmap_config_get_integer (&RoadMapConfigMinFeatureSize));
   RoadMapLabelMinFeatSizeSq *= RoadMapLabelMinFeatSizeSq;

}

#ifdef OGL_TILE
static void roadmap_label_on_delete_tile (int tile_id) {
   RoadMapListItem *item, *tmp;
   int i, j;
   
   ROADMAP_LIST_FOR_EACH (&RoadMapLabelCache, item, tmp) {
		roadmap_label *cPtr = (roadmap_label *)item;
      
      for (i = 0; i < cPtr->num_tiles; i++) {
         if (cPtr->tile_ids[i] == tile_id) {
            for (j = i ; j < cPtr->num_tiles -1; j++) {
               cPtr->tile_ids[j] = cPtr->tile_ids[j +1];
            }
            cPtr->num_tiles--;
         }
      }
      
		if (cPtr->num_tiles == 0) {
         cPtr->flags |= LABEL_FLAG_NOT_CACHED;
         //roadmap_list_append (&RoadMapLabelSpares, roadmap_list_remove(item));
      }
   }
   
   ROADMAP_LIST_FOR_EACH (&RoadMapLabelNew, item, tmp) {
		roadmap_label *cPtr = (roadmap_label *)item;
      
      for (i = 0; i < cPtr->num_tiles; i++) {
         if (cPtr->tile_ids[i] == tile_id) {
            for (j = i ; j < cPtr->num_tiles -1; j++) {
               cPtr->tile_ids[j] = cPtr->tile_ids[j +1];
            }
            cPtr->num_tiles--;
         }
      }
      
		if (cPtr->num_tiles == 0) {
         cPtr->flags |= LABEL_FLAG_NOT_CACHED;
         //roadmap_list_append (&RoadMapLabelSpares, roadmap_list_remove(item));
      }
   }
}
#endif //OGL_TILE

int roadmap_label_initialize (void) {

   roadmap_config_declare
       ("schema", &RoadMapConfigMinFeatureSize,  "25", NULL);

   roadmap_config_declare
       ("schema", &RoadMapConfigLabelsColor,  "#000000", NULL);

   roadmap_config_declare
          ("schema", &RoadMapConfigLabelsBgColor,  "#ffffffbf", NULL);


   ROADMAP_LIST_INIT(&RoadMapLabelCache);
   ROADMAP_LIST_INIT(&RoadMapLabelSpares);
   ROADMAP_LIST_INIT(&RoadMapLabelNew);

   roadmap_skin_register (roadmap_label_activate);
#ifdef OGL_TILE
   roadmap_canvas_tile_register_on_delete(roadmap_label_on_delete_tile);
#endif
   return 0;
}

void roadmap_label_clear (int square) {

   RoadMapListItem *item, *tmp;
   int west, east, south, north;
   return;//AVIR - this function is wrong, comparing coord to lon/lat.
	roadmap_tile_edges (square, &west, &east, &south, &north);

   ROADMAP_LIST_FOR_EACH (&RoadMapLabelCache, item, tmp) {
		roadmap_label *cPtr = (roadmap_label *)item;

		if ((cPtr->flags & LABEL_FLAG_PLACE) ?
			  (cPtr->center_point.x >= west &&
				cPtr->center_point.x < east &&
				cPtr->center_point.y >= south &&
				cPtr->center_point.y < north) :
			  (cPtr->line.plugin_id == ROADMAP_PLUGIN_ID &&
			   cPtr->line.square == square)) {
         roadmap_list_append
            (&RoadMapLabelSpares, roadmap_list_remove(item));
		}
   }

   ROADMAP_LIST_FOR_EACH (&RoadMapLabelNew, item, tmp) {
		roadmap_label *cPtr = (roadmap_label *)item;

		if ((cPtr->flags & LABEL_FLAG_PLACE) ?
			  (cPtr->center_point.x >= west &&
				cPtr->center_point.x < east &&
				cPtr->center_point.y >= south &&
				cPtr->center_point.y < north) :
			  (cPtr->line.plugin_id == ROADMAP_PLUGIN_ID &&
			   cPtr->line.square == square)) {
         roadmap_list_append
            (&RoadMapLabelSpares, roadmap_list_remove(item));
		}
   }
}

void roadmap_label_clear_all (void) {
   ROADMAP_LIST_SPLICE (&RoadMapLabelSpares, &RoadMapLabelCache);
}
