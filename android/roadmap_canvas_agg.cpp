/* roadmap_canvas_agg.cpp - manage the canvas that is used to draw the map with agg
 *
 * LICENSE:
 *
  *   Copyright 2008 Alex Agranovich
 *
 *   This file is part of RoadMap.
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
 * SYNOPSYS:
 *
 *   See roadmap_canvas.h.
 */

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
#include "colors.h"
#include "roadmap_libpng.h"
#include "roadmap_types.h"

extern "C" {
#include "roadmap.h"
#include "roadmap_canvas.h"
#include "roadmap_native_keyboard.h"
#include "roadmap_messagebox.h"
#include "roadmap_math.h"
#include "roadmap_config.h"
#include "roadmap_path.h"
#include "roadmap_screen.h"
#include "JNI/FreeMapJNI.h"
#include "roadmap_wchar.h"
#include "ssd/ssd_dialog.h"
#include "roadmap_device_events.h"
}
#include "roadmap_androidcanvas.h"
#include "roadmap_canvas_agg.h"



static roadmap_android_canvas_type	gAndrCanvas = {0, 0, 0, NULL};

/////////////// Forward declarations ///////////////////
static int get_native_bpp();


/*************************************************************************************************
 * roadmap_canvas_agg_to_wchar
 * Converts multibyte to the wchar
 */
int roadmap_canvas_agg_to_wchar (const char *text, wchar_t *output, int size)
{
  int length = mbstowcs( ( RM_WCHAR_T *) output, text, size - 1);
  output[length] = 0;
  return length;
}

/*************************************************************************************************
 * roadmap_canvas_agg_parse_color
 * Parsing color for the agg
 */
agg::rgba8 roadmap_canvas_agg_parse_color (const char *color)
{
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
		for (low=(-1), high=sizeof( color_table )/sizeof(color_table[0]);
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

/*************************************************************************************************
 * roadmap_canvas_set_properties
 * Initializes the canvas_agg context this the display dvice parameters
 */
EXTERN_C void roadmap_canvas_set_properties( int aWidth, int aHeight, int aPixelFormat )
{
	int cur_orientation = ( gAndrCanvas.height > gAndrCanvas.width );
	int new_orientation = ( aHeight > aWidth );

	if ( cur_orientation != new_orientation )
		roadmap_device_event_notification( device_event_window_orientation_changed );

	gAndrCanvas.height = aHeight;
	gAndrCanvas.width = aWidth;
	gAndrCanvas.pixel_format = aPixelFormat;
	// The buffer gAndrCanvas.pCanvasBuf will be allocated later
	gAndrCanvas.pCanvasBuf = NULL;
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

	// Signal the java layer to update the screen
	FreeMapNativeCanvas_RefreshScreen();
}

/*************************************************************************************************
 * roadmap_canvas_new()
 * Memory allocation for the new canvas
 */
EXTERN_C void roadmap_canvas_new ()
{
	int lBytes2Allocate;
	int lStride;

	roadmap_log( ROADMAP_INFO, "Canvas new .... \n" );

	// The gAndrCanvas has to be initialized before calling this function
	if ( !gAndrCanvas.width || !gAndrCanvas.height )
	{
		roadmap_log( ROADMAP_ERROR, "Canvas failed - canvas properties are incorrect or weren't set \n" );
		return;
	}
	// If canvas already allocated - free the memory
	if ( gAndrCanvas.pCanvasBuf )
	{
		free( gAndrCanvas.pCanvasBuf );
		gAndrCanvas.pCanvasBuf = NULL;
	}
	// Allocate the memory for the canvas
	lStride = DEFAULT_BPP * gAndrCanvas.width;
	lBytes2Allocate = lStride * gAndrCanvas.height;
	gAndrCanvas.pCanvasBuf = ( unsigned int * ) calloc( lBytes2Allocate, 1 );

	if ( !gAndrCanvas.pCanvasBuf )
	{
		roadmap_log( ROADMAP_ERROR, "Canvas allocation failed \n" );
		return;
	}
	// There are no alignment problems ....
	roadmap_canvas_agg_configure( ( unsigned char* ) gAndrCanvas.pCanvasBuf, gAndrCanvas.width,
									 gAndrCanvas.height, lStride );
	// Run the configurator
	(*RoadMapCanvasConfigure) ();
	// Notify about system event
	roadmap_device_event_notification( device_event_window_orientation_changed);
	// Restore keyboard if suppposed to be visible
	if ( roadmap_native_keyboard_visible() )
	{
		RMNativeKBParams params;
		roadmap_native_keyboard_get_params( &params );
		roadmap_native_keyboard_show( &params );
	}
}

/*************************************************************************************************
 * get_native_bpp()
 * Translates the native pixel format to the bytes per pixel allocation
 */
static int get_native_bpp()
{
	int lTargetBPP = DEFAULT_BPP;
	switch ( ( android_pixel_format_type ) gAndrCanvas.pixel_format )
	{
		case _andr_pix_fmt_RGBX_8888:
		case _andr_pix_fmt_RGBA_8888:
		{
			lTargetBPP = 4;
			break;
		}
		case _andr_pix_fmt_RGB_888:
		{
			lTargetBPP = 3;
			break;
		}
		case _andr_pix_fmt_RGB_565:
		default:
		{
			lTargetBPP = 2;
			break;
		}
	}
	return lTargetBPP;
}

EXTERN_C void roadmap_canvas_agg_close()
{
	roadmap_log( ROADMAP_INFO, "Canvas close \n" );
	// Memory deallocation
	free( gAndrCanvas.pCanvasBuf );
}

EXTERN_C void GetAndrCanvas( const roadmap_android_canvas_type** aCanvasPtr )
{
	*aCanvasPtr = &gAndrCanvas;
}

EXTERN_C void roadmap_canvas_mouse_pressed( int aX, int aY )
{
	RoadMapGuiPoint point;
	point.x = aX;
	point.y = aY;
	(*RoadMapCanvasMouseButtonPressed)( &point );
}

EXTERN_C void roadmap_canvas_mouse_released( int aX, int aY )
{
	RoadMapGuiPoint point;
	point.x = aX;
	point.y = aY;
	(*RoadMapCanvasMouseButtonReleased)( &point );
}

EXTERN_C void roadmap_canvas_mouse_moved( int aX, int aY )
{
	RoadMapGuiPoint point;
	point.x = aX;
	point.y = aY;
	(*RoadMapCanvasMouseMoved)( &point );
}

EXTERN_C BOOL roadmap_canvas_agg_is_landscape()
{
	return ( gAndrCanvas.width > gAndrCanvas.height );
}
