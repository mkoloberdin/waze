/* roadmap_screen.c - Draw the map on the screen.
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
 *   See roadmap_screen.h.
 */

#include <math.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_bar.h"
#include "roadmap_gui.h"
#include "roadmap_math.h"
#include "roadmap_config.h"
#include "roadmap_layer.h"
#include "roadmap_square.h"
#include "roadmap_line.h"
#include "roadmap_shape.h"
#include "roadmap_point.h"
#include "roadmap_polygon.h"
#include "roadmap_locator.h"
#include "roadmap_navigate.h"
#include "roadmap_street.h"
#include "roadmap_lang.h"
#include "roadmap_messagebox.h"
#include "roadmap_line_route.h"
#include "roadmap_performance.h"
#include "roadmap_map_settings.h"
#include "roadmap_sprite.h"
#include "roadmap_object.h"
#include "roadmap_trip.h"
#include "roadmap_message.h"
#include "roadmap_canvas.h"
#include "roadmap_screen_obj.h"
#include "roadmap_state.h"
#include "roadmap_pointer.h"
#include "roadmap_display.h"
#include "roadmap_label.h"
#include "roadmap_plugin.h"
#include "roadmap_skin.h"
#include "roadmap_main.h"
#include "roadmap_device_events.h"
#include "roadmap_softkeys.h"
#include "editor/editor_screen.h"
#include "navigate/navigate_main.h"
#include "roadmap_download_settings.h"
#include "roadmap_view.h"
#include "roadmap_screen.h"
#include "roadmap_start.h"

#include "Realtime/RealtimeAlerts.h"
#include "Realtime/RealtimeTrafficInfo.h"
#include "Realtime/RealtimeTrafficInfoPlugin.h"
#include "Realtime/RealtimeAlertCommentsList.h"
#include "roadmap_alert.h"
#include "roadmap_alerter.h"
#include "roadmap_res.h"
#include "roadmap_ticker.h"
#include "roadmap_message_ticker.h"
#include "roadmap_warning.h"
#include "roadmap_time.h"
#include "animation/roadmap_animation.h"

//#define HI_RES_SCREEN

#ifdef SSD
#include "ssd/ssd_dialog.h"
#endif
#ifdef VIEW_MODE_3D_OGL
#include "roadmap_canvas3d.h" // roadmap_canvas3_project
#endif// VIEW_MODE_3D_OGL
#ifdef OGL_TILE
#include "roadmap_canvas_tile.h"

int next_pot(int num) {
   int outNum = num;
   
   --outNum;
   outNum |= outNum >> 1;
   outNum |= outNum >> 2;
   outNum |= outNum >> 4;
   outNum |= outNum >> 8;
   outNum |= outNum >> 16;
   outNum++;
   
   if (outNum < 16)
      outNum = 16;
   
   return outNum;
};

#endif// OGL_TILE
#include "roadmap_analytics.h"

extern BOOL roadmap_horizontal_screen_orientation();

static RoadMapConfigDescriptor RoadMapConfigAccuracyMouse =
                        ROADMAP_CONFIG_ITEM("Accuracy", "Mouse");

static RoadMapConfigDescriptor RoadMapConfigMapBackground =
                        ROADMAP_CONFIG_ITEM("Map", "Background");

static RoadMapConfigDescriptor RoadMapConfigMapSigns =
                        ROADMAP_CONFIG_ITEM("Map", "Signs");

static RoadMapConfigDescriptor RoadMapConfigMapRefresh =
                        ROADMAP_CONFIG_ITEM("Map", "Refresh");

static RoadMapConfigDescriptor RoadMapConfigStylePrettyDrag =
                  ROADMAP_CONFIG_ITEM("Style", "Pretty Lines when Dragging");

static RoadMapConfigDescriptor RoadMapConfigStyleObjects =
                  ROADMAP_CONFIG_ITEM("Style", "Show Objects when Dragging");

static RoadMapConfigDescriptor RoadMapConfigMapLabels =
                        ROADMAP_CONFIG_ITEM("Map", "Labels");

static RoadMapConfigDescriptor RoadMapConfigMapOrientation =
                        ROADMAP_CONFIG_ITEM("Map", "Orientation");

static RoadMapConfigDescriptor RoadMapConfigShowScreenIconsOnTap =
                        ROADMAP_CONFIG_ITEM("Map Icons", "Show on screen on tap");

static BOOL RoadMapScreenBackgroundRun = FALSE;

#ifndef TOUCH_SCREEN
static BOOL isXIconOpen = FALSE;
#endif

static int RoadMapScreenInitialized = 0;
static int RoadMapScreenFrozen = 0;

static RoadMapGuiPoint RoadMapScreenPointerLocation;
static RoadMapPosition RoadMapScreenCenter = {0,0};

static int RoadMapScreenViewMode = VIEW_MODE_2D;
static int RoadMapScreenOGLViewMode = VIEW_MODE_2D;
static int RoadMapScreenOrientationMode = ORIENTATION_DYNAMIC;
static int RoadMapScreen3dHorizon;
#ifdef VIEW_MODE_3D_OGL
static int RoadMapScreenOGL3dHorizon;
#endif
static int RoadMapScreenLabels;
static int RoadMapScreenRotation;
static int RoadMapScreenWidth;
static int RoadMapScreenHeight;
static int RoadMapScreenForceRefresh = 0;

static int RoadMapScreenDeltaX;
static int RoadMapScreenDeltaY;
static int RoadMapScreenCenterDelta;
static RoadMapGuiPoint RoadMapScreenCenterPixel;

static RoadMapGuiPoint RoadMapScreenLowerEdge;
static int RoadMapScreenAreaDist[LAYER_PROJ_AREAS-1];

static RoadMapPen RoadMapScreenLastPen = NULL;
static RoadMapImage RoadMapScreenLastImage = NULL;
static int RoadMapScreenLastOpposite = 0;
static void roadmap_screen_after_refresh (void) {}
static int RoadMapScreenDirty;

static uint32_t DragLastTime;
static int DragSpeedX;
static int DragSpeedY;

static void roadmap_screen_set_screen_scale( int screen_scale );
static void set_horizon(void);

#ifdef OPENGL
static void animation_set_callback (void *context);
static void animation_ended_callback (void *context);

static RoadMapAnimationCallbacks gAnimationCallbacks =
{
   animation_set_callback,
   animation_ended_callback
};

static long RoadMapScreenFinalScale = 0;
static int RoadMapScreenGlowScale = -1;
static RoadMapPosition RoadMapScreenGlowPos;
static RoadMapGuiPoint RoadMapScreenGlowOffset;
#endif

#if !defined(INLINE_DEC)
#define INLINE_DEC
#endif

INLINE_DEC void set_center (const RoadMapPosition *center, int duration, int timing);
INLINE_DEC void set_scale (long scale, int use_map_units, int duration, int timing);


static RoadMapScreenSubscriber RoadMapScreenAfterRefresh =
                                       roadmap_screen_after_refresh;

static RoadMapScreenSubscriber RoadMapScreenAfterFlowControlRefresh = NULL;

#if defined(IPHONE) || defined(ANDROID) || (defined(OPENGL) && defined(_WIN32))
static int CordingEvent = 0;
static RoadMapPosition CordingAnchors[MAX_CORDING_POINTS];
static BOOL CordingRotationEnabled = TRUE;
static int CordingAngle;
static float CordingScale = 0;
static zoom_t CordingStartZoom = 0;
#endif

/* Define the buffers used to group all actual drawings. */

#ifdef J2ME
#define ROADMAP_SCREEN_BULK  256
#else
#define ROADMAP_SCREEN_BULK  4096
#endif

#define SCREEN_TIMEOUT 5000
#ifdef OPENGL
static const char* SCREEN_OBJECT = "screen_map";
#endif
static int screen_touched = 0;
static int screen_touched_off = 0;

// 2D / 3D view mode event
static const char* ANALYTICS_EVENT_VIEWMODE_2D   = "2D";
static const char* ANALYTICS_EVENT_VIEWMODE_3D   = "3D";

/* This is a default definition, because we might want to set this smaller
 * for some memory-starved targets.
 */
#ifndef ROADMAP_MAX_VISIBLE
#define ROADMAP_MAX_VISIBLE  2000
#endif

#define SQUARE_IN_VIEW 0x1
#define SQUARE_DRAWN   0x2

#ifdef J2ME
#define REFRESH_FLOW_CONTROL_TIMEOUT 150
#elif (defined (OPENGL) && defined (IPHONE))
#define REFRESH_FLOW_CONTROL_TIMEOUT 30
#else
#define REFRESH_FLOW_CONTROL_TIMEOUT 50
#endif
#define SCREEN_FAST_DRAG      0x1
#define SCREEN_FAST_OTHER     0x2
#define SCREEN_FAST_NO_REDRAW 0x4
#define SCREEN_FAST_ANIMATING 0x8
#ifndef TOUCH_SCREEN
static void roadmap_screen_draw_Xicon();
#endif
static struct {

   int *cursor;
   int *end;
   int data[ROADMAP_SCREEN_BULK];

} RoadMapScreenObjects;

struct roadmap_screen_point_buffer {

   RoadMapGuiPoint *cursor;
   RoadMapGuiPoint *end;
   RoadMapGuiPoint data[ROADMAP_SCREEN_BULK];
   unsigned char real[ROADMAP_SCREEN_BULK];
};

static struct roadmap_screen_point_buffer RoadMapScreenLinePoints;
static RoadMapGuiPoint *RoadMapScreenLinePointsAccum = RoadMapScreenLinePoints.data;
static struct roadmap_screen_point_buffer RoadMapScreenPoints;

static int RoadMapPolygonGeoPoints[ROADMAP_SCREEN_BULK];


static RoadMapPen RoadMapBackground = NULL;
static RoadMapPen RoadMapNoTileBg = NULL;
static RoadMapPen RoadMapPenEdges = NULL;

static int RoadMapScreenRefreshFlowControl = 0;
static int RoadMapScreenFastRefresh = 0;
static int RoadMapScreenPrevFast = 0;
#ifdef OGL_TILE
static int RoadMapScreenFastRefreshOverride = 0;
#define FAST_REFRESH (RoadMapScreenFastRefreshOverride ? 0 : RoadMapScreenFastRefresh)
#else
#define FAST_REFRESH (RoadMapScreenFastRefresh)
#endif //OGL_TILE

#if defined(HI_RES_SCREEN)
static int RoadMapScreenType = RM_SCREEN_TYPE_HD_GENERIC;	/* High definition by default */
#else
static int RoadMapScreenType = RM_SCREEN_TYPE_SD_GENERIC;	/* Standard definition by default */
#endif

static int RoadMapScreenScale = 100; // in % (default is 100%)

static void roadmap_screen_repaint_now (void);
static void roadmap_screen_repaint (void);
#if 1
static void roadmap_screen_mark_fast_repaint (void);
#endif

#if !defined(INLINE_DEC)
#define INLINE_DEC static
#endif

INLINE_DEC void rds_unproject(RoadMapGuiPoint *point)
{
#ifdef VIEW_MODE_3D_OGL
	if (RoadMapScreenOGLViewMode== VIEW_MODE_3D) {
		roadmap_canvas3_unproject(point);
	}
#else
	roadmap_math_unproject(point);
#endif// VIEW_MODE_3D_OGL
}

INLINE_DEC void roadmap_screen_flush_points (void) {

   if (RoadMapScreenPoints.cursor == RoadMapScreenPoints.data) return;

   dbg_time_start(DBG_TIME_FLUSH_POINTS);
   roadmap_math_rotate_coordinates
       (RoadMapScreenPoints.cursor - RoadMapScreenPoints.data,
        RoadMapScreenPoints.data);

   roadmap_canvas_draw_multiple_points
       (RoadMapScreenPoints.cursor - RoadMapScreenPoints.data,
        RoadMapScreenPoints.data);

   RoadMapScreenPoints.cursor  = RoadMapScreenPoints.data;

   dbg_time_end(DBG_TIME_FLUSH_POINTS);
}


INLINE_DEC void roadmap_screen_flush_lines (void) {

   if (RoadMapScreenObjects.cursor == RoadMapScreenObjects.data) return;

   dbg_time_start(DBG_TIME_FLUSH_LINES);

   roadmap_math_rotate_coordinates
       (RoadMapScreenLinePoints.cursor - RoadMapScreenLinePoints.data,
        RoadMapScreenLinePoints.data);

   dbg_time_end(DBG_TIME_FLUSH_LINES);
   if (RoadMapScreenLastImage == NULL)
      roadmap_canvas_draw_multiple_lines
         (RoadMapScreenObjects.cursor - RoadMapScreenObjects.data,
          RoadMapScreenObjects.data,
          RoadMapScreenLinePoints.data, FAST_REFRESH);
#ifdef OPENGL
   else
      roadmap_canvas_draw_multiple_tex_lines
         (RoadMapScreenObjects.cursor - RoadMapScreenObjects.data,
          RoadMapScreenObjects.data,
          RoadMapScreenLinePoints.data, FAST_REFRESH,
          RoadMapScreenLastImage,
          RoadMapScreenLastOpposite);
#endif //OPENGL

   dbg_time_start(DBG_TIME_FLUSH_LINES);
   if (RoadMapScreenLinePoints.cursor < RoadMapScreenLinePointsAccum) {
      int count = RoadMapScreenLinePointsAccum - RoadMapScreenLinePoints.cursor;
      memmove(RoadMapScreenLinePoints.data, RoadMapScreenLinePointsAccum,
          sizeof(*RoadMapScreenLinePointsAccum) * count);

      RoadMapScreenLinePointsAccum = RoadMapScreenLinePoints.data + count;
   } else {
      RoadMapScreenLinePointsAccum = RoadMapScreenLinePoints.data;
   }
   RoadMapScreenObjects.cursor = RoadMapScreenObjects.data;
   RoadMapScreenLinePoints.cursor  = RoadMapScreenLinePoints.data;
   dbg_time_end(DBG_TIME_FLUSH_LINES);
}


#define SEGMENT_START    0x1
#define SEGMENT_END      0x2
#define SEGMENT_AS_POINT 0x4
#define SEGMENT_OPPOSITE 0x8

INLINE_DEC int roadmap_screen_add_segment_point (RoadMapGuiPoint *point,
                                                 RoadMapPen *pens,
                                                 int num_pens,
                                                 RoadMapImage image,
                                                 int flags) {

   int layer_proj = 0;
   RoadMapPen pen;

   dbg_time_start(DBG_TIME_ADD_SEGMENT);

   num_pens--;

   if (point) {
#ifndef OGL_TILE
      if (isViewModeAny3D())
      {
         int edge_distance = roadmap_math_screen_distance
            (point, &RoadMapScreenLowerEdge, MATH_DIST_SQUARED);

         for (layer_proj=0; layer_proj<LAYER_PROJ_AREAS-1; layer_proj++) {
            if ((layer_proj == num_pens) || (edge_distance < RoadMapScreenAreaDist[layer_proj]))
                  break;
         }

#if defined (OPENGL) && defined (VIEW_MODE_3D_OGL)
			if (pens[layer_proj]) pen = pens[layer_proj];
			else pen = NULL;
#else
         pen = pens[layer_proj];
#endif
      }
      else
#endif //!OGL_TILE
      {
         pen = pens[0];
      }

      if (flags & SEGMENT_AS_POINT) {

         /* This is a speical handling of a point */

         if (!pen) {
            dbg_time_end(DBG_TIME_ADD_SEGMENT);
            return -1;
         }

         if (RoadMapScreenPoints.cursor >= RoadMapScreenPoints.end) {
            roadmap_screen_flush_points ();
         }

         RoadMapScreenPoints.cursor[0] = *point;
         RoadMapScreenPoints.cursor += 1;

      } else {
         /* This is the start of the segment */
         *(RoadMapScreenLinePointsAccum++) = *point;
      }

   } else {
      pen = RoadMapScreenLastPen;
   }

   if ((RoadMapScreenLastPen != pen) ||
       (flags & SEGMENT_END) ||
       RoadMapScreenLastImage != image ||
       RoadMapScreenLastOpposite != (flags & SEGMENT_OPPOSITE) ) {

      if (RoadMapScreenLastPen &&
         (RoadMapScreenLinePointsAccum - RoadMapScreenLinePoints.cursor) > 1) {

         *RoadMapScreenObjects.cursor =
            RoadMapScreenLinePointsAccum - RoadMapScreenLinePoints.cursor;
         RoadMapScreenLinePoints.cursor = RoadMapScreenLinePointsAccum;
         RoadMapScreenObjects.cursor += 1;
      } else {
         RoadMapScreenLinePointsAccum = RoadMapScreenLinePoints.cursor;
      }


      if (RoadMapScreenLastPen != pen) {
         dbg_time_end(DBG_TIME_ADD_SEGMENT);
         roadmap_screen_flush_lines ();
         roadmap_screen_flush_points ();
         dbg_time_start(DBG_TIME_ADD_SEGMENT);
         if (pen) roadmap_canvas_select_pen (pen);
         RoadMapScreenLastPen = pen;
         RoadMapScreenLastImage = image;
         RoadMapScreenLastOpposite = flags & SEGMENT_OPPOSITE;
      } else if (RoadMapScreenLastImage != image ||
                 RoadMapScreenLastOpposite != (flags & SEGMENT_OPPOSITE) ) {
         roadmap_screen_flush_lines ();
         RoadMapScreenLastImage = image;
         RoadMapScreenLastOpposite = flags & SEGMENT_OPPOSITE;
      }

      if (!(flags & SEGMENT_END) && !(flags & SEGMENT_AS_POINT)) {
         *(RoadMapScreenLinePointsAccum++) = *point; /* Show the start of this segment. */
      }
   }

   dbg_time_end(DBG_TIME_ADD_SEGMENT);

   if (!pen) return -1;
   else return layer_proj;
}


#ifndef J2ME
INLINE_DEC int  roadmap_screen_draw_one_line_internal (RoadMapPosition *from,
                                                       RoadMapPosition *to,
                                                       int fully_visible,
                                                       RoadMapPosition *first_shape_pos,
                                                       int first_shape,
                                                       int last_shape,
                                                       RoadMapShapeItr shape_itr,
                                                       RoadMapPen *pens,
                                                       int num_pens,
                                                       int label_max_proj,
                                                       int *total_length_ptr,
                                                       RoadMapGuiPoint *middle,
                                                       int *angle,
                                                       RoadMapImage image,
                                                       BOOL opposite) {

   RoadMapGuiPoint point0;
   RoadMapGuiPoint point1;

   /* These are used when the line has a shape: */
   RoadMapPosition midposition;
   RoadMapPosition last_midposition;

   int i;

   int last_point_visible = 0;
   int longest = -1;

   int drawn = 0;
   int opposite_flag = 0;

   dbg_time_start(DBG_TIME_DRAW_ONE_LINE);

   if (total_length_ptr) *total_length_ptr = 0;

   if (opposite)
      opposite_flag = SEGMENT_OPPOSITE;

   //fully_visible = 0;

   /* if the pen has changed, we need to flush the previous lines and points
    */

   if (first_shape >= 0) {
      /* Draw a shaped line. */

      if (last_shape - first_shape + 3 >=
            RoadMapScreenLinePoints.end - RoadMapScreenLinePoints.cursor) {

         if (last_shape - first_shape + 3 >=
               (RoadMapScreenLinePoints.end - RoadMapScreenLinePoints.data)) {

            roadmap_log (ROADMAP_ERROR,
                  "cannot show all shape points (%d entries needed).",
                  last_shape - first_shape + 3);

            last_shape =
               first_shape
               + (RoadMapScreenLinePoints.end - RoadMapScreenLinePoints.data)
               - 4;
         }

         dbg_time_end(DBG_TIME_DRAW_ONE_LINE);
         roadmap_screen_flush_lines ();
         dbg_time_start(DBG_TIME_DRAW_ONE_LINE);
      }

      /* All the shape positions are relative: we need an absolute position
       * to start with.
       */
      last_midposition = *from;
      midposition = *first_shape_pos;

      if (fully_visible) {
         roadmap_math_coordinate (from, &point0);
         roadmap_screen_add_segment_point (&point0, pens, num_pens,
                                           image, opposite_flag | SEGMENT_START);

         for (i = first_shape; i <= last_shape; ++i) {

            if (shape_itr) (*shape_itr) (i, &midposition);
            else roadmap_shape_get_position (i, &midposition);

            roadmap_math_coordinate (&midposition, &point0);
            roadmap_screen_add_segment_point (&point0, pens, num_pens, image, opposite_flag);
         }

         roadmap_math_coordinate (to, &point0);
         roadmap_screen_add_segment_point (&point0, pens, num_pens,
                                           image, opposite_flag | SEGMENT_END);
         drawn = 1;

      } else {

         last_point_visible = 0; /* We have drawn nothing yet. */

         for (i = first_shape; i <= last_shape; ++i) {

            if (shape_itr) (*shape_itr) (i, &midposition);
            else roadmap_shape_get_position (i, &midposition);

            if (roadmap_math_line_is_visible (&last_midposition, &midposition) &&
                  roadmap_math_get_visible_coordinates
                  (&last_midposition, &midposition, &point0, &point1)) {

               if ((point0.x == point1.x) && (point0.y == point1.y)) {

                  if (last_point_visible) {

                     /* This segment is very short, we can skip it */
                     last_midposition = midposition;

                     continue;
                  }

               } else {

                  if (total_length_ptr) {

                     int length_sq = roadmap_math_screen_distance
                        (&point1, &point0, MATH_DIST_SQUARED);

                     /* bad math, but it's for a labelling heuristic anyway */
                     *total_length_ptr += length_sq;

                     if (length_sq > longest) {
                        longest = length_sq;
                        if (angle) {
                           *angle = roadmap_math_azymuth(&last_midposition, &midposition);
                        }
                        middle->x = (point1.x + point0.x) / 2;
                        middle->y = (point1.y + point0.y) / 2;
                     }
                  }
               }

               /* Show this line: add 2 points if this is the start of a new
                * complete line (i.e. the first visible line), or just add
                * one more point to the current complete line.
                */
               if (!last_point_visible) {
                  roadmap_screen_add_segment_point (&point0, pens, num_pens,
                                                    image, opposite_flag | SEGMENT_START);
                  drawn = 1;
               }

               last_point_visible = roadmap_math_point_is_visible (&midposition);
               if (last_point_visible) {
                  roadmap_screen_add_segment_point (&point1, pens, num_pens, image, opposite_flag);
                  drawn = 1;

               } else {

                  /* Show the previous segment as the end of a complete line.
                   * The remaining part of the shaped line, if any, will be
                   * drawn as a new complete line.
                   */
                  roadmap_screen_add_segment_point (&point1, pens, num_pens,
                                                    image, opposite_flag | SEGMENT_END);
                  drawn = 1;
                  if (last_shape - i + 3 >=
                        RoadMapScreenLinePoints.end - RoadMapScreenLinePoints.cursor) {
                     dbg_time_end(DBG_TIME_DRAW_ONE_LINE);
                     roadmap_screen_flush_lines ();
                     dbg_time_start(DBG_TIME_DRAW_ONE_LINE);
                  }
               }
            }
            last_midposition = midposition; /* The latest position is our new start. */
         }

         if (roadmap_math_get_visible_coordinates
               (&last_midposition, to, &point0, &point1)) {

            if (total_length_ptr) {

               int length_sq = roadmap_math_screen_distance
                  (&point1, &point0, MATH_DIST_SQUARED);
               if (length_sq) {
                  /* bad math, but it's for a labelling heuristic anyway */
                  *total_length_ptr += length_sq;

                  if (length_sq > longest) {
                     longest = length_sq;
                     if (angle) {
                        *angle = roadmap_math_azymuth(&last_midposition, to);
                     }
                     middle->x = (point1.x + point0.x) / 2;
                     middle->y = (point1.y + point0.y) / 2;
                  }
               }
            }

            if (!last_point_visible) {
               roadmap_screen_add_segment_point (&point0, pens, num_pens,
                                                 image, opposite_flag | SEGMENT_START);
            }
            roadmap_screen_add_segment_point (&point1, pens, num_pens, image, opposite_flag);

            /* set last point as visible to force line completion at the next
             * statement.
             */
            last_point_visible = 1;

            drawn = 1;
         }

         if (last_point_visible) {

            /* End the current complete line. */
            roadmap_screen_add_segment_point (NULL, pens, num_pens,
                                              image, opposite_flag | SEGMENT_END);

            drawn = 1;
         }
      }

   } else {
      /* Draw a line with no shape. */

      /* Optimization: do not draw a line that is obviously not visible. */
      if (! fully_visible) {
         if (! roadmap_math_line_is_visible (from, to)) {
            dbg_time_end(DBG_TIME_DRAW_ONE_LINE);
            return 0;
         }
      }

      /* Optimization: adjust the edges of the line so
       * they do not go out of the screen. */
      if (!roadmap_math_get_visible_coordinates (from, to, &point0, &point1)) {
         return 0;
      }

      if ((point0.x == point1.x) && (point0.y == point1.y)) {
         /* draw a point instead of a line */

         roadmap_screen_add_segment_point (&point0, pens, num_pens,
                                           image, opposite_flag | SEGMENT_AS_POINT);

         dbg_time_end(DBG_TIME_DRAW_ONE_LINE);
         return 1;

      } else {

         if (total_length_ptr) {

            *total_length_ptr = roadmap_math_screen_distance
                                   (&point1, &point0, MATH_DIST_SQUARED);

            if (angle) {
               *angle = roadmap_math_azymuth(from, to);
            }
            middle->x = (point1.x + point0.x) / 2;
            middle->y = (point1.y + point0.y) / 2;
         }
      }

      if (RoadMapScreenLinePoints.cursor + 2 >= RoadMapScreenLinePoints.end) {
         dbg_time_end(DBG_TIME_DRAW_ONE_LINE);
         roadmap_screen_flush_lines ();
         dbg_time_start(DBG_TIME_DRAW_ONE_LINE);
      }

      roadmap_screen_add_segment_point (&point0, pens, num_pens, image, opposite_flag | SEGMENT_START);
      roadmap_screen_add_segment_point (&point1, pens, num_pens, image, opposite_flag | SEGMENT_END);

      drawn = 1;
   }

   dbg_time_end(DBG_TIME_DRAW_ONE_LINE);
  // if (!drawn) {
//      printf("not drawn\n");
//   }
   return drawn;
}

#if 0
int  roadmap_screen_draw_one_line_internal (RoadMapPosition *from,
                                   RoadMapPosition *to,
                                   int fully_visible,
                                   RoadMapPosition *first_shape_pos,
                                   int first_shape,
                                   int last_shape,
                                   RoadMapShapeItr shape_itr,
                                   RoadMapPen *pens,
                                   int num_pens,
                                   int label_max_proj,
                                   int *total_length_ptr,
                                   RoadMapGuiPoint *middle,
                                   int *angle) {

   RoadMapGuiPoint point0;
   RoadMapGuiPoint point1;

   /* These are used when the line has a shape: */
   RoadMapPosition midposition;
   RoadMapPosition last_midposition;

   int i;

   int last_point_visible = 0;
   int longest = -1;

   int drawn = 0;

   fully_visible = 0;

   dbg_time_start(DBG_TIME_DRAW_ONE_LINE);

   if (total_length_ptr) *total_length_ptr = 0;

   /* if the pen has changed, we need to flush the previous lines and points
    */

   if (first_shape >= 0) {
      /* Draw a shaped line. */

      if (last_shape - first_shape + 3 >=
            RoadMapScreenLinePoints.end - RoadMapScreenLinePoints.cursor) {

         if (last_shape - first_shape + 3 >=
               (RoadMapScreenLinePoints.end - RoadMapScreenLinePoints.data)) {

            roadmap_log (ROADMAP_ERROR,
                  "cannot show all shape points (%d entries needed).",
                  last_shape - first_shape + 3);

            last_shape =
               first_shape
               + (RoadMapScreenLinePoints.data - RoadMapScreenLinePoints.cursor)
               - 3;
         }

         dbg_time_end(DBG_TIME_DRAW_ONE_LINE);
         roadmap_screen_flush_lines ();
         dbg_time_start(DBG_TIME_DRAW_ONE_LINE);
      }

      /* All the shape positions are relative: we need an absolute position
       * to start with.
       */
      last_midposition = *from;
      midposition = *first_shape_pos;

      if (fully_visible) {

         roadmap_math_coordinate (from, &point0);
         roadmap_screen_add_segment_point (&point0, pens, num_pens,
                                           SEGMENT_START);

         for (i = first_shape; i <= last_shape; ++i) {

            if (shape_itr) (*shape_itr) (i, &midposition);
            else roadmap_shape_get_position (i, &midposition);

            roadmap_math_coordinate (&midposition, &point1);
            if ((roadmap_screen_add_segment_point(&point1, pens, num_pens, 0)
                     <= label_max_proj) && total_length_ptr) {

               int length_sq = roadmap_math_screen_distance
                  (&point1, &point0, MATH_DIST_SQUARED);

               /* bad math, but it's for a labelling heuristic anyway */
               *total_length_ptr += length_sq;

               if (length_sq > longest) {
                  longest = length_sq;
                  if (angle) {
                     *angle = roadmap_math_azymuth(&last_midposition, &midposition);
                  }
                  middle->x = (point1.x + point0.x) / 2;
                  middle->y = (point1.y + point0.y) / 2;
               }
            }
            last_midposition = midposition;
            point0 = point1;
         }

         roadmap_math_coordinate (to, &point1);

         if ((roadmap_screen_add_segment_point(&point1, pens, num_pens, SEGMENT_END)
                  <= label_max_proj) && total_length_ptr) {

            int length_sq = roadmap_math_screen_distance
               (&point1, &point0, MATH_DIST_SQUARED);

            /* bad math, but it's for a labelling heuristic anyway */
            *total_length_ptr += length_sq;

            if (length_sq > longest) {
               longest = length_sq;
               if (angle) {
                  *angle = roadmap_math_azymuth(&last_midposition, to);
               }
               middle->x = (point1.x + point0.x) / 2;
               middle->y = (point1.y + point0.y) / 2;
            }
         }
         drawn = 1;

      } else {

         int segment_proj_layer = 0;

         last_point_visible = 0; /* We have drawn nothing yet. */

         for (i = first_shape; i <= last_shape; ++i) {

            if (shape_itr) (*shape_itr) (i, &midposition);
            else roadmap_shape_get_position (i, &midposition);

            if (roadmap_math_get_visible_coordinates
                  (&last_midposition, &midposition, &point0, &point1)) {

               if ((point0.x == point1.x) && (point0.y == point1.y)) {

                  if (last_point_visible) {

                     /* This segment is very short, we can skip it */
                     last_midposition = midposition;

                     continue;
                  }

               }

               /* Show this line: add 2 points if this is the start of a new
                * complete line (i.e. the first visible line), or just add
                * one more point to the current complete line.
                */
               if (!last_point_visible) {
                  roadmap_screen_add_segment_point (&point0, pens, num_pens,
                                                    SEGMENT_START);
                  drawn = 1;
               }

               last_point_visible = roadmap_math_point_is_visible (&midposition);
               if (last_point_visible) {
                  segment_proj_layer =
                     roadmap_screen_add_segment_point (&point1, pens, num_pens, 0);
                  drawn = 1;

               } else {

                  /* Show the previous segment as the end of a complete line.
                   * The remaining part of the shaped line, if any, will be
                   * drawn as a new complete line.
                   */
                  segment_proj_layer =
                     roadmap_screen_add_segment_point (&point1, pens, num_pens,
                                                    SEGMENT_END);
                  drawn = 1;
                  if (last_shape - i + 3 >=
                        RoadMapScreenLinePoints.end - RoadMapScreenLinePoints.cursor) {
                     dbg_time_end(DBG_TIME_DRAW_ONE_LINE);
                     roadmap_screen_flush_lines ();
                     dbg_time_start(DBG_TIME_DRAW_ONE_LINE);
                  }
               }

               if ((segment_proj_layer <= label_max_proj) && total_length_ptr) {

                  int length_sq = roadmap_math_screen_distance
                     (&point1, &point0, MATH_DIST_SQUARED);

                  /* bad math, but it's for a labelling heuristic anyway */
                  *total_length_ptr += length_sq;

                  if (length_sq > longest) {
                     longest = length_sq;
                     if (angle) {
                        *angle = roadmap_math_azymuth(&last_midposition, &midposition);
                     }
                     middle->x = (point1.x + point0.x) / 2;
                     middle->y = (point1.y + point0.y) / 2;
                  }
               }

            }
            last_midposition = midposition; /* The latest position is our new start. */
         }

         if (roadmap_math_get_visible_coordinates
               (&last_midposition, to, &point0, &point1)) {

            if (!last_point_visible) {
               roadmap_screen_add_segment_point (&point0, pens, num_pens,
                                                 SEGMENT_START);
            }

           if ((roadmap_screen_add_segment_point (&point1, pens, num_pens, 0)
                    <= label_max_proj) && total_length_ptr) {

               int length_sq = roadmap_math_screen_distance
                  (&point1, &point0, MATH_DIST_SQUARED);
               if (length_sq) {
                  /* bad math, but it's for a labelling heuristic anyway */
                  *total_length_ptr += length_sq;

                  if (length_sq > longest) {
                     longest = length_sq;
                     if (angle) {
                        *angle = roadmap_math_azymuth(&last_midposition, to);
                     }
                     middle->x = (point1.x + point0.x) / 2;
                     middle->y = (point1.y + point0.y) / 2;
                  }
               }
            }


            /* set last point as visible to force line completion at the next
             * statement.
             */
            last_point_visible = 1;

            drawn = 1;
         }

         if (last_point_visible) {

            /* End the current complete line. */
            roadmap_screen_add_segment_point (NULL, pens, num_pens,
                                              SEGMENT_END);

            drawn = 1;
         }
      }

   } else {
      /* Draw a line with no shape. */

      /* Optimization: do not draw a line that is obviously not visible. */
      if (! fully_visible) {
         if (! roadmap_math_line_is_visible (from, to)) {
            dbg_time_end(DBG_TIME_DRAW_ONE_LINE);
            return 0;
         }
      }

      /* Optimization: adjust the edges of the line so
       * they do not go out of the screen. */
      if (!roadmap_math_get_visible_coordinates (from, to, &point0, &point1)) {
         return 0;
      }

      if ((point0.x == point1.x) && (point0.y == point1.y)) {
         /* draw a point instead of a line */

         roadmap_screen_add_segment_point (&point0, pens, num_pens,
                                           SEGMENT_AS_POINT);

         dbg_time_end(DBG_TIME_DRAW_ONE_LINE);
         return 1;

      }

      if (RoadMapScreenLinePoints.cursor + 2 >= RoadMapScreenLinePoints.end) {
         dbg_time_end(DBG_TIME_DRAW_ONE_LINE);
         roadmap_screen_flush_lines ();
         dbg_time_start(DBG_TIME_DRAW_ONE_LINE);
      }

      roadmap_screen_add_segment_point (&point0, pens, num_pens, SEGMENT_START);
      if ((roadmap_screen_add_segment_point
               (&point1, pens, num_pens, SEGMENT_END) <= label_max_proj &&
               total_length_ptr)) {

         *total_length_ptr = roadmap_math_screen_distance
            (&point1, &point0, MATH_DIST_SQUARED);

         if (angle) {
            *angle = roadmap_math_azymuth(from, to);
         }
         middle->x = (point1.x + point0.x) / 2;
         middle->y = (point1.y + point0.y) / 2;
      }

      drawn = 1;
   }

   dbg_time_end(DBG_TIME_DRAW_ONE_LINE);

   return drawn;
}

#endif
#else

INLINE_DEC int roadmap_screen_draw_one_line_internal (RoadMapPosition *from,
                                   RoadMapPosition *to,
                                   int fully_visible,
                                   RoadMapPosition *first_shape_pos,
                                   int first_shape,
                                   int last_shape,
                                   RoadMapShapeItr shape_itr,
                                   RoadMapPen *pens,
                                   int num_pens,
                                   int label_max_proj,
                                   int *total_length_ptr,
                                   RoadMapGuiPoint *middle,
                                   int *angle) {

   RoadMapGuiPoint point0;
   RoadMapGuiPoint point1;

   /* These are used when the line has a shape: */
   RoadMapPosition midposition;
   RoadMapPosition last_midposition;

   int i;

   int longest = -1;
   int last_point_visible = 0;

   int drawn = 0;

   dbg_time_start(DBG_TIME_DRAW_ONE_LINE);

   if (total_length_ptr) *total_length_ptr = 0;

   if (first_shape >= 0) {
      /* Draw a shaped line. */
      RoadMapPosition label_p1;
      RoadMapPosition label_p2;

      if (last_shape - first_shape + 3 >=
            RoadMapScreenLinePoints.end - RoadMapScreenLinePoints.cursor) {

         if (last_shape - first_shape + 3 >=
               (RoadMapScreenLinePoints.end - RoadMapScreenLinePoints.data)) {

            roadmap_log (ROADMAP_ERROR,
                  "cannot show all shape points (%d entries needed).",
                  last_shape - first_shape + 3);

            last_shape =
               first_shape
               + (RoadMapScreenLinePoints.data - RoadMapScreenLinePoints.cursor)
               - 3;
         }

         dbg_time_end(DBG_TIME_DRAW_ONE_LINE);
         roadmap_screen_flush_lines ();
         dbg_time_start(DBG_TIME_DRAW_ONE_LINE);
      }

      /* All the shape positions are relative: we need an absolute position
       * to start with.
       */
      last_midposition = *from;
      midposition = *first_shape_pos;

      for (i = first_shape; i <= (last_shape + 1); ++i) {

         if (i<= last_shape) {
            if (shape_itr) (*shape_itr) (i, &midposition);
            else roadmap_shape_get_position (i, &midposition);
         } else {
            midposition = *to;
         }

         if (!fully_visible && !roadmap_math_line_is_visible (&last_midposition, &midposition)) {
            last_midposition = midposition;

            if (last_point_visible) {
               roadmap_screen_add_segment_point (NULL, pens, num_pens,
                                                 SEGMENT_END);
               last_point_visible = 0;
               drawn = 1;
            }
            continue;
         }

         if (!last_point_visible) {
            roadmap_math_coordinate (&last_midposition, &point0);
            roadmap_screen_add_segment_point (&point0, pens, num_pens, SEGMENT_START);
            last_point_visible = 1;
         }

         roadmap_math_coordinate (&midposition, &point1);
         roadmap_screen_add_segment_point (&point1, pens, num_pens, i<=last_shape ? 0 : SEGMENT_END);

         if (total_length_ptr) {

            int length_sq = roadmap_math_screen_distance
               (&point1, &point0, MATH_DIST_SQUARED);

            /* bad math, but it's for a labelling heuristic anyway */
            *total_length_ptr += length_sq;

            if (length_sq > longest) {
               longest = length_sq;
               label_p1 = last_midposition;
               label_p2 = midposition;
               middle->x = (point1.x + point0.x) / 2;
               middle->y = (point1.y + point0.y) / 2;
            }
         }

         last_midposition = midposition;
         point0 = point1;

         drawn = 1;
      }

      if (angle && longest) {
         *angle = roadmap_math_azymuth(&last_midposition, &midposition);
      }

   } else {
      /* Draw a line with no shape. */

      /* Optimization: do not draw a line that is obviously not visible. */
      if (! fully_visible) {
         if (! roadmap_math_line_is_visible (from, to)) {
            dbg_time_end(DBG_TIME_DRAW_ONE_LINE);
            return 0;
         }
      }

      roadmap_math_coordinate (from, &point0);
      roadmap_math_coordinate (to, &point1);

      if ((point0.x == point1.x) && (point0.y == point1.y)) {
         /* draw a point instead of a line */

         roadmap_screen_add_segment_point (&point0, pens, num_pens,
                                           SEGMENT_AS_POINT);

         dbg_time_end(DBG_TIME_DRAW_ONE_LINE);
         return 1;

      } else {

         if (total_length_ptr) {

            *total_length_ptr = roadmap_math_screen_distance
                                   (&point1, &point0, MATH_DIST_SQUARED);

            if (angle) {
               *angle = roadmap_math_azymuth(from, to);
            }
            middle->x = (point1.x + point0.x) / 2;
            middle->y = (point1.y + point0.y) / 2;
         }
      }

      if (RoadMapScreenLinePoints.cursor + 2 >= RoadMapScreenLinePoints.end) {
         dbg_time_end(DBG_TIME_DRAW_ONE_LINE);
         roadmap_screen_flush_lines ();
         dbg_time_start(DBG_TIME_DRAW_ONE_LINE);
      }

      roadmap_screen_add_segment_point (&point0, pens, num_pens, SEGMENT_START);
      roadmap_screen_add_segment_point (&point1, pens, num_pens, SEGMENT_END);

      drawn = 1;
   }

   dbg_time_end(DBG_TIME_DRAW_ONE_LINE);
   return drawn;
}

#endif


int roadmap_screen_draw_one_line (RoadMapPosition *from,
                                   RoadMapPosition *to,
                                   int fully_visible,
                                   RoadMapPosition *first_shape_pos,
                                   int first_shape,
                                   int last_shape,
                                   RoadMapShapeItr shape_itr,
                                   RoadMapPen *pens,
                                   int num_pens,
                                   int label_max_proj,
                                   int *total_length_ptr,
                                   RoadMapGuiPoint *middle,
                                   int *angle) {

    int ret;
#ifdef VIEW_MODE_3D_OGL
    int OGL_2Dmode= FALSE;
    int OGL_3Dmode= (RoadMapScreenOGLViewMode== VIEW_MODE_3D);
    roadmap_canvas3_set3DMode(OGL_3Dmode);
#endif// VIEW_MODE_3D_OGL

   ret = roadmap_screen_draw_one_line_internal(from, to, fully_visible, first_shape_pos,
        first_shape, last_shape, shape_itr, pens, num_pens, label_max_proj,
        total_length_ptr, middle, angle, NULL, FALSE);
#ifdef VIEW_MODE_3D_OGL
//    roadmap_canvas3_set3DMode(OGL_2Dmode);
#endif// VIEW_MODE_3D_OGL
    return ret;
}

int roadmap_screen_draw_one_tex_line (RoadMapPosition *from,
                                      RoadMapPosition *to,
                                      int fully_visible,
                                      RoadMapPosition *first_shape_pos,
                                      int first_shape,
                                      int last_shape,
                                      RoadMapShapeItr shape_itr,
                                      RoadMapPen *pens,
                                      int num_pens,
                                      int label_max_proj,
                                      int *total_length_ptr,
                                      RoadMapGuiPoint *middle,
                                      int *angle,
                                      RoadMapImage image,
                                      BOOL opposite) {

   return roadmap_screen_draw_one_line_internal(from, to, fully_visible, first_shape_pos,
                                                first_shape, last_shape, shape_itr, pens, num_pens, label_max_proj,
                                                total_length_ptr, middle, angle, image, opposite);
}


static void roadmap_screen_flush_polygons (RoadMapPen outline) {

   int count = RoadMapScreenObjects.cursor - RoadMapScreenObjects.data;

   if (count == 0) {
       return;
   }

   if (outline) {
      int i;
      int j;
      int pos = 0;


      for (i = 0; i < count; i++) {
         for (j = 1; j < RoadMapScreenObjects.data[i]; j++) {

            RoadMapScreenLinePoints.real[pos] =
               RoadMapScreenLinePoints.real[pos] ||
               RoadMapScreenLinePoints.real[pos + 1] ||
               (RoadMapScreenLinePoints.data[pos].x != RoadMapScreenLinePoints.data[pos + 1].x &&
                RoadMapScreenLinePoints.data[pos].y != RoadMapScreenLinePoints.data[pos + 1].y);
            pos++;
         }
         pos++;
      }
   }


   roadmap_math_rotate_coordinates
       (RoadMapScreenLinePoints.cursor - RoadMapScreenLinePoints.data,
        RoadMapScreenLinePoints.data);

   roadmap_canvas_draw_multiple_polygons
      (count, RoadMapScreenObjects.data, RoadMapScreenLinePoints.data, 1,
       FAST_REFRESH);

   if (outline) {
      int i;
      int j;
      int pos = 0;
      int point_count;
      RoadMapPen old;

      if (0) { // TODO: check why this causes one polygon to be drawn too large while navigating
         roadmap_canvas_set_thickness (1);
         roadmap_canvas_draw_multiple_polygons
            (count, RoadMapScreenObjects.data, RoadMapScreenLinePoints.data, 0,
             FAST_REFRESH);
      }

      old = roadmap_canvas_select_pen (outline);
      roadmap_canvas_set_thickness (1);

      for (i = 0; i < count; i++) {

         point_count = 1;
         for (j = 1; j < RoadMapScreenObjects.data[i]; j++) {

            if (RoadMapScreenLinePoints.real[pos]) {

               point_count++;
            } else if (point_count > 1) {

               roadmap_canvas_draw_multiple_lines (1, &point_count, RoadMapScreenLinePoints.data + pos - point_count + 1, FAST_REFRESH);
               point_count = 1;
            }
            pos++;
         }
         if (point_count > 1) {

            roadmap_canvas_draw_multiple_lines (1, &point_count, RoadMapScreenLinePoints.data + pos - point_count + 1, FAST_REFRESH);
         }
         pos++;
      }


      roadmap_canvas_select_pen (old);
   }

   RoadMapScreenObjects.cursor = RoadMapScreenObjects.data;
   RoadMapScreenLinePoints.cursor  = RoadMapScreenLinePoints.data;
}


static void roadmap_screen_draw_polygons (void) {

   static RoadMapGuiPoint null_point = {0, 0};

   int i;
   int j;
   int size;
   int category;
   int *geo_point;
   RoadMapPosition position;
   RoadMapGuiPoint *graphic_point;
   RoadMapGuiPoint *previous_point;

   RoadMapGuiPoint upper_left;
   RoadMapGuiPoint lower_right;

   RoadMapArea edges;
   RoadMapPen pen = NULL;

   RoadMapScreenLastPen = NULL;

   if (! roadmap_is_visible (ROADMAP_SHOW_AREA)) return;


   for (i = roadmap_polygon_count() - 1; i >= 0; --i) {

      category = roadmap_polygon_category (i);
      pen = roadmap_layer_get_pen (category, 0, 0);

      if (pen == NULL) continue;

      if (RoadMapScreenLastPen != pen) {
         roadmap_screen_flush_polygons (NULL);
         roadmap_canvas_select_pen (pen);
         RoadMapScreenLastPen = pen;
      }

      roadmap_polygon_edges (i, &edges);

      if (! roadmap_math_is_visible (&edges)) continue;

      /* Declutter logic: do not show the polygon when it has been
       * reduced (by the zoom) to a quasi-point.
       */
      position.longitude = edges.west;
      position.latitude  = edges.north;
      roadmap_math_coordinate (&position, &upper_left);

      position.longitude = edges.east;
      position.latitude  = edges.south;
      roadmap_math_coordinate (&position, &lower_right);

      if (abs(upper_left.x - lower_right.x) < 5 &&
          abs(upper_left.y - lower_right.y) < 5) {
         continue;
      }

      size = roadmap_polygon_points
                (i,
                 RoadMapPolygonGeoPoints,
                 RoadMapScreenLinePoints.end
                 - RoadMapScreenLinePoints.cursor - 1);

      if (size <= 0) {

         roadmap_screen_flush_polygons (NULL);

         size = roadmap_polygon_points
                   (i,
                    RoadMapPolygonGeoPoints,
                    RoadMapScreenLinePoints.end
                       - RoadMapScreenLinePoints.cursor - 1);
      }

      geo_point = RoadMapPolygonGeoPoints;
      graphic_point = RoadMapScreenLinePoints.cursor;
      previous_point = &null_point;

      for (j = size; j > 0; --j) {

         roadmap_point_position  (*geo_point, &position);
         roadmap_math_coordinate (&position, graphic_point);
         RoadMapScreenLinePoints.real
            [graphic_point - RoadMapScreenLinePoints.data] = !(POINT_FAKE_FLAG & *geo_point);

         if ((graphic_point->x != previous_point->x) ||
             (graphic_point->y != previous_point->y)) {

            previous_point = graphic_point;
            graphic_point += 1;
         }
         geo_point += 1;
      }

      /* Do not show polygons that have been reduced to a single
       * graphical point because of the zoom factor (natural declutter).
       */
      if (graphic_point != RoadMapScreenLinePoints.cursor) {

         RoadMapGuiPoint center;

         RoadMapScreenLinePoints.real[graphic_point - RoadMapScreenLinePoints.data] = RoadMapScreenLinePoints.real[RoadMapScreenLinePoints.cursor - RoadMapScreenLinePoints.data];

         *(graphic_point++) = *RoadMapScreenLinePoints.cursor;

         *(RoadMapScreenObjects.cursor++) =
             graphic_point - RoadMapScreenLinePoints.cursor;

         RoadMapScreenLinePoints.cursor = graphic_point;

         roadmap_screen_flush_polygons (roadmap_layer_get_pen (category, 1, 0));

         if (!FAST_REFRESH) {
            int size;

            center.x = (upper_left.x + lower_right.x) / 2;
            center.y = (upper_left.y + lower_right.y) / 2;

            size = roadmap_math_screen_distance
                        (&upper_left, &lower_right, MATH_DIST_SQUARED);

            if (size < RoadMapScreenWidth * RoadMapScreenWidth << 2 &&
                  roadmap_layer_label_is_visible(category, 0)) {

               roadmap_label_add_place
                        (&center, 90 - roadmap_math_get_orientation(), size, roadmap_polygon_name (i),category, TRUE);
            }
         }
      }
   }

   roadmap_screen_flush_polygons (NULL);
}


static void roadmap_screen_draw_square_edges (int square) {

   int count;
   RoadMapArea edges;
   RoadMapPosition topleft;
   RoadMapPosition bottomright;
   RoadMapPosition position;

   RoadMapGuiPoint points[6];


   if (! roadmap_is_visible (ROADMAP_SHOW_SQUARE)) return;

   roadmap_square_edges (square, &edges);

   topleft.longitude     = edges.west;
   topleft.latitude      = edges.north;
   bottomright.longitude = edges.east;
   bottomright.latitude  = edges.south;

   roadmap_math_coordinate (&topleft, points);
   points[4] = points[0];

   position.longitude = bottomright.longitude;
   position.latitude  = topleft.latitude;
   roadmap_math_coordinate (&position, points+1);
   points[5] = points[1];

   roadmap_math_coordinate (&bottomright, points+2);

   position.longitude = topleft.longitude;
   position.latitude  = bottomright.latitude;
   roadmap_math_coordinate (&position, points+3);

   roadmap_canvas_select_pen (RoadMapPenEdges);
   count = 6;
   roadmap_math_rotate_coordinates (count, points);
   roadmap_canvas_draw_multiple_lines (1, &count, points,
                                       FAST_REFRESH);
   RoadMapScreenLastPen = NULL;
}

#if 0
struct square_cache_line {
   int cfcc;
   RoadMapPosition from;
   RoadMapPosition to;
   int first_shape;
   int last_shape;
   int contained;
};

struct square_cache {
   int count;
   struct square_cache_line lines[1];
};

static struct square_cache *SquareCache[9000];

static struct square_cache *roadmap_screen_cache_square
              (int square, int *layers, int layer_count) {

   int line;
   int first_line;
   int last_line;
   int first_shape;
   int last_shape;
   int fips = roadmap_locator_active ();

   int i;

   struct square_cache *cache = NULL;
   RoadMapArea square_edges;

   roadmap_square_edges (square, &square_edges);

   roadmap_square_set_current (square);

   for (i = 0; i < layer_count; ++i) {

      int cfcc = layers[i];

      if ((cfcc < ROADMAP_ROAD_FIRST) || (cfcc > ROADMAP_ROAD_LAST)) {
         continue;
      }

      if (roadmap_line_in_square (square, cfcc, &first_line, &last_line) > 0) {
         int has_shapes;

         if (!cache) {
            cache = malloc(sizeof(*cache) + sizeof(struct square_cache_line) * 2000);
            cache->count = 0;
         }

         if (roadmap_square_has_shapes (square)) {
            has_shapes = 1;
         } else {
            has_shapes = 0;
            first_shape = last_shape = -1;
         }

         for (line = first_line; line <= last_line; ++line) {

            /* A plugin may override a line: it can change the pen or
             * decide not to draw the line.
             */

            struct square_cache_line *cache_entry = cache->lines + cache->count;

            if (!roadmap_plugin_override_line (line, cfcc, fips)) {


               if (has_shapes) {

                  roadmap_line_shapes (line, square, &first_shape, &last_shape);
               }

               cache_entry->contained = 1;

               roadmap_line_from (line, &cache_entry->from);
               roadmap_line_to (line, &cache_entry->to);

               if (first_shape != -1) {
                  int j;
                  RoadMapPosition shape_pos;
                  shape_pos = cache_entry->from;
                  for (j = first_shape; j <= last_shape; ++j) {

                     roadmap_shape_get_position (j, &shape_pos);

                     if ((shape_pos.longitude > square_edges.east) ||
                           (shape_pos.longitude < square_edges.west) ||
                           (shape_pos.latitude  > square_edges.north) ||
                           (shape_pos.latitude  < square_edges.south)) {

                        cache_entry->contained = 0;
                        break;
                     }
                  }
               }

               cache_entry->first_shape = first_shape;
               cache_entry->last_shape = last_shape;
               cache_entry->cfcc = cfcc;
               cache->count++;
            }
         }
      }
   }

   roadmap_log_pop ();
   if (!cache) cache = -1;
   return cache;
}
#endif

//#define DEBUG_TIME
#ifdef J2ME
#include <java/lang.h>
#define roadmap_plugin_override_line(x,y,z) 0
#define roadmap_plugin_override_pen(x, y, z, a, b) 0
#endif


static void
roadmap_screen_draw_points (RoadMapGuiPoint *point0,
                            RoadMapGuiPoint *point1) {
   RoadMapGuiPoint from;
   int width;

   double delta_x = point1->x - point0->x;
   double delta_y = point1->y - point0->y;
   double ll = (1.0 * (abs((int)(delta_x)) + abs((int)(delta_y))) / 7);

   if (ll >= 1) {

      static int i=0;

      double step_x = delta_x / ll;
      double step_y = delta_y / ll;
      double x = point0->x + step_x;
      double y = point0->y + step_y;

      while (abs((int)(x + step_x - point0->x)) < abs((int)(delta_x))) {
         i++;

         from.x = (int)x;
         from.y = (int)y;
		 roadmap_math_rotate_coordinates (1, &from);

		 width = ADJ_SCALE(2);
   	     roadmap_canvas_draw_multiple_circles(1 , &from, &width, 5,1);

         x += step_x*3;
         y += step_y*3;
      }
   }
}

#define NOPH_System_currentTimeMillis() roadmap_time_get_millis()

void roadmap_screen_draw_line_points (RoadMapPosition *from,
                                         RoadMapPosition *to,
                                         RoadMapPosition *first_shape_pos,
                                         int first_shape,
                                         int last_shape,
                                         RoadMapShapeItr shape_itr,
                                         const char *color) {

   static RoadMapPen points_pen = NULL;
   RoadMapPen pen;
   RoadMapGuiPoint point0;
   RoadMapGuiPoint point1;

   /* These are used when the line has a shape: */
   RoadMapPosition midposition;
   RoadMapPosition last_midposition;

   int i;


  pen = roadmap_layer_get_pen (ROADMAP_ROAD_STREET,0 ,0);
  if (pen == NULL) {
  	return;
  }

  roadmap_screen_flush_lines ();

   if (points_pen == NULL) {
      points_pen = roadmap_canvas_create_pen ("points_mark");
      roadmap_canvas_set_foreground (color);
   } else {
     roadmap_canvas_select_pen (points_pen);
     roadmap_canvas_set_foreground (color);
   }

   if (first_shape >= 0) {

      last_midposition = *from;
      midposition = *first_shape_pos;

      for (i = first_shape; i <= last_shape; ++i) {

         if (!shape_itr) roadmap_shape_get_position(i, &midposition);
         else (*shape_itr) (i, &midposition);

         if (roadmap_math_line_is_visible (&last_midposition, &midposition) &&
            roadmap_math_get_visible_coordinates
                        (&last_midposition, &midposition, &point0, &point1)) {
            roadmap_screen_draw_points (&point0, &point1);

         }
         last_midposition = midposition;
      }

      if (roadmap_math_get_visible_coordinates
                        (&last_midposition, to, &point0, &point1)) {

         roadmap_screen_draw_points (&point0, &point1);

      }

   } else {
      if (roadmap_math_get_visible_coordinates (from, to, &point0, &point1)) {
         roadmap_screen_draw_points (&point0, &point1);
      }
   }

   roadmap_screen_flush_lines ();

   RoadMapScreenLastPen = NULL;
}

INLINE_DEC int roadmap_screen_draw_square
              (int square, int cfcc, int fully_visible, int pen_type) {

   int line;
   int first_line;
   int last_line;
   int first_shape;
   int last_shape;
   RoadMapPen layer_pens[LAYER_PROJ_AREAS];
   RoadMapPen layer_pens2[LAYER_PROJ_AREAS];
#ifdef J2ME
#define active_fips 77001
#else
   int active_fips = roadmap_locator_active ();
#endif
   int total_length;
   int label_max_proj = -1;
   int *total_length_ptr = 0;
   int angle = 90;
   int *angle_ptr = 0;
   RoadMapGuiPoint seg_middle;
   RoadMapGuiPoint loweredge;
   int cutoff_dist = 0;

   int drawn = 0;
   int i;

   char *color;

#ifdef DEBUG_TIME
   int start_time;
   int end_time;
#endif
//printf("drawing square %d\n", square);
   roadmap_log_push ("roadmap_screen_draw_square");

#ifdef DEBUG_TIME
    start_time = NOPH_System_currentTimeMillis();
#endif

   for (i = 0; i < LAYER_PROJ_AREAS; i++) {
      layer_pens[i] = roadmap_layer_get_pen (cfcc, pen_type, i);
   }

   for (i = 0; i < LAYER_PROJ_AREAS; i++) {
      layer_pens2[i] = roadmap_layer_get_pen (cfcc, 2, i);
      if (layer_pens2[i] == NULL)
      	layer_pens2[i] = roadmap_layer_get_pen (cfcc, 1, i);
   }
#ifdef DEBUG_TIME
    end_time = NOPH_System_currentTimeMillis();
    printf ("roadmap_screen_square after projs %d ms\n", end_time - start_time);
    start_time = end_time;
#endif

   if (layer_pens[0] == NULL) {
      roadmap_log_pop ();
      return 0;
   }

#ifndef J2ME
   if ((pen_type == 0) &&       /* we do labels only for the first pen */
         !FAST_REFRESH &&
         RoadMapScreenLabels) {

      label_max_proj =
         roadmap_view_show_labels(cfcc, layer_pens, LAYER_PROJ_AREAS);

      if (label_max_proj >= 0) {
         int should_arrange_labels = 0;
#ifndef OGL_TILE
         should_arrange_labels = isViewModeAny3D();
#endif

         total_length_ptr = &total_length;
         if (should_arrange_labels) {
            /* arrange to not do labels further than 3/4 up the screen */
            RoadMapGuiPoint label_cutoff;
            label_cutoff.y = roadmap_canvas_height() / 4;
            label_cutoff.x = roadmap_canvas_width() / 2;
            loweredge.x = roadmap_canvas_width() / 2;
            loweredge.y = roadmap_canvas_height();
            rds_unproject(&label_cutoff);
            rds_unproject(&loweredge);
            cutoff_dist = roadmap_math_screen_distance
               (&label_cutoff, &loweredge, MATH_DIST_SQUARED);
         } else {
#if !defined(J2ME) || defined(RIMAPI)
            angle_ptr = &angle;
#endif
         }
      }
   }
#endif
   /* Draw each line that belongs to this square. */

   if (roadmap_skin_state() == 0)
      color = "#55555555";
   else
      color = "#ffffff55";

#ifdef DEBUG_TIME
    end_time = NOPH_System_currentTimeMillis();
    printf ("roadmap_screen_square b4 search lines projs %d ms\n", end_time - start_time);
    start_time = end_time;
#endif
   if (roadmap_line_in_square (square, cfcc, &first_line, &last_line) > 0) {
      int has_shapes;

      if (roadmap_square_has_shapes (square)) {
         has_shapes = 1;
      } else {
         has_shapes = 0;
         first_shape = last_shape = -1;
      }

#ifdef DEBUG_TIME
    end_time = NOPH_System_currentTimeMillis();
    printf ("roadmap_screen_square b4 itr of:%d %d ms\n", (last_line - first_line), end_time - start_time);
    start_time = end_time;
#endif
      for (line = first_line; line <= last_line; ++line) {

         /* A plugin may override a line: it can change the pen or
          * decide not to draw the line.
          */

         if (!roadmap_plugin_override_line (line, cfcc, active_fips)) {
            RoadMapPosition from;
            RoadMapPosition to;
            RoadMapPen override_pen;


            if (has_shapes) {

               roadmap_line_shapes (line, &first_shape, &last_shape);
            }

            roadmap_line_from (line, &from);
            roadmap_line_to (line, &to);

            /* Check if the plugin wants to override the pen. */
            if (/*FAST_REFRESH == 0 &&*/
            	  roadmap_plugin_override_pen
                     (line, cfcc, active_fips, pen_type, &override_pen)) {

               if (override_pen == NULL) continue;
               roadmap_screen_draw_one_line_internal
                  (&from, &to, fully_visible, &from, first_shape, last_shape,
                   NULL, &override_pen, 1, label_max_proj,
                   total_length_ptr, &seg_middle, angle_ptr, NULL, FALSE);
            } else {
            	int low_weight;
            	int scale = roadmap_square_get_screen_scale ();
            	low_weight = roadmap_line_route_is_low_weight (line);
            	if ((cfcc < ROADMAP_ROAD_PEDESTRIAN) && (pen_type == 1) && low_weight && editor_screen_gray_scale() && !scale) {
       					roadmap_screen_draw_one_line_internal
                  			(&from, &to, fully_visible, &from, first_shape, last_shape,
                   				NULL, layer_pens2, LAYER_PROJ_AREAS,
                   				label_max_proj, total_length_ptr, &seg_middle, angle_ptr, NULL, FALSE);
						roadmap_screen_draw_line_points(&from, &to, &from, first_shape, last_shape,
                   				NULL, "#b2bfdc");
                }
                else{
                  int width = roadmap_canvas_get_thickness(layer_pens[0]);
                  int direction = roadmap_line_route_get_direction (line, ROUTE_CAR_ALLOWED);
     					roadmap_screen_draw_one_line_internal
                  			(&from, &to, fully_visible, &from, first_shape, last_shape,
                   				NULL, layer_pens, LAYER_PROJ_AREAS,
                   				label_max_proj, total_length_ptr, &seg_middle, angle_ptr, NULL, FALSE);

     					if (!FAST_REFRESH &&
     					    RoadMapScreenViewMode == VIEW_MODE_2D &&
                      RoadMapScreenOGLViewMode == VIEW_MODE_2D &&
     					      width >= 4 &&
     					      roadmap_math_get_zoom() < 15 &&
     					      (direction == ROUTE_DIRECTION_WITH_LINE || direction == ROUTE_DIRECTION_AGAINST_LINE))
     					   roadmap_screen_draw_line_direction (   &from,
                                                            &to,
                                                            &from,
                                                            first_shape,
                                                            last_shape,
     					                                          NULL,
     					                                          width,
     					                                          direction,
     					                                          color);

                }
            }

            if (total_length_ptr && total_length && (cutoff_dist == 0 ||
                    cutoff_dist > roadmap_math_screen_distance
                            (&seg_middle, &loweredge, MATH_DIST_SQUARED)) ) {
               PluginLine l;
               RoadMapPen pen;
               l.plugin_id = ROADMAP_PLUGIN_ID;
               l.line_id = line;
               l.cfcc = cfcc;
               l.square = square;
               l.fips = active_fips;
               if (cfcc > ROADMAP_ROAD_STREET)
                  pen = roadmap_layer_get_pen (cfcc, 0, 0);
               else
                  pen = roadmap_layer_get_pen (cfcc, 1, 0);
               if ((pen != NULL) && (roadmap_canvas_get_thickness(pen) > ADJ_SCALE(1)) &&
                     roadmap_layer_label_is_visible(cfcc, 0) &&
                     cfcc != ROADMAP_ROAD_RAMP)
                  roadmap_label_add (&seg_middle, angle, total_length, &l);
            }

            drawn += 1;
         }
      }
#ifdef DEBUG_TIME
    end_time = NOPH_System_currentTimeMillis();
    printf ("roadmap_screen_square after itr %d ms\n", end_time - start_time);
    start_time = end_time;
#endif
   }

#ifdef DEBUG_TIME
    end_time = NOPH_System_currentTimeMillis();
    printf ("roadmap_screen_square before sq2 %d ms\n", end_time - start_time);
    start_time = end_time;
#endif


#ifdef DEBUG_TIME
    end_time = NOPH_System_currentTimeMillis();
    printf ("roadmap_screen_square end %d ms\n", end_time - start_time);
    start_time = end_time;
#endif
   roadmap_log_pop ();
//printf("drawn %d lines\n", drawn);
   return drawn;
}


static void roadmap_screen_draw_object (const char *name,
                const char *sprite,
                const char *image_name,
                const RoadMapGpsPosition *gps_position,
                const RoadMapGuiPoint    *offset,
                BOOL is_visible,
                int scale,
                int opacity,
                int scale_y,
                const char *id,
                const char *text) {

   RoadMapPosition position;
   //RoadMapGuiPoint screen_point;
   RoadMapGuiPoint top_left_pos;
   RoadMapImage image;

   if ((sprite == NULL) && (image_name == NULL))return; /* Not a visible object. */

   position.latitude = gps_position->latitude;
   position.longitude = gps_position->longitude;

   if (is_visible) {
      roadmap_math_coordinate (&position, &top_left_pos);
      roadmap_math_rotate_project_coordinate(&top_left_pos);

      top_left_pos.y = (top_left_pos.y*scale_y)/100;

      if (image_name != NULL) {
#ifdef VIEW_MODE_3D_OGL
         if (roadmap_screen_get_view_mode() == VIEW_MODE_3D && (scale_y == 100) &&
             roadmap_canvas3_get_angle() > 0.8) {
            if (1.0 * top_left_pos.y / roadmap_canvas_height() < 0.2) {
               scale = scale * 0.5;
            } else if (1.0 * top_left_pos.y / roadmap_canvas_height() < 0.5) {
               scale = scale * 0.75;
            }

         }
#endif

         image =  (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN, image_name);
         if (image) {
#ifdef OPENGL
            RoadMapGuiPoint bottom_right_pos;
            top_left_pos.x -= (roadmap_canvas_image_width(image)*scale)/(2 * 100)  - offset->x*scale/100;
            top_left_pos.y -= (roadmap_canvas_image_height(image)*scale)/(2 * 100) - offset->y*scale/100;
            bottom_right_pos.x = top_left_pos.x + (roadmap_canvas_image_width(image)*scale)/100;
            bottom_right_pos.y = top_left_pos.y + (roadmap_canvas_image_height(image)*scale)/100;


            roadmap_canvas_draw_image_scaled(image, &top_left_pos, &bottom_right_pos, opacity, IMAGE_NORMAL);
#else
            top_left_pos.x -= roadmap_canvas_image_width(image)/2 - offset->x;
            top_left_pos.y -= roadmap_canvas_image_height(image)/2 - offset->y;
            roadmap_canvas_draw_image (image, &top_left_pos,  opacity, IMAGE_NORMAL);
#endif
            if (text != NULL){
               top_left_pos.x += roadmap_canvas_image_width(image)/2;
               top_left_pos.y += roadmap_canvas_image_height(image)/2;
               roadmap_canvas_create_pen("ObjectText");
               roadmap_canvas_set_foreground("#ffffff");
#ifdef OPENGL
               roadmap_canvas_set_background("#000000");
#endif
               roadmap_canvas_draw_formated_string_size(&top_left_pos, ROADMAP_CANVAS_CENTERMIDDLE, 12, FONT_TYPE_BOLD|FONT_TYPE_OUTLINE, text);
            }
         }
      else {
            roadmap_sprite_draw (sprite, &top_left_pos, gps_position->steering);
      }
   }
}
}

static void roadmap_screen_draw_alerts (void) {
   //RoadMapArea screen;
   int count;
   int i;
   int steering;
   RoadMapPosition pos;
   RoadMapGuiPoint top_left_pos;
#ifdef OPENGL
   RoadMapGuiPoint bottom_right_pos;
#endif
   RoadMapGuiPoint icon_screen_point;
   RoadMapPen pen;
   const char* icon;
   RoadMapImage image;
   int alertId;
   int scale = 100;
   RoadMapGuiPoint offset;

   count = roadmap_alert_count ();

   //roadmap_math_screen_edges (&screen);

   for (i=0; i<count; i++) {
	  if (!(roadmap_map_settings_isShowSpeedCams())){
	  	 	continue;
	  }
      roadmap_alert_get_position (i, &pos, &steering);
      if (!roadmap_math_point_is_visible (&pos)) continue;

      roadmap_math_coordinate (&pos, &top_left_pos);
      roadmap_math_rotate_project_coordinate(&top_left_pos);

#ifdef VIEW_MODE_3D_OGL
      if (roadmap_screen_get_view_mode() == VIEW_MODE_3D &&
          roadmap_canvas3_get_angle() > 0.8) {

         if (1.0 * top_left_pos.y / roadmap_canvas_height() < 0.1) {
            continue;
         } else if (1.0 * top_left_pos.y / roadmap_canvas_height() < 0.2) {
            scale = 50;
         } else if (1.0 * top_left_pos.y / roadmap_canvas_height() < 0.5) {
            scale = 75;
         } else {
            scale = 100;
         }
      }
#endif

      pen = roadmap_layer_get_pen (ROADMAP_ROAD_MAIN,0 ,0);
      if (pen != NULL) {
      	zoom_t zoom = roadmap_math_get_zoom();
         // display the icon only at street level zoom
         pen = roadmap_layer_get_pen (ROADMAP_ROAD_STREET,0 ,0);
         alertId = roadmap_alert_get_id(i);
         icon = roadmap_alert_get_map_icon(alertId);
         if (icon != NULL) {
	         if (zoom <= 22 ) {
               image =  (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN, icon);
               offset.x = ADJ_SCALE(-12);
               offset.y = -roadmap_canvas_image_height (image)  + ADJ_SCALE(2);

               if (scale == 100)
                  roadmap_sprite_draw ("AlertDirection", &top_left_pos, steering);

#ifdef OPENGL
               top_left_pos.x += offset.x*scale/100;
               top_left_pos.y += offset.y*scale/100;
               bottom_right_pos.x = top_left_pos.x + (roadmap_canvas_image_width(image)*scale)/100;
               bottom_right_pos.y = top_left_pos.y + (roadmap_canvas_image_height(image)*scale)/100;

               roadmap_canvas_draw_image_scaled(image, &top_left_pos, &bottom_right_pos, 0, IMAGE_NORMAL);
#else
       	      icon_screen_point.x = top_left_pos.x - 10; //roadmap_canvas_image_width(image) ;
               icon_screen_point.y = top_left_pos.y - roadmap_canvas_image_height (image)  + 2;
               roadmap_sprite_draw ("AlertDirection", &top_left_pos, steering);
               roadmap_canvas_draw_image (image, &icon_screen_point,
                      0, IMAGE_NORMAL);
#endif
            } else if (roadmap_math_get_scale(roadmap_screen_height()) <= 2000) {
               image =  (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN, "red_marker_small");
               if (image){
#ifdef OPENGL
                  offset.x = ADJ_SCALE(-4);
                  offset.y = -roadmap_canvas_image_height (image) + ADJ_SCALE(1);
                  top_left_pos.x += offset.x*scale/100;
                  top_left_pos.y += offset.y*scale/100;
                  bottom_right_pos.x = top_left_pos.x + (roadmap_canvas_image_width(image)*scale)/100;
                  bottom_right_pos.y = top_left_pos.y + (roadmap_canvas_image_height(image)*scale)/100;

                  roadmap_canvas_draw_image_scaled(image, &top_left_pos, &bottom_right_pos, 0, IMAGE_NORMAL);
#else
          	  icon_screen_point.x = top_left_pos.x - 3;
               	  icon_screen_point.y = top_left_pos.y - roadmap_canvas_image_height (image)  + 5;
               	  roadmap_canvas_draw_image (image, &icon_screen_point,
                     			     0, IMAGE_NORMAL);
#endif
               } else {
                  roadmap_sprite_draw ("alert", &top_left_pos, steering);
               }
         	}
      	}
      }
   }
}


static void draw_real_time_traffic_speed_signs(void){
   int i;
   int  NumReports;
   static RoadMapImage images[4] = {0,0,0,0};
   static const char *sign_name[4] = {"yellow_tag", "orange_tag", "red_tag", "red_tag"};
   int scale = 100;
   static RoadMapPen speed_pen = NULL;

#ifndef J2ME
   if (!isDisplayingTrafficInfoOn())
      return;
#endif
   if (!(roadmap_map_settings_color_roads())) // see if user chose not to display color on map
   	 return;
   NumReports = RTTrafficInfo_Count();

   if (speed_pen == NULL) {
      speed_pen = roadmap_canvas_create_pen("SpeedSign.Label");
      roadmap_canvas_set_foreground("#000000");
   }

   for (i=0; i< NumReports; i++){
      RoadMapPosition pos;
      RoadMapGuiPoint screen_point;
      RoadMapPen pen;

      int j;
      BOOL found = FALSE;
      RTTrafficInfo *TrafficRecord;



      TrafficRecord = RTTrafficInfo_Get(i);
      if (images[TrafficRecord->iType] == NULL)
         images[TrafficRecord->iType] =  (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN|RES_NOCACHE,sign_name[TrafficRecord->iType]);

     	if (!roadmap_math_is_visible (&TrafficRecord->boundingBox))
     		continue;

      for (j= 0; j <TrafficRecord->iNumGeometryPoints; j++){
         if (!roadmap_math_point_is_visible (&TrafficRecord->geometry[j])) continue;
         pos = TrafficRecord->geometry[j];
         found = TRUE;
         break;
      }

      if (!found)
         continue;

      roadmap_math_coordinate (&pos, &screen_point);
      roadmap_math_rotate_project_coordinate(&screen_point);

#ifdef VIEW_MODE_3D_OGL
      if (roadmap_screen_get_view_mode() == VIEW_MODE_3D &&
          roadmap_canvas3_get_angle() > 0.8) {

         if (1.0 * screen_point.y / roadmap_canvas_height() < 0.2) {
            continue;
         } else if (1.0 * screen_point.y / roadmap_canvas_height() < 0.5) {
            scale = 65;
         } else {
            scale = 100;
         }
      }
#endif

      pen = roadmap_layer_get_pen (ROADMAP_ROAD_STREET,0 ,0);
      if (pen != NULL) {
         char text[20];
         RoadMapGuiPoint icon_screen_point;
         RoadMapGuiPoint top_left_pos, bottom_right_pos;
         RoadMapGuiPoint offset;

         offset.x = -roadmap_canvas_image_width(images[TrafficRecord->iType])/2  ;
         offset.y = -roadmap_canvas_image_height (images[TrafficRecord->iType]) ;


         icon_screen_point.x = screen_point.x + offset.x*scale/100;
         icon_screen_point.y = screen_point.y + offset.y*scale/100;
         bottom_right_pos.x = icon_screen_point.x + (roadmap_canvas_image_width(images[TrafficRecord->iType])*scale)/100;
         bottom_right_pos.y = icon_screen_point.y + (roadmap_canvas_image_height(images[TrafficRecord->iType])*scale)/100;

#ifdef OPENGL
         roadmap_canvas_draw_image_scaled(images[TrafficRecord->iType], &icon_screen_point, &bottom_right_pos, 225, IMAGE_NORMAL);
#else
         icon_screen_point.x = screen_point.x - roadmap_canvas_image_width(images[TrafficRecord->iType])/2  ;
         icon_screen_point.y = screen_point.y - roadmap_canvas_image_height (images[TrafficRecord->iType]) ;
         roadmap_canvas_draw_image (images[TrafficRecord->iType], &icon_screen_point,  225, IMAGE_NORMAL);
#endif


         sprintf(text,"%d",TrafficRecord->iSpeed);

         offset.x = roadmap_canvas_image_width(images[TrafficRecord->iType])/2;
         offset.y = ADJ_SCALE(2);

         top_left_pos.x = icon_screen_point.x + (offset.x*scale)/100;
         top_left_pos.y = icon_screen_point.y + (offset.y*scale)/100;

         roadmap_canvas_select_pen(speed_pen);
#ifdef OPENGL
         roadmap_canvas_draw_string_size(&top_left_pos, ROADMAP_CANVAS_TOPMIDDLE, (13*scale)/100, text);
#else
	 roadmap_canvas_draw_string_size(&top_left_pos, ROADMAP_CANVAS_BOTTOMRIGHT, 14, text);
#endif

          offset.x = roadmap_canvas_image_width(images[TrafficRecord->iType])/2;
          offset.y = ADJ_SCALE(16);

         top_left_pos.x = icon_screen_point.x + offset.x*scale/100;
         top_left_pos.y = icon_screen_point.y + offset.y*scale/100;

         roadmap_canvas_draw_string_size(&top_left_pos, ROADMAP_CANVAS_TOPMIDDLE, (8*scale)/100, roadmap_lang_get(roadmap_math_speed_unit()));
      }
   }
}


static void fast_refresh_start (void) {
   //roadmap_layer_adjust();

   // This is a hack to recompute the scale
   //set_horizon();
}


static void fast_refresh_end (void) {
   //roadmap_layer_adjust();

   // This is a hack to recompute the scale
   //set_horizon();
}


INLINE_DEC int roadmap_screen_repaint_square (int square, int pen_type,
                                          int layer_count, int *layers) {

   int i;

   RoadMapArea edges;

   int drawn = 0;
   int category;
   int fully_visible;

   dbg_time_start(DBG_TIME_DRAW_SQUARE);

   if (pen_type == 0) roadmap_screen_draw_square_edges (square);

#if 0
   if (!SquareCache[square]) {
      SquareCache[square] = roadmap_screen_cache_square (square, layers, layer_count);
   }

   if (SquareCache[square] == -1) return 0;
#endif

   roadmap_log_push ("roadmap_screen_repaint_square");

   roadmap_square_edges (square, &edges);

   switch (roadmap_math_is_visible (&edges)) {
   case 0:
      roadmap_log_pop ();
      return 0;
   case 1:
      fully_visible = 1;
      break;
   default:
      fully_visible = 0;
   }

   RoadMapScreenLastPen = NULL;

   for (i = layer_count - 1; i >= 0; --i) {

        category = layers[i];

        drawn += roadmap_screen_draw_square
                    (square, category, fully_visible, pen_type);

   }

   dbg_time_end(DBG_TIME_DRAW_SQUARE);

   roadmap_screen_flush_lines();
   roadmap_screen_flush_points();

   roadmap_log_pop ();

   return drawn;
}

static RoadMapImage sky_image()
{
	RoadMapImage retImage = NULL;
   char  image_name[128];
	if (roadmap_skin_state() == 0)
	{
      snprintf(image_name, sizeof(image_name), "3D-Sky");
	}
	else
	{
      snprintf(image_name, sizeof(image_name), "3D-SkyNight");
	}

#ifdef IPHONE_NATIVE
   if (roadmap_main_get_platform() == ROADMAP_MAIN_PLATFORM_IPAD)
      strcat(image_name, "_long");
#endif //IPHONE_NATIVE

	retImage= (RoadMapImage) roadmap_res_get( RES_BITMAP, RES_SKIN, image_name );

	return retImage;
}

static int sky_image_offset () {
#if defined (OPENGL) && defined (VIEW_MODE_3D_OGL)
   int h= roadmap_canvas_image_height(sky_image());
   float angle = roadmap_canvas3_get_angle();
   int offset;
   //printf("angle: %f\n", angle);
   offset = h * (angle - 0.75) / (1.08 - 0.75) - h;
   if (h + offset > h)
      offset = 0;
   if (h + offset < 0)
      offset = -h;

   //printf("offset: %d h: %d\n", offset, h);

   return offset;
#else
   return 0;
#endif

}

#ifdef OPENGL
static int sky_image_top_height() {
	int h= roadmap_canvas_image_height(sky_image());

   h += sky_image_offset();

   if (h < 20)
      return 1;
   
#ifdef IPHONE_NATIVE
   if (roadmap_main_get_platform() == ROADMAP_MAIN_PLATFORM_IPAD)
      return (int)floor(h*.75);
#endif

   if (!is_screen_wide())
      return (int)floor(h*.4);
   else
      return 1;
}
#endif

void roadmap_screen_draw_sky(void){
   RoadMapImage SkyImage = NULL;
   RoadMapGuiPoint screen_point;
   int image_width;

#ifndef TOUCH_SCREEN
   return;
#endif

#ifndef IPHONE_NATIVE
   if (!isViewModeAny3D() ||
       (is_screen_wide())) // Do not draw sky in wide screen)
      return;
#else
   if (!isViewModeAny3D() ||
       (is_screen_wide() && roadmap_main_get_platform() != ROADMAP_MAIN_PLATFORM_IPAD)) // Do not draw sky in wide screen)
      return;
#endif //IPHONE_NATIVE

   SkyImage= sky_image();

   screen_point.y = sky_image_offset();
   if (!roadmap_map_settings_isShowTopBarOnTap())
      screen_point.y += roadmap_bar_top_height();

   if (SkyImage) {
      image_width = roadmap_canvas_image_width(SkyImage);
#ifndef IPHONE_NATIVE
      screen_point.x = image_width * roadmap_math_get_orientation() / 360;
      roadmap_canvas_draw_image (SkyImage, &screen_point,  0, IMAGE_NORMAL);
      if (roadmap_math_get_orientation() != 0) {
         screen_point.x -= image_width;
         roadmap_canvas_draw_image (SkyImage, &screen_point,  0, IMAGE_NORMAL);
      }
#else
      if (image_width >= roadmap_canvas_width()) {
         screen_point.x = image_width * roadmap_math_get_orientation() / 360;
         roadmap_canvas_draw_image (SkyImage, &screen_point,  0, IMAGE_NORMAL);
         if (roadmap_math_get_orientation() != 0) {
            screen_point.x -= image_width;
            roadmap_canvas_draw_image (SkyImage, &screen_point,  0, IMAGE_NORMAL);
         }
      } else {
         RoadMapGuiPoint bottom_right;
         image_width = roadmap_canvas_width();
         screen_point.x = image_width * roadmap_math_get_orientation() / 360;
         bottom_right.x = screen_point.x + image_width;
         bottom_right.y = screen_point.y + roadmap_canvas_image_height(SkyImage);
         roadmap_canvas_draw_image_scaled(SkyImage, &screen_point, &bottom_right, 0, IMAGE_NORMAL);
         if (roadmap_math_get_orientation() != 0) {
            screen_point.x -= image_width;
            bottom_right.x -= image_width;
            roadmap_canvas_draw_image_scaled(SkyImage, &screen_point, &bottom_right, 0, IMAGE_NORMAL);
         }
         
      }
#endif //IPHONE_NATIVE
   }

}

static void roadmap_screen_draw_glow (void) {
#ifdef OPENGL
   RoadMapGuiPoint pos;
   RoadMapImage image;
   float scale = 1.0f;
   if (RoadMapScreenGlowScale > -1) {
      roadmap_math_coordinate (&RoadMapScreenGlowPos, &pos);
      roadmap_math_rotate_project_coordinate(&pos);
#ifdef VIEW_MODE_3D_OGL
      if (roadmap_screen_get_view_mode() == VIEW_MODE_3D&&
          roadmap_canvas3_get_angle() > 0.8) {
         if (1.0 * pos.y / roadmap_canvas_height() < 0.2) {
            scale = 0.5;
         } else if (1.0 * pos.y / roadmap_canvas_height() < 0.5) {
            scale = 0.75;
         }

      }
#endif

      image =  (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN, "object_glow");
      if (image) {
         RoadMapGuiPoint bottom_right_pos, top_left_pos;
         int glow = RoadMapScreenGlowScale;
         top_left_pos.x = pos.x - (roadmap_canvas_image_width(image) * scale*glow)/(2 * 100)  - RoadMapScreenGlowOffset.x*scale;
         top_left_pos.y = pos.y -  (roadmap_canvas_image_height(image) * scale*glow)/(2 * 100) - RoadMapScreenGlowOffset.y*scale;
         bottom_right_pos.x = top_left_pos.x + (roadmap_canvas_image_width(image) * scale*glow)/100;
         bottom_right_pos.y = top_left_pos.y + (roadmap_canvas_image_height(image) * scale*glow)/100;
         roadmap_canvas_draw_image_scaled(image, &top_left_pos, &bottom_right_pos, 255*(101 - glow)/100, IMAGE_NORMAL);

         glow += 33;
         if (glow > 100)
            glow -= 100;
         top_left_pos.x = pos.x - (roadmap_canvas_image_width(image) * scale*glow)/(2 * 100)  - RoadMapScreenGlowOffset.x*scale;
         top_left_pos.y = pos.y -  (roadmap_canvas_image_height(image) * scale*glow)/(2 * 100) - RoadMapScreenGlowOffset.y*scale;
         bottom_right_pos.x = top_left_pos.x + (roadmap_canvas_image_width(image) * scale*glow)/100;
         bottom_right_pos.y = top_left_pos.y + (roadmap_canvas_image_height(image) * scale*glow)/100;
         roadmap_canvas_draw_image_scaled(image, &top_left_pos, &bottom_right_pos, 255*(101 - glow)/100, IMAGE_NORMAL);

         glow += 33;
         if (glow > 100)
            glow -= 100;
         top_left_pos.x = pos.x - (roadmap_canvas_image_width(image) * scale*glow)/(2 * 100)  - RoadMapScreenGlowOffset.x*scale;
         top_left_pos.y = pos.y -  (roadmap_canvas_image_height(image) * scale*glow)/(2 * 100) - RoadMapScreenGlowOffset.y*scale;
         bottom_right_pos.x = top_left_pos.x + (roadmap_canvas_image_width(image) * scale*glow)/100;
         bottom_right_pos.y = top_left_pos.y + (roadmap_canvas_image_height(image) * scale*glow)/100;
         roadmap_canvas_draw_image_scaled(image, &top_left_pos, &bottom_right_pos, 255*(101 - glow)/100, IMAGE_NORMAL);
      }
   }
#endif
}
void map_credit_display(void){
   const char* map_credit = roadmap_start_get_map_credit();
   static RoadMapPen MapCreditPen = NULL;

   if (map_credit && *map_credit){
      RoadMapGuiPoint pos;

      pos.x = ADJ_SCALE(5);
      pos.y = roadmap_canvas_height() - roadmap_bar_bottom_height() -ADJ_SCALE(15);

      if (!MapCreditPen){
         MapCreditPen = roadmap_canvas_create_pen ("Map.Credit");
         roadmap_canvas_set_foreground ("#848484");
         roadmap_canvas_set_opacity(220);
      }
      roadmap_canvas_select_pen (MapCreditPen);
      roadmap_canvas_draw_formated_string_size(&pos, ROADMAP_CANVAS_TOPLEFT, 8, FONT_TYPE_NORMAL, map_credit);
   }
}


void roadmap_screen_draw_map (RoadMapGuiRect *rect) {
   int j, k;
   int sq_count, fp_count;
   static int *fips = NULL;
   static int *in_view = NULL;
   int max_pen = roadmap_layer_max_pen();
   int use_only_main_pen = 0;
   
   if (rect) {
      roadmap_canvas_select_pen (RoadMapBackground);
      roadmap_canvas_erase ();
      RoadMapScreenLastPen = NULL;
      
      RoadMapArea focus;
      roadmap_math_to_area(rect, &focus);
      roadmap_math_set_focus(&focus);
   }
   
#ifndef SUPPORT_MULTI_FIPS
   if (!fips) {
#endif
      /* - Identifies the candidate counties. */
      
      fp_count = roadmap_locator_by_position (&RoadMapScreenCenter, &fips);
      
      /* Activate the first fips before erasing the canvas. This is useful
       * for small devices which it may take some time to load the fips
       * data.
       */
      
      if (fp_count) roadmap_locator_activate (fips[0]);
#ifndef SUPPORT_MULTI_FIPS
   } else {
      fp_count = 1;
   }
#endif
   
   if (in_view == NULL) {
      in_view = calloc (ROADMAP_MAX_VISIBLE, sizeof(int));
      roadmap_check_allocated(in_view);
   }
   
#ifdef DEBUG_TIME
   end_time = NOPH_System_currentTimeMillis();
   printf ("roadmap_screen_repaint start drawing squares %d ms\n", end_time - start_time);
   start_time = end_time;
#endif
   
   /* - For each candidate county: */
   
   dbg_time_end(DBG_TIME_T1);
#ifdef SUPPORT_MULTI_FIPS
   for (i = fp_count-1; i >= 0; --i) {
      
      dbg_time_start(DBG_TIME_T2);
      /* -- Access the county's database. */
      
      if (roadmap_locator_activate (fips[i]) != ROADMAP_US_OK) {
         dbg_time_end(DBG_TIME_T2);
         continue;
      }
#endif //SUPPORT_MULTI_FIPS
      
      /* -- Look for the squares that are currently visible. */
      
      sq_count = roadmap_square_view (in_view, rect, ROADMAP_MAX_VISIBLE);
      //printf("num of squares: %d\n", count);
#ifdef DEBUG_TIME
      printf("Got %d squares to draw!\n", count);
#endif
//      printf("Got %d squares to draw!\n", count);
//      for (j = count - 1; j >= 0; --j) {
//         printf("going to draw square: %d\n",in_view[j]);
//      }
      dbg_time_end(DBG_TIME_T2);
      max_pen--;
      for (k = 0; k <= max_pen; ++k) {
         
         int n;
         int layer_count;
         int layers[256];
         int pen_type = k;
//         printf("++++ pen: %d\n", k);
         dbg_time_start(DBG_TIME_T3);
         layer_count = roadmap_layer_visible_lines (layers, 256, k);
         if (!layer_count) {
            dbg_time_end(DBG_TIME_T3);
            continue;
         }
         
  //       printf("layer count: %d\n", layer_count);
         
         if (use_only_main_pen) {
            pen_type = -1;
         }
         
#ifdef DEBUG_TIME
         end_time = NOPH_System_currentTimeMillis();
         printf ("roadmap_screen_repaint before squares %d ms\n", end_time - start_time);
         start_time = end_time;
#endif
         dbg_time_end(DBG_TIME_T3);
         
#if 1
         for (n = layer_count -1; n >= 0; n--) {
            //printf("--layer: %d\n", n);
            
            for (j = sq_count - 1; j >= 0; --j) {
               if (roadmap_square_set_current (in_view[j])) {
                  
                  if (k == 0 && n == layer_count -1) {
                     roadmap_screen_draw_polygons ();
                  }
                  
                  roadmap_screen_repaint_square (in_view[j], pen_type, 1, layers + n);
               }
            }
         }
         roadmap_screen_flush_lines();
         roadmap_screen_flush_points();
#else
         
         for (j = sq_count - 1; j >= 0; --j) {
            roadmap_square_set_current (in_view[j]);
            
            if (k == 0) {
               roadmap_screen_draw_polygons ();
            }
            
            roadmap_screen_repaint_square (in_view[j], pen_type, layer_count, layers);
         }
#endif
         
#ifdef DEBUG_TIME
         end_time = NOPH_System_currentTimeMillis();
         printf ("roadmap_screen_repaint after squares %d ms\n", end_time - start_time);
         start_time = end_time;
#endif
      }
      
      dbg_time_end(DBG_TIME_FULL);
      roadmap_screen_flush_lines ();
      roadmap_screen_flush_points ();
      dbg_time_start(DBG_TIME_FULL);
      
#ifdef DEBUG_TIME
      end_time = NOPH_System_currentTimeMillis();
      printf ("roadmap_screen_repaint end drawing squares %d ms\n", end_time - start_time);
      start_time = end_time;
#endif
      
      //roadmap_plugin_screen_repaint (max_pen);
//      roadmap_screen_flush_lines ();
//      roadmap_screen_flush_points ();
#ifdef SUPPORT_MULTI_FIPS
   }
#endif
   
   if (rect) {
      roadmap_math_release_focus();
   }
}

static void tile_refresh_timeout (void) {
   roadmap_main_remove_periodic(tile_refresh_timeout);
   roadmap_screen_redraw();
}
static void roadmap_screen_refresh_flow_control(void);//TODO: AviR - move this

void roadmap_screen_repaint_now( void ) {

    static int *fips = NULL;
    static int *in_view = NULL;
#ifdef VIEW_MODE_3D_OGL
    int OGL_2Dmode= FALSE;
    int OGL_3Dmode= (RoadMapScreenOGLViewMode== VIEW_MODE_3D);
#endif// VIEW_MODE_3D_OGL

    int i;
    int j;
    int k;
    int count;
    int max_pen = roadmap_layer_max_pen();
    static int nomap = 0;
    int use_only_main_pen = 0;
    RoadMapGuiPoint area;
    static int attempt_full_redraw = 0;
    int repaint_start_time = roadmap_time_get_millis();
   RoadMapGuiPointF center_delta;
   static RoadMapGuiPoint prev_delta;
   static int prev_zoom = 0;
   static RoadMapPosition last_tile_center = {-1, -1};
   int full_draw;

#ifdef DEBUG_TIME
    int start_time;
    int end_time;
#endif


#ifdef GTK2_OGL
    void roadmap_canvas_ogl_begin();
    void roadmap_canvas_ogl_end();
#endif// GTK2_OGL
    if (!RoadMapScreenInitialized || RoadMapScreenBackgroundRun ) return;

#ifdef SSD
    if (RoadMapScreenFrozen) {
#ifdef GTK2_OGL
		roadmap_canvas_ogl_begin();
#endif// GTK2_OGL
#ifdef VIEW_MODE_3D_OGL
		roadmap_canvas3_set3DMode(OGL_2Dmode);
#endif// VIEW_MODE_3D_OGL

#ifndef IPHONE
       ssd_dialog_draw_prev ();
#ifndef TOUCH_SCREEN
       roadmap_bar_draw_top_bar(TRUE);
#endif
#ifdef OPENGL
   roadmap_animation_start_repaint();
#endif
       ssd_dialog_draw_now();
#ifdef GTK2_OGL
       roadmap_canvas_ogl_end();
#endif// GTK2_OGL
       roadmap_canvas_refresh ();
#else //IPHONE
#ifdef OPENGL
       roadmap_animation_start_repaint();
#endif
#endif //IPHONE
#ifdef OPENGL
       roadmap_animation_end_repaint();
#endif
       return;
    }
#endif

#ifdef OPENGL
   roadmap_animation_start_repaint();
#endif

#ifdef GTK2_OGL
    roadmap_canvas_ogl_begin();
#endif// GTK2_OGL
#ifdef VIEW_MODE_3D_OGL
	roadmap_canvas3_set3DMode(OGL_3Dmode);
#endif// VIEW_MODE_3D_OGL

    if (RoadMapScreenPrevFast && !FAST_REFRESH) {
       RoadMapScreenPrevFast = 0;
       fast_refresh_end();
    } else if (!RoadMapScreenPrevFast && FAST_REFRESH) {
       RoadMapScreenPrevFast = 1;
       fast_refresh_start();
    }

#ifdef DEBUG_TIME
    start_time = NOPH_System_currentTimeMillis();
    printf ("In roadmap_screen_repaint...\n");
#endif
    dbg_time_start(DBG_TIME_FULL);
    dbg_time_start(DBG_TIME_T1);

    if (isViewModeAny3D()) {
       RoadMapScreenLowerEdge.x = roadmap_canvas_width() / 2;
       RoadMapScreenLowerEdge.y = roadmap_canvas_height();
       rds_unproject(&RoadMapScreenLowerEdge);
       roadmap_math_counter_rotate_coordinate (&RoadMapScreenLowerEdge);

       for (i=0; i<LAYER_PROJ_AREAS-1; i++) {
          area.y = roadmap_canvas_height() / LAYER_PROJ_AREAS *
                     (LAYER_PROJ_AREAS - 1 - i);
          area.x = roadmap_canvas_width() / 2;

          rds_unproject(&area);

          roadmap_math_counter_rotate_coordinate (&area);

          RoadMapScreenAreaDist[i] = roadmap_math_screen_distance
             (&area, &RoadMapScreenLowerEdge, MATH_DIST_SQUARED);
       }
    }

#if 0
    if (FAST_REFRESH) {

       if (!attempt_full_redraw) {
          use_only_main_pen = 1;
          max_pen = 1;
       }
    } else {
       attempt_full_redraw = 1;
    }
#endif

    if (in_view == NULL) {
       in_view = calloc (ROADMAP_MAX_VISIBLE, sizeof(int));
       roadmap_check_allocated(in_view);
    }

   count = roadmap_square_view (in_view, NULL, ROADMAP_MAX_VISIBLE);

    roadmap_log_push ("roadmap_screen_repaint");

    /* Repaint the drawing buffer. */

    /* Clean the drawing buffer. */
#ifndef OGL_TILE
    roadmap_canvas_select_pen (RoadMapBackground);
#else
   roadmap_canvas_select_pen (RoadMapNoTileBg);
#endif

    roadmap_canvas_erase ();
    RoadMapScreenLastPen = NULL;
/*
    if (count == 0) {
       roadmap_display_text("Info", roadmap_lang_get ("No map available"));
       nomap = 1;
    } else if (nomap) {
       roadmap_display_hide("Info");
       nomap = 0;
    }
*/

#ifndef OGL_TILE
    if (RoadMapScreenFinalScale == roadmap_math_get_scale(roadmap_screen_height())) {
       roadmap_label_start();
    }
#endif

#ifndef OGL_TILE
   roadmap_screen_draw_map(NULL);
#else
   if (last_tile_center.longitude == -1) { //tODO: change this flag
      RoadMapGuiPoint current_center;
      RoadMapGuiPoint last_center;
      RoadMapPosition math_center;
      zoom_t zoom;

      roadmap_math_get_context(&math_center, &zoom);
      roadmap_math_coordinate(&math_center, &current_center);
      roadmap_math_coordinate(&last_tile_center, &last_center);

      center_delta.x = 0;
      center_delta.y = 0;

      last_tile_center = math_center;
        }

   RoadMapPosition math_center;
   zoom_t zoom;
   RoadMapGuiPointF delta;
   RoadMapGuiPointF current_center;
   roadmap_math_get_context(&math_center, &zoom);

   roadmap_math_coordinate_f(&last_tile_center, &center_delta);
   RoadMapPosition back_to_last;
   roadmap_math_to_position_f(&center_delta, &back_to_last, 0);
   //printf("D: %f, %f\nB: %d, %d\nA: %d, %d\n\n", center_delta.x, center_delta.y,
//          last_tile_center.longitude, last_tile_center.latitude,
//          back_to_last.longitude, back_to_last.latitude);

   roadmap_math_coordinate_f(&math_center, &current_center);
   delta.x = -current_center.x + center_delta.x;
   delta.y = -current_center.y + center_delta.y;
   last_tile_center = math_center;
      RoadMapScreenFastRefreshOverride = 1;
      roadmap_canvas_tile_set(0, roadmap_math_get_zoom(), &delta, roadmap_math_get_orientation(), OGL_3Dmode, roadmap_screen_draw_map, 0);

   full_draw = roadmap_canvas_tile_draw(0, RoadMapScreenFastRefresh);
   RoadMapScreenFastRefreshOverride = 0;

#endif //OGL_TILE

   
   max_pen--;
   roadmap_square_set_current (in_view[0]); //any square, in case we did not select any square yet...
   roadmap_plugin_screen_repaint (max_pen);
   roadmap_screen_flush_lines();
   roadmap_screen_flush_points();


   //draw labels
        dbg_time_start(DBG_TIME_T4);
   if (!FAST_REFRESH || 1) {
#ifdef VIEW_MODE_3D_OGL
    roadmap_canvas3_set3DMode(OGL_2Dmode);
#endif// VIEW_MODE_3D_OGL
      if (!roadmap_label_draw_cache (!isViewModeAny3D(), full_draw)){ //TODO: support non tiles
         full_draw = 0;
      }
#ifdef VIEW_MODE_3D_OGL
    roadmap_canvas3_set3DMode(OGL_3Dmode);
#endif// VIEW_MODE_3D_OGL
#ifdef DEBUG_TIME
    end_time = NOPH_System_currentTimeMillis();
    printf ("roadmap_screen_repaint end drawing labels %d ms\n", end_time - start_time);
    start_time = end_time;
#endif
        }



#ifdef VIEW_MODE_3D_OGL
    roadmap_canvas3_set3DMode(OGL_2Dmode);
#endif// VIEW_MODE_3D_OGL

   roadmap_screen_draw_sky();
   roadmap_screen_draw_glow();

    for (j = count - 1; j >= 0; --j) {
         roadmap_square_set_current (in_view[j]);
         roadmap_screen_draw_alerts ();
     }

    if (!FAST_REFRESH
#ifndef J2ME
          ||
        roadmap_config_match(&RoadMapConfigStyleObjects, "yes")
#endif
        ) {

       draw_real_time_traffic_speed_signs();

       roadmap_object_iterate (roadmap_screen_draw_object);

    }

    if (!navigate_main_alt_routes_display() && roadmap_config_match (&RoadMapConfigMapSigns, "yes")) {

       roadmap_trip_display ();
    }

#ifdef DEBUG_TIME
    end_time = NOPH_System_currentTimeMillis();
    printf ("roadmap_screen_repaint b4 after_refresh callback %d ms\n", end_time - start_time);
    start_time = end_time;
#endif

   roadmap_message_update ();

   if (navigate_main_alt_routes_display()){
      roadmap_screen_shade_bg();
      if (roadmap_config_match (&RoadMapConfigMapSigns, "yes")) {
         roadmap_trip_display ();
      }
   }

   map_credit_display();

   RoadMapScreenAfterRefresh();

   roadmap_bar_draw();

   roadmap_screen_obj_draw ();

   roadmap_alerter_display();

   roadmap_ticker_display();

   roadmap_message_ticker_display();

   roadmap_display_signs ();

#ifndef TOUCH_SCREEN
   if(roadmap_screen_is_xicon_open())
 		roadmap_screen_draw_Xicon(); // draw the x icon in the middle of the screen for non-touch screen clicks
#endif

#ifdef SSD
#ifndef TOUCH_SCREEN
   if (ssd_dialog_is_context_menu()){
      ssd_dialog_draw_prev ();
   }
#endif
   ssd_dialog_draw_now ();
#endif

#ifdef DEBUG_TIME
    end_time = NOPH_System_currentTimeMillis();
    printf ("roadmap_screen_repaint b4 canvas refresh %d ms\n", end_time - start_time);
    start_time = end_time;
#endif
    dbg_time_end(DBG_TIME_T4);
#ifdef GTK2_OGL
    roadmap_canvas_ogl_end();
#endif// GTK2_OGL
    roadmap_canvas_refresh ();

    roadmap_log_pop ();
    dbg_time_end(DBG_TIME_FULL);
//    dbg_time_print();
#ifdef DEBUG_TIME
    printf ("Finished roadmap_screen_repaint in %d ms\n", (int)NOPH_System_currentTimeMillis() - start_time);
#endif

#ifdef OPENGL
   roadmap_animation_end_repaint();
#endif
   //printf("=====\n");
#ifdef OGL_TILE
   if (!full_draw) {
      //RoadMapScreenRefreshFlowControl = 2;
      //roadmap_main_remove_periodic(roadmap_screen_refresh_flow_control);
      //roadmap_main_set_periodic
      //(REFRESH_FLOW_CONTROL_TIMEOUT, roadmap_screen_refresh_flow_control);
//      roadmap_screen_mark_fast_repaint();
//printf("not full draw\n");
      //FAST_REFRESH |= SCREEN_FAST_OTHER;
      //roadmap_screen_repaint();
      //roadmap_screen_redraw();
      
      roadmap_main_set_periodic(10, tile_refresh_timeout);
}
#endif
}

RoadMapScreenSubscriber roadmap_screen_subscribe_after_flow_control_refresh(RoadMapScreenSubscriber handler)
{
   RoadMapScreenSubscriber prev = RoadMapScreenAfterFlowControlRefresh;

   if( handler && RoadMapScreenAfterFlowControlRefresh)
   {
      if( handler == RoadMapScreenAfterFlowControlRefresh)
      {
         return prev;
         // Called twice by same client. This is ok
      }
   }

   RoadMapScreenAfterFlowControlRefresh = handler;
   return prev;
}


/* Instead of repainting the screen with every event,
 * we use this timer as a flow control. It may take time for the
 * application to finish the task of drawing the screen and we don't
 * want to lag.
 */
static void roadmap_screen_refresh_flow_control(void) {

   roadmap_main_remove_periodic(roadmap_screen_refresh_flow_control);

   if (RoadMapScreenRefreshFlowControl > 1) {
      /* Update focus */
      roadmap_screen_refresh ();

      if (RoadMapScreenFastRefresh & SCREEN_FAST_NO_REDRAW) {
         RoadMapScreenFastRefresh &= ~SCREEN_FAST_NO_REDRAW;
         RoadMapScreenRefreshFlowControl = 0;
      }

      roadmap_screen_repaint_now ();

      if (!RoadMapScreenRefreshFlowControl && !RoadMapScreenFastRefresh)
         return;

   } else if (!RoadMapScreenFastRefresh) {
      /* If we had no requests to redraw the screen we don't need
       * to reset the timer.
       */

      if (RoadMapScreenPrevFast) {
         roadmap_screen_repaint_now ();
      }

      RoadMapScreenRefreshFlowControl = 0;
      return;
   }

   if (RoadMapScreenFastRefresh) {
      RoadMapScreenRefreshFlowControl = 1;

      if (RoadMapScreenFastRefresh & SCREEN_FAST_OTHER) {
         if( RoadMapScreenAfterFlowControlRefresh)
            RoadMapScreenAfterFlowControlRefresh();
      }

      RoadMapScreenFastRefresh &= ~(SCREEN_FAST_OTHER|SCREEN_FAST_NO_REDRAW);

   } else {
      RoadMapScreenRefreshFlowControl = 1;
   }
   roadmap_main_set_periodic
      (REFRESH_FLOW_CONTROL_TIMEOUT, roadmap_screen_refresh_flow_control);
}

int SYMBIAN_HACK_NET = 0;

static void roadmap_screen_repaint (void) {

//	if ( RoadMapScreenFrozen )
//	{
//		roadmap_screen_repaint_now();
//		return;
//	}

   if (!RoadMapScreenRefreshFlowControl) {
   /* TODO SYMBIAN HACK!!! */
      if (SYMBIAN_HACK_NET) {
         RoadMapScreenRefreshFlowControl = 2;
      } else {
         RoadMapScreenRefreshFlowControl = 1;
         roadmap_screen_refresh ();
         /* roadmap_screen_refresh may recursively call roadmap_screen_repaint
          *  so we must reset RoadMapScreenRefreshFlowControl again.
          */
         RoadMapScreenRefreshFlowControl = 1;
         if (!(RoadMapScreenFastRefresh & SCREEN_FAST_NO_REDRAW)) {
            roadmap_screen_repaint_now ();
         }
      }
      roadmap_main_set_periodic
         (REFRESH_FLOW_CONTROL_TIMEOUT, roadmap_screen_refresh_flow_control);
   } else {
      RoadMapScreenRefreshFlowControl++;
      if (RoadMapScreenFastRefresh & SCREEN_FAST_NO_REDRAW) {
         roadmap_main_remove_periodic(roadmap_screen_refresh_flow_control);
         roadmap_main_set_periodic
            (REFRESH_FLOW_CONTROL_TIMEOUT, roadmap_screen_refresh_flow_control);
      }
   }
}


#if 0
static void roadmap_screen_mark_fast_repaint (void) {

   RoadMapScreenFastRefresh |= SCREEN_FAST_NO_REDRAW;
   roadmap_screen_repaint ();
}
#endif


static void roadmap_screen_repaint_fast (void) {

   RoadMapScreenFastRefresh |= SCREEN_FAST_OTHER;
   roadmap_screen_repaint ();
}


static void roadmap_screen_configure (void) {

   RoadMapScreenWidth = roadmap_canvas_width();
   RoadMapScreenHeight = roadmap_canvas_height();
   roadmap_log (ROADMAP_DEBUG, "In roadmap_screen_configure. width:%d, height:%d", RoadMapScreenWidth, RoadMapScreenHeight);

   RoadMapScreenLabels = ! roadmap_config_match(&RoadMapConfigMapLabels, "off");

   roadmap_math_set_size (RoadMapScreenWidth, RoadMapScreenHeight);
   roadmap_log (ROADMAP_DEBUG, "B4 RoadMapScreenInitialized:%d", RoadMapScreenInitialized);

   set_horizon();
   roadmap_canvas_tile_resize(0);
   roadmap_canvas_tile_reset_all(0, 0);
   
   RoadMapScreenFinalScale == roadmap_math_get_scale(roadmap_screen_height());

   if (RoadMapScreenInitialized) {
      roadmap_screen_refresh();
      //roadmap_screen_repaint_now ();
   }
}

static void cancel_screen_animation (void) {
   RoadMapAnimation *animation = roadmap_animation_create();

   if (animation) {
      snprintf(animation->object_id, ANIMATION_MAX_OBJECT_LENGTH, "%s", SCREEN_OBJECT);
      roadmap_animation_cancel(animation);
   }
}


//#define TEST_SCREEN_DRAW
#ifdef TEST_SCREEN_DRAW
#include <sys/timeb.h>
#endif

static int roadmap_screen_short_click (RoadMapGuiPoint *point) {

   PluginLine line;
   int distance;
   RoadMapPosition position;
   RoadMapPosition from;
   RoadMapPosition to;
   //int scale;
   //int AlertId;

#ifdef TEST_SCREEN_DRAW
   struct timeb tp, tp1;

   ftime(&tp);

   int i;
   for (i =0; i<100; i++) {
      roadmap_screen_repaint_now();
   }

   ftime(&tp1);

   char str[100];
   snprintf(str, sizeof(str), "Time: %d", (tp1.time - tp.time) * 1000 + (tp1.millitm - tp.millitm));
   roadmap_messagebox("test",str);
   return 1;
#endif

   roadmap_math_to_position (point, &position, 1);

    if (roadmap_screen_touched_state() == 0)
	    if (roadmap_navigate_retrieve_line
             (&position,
              -1,
              roadmap_config_get_integer (&RoadMapConfigAccuracyMouse),
              &line,
              &distance,
              LAYER_ALL_ROADS) != -1) {

       	    roadmap_trip_set_point ("Selection", &position);
            roadmap_street_extend_line_ends (&line, &from, &to, FLAG_EXTEND_BOTH, NULL, NULL);
            roadmap_display_update_points ("Selected Street", &from, &to);

            roadmap_screen_repaint ();
   }

   roadmap_screen_touched();

   return 1;
}


static void roadmap_screen_reset_delta (void) {
   RoadMapScreenDeltaX = 0;
   RoadMapScreenDeltaY = 0;
   RoadMapScreenRotation = 0;
}


static void roadmap_screen_record_move (int dx, int dy,int projected) {

   RoadMapGuiPoint center;

   RoadMapScreenDeltaX += dx;
   RoadMapScreenDeltaY += dy;

   center.x = (RoadMapScreenWidth / 2) + dx;
   center.y = (RoadMapScreenHeight / 2) + dy;

   roadmap_math_to_position (&center, &RoadMapScreenCenter, projected);
   roadmap_math_set_center (&RoadMapScreenCenter);
}



static int roadmap_screen_drag_start (RoadMapGuiPoint *point) {

   RoadMapScreenFastRefresh |= SCREEN_FAST_DRAG;
   cancel_screen_animation();

#if defined(IPHONE) || defined(ANDROID) || (defined(OPENGL) && defined(_WIN32))
   DragLastTime = roadmap_time_get_millis();
   DragSpeedX = DragSpeedY = 0;
   if (roadmap_canvas_is_cording() && !CordingEvent) {
      RoadMapGuiPoint CordingGuiPoints[MAX_CORDING_POINTS];
      int i;
      CordingEvent = 1;
      roadmap_canvas_get_cording_pt (CordingGuiPoints);
      for (i = 0; i < MAX_CORDING_POINTS; ++i) {
         if (CordingGuiPoints[i].y < 0)
            CordingGuiPoints[i].y = 0;
         
         roadmap_math_to_position(CordingGuiPoints+i, CordingAnchors+i, 1);
      }

      //save initial angl
      CordingAngle = 0;

      point->x = (CordingGuiPoints[1].x + CordingGuiPoints[0].x) /2;
      point->y = (CordingGuiPoints[1].y + CordingGuiPoints[0].y) /2;

      RoadMapScreenPointerLocation = *point;

      CordingStartZoom = roadmap_math_get_zoom();
      CordingScale = 1.0f;
   } else
#endif
   RoadMapScreenPointerLocation = *point;

   roadmap_screen_hold (); /* We don't want to move with the GPS position. */
   roadmap_screen_refresh ();

   return 1;
}

static int roadmap_screen_drag_end (RoadMapGuiPoint *point) {
   RoadMapPosition new_center;
   int distance;
   BOOL animate = FALSE;

#if defined(IPHONE) || defined(ANDROID) || (defined(OPENGL) && defined(_WIN32))
   if (CordingEvent) {
      CordingEvent = 0;
      
      RoadMapScreenFinalScale = roadmap_math_get_scale(roadmap_screen_height());
      roadmap_canvas_tile_set_target_zoom(0, roadmap_math_get_zoom());
   } else
#endif
   {
      distance = roadmap_math_screen_distance(point, &RoadMapScreenPointerLocation, 0);
      int delta_time = roadmap_time_get_millis()-DragLastTime;
      if (delta_time > 0 && //do not sample infinite speed
          DragSpeedX != 0 && DragSpeedY != 0) { //do not sample single movement
         DragSpeedX = DragSpeedX*0.7 + 10*(point->x - RoadMapScreenPointerLocation.x)*0.3/delta_time;
         DragSpeedY = DragSpeedY*0.7 + 10*(point->y - RoadMapScreenPointerLocation.y)*0.3/delta_time;
      }
      //printf("DragSpeed end: %d, %d\n", DragSpeedX, DragSpeedY);

      // AGA check if the test is necessary
//      if ( 1 ||  distance > 1 ) {
      if (delta_time < 200 && (DragSpeedX != 0 || DragSpeedY != 0)) {
         if (DragSpeedX < -50)
            DragSpeedX = -50;
         else if (DragSpeedX > 50)
            DragSpeedX = 50;
         if (DragSpeedY < -50)
            DragSpeedY = -50;
         else if (DragSpeedY > 50)
            DragSpeedY = 50;
         
         
         RoadMapGuiPoint pnt = RoadMapScreenPointerLocation;
         pnt.x = roadmap_canvas_width()/2;
         pnt.y = roadmap_canvas_height()/2;
         if (isViewModeAny3D()) {
            pnt.x -= DragSpeedX*pnt.x/12;
            pnt.y -= DragSpeedY*pnt.y/12;
         } else {
            pnt.x -= DragSpeedX*pnt.x/24;
            pnt.y -= DragSpeedY*pnt.y/24;

         }
         roadmap_math_to_position(&pnt, &new_center, 0);
         animate = TRUE;
      } else {
      roadmap_screen_record_move
         (RoadMapScreenPointerLocation.x - point->x,
          RoadMapScreenPointerLocation.y - point->y,
          0);
      }

      RoadMapScreenPointerLocation = *point;
   }

   RoadMapScreenFastRefresh &= ~SCREEN_FAST_DRAG;
#define MAX(a,b) ((a) > (b) ? (a) : (b))
   roadmap_screen_repaint ();
   if (animate) {
      // Give more time for HD
      set_center( &new_center, 100 + 20*MAX(abs(DragSpeedX), abs(DragSpeedY)), ANIMATION_TIMING_EASY_OUT );
   }
   return 1;
}

static int roadmap_screen_drag_motion (RoadMapGuiPoint *point) {
#if defined(IPHONE) || defined(ANDROID) || (defined(OPENGL) && defined(_WIN32))
   RoadMapGuiPoint CordingPt[MAX_CORDING_POINTS];
   RoadMapPosition CordingPos[MAX_CORDING_POINTS];
   int i;

   if (roadmap_canvas_is_cording() && CordingEvent) {
	  int angle_new ;
      int angle_old ;
      int dist_new ;
      int dist_old;
      float factor ;

      roadmap_canvas_get_cording_pt (&CordingPt[0]);
      for (i = 0; i < MAX_CORDING_POINTS; ++i) {
         if (CordingPt[i].y < 0)
            CordingPt[i].y = 0;
         
         roadmap_math_to_position(CordingPt+i, CordingPos+i, 1);
      }

      // Calculate rotation
      if ( CordingRotationEnabled )
      {
         angle_new = roadmap_math_azymuth (&CordingPos[0], &CordingPos[1]);
         angle_old =  roadmap_math_azymuth (&CordingAnchors[0], &CordingAnchors[1]);
         CordingAngle = angle_new - angle_old;
         RoadMapScreenOrientationMode = ORIENTATION_DYNAMIC;
         roadmap_screen_rotate (CordingAngle);
      }

      //Calculate point position

      point->x = (CordingPt[1].x + CordingPt[0].x) /2;
      point->y = (CordingPt[1].y + CordingPt[0].y) /2;

      roadmap_screen_record_move (RoadMapScreenPointerLocation.x - point->x,
                                  RoadMapScreenPointerLocation.y - point->y,
                                  0);

      RoadMapScreenPointerLocation = *point;


      // Calculate zoom change

      dist_new = roadmap_math_distance (&CordingPos[0], &CordingPos[1]);
      dist_old = roadmap_math_distance (&CordingAnchors[0], &CordingAnchors[1]);

      factor = (float)dist_old / (float)dist_new;
      CordingScale *= factor;

      roadmap_math_zoom_set (CordingStartZoom* (CordingScale));
      RoadMapScreenFinalScale = next_pot(roadmap_math_get_scale(roadmap_screen_height()));
      roadmap_canvas_tile_set_target_zoom(0, roadmap_math_scale_to_zoom(RoadMapScreenFinalScale, roadmap_screen_height()));
      set_horizon();
      roadmap_layer_adjust ();

      for (i = 0; i < MAX_CORDING_POINTS; ++i) {
         roadmap_math_to_position(CordingPt+i, CordingAnchors+i, 1);
      }

   } else
#endif
   {
      DragSpeedX = DragSpeedX*0.6 + 10*(point->x - RoadMapScreenPointerLocation.x)*0.4/(roadmap_time_get_millis()-DragLastTime);
      DragSpeedY = DragSpeedY*0.6 + 10*(point->y - RoadMapScreenPointerLocation.y)*0.4/(roadmap_time_get_millis()-DragLastTime);
      DragLastTime = roadmap_time_get_millis();
      
      if (isViewModeAny3D()) {

         RoadMapGuiPoint p = *point;
         RoadMapGuiPoint p2 = RoadMapScreenPointerLocation;

         rds_unproject (&p);
         rds_unproject (&p2);

         roadmap_screen_record_move (p2.x - p.x, p2.y - p.y,0);

      } else {
         roadmap_screen_record_move
            (RoadMapScreenPointerLocation.x - point->x,
             RoadMapScreenPointerLocation.y - point->y,
             0);
      }
      RoadMapScreenPointerLocation = *point;
   }

   roadmap_screen_repaint_fast ();


   return 1;
}

int roadmap_screen_get_view_mode (void) {
#ifdef VIEW_MODE_3D_OGL
	return RoadMapScreenOGLViewMode;
#else
   return RoadMapScreenViewMode;
#endif// VIEW_MODE_3D_OGL
}

int roadmap_screen_get_nonogl_view_mode (void) {
	return RoadMapScreenViewMode;
}


static int roadmap_screen_get_view_mode_screen_touched (void) {

  if (roadmap_screen_touched_state() == -1)
  		return -1;

   return RoadMapScreenViewMode;
}

int roadmap_screen_get_orientation_mode (void) {


   return RoadMapScreenOrientationMode;
}

static int roadmap_screen_get_orientation_mode_touched (void) {

   if (roadmap_screen_show_icons_only_when_touched() == -1)
      return -1;
   else if (roadmap_math_get_orientation() < 4 && roadmap_math_get_orientation() > -4) //AR: For animation from dynamic to fixed
      return RoadMapScreenOrientationMode;
   else
      return ORIENTATION_DYNAMIC;
}

static void roadmap_screen_reset_pens (void) {

#ifdef OGL_TILE
   RoadMapNoTileBg = roadmap_canvas_create_pen ("Map.NoTitleBackground");
   //roadmap_canvas_set_foreground ("#bfbfbf");
   roadmap_canvas_set_foreground
      (roadmap_config_get (&RoadMapConfigMapBackground));
   roadmap_canvas_tile_reset_all(0, 0);
#endif

    RoadMapBackground = roadmap_canvas_create_pen ("Map.Background");
    roadmap_canvas_set_foreground
        (roadmap_config_get (&RoadMapConfigMapBackground));
}


void roadmap_screen_update_center (const RoadMapPosition *pos) {
    RoadMapPosition view_center;

    roadmap_screen_reset_delta ();
    RoadMapScreenCenter = *pos;
    roadmap_math_set_center (&RoadMapScreenCenter);

    RoadMapScreenCenterPixel.x = (RoadMapScreenWidth / 2);

    RoadMapScreenCenterPixel.y = (RoadMapScreenHeight / 2) ;

    RoadMapScreenCenterPixel.y += RoadMapScreenCenterDelta;
    roadmap_math_to_position (&RoadMapScreenCenterPixel, &view_center, 0);
    roadmap_math_set_center (&view_center);
}

void roadmap_screen_update_center_animated (const RoadMapPosition *pos, int duration, BOOL linear) {
   set_center(pos, duration, (linear ? ANIMATION_TIMING_LINEAR : ANIMATION_TIMING_EASY_IN | ANIMATION_TIMING_EASY_OUT));
}


void roadmap_screen_move_center (int dy) {
   if (RoadMapScreenCenterDelta == dy)
      return;
   
   RoadMapScreenCenterDelta = dy;
   RoadMapScreenCenterPixel.y = (RoadMapScreenHeight / 2) ;
   RoadMapScreenCenterPixel.y -= RoadMapScreenCenterDelta;
   roadmap_math_to_position (&RoadMapScreenCenterPixel, &RoadMapScreenCenter, 0);
   roadmap_screen_update_center(&RoadMapScreenCenter);
}


int roadmap_screen_height (void) {
   int height = RoadMapScreenHeight;//AviR - why add delta here??  + RoadMapScreenCenterDelta;
   if (RoadMapScreenViewMode == VIEW_MODE_3D) {
      height = height * 3;
   }
   return height;
}

#ifdef OPENGL
static void animation_set_callback (void *context) {
   RoadMapAnimation *animation = (RoadMapAnimation *)context;
   int i;
   RoadMapPosition pos;
   pos.longitude = pos.latitude = -1;

   if (strcmp(animation->object_id, SCREEN_OBJECT)) {
      roadmap_log (ROADMAP_WARNING, "animation_set_callback() - unknown object '%s'", animation->object_id);
      return;
   }

   for (i = 0; i < animation->properties_count; i++) {
      switch (animation->properties[i].type) {
         case ANIMATION_PROPERTY_ROTATION:
            roadmap_math_set_orientation (animation->properties[i].current);
            break;
         case ANIMATION_PROPERTY_SCALE:
            if (roadmap_math_set_scale(animation->properties[i].current, roadmap_screen_height())) {
               set_horizon();
               roadmap_layer_adjust ();
            }
            if (animation->properties[i].current == animation->properties[i].to) {
               RoadMapScreenFinalScale = roadmap_math_get_scale(roadmap_screen_height());
               roadmap_canvas_tile_set_target_zoom(0, roadmap_math_get_zoom());
            }
            break;
         case ANIMATION_PROPERTY_POSITION_X:
            pos.longitude = animation->properties[i].current;
            break;
         case ANIMATION_PROPERTY_POSITION_Y:
            pos.latitude = animation->properties[i].current;
            break;
         default:
            break;
      }
   }

   if (pos.longitude != -1 &&
       pos.latitude != -1)
      roadmap_screen_update_center(&pos);
   else
      roadmap_screen_update_center (&RoadMapScreenCenter);

}

static void animation_ended_callback (void *context) {
}
#endif

INLINE_DEC void set_orientation (int orientation, int duration, int timing) {
#ifdef OPENGL
   RoadMapAnimation *animation;
   int from = roadmap_math_get_orientation();
   int clock_wise = TRUE;

   roadmap_math_normalize_orientation(&from);
   roadmap_math_normalize_orientation(&orientation);
   if (orientation - from < 0)
      clock_wise = FALSE;

   if (from == orientation) return;

   if (abs(orientation - from) < 3) {
      roadmap_math_set_orientation (orientation);
      return;
   }


   animation = roadmap_animation_create();

   if (animation) {
      snprintf(animation->object_id, ANIMATION_MAX_OBJECT_LENGTH, "%s", SCREEN_OBJECT);
      animation->properties_count = 1;
      animation->properties[0].type = ANIMATION_PROPERTY_ROTATION;
      if (orientation - from >= 180 ||
          orientation - from <= -180) {
         if (360 - from < 360 - orientation)
            from = from - 360;
         else
            orientation = orientation - 360;
      }
      animation->properties[0].from = from;
      animation->properties[0].to = orientation;
      if (duration > 0)
         animation->duration = duration;
      else
         animation->duration = 300 +(int)(600.0f * abs(orientation - from) / 180);
      animation->timing = timing;
      animation->callbacks = &gAnimationCallbacks;
      roadmap_animation_register(animation);
   } else {
      roadmap_math_set_orientation (orientation);
   }
#else
   roadmap_math_set_orientation (orientation);
#endif //OPENGL
}

INLINE_DEC void set_scale (long scale, int use_map_units, int duration, int timing) {
#ifdef OPENGL
   long from;
   RoadMapAnimation *animation;

   if (scale < 0) return;

   scale = (long)(scale * (float)roadmap_screen_height() / (float)use_map_units);

   from = roadmap_math_get_scale(roadmap_screen_height());
   scale = roadmap_math_valid_scale (scale, roadmap_screen_height());

   if (from == scale) return;
   
#ifndef OGL_TILE
   if (!from || 1.0f*abs(scale - from)/from < 0.1) {
      if (roadmap_math_set_scale(scale, roadmap_screen_height())) {
         set_horizon();
         roadmap_layer_adjust ();
      }
      return;
   }
#endif //OGL_TILE

   RoadMapScreenFinalScale = scale;
   roadmap_canvas_tile_set_target_zoom(0, roadmap_math_scale_to_zoom(scale, roadmap_screen_height()));
   
   animation = roadmap_animation_create();

   if (animation) {
      snprintf(animation->object_id, ANIMATION_MAX_OBJECT_LENGTH, "%s", SCREEN_OBJECT);
      animation->properties_count = 1;
      animation->properties[0].type = ANIMATION_PROPERTY_SCALE;
      animation->properties[0].from = from;
      animation->properties[0].to = scale;
      if (duration > 0) {
         animation->duration = duration;
      } else {
         animation->duration = 600;
      }
      animation->timing = timing;
      animation->callbacks = &gAnimationCallbacks;
      roadmap_animation_register(animation);
   } else {
      if (roadmap_math_set_scale(scale, roadmap_screen_height())) {
         set_horizon();
         roadmap_layer_adjust ();
      }
   }
#else
   if (roadmap_math_set_scale(scale, use_map_units)) {
      set_horizon();
      roadmap_layer_adjust ();
   }
#endif //OPENGL
}

INLINE_DEC void set_center (const RoadMapPosition *center, int duration, int timing) {
#ifdef OPENGL
   RoadMapPosition from;
   RoadMapGuiPoint gui_from, gui_to;
   long screen_distance;
   RoadMapAnimation *animation;

   roadmap_screen_get_center(&from);

   if ((from.longitude == center->longitude) &&
       (from.latitude == center->latitude))
      return;

   roadmap_math_coordinate(&from, &gui_from);
   roadmap_math_coordinate(center, &gui_to);

   screen_distance = roadmap_math_screen_distance(&gui_from, &gui_to, 0);
   if (abs(screen_distance) > roadmap_screen_height()*5) { //abs - workaround for large distance
      roadmap_screen_update_center (center);
      return;
   }
   //printf("screen distance: %d\n", screen_distance);

   animation = roadmap_animation_create();

   if (animation) {
      snprintf(animation->object_id, ANIMATION_MAX_OBJECT_LENGTH, "%s", SCREEN_OBJECT);
      animation->properties_count = 2;
      animation->properties[0].type = ANIMATION_PROPERTY_POSITION_X;
      animation->properties[0].from = from.longitude;
      animation->properties[0].to = center->longitude;
      animation->properties[1].type = ANIMATION_PROPERTY_POSITION_Y;
      animation->properties[1].from = from.latitude;
      animation->properties[1].to = center->latitude;
      if (duration > 0) {
         animation->duration = duration;
      } else {
         animation->duration = 150 + screen_distance /2;
      }
      animation->timing = timing;
      animation->callbacks = &gAnimationCallbacks;
      roadmap_animation_register(animation);
   } else {
      roadmap_screen_update_center (center);
   }
#else
   roadmap_screen_update_center (center);
#endif //OPENGL
}

int roadmap_screen_refresh (void) {

   int refresh = 0;
   const char *focus;
   static time_t last_move_time = 0;
   static int last_focus_change_time = 0;
   static int  last_orientation_move = 0;

   if (!roadmap_screen_height())
      return refresh;

   focus = roadmap_trip_get_focus_name ();

   roadmap_log_push ("roadmap_screen_refresh");

   if (focus && roadmap_trip_is_focus_changed() ||
       roadmap_view_should_refresh()) {

      roadmap_screen_reset_delta ();

      roadmap_view_refresh();
      if (RoadMapScreenOrientationMode != ORIENTATION_FIXED) {
         set_orientation(roadmap_view_get_orientation(), 800, ANIMATION_TIMING_EASY_IN | ANIMATION_TIMING_EASY_OUT);
      }

      set_scale(roadmap_view_get_scale(), roadmap_screen_height(), 800, ANIMATION_TIMING_EASY_IN | ANIMATION_TIMING_EASY_OUT);

      if (!roadmap_view_hold())
         set_center(roadmap_trip_get_focus_position(), 800, ANIMATION_TIMING_EASY_IN | ANIMATION_TIMING_EASY_OUT);

      refresh = 1;
      last_focus_change_time = time(NULL);
      
      if( RoadMapScreenAfterFlowControlRefresh)
         RoadMapScreenAfterFlowControlRefresh();

   } else if (focus &&
        (roadmap_trip_is_focus_moved() || RoadMapScreenForceRefresh)) {
      BOOL should_move = FALSE;
      RoadMapScreenForceRefresh = 0;
      roadmap_view_refresh();
      
      if (!(time(NULL) - last_move_time < 10 &&
            RoadMapScreenFinalScale != roadmap_math_get_scale(roadmap_screen_height()))) {
         if ((time(NULL) - last_move_time >= 10) &&
             RoadMapScreenFinalScale != roadmap_math_get_scale(roadmap_screen_height()))
            RoadMapScreenFinalScale = roadmap_math_get_scale(roadmap_screen_height());
         
         if (roadmap_math_get_scale(roadmap_screen_height()) <= 600 ||
             time(NULL) - last_focus_change_time < 3 ||
             time(NULL) - last_move_time > 4 ||
             abs(roadmap_view_get_orientation() - last_orientation_move) >= 5/*30*/) {
            should_move = TRUE;
            last_orientation_move = roadmap_view_get_orientation();
            last_move_time = time(NULL);
         }
         
         if (RoadMapScreenOrientationMode != ORIENTATION_FIXED && should_move) {
            set_orientation (roadmap_view_get_orientation() + RoadMapScreenRotation, 700, 0);
         }
         
         set_scale(roadmap_view_get_scale(), roadmap_screen_height(), RM_SCREEN_CAR_ANIMATION, 0);
         
         if (!roadmap_view_hold() && should_move) {
            set_center(roadmap_trip_get_focus_position(), RM_SCREEN_CAR_ANIMATION, 0);
         }
      }

      refresh = 1;

      if (RoadMapScreenDeltaX || RoadMapScreenDeltaY) {

         /* Force recomputation. */

         int dx = RoadMapScreenDeltaX;
         int dy = RoadMapScreenDeltaY;

         RoadMapScreenDeltaX = RoadMapScreenDeltaY = 0;
         roadmap_screen_record_move (dx, dy,0);
      }
   }

   if (roadmap_config_match (&RoadMapConfigMapRefresh, "forced")) {

      refresh = 1;
   }

	if (RoadMapScreenDirty) {
      RoadMapScreenDirty = 0;
		refresh = 1;
   }

   if (roadmap_trip_is_refresh_needed() || refresh) {

      refresh = 1;
      roadmap_screen_repaint ();
   }

   roadmap_log_pop ();

   return refresh;
}


void roadmap_screen_mark_redraw (void) {
   RoadMapScreenDirty++;
}


void roadmap_screen_redraw (void) {

   /* Since we already need a redraw, check if a refresh
    * is needed.
    */
#ifdef J2ME
   roadmap_screen_repaint_now ();
#else
   if (!roadmap_screen_refresh()) {
      roadmap_screen_repaint ();
   }
#endif
}


void roadmap_screen_freeze (void) {

   RoadMapScreenFrozen = 1;
}

void roadmap_screen_unfreeze (void) {

   if (!RoadMapScreenFrozen) return;

   RoadMapScreenFrozen = 0;
   roadmap_screen_repaint ();
}

static void set_right_softkey(const char *name, const char *str, RoadMapCallback callback){
	static Softkey s;
	strcpy(s.text, str);
	s.callback = callback;
	roadmap_softkeys_set_right_soft_key(name, &s);

}

void focus_on_location(){
   roadmap_screen_hold ();
   RoadMapScreenOrientationMode = ORIENTATION_DYNAMIC;
   roadmap_trip_set_focus ("Location");
   //roadmap_screen_zoom_reset ();
   roadmap_state_refresh ();
   set_scale(1000, roadmap_screen_height(), 600, ANIMATION_TIMING_EASY_IN | ANIMATION_TIMING_EASY_OUT);

   roadmap_screen_refresh ();
}

void show_me_on_map(void){
   static int count = 0;
   
   if (roadmap_view_hold() && count++ < 1) {
      return;
   } else if (roadmap_view_hold()) {
      count = 0;
      if (!roadmap_view_reset_hold())
         return;
   }

	ssd_dialog_hide_all(dec_close);

    if (!roadmap_gps_have_reception()) {
        const RoadMapPosition *Location;
        Location = roadmap_trip_get_position( "Location" );
        if ((Location != NULL) && !IS_DEFAULT_LOCATION( Location ) ){
           focus_on_location();
           return;
        }
    }
    roadmap_screen_hold ();
    RoadMapScreenOrientationMode = ORIENTATION_DYNAMIC;
    roadmap_trip_set_focus ("GPS");
    roadmap_screen_set_Xicon_state(FALSE);
    roadmap_softkeys_remove_right_soft_key("Me on map");
    roadmap_state_refresh ();

    roadmap_screen_refresh ();
}

void focus_on_me(void){

   if (!roadmap_gps_have_reception()) {
       const RoadMapPosition *Location;
       Location = roadmap_trip_get_position( "Location" );
       if ((Location != NULL) && !IS_DEFAULT_LOCATION( Location ) ){
          focus_on_location();
          return;
       }
   }

   roadmap_screen_hold ();
   RoadMapScreenOrientationMode = ORIENTATION_DYNAMIC;
   roadmap_trip_set_focus ("GPS");
   roadmap_screen_zoom_reset ();
   roadmap_softkeys_remove_right_soft_key("Me on map");
   roadmap_state_refresh ();

   roadmap_screen_refresh ();
}

void roadmap_screen_hold (void) {
   roadmap_trip_copy_focus ("Hold");
   roadmap_trip_set_focus ("Hold");
   roadmap_trip_is_focus_changed();
   roadmap_screen_reset_delta ();
   roadmap_screen_move_center(0);
   set_right_softkey("Me on map", "Me on map", show_me_on_map);
}

void roadmap_screen_add_focus_on_me_softkey(){
	set_right_softkey("Me on map", "Me on map", show_me_on_map);
}

void roadmap_screen_rotate (int delta) {

   int rotation = RoadMapScreenRotation + delta;
   int calculated_rotation;

   //if (RoadMapScreenRefreshFlowControl > 2) return;

   while (rotation >= 360) {
      rotation -= 360;
   }
   while (rotation < 0) {
      rotation += 360;
   }

   if (RoadMapScreenOrientationMode == ORIENTATION_DYNAMIC) {
      calculated_rotation = roadmap_math_get_orientation() + rotation;
   } else {
      calculated_rotation = rotation;
   }

	if (roadmap_math_set_orientation (calculated_rotation)) {
      RoadMapScreenRotation = rotation;
      roadmap_screen_update_center (&RoadMapScreenCenter);
      roadmap_screen_repaint_fast ();
   }
}


static void set_horizon(void) {
   enum projection_modes mode = PROJECTION_MODE_NONE;

#ifdef VIEW_MODE_3D_OGL
   if (  RoadMapScreenOGLViewMode == VIEW_MODE_3D ) {
	   RoadMapScreenOGL3dHorizon = -(sky_image_top_height());
      if (!roadmap_map_settings_isShowTopBarOnTap())
         RoadMapScreenOGL3dHorizon -= roadmap_bar_top_height();
      mode = PROJECTION_MODE_3D;
   } else {
	   RoadMapScreenOGL3dHorizon = 0;
   }
   roadmap_math_set_horizon (RoadMapScreenOGL3dHorizon, mode);
   roadmap_canvas_tile_set_horizon (RoadMapScreenOGL3dHorizon, mode);
#else
   if (!roadmap_screen_is_hd_screen() && ( RoadMapScreenViewMode == VIEW_MODE_3D ) ) {
      RoadMapScreen3dHorizon = -100;
      mode = PROJECTION_MODE_3D_NON_OGL;
   } else {
      RoadMapScreen3dHorizon = 0;
   }
   roadmap_math_set_horizon (RoadMapScreen3dHorizon, mode);
#endif// VIEW_MODE_3D_OGL
}


void roadmap_screen_toggle_view_mode (void) {
#ifdef VIEW_MODE_3D_OGL
   if (RoadMapScreenOGLViewMode == VIEW_MODE_2D) {
	   RoadMapScreenOGLViewMode = VIEW_MODE_3D;
	   roadmap_config_set (&RoadMapConfigMapOrientation,"3D");
	   roadmap_analytics_log_event(ANALYTICS_EVENT_CHANGE_VIEW, ANALYTICS_EVENT_INFO_NEW_MODE, ANALYTICS_EVENT_VIEWMODE_3D);
   } else {
	   RoadMapScreenOGLViewMode = VIEW_MODE_2D;
	   roadmap_config_set (&RoadMapConfigMapOrientation,"2D");
	   roadmap_analytics_log_event(ANALYTICS_EVENT_CHANGE_VIEW, ANALYTICS_EVENT_INFO_NEW_MODE, ANALYTICS_EVENT_VIEWMODE_2D);
   }
#else
   if ((!roadmap_screen_is_hd_screen()) && (RoadMapScreenViewMode == VIEW_MODE_2D)) {
      RoadMapScreenViewMode = VIEW_MODE_3D;
      roadmap_config_set (&RoadMapConfigMapOrientation,"3D");
      roadmap_analytics_log_event(ANALYTICS_EVENT_CHANGE_VIEW, ANALYTICS_EVENT_INFO_NEW_MODE, ANALYTICS_EVENT_VIEWMODE_3D);
   } else {
      RoadMapScreenViewMode = VIEW_MODE_2D;
      roadmap_config_set (&RoadMapConfigMapOrientation,"2D");
      roadmap_analytics_log_event(ANALYTICS_EVENT_CHANGE_VIEW, ANALYTICS_EVENT_INFO_NEW_MODE, ANALYTICS_EVENT_VIEWMODE_2D);
   }
#endif// VIEW_MODE_3D_OGL

   roadmap_label_clear_all();
   set_horizon();
   if (RoadMapScreenCenter.latitude != 0 ||
       RoadMapScreenCenter.longitude != 0) {
      roadmap_screen_update_center (&RoadMapScreenCenter);
      roadmap_trip_set_point ("Hold", &RoadMapScreenCenter);
   }
   RoadMapScreenForceRefresh = 1;
   roadmap_layer_adjust();
   roadmap_screen_redraw ();
}

void roadmap_screen_set_view(int view_mode){
#ifdef VIEW_MODE_3D_OGL
   if ( view_mode == VIEW_MODE_3D ) {
	   RoadMapScreenOGLViewMode = VIEW_MODE_3D;
	   roadmap_config_set (&RoadMapConfigMapOrientation,"3D");
	   roadmap_analytics_log_event(ANALYTICS_EVENT_CHANGE_VIEW, ANALYTICS_EVENT_INFO_NEW_MODE, ANALYTICS_EVENT_VIEWMODE_3D);
   } else {
	   RoadMapScreenOGLViewMode = VIEW_MODE_2D;
	   roadmap_config_set (&RoadMapConfigMapOrientation,"2D");
	   roadmap_analytics_log_event(ANALYTICS_EVENT_CHANGE_VIEW, ANALYTICS_EVENT_INFO_NEW_MODE, ANALYTICS_EVENT_VIEWMODE_2D);
   }
#else
   if (( view_mode == VIEW_MODE_3D ) ) {
      RoadMapScreenViewMode = VIEW_MODE_3D;
      roadmap_config_set (&RoadMapConfigMapOrientation,"3D");
      roadmap_analytics_log_event(ANALYTICS_EVENT_CHANGE_VIEW, ANALYTICS_EVENT_INFO_NEW_MODE, ANALYTICS_EVENT_VIEWMODE_3D);
   } else {
      RoadMapScreenViewMode = VIEW_MODE_2D;
      roadmap_config_set (&RoadMapConfigMapOrientation,"2D");
      roadmap_analytics_log_event(ANALYTICS_EVENT_CHANGE_VIEW, ANALYTICS_EVENT_INFO_NEW_MODE, ANALYTICS_EVENT_VIEWMODE_2D);
   }
#endif// VIEW_MODE_3D_OGL

   roadmap_label_clear_all();
   set_horizon();
   if (RoadMapScreenCenter.latitude != 0 ||
       RoadMapScreenCenter.longitude != 0) {
      roadmap_screen_update_center (&RoadMapScreenCenter);
      roadmap_trip_set_point ("Hold", &RoadMapScreenCenter);
   }
   RoadMapScreenForceRefresh = 1;
   roadmap_layer_adjust();
   roadmap_screen_redraw ();
}
void roadmap_screen_toggle_labels (void) {

   RoadMapScreenLabels = ! RoadMapScreenLabels;
   roadmap_screen_repaint();
}

void roadmap_screen_toggle_orientation_mode (void) {

   if (RoadMapScreenOrientationMode == ORIENTATION_DYNAMIC) {

      RoadMapScreenOrientationMode = ORIENTATION_FIXED;

   } else {
      RoadMapScreenOrientationMode = ORIENTATION_DYNAMIC;
   }

   RoadMapScreenRotation = 0;
   roadmap_state_refresh ();
	roadmap_screen_mark_redraw();
   //roadmap_screen_rotate (0);
   set_orientation(0, -1, ANIMATION_TIMING_EASY_IN | ANIMATION_TIMING_EASY_OUT);
   roadmap_screen_refresh();
}

void roadmap_screen_set_orientation_fixed (void) {

  RoadMapScreenOrientationMode = ORIENTATION_FIXED;

   RoadMapScreenRotation = 0;
   set_orientation (0, -1, ANIMATION_TIMING_EASY_IN | ANIMATION_TIMING_EASY_OUT);
}

void roadmap_screen_set_orientation_dynamic (void) {

  RoadMapScreenOrientationMode = ORIENTATION_DYNAMIC;
}


void roadmap_screen_increase_horizon (void) {

   if (RoadMapScreenViewMode != VIEW_MODE_3D) return;

   if (RoadMapScreen3dHorizon >= -100) return;

   RoadMapScreen3dHorizon += 100;
   roadmap_math_set_horizon (RoadMapScreen3dHorizon, PROJECTION_MODE_3D_NON_OGL);
   roadmap_screen_repaint ();
}


void roadmap_screen_decrease_horizon (void) {

   if (RoadMapScreenViewMode != VIEW_MODE_3D) return;

   RoadMapScreen3dHorizon -= 100;
   roadmap_math_set_horizon (RoadMapScreen3dHorizon, PROJECTION_MODE_3D_NON_OGL);
   roadmap_screen_repaint ();
}

#define FRACMOVE 15

void roadmap_screen_move (int dx, int dy) {

   roadmap_screen_record_move (dx, dy,1);
   roadmap_screen_repaint_fast ();
}


void roadmap_screen_move_up (void) {

   roadmap_screen_record_move (0, 0 - (RoadMapScreenHeight / FRACMOVE),0);
   roadmap_screen_repaint_fast ();
}


void roadmap_screen_move_down (void) {

   roadmap_screen_record_move (0, RoadMapScreenHeight / FRACMOVE,0);
   roadmap_screen_repaint_fast ();
}


void roadmap_screen_move_right (void) {

   roadmap_screen_record_move (RoadMapScreenHeight / FRACMOVE, 0, 0);
   roadmap_screen_repaint_fast ();
}


void roadmap_screen_move_left (void) {

   roadmap_screen_record_move (0 - (RoadMapScreenHeight / FRACMOVE), 0, 0);
   roadmap_screen_repaint_fast ();
}

void roadmap_screen_zoom_in  (void)
{
   roadmap_view_auto_zoom_suspend();
   set_scale(roadmap_math_get_scale(roadmap_screen_height())/2, roadmap_screen_height(), 300, ANIMATION_TIMING_EASY_IN | ANIMATION_TIMING_EASY_OUT);
   if( RoadMapScreenAfterFlowControlRefresh)
      RoadMapScreenAfterFlowControlRefresh();

}

void roadmap_screen_zoom_out (void) {
   long scale = roadmap_math_get_scale(roadmap_screen_height())*2;
   roadmap_view_auto_zoom_suspend();
   set_scale(scale, roadmap_screen_height(), 300, ANIMATION_TIMING_EASY_IN | ANIMATION_TIMING_EASY_OUT);
   if( RoadMapScreenAfterFlowControlRefresh)
      RoadMapScreenAfterFlowControlRefresh();

}

static void on_device_event(device_event event, void* context) {

   if( event == device_event_window_orientation_changed ){
      set_horizon();

   	/* Redraw the ssd buffer */
   	if (!roadmap_screen_refresh())
   	   roadmap_screen_redraw();
   }
}

void roadmap_screen_zoom_reset (void) {

   roadmap_math_zoom_reset ();

   roadmap_layer_adjust ();
   roadmap_screen_update_center (&RoadMapScreenCenter);
   roadmap_screen_repaint ();
}

void roadmap_screen_restore_view(void){

   const char *saved_orientation;
   saved_orientation = roadmap_config_get(&RoadMapConfigMapOrientation);
   if (!strcmp(saved_orientation, "3D"))
      roadmap_screen_set_view(VIEW_MODE_3D);
   else
      roadmap_screen_set_view(VIEW_MODE_2D);
}

static int roadmap_screen_pressed(RoadMapGuiPoint *point) {

   const char *focus = roadmap_trip_get_focus_name ();
   if (focus && !strcmp(focus, "Hold")) {
      cancel_screen_animation();
   }

	roadmap_pointer_enable_double_click();

   return 0;
}
static int roadmap_screen_double_click (RoadMapGuiPoint *point) {
   RoadMapPosition center;
   roadmap_screen_hold();
   roadmap_math_to_position(point, &center, 1);
   set_center(&center, 300, ANIMATION_TIMING_EASY_OUT);

   roadmap_screen_zoom_in();

   return 1;
}

void roadmap_screen_initialize (void) {

   roadmap_config_declare
       ("preferences", &RoadMapConfigAccuracyMouse,  "20", NULL);

   roadmap_config_declare
       ("schema", &RoadMapConfigMapBackground, "#ffffe0", NULL);

   roadmap_config_declare_enumeration
       ("preferences", &RoadMapConfigMapSigns, NULL, "yes", "no", NULL);

   roadmap_config_declare_enumeration
       ("preferences", &RoadMapConfigMapRefresh, NULL, "normal", "forced", NULL);

   roadmap_config_declare_enumeration
       ("preferences", &RoadMapConfigStylePrettyDrag, NULL, "yes", "no", NULL);

   roadmap_config_declare_enumeration
       ("preferences", &RoadMapConfigStyleObjects, NULL, "yes", "no", NULL);

   roadmap_config_declare_enumeration
        ("preferences", &RoadMapConfigMapLabels, NULL, "on", "off", NULL);
#ifdef VIEW_MODE_3D_OGL
   roadmap_config_declare_enumeration
        ("user", &RoadMapConfigMapOrientation, NULL, "3D", "2D", NULL);
#else
   roadmap_config_declare_enumeration
         ("user", &RoadMapConfigMapOrientation, NULL, "2D", "3D", NULL);
#endif //VIEW_MODE_3D_OGL

   roadmap_config_declare_enumeration
      ("user", &RoadMapConfigShowScreenIconsOnTap, NULL, "yes", "no", NULL);

   roadmap_pointer_register_pressed
      (&roadmap_screen_pressed, POINTER_DEFAULT);
   roadmap_pointer_register_short_click
      (&roadmap_screen_short_click, POINTER_DEFAULT);
   roadmap_pointer_register_double_click
      (&roadmap_screen_double_click, POINTER_DEFAULT);
   roadmap_pointer_register_drag_start
      (&roadmap_screen_drag_start, POINTER_DEFAULT);
   roadmap_pointer_register_drag_end
      (&roadmap_screen_drag_end, POINTER_DEFAULT);
   roadmap_pointer_register_drag_motion
      (&roadmap_screen_drag_motion, POINTER_DEFAULT);

   roadmap_canvas_register_configure_handler (&roadmap_screen_configure);

   RoadMapScreenObjects.cursor = RoadMapScreenObjects.data;
   RoadMapScreenObjects.end = RoadMapScreenObjects.data + ROADMAP_SCREEN_BULK;

   RoadMapScreenLinePoints.cursor = RoadMapScreenLinePoints.data;
   RoadMapScreenLinePoints.end = RoadMapScreenLinePoints.data + ROADMAP_SCREEN_BULK;

   RoadMapScreenPoints.cursor = RoadMapScreenPoints.data;
   RoadMapScreenPoints.end = RoadMapScreenPoints.data + ROADMAP_SCREEN_BULK;

   RoadMapScreen3dHorizon = 0;

   roadmap_state_add ("view_mode", &roadmap_screen_get_view_mode);
   roadmap_state_add ("view_mode_screen_touched", &roadmap_screen_get_view_mode_screen_touched);
   roadmap_state_add ("show_screen_icons", &roadmap_screen_show_icons_only_when_touched);
   roadmap_state_add ("show_top_bar", &roadmap_screen_show_top_bar);
   roadmap_state_add ("orientation_mode",
            &roadmap_screen_get_orientation_mode_touched);
   roadmap_skin_register (roadmap_screen_reset_pens);

   roadmap_state_monitor (&roadmap_screen_mark_redraw);


   roadmap_device_events_register( on_device_event, NULL);

}


void roadmap_screen_shutdown (void) {
   RoadMapGpsPosition point;
   point.longitude = RoadMapScreenCenter.longitude;
   point.latitude  = RoadMapScreenCenter.latitude;
   point.steering  = RoadMapScreenRotation;

   roadmap_trip_set_mobile ("Hold", &point);
   if (RoadMapScreenRefreshFlowControl) {
      roadmap_main_remove_periodic(roadmap_screen_refresh_flow_control);
      RoadMapScreenRefreshFlowControl = 0;
   }
   RoadMapScreenInitialized = 0;	// Nothing to refresh while shutting down
   RoadMapScreenFrozen = 1;
}


void roadmap_screen_set_initial_position (void) {

    RoadMapScreenInitialized = 1;

    roadmap_layer_initialize();

   roadmap_screen_reset_pens();

    RoadMapPenEdges = roadmap_canvas_create_pen ("Map.Edges");
    roadmap_canvas_set_thickness (20);
    roadmap_canvas_set_foreground ("#bebebe");

    roadmap_layer_adjust ();
   
   RoadMapScreenFinalScale = roadmap_math_get_scale(roadmap_screen_height());
   roadmap_canvas_tile_set_target_zoom(0, roadmap_math_get_zoom());
}

int is_screen_wide(){

   BOOL is_wide = (BOOL)roadmap_horizontal_screen_orientation();
   return (int)is_wide;
}

void roadmap_screen_get_center (RoadMapPosition *center) {

   if (center != NULL) {
      *center = RoadMapScreenCenter;
   }
}


RoadMapScreenSubscriber roadmap_screen_subscribe_after_refresh
                                 (RoadMapScreenSubscriber handler) {

   RoadMapScreenSubscriber previous = RoadMapScreenAfterRefresh;

   if (handler == NULL) {
      RoadMapScreenAfterRefresh = roadmap_screen_after_refresh;
   } else {
      RoadMapScreenAfterRefresh = handler;
   }

   return previous;
}


#ifndef J2ME
/* TODO: ugly hack (both the hack and the drawing itself are ugly!).
 * This should be rewritten to allow specifying a sprite for
 * the direction mark.
 */
static void
roadmap_screen_draw_direction (RoadMapGuiPoint *point0,
                               RoadMapGuiPoint *point1,
                               int width,
                               int direction) {
   RoadMapGuiPoint from;
   RoadMapGuiPoint to;

   double delta_x = point1->x - point0->x;
   double delta_y = point1->y - point0->y;
   double ll = (1.0 * (abs((int)(delta_x)) + abs((int)(delta_y))) / ADJ_SCALE(15));

   if (ll >= 1) {

      static int i=0;

      double step_x = delta_x / ll;
      double step_y = delta_y / ll;
      double x = point0->x + step_x;
      double y = point0->y + step_y;

      while (abs((int)(x + step_x - point0->x)) < abs((int)(delta_x))) {
         i++;

         from.x = (int)x;
         from.y = (int)y;
         to.x = (int) (x + step_x);
         to.y = (int) (y + step_y);

         if (RoadMapScreenLinePoints.cursor + 5 >=
               RoadMapScreenLinePoints.end) {
            roadmap_screen_flush_lines ();
         }

         /* main line */
         /*
         RoadMapScreenLinePoints.cursor[0] = from;
         RoadMapScreenLinePoints.cursor[1] = to;
         *RoadMapScreenObjects.cursor = 2;
         RoadMapScreenLinePoints.cursor  += 2;
         RoadMapScreenObjects.cursor += 1;
          */

         /* head */
         if ((direction == 1) ||
               ((direction == 3) && (i % 2))) {
            RoadMapGuiPoint head;
            double dir=atan2(from.x-to.x, from.y-to.y);
            int i1=ADJ_SCALE(6);
            head.x = (short)(to.x + i1*sin(dir+0.35));
            head.y = (short)(to.y + i1*cos(dir+0.35));
            RoadMapScreenLinePoints.cursor[0] = head;
            head.x = (short)(to.x + i1*sin(dir-0.35));
            head.y = (short)(to.y + i1*cos(dir-0.35));
            RoadMapScreenLinePoints.cursor[2] = head;
            RoadMapScreenLinePoints.cursor[1] = to;
            *RoadMapScreenObjects.cursor = 3;
            RoadMapScreenLinePoints.cursor  += 3;
            RoadMapScreenObjects.cursor += 1;
         }

         if ((direction == 2) ||
               ((direction == 3) && !(i % 2))) {
            /* second head */
            RoadMapGuiPoint head;
            double dir=atan2(to.x-from.x, to.y-from.y);
            int i1=ADJ_SCALE(6);
            head.x = (short)(from.x + i1*sin(dir+0.35));
            head.y = (short)(from.y + i1*cos(dir+0.35));
            RoadMapScreenLinePoints.cursor[0] = head;
            head.x = (short)(from.x + i1*sin(dir-0.35));
            head.y = (short)(from.y + i1*cos(dir-0.35));
            RoadMapScreenLinePoints.cursor[2] = head;
            RoadMapScreenLinePoints.cursor[1] = from;
            *RoadMapScreenObjects.cursor = 3;
            RoadMapScreenLinePoints.cursor  += 3;
            RoadMapScreenObjects.cursor += 1;
         }

         x += step_x*4;
         y += step_y*4;
      }
   }
}


/* TODO: this function should either be implemented in
 * roadmap_screen_draw_one_line(), or common parts should be extracted.
 */
void roadmap_screen_draw_line_direction (RoadMapPosition *from,
                                         RoadMapPosition *to,
                                         RoadMapPosition *first_shape_pos,
                                         int first_shape,
                                         int last_shape,
                                         RoadMapShapeItr shape_itr,
                                         int width,
                                         int direction,
                                         const char *color) {

   static RoadMapPen direction_pen = NULL;

   RoadMapGuiPoint point0;
   RoadMapGuiPoint point1;

   /* These are used when the line has a shape: */
   RoadMapPosition midposition;
   RoadMapPosition last_midposition;

   int i;

   roadmap_screen_flush_lines ();
   roadmap_screen_flush_points ();

   if (direction_pen == NULL) {
      direction_pen = roadmap_canvas_create_pen ("direction_mark");
      if (RoadMapScreenViewMode == VIEW_MODE_3D)
      	roadmap_canvas_set_thickness (ADJ_SCALE(1));
      else
      	roadmap_canvas_set_thickness (ADJ_SCALE(2));
      roadmap_canvas_set_foreground (color);
   } else {
     roadmap_canvas_select_pen (direction_pen);
     roadmap_canvas_set_foreground (color);
      if (RoadMapScreenViewMode == VIEW_MODE_3D)
      	roadmap_canvas_set_thickness (ADJ_SCALE(1));
      else
      	roadmap_canvas_set_thickness (ADJ_SCALE(2));
   }

   if (first_shape >= 0) {

      last_midposition = *from;
      midposition = *first_shape_pos;

      for (i = first_shape; i <= last_shape; ++i) {

         if (shape_itr) (*shape_itr) (i, &midposition);
         else roadmap_shape_get_position (i, &midposition);

         if (roadmap_math_get_visible_coordinates
                        (&last_midposition, &midposition, &point0, &point1)) {

            roadmap_screen_draw_direction (&point0, &point1, width, direction);

         }
         last_midposition = midposition;
      }

      if (roadmap_math_get_visible_coordinates
                        (&last_midposition, to, &point0, &point1)) {

         roadmap_screen_draw_direction (&point0, &point1, width, direction);

      }

   } else {

      if (roadmap_math_get_visible_coordinates (from, to, &point0, &point1)) {
         roadmap_screen_draw_direction (&point0, &point1, width, direction);
      }
   }

   roadmap_screen_flush_lines ();
   roadmap_screen_flush_points ();
   RoadMapScreenLastPen = NULL;
}
#endif

int roadmap_screen_fast_refresh (void) {

   return FAST_REFRESH;
}

int roadmap_screen_animating (void) {

   return (RoadMapScreenFastRefresh & SCREEN_FAST_ANIMATING);
}

#if 0
//#ifndef J2ME
//#ifdef _WIN32
static unsigned long dbg_time_rec[DBG_TIME_LAST_COUNTER];
static unsigned long dbg_time_tmp[DBG_TIME_LAST_COUNTER];

void dbg_time_start(int type) {
   dbg_time_tmp[type] = (long)NOPH_System_currentTimeMillis();
}

void dbg_time_end(int type) {
   dbg_time_rec[type] += (long)NOPH_System_currentTimeMillis() - dbg_time_tmp[type];
}

int dbg_time_print() {
    int i;
    for (i=0; i<DBG_TIME_LAST_COUNTER; i++) printf ("Timer %d: %ld\n", i, dbg_time_rec[i]);
}

#else
void dbg_time_start(int type) {
}

void dbg_time_end(int type) {
}

int dbg_time_print() { return 0; }
#endif

void roadmap_screen_draw_flush(void){
	RoadMapScreenLastPen = NULL;
}

static void roadmap_screen_touched_timer(void) {

   roadmap_main_remove_periodic(roadmap_screen_touched_timer);
   screen_touched = 0;
   roadmap_screen_repaint ();
}

void roadmap_screen_touched(void){

   if (screen_touched_off){
   		screen_touched_off = 0;
   		return;
   }

   if (screen_touched){
   	  	roadmap_main_remove_periodic(roadmap_screen_touched_timer);
   }
   else{
   		screen_touched = 1;
   		roadmap_screen_repaint_now ();
   }
   screen_touched = 1;

   roadmap_main_set_periodic (SCREEN_TIMEOUT, roadmap_screen_touched_timer);

}

void roadmap_screen_touched_off(void){
   if (screen_touched){
   	  	roadmap_main_remove_periodic(roadmap_screen_touched_timer);
   }
   screen_touched = 0;
   screen_touched_off = 1;
   roadmap_screen_repaint_now ();

}

int roadmap_screen_touched_state(void){
	if (screen_touched)
		return 0;
	else
		return -1;
}

int roadmap_screen_show_screen_icons(){
   if (roadmap_config_match(&RoadMapConfigShowScreenIconsOnTap, "no"))
      return TRUE;
   else
      return FALSE;
}

int roadmap_screen_show_icons_only_when_touched(void){
#ifndef TOUCH_SCREEN
   return 0;
#endif
   if (roadmap_screen_show_screen_icons())
      return 0;
   return roadmap_screen_touched_state();
}



int roadmap_screen_show_top_bar(void){
   if (roadmap_map_settings_isShowTopBarOnTap()) // configured so top bar disappear on tap
      return roadmap_screen_not_touched_state(); // return according to touched state
   return 1; // don't hide top bar
}


int roadmap_screen_not_touched_state(void){
   if (screen_touched)
      return -1;
   else
      return 0;
}

#ifndef TOUCH_SCREEN
/**
 * Draws an icon in the middle of the screen that will enable non-touch users to click things on the screen
 */
static void roadmap_screen_draw_Xicon(){
	if(!strcmp(roadmap_trip_get_focus_name(),"Hold")){
	   RoadMapImage cross_hair_image;
	   RoadMapGuiPoint screen_point;
	   cross_hair_image = (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN, "CrossHair");

		screen_point.x = roadmap_canvas_width()/2;
	    screen_point.y = roadmap_canvas_height()/2;
		if (cross_hair_image) {
	        //center based on image size
	        screen_point.x -= roadmap_canvas_image_width(cross_hair_image)/2;
	        screen_point.y -= roadmap_canvas_image_height(cross_hair_image)/2;
	        roadmap_canvas_draw_image (cross_hair_image, &screen_point,  0, IMAGE_NORMAL);
	    }
	}else{
		roadmap_screen_set_Xicon_state(FALSE); // we are not on hold focus, thus disable the XIcon
	}
}
#endif
/*
 * Sets the current screen type (see roadmap_types.h for definitions )
 */
void roadmap_screen_set_screen_type( int width, int height )
{
   int min_dim, scale = 100;
   RoadMapScreenType = RM_SCREEN_TYPE_SD_GENERIC;

   /*
    * Temporary just simple classification
    */
   if ( width >= 640 || height >= 640 )
   {
      RoadMapScreenType = RM_SCREEN_TYPE_HD_GENERIC;
}
   /*
    * Temporary just simple classification
    */
   if ( width <= 240 || height <= 240 )
   {
      RoadMapScreenType = RM_SCREEN_TYPE_LD_GENERIC;
   }

/*
    * Setting the scale based on the 320 as 100 scale value
    */
   min_dim = width < height ? width : height;

   scale = ( 100 * min_dim )/320;

   if (roadmap_screen_is_hd_screen() && scale < 150)
      scale = 140;
   roadmap_screen_set_screen_scale( scale );
}

/*
 * Returns the current screen type (see roadmap_types.h for definitions )
 */
int roadmap_screen_get_screen_type( void )
{
	return RoadMapScreenType;
}

/*
 * Checks if the screen is high definition
 */
int roadmap_screen_is_hd_screen( void )
{
#ifndef TOUCH_SCREEN
	RoadMapScreenType = RM_SCREEN_TYPE_SD_GENERIC;
#endif
	return ( ( RoadMapScreenType & RM_SCREEN_TYPE_CATEGORY_HD ) > 0 );
}
/*
 * Checks if the screen is low definition
 */
int roadmap_screen_is_ld_screen( void )
{
   return ( ( RoadMapScreenType & RM_SCREEN_TYPE_CATEGORY_LD ) > 0 );
}

/*
 * Sets the current screen scale
 */
void roadmap_screen_set_screen_scale( int screen_scale )
{
	RoadMapScreenScale = screen_scale;
}

/*
 * Returns the current screen scale
 */
int roadmap_screen_get_screen_scale( void )
{
	return RoadMapScreenScale;
}

/*
 * Adjust the width value according to the base width
 */
int roadmap_screen_adjust_width( int orig_width )
{
   int denom = is_screen_wide() ?  RM_SCREEN_BASE_HEIGHT : RM_SCREEN_BASE_WIDTH;
	return ( ( orig_width * roadmap_canvas_width() ) / denom );
}

/*
 * Adjust the height value according to the base height
 */
int roadmap_screen_adjust_height( int orig_height )
{
   int denom = is_screen_wide() ?  RM_SCREEN_BASE_WIDTH : RM_SCREEN_BASE_HEIGHT;
	return ( ( orig_height * roadmap_canvas_height() ) / denom );
}

#ifndef TOUCH_SCREEN
BOOL roadmap_screen_is_xicon_open(){
	return isXIconOpen;
}

void XiconTimer(void){
	isXIconOpen = FALSE;
	roadmap_screen_redraw();
}
#endif

void roadmap_screen_set_Xicon_state(BOOL state){
#ifndef TOUCH_SCREEN
	if(isXIconOpen){
			roadmap_main_remove_periodic(XiconTimer);
	}
	if(state){
		roadmap_main_set_periodic(2500,XiconTimer); // after a while of not moving, hide icon
	}
	isXIconOpen= state;
#endif
}


void roadmap_screen_set_background_run( BOOL value )
{
	RoadMapScreenBackgroundRun = value;
}

void roadmap_screen_flush_lines_and_points(){
   roadmap_screen_flush_lines ();
   roadmap_screen_flush_points ();
}

BOOL roadmap_screen_get_background_run( void )
{
	return RoadMapScreenBackgroundRun;
}

void roadmap_screen_shade_bg(void){
   RoadMapGuiPoint points[4];
   int height = roadmap_canvas_height();
   int width = roadmap_canvas_width();
   int count = 4;
   RoadMapPen pen;

   pen = roadmap_canvas_create_pen ("shade_pen");
   roadmap_canvas_select_pen (pen);

   points[0].x = 0;
   points[0].y = 0;

   points[1].x = width;
   points[1].y = 0;

   points[2].x = width;
   points[2].y = height;

   points[3].x = 0;
   points[3].y = height;

   if (roadmap_skin_state() == 0){
      roadmap_canvas_set_foreground("#ffffff");
      roadmap_canvas_set_opacity(160);
   }else{
      roadmap_canvas_set_foreground("#000000");
      roadmap_canvas_set_opacity(90);
   }
  roadmap_canvas_draw_multiple_polygons (1, &count, points, 1, 0);
}

void roadmap_screen_set_animating( BOOL value ) {
   if (value)
      RoadMapScreenFastRefresh |= SCREEN_FAST_ANIMATING;
   else
      RoadMapScreenFastRefresh &= ~SCREEN_FAST_ANIMATING;
}

#if defined(ANDROID)
void roadmap_screen_set_cording_rotation_enabled( BOOL value ) {
   CordingRotationEnabled = value;
}
#endif

void roadmap_screen_set_scale (long scale, int use_map_units) {
   set_scale(scale, use_map_units, -1, ANIMATION_TIMING_EASY_IN | ANIMATION_TIMING_EASY_OUT);
}

#ifdef OPENGL
static void glow_animation_set_callback (void *context) {

   RoadMapAnimation *animation = (RoadMapAnimation *)context;
   int i;

   if (RoadMapScreenGlowScale == -1)
      return;

   for (i = 0; i < animation->properties_count; i++) {
      switch (animation->properties[i].type) {
         case ANIMATION_PROPERTY_SCALE:
            RoadMapScreenGlowScale = animation->properties[i].current;
            break;
         default:
            break;
      }
   }
}

static void glow_animation_ended_callback (void *context) {
   RoadMapScreenGlowScale = -1;
}

static RoadMapAnimationCallbacks gGlowAnimationCallbacks =
{
   glow_animation_set_callback,
   glow_animation_ended_callback
};

void roadmap_screen_stop_glow (void) {
   RoadMapAnimation *animation;

   if (RoadMapScreenGlowScale == -1)
      return;

   animation = roadmap_animation_create();
   if (animation) {
      snprintf(animation->object_id, ANIMATION_MAX_OBJECT_LENGTH, "%s__glow", SCREEN_OBJECT);
      roadmap_animation_cancel(animation);
      RoadMapScreenGlowScale = -1;
   }
}

void roadmap_screen_start_glow (RoadMapPosition *position, int max_duraiton, RoadMapGuiPoint *offset) {
   RoadMapAnimation *animation = NULL;

   roadmap_screen_stop_glow ();

   animation = roadmap_animation_create();
   if (animation) {
      RoadMapScreenGlowPos = *position;
      RoadMapScreenGlowOffset = *offset;

      snprintf(animation->object_id, ANIMATION_MAX_OBJECT_LENGTH, "%s__glow", SCREEN_OBJECT);
      animation->properties_count = 1;

      //glow scale
      animation->properties[0].type = ANIMATION_PROPERTY_SCALE;
      animation->properties[0].from = 1;
      animation->properties[0].to = 100;
      RoadMapScreenGlowScale = 1;
      animation->duration = 3500;
      animation->loops = max_duraiton*1000/animation->duration;
      animation->callbacks = &gGlowAnimationCallbacks;
      roadmap_animation_register(animation);
   }
}
#endif
