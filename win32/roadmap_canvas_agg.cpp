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

#include <windows.h>

#define USE_FRIBIDI

#ifdef WIN32_PROFILE
#include <C:\Program Files\Windows CE Tools\Common\Platman\sdk\wce500\include\cecap.h>
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
#include "util/agg_color_conv_rgb8.h"
#include "platform/win32/agg_win32_bmp.h"

extern "C" {
#include "../roadmap.h"
#include "../roadmap_canvas.h"
#include "../roadmap_messagebox.h"
#include "../roadmap_math.h"
#include "../roadmap_config.h"
#include "../roadmap_path.h"
#include "../roadmap_screen.h"
#include "roadmap_wincecanvas.h"
#include "colors.h"
}

#include "roadmap_libpng.h"
#include "../roadmap_canvas_agg.h"

static HWND			RoadMapDrawingArea;
static HDC			RoadMapDrawingBuffer;
static RECT			ClientRect;
static HBITMAP		OldBitmap = NULL;


int roadmap_canvas_agg_to_wchar (const char *text, wchar_t *output, int size) {

	LPWSTR text_unicode = ConvertToWideChar(text, CP_UTF8);
   wcsncpy(output, text_unicode, size);
   free(text_unicode);
   output[size - 1] = 0;

   return wcslen(output);
}


agg::rgba8 roadmap_canvas_agg_parse_color (const char *color) {
	int high, i, low;

	if (*color == '#') {
		int r, g, b, a;
      int count;

		count = sscanf(color, "#%2x%2x%2x%2x", &r, &g, &b, &a);

      if (count == 4) {
         return agg::rgba8(r, g, b, a);
      } else {
         return agg::rgba8(r, g, b);
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
         return agg::rgba8(color_table[high].r, color_table[high].g,
            color_table[high].b);
		} else {
         return agg::rgba8(0, 0, 0);
		}
	}
}


void roadmap_canvas_button_pressed(POINT *data) {
   RoadMapGuiPoint point;

   point.x = (short)data->x;
   point.y = (short)data->y;

   (*RoadMapCanvasMouseButtonPressed) (&point);

}


void roadmap_canvas_button_released(POINT *data) {
   RoadMapGuiPoint point;

   point.x = (short)data->x;
   point.y = (short)data->y;

   (*RoadMapCanvasMouseButtonReleased) (&point);

}


void roadmap_canvas_mouse_moved(POINT *data) {
   RoadMapGuiPoint point;

   point.x = (short)data->x;
   point.y = (short)data->y;

   (*RoadMapCanvasMouseMoved) (&point);

}


void roadmap_canvas_refresh (void) {
   HDC hdc;

   if (RoadMapDrawingArea == NULL) return;

   dbg_time_start(DBG_TIME_FLIP);

   hdc = GetDC(RoadMapDrawingArea);
   BitBlt(hdc, ClientRect.left, ClientRect.top,
      ClientRect.right - ClientRect.left + 1,
      ClientRect.bottom - ClientRect.top + 1,
      RoadMapDrawingBuffer, 0, 0, SRCCOPY);

   DeleteDC(hdc);
   dbg_time_end(DBG_TIME_FLIP);
}


HWND roadmap_canvas_new (HWND hWnd, HWND tool_bar) {
   HDC hdc;
   static BITMAPINFO* bmp_info = (BITMAPINFO*) malloc(sizeof(BITMAPINFO) +
                                       (sizeof(RGBQUAD)*3));
   HBITMAP bmp;
   void* buf = 0;

   memset(bmp_info, 0, sizeof(BITMAPINFO) + sizeof(RGBQUAD)*3);

   if (RoadMapDrawingBuffer != NULL) {

      DeleteObject(SelectObject(RoadMapDrawingBuffer, OldBitmap));
      DeleteDC(RoadMapDrawingBuffer);
   }

   hdc = GetDC(RoadMapDrawingArea);

   RoadMapDrawingArea = hWnd;
   GetClientRect(hWnd, &ClientRect);
   if (tool_bar != NULL) {
      RECT tb_rect;
      GetClientRect(tool_bar, &tb_rect);
	  if (tb_rect.bottom < (ClientRect.bottom-2)) {
		ClientRect.top += tb_rect.bottom + 2;
	  }
   }


   RoadMapDrawingBuffer = CreateCompatibleDC(hdc);

   bmp_info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
   bmp_info->bmiHeader.biWidth = ClientRect.right;
   bmp_info->bmiHeader.biHeight = ClientRect.bottom;
   bmp_info->bmiHeader.biPlanes = 1;
   bmp_info->bmiHeader.biBitCount = 16;
   bmp_info->bmiHeader.biCompression = BI_BITFIELDS;
   bmp_info->bmiHeader.biSizeImage = 0;
   bmp_info->bmiHeader.biXPelsPerMeter = 0;
   bmp_info->bmiHeader.biYPelsPerMeter = 0;
   bmp_info->bmiHeader.biClrUsed = 0;
   bmp_info->bmiHeader.biClrImportant = 0;
   ((DWORD*)bmp_info->bmiColors)[0] = 0xF800;
   ((DWORD*)bmp_info->bmiColors)[1] = 0x07E0;
   ((DWORD*)bmp_info->bmiColors)[2] = 0x001F;

   bmp = CreateDIBSection(
      RoadMapDrawingBuffer,
      bmp_info,
      DIB_RGB_COLORS,
      &buf,
      0,
      0
      );

   
   int stride = ((ClientRect.right * 2 + 3) >> 2) << 2;
   roadmap_canvas_agg_configure((unsigned char*)buf,
      ClientRect.right,
      ClientRect.bottom,
      -stride);

   OldBitmap = (HBITMAP)SelectObject(RoadMapDrawingBuffer, bmp);

   DeleteDC(hdc);
   (*RoadMapCanvasConfigure) ();

   return RoadMapDrawingArea;
}


RoadMapImage roadmap_canvas_agg_load_bmp (const char *full_name) {

   agg::pixel_map pmap_tmp;

   if(!pmap_tmp.load_from_bmp(full_name)) {
      return NULL;
   }

   if (pmap_tmp.bpp() != 24) {
      return NULL;
   }

   int width = pmap_tmp.width();
   int height = pmap_tmp.height();
   int stride = pmap_tmp.stride();

   unsigned char *buf = (unsigned char *)malloc (width * height * 4);

   agg::rendering_buffer tmp_rbuf (pmap_tmp.buf(),
                                   width, height,
                                   -pmap_tmp.stride());

   RoadMapImage image =  new roadmap_canvas_image();

   image->rbuf.attach (buf,
                       width, height,
                       width * 4);

   agg::color_conv(&image->rbuf, &tmp_rbuf, agg::color_conv_bgr24_to_rgba32());
   return image;
}


RoadMapImage roadmap_canvas_agg_load_png (const char *full_name) {

   int width;
   int height;
   int stride;

   unsigned char *buf = read_png_file(full_name, &width, &height, &stride);

   if (!buf) return NULL;

   
   RoadMapImage image =  new roadmap_canvas_image();
   image->rbuf.attach (buf, width, height, stride);

   return image;
}



void roadmap_canvas_agg_free_image (RoadMapImage image) {

   free (image->rbuf.buf());
   delete image;
}


static void roadmap_canvas_convert_points (POINT *winpoints,
			RoadMapGuiPoint *points, int count)
{
    RoadMapGuiPoint *end = points + count;

    while (points < end) {
        winpoints->x = points->x;
        winpoints->y = points->y;
        winpoints += 1;
        points += 1;
    }
}

void select_native_color (COLORREF color, int thickness) {
   static HBRUSH oldBrush;
   static HPEN oldPen;
   static COLORREF last_color;
   static int last_thickness;
   static int init;

   if (!init) {
      oldBrush = (HBRUSH) SelectObject(RoadMapDrawingBuffer,
            CreateSolidBrush(color));

      oldPen = (HPEN) SelectObject(RoadMapDrawingBuffer, CreatePen(PS_SOLID,
               thickness, color));

      last_color = color;
      last_thickness = thickness;
      init = 1;
      return;
   }

   if (last_color != color) {
      DeleteObject(
         SelectObject(RoadMapDrawingBuffer, CreatePen(PS_SOLID,
               thickness, color)));
      DeleteObject(SelectObject(RoadMapDrawingBuffer,
            CreateSolidBrush(color)));
      last_color = color;
      last_thickness = thickness;
   } else if (last_thickness != thickness) {
      DeleteObject(
         SelectObject(RoadMapDrawingBuffer, CreatePen(PS_SOLID,
               thickness, color)));
      last_thickness = thickness;
   }
}


void roadmap_canvas_native_draw_multiple_lines (int count, int *lines,
				RoadMapGuiPoint *points, int r, int g, int b, int thickness)
{
	int i;
	int count_of_points;

	POINT winpoints[1024];

   static int init;

   select_native_color (RGB(r, g, b), thickness);

	for (i = 0; i < count; ++i) {

      RoadMapGuiPoint end_points[2];
      int first = 1;

		count_of_points = *lines;

      if (count_of_points < 2) continue;

		while (count_of_points > 1024) {

         if (first) {
            first = 0;
            end_points[0] = *points;
         }
			roadmap_canvas_convert_points (winpoints, points, 1024);
			Polyline(RoadMapDrawingBuffer, winpoints, 1024);
			/* We shift by 1023 only, because we must link the lines. */
			points += 1023;
			count_of_points -= 1023;
		}

      if (first) {
         first = 0;
         end_points[0] = *points;
      }

      end_points[1] = points[count_of_points - 1];
		roadmap_canvas_convert_points (winpoints, points, count_of_points);
		Polyline(RoadMapDrawingBuffer, winpoints, count_of_points);

#if 0
      if (CurrentPen->thinkness > 5) {

         HPEN oldPen = SelectObject(RoadMapDrawingBuffer,
            GetStockObject(NULL_PEN));

         int radius = CurrentPen->thinkness / 2;

         Ellipse(RoadMapDrawingBuffer,
			   end_points[0].x - radius, end_points[0].y - radius,
			   radius + end_points[0].x + 1,
			   radius + end_points[0].y + 1);

         Ellipse(RoadMapDrawingBuffer,
			   end_points[1].x - radius, end_points[1].y - radius,
			   radius + end_points[1].x + 1,
			   radius + end_points[1].y + 1);

         SelectObject(RoadMapDrawingBuffer, oldPen);
	   }
#endif

		points += count_of_points;
		lines += 1;
	}

//   DeleteObject(SelectObject(RoadMapDrawingBuffer, oldPen));
//   DeleteObject(SelectObject(RoadMapDrawingBuffer, oldBrush));
}


