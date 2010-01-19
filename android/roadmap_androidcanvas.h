/* roadmap_androidcanvas.h - the interface to the android canvas module
 *
 * LICENSE:
 *
 *   Copyright 2008 Alex Agranovich
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
 *
 *
 *
 */

#ifndef INCLUDE__ROADMAP_ANDROID_CANVAS__H
#define INCLUDE__ROADMAP_ANDROID_CANVAS__H

#include "roadmap.h"

#define 	DEFAULT_BPP		4		// Bytes per pixel

typedef struct
{
	int width;
	int height;
	int pixel_format;
	unsigned int *pCanvasBuf;		// Native canvas buffer
} roadmap_android_canvas_type;

EXTERN_C void roadmap_canvas_new( void );
EXTERN_C void GetAndrCanvas( const roadmap_android_canvas_type** aCanvasPtr );
EXTERN_C BOOL roadmap_canvas_agg_is_landscape( void );
#endif // INCLUDE__ROADMAP_ANDROID_CANVAS__H

