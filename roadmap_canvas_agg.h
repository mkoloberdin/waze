/* roadmap_canvas_agg.h - manage the roadmap canvas using agg
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
 */

#ifndef INCLUDE__ROADMAP_CANVAS_AGG__H
#define INCLUDE__ROADMAP_CANVAS_AGG__H

#include "roadmap_gui.h"
#include "agg_pixfmt_rgba.h"
#include "agg_pixfmt_rgb_packed.h"

struct roadmap_canvas_image {
   agg::rendering_buffer rbuf;
   agg::pixfmt_rgba32 pixfmt;

   roadmap_canvas_image():pixfmt(rbuf) {}
};

EXTERN_C void roadmap_canvas_set_properties( int aWidth, int aHeight, int aPixelFormat );

extern RoadMapCanvasMouseHandler RoadMapCanvasMouseButtonPressed;

extern RoadMapCanvasMouseHandler RoadMapCanvasMouseButtonReleased;

extern RoadMapCanvasMouseHandler RoadMapCanvasMouseMoved;

extern RoadMapCanvasConfigureHandler RoadMapCanvasConfigure;

void roadmap_canvas_agg_configure (unsigned char *buf, int width, int height, int stride);

/* GUI specific implementation */
int roadmap_canvas_agg_to_wchar (const char *text, wchar_t *output, int size);
agg::rgba8 roadmap_canvas_agg_parse_color (const char *color);

void roadmap_canvas_agg_free_image (RoadMapImage image);

void roadmap_canvas_native_draw_multiple_lines (int count, int *lines,
				RoadMapGuiPoint *points, int r, int g, int b, int thickness);

RoadMapImage roadmap_canvas_agg_load_png( const char *full_name );
RoadMapImage roadmap_canvas_agg_load_bmp( const char *full_name );



#endif // INCLUDE__ROADMAP_CANVAS_AGG__H

