/* roadmap_canvas.c - manage the canvas that is used to draw the map.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
 *   Copyright 2008 Morten Bek Ditlevsen
 *   Copyright 2008 Avi R.
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
 *   See roadmap_canvas.h.
 */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#import <Foundation/Foundation.h>
#import <UIKit/UIView.h>
#import <Foundation/NSString.h>



#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_gui.h"
#include "roadmap_device_events.h"

#include "roadmap_canvas.h"
#include "roadmap_iphonecanvas.h"
#include "colors.h"
#include "roadmap_path.h"
#include "roadmap_lang.h"

#define FONT_INC_FACTOR 1.2 //Trying to compensate the high PPI of iPhone screen
#define FONT_DEFAULT_SIZE 14



struct RoadMapColor {
   float r;
   float g;
   float b;
   float a;
};

struct roadmap_canvas_pen {

   struct roadmap_canvas_pen *next;

   char  *name;
   int style;
   float lineWidth;
   struct RoadMapColor stroke;
};


static struct roadmap_canvas_pen *RoadMapPenList = NULL;

static RoadMapCanvasView   *RoadMapDrawingArea;
static CGContextRef        RoadMapGc;
static CGSize              RoadMapGcSize;
static BOOL                gs_bShouldAcceptLayout = TRUE;

static RoadMapPen CurrentPen;

static NSMutableArray *CordingPoints; //Number of cording fingers. We actually use max of 3


/* The canvas callbacks: all callbacks are initialized to do-nothing
 * functions, so that we don't care checking if one has been setup.
 */
static void roadmap_canvas_ignore_mouse (RoadMapGuiPoint *point) {}

static RoadMapCanvasMouseHandler RoadMapCanvasMouseButtonPressed =
                                     roadmap_canvas_ignore_mouse;

static RoadMapCanvasMouseHandler RoadMapCanvasMouseButtonReleased =
                                     roadmap_canvas_ignore_mouse;

static RoadMapCanvasMouseHandler RoadMapCanvasMouseMoved =
                                     roadmap_canvas_ignore_mouse;

static RoadMapCanvasMouseHandler RoadMapCanvasMouseScroll =
                                     roadmap_canvas_ignore_mouse;



static void roadmap_canvas_ignore_configure (void) {}

static RoadMapCanvasConfigureHandler RoadMapCanvasConfigure =
                                     roadmap_canvas_ignore_configure;

static void roadmap_canvas_convert_points
                (CGPoint *cgpoints, RoadMapGuiPoint *points, int count) {

    RoadMapGuiPoint *end = points + count;

    while (points < end) {
        cgpoints->x = points->x;
        cgpoints->y = points->y;
        cgpoints += 1;
        points += 1;
    }
}

static void roadmap_canvas_convert_rects
                (CGRect *rects, RoadMapGuiPoint *points, int count) {

    RoadMapGuiPoint *end = points + count;

    while (points < end) {
        rects->origin.x = points->x;
        rects->origin.y = points->y;
        rects->size.width = 1.0;
        rects->size.height = 1.0;
        rects += 1;
        points += 1;
    }
}

void set_fast_draw (int fast_draw) {
    if (fast_draw) {
      CGContextSetAllowsAntialiasing (RoadMapGc, NO);
      CGContextSetInterpolationQuality  (RoadMapGc, kCGInterpolationNone);
      CGContextSetLineCap(RoadMapGc, kCGLineCapButt);
      CGContextSetLineJoin(RoadMapGc, kCGLineJoinMiter);
   }
}

void end_fast_draw (int fast_draw) {
    if (fast_draw) {
      CGContextSetAllowsAntialiasing (RoadMapGc, YES);
      CGContextSetInterpolationQuality  (RoadMapGc, kCGInterpolationDefault);
      CGContextSetLineCap(RoadMapGc, kCGLineCapRound);
      CGContextSetLineJoin(RoadMapGc, kCGLineJoinRound);
   }
}

void roadmap_canvas_get_text_extents
        (const char *text, int size, int *width,
            int *ascent, int *descent, int *can_tilt) 
{
   NSString *string;
   CGSize   textSize;
   UIFont   *font;
   
   if (text == NULL) {
      *width = 0;
      *ascent = 0;
      *descent = 0;
      if (can_tilt)
         *can_tilt = 1;
      roadmap_log (ROADMAP_DEBUG,
                   "roadmap_canvas_get_text_extents - null text");
      return;
   }
              
   
   if (size == -1)
   {
      size = FONT_DEFAULT_SIZE;
   }
   
   float size_f = (size * FONT_INC_FACTOR);
   font = [UIFont systemFontOfSize:size_f];

   string = [NSString stringWithUTF8String:text];
   
   if (string == NULL) {
      *width = 0;
      *ascent = 0;
      *descent = 0;
      if (can_tilt)
         *can_tilt = 1;
      
      roadmap_log (ROADMAP_ERROR,
                   "roadmap_canvas_get_text_extents - invalid text");
      
      return;
   }
   
   textSize = [string sizeWithFont:font];
   *width = (int)textSize.width + 1;
   *ascent = (int)font.ascender - 1;
   *descent = 0;// abs((int)font.descender);
}

RoadMapPen roadmap_canvas_select_pen (RoadMapPen pen) {
   float lengths[2] = {2.0f, 2.0f};
   RoadMapPen old_pen = CurrentPen;
   CurrentPen = pen;

   if (pen->style == 2) { /* dashed */
       CGContextSetLineDash(RoadMapGc, 0.0f,  lengths, 2);
   } else {
       CGContextSetLineDash(RoadMapGc, 0.0f,  NULL, 0);
   }
   CGContextSetLineWidth(RoadMapGc, pen->lineWidth);
   CGContextSetRGBStrokeColor( RoadMapGc, pen->stroke.r, pen->stroke.g, pen->stroke.b, pen->stroke.a);
   CGContextSetRGBFillColor( RoadMapGc, pen->stroke.r, pen->stroke.g, pen->stroke.b, pen->stroke.a);

   return old_pen;
}

RoadMapPen roadmap_canvas_create_pen (const char *name) {
   struct roadmap_canvas_pen *pen;

   for (pen = RoadMapPenList; pen != NULL; pen = pen->next) {
      if (strcmp(pen->name, name) == 0) break;
   }

   if (pen == NULL) {

      /* This is a new pen: create it. */
      pen = (struct roadmap_canvas_pen *)
                malloc (sizeof(struct roadmap_canvas_pen));
      roadmap_check_allocated(pen);

      pen->name = strdup (name);
      pen->style = 1; //solid
      pen->stroke.r = 0.0;
      pen->stroke.g = 0.0;
      pen->stroke.b = 0.0;
      pen->stroke.a = 1.0;
      pen->lineWidth = 1.0;
      pen->next = RoadMapPenList;

      RoadMapPenList = pen;
   }

   roadmap_canvas_select_pen (pen);

   return pen;
}

void roadmap_canvas_set_foreground (const char *color) {

    float r, g, b, a; 
    a = 1.0;
    int high, i, low;

    if (*color == '#') {
        int ir, ig, ib, ia;
        int count;
        count = sscanf(color, "#%2x%2x%2x%2x", &ir, &ig, &ib, &ia);
        r = ir/255.0;
        g = ig/255.0;
        b = ib/255.0;
        
        if (count == 4)
        {
            a = ia/255.0;
        }
    } else {
        /* Do binary search on color table */
        for (low=(-1), high=sizeof(color_table)/sizeof(color_table[0]);
                high-low > 1;) {
            i = (high+low) / 2;
            if (strcmp(color, color_table[i].name) <= 0) high = i;
            else low = i;
        }

        if (!strcmp(color, color_table[high].name)) {
            r = color_table[high].r / 255.0;
            g = color_table[high].g / 255.0;
            b = color_table[high].b / 255.0;
        } else {
            r = 0.0;
            g = 0.0;
            b = 0.0;
        }
    }

    CurrentPen->stroke.r = r;
    CurrentPen->stroke.g = g;
    CurrentPen->stroke.b = b;
    CurrentPen->stroke.a = a;  

	roadmap_canvas_select_pen(CurrentPen);
}

void roadmap_canvas_set_linestyle (const char *style) {
   if (strcasecmp (style, "dashed") == 0) {
       CurrentPen->style = 2;
   } else {
       CurrentPen->style = 1;
   }
}

void roadmap_canvas_set_thickness  (int thickness) {
   if (thickness < 0) {
      thickness = 0;
   }
   
   CurrentPen->lineWidth = thickness * 1.0;
}

void roadmap_canvas_erase (void) {
   /* 'erase' means fill the canvas with the foreground color */
   CGContextFillRect(RoadMapGc, [RoadMapDrawingArea bounds]);
}

void roadmap_canvas_erase_area (const RoadMapGuiRect *rect)
{
	CGRect imageRect = CGRectMake(rect->minx * 1.0f, rect->miny * 1.0f, (rect->maxx - rect->minx) * 1.0f, (rect->maxy - rect->miny) * 1.0f);
	CGContextFillRect(RoadMapGc, imageRect);
}

void roadmap_canvas_draw_string_size (RoadMapGuiPoint *position,
                                      int corner,
                                      int size,
                                      const char *text) {

   int x;
   int y;
   int text_width;
   int text_ascent;
   int text_descent;
   int text_height;
   
   roadmap_canvas_get_text_extents 
         (text, size, &text_width, &text_ascent, &text_descent, NULL);
   
   text_height = text_ascent + text_descent;
   
   switch (corner) {
      
   case ROADMAP_CANVAS_TOPLEFT:
      y = position->y;
      x = position->x;
      break;
      
   case ROADMAP_CANVAS_TOPRIGHT:
      y = position->y;
      x = position->x - text_width;
      break;
      
   case ROADMAP_CANVAS_BOTTOMRIGHT:
      y = position->y - text_height;
      x = position->x - text_width;
      break;
      
   case ROADMAP_CANVAS_BOTTOMLEFT:
      y = position->y - text_height;
      x = position->x;
      break;
      
   case ROADMAP_CANVAS_CENTER:
      y = position->y - (text_height / 2);
      x = position->x - (text_width / 2);
      break;
      
   default:
      return;
   }

   RoadMapGuiPoint start = {x, y+text_height};
   roadmap_canvas_draw_string_angle (&start, position, 0, size, text);
}


void roadmap_canvas_draw_string  (RoadMapGuiPoint *position,
                                  int corner,
                                  const char *text) {

   roadmap_canvas_draw_string_size (position, corner, -1, text);
}

void roadmap_canvas_draw_string_context (const RoadMapGuiPoint *position,
                                       RoadMapGuiPoint *center,
                                       int angle, int size,
                                       const char *text,
                                       CGContextRef context)
{
   if (size == -1) {
      size = FONT_DEFAULT_SIZE;
   }
   CGAffineTransform rotatedTrans, counterRotateTrans;
   NSString *string;
   float angle_f = (angle) * M_PI / 180.0f;
   float counter_angle_f = (angle * -1.0f) * M_PI / 180.0f;
   float x = position->x * 1.0f;
   float y = position->y * 1.0f;
   float size_f = (size * FONT_INC_FACTOR);
   UIFont *font = [UIFont systemFontOfSize:size_f];
   
   //set rotation
   rotatedTrans = CGAffineTransformMakeRotation(angle_f);
   counterRotateTrans = CGAffineTransformMakeRotation(counter_angle_f);
   //save state
   CGContextSaveGState(context);
   CGContextConcatCTM(context, rotatedTrans);
   //change active context
   UIGraphicsPushContext (context);
   //draw
   string = [NSString stringWithUTF8String:text];
   CGPoint point = CGPointApplyAffineTransform (CGPointMake(x, y), counterRotateTrans);
   point.y -= (int)font.ascender + 1;
   [string drawAtPoint:point withFont:[UIFont systemFontOfSize:size_f]];
   //restore state
   CGContextRestoreGState(context);
   //restore context
   UIGraphicsPopContext();
}

void roadmap_canvas_draw_string_angle (const RoadMapGuiPoint *position,
                                       RoadMapGuiPoint *center,
                                       int angle, int size,
                                       const char *text)
{
   roadmap_canvas_draw_string_context (position, center, angle, size, text, RoadMapGc);
}


void roadmap_canvas_draw_multiple_points (int count, RoadMapGuiPoint *points) {
   CGRect rects[1024];

   while (count > 1024) {
       roadmap_canvas_convert_rects (rects, points, 1024);
       CGContextFillRects(RoadMapGc, rects, 1024);
       points += 1024;
       count -= 1024;
   }
   roadmap_canvas_convert_rects (rects, points, count);
   CGContextFillRects(RoadMapGc, rects, count);
}


void roadmap_canvas_draw_multiple_lines (int count, int *lines, RoadMapGuiPoint *points, int fast_draw) {
   
   int i;
   int count_of_points;
   CGPoint cgpoints[1024];
   
   set_fast_draw (fast_draw);
   
   for (i = 0; i < count; ++i) {
      
      count_of_points = *lines;
      
      while (count_of_points > 1024) {
         roadmap_canvas_convert_points (cgpoints, points, 1024);
         CGContextBeginPath(RoadMapGc);
         CGContextAddLines(RoadMapGc, cgpoints, 1024);
         CGContextStrokePath(RoadMapGc);
         points += 1023;
         count_of_points -= 1023;
      }
      
      roadmap_canvas_convert_points (cgpoints, points, count_of_points);
      
      CGContextBeginPath(RoadMapGc);
      CGContextAddLines(RoadMapGc, cgpoints, count_of_points);
      CGContextStrokePath(RoadMapGc);
      
      points += count_of_points;
      lines += 1;
   }
   
   end_fast_draw (fast_draw);
}

void roadmap_canvas_draw_multiple_polygons
         (int count, int *polygons, RoadMapGuiPoint *points, int filled,
            int fast_draw) {
            
    int i;
    int count_of_points;
    CGPoint cgpoints[1024];
    
   set_fast_draw (fast_draw);
   
	for (i = 0; i < count; ++i) {

        count_of_points = *polygons;

        while (count_of_points > 1024) {
            roadmap_canvas_convert_points (cgpoints, points, 1024);
            CGContextBeginPath(RoadMapGc);
            CGContextAddLines(RoadMapGc, cgpoints, 1024);
            CGContextClosePath(RoadMapGc);
            if (filled)
                CGContextFillPath(RoadMapGc);
            else
                CGContextStrokePath(RoadMapGc);
            points += 1023;
            count_of_points -= 1023;
        }

        roadmap_canvas_convert_points (cgpoints, points, count_of_points);

        CGContextBeginPath(RoadMapGc);
        CGContextAddLines(RoadMapGc, cgpoints, count_of_points);
        CGContextClosePath(RoadMapGc);
        if (filled)
            CGContextFillPath(RoadMapGc);
        else
            CGContextStrokePath(RoadMapGc);

        points += count_of_points;
        polygons += 1;
    }
    
   end_fast_draw (fast_draw);
}

void roadmap_canvas_draw_multiple_circles
        (int count, RoadMapGuiPoint *centers, int *radius, int filled,
            int fast_draw) {

   int i;

   set_fast_draw (fast_draw);
   
   for (i = 0; i < count; ++i) {

       int r = radius[i];

       CGRect circle = CGRectMake(centers[i].x-r, centers[i].y-r, r*2, r*2);
       CGContextAddEllipseInRect(RoadMapGc, circle);
       if (filled)
           CGContextFillEllipseInRect(RoadMapGc, circle);
       else
           CGContextStrokeEllipseInRect(RoadMapGc, circle);

   }
   
   end_fast_draw (fast_draw);
}

void roadmap_canvas_register_configure_handler
                    (RoadMapCanvasConfigureHandler handler) {

   RoadMapCanvasConfigure = handler;
}

void roadmap_canvas_register_button_pressed_handler
                    (RoadMapCanvasMouseHandler handler) {

   RoadMapCanvasMouseButtonPressed = handler;
}


void roadmap_canvas_register_button_released_handler
                    (RoadMapCanvasMouseHandler handler) {
       
   RoadMapCanvasMouseButtonReleased = handler;
}


void roadmap_canvas_register_mouse_move_handler
                    (RoadMapCanvasMouseHandler handler) {

   RoadMapCanvasMouseMoved = handler;
}


void roadmap_canvas_register_mouse_scroll_handler
                    (RoadMapCanvasMouseHandler handler) {
   RoadMapCanvasMouseScroll = handler;
}



int roadmap_canvas_width (void) {
   if (RoadMapDrawingArea == NULL) {
      return 0;
   }
   return RoadMapGcSize.width;
}

int roadmap_canvas_agg_is_landscape()
{
   if (RoadMapDrawingArea == NULL) {
      return 0;
   }
   
   return ( [RoadMapDrawingArea frame].size.width > [RoadMapDrawingArea frame].size.height );
}

int roadmap_canvas_height (void) {
   if (RoadMapDrawingArea == NULL) {
      return 0;
   }
   return RoadMapGcSize.height;
}

void roadmap_canvas_refresh (void) {
   [RoadMapDrawingArea setNeedsDisplay];
}

void roadmap_canvas_save_screenshot (const char* filename) {
    if (RoadMapGc)
    {
        CGImageRef image = CGBitmapContextCreateImage (RoadMapGc);
        // TODO: save image
        CGImageRelease(image);
    }
}

void roadmap_canvas_set_opacity (int opacity)
{
	if (!CurrentPen) return;
	
	if ((opacity <= 0) || (opacity >= 255)) {
      opacity = 255;
	}
   
	CurrentPen->stroke.a = opacity/255.0;
	roadmap_canvas_select_pen(CurrentPen);
}

RoadMapImage roadmap_canvas_new_image (int width, int height) {

   CGRect rect;
   CGLayerRef layer;
   CGContextRef layerContext;

   rect  = CGRectMake (0, 0, width, height);
   
   layer = CGLayerCreateWithContext (RoadMapGc, rect.size, NULL);
   
   layerContext = CGLayerGetContext (layer);

   return ((RoadMapImage) layer);
}

static RoadMapImage load_image (const char *full_name) {
   
   CGRect rect;
   CGImageRef image;
   CGLayerRef layer;
   CGContextRef layerContext;
	
	image = [UIImage imageWithContentsOfFile:[NSString stringWithUTF8String:full_name]].CGImage;
   
   if (!image) return NULL;
   
   rect  = CGRectMake (0, 0, CGImageGetWidth(image), CGImageGetHeight(image));
   
   layer = CGLayerCreateWithContext (RoadMapGc, rect.size, NULL);
   
   layerContext = CGLayerGetContext (layer);

   CGContextSaveGState(layerContext);
	CGContextTranslateCTM(layerContext, 0, CGRectGetHeight(rect));
	CGContextScaleCTM(layerContext, 1.0, -1.0);

   CGContextDrawImage (layerContext, rect, image);   
   
   CGContextRestoreGState(layerContext);
   
   return ((RoadMapImage) layer);
}

RoadMapImage roadmap_canvas_load_image (const char *path,
                                        const char* file_name)
{
   char *full_name = roadmap_path_join (path, file_name);
   RoadMapImage image;
   //TODO: should be image = NULL but it breaks the nav panel draw ?!
	
	image = load_image(full_name);


   free (full_name);

   return (image);
}



void roadmap_canvas_image_set_mutable (RoadMapImage src) {}

void roadmap_canvas_draw_image (RoadMapImage image, const RoadMapGuiPoint *pos,
                                int opacity, int mode)
{
   if (image==NULL) {
      roadmap_log (ROADMAP_WARNING, "Image is null");
      return;
   }
	
	//Handle opacity
	if ((mode == IMAGE_SELECTED) || (opacity <= 0) || (opacity >= 255)) {
      opacity = 255;
   }
	CGContextSetAlpha (RoadMapGc,opacity/255.0);
	
   CGRect imageRect = CGRectMake(pos->x * 1.0f,
                  pos->y * 1.0f,
                  roadmap_canvas_image_width(image) * 1.0f,
                  roadmap_canvas_image_height(image) * 1.0f);

   CGContextDrawLayerInRect (RoadMapGc, imageRect, (CGLayerRef) image);
   
   static RoadMapPen selection;
	if (mode == IMAGE_SELECTED) {

      if (!selection) {
         selection = roadmap_canvas_create_pen("selection");
         roadmap_canvas_set_foreground ("#000000");
      }

      RoadMapGuiPoint points[5] = {
         {pos->x, pos->y},
         {pos->x + roadmap_canvas_image_width(image), pos->y},
         {pos->x + roadmap_canvas_image_width(image), pos->y + roadmap_canvas_image_height(image)},
         {pos->x, pos->y + roadmap_canvas_image_height(image)},
         {pos->x, pos->y}};

      int num_points = 5;

      RoadMapPen current = roadmap_canvas_select_pen (selection);
      roadmap_canvas_draw_multiple_lines (1, &num_points, points, 0);
      roadmap_canvas_select_pen (current);
   }
   
   
   CGContextSetAlpha (RoadMapGc,1.0);
}

void roadmap_canvas_copy_image (RoadMapImage dst_image,
                                const RoadMapGuiPoint *pos,
                                const RoadMapGuiRect  *rect,
                                RoadMapImage src_image, int mode)
{
   CGRect dstRect;
   CGPoint point;
   CGContextRef dstContext = CGLayerGetContext ((CGLayerRef) dst_image);
   
   point.x = pos->x;
   point.y = pos->y;

   if (rect) {
      dstRect = CGRectMake(rect->minx * 1.0f, rect->miny * 1.0f, rect->maxx - rect->minx + 1.0f, rect->maxy - rect->miny + 1.0f);
      CGContextSaveGState(dstContext);
      if (mode == CANVAS_COPY_NORMAL) {
	      CGContextClearRect(dstContext, dstRect);
	      //CGContextSetBlendMode(dstContext, kCGBlendModeDestinationAtop);
      }
      CGContextClipToRect(dstContext, dstRect);
      if (mode == CANVAS_COPY_NORMAL) {
         //CGRect rect = CGRectMake (0, 0, CGImageGetWidth(src_image), CGImageGetHeight(src_image));
         //CGContextDrawImage (dstContext, rect, src_image);   
         CGContextDrawLayerAtPoint (dstContext, point, (CGLayerRef) src_image);
      } else {
         CGContextDrawLayerAtPoint (dstContext, point, (CGLayerRef) src_image);
      }
      CGContextRestoreGState(dstContext);
   } else {
      CGContextDrawLayerAtPoint (dstContext, point, (CGLayerRef) src_image);
   }
   
/*   
   if (mode == CANVAS_COPY_NORMAL) {
      //CGContextClearRect(dstContext, dstRect);
      CGContextDrawLayerInRect (dstContext, dstRect, (CGLayerRef) src_image);
   } else {
      CGContextDrawLayerInRect (dstContext, dstRect, (CGLayerRef) src_image);
   }*/
}

void roadmap_canvas_draw_image_text (RoadMapImage image,
                                     const RoadMapGuiPoint *position,
                                     int size, const char *text)
{
   CGContextRef layerContext;
   RoadMapGuiPoint text_pos;
   
   layerContext = CGLayerGetContext ((CGLayerRef) image);   
   
   text_pos.x = position->x;
   text_pos.y = position->y + floor(size * FONT_INC_FACTOR) - 8;
   
   CGContextSetRGBStrokeColor (layerContext, CurrentPen->stroke.r, CurrentPen->stroke.g, CurrentPen->stroke.b, CurrentPen->stroke.a);
   CGContextSetRGBFillColor (layerContext, CurrentPen->stroke.r, CurrentPen->stroke.g, CurrentPen->stroke.b, CurrentPen->stroke.a);
   roadmap_canvas_draw_string_context (&text_pos, NULL, 0, size, text, layerContext);
}

void roadmap_canvas_free_image (RoadMapImage image)
{
   CGLayerRelease ((CGLayerRef) image);
}

int  roadmap_canvas_image_width  (const RoadMapImage image) {

   if (!image) return 0;
   
   CGSize size = CGLayerGetSize((CGLayerRef) image);
   int width = ceil(size.width);
   
   return (width);
}


int  roadmap_canvas_image_height (const RoadMapImage image) {
   
   if (!image) return 0;
   
   CGSize size = CGLayerGetSize((CGLayerRef) image);
   int height = ceil(size.height);
   
   return (height);
}

int  roadmap_canvas_get_thickness  (RoadMapPen pen)
{
	return (pen->lineWidth);
}


void roadmap_canvas_get_cording_pt (RoadMapGuiPoint points[MAX_CORDING_POINTS])
{
	int i;
	UITouch *touch;
	
	for (i = 0; i < MAX_CORDING_POINTS; ++i) {
		if ([CordingPoints count] > i) {
			touch = [CordingPoints objectAtIndex:i];
			points[i].x = [touch locationInView:RoadMapDrawingArea].x;
			points[i].y = [touch locationInView:RoadMapDrawingArea].y;
		} else {
			points[i].x = -1;
			points[i].y = -1;
		}
	}
}

int roadmap_canvas_is_cording()
{
   return ([CordingPoints count] > 1);
}

void roadmap_canvas_cancel_touches()
{
   [RoadMapDrawingArea cancelAllTouches];
}

RoadMapImage roadmap_canvas_image_from_buf( unsigned char* buf, int width, int height, int stride ) { assert(0); return NULL; }

void roadmap_canvas_show_view (UIView *view) {
	[RoadMapDrawingArea addSubview:view];
}

void roadmap_canvas_should_accept_layout (int bAcceptLayout) {
   gs_bShouldAcceptLayout = bAcceptLayout;
}


@implementation RoadMapCanvasView

-(void) setContext
{
   if (RoadMapGc!= NULL) 
      CGContextRelease (RoadMapGc);
      
   RoadMapGcSize = self.bounds.size;
   
   CGColorSpaceRef    imageColorSpace = CGColorSpaceCreateDeviceRGB();

   RoadMapGc = CGBitmapContextCreate(nil, RoadMapGcSize.width, RoadMapGcSize.height, 8, 0, imageColorSpace, kCGImageAlphaPremultipliedFirst);
   CGColorSpaceRelease(imageColorSpace);

   CGContextSetLineCap(RoadMapGc, kCGLineCapRound);
   CGContextSetLineJoin(RoadMapGc, kCGLineJoinRound);
}

-(RoadMapCanvasView *) initWithFrame: (struct CGRect) rect
{
   self = [super initWithFrame: rect];
   [self setClearsContextBeforeDrawing: NO]; //this should improve performance

   RoadMapDrawingArea = self;

   [self setContext];
	
   
   (*RoadMapCanvasConfigure) ();
   
	[self setMultipleTouchEnabled:YES];
	CordingPoints = [[NSMutableArray alloc] initWithCapacity:MAX_CORDING_POINTS];
	
   return self;
}


- (void)layoutSubviews
{
   if ((!gs_bShouldAcceptLayout) ||
      (roadmap_canvas_height() == self.bounds.size.height &&
       roadmap_canvas_width() == self.bounds.size.width))
      return;
   
	[self setContext];
   
   (*RoadMapCanvasConfigure) ();
}


-(void) drawRect: (CGRect) rect
{
    if (RoadMapGc)
    {
        /* CGBitmapContextCreateImage might use copy-on-change
           so if we're lucky no copy is actually made! */
          
		CGImageRef image = CGBitmapContextCreateImage (RoadMapGc);
        CGContextDrawImage (UIGraphicsGetCurrentContext(), [self bounds], image);
        CGImageRelease(image);
    }
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	CGPoint p;
	RoadMapGuiPoint point;
	UITouch *touch;
	
	if ([CordingPoints count] > 0) {
		touch = [CordingPoints objectAtIndex:0];
		p = [touch locationInView:self];
		point.x = (int)p.x;
		point.y = (int)p.y;
		
      (*RoadMapCanvasMouseMoved) (&point);
      (*RoadMapCanvasMouseButtonReleased) (&point);	
	}
	
	[CordingPoints addObjectsFromArray:[touches allObjects]];
   
   if ([CordingPoints count] > 2) {
      //TODO: pick the most far fingers
   }
	
	touch = [CordingPoints objectAtIndex:0];
	p = [touch locationInView:self];
	point.x = (int)p.x;
	point.y = (int)p.y;
	(*RoadMapCanvasMouseButtonPressed) (&point);
	
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{  
	CGPoint p;
	RoadMapGuiPoint point;
	UITouch *touch;
	
	touch = [touches anyObject];
	p = [touch locationInView:self];
	point.x = (int)p.x;
	point.y = (int)p.y;
	
	[CordingPoints removeObjectsInArray:[touches allObjects]];
	
   (*RoadMapCanvasMouseButtonReleased) (&point);
   
	if ([CordingPoints count] > 0) {
		touch = [CordingPoints objectAtIndex:0];
		p = [touch locationInView:self];
		point.x = (int)p.x;
		point.y = (int)p.y;
		(*RoadMapCanvasMouseButtonPressed) (&point);
      (*RoadMapCanvasMouseMoved) (&point);
	}
}


- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{ 	
	CGPoint p;
	RoadMapGuiPoint point;
	UITouch *touch;
	
   if ([CordingPoints count] < 1) {
      [self cancelAllTouches];
      return;
   }
   
	touch = [CordingPoints objectAtIndex:0];
	p = [touch locationInView:self];
	point.x = (int)p.x;
	point.y = (int)p.y;
	
	(*RoadMapCanvasMouseMoved) (&point);
}

- (void)cancelAllTouches
{
   [CordingPoints removeObjectsInArray:CordingPoints];
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
   [CordingPoints removeObjectsInArray:[touches allObjects]];
}

@end

