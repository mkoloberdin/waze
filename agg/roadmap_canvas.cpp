/* roadmap_canvas.cpp - manage the canvas that is used to draw the map with agg
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
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



#ifdef WIN32_PROFILE
#include <C:\Program Files\Windows CE Tools\Common\Platman\sdk\wce500\include\cecap.h>
#endif

#ifndef __SYMBIAN32__
#include <wchar.h>
#endif
#include "agg_rendering_buffer.h"
#include "agg_curves.h"
#include "agg_conv_stroke.h"
#include "agg_conv_contour.h"
#include "agg_conv_transform.h"
#include "agg_rasterizer_scanline_aa.h"
#include "agg_rasterizer_outline_aa.h"
#include "agg_rasterizer_outline.h"
#include "agg_renderer_primitives.h"
#include "agg_ellipse.h"
#include "agg_renderer_scanline.h"
#include "agg_scanline_p.h"
#include "agg_renderer_outline_aa.h"
#include "agg_pixfmt_rgb_packed.h"
#include "agg_path_storage.h"
#include "agg_rounded_rect.h"
#include "font_freetype/agg_font_freetype.h"

#ifdef USE_FRIBIDI
#include <fribidi.h>
#endif

extern "C" {
#include "../roadmap.h"
#include "../roadmap_types.h"
#include "../roadmap_gui.h"
#include "../roadmap_screen.h"
#include "../roadmap_canvas.h"
#include "../roadmap_messagebox.h"
#include "../roadmap_math.h"
#include "../roadmap_config.h"
#include "../roadmap_path.h"
#include "../roadmap_file.h"
}
#include "../roadmap_canvas_agg.h"

EXTERN_C unsigned char *read_png_file(const char* file_name, int *width, int *height,
        int *stride );

#define MAX_THICKNESS 50

#ifdef _WIN32
	#define CANVAS_HD_FONT_FACTOR  1.8
#else
	#define CANVAS_HD_FONT_FACTOR  1.5
#endif

//#define RGB565
#ifdef RGB565
typedef agg::pixfmt_rgb565 pixfmt;
#else
typedef agg::pixfmt_bgra32 pixfmt;
#endif
typedef agg::renderer_base<pixfmt> renbase_type;
typedef agg::renderer_primitives<renbase_type> renderer_pr;
typedef agg::font_engine_freetype_int32 font_engine_type;
typedef agg::font_cache_manager<font_engine_type> font_manager_type;
typedef agg::rounded_rect rounded;

agg::rendering_buffer agg_rbuf;

static pixfmt agg_pixf(agg_rbuf);
static agg::renderer_base<pixfmt> agg_renb;

static agg::line_profile_aa def_profile(2, agg::gamma_none());
static agg::line_profile_aa *profiles[MAX_THICKNESS];

static agg::renderer_outline_aa<renbase_type> reno(agg_renb, def_profile);
static agg::rasterizer_outline_aa< agg::renderer_outline_aa<renbase_type> >  raso(reno);

static agg::rasterizer_scanline_aa<> ras;
static agg::scanline_p8 sl;
static agg::renderer_scanline_aa_solid<agg::renderer_base<pixfmt> > ren_solid(agg_renb);

static font_engine_type             m_feng;
static font_manager_type            m_fman(m_feng);

static font_engine_type             m_image_feng;
static font_manager_type            m_image_fman(m_image_feng);

static font_engine_type             m_feng_nor;
static font_manager_type            m_fman_nor(m_feng_nor);

static font_engine_type             m_image_feng_nor;
static font_manager_type            m_image_fman_nor(m_image_feng_nor);

static RoadMapConfigDescriptor RoadMapConfigFont =
                        ROADMAP_CONFIG_ITEM("Labels", "FontName");

static RoadMapConfigDescriptor RoadMapConfigFontNormal =
                        ROADMAP_CONFIG_ITEM("Labels", "FontNameNormal");

struct roadmap_canvas_pen {
   struct roadmap_canvas_pen *next;
   char  *name;
   agg::rgba8 color;
   int thickness;
   agg::line_profile_aa *profile;
};

static struct roadmap_canvas_pen *RoadMapPenList = NULL;
static RoadMapPen CurrentPen;
static int        RoadMapCanvasFontLoaded = 0;
static int        RoadMapCanvasNormalFontLoaded = 0;

/* The canvas callbacks: all callbacks are initialized to do-nothing
 * functions, so that we don't care checking if one has been setup.
 */
static void roadmap_canvas_ignore_mouse (RoadMapGuiPoint *point) {}

RoadMapCanvasMouseHandler RoadMapCanvasMouseButtonPressed =
                                     roadmap_canvas_ignore_mouse;

RoadMapCanvasMouseHandler RoadMapCanvasMouseButtonReleased =
                                     roadmap_canvas_ignore_mouse;

RoadMapCanvasMouseHandler RoadMapCanvasMouseMoved =
                                     roadmap_canvas_ignore_mouse;


static void roadmap_canvas_ignore_configure (void) {}

RoadMapCanvasConfigureHandler RoadMapCanvasConfigure =
                                     roadmap_canvas_ignore_configure;

void roadmap_canvas_get_formated_text_extents
        (const char *text, int size, int *width,
            int *ascent, int *descent, int *can_tilt, int font_type){

   *ascent = 0;
   *descent = 0;
   font_manager_type *fman;
   font_engine_type  *feng;
   font_engine_type  *image_feng;
   wchar_t wstr[255];
   if (can_tilt) *can_tilt = 1;

   if ((font_type & FONT_TYPE_NORMAL) && (!RoadMapCanvasNormalFontLoaded))
      font_type = FONT_TYPE_BOLD;

   int length = roadmap_canvas_agg_to_wchar (text, wstr, 255);

   if (length <=0) {
      *width = 0;
      return;
   }

   double x  = 0;
   double y  = 0;
   const wchar_t* p = wstr;


   if (font_type & FONT_TYPE_NORMAL){
      feng = &m_feng_nor;
      image_feng = &m_image_feng_nor;
   }
   else{
      feng = &m_feng;
      image_feng = &m_image_feng;
   }

   if (size == -1) {
      size = 15;
      if ( roadmap_screen_is_hd_screen() )
      {
            size = (int)(size * CANVAS_HD_FONT_FACTOR);
      }
      else{
   #ifdef _WIN32
         size = (int) (size * 0.9);
   #endif
         size = (int) (size * 1.2);
      }
      feng->height(size);
      feng->width(size);

      /* Use the regular font */
      *descent = abs((int)feng->descender()) + 1;
      *ascent = (int)feng->ascender() + 1;
      if (font_type & FONT_TYPE_NORMAL)
         fman = &m_fman_nor;
      else
         fman = &m_fman;
   } else {
      if ( roadmap_screen_is_hd_screen() )
      {
         size = (int) (size * CANVAS_HD_FONT_FACTOR);
      }
      else{
#ifdef _WIN32
         size = (int) (size * 0.9);
#endif
         size = (int) (size * 1.2);
      }
      image_feng->height(size);
      image_feng->width(size);

      *descent = abs((int)image_feng->descender()) + 1;
      *ascent = (int)image_feng->ascender() + 1;
      if (font_type & FONT_TYPE_NORMAL)
         fman = &m_image_fman_nor;
      else
         fman = &m_image_fman;
   }

   while(*p) {
      const agg::glyph_cache* glyph = fman->glyph(*p);

      if(glyph) {
         x += glyph->advance_x;
         y += glyph->advance_y;
         //if (-glyph->bounds.y1 > *descent) *descent=-glyph->bounds.y1 - 1;
      }
      ++p;
   }

   *width = (int)x;

}


void roadmap_canvas_get_text_extents
        (const char *text, int size, int *width,
            int *ascent, int *descent, int *can_tilt) {
   roadmap_canvas_get_formated_text_extents(text, size, width, ascent, descent, can_tilt, FONT_TYPE_BOLD);
}

RoadMapPen roadmap_canvas_select_pen (RoadMapPen pen)
{
   if( !pen)
      return NULL;

   RoadMapPen old_pen = CurrentPen;
   dbg_time_start(DBG_TIME_SELECT_PEN);
   reno.profile(*pen->profile);
   reno.color(pen->color);
   CurrentPen = pen;

   dbg_time_end(DBG_TIME_SELECT_PEN);

   return old_pen;
}


RoadMapPen roadmap_canvas_create_pen (const char *name)
{
   struct roadmap_canvas_pen *pen;

   for (pen = RoadMapPenList; pen != NULL; pen = pen->next) {
      if (strcmp(pen->name, name) == 0) break;
   }

   if (pen == NULL) {

      pen = new roadmap_canvas_pen();
      roadmap_check_allocated(pen);

      pen->name = strdup (name);
      pen->color = agg::rgba8(0, 0, 0);
      pen->thickness = 1;
      pen->next = RoadMapPenList;

      RoadMapPenList = pen;
   }

   roadmap_canvas_select_pen (pen);
   roadmap_canvas_set_thickness (pen->thickness);

   return pen;
}


void roadmap_canvas_set_foreground (const char *color) {

   if (!CurrentPen) return;

   CurrentPen->color = roadmap_canvas_agg_parse_color(color);
   roadmap_canvas_select_pen(CurrentPen);
}


int  roadmap_canvas_get_thickness  (RoadMapPen pen) {

   if (pen == NULL) return 0;

   return pen->thickness;
}


void roadmap_canvas_set_thickness (int thickness) {

   if (thickness >= MAX_THICKNESS) thickness = MAX_THICKNESS - 1;

   CurrentPen->thickness = thickness;
   if (!profiles[thickness]) {
      profiles[thickness] =
         new agg::line_profile_aa(thickness, agg::gamma_none());
   }

   CurrentPen->profile = profiles[thickness];
}


void roadmap_canvas_set_opacity (int opacity) {

   if (!CurrentPen) return;
   CurrentPen->color.a = opacity;
   roadmap_canvas_select_pen(CurrentPen);
}


void roadmap_canvas_erase (void) {

   agg_renb.clear(CurrentPen->color);
}


void roadmap_canvas_erase_area (const RoadMapGuiRect *rect) {

   renderer_pr ren_pr(agg_renb);
   ren_pr.fill_color (CurrentPen->color);
   ren_pr.solid_rectangle(rect->minx, rect->miny, rect->maxx, rect->maxy);
}

void roadmap_canvas_draw_formated_string_size (RoadMapGuiPoint *position,
                                                int corner,
                                                int size,
                                                int font_type,
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

   case ROADMAP_CANVAS_TOPMIDDLE:
      y = position->y;
      x = position->x - (text_width / 2);
      break;

   case ROADMAP_CANVAS_BOTTOMMIDDLE:
      y = position->y - text_height;
      x = position->x - (text_width / 2);
      break;

   case ROADMAP_CANVAS_CENTERMIDDLE:
      y = position->y - (text_ascent / 2) - text_descent;
      x = position->x - (text_width / 2);
      break;

   case ROADMAP_CANVAS_CENTERRIGHT:
      y = position->y - (text_ascent / 2 ) - text_descent;
      x = position->x - text_width;
      break;

   case ROADMAP_CANVAS_CENTERLEFT:
      y = position->y - (text_ascent / 2) - text_descent;
      x = position->x;
      break;

   default:
      return;
   }

   RoadMapGuiPoint start = {x, y+text_height};
   roadmap_canvas_draw_formated_string_angle (&start, position, 0, size, font_type, text);
}

void roadmap_canvas_draw_string_size (RoadMapGuiPoint *position,
                                 int corner,
                                 int size,
                                 const char *text) {
   roadmap_canvas_draw_formated_string_size (position, corner, size, FONT_TYPE_BOLD, text);
}

void roadmap_canvas_draw_string (RoadMapGuiPoint *position,
                                 int corner,
                                 const char *text) {

    roadmap_canvas_draw_string_size(position, corner, -1, text);
}


void roadmap_canvas_draw_multiple_points (int count, RoadMapGuiPoint *points) {

   int i;

   for (i=0; i<count; i++) {

      agg_renb.copy_pixel(points[i].x, points[i].y, CurrentPen->color);
   }
}

#if !defined(ANDROID) && !defined(WIN32)// Linkage fails
void roadmap_canvas_draw_rounded_rect(RoadMapGuiPoint *bottom, RoadMapGuiPoint *top, int radius){

	rounded r = agg::rounded_rect(bottom->x, bottom->y, top->x, top->y, 10);
   r.normalize_radius();
   agg::conv_stroke<agg::rounded_rect> p(r);
   p.width(1.0);
   raso.add_path(p);
}
#endif

void roadmap_canvas_draw_multiple_lines (int count, int *lines,
      RoadMapGuiPoint *points, int fast_draw) {

   int i;
   int count_of_points;


   dbg_time_start(DBG_TIME_DRAW_LINES);
#ifdef WIN32_PROFILE
   ResumeCAPAll();
#endif

#ifdef _WIN32_
   if (fast_draw) {
      roadmap_canvas_native_draw_multiple_lines (count, lines, points,
         CurrentPen->color.r, CurrentPen->color.g, CurrentPen->color.b,
         CurrentPen->thickness);
      return;
   }
#endif

   if (!fast_draw) {
      raso.round_cap(true);
      raso.line_join(agg::outline_miter_accurate_join);
   } else {
      raso.round_cap(false);
      raso.line_join(agg::outline_no_join);
   }

//   raso.accurate_join(true);

   static agg::path_storage path;

   for (i = 0; i < count; ++i) {

      int first = 1;

      count_of_points = *lines;

      if (count_of_points < 2) continue;

      dbg_time_start(DBG_TIME_CREATE_PATH);

      for (int j=0; j<count_of_points; j++) {

         if (first) {
            first = 0;
            path.move_to(points->x, points->y);
         } else {
            path.line_to(points->x, points->y);
         }

         points++;
      }

      dbg_time_end(DBG_TIME_CREATE_PATH);
      dbg_time_start(DBG_TIME_ADD_PATH);

#if 0
      if (fast_draw) {
         renderer_pr ren_pr(agg_renb);
         agg::rasterizer_outline<renderer_pr> ras_line(ren_pr);
         ren_pr.line_color(CurrentPen->color);
         ras_line.add_path(path);

      } else {
     #endif
         raso.add_path(path);
      //}

      path.remove_all ();
      dbg_time_end(DBG_TIME_ADD_PATH);

      lines += 1;
   }


#ifdef WIN32_PROFILE
   SuspendCAPAll();
#endif



	return;


   dbg_time_end(DBG_TIME_DRAW_LINES);
}

void roadmap_canvas_draw_multiple_polygons
         (int count, int *polygons, RoadMapGuiPoint *points, int filled,
          int fast_draw) {

   int i;
   int count_of_points;

   static agg::path_storage path;

   for (i = 0; i < count; ++i) {

      count_of_points = *polygons;

      int first = 1;

      for (int j=0; j<count_of_points; j++) {

         if (first) {
            first = 0;
            path.move_to(points->x, points->y);
         } else {
            path.line_to(points->x, points->y);
         }
         points++;
      }

      path.close_polygon();

      if (filled) {

         ras.reset();
         ras.add_path(path);
         ren_solid.color(CurrentPen->color);
         agg::render_scanlines( ras, sl, ren_solid);

      } else if (fast_draw) {
         renderer_pr ren_pr(agg_renb);
         agg::rasterizer_outline<renderer_pr> ras_line(ren_pr);
         ren_pr.line_color(CurrentPen->color);
         ras_line.add_path(path);

      } else {

         raso.add_path(path);
      }

      path.remove_all ();

      polygons += 1;
   }
}


void roadmap_canvas_draw_multiple_circles
        (int count, RoadMapGuiPoint *centers, int *radius, int filled,
         int fast_draw) {

   int i;

   static agg::path_storage path;

   for (i = 0; i < count; ++i) {

      int r = radius[i];

      int x = centers[i].x;
      int y = centers[i].y;

      agg::ellipse e( x, y, r, r);
      path.concat_path(e);

      if (filled) {

         ras.reset();
         ras.add_path(path);
         ren_solid.color(CurrentPen->color);
         agg::render_scanlines( ras, sl, ren_solid);

/*      } else if (fast_draw) {
         renderer_pr ren_pr(agg_renb);
         agg::rasterizer_outline<renderer_pr> ras_line(ren_pr);
         ren_pr.line_color(CurrentPen->color);
         ras_line.add_path(path);
*/
      } else {

         raso.add_path(path);
      }

      path.remove_all ();
   }
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


void roadmap_canvas_register_configure_handler
                    (RoadMapCanvasConfigureHandler handler) {

   RoadMapCanvasConfigure = handler;
}


int roadmap_canvas_width (void) {
   return agg_renb.width();
}


int roadmap_canvas_height (void) {
   return agg_renb.height();
}


void roadmap_canvas_save_screenshot (const char* filename) {
   /* NOT IMPLEMENTED. */
}


/*
** Use FRIBIDI to encode the string.
** The return value must be freed by the caller.
*/

#ifdef USE_FRIBIDI
static wchar_t* bidi_string(wchar_t *logical) {

   FriBidiCharType base = FRIBIDI_TYPE_ON;
   size_t len;

   len = wcslen(logical);

   FriBidiChar *visual;

   FriBidiStrIndex *ltov, *vtol;
   FriBidiLevel *levels;
   FriBidiStrIndex new_len;
   fribidi_boolean log2vis;

   visual = (FriBidiChar *) malloc (sizeof (FriBidiChar) * (len + 1));
   ltov = NULL;
   vtol = NULL;
   levels = NULL;

   /* Create a bidi string. */
   log2vis = fribidi_log2vis ((FriBidiChar *)logical, len, &base,
      /* output */
      visual, ltov, vtol, levels);
   visual[len] = 0;
   if (!log2vis) {
      //msSetError(MS_IDENTERR, "Failed to create bidi string.",
      //"msGetFriBidiEncodedString()");
      return NULL;
   }
   visual[len] = 0;
   new_len = len;
   visual[len] = 0;
   return (wchar_t *)visual;

}
#endif

void roadmap_canvas_draw_formated_string_angle (const RoadMapGuiPoint *position,
                                                RoadMapGuiPoint *center,
                                                int angle, int size,
                                                int font_type,
                                                const char *text)
{

   font_manager_type *fman;
   font_manager_type *image_fman;
   font_engine_type  *feng;
   font_engine_type  *image_feng;

   if ((font_type & FONT_TYPE_NORMAL) && (!RoadMapCanvasNormalFontLoaded))
      font_type = FONT_TYPE_BOLD;


   if (font_type & FONT_TYPE_NORMAL){
      fman = &m_fman_nor;
      feng = &m_feng_nor;
      image_feng = &m_image_feng_nor;
      image_fman = &m_image_fman_nor;
   }
   else{
      fman = &m_fman;
      feng = &m_feng;
      image_feng = &m_image_feng;
      image_fman = &m_image_fman;
   }


   if ((font_type & FONT_TYPE_BOLD) && (RoadMapCanvasFontLoaded != 1)) return;


   dbg_time_start(DBG_TIME_TEXT_FULL);
   dbg_time_start(DBG_TIME_TEXT_CNV);

   wchar_t wstr[255];
   int length = roadmap_canvas_agg_to_wchar (text, wstr, 255);
   if (length <=0) return;

#ifdef USE_FRIBIDI
   wchar_t *bidi_text = bidi_string(wstr);
   const wchar_t* p = bidi_text;
#else
   const wchar_t* p = wstr;
#endif

   ren_solid.color(CurrentPen->color);
   dbg_time_end(DBG_TIME_TEXT_CNV);

   double x  = 0;
   double y  = 0;
   if (size < 0) size = 15;

   if ( roadmap_screen_is_hd_screen() )
   {
     size = (int)(size * CANVAS_HD_FONT_FACTOR);
   }
   else{
#ifdef _WIN32
      size = (int) (size * 0.9);
#endif
      size = (int) (size * 1.2);
   }

   if (angle == 0) {

      /* Use faster drawing for text with no angle */
      x  = position->x;
      y  = position->y;

//      ren_solid.color(agg::rgba8(0, 0, 0));

      image_feng->height(size);
      image_feng->width(size);

      while(*p) {
         const agg::glyph_cache* glyph = image_fman->glyph(*p);

         if(glyph) {
            image_fman->init_embedded_adaptors(glyph, x, y);

            agg::render_scanlines(image_fman->gray8_adaptor(),
                  image_fman->gray8_scanline(),
                  ren_solid);

            // increment pen position
            x += glyph->advance_x;
            y += glyph->advance_y;
         }
         ++p;
      }
   }
   else{
      feng->height(size);
      feng->width(size);
   }

   while(*p) {
      dbg_time_start(DBG_TIME_TEXT_ONE_LETTER);
      dbg_time_start(DBG_TIME_TEXT_GET_GLYPH);
      const agg::glyph_cache* glyph = fman->glyph(*p);
      dbg_time_end(DBG_TIME_TEXT_GET_GLYPH);

      if(glyph) {
         fman->init_embedded_adaptors(glyph, x, y);

         //agg::conv_curve<font_manager_type::path_adaptor_type> stroke(fman->path_adaptor());

         agg::trans_affine mtx;
         if (abs(angle) > 0) {
            mtx *= agg::trans_affine_rotation(agg::deg2rad(angle));
         }
         mtx *= agg::trans_affine_translation(position->x, position->y);

         agg::conv_transform<font_manager_type::path_adaptor_type> tr(fman->path_adaptor(), mtx);

         agg::conv_curve<agg::conv_transform<font_manager_type::path_adaptor_type> > fill(tr);

         //agg::conv_stroke<
            //agg::conv_curve<agg::conv_transform<font_manager_type::path_adaptor_type> > >
         //stroke(fill);

         //agg::conv_contour<agg::conv_transform<font_manager_type::path_adaptor_type> >contour(tr);
         //contour.width(2);

         //agg::conv_stroke< agg::conv_contour<agg::conv_transform<font_manager_type::path_adaptor_type> > > stroke(contour);
         //agg::conv_stroke< agg::conv_transform<font_manager_type::path_adaptor_type> > stroke(tr);

         dbg_time_start(DBG_TIME_TEXT_ONE_RAS);

#ifdef WIN32_PROFILE
         ResumeCAPAll();
#endif
         ras.reset();
         ras.add_path(tr);
         agg::render_scanlines(ras, sl, ren_solid);
         //ras.add_path(fill);
         //ras.add_path(stroke);
         //ren_solid.color(agg::rgba8(255, 255, 255));
         //agg::render_scanlines(ras, sl, ren_solid);
         //ras.add_path(tr);
         //ren_solid.color(agg::rgba8(0, 0, 0));
         //agg::render_scanlines(ras, sl, ren_solid);

#ifdef WIN32_PROFILE
         SuspendCAPAll();
#endif

         dbg_time_end(DBG_TIME_TEXT_ONE_RAS);

         // increment pen position
         x += glyph->advance_x;
         y += glyph->advance_y;
         dbg_time_end(DBG_TIME_TEXT_ONE_LETTER);
      }
      ++p;
   }

#ifdef USE_FRIBIDI
   free(bidi_text);
#endif

   dbg_time_end(DBG_TIME_TEXT_FULL);
}

void roadmap_canvas_draw_string_angle (const RoadMapGuiPoint *position,
                                                RoadMapGuiPoint *center,
                                                int angle, int size,
                                                const char *text){

      roadmap_canvas_draw_formated_string_angle(position, center, angle, size, FONT_TYPE_BOLD, text);
}


static int roadmap_canvas_agg_get_screen_type( int width, int height )
{
	int screen_type = RM_SCREEN_TYPE_SD_GENERIC;
	/*
	 * Temporary just simple classification
	 */
	if ( width >= 640 || height >= 640 )
	{
		screen_type = RM_SCREEN_TYPE_HD_GENERIC;
	}

	return screen_type;
}

void roadmap_canvas_agg_configure (unsigned char *buf, int width, int height, int stride) {

   roadmap_log( ROADMAP_ERROR, "roadmap_canvas_agg_configure, height =%d width=%d",height, width);
   agg_rbuf.attach(buf, width, height, stride);

   agg_renb.attach(agg_pixf);
   agg_renb.reset_clipping(true);
   ras.clip_box(0, 0, agg_renb.width() - 1, agg_renb.height() - 1);

   agg::glyph_rendering gren = agg::glyph_ren_outline;
   agg::glyph_rendering image_gren = agg::glyph_ren_agg_gray8;

   roadmap_config_declare
       ("preferences", &RoadMapConfigFont, "font.ttf", NULL);

   roadmap_config_declare
       ("preferences", &RoadMapConfigFontNormal, "font_normal.ttf", NULL);

   char *font_file = roadmap_path_join(roadmap_path_user(),
		   roadmap_config_get (&RoadMapConfigFont));

   if ((width) && (height))
      roadmap_screen_set_screen_type( width, height );

   if (!RoadMapCanvasFontLoaded) {

      if(m_feng.load_font(font_file, 0, gren) &&
            m_image_feng.load_font(font_file, 0, image_gren)) {

         m_feng.hinting(true);

         if ( roadmap_screen_is_hd_screen() )
         {
			 m_feng.height(22);
			 m_feng.width(22);
         }
         else
         {
#ifdef _WIN32
          m_feng.height(12);
          m_feng.width(12);
#else
          m_feng.height(15);
          m_feng.width(15);
#endif
         }

         m_feng.flip_y(true);

         m_image_feng.hinting(true);
         m_image_feng.flip_y(true);

         RoadMapCanvasFontLoaded = 1;
      } else {
         RoadMapCanvasFontLoaded = -1;
         char message[300];
         snprintf(message, sizeof(message), "Can't load font: %s\n", font_file);
         roadmap_messagebox("Error", message);
      }
   }
   RoadMapCanvasFontLoaded = 1;
   roadmap_path_free(font_file);

   font_file = roadmap_path_join(roadmap_path_user(),
         roadmap_config_get (&RoadMapConfigFontNormal));


   if(m_feng_nor.load_font(font_file, 0, gren) &&
            m_image_feng_nor.load_font(font_file, 0, image_gren)) {

         m_feng_nor.hinting(true);

         if ( roadmap_screen_is_hd_screen() )
         {
          m_feng_nor.height(22);
          m_feng_nor.width(22);
         }
         else
         {
#ifdef _WIN32
          m_feng_nor.height(12);
          m_feng_nor.width(12);
#else
          m_feng_nor.height(15);
          m_feng_nor.width(15);
#endif
         }

         m_feng_nor.flip_y(true);

         m_image_feng_nor.hinting(true);
         m_image_feng_nor.flip_y(true);
         RoadMapCanvasNormalFontLoaded = 1;
   }
}

static BOOL check_file_suffix( const char* file_name, const char* suffix )
{
	BOOL res = FALSE;
	int suffix_len = strlen( suffix );

	if ( (int) strlen( file_name ) > suffix_len )
	{
		res = !strcasecmp( file_name + strlen( file_name ) - suffix_len, suffix );
	}

	return res;
}

/*************************************************************************************************
 * roadmap_canvas_load_bmp_buffer()
 * Loads and returns the buffer of the bmp image
 */
static unsigned char * roadmap_canvas_load_bmp_buffer( const char *full_path, int *width, int *height, int *stride  )
{

	roadmap_log( ROADMAP_ERROR, "Bmp image loading is not supported!" );
	return NULL;
}

/*************************************************************************************************
 * roadmap_canvas_load_png_buffer()
 * Loads and returns the buffer of the png image ( RGBA 32 )
 */
static unsigned char* roadmap_canvas_load_png_buffer( const char *full_path, int *width, int *height, int *stride )
{

   unsigned char *buf = read_png_file( full_path, width, height, stride );

   if ( !buf ) return NULL;

   return buf;
}

/*************************************************************************************************
 * roadmap_canvas_load_buffer()
 * Loads and returns the buffer of the bmp image ( RGBA 32 )
 */
static unsigned char * roadmap_canvas_load_buffer( const char *full_path, int *width, int *height, int *stride  )
{

   unsigned char* buf;

   if ( check_file_suffix( full_path, ".bmp" ) )
   {
      buf = roadmap_canvas_load_bmp_buffer( full_path, width, height, stride );
   }
   else
   {
	  buf = roadmap_canvas_load_png_buffer( full_path, width, height, stride );
   }

   if ( !buf )
   {
	   roadmap_log( ROADMAP_ERROR, "Failed to load image file: %s", full_path );
   }

   return buf;
}



RoadMapImage roadmap_canvas_load_bmp (const char *full_path) {

   RoadMapImage image = NULL;
   int width, height, stride;

   unsigned char *buf = roadmap_canvas_load_bmp_buffer( full_path, &width, &height, &stride );

   if ( buf )
   {
	   image =  new roadmap_canvas_image();
	   image->rbuf.attach ( buf, width, height, stride );
   }

   return image;
}


RoadMapImage roadmap_canvas_load_png ( const char *full_path )
{
   RoadMapImage image = NULL;
   int width, height, stride;
   unsigned char *buf = roadmap_canvas_load_png_buffer( full_path, &width, &height, &stride );

   if ( buf )
   {
	   image =  new roadmap_canvas_image();
	   image->rbuf.attach ( buf, width, height, stride );
   }

   return image;
}




/*************************************************************************************************
 * roadmap_canvas_agg_load_image()
 *
 */
RoadMapImage roadmap_canvas_load_image ( const char *path, const char *file_name )
{

   char *full_path = roadmap_path_join( path, file_name );

   RoadMapImage image = NULL;

   roadmap_log( ROADMAP_INFO, "Loading image : %s\n", file_name );

   if ( check_file_suffix( file_name, ".bmp" ) )
   {
      image = roadmap_canvas_load_bmp( full_path );
   }
   else
   {
	  image = roadmap_canvas_load_png( full_path );
   }

   if ( image )
   {
	   image->full_path = full_path;
	   image->isCached = 1;
   }
   else
   {
	   free( full_path );
   }

   return image;
}

inline static void roadmap_canvas_free_image_buffer( RoadMapImage image ) {

   if ( image->isCached )
   {
	   free( image->rbuf.buf() );
	   image->isCached = 0;
   }
}

void roadmap_canvas_free_image (RoadMapImage image) {

   roadmap_canvas_free_image_buffer( image );

   if ( image->full_path )
   {
	   free( (void*) image->full_path );
   }

   delete image;
}


inline static BOOL roadmap_canvas_check_cached( RoadMapImage image )
{
	int width, height, stride;
   /*
	* Image is not available in memory - try to load from FS
	*/
   if ( !image->isCached )
   {
	   if ( image->full_path )
	   {

		   unsigned char* buf = roadmap_canvas_load_buffer( image->full_path, &width, &height, &stride );

		   if ( buf )
		   {
			   image->rbuf.attach ( buf, width, height, stride );
			   image->isCached = 1;
		   }
		   else	// Failed to load the buffer
		   {
			  // roadmap_log( ROADMAP_ERROR, "Failed to load the image from the FS %s", image->full_path );
		   }
	   }
   }
   return image->isCached;
}


void roadmap_canvas_draw_image (RoadMapImage image, const RoadMapGuiPoint *pos,
                                int opacity, int mode) {

   if (!image){
   	assert(1);
   	return;
   }


   if ( !roadmap_canvas_check_cached( image ) )
   {
	   return;
   }

   if ((mode == IMAGE_SELECTED) || (opacity <= 0) || (opacity >= 255)) {
      opacity = 255;
   }

   agg_renb.blend_from(image->pixfmt, 0, pos->x, pos->y, opacity);

   if (mode == IMAGE_SELECTED) {
      static RoadMapPen selection;

      if (!selection) {
         selection = roadmap_canvas_create_pen("selection");
         roadmap_canvas_set_foreground ("#000000");
      }

      RoadMapGuiPoint points[5] = {
         {pos->x, pos->y},
         {pos->x + image->rbuf.width(), pos->y},
         {pos->x + image->rbuf.width(), pos->y + image->rbuf.height()},
         {pos->x, pos->y + image->rbuf.height()},
         {pos->x, pos->y}};

      int num_points = 5;

      RoadMapPen current = roadmap_canvas_select_pen (selection);
      roadmap_canvas_draw_multiple_lines (1, &num_points, points, 0);
      roadmap_canvas_select_pen (current);
   }

   // AGA DEBUG
   if ( image->full_path )
   {
	   // roadmap_canvas_free_image_buffer( image );
   }

}


void roadmap_canvas_copy_image (RoadMapImage dst_image,
                                const RoadMapGuiPoint *pos,
                                const RoadMapGuiRect  *rect,
                                RoadMapImage src_image, int mode) {

   agg::renderer_base<agg::pixfmt_rgba32> renb(dst_image->pixfmt);

   agg::rect_i agg_rect;
   agg::rect_i *agg_rect_p = NULL;

   if ( !roadmap_canvas_check_cached( src_image ) ||
		   !roadmap_canvas_check_cached( dst_image ) )
	   return;


   if (rect) {
      agg_rect.x1 = rect->minx;
      agg_rect.y1 = rect->miny;
      agg_rect.x2 = rect->maxx;
      agg_rect.y2 = rect->maxy;

      agg_rect_p = &agg_rect;
   }

   if (mode == CANVAS_COPY_NORMAL) {
      renb.copy_from(src_image->rbuf, agg_rect_p, pos->x, pos->y);
   } else {
      renb.blend_from(src_image->pixfmt, agg_rect_p, pos->x, pos->y, 255);
   }
}


RoadMapImage roadmap_canvas_new_image (int width, int height) {
   RoadMapImage image =  new roadmap_canvas_image();
   unsigned char *buf = (unsigned char *)malloc (width*height*4);

   if (buf == NULL){
      roadmap_log( ROADMAP_FATAL, "Failed to create new image width=%d, height=%d", width, height );
      return NULL;
   }
   memset(buf, 0, (width*height*4));

   image->rbuf.attach (buf,
                       width, height,
                       width * 4);

   image->isCached = 1;

   return image;
}


int  roadmap_canvas_image_width  (const RoadMapImage image) {

   if (!image) return 0;

   return image->rbuf.width();
}


int  roadmap_canvas_image_height (const RoadMapImage image) {

   if (!image) return 0;

   return image->rbuf.height();
}


void roadmap_canvas_draw_image_formated_text (RoadMapImage image,
                                     const RoadMapGuiPoint *position,
                                     int size, const char *text, int font_type) {
   font_manager_type *image_fman;
   font_engine_type  *image_feng;

   if (RoadMapCanvasFontLoaded != 1) return;

   if ((font_type & FONT_TYPE_NORMAL) && (!RoadMapCanvasNormalFontLoaded))
      font_type = FONT_TYPE_BOLD;

   if (font_type & FONT_TYPE_NORMAL){
      image_fman = &m_image_fman_nor;
      image_feng = &m_image_feng_nor;
   }
   else{
      image_fman = &m_image_fman;
      image_feng = &m_image_feng;
   }

   wchar_t wstr[255];
   int length = roadmap_canvas_agg_to_wchar (text, wstr, 255);
   if (length <=0) return;

#ifdef USE_FRIBIDI
   wchar_t *bidi_text = bidi_string(wstr);
   const wchar_t* p = bidi_text;
#else
   const wchar_t* p = wstr;
#endif

   if ( !roadmap_canvas_check_cached( image ) )
   {
      return;
   }


   if ( roadmap_screen_is_hd_screen() )
   {
      size = (int) (size * CANVAS_HD_FONT_FACTOR);
   }
   else{
#ifdef _WIN32
      size = (int) (size * 0.9);
#endif
      size = (int) (size * 1.2);
   }

   double x  = position->x;
   double y  = position->y + size - 7;

   if ( roadmap_screen_is_hd_screen() )
   {
      y -= 3;
   }

   agg::renderer_base<agg::pixfmt_rgba32> renb(image->pixfmt);
   agg::renderer_scanline_aa_solid< agg::renderer_base<agg::pixfmt_rgba32> > ren_solid (renb);

   //ren_solid.color(agg::rgba8(255, 255, 255));
   ren_solid.color(CurrentPen->color);

   image_feng->height(size);
   image_feng->width(size);

   while(*p) {
      const agg::glyph_cache* glyph = image_fman->glyph(*p);

      if(glyph) {
         image_fman->init_embedded_adaptors(glyph, x, y);

         agg::render_scanlines(image_fman->gray8_adaptor(),
               image_fman->gray8_scanline(),
               ren_solid);

         // increment pen position
         x += glyph->advance_x;
         y += glyph->advance_y;
      }
      ++p;
   }

#ifdef USE_FRIBIDI
   free(bidi_text);
#endif
}

void roadmap_canvas_draw_image_text (RoadMapImage image,
                                     const RoadMapGuiPoint *position,
                                     int size, const char *text) {


   roadmap_canvas_draw_image_formated_text(image, position, size, text, FONT_TYPE_BOLD);
}

RoadMapImage roadmap_canvas_image_from_buf( unsigned char* buf, int width, int height, int stride )
{
    RoadMapImage image =  new roadmap_canvas_image();
    image->rbuf.attach( buf, width, height, stride );

    return image;
}

void roadmap_canvas_image_set_mutable (RoadMapImage img) {}

void roadmap_canvas_shutdown(){}
void roadmap_canvas_image_invalidate( RoadMapImage image ) {}
void roadmap_canvas_unmanaged_list_add( RoadMapImage image ) {}
