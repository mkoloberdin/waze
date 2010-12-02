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

#include "roadmap_winogl.h"

#include "roadmap_canvas.h"
#include "roadmap_canvas_ogl.h"

#include "roadmap_path.h"
#include "roadmap_lang.h"
//#include "roadmap_wchar.h"


static int sgOGLWidth = 320;
static int sgOGLHeight = 455;
static int sgOGLPixelFormat = 0;

#define CANVAS_OGL_TARGET_FPS 25		// FPS
#define SSD_CANVAS_OGL_TARGET_FPS 10		// FPS
#define EGL_CONTEXT_INITIALIZER {NULL, NULL, NULL};

#define ANDROID_SD_FONT_FACTOR         1.2F
#define ANDROID_HD_FONT_FACTOR			1.8F
#define ANDROID_MIN_LINE_THICKNESS		3.0F

EGLContextPack sgMainEGLContextPack = EGL_CONTEXT_INITIALIZER;
static EGLContextPack sgSavedEGLContextPack = EGL_CONTEXT_INITIALIZER;
static EGLContextPack sgPBufferEGLContextPack = EGL_CONTEXT_INITIALIZER;
static BOOL sgEGLContextPackIsSaved = FALSE;

BOOL isMTEvent = FALSE;
POINT MTpts[2];


void setMTEvent(BOOL isMT){
	isMTEvent = isMT;
}

void setMtPoints(POINT Points[2]){
	MTpts[0] = Points[0];
	MTpts[1] = Points[1];

}

BOOL roadmap_canvas_is_cording(void){
	return isMTEvent;
}

roadmap_canvas_get_cording_pt (RoadMapGuiPoint points[2]){
	points[0].x = MTpts[0].x;
	points[0].y = MTpts[0].y;
	points[1].x = MTpts[1].x ;
	points[1].y = MTpts[1].y;

}
#define egl_error( s )	\
{	\
	unsigned err = eglGetError(); \
	if(err != EGL_SUCCESS)	\
	{						\
		roadmap_log( ROADMAP_ERROR, "%s. EGL Error %x", s, err );	\
	}	\
}
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
	if (!sgMainEGLContextPack.display || !sgMainEGLContextPack.surface || !sgMainEGLContextPack.context) {
		roadmap_log( ROADMAP_DEBUG, "roadmap_canvas_refresh() called before init !" );
		return;
	}
	
	if ( eglSwapBuffers( sgMainEGLContextPack.display,
			sgMainEGLContextPack.surface ) != EGL_TRUE )
	{
		roadmap_log( ROADMAP_ERROR, "Swapping buffers failed!" );
	}
}

void roadmap_canvas_init_egl (HWND hWnd) {
  EGLConfig configs[10];
  EGLint matchingConfigs;	
HDC hDC;
  RECT r;
  static BOOL initialized = FALSE;
EGLDisplay glesDisplay;  // EGL display
EGLSurface glesSurface;	 // EGL rendering surface
EGLContext glesContext;	 // EGL rendering context

//Texture handles
GLuint texture1 = 0; 
GLuint texture2 = 0;
GLuint fontTexture = 0;


  /*configAttribs is a integers list that holds the desired format of 
   our framebuffer. We will ask for a framebuffer with 24 bits of 
   color and 16 bits of z-buffer. We also ask for a window buffer, not 
   a pbuffer or pixmap buffer*/	
  const EGLint configAttribs[] =
  {
      EGL_DEPTH_SIZE,     16,
	  EGL_BUFFER_SIZE,	  32,
      EGL_SURFACE_TYPE,   EGL_WINDOW_BIT,
      EGL_NONE,           EGL_NONE
  };

  const EGLint configAttribs2[] =
  {
        EGL_NONE,           EGL_NONE
  };

  if (initialized)
	  return;
  if (hWnd == NULL)
	  return;
  initialized = TRUE;

  hDC = GetWindowDC(hWnd);
  glesDisplay = eglGetDisplay(hDC);	 //Ask for an available display
  if( glesDisplay == NULL )
  {
		roadmap_log (ROADMAP_FATAL, "Error getting EGL_DEFAULT_DISPLAY !");	
  }


  /* Initialize display */
  if( eglInitialize( glesDisplay, NULL, NULL ) == EGL_FALSE )
  {
		roadmap_log (ROADMAP_FATAL, "Error initializing egl view !");
  }
  sgMainEGLContextPack.display = glesDisplay;
	
  /*Ask for the framebuffer confiburation that best fits our 
  parameters. At most, we want 10 configurations*/
  if(!eglChooseConfig(glesDisplay, configAttribs, &configs[0], 10,  &matchingConfigs)) 
   return ;
	
  //If there isn't any configuration enough good
  if (matchingConfigs < 1)  {
		if(!eglChooseConfig(glesDisplay, configAttribs2, &configs[0], 10,  &matchingConfigs)) 
			return ;
		//If there isn't any configuration enough good
		if (matchingConfigs < 1)
		  roadmap_log (ROADMAP_FATAL, "Error initializing egl view  - No configs found!");
  }

  /*eglCreateWindowSurface creates an onscreen EGLSurface and returns 
  a handle  to it. Any EGL rendering context created with a 
  compatible EGLConfig can be used to render into this surface.*/
  glesSurface = eglCreateWindowSurface(glesDisplay, configs[0], hWnd, 0);	
  if(!glesSurface){
	  int i;
	  for (i = 1; i < matchingConfigs; i++){
		  glesSurface = eglCreateWindowSurface(glesDisplay, configs[i], hWnd, 0);	
		  if (glesSurface)
			  break;
	  }
	  if(!glesSurface){
		int err = eglGetError();
		roadmap_log (ROADMAP_FATAL, "Error initializing egl - eglCreateWindowSurface returned %d error!", err);
		return ;
	  }
  }
  sgMainEGLContextPack.surface = glesSurface;


  // Let's create our rendering context
  glesContext=eglCreateContext(glesDisplay,configs[0],0,0);

  if(!glesContext) {
	  int err = eglGetError();
	  return ;
  }
  sgMainEGLContextPack.context = glesContext;

  if( eglMakeCurrent( glesDisplay, glesSurface, glesSurface, glesContext ) == EGL_FALSE )
  {
		roadmap_log (ROADMAP_FATAL, "eglMakeCurrent failed !");
  }
    
  eglQuerySurface( sgMainEGLContextPack.display, sgMainEGLContextPack.surface, EGL_WIDTH, &sgOGLWidth );
  eglQuerySurface( sgMainEGLContextPack.display, sgMainEGLContextPack.surface, EGL_HEIGHT, &sgOGLHeight );

  GetWindowRect(hWnd, &r);  
  glViewport(r.left, r.top, r.right - r.left, r.bottom - r.top);		  

  roadmap_canvas_init(r.right - r.left, r.bottom - r.top, 16);

  return ;
  
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
	LPWSTR text_unicode = ConvertToWideChar(text, CP_UTF8);
   wcsncpy(output, text_unicode, size);
   free(text_unicode);
   output[size - 1] = 0;

   return wcslen(output);
}


 void roadmap_canvas_mouse_pressed( int aX, int aY )
{
	RoadMapGuiPoint point;
	point.x = aX;
	point.y = aY;
	(*RoadMapCanvasMouseButtonPressed)( &point );
}

 void roadmap_canvas_mouse_released( POINT *data )
{
	RoadMapGuiPoint point;
    point.x = (short)data->x;
    point.y = (short)data->y;
	(*RoadMapCanvasMouseButtonReleased)( &point );
}

 void roadmap_canvas_mouse_moved( POINT *data )
{
	RoadMapGuiPoint point;
	point.x = (short)data->x;
	point.y = (short)data->y;
	(*RoadMapCanvasMouseMoved)( &point );
}
void roadmap_canvas_button_pressed(POINT *data) {
   RoadMapGuiPoint point;

   point.x = (short)data->x;
   point.y = (short)data->y;

   (*RoadMapCanvasMouseButtonPressed) (&point);

}

void roadmap_canvas_mt_released(POINT *data) {
   RoadMapGuiPoint point;

   point.x = (short)data->x;
   point.y = (short)data->y;

   (*RoadMapCanvasMouseButtonReleased) (&point);

}


void roadmap_canvas_button_released(POINT *data) {
   RoadMapGuiPoint point;

   if (isMTEvent)
	   return;

   point.x = (short)data->x;
   point.y = (short)data->y;

   (*RoadMapCanvasMouseButtonReleased) (&point);

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
		static int first_time = 1;

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

	aa_factor = 0.0F;

    if ( roadmap_screen_is_hd_screen() )
    {
    	font_factor = ANDROID_HD_FONT_FACTOR;
    }
	if ( first_time )
	{ 
	   roadmap_canvas_ogl_configure( aa_factor, font_factor, ANDROID_MIN_LINE_THICKNESS );
       	   first_time = 0;
	   roadmap_canvas3_ogl_prepare();
	}

}

void roadmap_canvas_new (HWND hWnd)
{
	RECT r;
	static BOOL first_time = TRUE;
	
	if (hWnd == NULL)
		return;
	GetWindowRect(hWnd, &r);  

	sgOGLWidth = r.right - r.left; 
	sgOGLHeight = r.bottom - r.top;
	
	if (!first_time) {
		roadmap_log (ROADMAP_DEBUG, "roadmap_canvas_new() - already init (probably rotation)");
	} else {
		roadmap_canvas_init_egl(hWnd);
		first_time = FALSE;
		roadmap_log (ROADMAP_WARNING, "Device OpenGL renderer string: '%s'", glGetString ( GL_RENDERER ));
	}
	
	
	roadmap_canvas_prepare();

	// Run the configurator
	(*RoadMapCanvasConfigure) ();

	// AGA DEBUG - Reduce level
	roadmap_log( ROADMAP_INFO, "roadmap_canvas_new. Width: %d, Height: %d", sgOGLWidth, sgOGLHeight );	
}

int roadmap_canvas_is_active()
{
	//return FreeMapNativeManager_IsActive();
	return TRUE;
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
