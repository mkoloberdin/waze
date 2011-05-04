/* roadmap_math.c - Manage the little math required to place points on a map.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
 *   Copyright 2005,2006 Ehud Shabtai
 *
 *   3D perspective support was integrated from the RoadNav project
 *   Copyright (c) 2004 - 2006 Richard L. Lynch <rllynch@users.sourceforge.net>
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
 *   See roadmap_math.h.
 *
 * These functions are used to compute the position of points on the map,
 * or the distance between two points, given their position.
 */

#define DECLARE_ROADMAP_MATH

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#include "roadmap.h"
#include "roadmap_square.h"
#include "roadmap_state.h"
#include "roadmap_config.h"
#include "roadmap_layer.h"
#include "roadmap_shape.h"
#include "navigate/navigate_main.h"

#include "roadmap_trigonometry.h"

#include "roadmap_math.h"
#ifdef OPENGL
#include "roadmap_canvas3d.h" // roadmap_canvas3_ogl_updateScale
#endif// OPENGL


#define MIN_ZOOM_IN     6

#define MAX_ZOOM_OUT    0x10000

#define DOTS_PER_INCH 72
#define INCHES_PER_DEGREE 4374754

static zoom_t overide_min_zoom = -1;

static RoadMapConfigDescriptor RoadMapConfigGeneralDefaultZoom =
                        ROADMAP_CONFIG_ITEM("General", "Default Zoom");

static RoadMapConfigDescriptor RoadMapConfigGeneralZoom =
                        ROADMAP_CONFIG_ITEM("General", "Zoom");

static RoadMapUnitChangeCallback sUnitChangeCb = NULL;

#define ROADMAP_REFERENCE_ZOOM 20


typedef struct RoadMapUnits_t {
    int value;
    float unit_per_latitude;
    float unit_per_longitude;
    float speed_per_knot;
    float speed_per_m_p_s;
    float cm_to_unit;
    int    to_trip_unit;
    float  to_screen_unit;
    char  *length;
    char  *trip_distance;
    char  *speed;

} RoadMapUnits;


static RoadMapUnits RoadMapMetricSystem = {
    ROADMAP_BASE_METRIC,
    0.11112F, /* Meters per latitude. */
    0.0F,     /* Meters per longitude (dynamic). */
    1.852F,   /* Kmh per knot. */
	3.6F,	  /* Kmh per m/s  */
    0.01F,    /* centimeters to meters. */
    1000,    /* meters per kilometer. */
    1.0f,
    "m",
    "km.",
    "kmh"
};

static RoadMapUnits RoadMapImperialSystem = {
    ROADMAP_BASE_IMPERIAL,
    0.11112F, /* Feet per latitude. */
    0.0f,     /* Feet per longitude (dynamic). */
    1.151f,   /* Mph per knot. */
    2.24F, 	  /* Mph per m/s */
    0.01f, /* centimeters to meters. */
    1609,    /* Feet per mile. */
    0.3048f,
    "ft",
    "mi.",
    "mph",
};

struct RoadMapContext_t RoadMapContext;

int RoadMapMathTileMode = 0;

void roadmap_math_set_min_zoom(zoom_t zoom){
   overide_min_zoom = zoom;
}

static void roadmap_math_trigonometry (int angle, int *sine_p, int *cosine_p) {

   int i = angle % 90;
   int sine;
   int cosine;

   while (i < 0) i += 90;

   if (i <= 45) {
      sine   = RoadMapTrigonometricTable[i].x;
      cosine = RoadMapTrigonometricTable[i].y;
   } else {
      i = 90 - i;
      sine   = RoadMapTrigonometricTable[i].y;
      cosine = RoadMapTrigonometricTable[i].x;
   }

   while (angle < 0) angle += 360;

   i = (angle / 90) % 4;
   if (i < 0) {
      i += 4;
   }

   switch (i) {
   case 0:   *sine_p = sine;       *cosine_p = cosine;     break;
   case 1:   *sine_p = cosine;     *cosine_p = 0 - sine;   break;
   case 2:   *sine_p = 0 - sine;   *cosine_p = 0 - cosine; break;
   case 3:   *sine_p = 0 - cosine; *cosine_p = sine;       break;
   }
}


static int roadmap_math_arccosine (int cosine, int sign) {

    int i;
    int low;
    int high;
    int result;
    int cosine_negative = 0;

    if (cosine < 0) {
        cosine = 0 - cosine;
        cosine_negative = 1;
    }
    if (cosine >= 32768) {
        if (cosine > 32768) {
            roadmap_log (ROADMAP_ERROR, "invalid cosine value %d", cosine);
            return 0;
        }
        cosine = 32767;
    }

    high = 45;
    low  = 0;

    if (cosine >= RoadMapTrigonometricTable[45].y) {

        while (high > low + 1) {

            i = (high + low) / 2;

            if (cosine > RoadMapTrigonometricTable[i-1].y) {
                high = i - 1;
            } else if (cosine < RoadMapTrigonometricTable[i].y) {
                low = i;
            } else {
                high = i;
                break;
            }
        }

        result = high;

    } else {

        while (high > low + 1) {

            i = (high + low) / 2;

            if (cosine >= RoadMapTrigonometricTable[i].x) {
                low = i;
            } else if (cosine < RoadMapTrigonometricTable[i-1].y) {
                high = i - 1;
            } else {
                high = i;
                break;
            }
        }

        result = 90 - high;
    }

    result = sign * result;

    if (cosine_negative) {
        result = 180 - result;
        if (result > 180) {
            result = result - 360;
        }
    }
    return result;
}


static void roadmap_math_compute_scale (void) {

   int orientation;

   int sine;
   int cosine = 0;

   RoadMapGuiPoint point;
   RoadMapPosition position;


   if (RoadMapContext.zoom == 0) {
       RoadMapContext.zoom = ROADMAP_REFERENCE_ZOOM;
   }

#ifndef IPHONE_NATIVE
   if ( roadmap_screen_is_hd_screen() )
   {
      roadmap_square_adjust_scale ((int)(RoadMapContext.zoom / 12));
   }
   else
#endif
   {
#ifndef OPENGL
	   if (roadmap_screen_fast_refresh())
#else
      if (RoadMapContext._is3D_projection == PROJECTION_MODE_3D)
#endif
      {
		  roadmap_square_adjust_scale ((int)(RoadMapContext.zoom / 10));
	   } else {
		  roadmap_square_adjust_scale ((int)(RoadMapContext.zoom / 20));
	   }
   }

   RoadMapContext.center_x = RoadMapContext.width / 2;
   RoadMapContext.center_y = RoadMapContext.height / 2;
   
   RoadMapContext.zoom_x = RoadMapContext.zoom_y = RoadMapContext.zoom * 100 / roadmap_screen_get_screen_scale();

   /* The horizontal ratio is derived from the vertical one,
    * with a scaling depending on the latitude. The goal is to
    * compute a map projection and avoid an horizontal distortion
    * when getting close to the poles.
    */

   roadmap_math_trigonometry (RoadMapContext.center.latitude / 1000000,
                                &sine,
                                &cosine);

   RoadMapMetricSystem.unit_per_longitude =
      (RoadMapMetricSystem.unit_per_latitude * cosine) / 32768;

   RoadMapImperialSystem.unit_per_longitude =
      (RoadMapImperialSystem.unit_per_latitude * cosine) / 32768;

   RoadMapContext.zoom_x =
      (RoadMapContext.zoom_x * 32768 / cosine);

   RoadMapContext.upright_screen.west =
      RoadMapContext.center.longitude
         - (RoadMapContext.center_x * RoadMapContext.zoom_x);

   RoadMapContext.upright_screen.north =
      RoadMapContext.center.latitude
         + (RoadMapContext.center_y * RoadMapContext.zoom_y);

   orientation = RoadMapContext.orientation;
   roadmap_math_set_orientation (0);

   point.x = RoadMapContext.width;
   point.y = RoadMapContext.height;
   roadmap_math_to_position (&point, &position, 1);
   RoadMapContext.upright_screen.south = position.latitude;
   RoadMapContext.upright_screen.east  = position.longitude;

   roadmap_math_set_orientation (orientation);
}


static int roadmap_math_check_point_in_segment (const RoadMapPosition *from,
											   const RoadMapPosition *to,
											   const RoadMapPosition *point,
											   int count,
											   RoadMapPosition intersections[]) {
	   if ( (((from->longitude >= (point->longitude - 1)) &&
	           ((point->longitude + 1) >= to->longitude))    ||
	         ((from->longitude <= (point->longitude + 1)) &&
	          ((point->longitude - 1) <= to->longitude)))       &&
	        (((from->latitude >= (point->latitude - 1))   &&
	          ((point->latitude + 1) >= to->latitude))       ||
	         ((from->latitude <= (point->latitude + 1))   &&
	          ((point->latitude - 1) <= to->latitude))) ) {

	      intersections[count] = *point;
	      count++;
	   }

	   return count;
}
static int roadmap_math_check_visible_point_in_segment (const RoadMapPosition *from,
                                                const RoadMapPosition *to,
                                                const RoadMapPosition *point,
                                                int count,
                                                RoadMapPosition intersections[]) {

   if (!roadmap_math_point_is_visible (point)) return count;

   return roadmap_math_check_point_in_segment(from,to,point,count,intersections);
}


static int roadmap_math_find_screen_intersection (const RoadMapPosition *from,
                                                  const RoadMapPosition *to,
                                                  double a,
                                                  double b,
                                                  RoadMapPosition intersections[],
                                                  int max_intersections) {

   int count = 0;
   RoadMapPosition point;
   int visibility_distance;
   
   if (!RoadMapMathTileMode) {
      visibility_distance = ROADMAP_VISIBILITY_DISTANCE;
   } else {
      visibility_distance = ROADMAP_VISIBILITY_FACTOR * (int)RoadMapContext.zoom;
   }

   if ((from->longitude - to->longitude > -1.0) &&
         (from->longitude - to->longitude < 1.0)) {

      /* The two points are very close, or the line is quasi vertical:
       * approximating the line to a vertical one is good enough.
       */
      point.longitude = from->longitude;
      point.latitude = RoadMapContext.focus.north + visibility_distance;
      count =
         roadmap_math_check_visible_point_in_segment
               (from, to, &point, count, intersections);

      if (count == max_intersections) return count;

      point.latitude = RoadMapContext.focus.south - visibility_distance;
      count =
         roadmap_math_check_visible_point_in_segment
               (from, to, &point, count, intersections);

      return count;

   } else if ((from->latitude - to->latitude > -1.0) &&
               (from->latitude - to->latitude < 1.0)) {

      /* The two points are very close, or the line is quasi horizontal:
       * approximating the line to a horizontal one is good enough.
       */

      point.latitude = from->latitude;
      point.longitude = RoadMapContext.focus.west - visibility_distance;
      count =
         roadmap_math_check_visible_point_in_segment
             (from, to, &point, count, intersections);

      if (count == max_intersections) return count;

      point.longitude = RoadMapContext.focus.east + visibility_distance;
      count =
         roadmap_math_check_visible_point_in_segment
             (from, to, &point, count, intersections);

      return count;

   } else {

      point.latitude = RoadMapContext.focus.north + visibility_distance;
      point.longitude = (int) ((point.latitude - b) / a);
      count =
         roadmap_math_check_visible_point_in_segment
               (from, to, &point, count, intersections);
      if (count == max_intersections) return count;

      point.latitude = RoadMapContext.focus.south - visibility_distance;
      point.longitude = (int) ((point.latitude - b) / a);
      count =
         roadmap_math_check_visible_point_in_segment
               (from, to, &point, count, intersections);
      if (count == max_intersections) return count;

      point.longitude = RoadMapContext.focus.west - visibility_distance;
      point.latitude = (int) (b + a * point.longitude);
      count =
         roadmap_math_check_visible_point_in_segment
               (from, to, &point, count, intersections);
      if (count == max_intersections) return count;

      point.longitude = RoadMapContext.focus.east + visibility_distance;
      point.latitude = (int) (b + a * point.longitude);
      count =
         roadmap_math_check_visible_point_in_segment
               (from, to, &point, count, intersections);
      if (count == max_intersections) return count;

   }

   return count;
}

#if defined(OPENGL) && defined(VIEW_MODE_3D_OGL)
static int roadmap_math_intersect_with_vector(const RoadMapPosition *from,
											const RoadMapPosition *to,
											double a,
											double b,
											const RoadMapPosition *point,
											const int dxy [],
                                            RoadMapPosition *intersection) {
	int x0= point->longitude;
	int y0= point->latitude;
	int dx= dxy[0];
	int dy= dxy[1];
	// intersect the two lines
	// y=a x + b
	// y= (dy/dx)(x-x0)+y0
	int xc;
	RoadMapPosition linesIntersection;

	if ((from->longitude - to->longitude > -1.0) &&
		 (from->longitude - to->longitude < 1.0)) {

		/* The two points are very close, or the line is quasi vertical:
		* approximating the line to a vertical one is good enough.
		*/
		if (dx!= 0) {
			xc= from->longitude;
		}
		else {
			return 0;
		}
	} else {
		// intersection point is(holds if dx==0):
		// x_c= ((b-y0)dx + x0 dy)/(dy- a dx)
		// y_c= a x0 + b
		double x_denominator= ((((double)(b-y0))*dx) + (((double)x0)*dy));
		xc= x_denominator/(dy- (a*dx));
	}
	linesIntersection.longitude= xc;
	if (dx!= 0)
		linesIntersection.latitude= ((dy*(xc-x0))/dx)+ y0;
	else
		linesIntersection.latitude= (a*xc)+ b;

	return roadmap_math_check_point_in_segment(from,to,&linesIntersection,0,intersection);

}
static int roadmap_math_find_screen_intersection_3D (const RoadMapPosition *from,
                                                  const RoadMapPosition *to,
                                                  double a,
                                                  double b,
                                                  RoadMapPosition intersections[],
                                                  int max_intersections) {
	int count = 0;
	int iv;
	RoadMapPosition point;
	const RoadMapPosition *persp_edges= RoadMapContext.perspective_edges;
	for (iv= 0; iv<4; iv++) {
		const int *dxy;
		int iv2;
		switch (iv) {
		case 0:
			dxy= RoadMapContext.d03;
			iv2= 3;
			break;
		case 1:
			dxy= RoadMapContext.d10;
			iv2= 0;
			break;
		case 2:
			dxy= RoadMapContext.d21;
			iv2= 1;
			break;
		case 3:
			dxy= RoadMapContext.d32;
			iv2= 2;
			break;
		}
		if (roadmap_math_intersect_with_vector(from,to,a,b,
				persp_edges+iv,dxy,&point)) {
			count= roadmap_math_check_point_in_segment(persp_edges+iv,persp_edges+iv2,&point,count,intersections);
			if (count== max_intersections)
				return count;
		}
	}

	return 0;
}

#endif// OPENGL,VIEW_MODE_3D_OGL

void roadmap_math_counter_rotate_coordinate (RoadMapGuiPoint *point) {

   int x = point->x - RoadMapContext.center_x;
   int y = RoadMapContext.center_y - point->y;

   point->x =
       RoadMapContext.center_x +
           (((x * RoadMapContext.cos_orientation)
                - (y * RoadMapContext.sin_orientation) + 16383) / 32768);

   point->y =
       RoadMapContext.center_y -
           (((x * RoadMapContext.sin_orientation)
                + (y * RoadMapContext.cos_orientation) + 16383) / 32768);
}


#if USE_FLOAT  /* for reference, until we're sure integer version works */
void roadmap_math_project (RoadMapGuiPoint *point) {

   /* how far away is this point along the Y axis */
   float fDistFromCenterY = RoadMapContext.height - point->y;

   /* how far from the bottom of the screen is the horizon */
   float fVisibleRange = RoadMapContext.height - RoadMapContext._3D_horizon;

   float fDistFromCenterX;
   float fDistFromHorizon;

   /* make the Y coordinate converge on the horizon as the
    * distance from the center goes to infinity */
   point->y = (int) (RoadMapContext.height - fDistFromCenterY /
                ( fabs(fDistFromCenterY / fVisibleRange) + 1 ));

   /* X distance from center of the screen */
   fDistFromCenterX = point->x - RoadMapContext.width / 2;

   /* distance from the horizon after adjusting for perspective */
   fDistFromHorizon = point->y - RoadMapContext._3D_horizon;

   /* squeeze the X axis, make it a point at the horizon and
    * normal sized at the bottom of the screen */
   point->x = (int) (fDistFromCenterX * ( fDistFromHorizon / fVisibleRange )
                        + RoadMapContext.width / 2);
}

void roadmap_math_unproject (RoadMapGuiPoint *point) {

   RoadMapGuiPoint point2;

   /* X distance from center of screen */
   float fDistFromCenterX = point->x - RoadMapContext.width / 2;

   /* Y distance from horizon */
   float fDistFromHorizon = point->y - RoadMapContext._3D_horizon;

   /* distance from bottom of screen to horizon */
   float fVisibleRange = RoadMapContext.height - RoadMapContext._3D_horizon;
   float fDistFromBottom;
   float fD;

   if (RoadMapContext._is3D_projection != PROJECTION_MODE_3D_NON_OGL) {
      return;
   }

   /* unsqueeze the X axis */
   point2.x = (int) (fDistFromCenterX /
         ( fDistFromHorizon / fVisibleRange ) + RoadMapContext.width / 2);

   /* distance from bottom of screen */
   fDistFromBottom = RoadMapContext.height - point->y;
   /* inverse Y squeezing formula */
   fD = fDistFromBottom / ( 1.0 - fDistFromBottom / fVisibleRange);

   /* center on screen */
   point2.y = (int) (RoadMapContext.height - fD);

   *point = point2;
}
#else
void roadmap_math_project (RoadMapGuiPoint *point) {

   /* how far away is this point along the Y axis */
   long DistFromCenterY = RoadMapContext.height - point->y;

   /* how far from the bottom of the screen is the horizon */
   long VisibleRange = RoadMapContext.height - RoadMapContext._3D_horizon;

   long DistFromCenterX;
   long DistFromHorizon;

   /* make the Y coordinate converge on the horizon as the
    * distance from the center goes to infinity */
   point->y = (short) (RoadMapContext.height -
               (DistFromCenterY * VisibleRange) /
                        (abs(DistFromCenterY) + VisibleRange));

   /* X distance from center of the screen */
   DistFromCenterX = point->x - RoadMapContext.width / 2;

   /* distance from the horizon after adjusting for perspective */
   DistFromHorizon = point->y - RoadMapContext._3D_horizon;

   /* squeeze the X axis, make it a point at the horizon and
    * normal sized at the bottom of the screen */
   point->x = (DistFromCenterX * DistFromHorizon) / VisibleRange
                        + (RoadMapContext.width / 2);
}

void roadmap_math_unproject (RoadMapGuiPoint *point) {

   RoadMapGuiPoint point2;

   /* X distance from center of screen */
   long DistFromCenterX = point->x - RoadMapContext.width / 2;

   /* Y distance from horizon */
   long DistFromHorizon = point->y - RoadMapContext._3D_horizon;

   /* distance from bottom of screen to horizon */
   long VisibleRange = RoadMapContext.height - RoadMapContext._3D_horizon;
   long DistFromBottom;
   long D;

   if (RoadMapContext._is3D_projection != PROJECTION_MODE_3D_NON_OGL) {
      return;
   }

   /* unsqueeze the X axis */
   point2.x = DistFromCenterX * VisibleRange / DistFromHorizon +
                RoadMapContext.width / 2;

   /* distance from bottom of screen */
   DistFromBottom = RoadMapContext.height - point->y;

   /* inverse Y squeezing formula */
   D = (DistFromBottom * VisibleRange) / ( (VisibleRange - DistFromBottom) );

   /* center on screen */
   point2.y = (short) (RoadMapContext.height - D);

   *point = point2;
}
#endif


static int roadmap_math_zoom_state (void) {

   if (RoadMapContext.zoom ==
         roadmap_config_get_integer (&RoadMapConfigGeneralDefaultZoom)) {

      return MATH_ZOOM_RESET;
   } else {

      return MATH_ZOOM_NO_RESET;
   }
}


long roadmap_math_get_scale (int use_map_units) {

   zoom_t res;
   long scale;

   res = RoadMapContext.zoom * 100 / roadmap_screen_get_screen_scale();

   if (use_map_units) {
      scale = (long)(res * RoadMapContext.units->unit_per_latitude * use_map_units +0.5);
   } else {
      scale = (long)(res * (1.0 * DOTS_PER_INCH * INCHES_PER_DEGREE / 1000000));
   }

   return scale;
}

zoom_t roadmap_math_scale_to_zoom (long scale, int use_map_units) {
   zoom_t res;
   
	if (scale < 0) return roadmap_math_get_zoom();
   
   if (use_map_units) {
      res = scale / (RoadMapContext.units->unit_per_latitude * use_map_units);
   } else {
      res = scale / (1.0 * DOTS_PER_INCH * INCHES_PER_DEGREE / 1000000);
   }
   
   res = ADJ_SCALE(res);
   
   return res;
}

int roadmap_math_set_scale (long scale, int use_map_units) {
   zoom_t res;
   
	if (scale < 0) return 0;
   
   res = roadmap_math_scale_to_zoom(scale, use_map_units);
   
   return roadmap_math_zoom_set (res);
}

long roadmap_math_valid_scale (long scale, int use_map_units) {

   long valid_scale;
   zoom_t zoom;

   if (scale < 0)
      return scale;

   zoom = MIN_ZOOM_IN * 100 / roadmap_screen_get_screen_scale();

   if (use_map_units) {
      valid_scale = (long)(zoom * RoadMapContext.units->unit_per_latitude * use_map_units +0.5);
   } else {
      valid_scale = (long)(zoom * (1.0 * DOTS_PER_INCH * INCHES_PER_DEGREE / 1000000));
   }

   if (scale < valid_scale)
      return valid_scale;
   else
      return scale;
}


void roadmap_math_rotate_project_coordinate (RoadMapGuiPoint *point)
{
    roadmap_math_rotate_coordinates (1, point);
	if (RoadMapContext._is3D_projection == PROJECTION_MODE_3D) {
		// alternative check if RoadMapScreenOGLViewMode == VIEW_MODE_3D
#ifdef OPENGL
		roadmap_canvas3_project(point);
#endif// OPENGL
	}
}

/* Rotation of the screen:
 * rotate the coordinates of a point on the screen, the center of
 * the rotation being the center of the screen.
 */
void roadmap_math_rotate_coordinates (int count, RoadMapGuiPoint *points) {

   int i;
   int x;
   int y;


   if (!RoadMapContext.orientation && RoadMapContext._is3D_projection != PROJECTION_MODE_3D_NON_OGL) return;

   for (i = count; i > 0; --i) {

      if (RoadMapContext.orientation) {
         x = points->x - RoadMapContext.center_x;
         y = RoadMapContext.center_y - points->y;

         points->x =
            RoadMapContext.center_x +
            (((x * RoadMapContext.cos_orientation)
              + (y * RoadMapContext.sin_orientation) + 16383) / 32768);

         points->y =
            RoadMapContext.center_y -
            (((y * RoadMapContext.cos_orientation)
              - (x * RoadMapContext.sin_orientation) + 16383) / 32768);
      }

      if (RoadMapContext._is3D_projection == PROJECTION_MODE_3D_NON_OGL) {

         roadmap_math_project (points);
      }

      points += 1;
   }
}


/*
 * rotate the coordinates of a point to an arbitrary angle
 */
void roadmap_math_rotate_point (RoadMapGuiPoint *point,
                                RoadMapGuiPoint *center, int angle) {

   static int cached_angle = -99999;
   static int sin_orientation;
   static int cos_orientation;

   int x;
   int y;

   if (angle != cached_angle) {
      cached_angle = angle;
      roadmap_math_trigonometry (cached_angle,
                                 &sin_orientation,
                                 &cos_orientation);
   }

   x = point->x;
   y = point->y;

   point->x =
      center->x +
      (((x * cos_orientation)
        + (y * sin_orientation) + 16383) / 32768);

   point->y =
      center->y -
      (((y * cos_orientation)
        - (x * sin_orientation) + 16383) / 32768);

   if (angle == 0) return;

   if (RoadMapContext._is3D_projection == PROJECTION_MODE_3D_NON_OGL) {
      roadmap_math_project (point);
   }
}


/* Rotate a specific object:
 * rotate the coordinates of the object's points according to the provided
 * rotation.
 */
void roadmap_math_rotate_object
         (int count, RoadMapGuiPoint *points,
          RoadMapGuiPoint *center, int orientation) {

   int i;
   int x;
   int y;
   int sin_o = 0;
   int cos_o = 0;
   int total = (RoadMapContext.orientation + orientation) % 360;


   if ((total == 0) && RoadMapContext._is3D_projection != PROJECTION_MODE_3D_NON_OGL) return;

   if (total) {
      roadmap_math_trigonometry (total, &sin_o, &cos_o);
   }

   for (i = count; i > 0; --i) {

      if (total) {
         x = points->x - center->x;
         y = center->y - points->y;

         points->x = center->x + (((x * cos_o) + (y * sin_o) + 16383) / 32768);
         points->y = center->y - (((y * cos_o) - (x * sin_o) + 16383) / 32768);
      }

      points += 1;
   }
}


void roadmap_math_initialize (void) {

    memset(&RoadMapContext, 0, sizeof(RoadMapContext));

    roadmap_config_declare ("session", &RoadMapConfigGeneralZoom, "100", NULL);
    roadmap_config_declare
        ("preferences", &RoadMapConfigGeneralDefaultZoom, "20", NULL);

    roadmap_state_add ("zoom_reset", &roadmap_math_zoom_state);

    RoadMapContext.orientation = 0;

    RoadMapContext._3D_horizon = 0;
    RoadMapContext._is3D_projection = PROJECTION_MODE_NONE;

    roadmap_math_use_imperial ();
    roadmap_math_compute_scale ();
}


RoadMapUnitChangeCallback roadmap_math_register_unit_change_callback (RoadMapUnitChangeCallback cb) {

	RoadMapUnitChangeCallback oldValue = sUnitChangeCb;
	sUnitChangeCb = cb;
	return oldValue;
}


void roadmap_math_use_metric (void) {

    RoadMapContext.units = &RoadMapMetricSystem;

	if (sUnitChangeCb != NULL)
		sUnitChangeCb ();
}


void roadmap_math_use_imperial (void) {

    RoadMapContext.units = &RoadMapImperialSystem;

	if (sUnitChangeCb != NULL)
		sUnitChangeCb ();
}


void roadmap_math_set_focus (const RoadMapArea *focus) {

   RoadMapContext.focus = *focus;
}


void roadmap_math_release_focus (void) {

   RoadMapContext.focus = RoadMapContext.current_screen;
}


int roadmap_math_get_visible_coordinates (const RoadMapPosition *from,
                                          const RoadMapPosition *to,
                                          RoadMapGuiPoint *point0,
                                          RoadMapGuiPoint *point1) {

   int from_visible = roadmap_math_point_is_visible (from);
   int to_visible = roadmap_math_point_is_visible (to);
   int count;
   double a;
   double b;
   int rc= 0;
   RoadMapPosition intersections[2];
   int max_intersections = 0;

   if (!point0 && (from_visible || to_visible)) {
      /* This is only a visibility test */
      return 1;
   }

   if (!from_visible || !to_visible) {

      if (from_visible || to_visible) {
    	   if (RoadMapContext._is3D_projection== PROJECTION_MODE_3D && !RoadMapMathTileMode) {
   			   roadmap_math_coordinate(from,point0);
   			   roadmap_math_coordinate(to,point1);
   			   return 1;
   		   }
    	   max_intersections = 1;
      } else {
         max_intersections = 2;
      }


      /* Equation of the line: */

      if ((from->longitude - to->longitude) == 0) {
         a = b = 0;
      } else {
         a = (double) (from->latitude - to->latitude) /
                   (from->longitude - to->longitude);
         b = from->latitude - a * from->longitude;
      }

      if (RoadMapContext._is3D_projection == PROJECTION_MODE_3D && !RoadMapMathTileMode)
      {
#if defined(OPENGL) && defined(VIEW_MODE_3D_OGL)
    	  rc= roadmap_math_find_screen_intersection_3D
			  (from, to, a, b, intersections, max_intersections);
#else
          roadmap_log (ROADMAP_ERROR, "projection mode 3D unexpected");
    	  rc= -1;
#endif// OPENGL,VIEW_MODE_3D_OGL
      }
      else {
    	  rc= roadmap_math_find_screen_intersection
            (from, to, a, b, intersections, max_intersections);
      }
      if (rc != max_intersections) {
    	  return 0;
      }
   }

   if (!point0) return 1;

   if (max_intersections == 2) {
      /* Make sure we didn't swap the from/to points */
      if (roadmap_math_distance (from, &intersections[0]) >
          roadmap_math_distance (from, &intersections[1])) {

         RoadMapPosition tmp = intersections[1];
         intersections[1] = intersections[0];
         intersections[0] = tmp;
      }
   }

   count = 0;

   if (from_visible) {
      roadmap_math_coordinate (from, point0);
   } else {
      roadmap_math_coordinate (&intersections[count], point0);
      count++;
   }

   if (to_visible) {
      roadmap_math_coordinate (to, point1);
   } else {
      roadmap_math_coordinate (&intersections[count], point1);
   }

   return 1;
}

void roadmap_math_restore_zoom (void) {

    RoadMapContext.zoom =
        roadmap_config_get_integer (&RoadMapConfigGeneralZoom);
    if (RoadMapContext.zoom == 0) {
         RoadMapContext.zoom =
            roadmap_config_get_integer (&RoadMapConfigGeneralDefaultZoom);
        if (RoadMapContext.zoom == 0) {
            RoadMapContext.zoom = ROADMAP_REFERENCE_ZOOM;
        }
    }
#ifdef OPENGL
    roadmap_canvas3_ogl_updateScale(RoadMapContext.zoom);
#endif// OPENGL
    roadmap_math_compute_scale ();
}

void roadmap_math_zoom_out (void) {

   zoom_t zoom;

   zoom = (3 * RoadMapContext.zoom) / 2;
   if (zoom < MAX_ZOOM_OUT) {
      RoadMapContext.zoom = zoom;
   }
   roadmap_config_set_integer (&RoadMapConfigGeneralZoom, (int)RoadMapContext.zoom);
   roadmap_math_compute_scale ();
}


void roadmap_math_zoom_in (void) {

   RoadMapContext.zoom = (2 * RoadMapContext.zoom) / 3;
   if (RoadMapContext.zoom < MIN_ZOOM_IN) {
      RoadMapContext.zoom = MIN_ZOOM_IN;
   }

   if (overide_min_zoom != -1){
      if (RoadMapContext.zoom < overide_min_zoom) {
           RoadMapContext.zoom = overide_min_zoom;
      }
   }


   roadmap_config_set_integer (&RoadMapConfigGeneralZoom, (int)RoadMapContext.zoom);
   roadmap_math_compute_scale ();
}


/* return true if zoom changed */
int roadmap_math_zoom_set (zoom_t zoom) {
   if (RoadMapContext.zoom == zoom) return 0;

   RoadMapContext.zoom = zoom;
   if (RoadMapContext.zoom < MIN_ZOOM_IN) {
      RoadMapContext.zoom = MIN_ZOOM_IN;
   } else if (zoom > MAX_ZOOM_OUT) {
      RoadMapContext.zoom = 0xffff;
   }

   if (overide_min_zoom != -1){
       if (RoadMapContext.zoom < overide_min_zoom) {
            RoadMapContext.zoom = overide_min_zoom;
       }
    }

   roadmap_config_set_integer (&RoadMapConfigGeneralZoom, (int)RoadMapContext.zoom);
#ifdef OPENGL
   roadmap_canvas3_ogl_updateScale(RoadMapContext.zoom);
#endif// OPENGL
   roadmap_math_compute_scale ();

   return 1;
}


void roadmap_math_adjust_zoom	 (int square) {
	/*
	 * TODO: This is a temporary patch
	 */

	if (!roadmap_square_at_current_scale (square)) {
		if (roadmap_square_scale (square) == 0) {
			roadmap_math_zoom_set (90);
		} else {
			roadmap_math_zoom_set (130);
		}
	}
}


void roadmap_math_zoom_reset (void) {

   RoadMapContext.zoom =
        roadmap_config_get_integer (&RoadMapConfigGeneralDefaultZoom);

   roadmap_config_set_integer (&RoadMapConfigGeneralZoom, (int)RoadMapContext.zoom);

   roadmap_math_compute_scale ();
}


int roadmap_math_thickness (int base, int declutter, int zoom_level,
                            int use_multiple_pens) {

   float ratio;
   zoom_t zoom = RoadMapContext.zoom;
   float factor = 2.5;

   if (zoom > 50) {
      zoom = roadmap_math_area_zoom (zoom_level - 1);
   }

   if (zoom < 15)
      factor = 1.5;

   ratio = (float)(((factor * ROADMAP_REFERENCE_ZOOM) * base) / zoom);

   if (ratio < 0.1 / base) {
      return 1;
   }

   if (declutter > (ROADMAP_REFERENCE_ZOOM*100)) {
      declutter = ROADMAP_REFERENCE_ZOOM*100;
   }

   if (ratio < 1.0 * base) {

      /* Use the declutter value to decide how fast should a line shrink.
       * This way, a street shrinks faster than a freeway when we zoom out.
       */
      ratio += (float)((base-ratio) * (0.30*declutter/ zoom));
      if (ratio > base) {
         ratio = (float)base;
      }
   }

   if (zoom_level > 1) ratio -= (zoom_level - 1)*2;

   if (use_multiple_pens && (ratio < 3)) {
      ratio = 1;
   }
   return (int) ratio;
}


void roadmap_math_set_size (int width, int height) {

   RoadMapContext.width = width;
   RoadMapContext.height = height;

   roadmap_math_compute_scale ();
}


void roadmap_math_set_center (RoadMapPosition *position) {

   //printf("math set center: %d %d\n", position->latitude, position->longitude);
   RoadMapContext.center = *position;

   roadmap_math_compute_scale ();
}


void roadmap_math_set_horizon (int horizon,int is_projection) {

   RoadMapContext._3D_horizon = horizon;
   if (horizon)
	   RoadMapContext._is3D_projection = is_projection;
   else
	   RoadMapContext._is3D_projection = 0;

   roadmap_math_compute_scale ();
}


void roadmap_math_normalize_orientation (int *direction) {
   *direction = *direction % 360;
   while (*direction < 0) *direction += 360;
}


int roadmap_math_set_orientation (int direction) {

   /* FIXME: this function, which primary purpose was to
    * compute the span of the visible map area when rotated,
    * has become THE way for setting the visible map area
    * (i.e. RoadMapContext.current_screen). Therefore, one
    * must execute it to the end.
    */
   int status = 1; /* Force a redraw by default. */

   roadmap_math_normalize_orientation (&direction);

   if (direction == RoadMapContext.orientation) {

      status = 0; /* Not modified at all. */

   }
#ifndef OPENGL
   else if ((direction != 0) &&
              (abs(direction - RoadMapContext.orientation) <= 2)) {

      /* We do not force a redraw for every small move, except
       * when it is a back-to-zero event, which might be a reset.
       */
      status = 0; /* Not modified enough. */
   }
#endif

   RoadMapContext.orientation = direction;

   roadmap_math_trigonometry (RoadMapContext.orientation,
                              &RoadMapContext.sin_orientation,
                              &RoadMapContext.cos_orientation);

   RoadMapContext.current_screen = RoadMapContext.upright_screen;

	if ((RoadMapContext.orientation != 0) || RoadMapContext._is3D_projection != PROJECTION_MODE_NONE) {

		int i;
		RoadMapGuiPoint point;
		RoadMapPosition position[4];

		RoadMapPosition *persp_edges;

		point.x = 0;
		if (RoadMapContext._is3D_projection == PROJECTION_MODE_3D_NON_OGL)
			point.y = 0;
		else
			point.y = -RoadMapContext._3D_horizon;
		roadmap_math_to_position (&point, position, 1);

		point.x = RoadMapContext.width;
		roadmap_math_to_position (&point, position+1, 1);

		point.y = RoadMapContext.height;
		roadmap_math_to_position (&point, position+2, 1);

		point.x = 0;
		roadmap_math_to_position (&point, position+3, 1);

		for (i = 0; i < 4; ++i) {
			if (position[i].longitude > RoadMapContext.current_screen.east) {

				RoadMapContext.current_screen.east = position[i].longitude;
			}
			if (position[i].longitude < RoadMapContext.current_screen.west) {

				RoadMapContext.current_screen.west = position[i].longitude;
			}
			if (position[i].latitude < RoadMapContext.current_screen.south) {

				RoadMapContext.current_screen.south = position[i].latitude;
			}
			if (position[i].latitude > RoadMapContext.current_screen.north) {

				RoadMapContext.current_screen.north = position[i].latitude;
			}
		}
#if defined(OPENGL) && defined(VIEW_MODE_3D_OGL)
		if (RoadMapContext._is3D_projection == PROJECTION_MODE_3D) {
			int hdist;
			int vdist;
			int dist;
			int normalize_by;
			// calculate perspective edges
			persp_edges= RoadMapContext.perspective_edges;
			for (i = 0; i < 4; ++i) {
				persp_edges[i]= position[i];
			}

			hdist= RoadMapContext.current_screen.east-RoadMapContext.current_screen.west;
			vdist= RoadMapContext.current_screen.north-RoadMapContext.current_screen.south;

			if (hdist > vdist)
				dist= hdist;
			else
				dist= vdist;

			normalize_by=dist/1000;
			if (normalize_by < 1)
				normalize_by = 1;
			RoadMapContext.d03[0]= (persp_edges[3].longitude- persp_edges[0].longitude)/normalize_by;
			RoadMapContext.d03[1]= (persp_edges[3].latitude- persp_edges[0].latitude)/normalize_by;
			RoadMapContext.d21[0]= (persp_edges[1].longitude- persp_edges[2].longitude)/normalize_by;
			RoadMapContext.d21[1]= (persp_edges[1].latitude- persp_edges[2].latitude)/normalize_by;
			RoadMapContext.d10[1]= (persp_edges[0].latitude- persp_edges[1].latitude)/normalize_by;
			RoadMapContext.d10[0]= (persp_edges[0].longitude- persp_edges[1].longitude)/normalize_by;
			RoadMapContext.d32[1]= (persp_edges[2].latitude- persp_edges[3].latitude)/normalize_by;
			RoadMapContext.d32[0]= (persp_edges[2].longitude- persp_edges[3].longitude)/normalize_by;
		}
		else {
			RoadMapContext.d03[0]= RoadMapContext.d21[0]= RoadMapContext.d10[1]= RoadMapContext.d32[1]= 0;
		}
#endif// VIEW_MODE_3D_OGL
	}
#if defined(OPENGL) && defined(VIEW_MODE_3D_OGL)
	else {
		RoadMapContext.d03[0]= RoadMapContext.d21[0]= RoadMapContext.d10[1]= RoadMapContext.d32[1]= 0;
	}
#endif// VIEW_MODE_3D_OGL

   roadmap_math_release_focus ();
   return status;
}


int  roadmap_math_get_orientation (void) {

   return RoadMapContext.orientation;
}

zoom_t  roadmap_math_get_zoom (void) {

   return RoadMapContext.zoom;
}

void roadmap_math_to_position (const RoadMapGuiPoint *point,
                               RoadMapPosition *position,
                               int projected) {
	RoadMapGuiPoint point2;
	RoadMapGuiPoint point3;

	if (projected &&
		// alternative check if RoadMapScreenOGLViewMode == VIEW_MODE_3D
		( RoadMapContext._is3D_projection == PROJECTION_MODE_3D)) {
		point3= *point;
#ifdef OPENGL
		roadmap_canvas3_unproject(&point3);
#endif// OPENGL
		point= &point3;
	}


   if (projected && RoadMapContext._is3D_projection == PROJECTION_MODE_3D_NON_OGL) {

      point2 = *point;
      roadmap_math_unproject (&point2);

      point = &point2;
   }

   if (RoadMapContext.orientation) {

      if (!projected || RoadMapContext._is3D_projection != PROJECTION_MODE_3D_NON_OGL) {
         point2 = *point;
      }

      roadmap_math_counter_rotate_coordinate (&point2);
      point = &point2;
   }

   position->longitude =
      RoadMapContext.upright_screen.west
         + (point->x * RoadMapContext.zoom_x);

   position->latitude =
      RoadMapContext.upright_screen.north
         - (point->y * RoadMapContext.zoom_y);
}

void roadmap_math_to_position_f (const RoadMapGuiPointF *point,
                               RoadMapPosition *position,
                               int projected) {
   
   position->longitude = (int) ((double)RoadMapContext.upright_screen.west
                                + (point->x * (double)RoadMapContext.zoom_x));
   
   position->latitude = (int) ((double)RoadMapContext.upright_screen.north
                               - (point->y * (double)RoadMapContext.zoom_y));
}


int roadmap_math_azymuth
       (const RoadMapPosition *point1, const RoadMapPosition *point2) {

    int result;
    float x;
    float y;
    float d;


    x = RoadMapContext.units->unit_per_longitude
            * (point2->longitude - point1->longitude);
    y = RoadMapContext.units->unit_per_latitude
            * (point2->latitude  - point1->latitude);

    d = (float)sqrt ((x * x) + (y * y));

    if (d > 0.0001 || d < -0.0001) {
        result = roadmap_math_arccosine
                    ((int) ((32768 * y) / d), (x > 0)?1:-1);
    } else {
        result = 0;
    }

    return result;
}


int roadmap_math_angle
       (const RoadMapGuiPoint *point1, const RoadMapGuiPoint *point2) {

    int result;
    int x;
    int y;
    float d;

    x = point2->x - point1->x;
    y = point2->y - point1->y;

    d = (float)sqrt ((x * x) + (y * y));

    if (d > 0.0001 || d < -0.0001) {
        result = roadmap_math_arccosine
                    ((int) ((32768 * y) / d), (x > 0)?1:-1);
    } else {
        result = 0;
    }

    return result;
}

long roadmap_math_screen_distance
       (const RoadMapGuiPoint *pt1, const RoadMapGuiPoint *pt2, int squared) {

   long dx;
   long dy;
   long ret;


   dx = pt1->x - pt2->x;
   dy = pt1->y - pt2->y;


   ret = (dx * dx) + (dy * dy);

   if (squared == MATH_DIST_ACTUAL) {
        ret =  (long) sqrt ((float)ret);
   }

   return ret;
}


int roadmap_math_distance
       (const RoadMapPosition *position1, const RoadMapPosition *position2) {

   float x;
   float y;


   x = RoadMapContext.units->unit_per_longitude
          * (position1->longitude - position2->longitude);
   y = RoadMapContext.units->unit_per_latitude
          * (position1->latitude  - position2->latitude);

   return (int) sqrt ((x * x) + (y * y));
}


/* Take a number followed by ft/mi/m/km, and converts it to current units. */
int roadmap_math_distance_convert(const char *string, int *was_explicit)
{
    char *suffix;
    float distance;
    RoadMapUnits *my_units, *other_units;
    int had_units = 1;

    my_units = RoadMapContext.units;
    if (my_units == &RoadMapMetricSystem) {
        other_units = &RoadMapImperialSystem;
    } else {
        other_units = &RoadMapMetricSystem;
    }

    distance = (float)strtol (string, &suffix, 10);

    while (*suffix && isspace(*suffix)) suffix++;

    if (*suffix) {
        if (0 == strcasecmp(suffix, my_units->length)) {
            /* Nothing to do, hopefully this is the usual case. */
        } else if (0 == strcasecmp(suffix, my_units->trip_distance)) {
            distance *= my_units->to_trip_unit;
        } else if (0 == strcasecmp(suffix, other_units->length)) {
            distance /= other_units->cm_to_unit;
            distance *= my_units->cm_to_unit;
        } else if (0 == strcasecmp(suffix, other_units->trip_distance)) {
            distance *= other_units->to_trip_unit;
            distance /= other_units->cm_to_unit;
            distance *= my_units->cm_to_unit;
        } else {
            roadmap_log (ROADMAP_WARNING,
                "dropping unknown units '%s' from '%s'", suffix, string);
            had_units = 0;
        }
    } else {
        had_units = 0;
    }

    if (was_explicit) *was_explicit = had_units;

    return (int)distance;
}


char *roadmap_math_distance_unit (void) {

    return RoadMapContext.units->length;
}


char *roadmap_math_trip_unit (void) {

    return RoadMapContext.units->trip_distance;
}


char *roadmap_math_speed_unit (void) {

    return RoadMapContext.units->speed;
}

int roadmap_math_distance_to_current(int distance){
	return (int)(distance / RoadMapContext.units->to_screen_unit);
}

int roadmap_math_to_trip_distance (int distance) {

    return distance / RoadMapContext.units->to_trip_unit;
}

int roadmap_math_to_trip_distance_tenths (int distance) {

    return (10 * distance) / RoadMapContext.units->to_trip_unit;
}


int  roadmap_math_get_distance_from_segment
        (const RoadMapPosition *position,
         const RoadMapPosition *position1,
         const RoadMapPosition *position2,
               RoadMapPosition *intersection,
                           int *which) {

   int distance;
   int minimum;

   float x1;
   float y1;
   float x2;
   float y2;
   float x3;
   float y3;


   /* Compute the coordinates relative to the "position" point. */

   x1 = RoadMapContext.units->unit_per_longitude
           * (position->longitude - position1->longitude);
   y1 = RoadMapContext.units->unit_per_latitude
           * (position->latitude  - position1->latitude);

   x2 = RoadMapContext.units->unit_per_longitude
           * (position->longitude - position2->longitude);
   y2 = RoadMapContext.units->unit_per_latitude
           * (position->latitude  - position2->latitude);


   /* Compute the coordinates of the intersection with the perpendicular. */

   if ((x1 - x2 > -1.0) && (x1 - x2 < 1.0)) {

      /* The two points are very close, or the line is quasi vertical:
       * approximating the line to a vertical one is good enough.
       */
      x3 = (x1 + x2) / 2;
      y3 = 0.0;

      if (intersection != NULL) {
         intersection->longitude =
            (position1->longitude + position2->longitude) / 2;
      }

   } else {

      /* Equation of the line: */

      float a = (y1 - y2) / (x1 - x2);
      float b = y1 - a * x1;

      /* The equation of the perpendicular is: y = - (x / a). */

      y3 = (float)(b / ((a * a) + 1.0));
      x3 = -a * y3;

      if (intersection != NULL) {
         intersection->longitude =
            position1->longitude + (int)(((x1 - x3)
                    * (position2->longitude - position1->longitude))
                       / (x1 - x2));
      }
   }


   if ((((x1 >= x3) && (x3 >= x2)) || ((x1 <= x3) && (x3 <= x2))) &&
       (((y1 >= y3) && (y3 >= y2)) || ((y1 <= y3) && (y3 <= y2)))) {

      /* The intersection point is in the segment. */

      if (intersection != NULL) {
         if ((y1 - y2 > -1.0) && (y1 - y2 < 1.0)) {

            intersection->latitude =
               (position1->latitude + position2->latitude) / 2;

         } else {

            intersection->latitude =
               position1->latitude + (int)(((y1 - y3)
                      * (position2->latitude - position1->latitude))
                         / (y1 - y2));
         }
      }

      if (which != NULL) *which = 0;  /* neither endpoint is closest */

      return (int) sqrt ((x3 * x3) + (y3 * y3));
   }

   /* The intersection point is not in the segment: use the distance to
    * the closest segment's endpoint.
    */
   minimum  = (int) sqrt ((x1 * x1) + (y1 * y1));
   distance = (int) sqrt ((x2 * x2) + (y2 * y2));

   if (distance < minimum) {

      if (intersection != NULL) *intersection = *position2;

      if (which != NULL) *which = 2;  /* endpoint 2 is closest */

      return distance;
   }

   if (intersection != NULL) *intersection = *position1;

   if (which != NULL) *which = 1;  /* endpoint 1 is closest */

   return minimum;
}


int roadmap_math_to_speed_unit (int knots) {

    return (int) (knots * RoadMapContext.units->speed_per_knot);
}

int roadmap_math_to_kph(int knots){
   if (roadmap_math_is_metric()){
         return roadmap_math_to_speed_unit(knots);
   }
   else{
         return (int) (knots*1.852F);
   }
}


float roadmap_math_meters_p_second_to_speed_unit (float meters_per_second) {

    return (float) (meters_per_second * RoadMapContext.units->speed_per_m_p_s);
}

int roadmap_math_to_current_unit (int value, const char *unit) {

    if (strcasecmp (unit, "cm") == 0) {

        static int PreviousValue = 0;
        static int PreviousResult = 0;

        if (value != PreviousValue) {
            PreviousResult = (int) (RoadMapContext.units->cm_to_unit * value);
            PreviousValue = value;
        }
        return PreviousResult;
    }

    roadmap_log (ROADMAP_ERROR, "unsupported unit '%s'", unit);
    return value;
}


int roadmap_math_to_cm (int value) {

    static int PreviousValue = 0;
    static int PreviousResult = 0;

    if (value != PreviousValue) {
        PreviousResult = (int) (value / RoadMapContext.units->cm_to_unit);
        PreviousValue = value;
    }

    return PreviousResult;
}


void roadmap_math_screen_edges (RoadMapArea *area) {

   *area = RoadMapContext.current_screen;
}

void roadmap_math_displayed_screen_edges (RoadMapArea *area) {
   //it is more efficient to separate this code from roadmap_math_set_orientation() which is called more frequently
   if ((RoadMapContext.orientation != 0) || RoadMapContext._is3D_projection != PROJECTION_MODE_NONE) {
      int i;
      RoadMapGuiPoint point;
      RoadMapPosition position[4];
      *area = RoadMapContext.upright_screen;
      point.x = 0;
      if (RoadMapContext._is3D_projection == PROJECTION_MODE_3D_NON_OGL)
         point.y = roadmap_bar_top_height();
      else
         point.y = (RoadMapContext.height + RoadMapContext._3D_horizon) *3/32;
      roadmap_math_to_position (&point, position, 1);
      point.x = RoadMapContext.width;
      roadmap_math_to_position (&point, position+1, 1);
      point.y = RoadMapContext.height;
      roadmap_math_to_position (&point, position+2, 1);
      point.x = 0;
      roadmap_math_to_position (&point, position+3, 1);
      for (i = 0; i < 4; ++i) {
         if (position[i].longitude > area->east) {
            area->east = position[i].longitude;
         }
         if (position[i].longitude < area->west) {
            area->west = position[i].longitude;
         }
         if (position[i].latitude < area->south) {
            area->south = position[i].latitude;
         }
         if (position[i].latitude > area->north) {
            area->north = position[i].latitude;
         }
      }
   } else {
      roadmap_math_screen_edges(area);
   }
}

void roadmap_math_displayed_screen_coordinates(RoadMapPosition position[5]){
     RoadMapGuiPoint point;
     point.x = 0;
     if (RoadMapContext._is3D_projection == PROJECTION_MODE_3D_NON_OGL)
         point.y = roadmap_bar_top_height();
     else
         point.y = (RoadMapContext.height + RoadMapContext._3D_horizon) *3/32;
     roadmap_math_to_position (&point, &position[0], 1);
     point.x = RoadMapContext.width;
     roadmap_math_to_position (&point, &position[1], 1);
     point.y = RoadMapContext.height - roadmap_bar_bottom_height();
     roadmap_math_to_position (&point, &position[2], 1);
     point.x = 0;
     roadmap_math_to_position (&point, &position[3], 1);
     point.x = RoadMapContext.center_x;
     point.y = RoadMapContext.center_y;
     roadmap_math_to_position (&point, &position[4], 1);

}


void roadmap_math_to_area (const RoadMapGuiRect *rect, RoadMapArea *area) {
   RoadMapGuiPoint point;
   RoadMapPosition position[4];
   RoadMapPosition center;
   int i;
   
   
   point.x = (rect->maxx + rect->minx)/2;
   point.y = (rect->maxy + rect->miny)/2;
   roadmap_math_to_position(&point, &center, 0);
   area->west = center.longitude;
   area->north = center.latitude;
   area->east = center.longitude;
   area->south = center.latitude;
   
   
   point.x = rect->minx ;
   point.y = rect->miny ;
   roadmap_math_to_position(&point, position, 0);
   
   point.y = rect->maxy ;
   roadmap_math_to_position(&point, position +1, 0);
   
   point.x = rect->maxx ;
   roadmap_math_to_position(&point, position +2, 0);
   
   point.y = rect->miny ;
   roadmap_math_to_position(&point, position +3, 0);
   
   
   for (i = 0; i < 4; ++i) {
      if (position[i].longitude > area->east) {
         area->east = position[i].longitude;
      }
      if (position[i].longitude < area->west) {
         area->west = position[i].longitude;
      }
      if (position[i].latitude < area->south) {
         area->south = position[i].latitude;
      }
      if (position[i].latitude > area->north) {
         area->north = position[i].latitude;
      }
   }
}


int  roadmap_math_area_contains(RoadMapArea *a, RoadMapArea *b) {
   if (b->west < a->west) return 0;
   if (b->east > a->east) return 0;
   if (b->south < a->south) return 0;
   if (b->north > a->north) return 0;

   return 1;
}


int  roadmap_math_street_address (const char *image, int length) {

   int i;
   int digit;
   int result = 0;

   if ((length > 8) &&
       ((image[0] == 'W') || (image[0] == 'E') ||
        (image[0] == 'S') || (image[0] == 'N'))) {

       /* This is a very special case: the street number is organized
        * like a geographical position (longitude/latitude). Skip
        * The west/north flags and format the number accordingly.
        */
       int separator = 0;
       int part1 = 0;
       int part2 = 0;
       int multiplier = 10;


       for (i = length - 1; i > 0; --i) {

          if ((image[i] == 'W') || (image[i] == 'E') ||
              (image[i] == 'S') || (image[i] == 'N') || (image[i] == '-')) {

             separator = i;
          }
       }

       for (i = 1; i < separator; ++i) {
          if (image[i] < '0' || image[i] > '9') {
             roadmap_log
                (ROADMAP_WARNING, "bad numerical character %c", image[i]);
          }
          part1 = (part1 * 10) + (image[i] - '0');
       }

       for (i = separator + 1; i < length; ++i) {
          if (image[i] < '0' || image[i] > '9') {
             roadmap_log
                (ROADMAP_ERROR, "bad numerical character %c", image[i]);
          }
          part2 = (part2 * 10) + (image[i] - '0');
          multiplier *= 10;
       }

       return (part1 * multiplier) + part2;
   }

   for (i = 0; i < length; i++) {

      digit = image[i];

      if ((digit < '0') || (digit > '9')) {

         if (length >= 9) {
            continue;
         } else if (digit == '-' || digit == ' ') {
            continue;
         } else if (digit >= 'A' && digit <= 'Z') {
            digit = '1' + digit - 'A';
         } else if (digit >= 'a' && digit <= 'z') {
            digit = '1' + digit - 'a';
         } else  {
            roadmap_log (ROADMAP_ERROR, "bad numerical character %c", digit);
            continue;
         }
      }
      result = (result * 10) + (digit - '0');
   }

   return result;
}


int roadmap_math_intersection (RoadMapPosition *from1,
                               RoadMapPosition *to1,
                               RoadMapPosition *from2,
                               RoadMapPosition *to2,
                               RoadMapPosition *intersection) {

   float a1,b1;
   float a2,b2;

   if (from1->longitude == to1->longitude) {

      a1 = 0;
      b1 = (float)from1->latitude;
   } else {
      a1 = (float)(1.0 * (from1->latitude - to1->latitude) /
         (from1->longitude - to1->longitude));
      b1 = (float)(from1->latitude - 1.0 * a1 * from1->longitude);
   }

   if ((from2->longitude - to2->longitude) == 0) {

      a2 = 0;
      b2 = (float)from2->latitude;
   } else {
      a2 = (float)(1.0 * (from2->latitude - to2->latitude) /
         (from2->longitude - to2->longitude));
      b2 = (float)(from2->latitude - 1.0 * a2 * from2->longitude);
   }

   if (a1 == a2) return 0;

   intersection->longitude = (int) ((b1 - b2) / (a2 - a1));
   intersection->latitude = (int) (b1 + intersection->longitude * a1);

   return 1;
}

/* this routine isn't accurate for segments that are either very flat
 * or very steep (i.e. within a few degrees of 0 or 90).
 */
int roadmap_math_screen_intersect (RoadMapGuiPoint *f1, RoadMapGuiPoint *t1,
                           RoadMapGuiPoint *f2, RoadMapGuiPoint *t2,
                           RoadMapGuiPoint *isect) {

#if USE_FLOAT  /* for reference, until we're sure integer version works */
   float a1,b1;
   float a2,b2;
   float x;

   if (f1->x == t1->x) {

      a1 = 0;
      b1 = f1->y;
   } else {
      a1 = 1.0 * (f1->y - t1->y) / (f1->x - t1->x);
      b1 = f1->y - 1.0 * a1 * f1->x;
   }

   if ((f2->x - t2->x) == 0) {
      a2 = 0;
      b2 = f2->y;
   } else {
      a2 = 1.0 * (f2->y - t2->y) / (f2->x - t2->x);
      b2 = f2->y - 1.0 * a2 * f2->x;
   }

   if (a1 == a2) return 0;

   x = (b1 - b2) / (a2 - a1);
   if (fabs(a1) < fabs(a2)) {
      isect->y = (int) (b1 + x * a1);
   } else {
      isect->y = (int) (b2 + x * a2);
   }
   isect->x = (int)x;
#else
   long a1,b1;
   long a2,b2;
   long x;

   if (f1->x == t1->x) {
      a1 = 0;
      b1 = 1024 * f1->y;
   } else {
      a1 = 1024 * (f1->y - t1->y) / (f1->x - t1->x);
      b1 = 1024 * f1->y - a1 * f1->x;
   }

   if (f2->x == t2->x) {
      a2 = 0;
      b2 = 1024 * f2->y;
   } else {
      a2 = 1024 * (f2->y - t2->y) / (f2->x - t2->x);
      b2 = 1024 * f2->y - a2 * f2->x;
   }

   if (a1 == a2) return 0;

   x = (b1 - b2) / (a2 - a1);
   if (abs(a1) < abs(a2)) {
      isect->y = (b1 + x * a1) / 1024;
   } else {
      isect->y = (b2 + x * a2) / 1024;
   }
   isect->x = (short)x;
#endif

   return 1;
}



int roadmap_math_compare_points (const RoadMapPosition *p1,
                                 const RoadMapPosition *p2) {

   if ((p1->longitude == p2->longitude) &&
         (p1->latitude == p2->latitude))
      return 0;

   if (p1->longitude < p2->longitude) return -1;
   if (p2->longitude > p1->longitude) return 1;

   if (p1->latitude < p2->latitude) {
      return -1;
   } else {
      return 1;
   }
}


int roadmap_math_delta_direction (int direction1, int direction2) {

    int delta = direction2 - direction1;

    while (delta > 180)  delta -= 360;
    while (delta < -180) delta += 360;

    if (delta < 0) delta = 0 - delta;

    while (delta >= 360) delta -= 360;

    return delta;
}


void roadmap_math_set_context (const RoadMapPosition *position, zoom_t zoom) {

   RoadMapContext.center = *position;

   if (zoom < MAX_ZOOM_OUT) {
      RoadMapContext.zoom = zoom;
   }

   if (RoadMapContext.zoom <= 1) {
      RoadMapContext.zoom = 2;
   }

   roadmap_math_compute_scale ();
}


void roadmap_math_get_context (RoadMapPosition *position, zoom_t *zoom) {

   *position = RoadMapContext.center;
   *zoom = RoadMapContext.zoom;
}


int roadmap_math_calc_line_length (const RoadMapPosition *position,
                                   const RoadMapPosition *from_pos,
                                   const RoadMapPosition *to_pos,
                                   int                    first_shape,
                                   int                    last_shape,
                                   RoadMapShapeItr        shape_itr,
                                   int *total_length) {

   RoadMapPosition from;
   RoadMapPosition to;
   RoadMapPosition intersection;
   int current_length = 0;
   int length_result = 0;
   int smallest_distance = 0x7fffffff;
   int distance;
   int i;

   if (first_shape <= -1) {

      from = *from_pos;
      to = *to_pos;
   } else {

      from = *from_pos;
      to   = *from_pos;

      for (i = first_shape; i <= last_shape; i++) {

         if (shape_itr== NULL)
            roadmap_shape_get_position(i, &to);
         else
            shape_itr (i, &to);

         distance =
            roadmap_math_get_distance_from_segment
            (position, &from, &to, &intersection, NULL);

         if (distance < smallest_distance) {
            smallest_distance = distance;
            length_result = current_length +
               roadmap_math_distance (&from, &intersection);
         }

         current_length += roadmap_math_distance (&from, &to);
         from = to;
      }

      to = *to_pos;
   }

   distance =
      roadmap_math_get_distance_from_segment
      (position, &from, &to, &intersection, NULL);

   if (distance < smallest_distance) {

      length_result = current_length +
                        roadmap_math_distance (&from, &intersection);
   }

   current_length += roadmap_math_distance (&from, &to);

   if (total_length) *total_length = current_length;

   return length_result;
}



BOOL roadmap_math_is_metric(void){
   return (RoadMapContext.units->value == ROADMAP_BASE_METRIC);
}

void roadmap_math_set_tile_visibility (int mode) {
   RoadMapMathTileMode = mode;
}
