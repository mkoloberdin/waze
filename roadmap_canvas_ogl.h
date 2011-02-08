/* roadmap_canvas_ogl.h - manage the roadmap canvas using the OpenGL ES library.
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
 *   toolkit-dependent, the interface must not.
 */

#ifndef INCLUDE__ROADMAP_CANVAS_OGL__H
#define INCLUDE__ROADMAP_CANVAS_OGL__H

#include "roadmap.h"
#include "roadmap_canvas.h"
#include "roadmap_canvas3d.h" // roadmap_canvas3_ogl_prepare()

#ifdef GTK2_OGL
#include <GL/gl.h>
#endif// GTK2_OGL


#ifdef IPHONE
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include "roadmap_wchar.h"
#include "roadmap_libpng.h"
#endif //IPHONE

#ifdef ANDROID
#include "android/roadmap_androidogl.h"
#endif

#if defined(__SYMBIAN32__) || defined (_WIN32)
#include <GLES/gl.h> // OpenGL ES header file
#include <GLES/egl.h>      // EGL header file
#endif


extern RoadMapCanvasMouseHandler RoadMapCanvasMouseButtonPressed;
extern RoadMapCanvasMouseHandler RoadMapCanvasMouseButtonReleased;
extern RoadMapCanvasMouseHandler RoadMapCanvasMouseMoved;
extern RoadMapCanvasConfigureHandler RoadMapCanvasConfigure;

int roadmap_canvas_ogl_to_wchar (const char *text, wchar_t *output, int size);
void roadmap_canvas_ogl_configure( float anti_alias_factor, float font_factor, float thickness_factor );
BOOL roadmap_canvas_is_ready( void );
void roadmap_canvas_set_max_tex_units( int units_num );

//#define OPENGL_DEBUG

#ifdef OPENGL_DEBUG
#define check_gl_error() { \
   int err = glGetError();		\
   if (err != GL_NO_ERROR) {			\
      roadmap_log (ROADMAP_ERROR, "GL ERROR ! 0x%04x", err); \
   } \
}
#else
#define check_gl_error() {}
#endif //OPENGL_DEBUG

typedef BOOL (*RMOglImageRestoreCb)( RoadMapImage image );	/* Image restoration callback. Called when the image texture has to be restored */

#define		OGL_IMAGE_UNMANAGED_ID_UNKNOWN		-1

struct roadmap_canvas_image {
   unsigned long  		texture;
   int            		width;
   int            		height;
   void           		*buf; 		// temporary buffer to hold image data
   unsigned long  		frame_buf;
   char* 	  			full_path;
   int			  		is_valid;
   int 					unmanaged_id;	// The index of the unmanaged list entry for this image. OGL_IMAGE_MANAGED_ID for managed images
   RMOglImageRestoreCb	restore_cb;		// The image restoration callback. Allows restore the texture if deleted
   long			 update_time;		/* AGA DEBUG */
   RoadMapGuiPoint   offset;
} roadmap_canvas_image;


#if defined(ROADMAP_CANVAS_C) || defined(ROADMAP_CANVAS_FONT_C) || defined(ROADMAP_CANVAS_OGL_C) || defined(ROADMAP_CANVAS_TILE_C)
#ifndef INLINE_DEC
#define INLINE_DEC static
#endif //INLINE_DEC
INLINE_DEC int next_pot(int num) {
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
#endif

#endif // INCLUDE__ROADMAP_CANVAS_OGL__H
