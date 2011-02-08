/* roadmap_canvas_font.h - manage the roadmap canvas font.
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi R.
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
 * DESCRIPTION:
 *
 *   This defines a toolkit-independent interface for the canvas used
 *   to draw the map. Even while the canvas implementation is highly
 *   toolkit-dependent, this interface is not.
 */

#ifndef INCLUDE__ROADMAP_CANVAS_FONT__H
#define INCLUDE__ROADMAP_CANVAS_FONT__H

#define ACTUAL_FONT_SIZE(size) ((size / 10 +1) * 10)

typedef struct  {
   RoadMapImage   image;
   RoadMapImage   outline_image;
   float          advance_x;
   int            left;
   int            top;
   int            hash;
} RoadMapFontImage;

RoadMapFontImage *roadmap_canvas_font_tex (wchar_t ch, int size, int bold);
void roadmap_canvas_font_metrics (int size, int *ascent, int *descent, int bold);
void roadmap_canvas_font_shutdown();

#endif // INCLUDE__ROADMAP_CANVAS_FONT__H

