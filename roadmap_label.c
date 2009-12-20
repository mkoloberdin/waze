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

#ifdef ROADMAP_USE_LINEFONT
#include "roadmap_linefont.h"
#endif

#include "roadmap_skin.h"
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

static RoadMapConfigDescriptor RoadMapConfigLabelsColor =
                        ROADMAP_CONFIG_ITEM("Labels", "Color");

/* this is fairly arbitrary */
#ifdef J2ME
#define MAX_LABELS 1024
#else
#define MAX_LABELS 2048
#endif
#define ROADMAP_LABEL_STREETLABEL_SIZE 16

#define LABEL_FLAG_NOTEXT   0x1
#define LABEL_FLAG_PLACE    0x2

typedef struct {
   RoadMapListItem link;

   int featuresize_sq;

   PluginLine line;
   PluginStreet street;
   char *text;

   RoadMapGuiPoint text_point; /* label point */
   RoadMapGuiPoint center_point; /* label point */
   RoadMapGuiRect bbox; /* label bounding box */
   RoadMapGuiPoint poly[4];

   unsigned short zoom;
   short angle;  /* degrees */
   int gen;    /* combination "drawn" flag and generation marker */

   unsigned char flags;

} roadmap_label;

static RoadMapList RoadMapLabelCache;
static RoadMapList RoadMapLabelSpares;
static RoadMapList RoadMapLabelNew;

static int RoadMapLabelCacheFull;
static int RoadMapLabelCacheAlloced;

static int RoadMapLabelGeneration = 1;
static int RoadMapLabelGenerationMaxAge = 36;
static int RoadMapLabelCurrentZoom;

static RoadMapPen RoadMapLabelPen;

static int RoadMapLabelMinFeatSizeSq;

static int MaxPlaceLabel;

static int rect_overlap (RoadMapGuiRect *a, RoadMapGuiRect *b) {

   if(a->minx > b->maxx) return 0;
   if(a->maxx < b->minx) return 0;
   if(a->miny > b->maxy) return 0;
   if(a->maxy < b->miny) return 0;

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

void roadmap_label_text_extents(const char *text, int size,
        int *width, int *ascent, int *descent,
        int *can_tilt, int *easy_reading)
{
#ifdef ROADMAP_USE_LINEFONT

   roadmap_linefont_extents
        (text, ROADMAP_LABEL_STREETLABEL_SIZE, width, ascent, descent);

   /* The linefont font isn't pretty.  Reading it is hard with
    * a road running through it, so we don't center labels on
    * the road.
    */
   *easy_reading = 0;
   if (can_tilt) *can_tilt = 1;

#else

   roadmap_canvas_get_text_extents
      (text, -1, width, ascent, descent, can_tilt);

   *easy_reading = 1;

#endif
}

void roadmap_label_draw_text(const char *text,
        RoadMapGuiPoint *start, RoadMapGuiPoint *center,
        int doing_angles, int angle, int size)
{

#ifdef ROADMAP_USE_LINEFONT
   roadmap_linefont_text
        (text, doing_angles ? ROADMAP_CANVAS_CENTER_BOTTOM :
                    ROADMAP_CANVAS_CENTER, center, size, angle);
#else
   if (doing_angles) {
      roadmap_canvas_draw_string_angle (start, center, angle, -1, text);
   } else {
      roadmap_canvas_draw_string (center, ROADMAP_CANVAS_CENTER, text);
   }
#endif
}

/* called when a screen repaint is complete.  keeping track of
 * label "generations" involves keeping track of full refreshes,
 * not individual calls to roadmap_label_draw_cache().
 */
void roadmap_label_start (void) {

   static RoadMapPosition last_center;

   /* Cheap cache invalidation:  if the previous center of screen
    * is still visible, assume that the cache is still more useful
    * than not.
    */
   if ( !roadmap_math_point_is_visible (&last_center) ) {
      ROADMAP_LIST_SPLICE (&RoadMapLabelSpares, &RoadMapLabelCache);
   }

   roadmap_math_get_context (&last_center, &RoadMapLabelCurrentZoom);

   MaxPlaceLabel = roadmap_canvas_height() / 75;
}


int roadmap_label_add (const RoadMapGuiPoint *point, int angle,
                       int featuresize_sq, const PluginLine *line) {

   roadmap_label *cPtr = 0;
   PluginStreetProperties properties;

   if (featuresize_sq < RoadMapLabelMinFeatSizeSq) {
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

   cPtr->bbox.minx = 1;
   cPtr->bbox.maxx = -1;

   cPtr->line = *line;
   cPtr->featuresize_sq = featuresize_sq;
   cPtr->angle = normalize_angle(angle);

   cPtr->center_point = *point;

   /* The stored point is not screen oriented, rotate is needed */
   roadmap_math_rotate_coordinates (1, &cPtr->center_point);


   /* the "generation" of a label is refreshed when it is re-added.
    * later, labels cache entries can be aged, and discarded.
    */
   cPtr->gen = RoadMapLabelGeneration;

   cPtr->zoom = RoadMapLabelCurrentZoom;

   roadmap_plugin_get_street_properties
         (&cPtr->line, &properties, PLUGIN_STREET_ONLY);

   if (!properties.street || !*properties.street) {
      cPtr->text = "";
   } else {
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
   cPtr->street = properties.plugin_street;

   roadmap_list_append(&RoadMapLabelNew, &cPtr->link);

   return 0;
}


int roadmap_label_add_place (const RoadMapGuiPoint *point, int angle,
                             int featuresize_sq, const char *name) {

   roadmap_label *cPtr = 0;

   if (featuresize_sq < RoadMapLabelMinFeatSizeSq) {
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
   if (name && *name)
      cPtr->text = strdup(name);
   else
      cPtr->text = "";

   cPtr->bbox.minx = 1;
   cPtr->bbox.maxx = -1;

   cPtr->featuresize_sq = featuresize_sq;
   cPtr->angle = normalize_angle(angle);

   cPtr->center_point = *point;

   /* The stored point is not screen oriented, rotate is needed */
   roadmap_math_rotate_coordinates (1, &cPtr->center_point);


   /* the "generation" of a label is refreshed when it is re-added.
    * later, labels cache entries can be aged, and discarded.
    */
   cPtr->gen = RoadMapLabelGeneration;

   cPtr->zoom = RoadMapLabelCurrentZoom;

   roadmap_list_append(&RoadMapLabelNew, &cPtr->link);

   return 0;
}

int roadmap_label_draw_cache (int angles) {

   RoadMapListItem *item, *tmp;
   RoadMapListItem *item2, *tmp2;
   RoadMapListItem *end;
   RoadMapList undrawn_labels;
   int width, ascent, descent;
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

   ROADMAP_LIST_INIT(&undrawn_labels);
   roadmap_canvas_select_pen (RoadMapLabelPen);

   /* We want to process the cache first, in order to render previously
    * rendered labels again.  Only after doing so (checking for updates
    * in the new list as we go), we'll process what's left of the new
    * list -- those are the truly new labels for this repaint.
    */
   for (whichlist = OLDLIST; whichlist <= NEWLIST; whichlist++)  {

      ROADMAP_LIST_FOR_EACH
           (whichlist == OLDLIST ? &RoadMapLabelCache : &RoadMapLabelNew,
                item, tmp) {

         cPtr = (roadmap_label *)item;

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
               ncPtr = (roadmap_label *)item2;
               if (ncPtr->gen == 0) {
                   continue;
               }

               if ((!(cPtr->flags & LABEL_FLAG_PLACE) &&
                    !(ncPtr->flags & LABEL_FLAG_PLACE) &&
                    roadmap_plugin_same_line (&cPtr->line, &ncPtr->line)) ||
                   (cPtr->flags & LABEL_FLAG_PLACE &&
                    ncPtr->flags & LABEL_FLAG_PLACE &&
                    !strcmp(cPtr->text, ncPtr->text))) {
                   /* Found a new version of this existing line */

                   if (cPtr->flags & LABEL_FLAG_NOTEXT) {
                     cPtr->gen = ncPtr->gen;

                     roadmap_list_insert
                        (&RoadMapLabelSpares,
                         roadmap_list_remove(&ncPtr->link));

                     break;
                   }

                   if ((cPtr->angle != ncPtr->angle) ||
                         (cPtr->zoom != ncPtr->zoom)) {
                       cPtr->bbox.minx = 1;
                       cPtr->bbox.maxx = -1;
                       cPtr->angle = ncPtr->angle;
                       cPtr->zoom = ncPtr->zoom;
                   } else {
                       /* Angle is unchanged -- simple movement only */
                       int dx, dy;

                       dx = ncPtr->center_point.x - cPtr->center_point.x;
                       dy = ncPtr->center_point.y - cPtr->center_point.y;

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
                       }
                   }

                   cPtr->center_point = ncPtr->center_point;

                   cPtr->featuresize_sq = ncPtr->featuresize_sq;
                   cPtr->gen = ncPtr->gen;

                   roadmap_list_insert
                      (&RoadMapLabelSpares, roadmap_list_remove(&ncPtr->link));
                   break;
               }
            }
         }

         if ((cPtr->gen != RoadMapLabelGeneration) ||
             (cPtr->flags & LABEL_FLAG_NOTEXT)) {
            roadmap_list_append
               (&undrawn_labels, roadmap_list_remove(&cPtr->link));
            continue;
         }

         if (!*cPtr->text) {
            cPtr->flags |= LABEL_FLAG_NOTEXT;

            /* We keep the no-label lines in the cache to avoid repeating
             * all these tests just to find that it's invalid, again!
             */
            roadmap_list_append
               (&undrawn_labels, roadmap_list_remove(&cPtr->link));
            continue;
         }


         if (cPtr->bbox.minx > cPtr->bbox.maxx) {
            int can_tilt;
            roadmap_label_text_extents
                    (cPtr->text, ROADMAP_LABEL_STREETLABEL_SIZE ,
                    &width, &ascent, &descent, &can_tilt, &label_center_y);
            angles = angles && can_tilt;

            /* text is too long for this feature */
            /* (4 times longer than feature) */
            if ((width * width / 16) > cPtr->featuresize_sq) {
               /* Keep this one in the cache as the feature size may change
                * in the next run.  Keeping it is cheaper than looking it
                * up again.
                */
               roadmap_list_append
                  (&undrawn_labels, roadmap_list_remove(&cPtr->link));
               continue;
            }

            r.minx = 0;
            r.maxx = width+1;
            r.miny = 0;
            r.maxy = ascent + descent + 1;

            if (angles) {

               cPtr->text_point = get_metrics (cPtr, &r, label_center_y);

            } else {
               /* Text will be horizontal, so bypass a lot of math.
                * (and compensate for eventual centering of text.)
                */
               cPtr->text_point = cPtr->center_point;
               cPtr->bbox.minx =
                  r.minx + cPtr->text_point.x - (r.maxx - r.minx)/2;
               cPtr->bbox.maxx =
                  r.maxx + cPtr->text_point.x - (r.maxx - r.minx)/2;
               cPtr->bbox.miny =
                  r.miny + cPtr->text_point.y - (r.maxy - r.miny)/2;
               cPtr->bbox.maxy =
                  r.maxy + cPtr->text_point.y - (r.maxy - r.miny)/2;
            }
         }

#if POLY_OUTLINE
         {
            int lines = 4;
            roadmap_canvas_draw_multiple_lines(1, &lines, cPtr->poly, 1);
         }
#endif


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

         if(cannot_label) {
            /* Keep this one in the cache as we may need it for the next
             * run.  Keeping it is cheaper than looking it up again.
             * ID: remove label from cache to save space
             */
            roadmap_list_append
               (&/*undrawn_labels*/RoadMapLabelSpares, roadmap_list_remove(&cPtr->link));
            continue; /* next label */
         }

         if ( !(cPtr->flags & LABEL_FLAG_PLACE) ||
               (place_label_count++ < MaxPlaceLabel)) {

            roadmap_label_draw_text
               (cPtr->text, &cPtr->text_point, &cPtr->center_point,
                angles, angles ? cPtr->angle : 0,
                ROADMAP_LABEL_STREETLABEL_SIZE );
         }

         if (whichlist == NEWLIST) {
            /* move the rendered label to the cache */
            roadmap_list_append
               (&RoadMapLabelCache, roadmap_list_remove(&cPtr->link));
         }

      } /* next label */
   } /* next list */

   ROADMAP_LIST_SPLICE (&RoadMapLabelCache, &undrawn_labels);
   RoadMapLabelGeneration++;
   return 0;
}

void roadmap_label_activate (void) {


   RoadMapLabelPen = roadmap_canvas_create_pen ("labels.main");
   roadmap_canvas_set_foreground
      (roadmap_config_get (&RoadMapConfigLabelsColor));

   roadmap_canvas_set_thickness (2);

   RoadMapLabelMinFeatSizeSq = roadmap_config_get_integer (&RoadMapConfigMinFeatureSize);
   RoadMapLabelMinFeatSizeSq *= RoadMapLabelMinFeatSizeSq;
}

int roadmap_label_initialize (void) {

   roadmap_config_declare
       ("schema", &RoadMapConfigMinFeatureSize,  "25", NULL);

   roadmap_config_declare
       ("schema", &RoadMapConfigLabelsColor,  "#000000", NULL);


   ROADMAP_LIST_INIT(&RoadMapLabelCache);
   ROADMAP_LIST_INIT(&RoadMapLabelSpares);
   ROADMAP_LIST_INIT(&RoadMapLabelNew);

   roadmap_skin_register (roadmap_label_activate);
   return 0;
}

void roadmap_label_clear (int square) {

   RoadMapListItem *item, *tmp;
   int west, east, south, north;

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

