/* roadmap_androidmain.h - the interface to the android main module
 *
 * LICENSE:
 *
 *   Copyright 2009 Alex Agranovich (AGA)
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
 */

#ifndef INCLUDE__ROADMAP_ANDROID_OGL_H_
#define INCLUDE__ROADMAP_ANDROID_OGL_H_


#include <EGL/egl.h>
#include <GLES/gl.h>
#include <GLES/glext.h>
#include "roadmap_androidmain.h"
//#include <ui/EventHub.h>

typedef struct
{
	EGLContext context;
	EGLDisplay display;
	EGLSurface surface;
} EGLContextPack;

void roadmap_canvas_prepare_main_context( void );
void roadmap_canvas_prepare( void );
void roadmap_canvas_draw_multiple_lines_slow( int count, int *lines, RoadMapGuiPoint *points, int fast_draw );
#endif /* INCLUDE__ROADMAP_ANDROID_OGL_H_ */
