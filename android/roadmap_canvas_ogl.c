/* roadmap_canvas_ogl.c - Android opengl renderer implementation
 *
 * LICENSE:
 *
  *   Copyright 2009 Alex Agranovich (AGA),    Waze Ltd
 *
 *   This file is part of Waze.
 *
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
 */


#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_screen.h"
#include "roadmap_gui.h"
#include "roadmap_device_events.h"
#include "roadmap_performance.h"
#include "roadmap_canvas.h"
#include "roadmap_canvas_ogl.h"

#include "roadmap_path.h"
#include "roadmap_lang.h"
#include "roadmap_wchar.h"

#include "roadmap_androidogl.h"
#include "roadmap_androidmain.h"
#include "JNI/FreeMapJNI.h"
#include "ssd/ssd_dialog.h"

static int sgOGLWidth = 320;
static int sgOGLHeight = 455;
static int sgOGLPixelFormat = 0;
static const int sgcCordingPointInvalid[2] = {-1, -1 };
static int sgCordingPointsXY[2*MAX_CORDING_POINTS];
static int sgCordingPointsCount = 0;

#define CANVAS_OGL_TARGET_FPS 25		// FPS
#define SSD_CANVAS_OGL_TARGET_FPS 10		// FPS
#define EGL_CONTEXT_INITIALIZER {NULL, NULL, NULL};

#define ANDROID_LD_FONT_FACTOR         0.9F
#define ANDROID_SD_FONT_FACTOR         1.2F
#define ANDROID_HD_FONT_FACTOR			1.8F
#define ANDROID_MIN_LINE_THICKNESS		3.0F

EGLContextPack sgMainEGLContextPack = EGL_CONTEXT_INITIALIZER;
static EGLContextPack sgSavedEGLContextPack = EGL_CONTEXT_INITIALIZER;
static EGLContextPack sgPBufferEGLContextPack = EGL_CONTEXT_INITIALIZER;
static BOOL sgEGLContextPackIsSaved = FALSE;

#define egl_error( s )	\
{	\
	unsigned err = eglGetError(); \
	if(err != EGL_SUCCESS)	\
	{						\
		roadmap_log( ROADMAP_ERROR, "%s. EGL Error %x", s, err );	\
	}	\
}

#define PNT_LIMIT_X( pnt ) ({ \
   (pnt) = ( (pnt) < 0 ) ? 0 : (pnt); \
   (pnt) = ( (pnt) >= sgOGLWidth ) ? (sgOGLWidth-1) : (pnt), \
   (pnt); \
})

#define PNT_LIMIT_Y( pnt ) ({ \
   (pnt) = ( (pnt) < 0 ) ? 0 : (pnt); \
   (pnt) = ( (pnt) >= sgOGLHeight ) ? (sgOGLHeight-1) : (pnt), \
   (pnt); \
})

/*************************************************************************************************
 * roadmap_canvas_init
 * Initializes the canvas context this the display device parameters
 */
 void roadmap_canvas_init( int aWidth, int aHeight, int aPixelFormat )
{
	int cur_orientation = ( sgOGLHeight > sgOGLWidth );
	int new_orientation = ( aHeight > aWidth );
	EGLint w, h;

	sgOGLHeight = aHeight;
	sgOGLWidth = aWidth;
	sgOGLPixelFormat = aPixelFormat;

	/*
     * Screen type
     */
	roadmap_screen_set_screen_type( roadmap_canvas_get_generic_screen_type( sgOGLWidth, sgOGLHeight ) );

	roadmap_canvas_prepare_main_context();

	roadmap_canvas_prepare();

	roadmap_log( ROADMAP_INFO, "Canvas properties. Width: %d, Height: %d.", sgOGLWidth, sgOGLHeight );
}

/*
 * Swap OGL buffers
 */
static void roadmap_canvas_swap_buffers()
{
	static int swap_ignore_count = 1;

	if ( swap_ignore_count > 0 )
	{
		swap_ignore_count--;
		return;
	}

  	/*
	 * Must flush the GPU pipeline because of the Froyo crashes
	 */
	glFinish();
	check_gl_error();

	/*
    * Post swap buffers call
    */
   FreeMapNativeManager_SwapBuffersEgl();
}

const EGLContextPack* roadmap_canvas_get_main_context()
{
	return &sgMainEGLContextPack;
}

static void roadmap_canvas_load_current_context( EGLContextPack* ctx )
{
	ctx->display = eglGetCurrentDisplay();
	egl_error( "eglGetCurrentDisplay" )
	ctx->context = eglGetCurrentContext();
	egl_error( "eglGetCurrentContext" )
	ctx->surface = eglGetCurrentSurface( EGL_DRAW );
	egl_error( "eglGetCurrentSurface" )
}

void roadmap_canvas_prepare_main_context( void )
{

	roadmap_canvas_load_current_context( &sgMainEGLContextPack );

	roadmap_log( ROADMAP_INFO, "Main context pack: 0x%x, 0x%x, 0x%x", sgMainEGLContextPack.display,
			sgMainEGLContextPack.surface, sgMainEGLContextPack.display );


	return;
}

/*************************************************************************************************
 * roadmap_canvas_refresh
 * Canvas refresh interface function
 */
 void roadmap_canvas_refresh (void)
{
	/*
	 * Enable the menu if the map is shown ( no dialogs on it )
	 */
	FreeMapNativeManager_SetIsMenuEnabled( !ssd_dialog_is_currently_active() );

	if ( !roadmap_screen_get_background_run() )
	{
		roadmap_canvas_swap_buffers();
	}
}

/*************************************************************************************************
 * roadmap_canvas_ogl_to_wchar
 * Converts multibyte to the wchar
 */
 int roadmap_canvas_ogl_to_wchar (const char *text, wchar_t *output, int size)
{
   int length = mbstowcs( ( RM_WCHAR_T *) output, text, size - 1);
   output[length] = 0;
   return length;
}

static BOOL is_ogl_matrix_load_available()
{
   GLfloat mtx[16] = {0};
   BOOL res = TRUE;
   int i;

   glGetFloatv( GL_MODELVIEW_MATRIX, mtx );
   for( i = 0; i < 16; ++i )
   {
      if ( mtx[i] != 0 )
         break;
   }
   if ( i == 16 )
      res = FALSE;

   glGetFloatv( GL_PROJECTION_MATRIX, mtx );
   for( i = 0; i < 16; ++i )
   {
      if ( mtx[i] != 0 )
         break;
   }
   if ( i == 16 )
      res = FALSE;

   if ( !res || ( glGetError() != GL_NO_ERROR ) || ( eglGetError() != EGL_SUCCESS ) )
   {
      roadmap_log( ROADMAP_WARNING, "OpenGL matrix load not available - using matrix tracking" );
      res = FALSE;
   }
   return res;
}

static inline void cording_reset()
{
   int i;
   const int size = sizeof( sgcCordingPointInvalid ) * sizeof( sgcCordingPointInvalid[0] );
   for ( i = 0; i < MAX_CORDING_POINTS; ++i )
   {
      memcpy( &sgCordingPointsXY[2*i], sgcCordingPointInvalid, size );
   }
   sgCordingPointsCount = 0;
}

static inline void cording_set( int xy[], int count )
{
   // count should be < MAX_CORDING_POINTS
   int size = 2 * count * sizeof( int );
   memcpy( sgCordingPointsXY, xy, size );
   sgCordingPointsCount = count;
}

static inline void cording_debug( const char* entry_point, int count )
{
   int i = 0;
   roadmap_log_raw_data_fmt( " ####### Cording at %s. Count: %d \n", entry_point, count );
   roadmap_log_raw_data_fmt( " ####### Points count: %d. Points: ", sgCordingPointsCount );
   for ( ;i < MAX_CORDING_POINTS; ++i )
   {
      roadmap_log_raw_data_fmt( "(%d,%d) ", sgCordingPointsXY[2*i], sgCordingPointsXY[2*i+1] );
   }
   roadmap_log_raw_data_fmt( "\n" );
}

void roadmap_canvas_get_cording_pt ( RoadMapGuiPoint points[MAX_CORDING_POINTS] )
{
   int i = 0;
   for( ; i < MAX_CORDING_POINTS; ++i )
   {
      if ( i < sgCordingPointsCount )
      {
         points[i].x = sgCordingPointsXY[2*i];
         points[i].y = sgCordingPointsXY[2*i+1];
      }
      else
      {
         points[i].x = -1;
         points[i].y = -1;
      }
   }
}

int roadmap_canvas_is_cording( void )
{
   return ( sgCordingPointsCount > 1 );
}


void roadmap_canvas_mouse_pressed( int aXY[], int aCount )
{
   RoadMapGuiPoint point;
   point.x = PNT_LIMIT_X( aXY[0] );
   point.y = PNT_LIMIT_Y( aXY[1] );

   if ( roadmap_main_mtouch_supported() )
   {
      cording_set( aXY, aCount );
   }

//   cording_debug( "Pressed", aCount );
	(*RoadMapCanvasMouseButtonPressed)( &point );
}

void roadmap_canvas_mouse_released( int aXY[], int aCount )
{
	RoadMapGuiPoint point;
   point.x = PNT_LIMIT_X( aXY[0] );
   point.y = PNT_LIMIT_Y( aXY[1] );

	if ( roadmap_main_mtouch_supported() )
	{
	   cording_reset();
	}
//	cording_debug( "Released", aCount );
  (*RoadMapCanvasMouseButtonReleased)( &point );
}

void roadmap_canvas_mouse_moved( int aXY[], int aCount )
{
	RoadMapGuiPoint point;
   point.x = PNT_LIMIT_X( aXY[0] );
   point.y = PNT_LIMIT_Y( aXY[1] );

   if ( roadmap_main_mtouch_supported() )
   {
      cording_set( aXY, aCount );
   }
//	cording_debug( "Moved I", aCount );
	(*RoadMapCanvasMouseMoved)( &point );
}

 int roadmap_canvas_width (void) {

   return sgOGLWidth;
}

 int roadmap_canvas_is_landscape()
{

   return ( roadmap_canvas_width() > roadmap_canvas_height() );
}

 int roadmap_canvas_height (void) {
   return sgOGLHeight;
}


void roadmap_canvas_prepare()
{
	float aa_factor, font_factor = ANDROID_SD_FONT_FACTOR;

	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	glOrthof( 0.0f, sgOGLWidth,   // left,right
			 sgOGLHeight, 0.0f, // bottom, top
			-100.0f, 100.0f ); // near, far

	glViewport( 0, 0, sgOGLWidth, sgOGLHeight );

	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	glEnable( GL_BLEND );

	glDisable(GL_DEPTH_TEST);

	glDisable( GL_DITHER );

	glDisable( GL_LIGHTING );

    glDisableClientState( GL_COLOR_ARRAY );

    glDisableClientState( GL_NORMAL_ARRAY );

	glEnableClientState( GL_VERTEX_ARRAY );

	check_gl_error()

	aa_factor = ( roadmap_main_get_build_sdk_version() <= ANDROID_OS_VER_DONUT ) ? 0.5F : 0.0F;

    if ( roadmap_screen_is_hd_screen() )
    {
    	font_factor = ANDROID_HD_FONT_FACTOR;
    }
    if ( roadmap_screen_is_ld_screen() )
    {
      font_factor = ANDROID_LD_FONT_FACTOR;
    }

   if ( !strcasecmp( roadmap_main_get_device_name(), "U8100") /* Huaiwei */ ||
         !strcasecmp( roadmap_main_get_device_name(), "generic") /* Emulator */ )
   {
      roadmap_canvas_set_max_tex_units( 1 );
   }

	roadmap_canvas_ogl_configure( aa_factor, font_factor, ANDROID_MIN_LINE_THICKNESS );

	// Enable matrix tracking for the earlier android builds
	// to solve the absence of API call glGetFloatv( XXX_MATRIX, mtx )
	if ( !is_ogl_matrix_load_available() )
	{
	   roadmap_canvas3_glmatrix_enable();
	}

	roadmap_canvas3_ogl_prepare();
}

void roadmap_canvas_new ( void )
{
	// Run the configurator
	(*RoadMapCanvasConfigure) ();

	roadmap_log( ROADMAP_INFO, "roadmap_canvas_new. Width: %d, Height: %d.", sgOGLWidth, sgOGLHeight );
}

int roadmap_canvas_is_active()
{
	return FreeMapNativeManager_IsActive();
}

/*
 * PBuf not in use currently
 */
#if 0
static void init_pbuffer_context()
{
	  EGLint numConfigs = 0;
	  GLint maxDim;
	  EGLConfig config = 0;
	  EGLBoolean res;
	  sgPBufferEGLContextPack.display = eglGetCurrentDisplay();
	  sgPBufferEGLContextPack.context = eglGetCurrentContext();
	  //  = eglGetCurrentSurface(EGL_DRAW); //assuming read and draw are the same surface. If not, save the read surface as well
	  res = eglGetConfigs ( sgPBufferEGLContextPack.display, &config, 1, &numConfigs );
	  if ( !res )
	  {
		  roadmap_log( ROADMAP_ERROR, "Cannot get config. Error: 0x%04x",
				  eglGetError() );
	  }


	  maxDim = (roadmap_canvas_width() > roadmap_canvas_height()) ? roadmap_canvas_width() : roadmap_canvas_height(); //will be good for landscape and portrate
	  EGLint attributes[] = { EGL_WIDTH, next_pot( maxDim ),
	                              EGL_HEIGHT, next_pot( maxDim ),
//	                              EGL_TEXTURE_FORMAT, EGL_TEXTURE_RGBA,
//	                              EGL_TEXTURE_TARGET, EGL_TEXTURE_2D,
	                              EGL_NONE };
	  //create Pbuffer Surface
	  sgPBufferEGLContextPack.surface = eglCreatePbufferSurface ( sgPBufferEGLContextPack.display,
			  config, attributes );

	  if ( sgPBufferEGLContextPack.surface == EGL_NO_SURFACE )
	  {
		  roadmap_log( ROADMAP_ERROR, "Cannot create the offline surface. Error: 0x%04x. Config: 0x%x. Num config: %d. \
				  Display: 0x%x!. Max dim: %d, %d",
				  eglGetError(), config, numConfigs, sgPBufferEGLContextPack.display, maxDim, next_pot( maxDim ) );
	  }
	  else
	  {
		  roadmap_log( ROADMAP_ERROR, "Success create the offline surface. Surface: 0x%x. Config: 0x%x. Num config: %d. \
				  Display: 0x%04x!. Max dim: %d, %d. Context: %d",
				  sgPBufferEGLContextPack.surface, config, numConfigs,
				  sgPBufferEGLContextPack.display, maxDim, next_pot( maxDim ), sgPBufferEGLContextPack.context );

	  }
}
void roadmap_canvas_set_current_context( const EGLContextPack* ctx )
{
	EGLBoolean res;
	roadmap_log( ROADMAP_INFO, "Making current display: 0x%x, 0x%x, 0x%x", ctx->display,
			ctx->surface, ctx->context );

	res = eglMakeCurrent( ctx->display, ctx->surface, ctx->surface, ctx->context );

	if ( res == EGL_FALSE )
	{
		roadmap_log( ROADMAP_ERROR, "Failed make current context (%x,%x,%x)!!! Error: 0x%04x",
				ctx->display, ctx->surface, ctx->context, eglGetError() );
	}
}


static BOOL is_pbuf_context( const EGLContextPack* ctx )
{
	BOOL res = TRUE;
	res = res && ( ctx->display == sgPBufferEGLContextPack.display );
	res = res && ( ctx->surface == sgPBufferEGLContextPack.surface );
	res = res && ( ctx->context == sgPBufferEGLContextPack.context );
	return res;
}

void roadmap_canvas_set_pbuf_context( BOOL save_prev )
{
	EGLContextPack ctx;

	roadmap_canvas_load_current_context( &ctx );
	if ( is_pbuf_context( &ctx ) )
	{
		sgEGLContextPackIsSaved = FALSE;
		return;
	}
	if ( save_prev )
	{
		sgEGLContextPackIsSaved = TRUE;
		roadmap_canvas_save_current_context();
	}
	roadmap_canvas_set_current_context( &sgPBufferEGLContextPack );
}
#endif
