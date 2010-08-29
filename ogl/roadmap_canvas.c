/* roadmap_canvas.c - manage the canvas that is used to draw the map with OpenGL ES.
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
 * SYNOPSYS:
 *
 *   See roadmap_canvas.h.
 */

#define ROADMAP_CANVAS_C

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "colors.h"
#include "roadmap.h"
#include "roadmap_performance.h"
#include "roadmap_types.h"
#include "roadmap_gui.h"
#include "roadmap_screen.h"
#include "roadmap_canvas.h"
#include "roadmap_canvas_ogl.h"
#include "roadmap_canvas_atlas.h"
#include "roadmap_canvas_font.h"
#include "roadmap_math.h"
#include "roadmap_path.h"
#include "roadmap_lang.h"

#ifdef USE_FRIBIDI
#ifdef GTK2_OGL
#include <fribidi.h>
#else
#include "../libfribidi/fribidi.h"
#endif// GTK2_OGL
#endif //USE_FRIBIDI

#ifdef GTK2_OGL
#include <GL/glu.h>
#else
#include "glu/glu.h"
#endif// GTK2_OGL

#define OPTIMIZE_SQRT

#define FONT_DEFAULT_SIZE  14
#define Z_LEVEL            -6
#define MAX_AA_SIZE        32
#define IMAGE_HINT         "image"
#define MAX_TEX_UNITS      8

#ifndef MIN
#define MIN(A,B)	({ __typeof__(A) __a = (A); __typeof__(B) __b = (B); __a < __b ? __a : __b; })
#endif
#ifndef MAX
#define MAX(A,B)	({ __typeof__(A) __a = (A); __typeof__(B) __b = (B); __a < __b ? __b : __a; })
#endif


#define 	OGL_UNMANAGED_IMAGES_LIST_SIZE				64

#define 	OGL_TEX_NEXT_POINT_OFFSET					0.0001F

// some convienence constants for the texture dimensions:
// point size, size minus one, half size squared, double size, point center, radius squared
#define psz (phf * 2)
#define psm (psz - 1)
#define phs (phf * phf)
#define pdb (psz * 2)
#define pct (psz + phf)
#define prs ((phf-1)*(phf-1))


#define CANVAS_IMAGE_FROM_BUF_TYPE		"IMAGE_FROM_BUF"
#define CANVAS_IMAGE_NEW_TYPE			"IMAGE_NEW"

#define CANVAS_INVALID_TEXTURE_ID		0xFFFFFFFFL

#define OGL_FLT_SHIFT_VALUE                     0.5F        // Shifting of the central filter point
// in terms of subpixel accuracy
#define OGL_FLT_SHIFT_POS( coord ) ( (GLfloat) coord + OGL_FLT_SHIFT_VALUE )      // positive shift
#define OGL_FLT_SHIFT_NEG( coord ) ( (GLfloat) coord - OGL_FLT_SHIFT_VALUE )      // negative shift

enum states {
   CANVAS_GL_STATE_NOT_READY = 0,
   CANVAS_GL_STATE_GEOMETRY = 1,
   CANVAS_GL_STATE_IMAGE,
   CANVAS_GL_STATE_FORCE_GEOMETRY
};

struct RoadMapColor {
   float r;
   float g;
   float b;
   float a;
};

struct roadmap_canvas_pen {

   struct roadmap_canvas_pen *next;

   char  *name;
   //int style;
   float lineWidth;
   struct RoadMapColor stroke;
   struct RoadMapColor background;
};

typedef struct roadmap_gl_vertex {
   GLfloat	x;
   GLfloat	y;
   GLfloat  z;
   GLfloat	tx;
   GLfloat	ty;
} roadmap_gl_vertex;

typedef struct roadmap_gl_poly_vertex {
   GLdouble x;
   GLdouble y;
   GLdouble z;
} roadmap_gl_poly_vertex;

static struct roadmap_canvas_pen *RoadMapPenList = NULL;
static GLuint                    RoadMapAATex = 0;
static GLuint                    RoadMapAATexNf = 0;

static int 						 RoadMapCanvasSavedState = CANVAS_GL_STATE_NOT_READY;
static RoadMapPen CurrentPen;

static float 					RoadMapFontFactor 	= 1.0F;
static float 					RoadMapMinThickness = 2.0F;
static float 					RoadMapAAFactor 	= 0.0F;
/*
 * Unmanaged list definitions
 */
static RoadMapImage RMUnmanagedImages[OGL_UNMANAGED_IMAGES_LIST_SIZE] = {0};

/* Multiple lines buffer */
static roadmap_gl_vertex Glpoints[12288];

static roadmap_gl_poly_vertex GLcombined[1024];
static int                 GlcombinedCount;
static GLuint              GlpointsCount;
static GLenum              GlpointsType;

static GLuint  tex_units[MAX_TEX_UNITS];
static GLint   num_tex_units;
static int     current_tex_unit;


#if !defined(INLINE_DEC)
#define INLINE_DEC
#endif

static void generate_aa_tex();
INLINE_DEC float frsqrtes_nr(float x);


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

RoadMapCanvasMouseHandler RoadMapCanvasMouseScroll =
                                     roadmap_canvas_ignore_mouse;

static void roadmap_canvas_ignore_configure (void) {}

RoadMapCanvasConfigureHandler RoadMapCanvasConfigure =
                                     roadmap_canvas_ignore_configure;

BOOL roadmap_canvas_set_image_texture( RoadMapImage image );
static BOOL roadmap_canvas_load_image_file( RoadMapImage image );
static RoadMapImage roadmap_canvas_load_png ( const char *full_name, RoadMapImage update_image );
static RoadMapImage roadmap_canvas_load_bmp ( const char *full_name, RoadMapImage update_image );



BOOL roadmap_canvas_is_ready( void )
{
   return ( RoadMapCanvasSavedState == CANVAS_GL_STATE_NOT_READY );
}

INLINE_DEC BOOL is_canvas_ready_src_line( int line )
{
	/*
	 * If canvas not ready calling OpenGL functions introduces distortions in the subsequent buffer swaps
	 * in android
	 */
   if ( RoadMapCanvasSavedState == CANVAS_GL_STATE_NOT_READY )
   {
	   roadmap_log( ROADMAP_MESSAGE_WARNING, __FILE__, line, "Canvas module is not ready!!! Can not call OGL functions!" );
	   return FALSE;
   }
   return TRUE;
}

#define is_canvas_ready() 	\
			is_canvas_ready_src_line( __LINE__ )

static void roadmap_canvas_color_fix()
{
#ifdef ANDROID
	if ( roadmap_main_get_build_sdk_version() <= ANDROID_OS_VER_DONUT )
	{
		roadmap_canvas_select_pen( CurrentPen );
	}
#endif //ANDROID
}

#define set_state( new_state, texture ) \
set_state_src_line ( new_state, texture, __LINE__ )

static BOOL set_state_src_line (int new_state, GLuint texture, int line )
{
   static BOOL    tex_enabled = FALSE;
   int            i;
   
   /*
    * If canvas not ready calling OpenGL functions introduces distortions in the subsequent buffer swaps
    * in android
    */
   if ( !is_canvas_ready_src_line( line ) )
   {
	   return FALSE;
   }
   /*
    * Same state nothing to do ...
    */
   if ( RoadMapCanvasSavedState == new_state &&
       (RoadMapCanvasSavedState != CANVAS_GL_STATE_IMAGE ||
        tex_units[current_tex_unit] == texture))
   {
	   return TRUE;
   }

   switch ( new_state ) {
      case CANVAS_GL_STATE_FORCE_GEOMETRY:
      {
         glDisable(GL_TEXTURE_2D);
         tex_enabled = FALSE;
         glDisableClientState(GL_TEXTURE_COORD_ARRAY);
         break;
      }
      case CANVAS_GL_STATE_GEOMETRY:
      {
         if ( tex_enabled )
         {
            glDisableClientState( GL_TEXTURE_COORD_ARRAY );
            glDisable( GL_TEXTURE_2D );
    	      tex_enabled = FALSE;
         }
         break;
      }
      case CANVAS_GL_STATE_IMAGE:
      {
         if (tex_units[current_tex_unit] != texture) {
            //roadmap_log (ROADMAP_ERROR, "Not same texture, replacing !");
            if (tex_enabled)
            {
               glDisable(GL_TEXTURE_2D);
               glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            }
            for (i = 0; i < num_tex_units; i++)
            {
               if (tex_units[i] == texture)
               {
                  current_tex_unit = i;
                  //roadmap_log (ROADMAP_ERROR, "Reusing !");
                  break;
               }
            }
            if (i == num_tex_units)
               current_tex_unit = (current_tex_unit + 1) % num_tex_units;
            
            glActiveTexture(GL_TEXTURE0 + current_tex_unit);
            glClientActiveTexture(GL_TEXTURE0 + current_tex_unit);
            
            glEnable(GL_TEXTURE_2D);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            tex_enabled = TRUE;
            
            if (tex_units[current_tex_unit] != texture) {
               tex_units[current_tex_unit] = texture;
               glBindTexture(GL_TEXTURE_2D, texture);
            }
               
         }
         else if ( !tex_enabled )
         {
            glEnableClientState( GL_TEXTURE_COORD_ARRAY );
    	      glEnable(GL_TEXTURE_2D);
    	      tex_enabled = TRUE;
         }
         /*
         else
         {
            roadmap_log (ROADMAP_ERROR, "Same texture, doing nothing !");
         }
          */

         break;
      }
      default:
         break;
   }
   RoadMapCanvasSavedState = new_state;
   
   return TRUE;
}

void roadmap_canvas_bind (int texture) {
   set_state(CANVAS_GL_STATE_IMAGE, texture);
}


/*
 * Adds the image to the unmanaged list of images
 * returns index of the image in the list, -1 if there is no available entry
 */
static inline int unmanaged_list_add( RoadMapImage image )
{
	int i;
	for ( i = 0; i < OGL_UNMANAGED_IMAGES_LIST_SIZE; ++i )
	{
		if ( RMUnmanagedImages[i] == NULL )
		{
			break;
		}
	}

	if ( i == OGL_UNMANAGED_IMAGES_LIST_SIZE )
	{
		roadmap_log( ROADMAP_ERROR, "There are no available entries in the unmanaged images list. List size: %d", i );
		return -1;
	}

	RMUnmanagedImages[i] = image;
	image->unmanaged_id = i;

	roadmap_log( ROADMAP_DEBUG, "Image %s, Address: 0x%x is successfully added to the unmanaged list at slot: %d", SAFE_STR( image->full_path ), image, i );

	return i;
}

static inline void unmanaged_list_remove( RoadMapImage image )
{
	int index = image->unmanaged_id;
	if ( ( index >= 0 ) && ( index < OGL_UNMANAGED_IMAGES_LIST_SIZE ) )
	{
		RMUnmanagedImages[index] = NULL;
		image->unmanaged_id = OGL_IMAGE_UNMANAGED_ID_UNKNOWN;
	}
	roadmap_log( ROADMAP_DEBUG, "Image %s, Address: 0x%x is removed from the unmanaged list. Slot: %d", SAFE_STR( image->full_path ), image, index );
}

/*
 * Passes through the unmanaged list of images. Deletes the textures
 * and invalidates the images for the non empty entries.
 * Return count of the invalidated images
 */
static inline int unmanaged_list_invalidate()
{
	int i;
	int count = 0;
	for ( i = 0; i < OGL_UNMANAGED_IMAGES_LIST_SIZE; ++i )
	{
		if ( RMUnmanagedImages[i] != NULL )
		{
			roadmap_canvas_image_invalidate( RMUnmanagedImages[i] );
			count++;
		}
	}

	roadmap_log( ROADMAP_INFO, "Invalidated %d images from the unmanaged list", count );

	return count;
}

void roadmap_canvas_unmanaged_list_add( RoadMapImage image )
{
	unmanaged_list_add( image );
}



static void roadmap_canvas_convert_points (GLfloat *glpoints,
                                           RoadMapGuiPoint *points,
                                           int count) {

   RoadMapGuiPoint *end = points + count;

   while (points < end) {
      *glpoints++ = (GLfloat)(points->x);
      *glpoints++ = (GLfloat)(points->y);
      *glpoints++ = Z_LEVEL;

      points++;
   }
}

//////////////////////////////
// triangulation code
static double area (RoadMapGuiPoint *points,
             int count) {
   double A=0.0f;
   int p, q;

   for(p=count-1,q=0; q<count; p=q++)
   {
      A+= points[p].x*points[q].y - points[q].x*points[p].y;
   }
   return A*0.5f;

}

static BOOL inside_triangle(float Ax, float Ay,
                                 float Bx, float By,
                                 float Cx, float Cy,
                                 float Px, float Py)

{
   float ax, ay, bx, by, cx, cy, apx, apy, bpx, bpy, cpx, cpy;
   float cCROSSap, bCROSScp, aCROSSbp;

   ax = Cx - Bx;  ay = Cy - By;
   bx = Ax - Cx;  by = Ay - Cy;
   cx = Bx - Ax;  cy = By - Ay;
   apx= Px - Ax;  apy= Py - Ay;
   bpx= Px - Bx;  bpy= Py - By;
   cpx= Px - Cx;  cpy= Py - Cy;

   aCROSSbp = ax*bpy - ay*bpx;
   cCROSSap = cx*apy - cy*apx;
   bCROSScp = bx*cpy - by*cpx;

   return ((aCROSSbp >= 0.0f) && (bCROSScp >= 0.0f) && (cCROSSap >= 0.0f));
}

//static const float EPSILON=0.0000000001f;
static const float EPSILON=0.0f;

static BOOL snip(RoadMapGuiPoint *points,int u,int v,int w,int n,int *V)
{
   int p;
   float Ax, Ay, Bx, By, Cx, Cy, Px, Py;

   Ax = points[V[u]].x;
   Ay = points[V[u]].y;

   Bx = points[V[v]].x;
   By = points[V[v]].y;

   Cx = points[V[w]].x;
   Cy = points[V[w]].y;

   if ( EPSILON > (((Bx-Ax)*(Cy-Ay)) - ((By-Ay)*(Cx-Ax))) ) return FALSE;

   for (p=0;p<n;p++)
   {
      if( (p == u) || (p == v) || (p == w) ) continue;
      Px = points[V[p]].x;
      Py = points[V[p]].y;
      if (inside_triangle(Ax,Ay,Bx,By,Cx,Cy,Px,Py)) return FALSE;
   }

   return TRUE;
}

static int roadmap_canvas_convert_polygons (GLfloat *glpoints,
                                             RoadMapGuiPoint *points,
                                             int num) {
   int *V = malloc(sizeof(int)*num);
   int v;
   int n = num;
   int nv;
   int count;
   int m, u, w;
   int i, j, k;


   //Remove duplicate points
   i = 0;
   while (i < n -1) {
      j = i + 1;
      while (j < n) {
         if (points[i].x == points[j].x && points[i].y == points[j].y) {
            for (k = j + 1; k < n; k++) {
               points[k - 1] = points[k];
            }
            n--;
         }
         j++;
      }
      i++;
   }

   //Remove point if in strsight line
   for (i = 0; i < n - 2; i++) {
      if ((points[i].x == points[i+1].x && points[i+1].x == points[i+2].x) ||
          (points[i].y == points[i+1].y && points[i+1].y == points[i+2].y)) {
         for (k = i + 2; k < n; k++) {
            points[k - 1] = points[k];
         }
         n--;
      }
   }

   if (n < 3) {
      free (V);
      return 0;
   }

   nv = n;
   count = 2*nv;


   if ( 0.0f < area(points, n) )
      for (v=0; v<n; v++) V[v] = v;
   else
      for (v=0; v<n; v++) V[v] = (n-1)-v;

   for (m=0, v=nv-1; nv>2; )
   {
      /* if we loop, it is probably a non-simple polygon */
      if (0 >= (count--))
      {
         //** Triangulate: ERROR - probable bad polygon!
         free (V);
         return 0;
      }

      /* three consecutive vertices in current polygon, <u,v,w> */
      u = v  ; if (nv <= u) u = 0;     /* previous */
      v = u+1; if (nv <= v) v = 0;     /* new v    */
      w = v+1; if (nv <= w) w = 0;     /* next     */

      if ( snip(points,u,v,w,nv,V) )
      {
         int a,b,c,s,t;

         /* true names of the vertices */
         a = V[u]; b = V[v]; c = V[w];

         /* output Triangle */
         *glpoints++ = (GLfloat)(points[a].x);
         *glpoints++ = (GLfloat)(points[a].y);
         *glpoints++ = Z_LEVEL;
         *glpoints++ = (GLfloat)(points[b].x);
         *glpoints++ = (GLfloat)(points[b].y);
         *glpoints++ = Z_LEVEL;
         *glpoints++ = (GLfloat)(points[c].x);
         *glpoints++ = (GLfloat)(points[c].y);
         *glpoints++ = Z_LEVEL;

         m++;

         /* remove v from remaining polygon */
         for(s=v,t=v+1;t<nv;s++,t++) V[s] = V[t]; nv--;

         /* resest error detection counter */
         count = 2*nv;
      }
   }
   free (V);
   return m*3;
}


static void set_fast_draw (int fast_draw) {
    if (fast_draw) {
//        glDisable(GL_LINE_SMOOTH);
   }
}

static void end_fast_draw (int fast_draw) {
    if (fast_draw) {
//       glEnable(GL_LINE_SMOOTH);
   }
}

void roadmap_canvas_get_formated_text_extents (const char *text, int s, int *width,int *ascent, int *descent, int *can_tilt, int font_type){

   int size;
	int bold;
   const wchar_t* p;
   float x = 0;
   RoadMapFontImage *fontImage;
   wchar_t wstr[255];

   *width = 0;
   *ascent = 0;
   *descent = 0;

   if ( !is_canvas_ready() )
	   return;

   if (can_tilt)
      *can_tilt = 1;

   if (text == NULL || *text == 0) {
       return;
   }

	if (font_type & FONT_TYPE_BOLD)
		bold = TRUE;
	else
		bold = FALSE;

   int length = roadmap_canvas_ogl_to_wchar (text, wstr, 255);
   if (length <=0)  {
      return;
   }

   if (s == -1)
   {
      s = FONT_DEFAULT_SIZE;
   }

   size = floor( s * RoadMapFontFactor );

	roadmap_canvas_font_metrics (size, ascent, descent, bold);

   p = wstr;

   while(*p) {
		fontImage =  roadmap_canvas_font_tex(*p, size, bold);
         // increment pen position
      x += fontImage->advance_x;
      ++p;
   }

   *width = ceilf(x);
}

void roadmap_canvas_get_text_extents
        (const char *text, int size, int *width,
            int *ascent, int *descent, int *can_tilt)
{
   roadmap_canvas_get_formated_text_extents(text, size, width, ascent, descent, can_tilt, FONT_TYPE_BOLD);
}

RoadMapPen roadmap_canvas_select_pen (RoadMapPen pen) {
   RoadMapPen old_pen = CurrentPen;
   CurrentPen = pen;

   if ( RoadMapCanvasSavedState == CANVAS_GL_STATE_NOT_READY )
   {
	   roadmap_log( ROADMAP_INFO, "Canvas not ready - cannot call select pen" );
	   return old_pen;
   }

   glColor4f(pen->stroke.r, pen->stroke.g, pen->stroke.b, pen->stroke.a);

   check_gl_error();

   return old_pen;
}

static void select_background_color (RoadMapPen pen) {

   if ( RoadMapCanvasSavedState == CANVAS_GL_STATE_NOT_READY )
   {
	   roadmap_log( ROADMAP_INFO, "Canvas not ready - cannot call select background" );
	   return;
   }
   
   glColor4f(pen->background.r, pen->background.g, pen->background.b, pen->background.a);
   
   check_gl_error();
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
      //pen->style = 1; //solid
      pen->stroke.r = 0.0;
      pen->stroke.g = 0.0;
      pen->stroke.b = 0.0;
      pen->stroke.a = 1.0;
      pen->background.r = 0.0;
      pen->background.g = 0.0;
      pen->background.b = 0.0;
      pen->background.a = 1.0;
      pen->lineWidth = 1.0;
      pen->next = RoadMapPenList;

      RoadMapPenList = pen;
   }

   roadmap_canvas_select_pen (pen);

   return pen;
}

static void set_color (const char *color, BOOL foreground) {
   
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
   
   if (foreground) {
      CurrentPen->stroke.r = r;
      CurrentPen->stroke.g = g;
      CurrentPen->stroke.b = b;
      CurrentPen->stroke.a = a;
   } else {
      CurrentPen->background.r = r;
      CurrentPen->background.g = g;
      CurrentPen->background.b = b;
      CurrentPen->background.a = a;
   }
   
	roadmap_canvas_select_pen(CurrentPen);
}

void roadmap_canvas_set_foreground (const char *color) {

   set_color(color, TRUE);
}

void roadmap_canvas_set_background (const char *color) {
   
   set_color(color, FALSE);
}

void roadmap_canvas_set_linestyle (const char *style) {
   /*
   if (strcasecmp (style, "dashed") == 0) {
       CurrentPen->style = 2;
   } else {
       CurrentPen->style = 1;
   }
    */
}

void roadmap_canvas_set_thickness ( int thickness ) {

   if (thickness < 0) {
      thickness = 0;
      roadmap_log (ROADMAP_WARNING, "Thickness set to 0 !");
   }

   if ( thickness < RoadMapMinThickness )
   {
      CurrentPen->lineWidth = RoadMapMinThickness * 1.0;
   }
   else
   {
	   CurrentPen->lineWidth = thickness * 1.0;
   }
}


void roadmap_canvas_erase (void)
{
   /* 'erase' means fill the canvas with the foreground color */
   if ( !set_state(CANVAS_GL_STATE_FORCE_GEOMETRY, 0) )
	   return;

   glClearColor(CurrentPen->stroke.r, CurrentPen->stroke.g, CurrentPen->stroke.b, CurrentPen->stroke.a);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}

void roadmap_canvas_erase_area (const RoadMapGuiRect *rect)
{

	if ( !set_state(CANVAS_GL_STATE_GEOMETRY, 0) )
		return;

   GLfloat glpoints[4*3] = {
      rect->minx * 1.0f, rect->miny * 1.0f, Z_LEVEL,
      (rect->maxx +1) * 1.0f, rect->miny * 1.0f, Z_LEVEL,
      (rect->maxx +1) * 1.0f, (rect->maxy +1) * 1.0f, Z_LEVEL,
      rect->minx * 1.0f, (rect->maxy +1) * 1.0f, Z_LEVEL
   };

   glVertexPointer(3, GL_FLOAT, 0, glpoints);
   glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
   check_gl_error();

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

   case ROADMAP_CANVAS_BOTTOMMIDDLE:
      y = position->y - text_height;
      x = position->x - (text_width / 2);
      break;

   case ROADMAP_CANVAS_CENTERMIDDLE:
      y = position->y - (text_ascent / 2) - text_descent;
      x = position->x - (text_width / 2);
      break;

   case ROADMAP_CANVAS_CENTERRIGHT:
      y = position->y - (text_ascent / 2) - text_descent;
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


void roadmap_canvas_draw_string  (RoadMapGuiPoint *position,
                                  int corner,
                                  const char *text) {

   roadmap_canvas_draw_string_size (position, corner, -1, text);
}

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

   if ( len > 1000000 )
   {
	   roadmap_log( ROADMAP_FATAL, "Allocation FRIBIDI  %d exception", len );
   }

   visual = (FriBidiChar *) malloc (sizeof (FriBidiChar) * (len + 1));
   ltov = NULL;
   vtol = NULL;
   levels = NULL;

   /* Create a bidi string. */
   log2vis = fribidi_log2vis ((FriBidiChar *)logical, len, &base,
                              /* output */
                              visual, ltov, vtol, levels);

   if (!log2vis) {
      roadmap_log (ROADMAP_WARNING, "log2vis failed to convert string");
      return NULL;
   }

   new_len = len;

   return (wchar_t *)visual;

}
#endif

void roadmap_canvas_draw_formated_string_angle (const RoadMapGuiPoint *position,
                                       RoadMapGuiPoint *center,
                                                int angle, int s, int font_type,
                                                const char *text){
   int size;
   wchar_t wstr[255];
   int length = roadmap_canvas_ogl_to_wchar (text, wstr, 255);

   if (length <=0) return;
   
   if ( !is_canvas_ready() )
	   return;

#ifdef USE_FRIBIDI
   wchar_t *bidi_text = bidi_string(wstr);
   const wchar_t* p = bidi_text;
#else
   const wchar_t* p = wstr;
#endif

   double x  = position->x;
   double y  = position->y;
   float width;
   float height;
   float x_offset, y_offset;

   if (s <= 0)
      s = FONT_DEFAULT_SIZE;

   size = floor(s * RoadMapFontFactor);


   RoadMapFontImage *fontImage;
   RoadMapImage image, outline_image;

	int bold;

	if (font_type & FONT_TYPE_BOLD)
		bold = TRUE;
	else
		bold = FALSE;

   
   glPushMatrix();
   glTranslatef(x, y, 0);
   glRotatef(angle, 0, 0, 1);
   check_gl_error();
   x = 0;
   y = 0;

   while(*p) {
      //roadmap_log (ROADMAP_INFO, "--->>> drawing character: %ld", *p);
		fontImage =  roadmap_canvas_font_tex(*p, size, bold);
      image = fontImage->image;
      outline_image = fontImage->outline_image;

      if (image) {
         if ((font_type & FONT_TYPE_OUTLINE) &&
             outline_image) {
            
            width = outline_image->width;
            height = outline_image->height;
            x_offset = outline_image->offset.x;
            y_offset = outline_image->offset.y;
            
            GLfloat glpoints[] = {
               x -0.5, y -0.5, Z_LEVEL,
               x + width +0.5, y -0.5, Z_LEVEL,
               x -0.5, y + height +0.5, Z_LEVEL,
               x + width +0.5, y + height +0.5, Z_LEVEL,
            };
            
            GLfloat shift = -1.0f/(2.0f* CANVAS_ATLAS_TEX_SIZE);
            
            GLfloat texCoords[] = {
               (GLfloat)x_offset / (GLfloat)CANVAS_ATLAS_TEX_SIZE + shift, (GLfloat)y_offset / (GLfloat)CANVAS_ATLAS_TEX_SIZE + shift,
               (GLfloat)(x_offset + width) / (GLfloat)CANVAS_ATLAS_TEX_SIZE - shift, (GLfloat)y_offset / (GLfloat)CANVAS_ATLAS_TEX_SIZE + shift,
               (GLfloat)x_offset / (GLfloat)CANVAS_ATLAS_TEX_SIZE + shift, (GLfloat)(y_offset + height) / (GLfloat)CANVAS_ATLAS_TEX_SIZE - shift,
               (GLfloat)(x_offset + width) / (GLfloat)CANVAS_ATLAS_TEX_SIZE - shift, (GLfloat)(y_offset + height) / (GLfloat)CANVAS_ATLAS_TEX_SIZE - shift,
            };
            
            select_background_color(CurrentPen);
            
            glPushMatrix();
            glTranslatef(fontImage->left - (outline_image->width - image->width)*0.5f, 0, 0);
            glTranslatef(0, -fontImage->top - (outline_image->height - image->height)*0.5f, 0);
            
            set_state(CANVAS_GL_STATE_IMAGE, outline_image->texture);
            
            glVertexPointer(3, GL_FLOAT, 0, glpoints);
            glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            check_gl_error();
            
            
            glPopMatrix();
            
            roadmap_canvas_select_pen(CurrentPen);
         }
         
         width = image->width;
         height = image->height;
         x_offset = image->offset.x;
         y_offset = image->offset.y;

         GLfloat glpoints[] = {
            x -0.5, y -0.5, Z_LEVEL,
            x + width +0.5, y -0.5, Z_LEVEL,
            x -0.5, y + height +0.5, Z_LEVEL,
            x + width +0.5, y + height +0.5, Z_LEVEL,
         };
         
         GLfloat shift = -1.0f/(2.0f* CANVAS_ATLAS_TEX_SIZE);

         GLfloat texCoords[] = {
            (GLfloat)x_offset / (GLfloat)CANVAS_ATLAS_TEX_SIZE + shift, (GLfloat)y_offset / (GLfloat)CANVAS_ATLAS_TEX_SIZE + shift,
            (GLfloat)(x_offset + width) / (GLfloat)CANVAS_ATLAS_TEX_SIZE - shift, (GLfloat)y_offset / (GLfloat)CANVAS_ATLAS_TEX_SIZE + shift,
            (GLfloat)x_offset / (GLfloat)CANVAS_ATLAS_TEX_SIZE + shift, (GLfloat)(y_offset + height) / (GLfloat)CANVAS_ATLAS_TEX_SIZE - shift,
            (GLfloat)(x_offset + width) / (GLfloat)CANVAS_ATLAS_TEX_SIZE - shift, (GLfloat)(y_offset + height) / (GLfloat)CANVAS_ATLAS_TEX_SIZE - shift,
         };

         glPushMatrix();
         glTranslatef(fontImage->left, 0, 0);
         glTranslatef(0, -fontImage->top, 0);

         set_state(CANVAS_GL_STATE_IMAGE, image->texture);

         glVertexPointer(3, GL_FLOAT, 0, glpoints);
         glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
         roadmap_canvas_color_fix();	/* Android only. Empty for others */
         glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
         check_gl_error();


         glPopMatrix();

         // increment pen position
         x += fontImage->advance_x;
      } else {
         if (*p != 32)
            roadmap_log (ROADMAP_DEBUG, "Invalid texture when loading character: %ld", *p);
         x += fontImage->advance_x;
      }
      ++p;
   }

   glPopMatrix();
   check_gl_error();
#ifdef USE_FRIBIDI
   free(bidi_text);
#endif
}

void roadmap_canvas_draw_string_angle (const RoadMapGuiPoint *position,
                                       RoadMapGuiPoint *center,
                                       int angle, int s,
                                       const char *text)
{
  	roadmap_canvas_draw_formated_string_angle(position, center, angle, s, FONT_TYPE_BOLD, text);

}


void roadmap_canvas_draw_multiple_points (int count, RoadMapGuiPoint *points) {
   GLfloat glpoints[512*3];
//roadmap_log (ROADMAP_INFO, "\n\nroadmap_canvas_draw_multiple_points");

   if ( !set_state( CANVAS_GL_STATE_GEOMETRY, 0) )
   		return;

   while (count > 512) {
      roadmap_canvas_convert_points (glpoints, points, 512);
      glVertexPointer(3, GL_FLOAT, 0, glpoints);
      glDrawArrays(GL_POINTS, 0, 512);
      check_gl_error();
      points += 512;
      count -= 512;
   }
   roadmap_canvas_convert_points (glpoints, points, count);
   glVertexPointer(3, GL_FLOAT, 0, glpoints);
   glDrawArrays(GL_POINTS, 0, count);
   check_gl_error();

}

void roadmap_canvas_draw_multiple_lines (int count, int *lines, RoadMapGuiPoint *points, int fast_draw) {

#ifdef ANDROID /* Temporary workaround for the colors distortions problem in android <= 1.6    *** AGA *** */
   if ( roadmap_main_get_build_sdk_version() <= ANDROID_OS_VER_DONUT )
   {
      roadmap_canvas_draw_multiple_lines_slow( count, lines, points, fast_draw );
      return;
   }
#endif
   
   roadmap_gl_vertex glpoints[8];
   roadmap_gl_vertex *gl_array = Glpoints;
   int phf = MAX_AA_SIZE;
   float r = CurrentPen->lineWidth;
   float bord = phf / r;
   int count_of_points;
   RoadMapGuiPoint *prev;
   int i;
   float x, y, x1, y1;
   float perp_y, perp_x, perpd, factor;
   float width; 			// = (maxr+1.0f)*0.5f;
   float unit_x, unit_y;
   float parl_x, parl_y;
   
   int pts_count = 0;
   
   if ( !is_canvas_ready() )
      return;
   
   generate_aa_tex();
   
   set_state(CANVAS_GL_STATE_IMAGE, RoadMapAATex);
   
   glpoints[0].z = glpoints[1].z =
   glpoints[2].z = glpoints[3].z =
   glpoints[4].z = glpoints[5].z =
   glpoints[6].z = glpoints[7].z = Z_LEVEL;
   
   glpoints[3].tx = glpoints[1].tx = glpoints[5].tx = glpoints[7].tx =
   glpoints[0].ty = glpoints[1].ty = glpoints[6].ty = glpoints[7].ty = (float)(psz-bord)/(float)pdb;
   glpoints[2].tx = glpoints[0].tx = glpoints[4].tx = glpoints[6].tx = (float)(pdb+bord)/(float)pdb;
   glpoints[3].ty = glpoints[2].ty = (float)(pct)/(float)pdb;
   glpoints[5].ty = glpoints[4].ty = (float)(pct + 0.0001f)/(float)pdb;
   
   for (i = 0; i < count; ++i) {
      
      count_of_points = *lines;
      prev = points;
      points++;
      count_of_points--;
      
      while (count_of_points) {
         x1 = (float)prev->x;
         y1 = (float)prev->y;
         x = (float)points->x;
         y = (float)points->y;
         /*
         
         ///// test points
         int phf = MAX_AA_SIZE;
         float point_border = 2;
         GLfloat points_test[] = {
            x-2,      y-2, Z_LEVEL +1,
            x+2, y-2, Z_LEVEL +1,
            x+2, y+2, Z_LEVEL +1,
            x-2,      y+2, Z_LEVEL +1
         };
   
         GLfloat tex_test[] = {
            (psz-point_border)/pdb, (psz-point_border)/pdb,
            (pdb+point_border)/pdb, (psz-point_border)/pdb,
            (pdb+point_border)/pdb, (pdb+point_border)/pdb,
            (psz-point_border)/pdb, (pdb+point_border)/pdb
         };
          
         glVertexPointer(3, GL_FLOAT, 0, points_test);
         glTexCoordPointer(2, GL_FLOAT, 0, tex_test);
         glColor4f(1.0f, 0, 1.0f, 1.0f);
         glDrawArrays(GL_TRIANGLE_FAN, 0, 3);
         check_gl_error();
         roadmap_canvas_select_pen(CurrentPen);
         ///// test points - end
         */
         
         perp_y = x1-x;
         perp_x = y-y1;
         factor = perp_y*perp_y+perp_x*perp_x;
         
         if ((perp_x < 2 && perp_x > -2) &&
             (perp_y < 2 && perp_y > -2)) {
            //prev = points;
            //points++;
            //count_of_points--;
            //continue;
                        
            /*
            //printf("draw mult lines - small line: %f, %f\n", perp_x, perp_y);
            test_avi_small++;
            if (perp_x == 0 && perp_y == 0)
               test_avi_zero++;
             */
         }
         
         
         if (factor) {
            perpd = frsqrtes_nr(factor);
            perp_y *= perpd;				// normalize to 1px
            perp_x *= perpd;
            x1 -= perp_y*0.5f;
            y1 += perp_x*0.5f;
         } else {
            perp_y = 0.0f;
            perp_x = 1.0f;
         }
         
         width = (r+1.0f)*0.5f;
         unit_x = -perp_y;
         unit_y = perp_x;
         perp_y *= width;
         perp_x *= width;
         parl_x = -perp_y;
         parl_y =  perp_x;
         
         glpoints[0].x = x1-perp_x-parl_x;		glpoints[0].y = y1-perp_y-parl_y;
         glpoints[1].x = x1+perp_x-parl_x;		glpoints[1].y = y1+perp_y-parl_y;
         glpoints[2].x = x1-perp_x;			glpoints[2].y = y1-perp_y;
         glpoints[3].x = x1+perp_x;			glpoints[3].y = y1+perp_y;
         glpoints[4].x = x-perp_x;				glpoints[4].y = y-perp_y;
         glpoints[5].x = x+perp_x;				glpoints[5].y = y+perp_y;
         glpoints[6].x = x-perp_x+parl_x;		glpoints[6].y = y-perp_y+parl_y;
         glpoints[7].x = x+perp_x+parl_x;		glpoints[7].y = y+perp_y+parl_y;
         
         memcpy(gl_array, glpoints, sizeof(*glpoints) * 3);
         gl_array += 3;
         memcpy(gl_array, glpoints+1, sizeof(*glpoints) * 3);
         gl_array += 3;
         memcpy(gl_array, glpoints+2, sizeof(*glpoints) * 3);
         gl_array += 3;
         memcpy(gl_array, glpoints+3, sizeof(*glpoints) * 3);
         gl_array += 3;
         memcpy(gl_array, glpoints+4, sizeof(*glpoints) * 3);
         gl_array += 3;
         memcpy(gl_array, glpoints+5, sizeof(*glpoints) * 3);
         gl_array += 3;
         
         pts_count += 18;
         if ((pts_count + 18) >= (sizeof(Glpoints) / sizeof(Glpoints[0]))) {
            glVertexPointer(3, GL_FLOAT, sizeof(roadmap_gl_vertex), &Glpoints[0].x);
            glTexCoordPointer(2, GL_FLOAT, sizeof(roadmap_gl_vertex), &Glpoints[0].tx);
            
            roadmap_canvas_color_fix();	/* Android only. Empty for others */
            glDrawArrays(GL_TRIANGLES, 0, pts_count);
            check_gl_error();
            gl_array = Glpoints;
            pts_count = 0;
            roadmap_log(ROADMAP_DEBUG, "ogl roadmap_canvas_draw_multiple_lines - too many points for one pass");
         }
         prev = points;
         points++;
         count_of_points--;
      }
      lines++;
   }
   
   if (pts_count) {
      glVertexPointer(3, GL_FLOAT, sizeof(roadmap_gl_vertex), &Glpoints[0].x);
      glTexCoordPointer(2, GL_FLOAT, sizeof(roadmap_gl_vertex), &Glpoints[0].tx);
      
      roadmap_canvas_color_fix();	/* Android only. Empty for others */
      glDrawArrays(GL_TRIANGLES, 0, pts_count);
      check_gl_error();
   }
}


void roadmap_canvas_draw_multiple_tex_lines (int count, int *lines, RoadMapGuiPoint *points, int fast_draw,
                                             RoadMapImage image, int opposite) {   
   roadmap_gl_vertex glpoints[4];
   roadmap_gl_vertex *gl_array = Glpoints;
   float r = CurrentPen->lineWidth;
   int count_of_points;
   RoadMapGuiPoint *prev;
   int i;
   float x, y, x1, y1;
   float perp_y, perp_x, perpd, factor;
   float width;
   float size_factor;
   long distance;
   int pts_count = 0;
   
   if ( !is_canvas_ready() )
      return;
   
   
   set_state(CANVAS_GL_STATE_IMAGE, image->texture);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
   
   glpoints[0].z = glpoints[1].z =
   glpoints[2].z = glpoints[3].z = Z_LEVEL;
   
   if (!opposite) {
      glpoints[0].tx = (GLfloat)image->width / (GLfloat)next_pot(image->width);
      glpoints[1].tx = 0.0f;
      glpoints[2].tx = (GLfloat)image->width / (GLfloat)next_pot(image->width);
      glpoints[3].tx = 0.0f;
      glpoints[2].ty = 0.0f;
      glpoints[3].ty = 0.0f;
   } else {
      glpoints[0].tx = 0.0f;
      glpoints[1].tx = (GLfloat)image->width / (GLfloat)next_pot(image->width);
      glpoints[2].tx = 0.0f;
      glpoints[3].tx = (GLfloat)image->width / (GLfloat)next_pot(image->width);
      glpoints[0].ty = 0.0f;
      glpoints[1].ty = 0.0f;
   }
   
   for (i = 0; i < count; ++i) {
      
      count_of_points = *lines;
      prev = points;
      points++;
      count_of_points--;
      
      while (count_of_points) {
         x1 = (float)prev->x;
         y1 = (float)prev->y;
         x = (float)points->x;
         y = (float)points->y;
    
         perp_y = x1-x;
         perp_x = y-y1;
         factor = perp_y*perp_y+perp_x*perp_x;
         
         if (factor) {
            perpd = frsqrtes_nr(factor);
            perp_y *= perpd;				// normalize to 1px
            perp_x *= perpd;
            x1 -= perp_y*0.5f;
            y1 += perp_x*0.5f;
         } else {
            perp_y = 0.0f;
            perp_x = 1.0f;
         }
         
         width = r*0.5f;
         perp_y *= width;
         perp_x *= width;
         
         glpoints[0].x = x1-perp_x;    glpoints[0].y = y1-perp_y;
         glpoints[1].x = x1+perp_x;    glpoints[1].y = y1+perp_y;
         glpoints[2].x = x-perp_x;     glpoints[2].y = y-perp_y;
         glpoints[3].x = x+perp_x;     glpoints[3].y = y+perp_y;
         
         //Texture repeat:
         size_factor = r / image->width;
         distance = roadmap_math_screen_distance(prev, points, 0);
         if (distance < 1)
            distance = 1;
         float coord_y = (float)distance / (float)image->height  / size_factor;
         if (coord_y <= 0.00001f) {
            prev = points;
            points++;
            count_of_points--;
            continue;
         }
         coord_y = floorf(coord_y);
         if (coord_y < 1)
            coord_y = 1;
         
         if (!opposite) {
            glpoints[0].ty = glpoints[1].ty = coord_y;
         } else {
            glpoints[2].ty = glpoints[3].ty = coord_y;
         }
         
         memcpy(gl_array, glpoints, sizeof(*glpoints) * 3);
         gl_array += 3;
         memcpy(gl_array, glpoints+1, sizeof(*glpoints) * 3);
         gl_array += 3;
         
         pts_count += 6;
         if ((pts_count + 6) >= (sizeof(Glpoints) / sizeof(Glpoints[0]))) {
            glVertexPointer(3, GL_FLOAT, sizeof(roadmap_gl_vertex), &Glpoints[0].x);
            glTexCoordPointer(2, GL_FLOAT, sizeof(roadmap_gl_vertex), &Glpoints[0].tx);
            
            roadmap_canvas_color_fix();	/* Android only. Empty for others */
            glDrawArrays(GL_TRIANGLES, 0, pts_count);
            check_gl_error();
            gl_array = Glpoints;
            pts_count = 0;
            roadmap_log(ROADMAP_DEBUG, "ogl roadmap_canvas_draw_multiple_lines - too many points for one pass");
         }
         prev = points;
         points++;
         count_of_points--;
      }
      lines++;
   }
   
   if (pts_count) {
      glVertexPointer(3, GL_FLOAT, sizeof(roadmap_gl_vertex), &Glpoints[0].x);
      glTexCoordPointer(2, GL_FLOAT, sizeof(roadmap_gl_vertex), &Glpoints[0].tx);
      
      roadmap_canvas_color_fix();	/* Android only. Empty for others */
      glDrawArrays(GL_TRIANGLES, 0, pts_count);
      check_gl_error();
   }
}




void tcbBegin (GLenum type) {

   switch (type) {
      case GL_TRIANGLES:
         break;
      case GL_TRIANGLE_STRIP:
         break;
      case GL_TRIANGLE_FAN:
         break;
      default:
         roadmap_log (ROADMAP_WARNING, "tcbBegin() - unsupported draw type: %d", type); 
         break;
   }
   
   GlpointsCount = 0;
   GlcombinedCount = 0;
   GlpointsType = type;
}

void tcbVertex (void *data) {
   
   GLdouble *coords = (GLdouble *)data;
   
   Glpoints[GlpointsCount].x = coords[0];
   Glpoints[GlpointsCount].y = coords[1];
   Glpoints[GlpointsCount].z = Z_LEVEL;
   
   GlpointsCount++;
}

void tcbEnd () {
   if (GlpointsCount > 0) {
      glVertexPointer(3, GL_FLOAT, sizeof(roadmap_gl_vertex), Glpoints);
      glDrawArrays(GlpointsType, 0, GlpointsCount);
      check_gl_error();
      
      GlpointsCount = 0;
      GlcombinedCount = 0;
      GlpointsType = 0;
   }
}

void tcbCombine (GLdouble c[3], void *d[4], GLfloat w[4], void **out) {
   
   GLcombined[GlcombinedCount].x = c[0];
   GLcombined[GlcombinedCount].y = c[1];
   GLcombined[GlcombinedCount].z = c[2];
   
   *out = GLcombined + GlcombinedCount;
   
   GlcombinedCount++;
}

void tcbError(GLenum errorCode)
{
   roadmap_log (ROADMAP_WARNING, "tcbError: %d", errorCode);
}


void roadmap_canvas_draw_multiple_polygons (int count, int *polygons, RoadMapGuiPoint *points, int filled,
                                            int fast_draw) {
   int i;
   int count_of_points;
   float saved_line_width;
   BOOL shouldAA;

   GLfloat glpoints[1024*3];
   GLdouble coords[1024*3];
   RoadMapGuiPoint first_point;
   RoadMapGuiPoint closed_loop_points[1025];
   int count_of_lines;
   
   if (!set_state(CANVAS_GL_STATE_GEOMETRY, 0))
      return;
   
   GLUtesselator *tess = gluNewTess();
   gluTessCallback (tess, GLU_TESS_BEGIN, tcbBegin);
   gluTessCallback (tess, GLU_TESS_VERTEX, tcbVertex);
   gluTessCallback (tess, GLU_TESS_END, tcbEnd);
   gluTessCallback (tess, GLU_TESS_COMBINE, tcbCombine);
   gluTessCallback (tess, GLU_TESS_ERROR, tcbError);
   
   set_fast_draw (fast_draw);
   
   
	for (i = 0; i < count; ++i) {
      
      count_of_points = *polygons;
      
      first_point = points[0];
      
      shouldAA = !fast_draw;
      if (count_of_points > 2 &&
          ((points[0].x == points[1].x ||
            points[1].x == points[2].x) &&
           (points[0].y == points[1].y ||
            points[1].y == points[2].y)))
         shouldAA = FALSE; //Try to identify vert/hor lines in polygon and skip anti aliasing
      
      while (count_of_points > 1024) {
         roadmap_canvas_convert_points (glpoints, points, 1024);
         glVertexPointer(3, GL_FLOAT, 0, glpoints);
         if (filled)
            glDrawArrays(GL_TRIANGLE_FAN, 0, 1024);
         else
            glDrawArrays(GL_LINE_STRIP, 0, 1024);
         check_gl_error();
         
         points += 1023;
         count_of_points -= 1023;
      }
      
      memcpy(closed_loop_points, points, sizeof(RoadMapGuiPoint) * (count_of_points +1));
      if (first_point.x != points[count_of_points -1].x ||
          first_point.y != points[count_of_points -1].y) {
         closed_loop_points[count_of_points] = first_point;
         count_of_lines = count_of_points + 1;
      } else {
         count_of_lines = count_of_points;
      }
      
      if (!filled || shouldAA) {
         saved_line_width = CurrentPen->lineWidth;
         CurrentPen->lineWidth = 2.0f;
         roadmap_canvas_draw_multiple_lines(1, &count_of_lines, closed_loop_points, fast_draw);
         set_state(CANVAS_GL_STATE_GEOMETRY, 0);
         CurrentPen->lineWidth = saved_line_width;
      }// else
      //roadmap_log(ROADMAP_DEBUG, "NO AA (pen: %s)", CurrentPen->name);
      
      if (filled) {
         int i;
         gluTessBeginPolygon (tess, NULL); 
         gluTessBeginContour (tess); 
         for ( i=0; i<count_of_points;i++) {
            coords[i*3 +0] = closed_loop_points[i].x;
            coords[i*3 +1] = closed_loop_points[i].y;
            coords[i*3 +2] = Z_LEVEL;

            gluTessVertex (tess, &coords[i*3], &coords[i*3]); 
         }
         //gluTessEndContour (tess); 
         gluEndPolygon (tess);
      }
      
      
      
      points += count_of_points;
      polygons += 1;
   }
   
   gluDeleteTess(tess);
   
   end_fast_draw (fast_draw);
   
}

void roadmap_canvas_draw_multiple_circles (int count, RoadMapGuiPoint *centers, int *radius,
									int filled, int fast_draw) {
//roadmap_log (ROADMAP_INFO, "\n\nroadmap_canvas_draw_multiple_circles");
   int phf = MAX_AA_SIZE;
   int i;
   int r;
   float point_border;
   float size;
   float centx;
   float centy;


   generate_aa_tex();

   if ( !is_canvas_ready() )
   		return;

   set_fast_draw (fast_draw);

   if (filled)
      set_state(CANVAS_GL_STATE_IMAGE, RoadMapAATex);
   else
      set_state(CANVAS_GL_STATE_IMAGE, RoadMapAATexNf);

   for (i = 0; i < count; ++i) {
      r = radius[i] * 2;

      point_border = MAX_AA_SIZE / r;

      size = r+1.0f;
      centx = centers[i].x- size*0.5f;
      centy = centers[i].y- size*0.5f;

      GLfloat glpoints[] = {
         centx,      centy, Z_LEVEL,
         centx+size, centy, Z_LEVEL,
         centx+size, centy+size, Z_LEVEL,
         centx,      centy+size, Z_LEVEL
      };

      GLfloat texCoords[] = {
         (psz-point_border)/pdb, (psz-point_border)/pdb,
         (pdb+point_border)/pdb, (psz-point_border)/pdb,
         (pdb+point_border)/pdb, (pdb+point_border)/pdb,
         (psz-point_border)/pdb, (pdb+point_border)/pdb
      };

      glVertexPointer(3, GL_FLOAT, 0, glpoints);
      glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
      glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

      check_gl_error();
   }

   set_state(CANVAS_GL_STATE_GEOMETRY, 0);

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






void roadmap_canvas_save_screenshot (const char* filename) {

}

void roadmap_canvas_set_opacity (int opacity)
{
   //roadmap_log (ROADMAP_INFO, "\n\nroadmap_canvas_set_opacity");
	if (!CurrentPen) return;

	if ((opacity <= 0) || (opacity >= 255)) {
      opacity = 255;
	}

	CurrentPen->stroke.a  = opacity/255.0;
	roadmap_canvas_select_pen(CurrentPen);
}


/*
 * BMP loader. Not available
 */
static RoadMapImage roadmap_canvas_load_bmp ( const char *full_name, RoadMapImage update_image )
{
	roadmap_log( ROADMAP_ERROR, "Cannot load bmp image %s", full_name );
	return NULL;
}


/*
* PNG loader. If update image not null update its buffer otherwise create new image
*/
static RoadMapImage roadmap_canvas_load_png ( const char *full_name, RoadMapImage update_image )
{

  int width;
  int height;
  int img_w;
  int img_h;
  int stride;
  int i,j;
  unsigned char *buf;
  RoadMapImage image;


  buf = read_png_file(full_name, &img_w, &img_h, &stride);

  roadmap_log( ROADMAP_INFO, "Loading file : %s\n", full_name );

  if (!buf) return NULL;

  roadmap_log( ROADMAP_INFO, "Success loading image : %s\n", full_name );

  if ( update_image )
  {
	   image = update_image;
  }
  else
  {
	   //create texture
	   image = malloc(sizeof(roadmap_canvas_image));
  }

  image->width = img_w;
  image->height = img_h;
   
   
   if (image->width <= CANVAS_ATLAS_TEX_SIZE &&
       image->height <= CANVAS_ATLAS_TEX_SIZE) {
      image->buf = buf;
   } else {
      width = next_pot( img_w );
      height = next_pot( img_h );
      
      if ( width > 1025 || height > 1025 )
      {
         roadmap_log( ROADMAP_FATAL, "Allocation %d, %d, %d, %d exception: %s", img_w, img_h, width, height, full_name );
      }
      GLubyte* temp_buf= malloc(4 * width * height);
      
      for( j = 0; j < height; j++) {
         for( i = 0; i < width; i++){
            temp_buf[4*(i+j*width)]     = (i >= img_w || j >= img_h) ? 0 : (int)buf[4*(i+j*img_w)   ];
            temp_buf[4*(i+j*width) +1]  = (i >= img_w || j >= img_h) ? 0 : (int)buf[4*(i+j*img_w) +1];
            temp_buf[4*(i+j*width) +2]  = (i >= img_w || j >= img_h) ? 0 : (int)buf[4*(i+j*img_w) +2];
            temp_buf[4*(i+j*width) +3]  = (i >= img_w || j >= img_h) ? 0 : buf[4*(i+j*img_w) +3];
         }
      }
      
      image->buf = temp_buf;
      
      free (buf);
      image->offset.x = 0;
      image->offset.y = 0;
   }

  return image;
}

RoadMapImage roadmap_canvas_new_image (int width, int height) {

   GLuint uWidth;
   GLuint uHeight;
   RoadMapImage image;

   char temp[512];

   image = malloc(sizeof(roadmap_canvas_image));
   roadmap_check_allocated (image);

   image->width = width;
   image->height = height;
   image->restore_cb = roadmap_canvas_set_image_texture;
   image->full_path = strdup( CANVAS_IMAGE_NEW_TYPE );
   image->is_valid = 0;
   image->frame_buf = CANVAS_INVALID_TEXTURE_ID;

   unmanaged_list_add( image );

   uWidth = next_pot(image->width);
   uHeight = next_pot(image->height);

   sprintf( temp, "roadmap_canvas_new_image. (%d, %d), (%d,%d)", width, height, uWidth, uHeight );

   image->buf = malloc( uHeight * uWidth * 4 );
   roadmap_check_allocated (image->buf);
   memset(image->buf, 0, ( uHeight * uWidth * 4 ));

   if ( is_canvas_ready() )
   {
	   roadmap_canvas_set_image_texture( image );
	   check_gl_error();
   }

   roadmap_log(ROADMAP_INFO, "Created ID: %d usize: %d, %d size: %d, %d", image->texture, uWidth, uHeight, image->width, image->height);

   return image;
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

RoadMapImage roadmap_canvas_load_image (const char *path,
                                        const char* file_name)
{
   //roadmap_log (ROADMAP_INFO, "\n\nroadmap_canvas_load_image");

   char *full_name = roadmap_path_join( path, file_name );

   RoadMapImage image = malloc( sizeof(roadmap_canvas_image) );;

   image->full_path = full_name;
   image->is_valid = 0;
   image->update_time = 0;
   image->unmanaged_id = OGL_IMAGE_UNMANAGED_ID_UNKNOWN;
   image->restore_cb = roadmap_canvas_load_image_file;

   //create texture
   if ( !roadmap_canvas_load_image_file( image ) )
   {
	  roadmap_log( ROADMAP_INFO, "Failed loading image %s", image->full_path );
      if (image->full_path)
         free (image->full_path);

	   free( image );
	   return NULL;
   }
   else
   {
	   roadmap_log( ROADMAP_INFO, "Image file %s is loaded. Address: 0x%x", SAFE_STR( image->full_path ), image );
   }

   return image;
}

static BOOL roadmap_canvas_load_image_file( RoadMapImage image )
{
   if ( !image )
   {
		roadmap_log( ROADMAP_WARNING, "Cannot load image buffer: reference not valid!" );
		return FALSE;
   }

   image->buf = NULL;

   if ( check_file_suffix( image->full_path, ".bmp" ) )
   {
	  roadmap_canvas_load_bmp( image->full_path, image );
   }
   else
   {
	  roadmap_canvas_load_png( image->full_path, image );
   }

   if ( image->buf )
   {
	   roadmap_canvas_set_image_texture( image );
	   free( image->buf );
	   image->buf = NULL;
	   return TRUE;
   }
   else
   {
	   roadmap_log( ROADMAP_INFO, "Failed loading image data for file: %s", SAFE_STR( image->full_path ) );
   }
   return FALSE;
}

BOOL roadmap_canvas_set_image_texture( RoadMapImage image )
{
   GLuint textures[1];
   if ( !image->buf )
   {
	   roadmap_log( ROADMAP_WARNING, "No image data for image: %s", SAFE_STR( image->full_path ) );
	   return FALSE;
   }

   if ( !is_canvas_ready() )
   {
	   image->is_valid = 0;
	   return FALSE;
   }
   
   if (image->width <= CANVAS_ATLAS_TEX_SIZE &&
       image->height <= CANVAS_ATLAS_TEX_SIZE) {
      if (!roadmap_canvas_atlas_insert (IMAGE_HINT, &image)) {
         roadmap_log (ROADMAP_ERROR, "Could not cache image '%s' in texture atlas !", SAFE_STR( image->full_path));
         return FALSE;
      }
   } else {
      glGenTextures( 1, textures );
      
      image->texture = textures[0];
      
      set_state( CANVAS_GL_STATE_IMAGE, image->texture );
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, next_pot( image->width ), next_pot( image->height ), 0, GL_RGBA, GL_UNSIGNED_BYTE, image->buf );
      
   }
   
   image->frame_buf = CANVAS_INVALID_TEXTURE_ID;
   image->is_valid = 1;
   
   return TRUE;
}

void roadmap_canvas_image_set_mutable (RoadMapImage src) {}

void roadmap_canvas_draw_image (RoadMapImage image, const RoadMapGuiPoint *pos,
                                int opacity, int mode )
{
	RoadMapGuiPoint bottom_right_point;

	bottom_right_point.x = pos->x + image->width; // Actually w-1
	bottom_right_point.y = pos->y + image->height; // Actually h-1

	roadmap_canvas_draw_image_scaled( image, pos, &bottom_right_point, opacity, mode );
}


INLINE_DEC BOOL draw_image_prepare( RoadMapImage image, int opacity, int mode )
{
   float alpha;

   if ( image == NULL )
   {
	  roadmap_log ( ROADMAP_WARNING, "Image is null" );
	  return FALSE;
   }

   if ( !is_canvas_ready() )
	   return FALSE;

   /*
	* If not valid - restore
	*/
   if ( !image->is_valid )
   {
	   if ( image->restore_cb )
	   {
		   if ( !image->restore_cb( image ) )
		   {
			   roadmap_log ( ROADMAP_ERROR, "Restoration of the image is failed for image: %s. Address: %x", SAFE_STR( image->full_path ), image );
			   return FALSE;
		   }

	   }
	   else
	   {
		   roadmap_log ( ROADMAP_ERROR, "Restoration callback is undefined for image: %s", SAFE_STR( image->full_path ) );
		   return FALSE;
	   }
   }

   if ( !image->is_valid || image->texture == CANVAS_INVALID_TEXTURE_ID )
   {
	   roadmap_log( ROADMAP_ERROR, "Cannot draw not initialized texture!!! %s", SAFE_STR( image->full_path ) );
	   return FALSE;
   }

   //Handle opacity
	if ( ( mode == IMAGE_SELECTED ) || ( opacity <= 0 ) || ( opacity >= 255 ) )
	{
	   opacity = 255;
    }

   alpha = opacity/255.0;

   if ( mode == IMAGE_NOBLEND )
   {
	   glDisable( GL_BLEND );
   }


   glColor4f(1, 1, 1, alpha);
   
   if ( !set_state( CANVAS_GL_STATE_IMAGE, image->texture ) )
	   return FALSE;

   return TRUE;
}

void roadmap_canvas_draw_image_scaled( RoadMapImage image, const RoadMapGuiPoint *top_left_pos, const RoadMapGuiPoint *bottom_right_pos,
                                int opacity, int mode )
{
   int tex_size_w, tex_size_h;
   
   if ( !draw_image_prepare( image, opacity, mode ) )
   {
      return;
   }
   
   GLfloat glpoints[] = {
      top_left_pos->x, top_left_pos->y, Z_LEVEL,
      bottom_right_pos->x , top_left_pos->y, Z_LEVEL,
      bottom_right_pos->x , bottom_right_pos->y, Z_LEVEL,
      top_left_pos->x, bottom_right_pos->y, Z_LEVEL
   };
   
   GLfloat x_offset = image->offset.x;
   GLfloat y_offset = image->offset.y;
   
   if (image->width <= CANVAS_ATLAS_TEX_SIZE &&
       image->height <= CANVAS_ATLAS_TEX_SIZE) {
      tex_size_h = tex_size_w = CANVAS_ATLAS_TEX_SIZE;
   } else {
      tex_size_w = next_pot(image->width);
      tex_size_h = next_pot(image->height);
   }

   
   GLfloat texCoords[] = {
      OGL_FLT_SHIFT_POS( x_offset )/ (GLfloat)tex_size_w, OGL_FLT_SHIFT_POS( y_offset )/ (GLfloat)tex_size_h,
      OGL_FLT_SHIFT_NEG( x_offset + image->width ) / (GLfloat)tex_size_w, OGL_FLT_SHIFT_POS( y_offset )/ (GLfloat)tex_size_h,
      OGL_FLT_SHIFT_NEG( x_offset + image->width ) / (GLfloat)tex_size_w, OGL_FLT_SHIFT_NEG( y_offset + image->height ) / (GLfloat)tex_size_h,
      OGL_FLT_SHIFT_POS( x_offset ) / (GLfloat)tex_size_w, OGL_FLT_SHIFT_NEG( y_offset + image->height ) / (GLfloat)tex_size_h
   };
   
   
   //glBindTexture( GL_TEXTURE_2D, image->texture );
   glVertexPointer( 3, GL_FLOAT, 0, glpoints );
   glTexCoordPointer( 2, GL_FLOAT, 0, texCoords );
   glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
   check_gl_error();
   
   if (CurrentPen)
      roadmap_canvas_select_pen(CurrentPen);
   
   
   if ( mode == IMAGE_NOBLEND )
   {
      glEnable( GL_BLEND );
   }
}
/*
 * Draws the target image in the area defined by top_left_pos and bottom_right_pos by
 * stretching of the source image according to the pivot position. Preserves the corners defined by the pivot.
 * Stretches the rectangle ( pivot.x, pivot.y ), ( pivot.x + 1, pivot.y + 1) to fill the rest of the area in the
 * target.
 *
 * Written by: AGA
 */
void roadmap_canvas_draw_image_stretch( RoadMapImage image, const RoadMapGuiPoint *top_left_pos, const RoadMapGuiPoint *bottom_right_pos,
											const RoadMapGuiPoint *pivot_pos, int opacity, int mode )
{
   RoadMapGuiPoint top_left_offset;
   RoadMapGuiPoint btm_right_offset;
   static const int scMapPointsCount = 22;
   
   GLfloat middle_hrz_tex;
   GLfloat middle_vrt_tex;
   GLfloat right_tex;
   GLfloat bottom_tex;
   GLfloat x_offset = 0.0f;
   GLfloat y_offset = 0.0f;
   
   if ( !draw_image_prepare( image, opacity, mode ) )
   {
      return;
   }
   
   top_left_offset.x = pivot_pos->x;
   top_left_offset.y = pivot_pos->y;
   
   btm_right_offset.x = image->width - pivot_pos->x;
   btm_right_offset.y = image->height - pivot_pos->y;
   
   if (image->width <= CANVAS_ATLAS_TEX_SIZE &&
       image->height <= CANVAS_ATLAS_TEX_SIZE) {
      x_offset = image->offset.x / (GLfloat)CANVAS_ATLAS_TEX_SIZE;
      y_offset = image->offset.y / (GLfloat)CANVAS_ATLAS_TEX_SIZE;
      middle_hrz_tex = x_offset + ( GLfloat) top_left_offset.x / (GLfloat)CANVAS_ATLAS_TEX_SIZE;
      middle_vrt_tex = y_offset + ( GLfloat)top_left_offset.y / (GLfloat)CANVAS_ATLAS_TEX_SIZE;
      right_tex = x_offset + (GLfloat)image->width / (GLfloat)CANVAS_ATLAS_TEX_SIZE;
      bottom_tex = y_offset + (GLfloat)image->height / (GLfloat)CANVAS_ATLAS_TEX_SIZE;
   } else {
      middle_hrz_tex = ( GLfloat) top_left_offset.x / (GLfloat)next_pot(image->width );
      middle_vrt_tex = ( GLfloat) top_left_offset.y / (GLfloat)next_pot(image->height );
      right_tex = (GLfloat)image->width / (GLfloat) next_pot(image->width);
      bottom_tex = (GLfloat)image->height / (GLfloat)next_pot(image->height);
   }
   
   GLfloat glpoints[] = {
      // Top Left Rectangle
      top_left_pos->x, top_left_pos->y, Z_LEVEL,
      top_left_pos->x + top_left_offset.x, top_left_pos->y, Z_LEVEL,
      top_left_pos->x, top_left_pos->y + top_left_offset.y, Z_LEVEL,
      top_left_pos->x + top_left_offset.x, top_left_pos->y + top_left_offset.y, Z_LEVEL,
      // Middle Left Rectangle
      top_left_pos->x, bottom_right_pos->y - btm_right_offset.y, Z_LEVEL,
      top_left_pos->x + top_left_offset.x, bottom_right_pos->y - btm_right_offset.y, Z_LEVEL,
      // Bottom Left Rectangle
      top_left_pos->x, bottom_right_pos->y, Z_LEVEL,
      top_left_pos->x + top_left_offset.x, bottom_right_pos->y, Z_LEVEL,
      // Dummy triangle
      bottom_right_pos->x - btm_right_offset.x, bottom_right_pos->y, Z_LEVEL,
      // Bottom Middle Rectangle
      top_left_pos->x + top_left_offset.x, bottom_right_pos->y - btm_right_offset.y, Z_LEVEL,
      bottom_right_pos->x - btm_right_offset.x, bottom_right_pos->y - btm_right_offset.y, Z_LEVEL,
      // Middle Middle Rectangle
      top_left_pos->x + top_left_offset.x, top_left_pos->y + top_left_offset.y, Z_LEVEL,
      bottom_right_pos->x - btm_right_offset.x, top_left_pos->y + top_left_offset.y, Z_LEVEL,
      // Top Middle Rectangle
      top_left_pos->x + top_left_offset.x, top_left_pos->y, Z_LEVEL,
      bottom_right_pos->x - btm_right_offset.x, top_left_pos->y, Z_LEVEL,
      // Dummy triangle
      bottom_right_pos->x, top_left_pos->y, Z_LEVEL,
      // Top Right Rectangle
      bottom_right_pos->x - btm_right_offset.x, top_left_pos->y + top_left_offset.y, Z_LEVEL,
      bottom_right_pos->x, top_left_pos->y + top_left_offset.y, Z_LEVEL,
      // Middle Right Rectangle
      bottom_right_pos->x - btm_right_offset.x, bottom_right_pos->y - btm_right_offset.y, Z_LEVEL,
      bottom_right_pos->x, bottom_right_pos->y - btm_right_offset.y, Z_LEVEL,
      // Bottom Right Rectangle
      bottom_right_pos->x - btm_right_offset.x, bottom_right_pos->y, Z_LEVEL,
      bottom_right_pos->x, bottom_right_pos->y, Z_LEVEL
   };
   
   GLfloat texCoords[] =
   {
      // Top Left Rectangle
      x_offset, y_offset,
      middle_hrz_tex, y_offset,
      x_offset, middle_vrt_tex,
      middle_hrz_tex, middle_vrt_tex,
      // Middle Left Rectangle
      x_offset, middle_vrt_tex + OGL_TEX_NEXT_POINT_OFFSET,
      middle_hrz_tex, middle_vrt_tex + OGL_TEX_NEXT_POINT_OFFSET,
      // Bottom Left Rectangle
      x_offset, bottom_tex,
      middle_hrz_tex, bottom_tex,
      // Dummy triangle
      middle_hrz_tex + OGL_TEX_NEXT_POINT_OFFSET, bottom_tex,
      // Bottom Middle Rectangle
      middle_hrz_tex, middle_vrt_tex + OGL_TEX_NEXT_POINT_OFFSET,
      middle_hrz_tex + OGL_TEX_NEXT_POINT_OFFSET, middle_vrt_tex + OGL_TEX_NEXT_POINT_OFFSET,
      // Middle Middle Rectangle
      middle_hrz_tex, middle_vrt_tex,
      middle_hrz_tex + OGL_TEX_NEXT_POINT_OFFSET, middle_vrt_tex,
      // Top Middle Triangle
      middle_hrz_tex, y_offset,
      middle_hrz_tex + OGL_TEX_NEXT_POINT_OFFSET, y_offset,
      // Dummy triangle
      right_tex, y_offset,
      // Top Right Rectangle
      middle_hrz_tex + OGL_TEX_NEXT_POINT_OFFSET, middle_vrt_tex,
      right_tex, middle_vrt_tex,
      // Middle Right Rectangle
      middle_hrz_tex + OGL_TEX_NEXT_POINT_OFFSET, middle_vrt_tex  + OGL_TEX_NEXT_POINT_OFFSET,
      right_tex, middle_vrt_tex + OGL_TEX_NEXT_POINT_OFFSET,
      // Bottom Right Rectangle
      middle_hrz_tex + OGL_TEX_NEXT_POINT_OFFSET, bottom_tex,
      right_tex, bottom_tex
   };
   
   //glBindTexture( GL_TEXTURE_2D, image->texture );
   glVertexPointer( 3, GL_FLOAT, 0, glpoints );
   glTexCoordPointer( 2, GL_FLOAT, 0, texCoords );
   glDrawArrays( GL_TRIANGLE_STRIP, 0, scMapPointsCount );
   check_gl_error();
   
   if (CurrentPen)
      roadmap_canvas_select_pen(CurrentPen);
   if ( mode == IMAGE_NOBLEND )
   {
      glEnable( GL_BLEND );
   }
}

/*
 * Special case of scaling - the middle parts of the image borders are stretched
 */
void roadmap_canvas_draw_image_middle_stretch( RoadMapImage image, const RoadMapGuiPoint *top_left_pos, const RoadMapGuiPoint *bottom_right_pos,
                                int opacity, int mode )
{
	const RoadMapGuiPoint pnt = {image->width >> 1, image->height >> 1};
	roadmap_canvas_draw_image_stretch( image, top_left_pos, bottom_right_pos, &pnt, opacity, mode );
}


void roadmap_canvas_copy_image (RoadMapImage dst_image,
                                const RoadMapGuiPoint *pos,
                                const RoadMapGuiRect  *rect,
                                RoadMapImage src_image, int mode)
{
   return;
}


void roadmap_canvas_draw_image_formated_text (RoadMapImage image,
                                     const RoadMapGuiPoint *position,
                                              int s, const char *text, int font_type)
{
   return;
}

void roadmap_canvas_draw_image_text (RoadMapImage image,
                                     const RoadMapGuiPoint *position,
                                     int size, const char *text)
{
   roadmap_canvas_draw_image_formated_text(image, position, size, text, FONT_TYPE_BOLD);
}

void roadmap_canvas_free_image (RoadMapImage image)
{
   if (!image)
      return;
   GLuint item;

   roadmap_log (ROADMAP_INFO, "Freeing image id: %d. Address: 0x%x. Path: %s", image->texture, image, SAFE_STR( image->full_path ) );

   if ( !is_canvas_ready() )
	   return;

   item = image->texture;
/*
   if (item != CANVAS_INVALID_TEXTURE_ID )
      glDeleteTextures( 1, &item );
*/
#ifdef IPHONE
   item = image->frame_buf;
   if (item != CANVAS_INVALID_TEXTURE_ID )
      glDeleteFramebuffersOES(1, &item);
#endif
   check_gl_error();

   unmanaged_list_remove( image );

   if ( image->full_path )
	   free( image->full_path );
   if ( image->buf )
	   free( image->buf );

   free (image);
}

void roadmap_canvas_image_invalidate( RoadMapImage image )
{
   if ( !image )
      return;

   GLuint texture;

   texture = image->texture;
   image->texture = CANVAS_INVALID_TEXTURE_ID;
   /*
    * AGA NOTE: glDeleteTextures causes distortions after return from background
    * Must check this and understand!!!!
    *
    * glDeleteTextures(1, &texture);
    */
   image->is_valid = 0;
}

int  roadmap_canvas_image_width  (const RoadMapImage image) {

   if (!image) return 0;

   return image->width;
}


int  roadmap_canvas_image_height (const RoadMapImage image) {

   if (!image) return 0;

   return image->height;
}

int  roadmap_canvas_get_thickness  (RoadMapPen pen)
{
	return (pen->lineWidth);
}


RoadMapImage roadmap_canvas_image_from_buf( unsigned char* buf, int width, int height, int stride ) {

   GLuint uWidth;
   GLuint uHeight;
   RoadMapImage image = malloc(sizeof(roadmap_canvas_image));
   roadmap_check_allocated (image);
   int i, j;

   image->width = width;
   image->height = height;
   image->full_path = strdup( CANVAS_IMAGE_FROM_BUF_TYPE );
   image->restore_cb = roadmap_canvas_set_image_texture;
   unmanaged_list_add( image );

   if (image->width <= CANVAS_ATLAS_TEX_SIZE &&
       image->height <= CANVAS_ATLAS_TEX_SIZE) {
      image->buf = buf;
   } else {
      uWidth = next_pot(image->width);
      uHeight = next_pot(image->height);
      
      GLubyte* temp_buf= malloc(4 * uWidth * uHeight);
      roadmap_check_allocated (temp_buf);
      
      for( j = 0; j < uHeight; j++) {
         for( i = 0; i < uWidth; i++){
            temp_buf[4*(i+j*uWidth)]     = (i >= width || j >= height) ? 0 : (int)buf[4*(i+j*width)   ];
            temp_buf[4*(i+j*uWidth) +1]  = (i >= width || j >= height) ? 0 : (int)buf[4*(i+j*width) +1];
            temp_buf[4*(i+j*uWidth) +2]  = (i >= width || j >= height) ? 0 : (int)buf[4*(i+j*width) +2];
            temp_buf[4*(i+j*uWidth) +3]  = (i >= width || j >= height) ? 0 : (int)buf[4*(i+j*width) +3];
         }
      }
      
      free(buf);
      image->buf = temp_buf;
      image->offset.x = 0;
      image->offset.y = 0;
   }
   
   image->frame_buf = CANVAS_INVALID_TEXTURE_ID;

   /*
    * If canvas ready - define the texture, otherwise just assign the buffer and mark not valid
    */
   if ( is_canvas_ready() )
   {
	   roadmap_canvas_set_image_texture( image );
   }
   else
   {
	   image->is_valid = 0;
   }

   return image;
}

int roadmap_canvas_buf_from_image( RoadMapImage image, unsigned char** image_buf) {
   //Create buffer from image
   unsigned char* temp_buf =  image->buf;
   unsigned char *new_buf;
   int uWidth, i, j;
   
   if (!image_buf) { // image must have cached buffer, TODO: add buffer from texture support
      *image_buf = NULL;
      return 0;
   }
   
   new_buf = malloc(4 * image->width * image->height);
   roadmap_check_allocated (new_buf);
   
   if (image->width <= CANVAS_ATLAS_TEX_SIZE &&
       image->height <= CANVAS_ATLAS_TEX_SIZE) {
      memcpy(new_buf, temp_buf, 4 * image->width * image->height);
   } else {
      uWidth = next_pot(image->width);
      
      for( j = 0; j < image->height; j++) {
         for( i = 0; i < image->width; i++){
            new_buf[4*(i+j*image->width)]     = (int)temp_buf[4*(i+j*uWidth)   ];
            new_buf[4*(i+j*image->width) +1]  = (int)temp_buf[4*(i+j*uWidth) +1];
            new_buf[4*(i+j*image->width) +2]  = (int)temp_buf[4*(i+j*uWidth) +2];
            new_buf[4*(i+j*image->width) +3]  = (int)temp_buf[4*(i+j*uWidth) +3];
         }
      }
   }
   
   *image_buf = new_buf;
   
   return (4 * image->width * image->height);
}

int roadmap_canvas_get_generic_screen_type( int width, int height )
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

void roadmap_canvas_ogl_configure( float anti_alias_factor, float font_factor, float thickness_factor )
{
   static BOOL initialized = FALSE;

   RoadMapCanvasSavedState = CANVAS_GL_STATE_FORCE_GEOMETRY;

   set_state(CANVAS_GL_STATE_FORCE_GEOMETRY, 0);

   if ( !initialized )
   {
      int i;
      int width, ascent, descent, can_tilt=1;
      
      /*
       * Font
       */
      RoadMapFontFactor = font_factor;
      
      /*
       * Thickness  factor
       */
      RoadMapMinThickness = thickness_factor;
      
      /*
       * Texture units
       */
      glGetIntegerv(GL_MAX_TEXTURE_UNITS, &num_tex_units);
      roadmap_log(ROADMAP_DEBUG, "Number of texture units: %d", (int)num_tex_units);
      if (num_tex_units > MAX_TEX_UNITS)
         num_tex_units = MAX_TEX_UNITS;
      for (i = 0; i < num_tex_units; i++) {
         tex_units[i] = CANVAS_INVALID_TEXTURE_ID;
      }
      current_tex_unit = 0;
      
      /*
       * preload digits 0-9
       */
      roadmap_canvas_get_text_extents("0123456789", -1, &width, &ascent, &descent, &can_tilt);
      RoadMapAAFactor = anti_alias_factor;


      initialized = TRUE;
   }
}


//////////////////////////////////////////
// Anti Alias
// minimal implementation, based on glAA code by arekkusu

INLINE_DEC float frsqrtes_nr(float x) {
#ifdef OPTIMIZE_SQRT
   volatile float fX = x;
   volatile float *pfX = &fX;
   volatile int   *piX = NULL;

   float xhalf = 0.5f * fX;

   piX = ( volatile int* ) pfX;				// store floating-point bits in integer
   *piX = 0x5f3759d5 - ( *piX >> 1 );       // initial guess for Newton's method
   pfX = ( volatile float* ) piX;				// convert new bits into float
   fX = *pfX;
   fX = fX * ( 1.5f - xhalf*fX*fX );       // One round of Newton's method

   return fX;

#else

	return 1.0f/sqrtf(x);
#endif //OPTIMIZE_SQRT
}

static inline float ifun(float x, float y, float F, int phf) {   		// compute falloff at x,y with exponent F [-1..1]
	float S = (x*x + y);
	float L;
	if (S) {											// estimate sqrt, accurate to about 1/4096
		L = frsqrtes_nr(S);
		L = L * S;
	}
	else
		L = 0.0f;
	if (F == 0.0f)										// intensity: [-1..0..1] = bloom..normal..fuzzy
		S = 0.0f;
	else if (F > 0.0f)
		// this is just pow(L/phf, F) but with a lookup table
		//S = exp(logtbl[(int)(L*4.0f)]/F);
      S = pow(L/phf, F); //anyhow F is 0.0 for now...
	//else
		// this is a bit more complex-- logarithm ramp up to a solid dot with slight S blend at cusp
		//S = L<(1.0f+F)*phf ? 0.0f : exp((1.0f+F*0.9f)/(L/phf-1.0f-F) * logtbl[(int)(L*4.0f)]);
	return 1.0f-S;
}

// 8-way symmetric macro
#define putpixel(x, y, I) {				   \
unsigned char c = 0xFF & ((int) (I*255));  \
texture[(pct   + x) + (pct   + y)*pdb] =   \
texture[(pct-1 - x) + (pct   + y)*pdb] =   \
texture[(pct   + x) + (pct-1 - y)*pdb] =   \
texture[(pct-1 - x) + (pct-1 - y)*pdb] =   \
texture[(pct   + y) + (pct   + x)*pdb] =   \
texture[(pct-1 - y) + (pct   + x)*pdb] =   \
texture[(pct   + y) + (pct-1 - x)*pdb] =   \
texture[(pct-1 - y) + (pct-1 - x)*pdb] = c;\
}


static void generate_aa_tex_f() { //filled version
   unsigned char			*texture, *texture_prev, *texture_temp;
   float F = 0.0f;
   int level = 0;
   unsigned char mipfix[4];
   unsigned char fix;
   float alias = RoadMapAAFactor;
   int phf = MAX_AA_SIZE;

   if (RoadMapAATex != 0)
      return;

   if ( !is_canvas_ready())
	   return;


   texture = calloc(1, pdb*pdb);
   roadmap_check_allocated (texture);
   texture_prev = calloc(1, pdb*pdb);
   roadmap_check_allocated (texture_prev);

   glGenTextures(1, &RoadMapAATex);
   set_state( CANVAS_GL_STATE_IMAGE, RoadMapAATex);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   //glTexParameterf(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
   // Set filtering depending on our aliasing mode.
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (alias>0.66)?GL_NEAREST:GL_LINEAR );
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (alias>0.33)?((alias>0.66)?GL_NEAREST:GL_LINEAR_MIPMAP_NEAREST):GL_LINEAR_MIPMAP_LINEAR );
   
   while (phf > 0) {
      
      if (level == 0) {
         int x = phf-1;
         int y = 0;
         int ax;
         float T = 0.0f;
         float ys = 0.0f;
         while (x > y) {
            ys = (float)(y*y);
            float L = sqrt(prs - ys);
            float D = ceilf(L)-L;
            if (D < T) x--;
            for(ax=y; ax<x; ax++)
            {
               putpixel(ax, y, ifun(ax, ys, F, phf));						// fill wedge
            }
            {
               float I = (1.0f-D)*( ifun(x, ys, F, phf ) );
               putpixel( x, y, I );					// outer edge
            }
            T = D;
            y++;
         }
         
         // fix the upper mip levels so that 1 px points/lines stay bright:
         fix = (texture[(int)(psz+phf*0.2f)+pct*pdb]+texture[(int)(psz+phf*0.7f)+pct*pdb])>>1;
         mipfix[0]=mipfix[1]=mipfix[2]=mipfix[3]=fix;
      } else {
         unsigned char  *s;
         unsigned char  *t;
         int i, j;
         
         s = texture;
         t = texture_prev;
         
         for (i = 0; i < pdb; i++) {
            for (j = 0; j < pdb; j++) {
               s[0] = (t[0] + t[1] + 
                       t[pdb*2] + t[pdb*2 +1] + 2) / 4;
               t += 2;
               s++;
            }
            t += pdb*2;
         }
      }
      

      // Clamp the max mip level to 2x2 (1 px with 1 px border all around...)
      glTexImage2D( GL_TEXTURE_2D, level , GL_ALPHA, pdb, pdb, 0, GL_ALPHA, GL_UNSIGNED_BYTE, texture );
      check_gl_error();
      
      texture_temp = texture_prev;
      texture_prev = texture;
      texture = texture_temp;

      if (phf == 1)
         phf = 0;
      else {
         phf /= 2;
         //memset (texture, 0, sizeof(texture));
      }
      level++;
   }

   //fix the 4x4 level
   //glTexSubImage2D(GL_TEXTURE_2D, level -1, 2, 2, 2, 2, GL_ALPHA, GL_UNSIGNED_BYTE, mipfix);

   //create the 2x2 level
   mipfix[0] = (texture_prev[0] + texture_prev[1] + texture_prev[4] + texture_prev[5])/4;
   mipfix[1] = (texture_prev[2] + texture_prev[3] + texture_prev[6] + texture_prev[7])/4;
   mipfix[2] = (texture_prev[8] + texture_prev[9] + texture_prev[12] + texture_prev[13])/4;
   glTexImage2D( GL_TEXTURE_2D, level++ , GL_ALPHA, 2, 2, 0, GL_ALPHA, GL_UNSIGNED_BYTE, mipfix );

   //level 1x1
   glTexImage2D( GL_TEXTURE_2D, level , GL_ALPHA, 1, 1, 0, GL_ALPHA, GL_UNSIGNED_BYTE, mipfix );
   check_gl_error();

   free(texture);
   free(texture_prev);
}

static void generate_aa_tex_nf() { //no fill version
   unsigned char			*texture, *texture_prev, *texture_temp;
   float F = 0.0f;
   int level = 0;
   unsigned char mipfix[4];
   unsigned char fix;
   float alias = RoadMapAAFactor;

   // some convienence constants for the texture dimensions:
   // point size, size minus one, half size squared, double size, point center, radius squared
   int phf = MAX_AA_SIZE;

   if (RoadMapAATexNf != 0)
      return;

   if (!is_canvas_ready())
      return;

   texture = calloc(1, pdb*pdb);
   roadmap_check_allocated (texture);
   texture_prev = calloc(1, pdb*pdb);
   roadmap_check_allocated (texture_prev);

   glGenTextures(1, &RoadMapAATexNf);
   set_state( CANVAS_GL_STATE_IMAGE, RoadMapAATexNf);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   //glTexParameterf(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
   // Set filtering depending on our aliasing mode.
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (alias>0.66)?GL_NEAREST:GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (alias>0.33)?((alias>0.66)?GL_NEAREST:GL_LINEAR_MIPMAP_NEAREST):GL_LINEAR_MIPMAP_LINEAR);

   while (phf > 0) {
      if (level == 0) {

      int x = phf-1;
      int y = 0;
      int ax;
      float T = 0.0f;
      float ys = 0.0f;
      while (x > y) {
         ys = (float)(y*y);
         float L = sqrt(prs - ys);
         float D = ceilf(L)-L;
         if (D < T) x--;
         for(ax=x - 1; ax<x; ax++)
         {
            putpixel(ax, y, ifun(ax, ys, F, phf));						// fill wedge
         }
         {
            float I = (1.0f-D)*( ifun(x, ys, F, phf ) );
            putpixel(x, y, I );					// outer edge
         }
         T = D;
         y++;
      }
         // fix the upper mip levels so that 1 px points/lines stay bright:
         fix = (texture[(int)(psz+phf*0.2f)+pct*pdb]+texture[(int)(psz+phf*0.7f)+pct*pdb])>>1;
         mipfix[0]=mipfix[1]=mipfix[2]=mipfix[3]=fix;
      } else {
         unsigned char  *s;
         unsigned char  *t;
         int i, j;
         
         s = texture;
         t = texture_prev;
         
         for (i = 0; i < pdb; i++) {
            for (j = 0; j < pdb; j++) {
               s[0] = (t[0] + t[1] + 
                       t[pdb*2] + t[pdb*2 +1] + 2) / 4;
               t += 2;
               s++;
            }
            t += pdb*2;
         }
      }


      // Clamp the max mip level to 2x2 (1 px with 1 px border all around...)
      glTexImage2D( GL_TEXTURE_2D, level , GL_ALPHA, pdb, pdb, 0, GL_ALPHA, GL_UNSIGNED_BYTE, texture );
      check_gl_error();
      
      texture_temp = texture_prev;
      texture_prev = texture;
      texture = texture_temp;
      
      if (phf == 1)
         phf = 0;
      else {
         phf /= 2;
         //memset (texture, 0, sizeof(texture));
      }
      level++;
   }

   //fix the 4x4 level
   //glTexSubImage2D(GL_TEXTURE_2D, level -1, 2, 2, 2, 2, GL_ALPHA, GL_UNSIGNED_BYTE, mipfix);
   
   //create the 2x2 level
   mipfix[0] = (texture_prev[0] + texture_prev[1] + texture_prev[4] + texture_prev[5])/4;
   mipfix[1] = (texture_prev[2] + texture_prev[3] + texture_prev[6] + texture_prev[7])/4;
   mipfix[2] = (texture_prev[8] + texture_prev[9] + texture_prev[12] + texture_prev[13])/4;
   glTexImage2D( GL_TEXTURE_2D, level++ , GL_ALPHA, 2, 2, 0, GL_ALPHA, GL_UNSIGNED_BYTE, mipfix );
   
   //level 1x1
   glTexImage2D( GL_TEXTURE_2D, level , GL_ALPHA, 1, 1, 0, GL_ALPHA, GL_UNSIGNED_BYTE, mipfix );
   check_gl_error();
   
   free(texture);
   free(texture_prev);
}


static void generate_aa_tex() {
   generate_aa_tex_f();
   generate_aa_tex_nf();
}

void roadmap_canvas_shutdown()
{
	RoadMapCanvasSavedState = CANVAS_GL_STATE_NOT_READY;
	//glDeleteTextures( 1, &RoadMapAATexNf );
	RoadMapAATexNf = 0;
	//glDeleteTextures( 1, &RoadMapAATex );
	RoadMapAATex = 0;
	roadmap_canvas_font_shutdown();
	unmanaged_list_invalidate();
   roadmap_canvas_atlas_clean(IMAGE_HINT);
}



#ifdef ANDROID	/* Temporary workaround for the colors distortions problem in android <= 1.6    *** AGA *** */
void roadmap_canvas_draw_multiple_lines_slow( int count, int *lines, RoadMapGuiPoint *points, int fast_draw )
{

   int phf = MAX_AA_SIZE;
   roadmap_gl_vertex glpoints[8];
   float r = CurrentPen->lineWidth;
   int count_of_points;
   float bord = phf / r;
   RoadMapGuiPoint *prev;
   int i;
   float x, y, x1, y1;
   float perp_y, perp_x, perpd, leng, factor;
   float width; 			// = (maxr+1.0f)*0.5f;
   float unit_x, unit_y;
   float parl_x, parl_y;


   generate_aa_tex();

   set_state(CANVAS_GL_STATE_IMAGE, RoadMapAATex);

   glpoints[0].z = glpoints[1].z =
   glpoints[2].z = glpoints[3].z =
   glpoints[4].z = glpoints[5].z =
   glpoints[6].z = glpoints[7].z = Z_LEVEL;

   glpoints[3].tx = glpoints[1].tx = glpoints[5].tx = glpoints[7].tx =
   glpoints[0].ty = glpoints[1].ty = glpoints[6].ty = glpoints[7].ty = (float)(psz-bord)/(float)pdb;
   glpoints[2].tx = glpoints[0].tx = glpoints[4].tx = glpoints[6].tx = (float)(pdb+bord)/(float)pdb;
   glpoints[3].ty = glpoints[2].ty = (float)(pct)/(float)pdb;
   glpoints[5].ty = glpoints[4].ty = (float)(pct + 0.0001f)/(float)pdb;

   for (i = 0; i < count; ++i) {

      count_of_points = *lines;
      prev = points;
      points++;
      count_of_points--;

      while (count_of_points) {
         x1 = (float)prev->x;
         y1 = (float)prev->y;
         x = (float)points->x;
         y = (float)points->y;

         perp_y = x1-x;
         perp_x = y-y1;
         factor = perp_y*perp_y+perp_x*perp_x;

         if (factor) {
            perpd = frsqrtes_nr(factor);
            perp_y *= perpd;				// normalize to 1px
            perp_x *= perpd;

            x1 -= perp_y*0.5f;
            y1 += perp_x*0.5f;
         } else {
            perp_y = 0.0f;
            perp_x = 1.0f;
         }

         width = (r+1.0f)*0.5f;
         unit_x = -perp_y;
         unit_y = perp_x;
         perp_y *= width;
         perp_x *= width;
         parl_x = -perp_y;
         parl_y =  perp_x;

         glpoints[0].x = x1-perp_x-parl_x;		glpoints[0].y = y1-perp_y-parl_y;
         glpoints[1].x = x1+perp_x-parl_x;		glpoints[1].y = y1+perp_y-parl_y;
         glpoints[2].x = x1-perp_x;			glpoints[2].y = y1-perp_y;
         glpoints[3].x = x1+perp_x;			glpoints[3].y = y1+perp_y;
         glpoints[4].x = x-perp_x;				glpoints[4].y = y-perp_y;
         glpoints[5].x = x+perp_x;				glpoints[5].y = y+perp_y;
         glpoints[6].x = x-perp_x+parl_x;		glpoints[6].y = y-perp_y+parl_y;
         glpoints[7].x = x+perp_x+parl_x;		glpoints[7].y = y+perp_y+parl_y;


         glVertexPointer(3, GL_FLOAT, sizeof(roadmap_gl_vertex), &glpoints[0].x);
         glTexCoordPointer(2, GL_FLOAT, sizeof(roadmap_gl_vertex), &glpoints[0].tx);
         roadmap_canvas_color_fix();	/* Android only. Empty for others */
         glDrawArrays(GL_TRIANGLE_STRIP, 0, 8);
         check_gl_error();

         prev = points;
         points++;
         count_of_points--;
      }

      lines++;
   }
}
#endif	// ANDROID multiple_lines

