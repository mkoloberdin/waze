/* roadmap_border_ogl.c - Handle drawing of borders through opengl
 * CUrrently working for touch. Should replace the roadmap_border when
 * all the platforms will supoort opengl
 *
 * LICENSE:
 *
 *   Copyright 2010 Alex Agranovich (AGA), Waze Ltd
 *   Copyright 2008 Avi B.S
 *   Copyright 2008 Ehud Shabtai
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

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_gui.h"
#include "roadmap_math.h"
#include "roadmap_line.h"
#include "roadmap_street.h"
#include "roadmap_config.h"
#include "roadmap_canvas.h"
#include "roadmap_message.h"
#include "roadmap_sprite.h"
#include "roadmap_voice.h"
#include "roadmap_skin.h"
#include "roadmap_plugin.h"
#include "roadmap_square.h"
#include "roadmap_math.h"
#include "roadmap_res.h"
#include "roadmap_bar.h"
#include "roadmap_border.h"
#include "roadmap_canvas_ogl.h"
#include "roadmap_display.h"
#include "roadmap_device.h"


typedef enum
{
	_border_image_none = -1,
	_border_image_black = 0x0,
	_border_image_white,
	_border_image_black_pointer_comment,
	_border_image_white_menu,
	_border_image_count
} OGLBorderImageType;

/*
 * Points for the stretch to start from
 * derived from the resources
 */
const RoadMapGuiPoint OGLBorderStretchPoints[] =
{
	{14, 14},	// Black border
	{6, 6},		// White border
	{10, 10},   // Black border pointer comment
	{30, 5}    // White border menu
};


/*
 * Map of the border images files
 */
static const char* OGLBorderImageFiles[_border_image_count] =
{
		"border_black",
		"border_white",
		"border_black_pointer_comment",
		"border_white_menu"
};



void roadmap_border_shutdown()
{
}

BOOL roadmap_border_initialize()
{
   int   i;
   // Preload of borders to allow dialogs up faster
   for( i=0; i < _border_image_count; i++ )
   {

      roadmap_res_get( RES_BITMAP, RES_SKIN, OGLBorderImageFiles[i] );
   }
   return TRUE;
}

/*
 * Returns the matching border type according to its parameters
 */
static OGLBorderImageType get_border_type( int style, int header, int pointer_type )
{
	if ( ( style == STYLE_BLACK ) && ( pointer_type == POINTER_COMMENT ) )
	{
		return _border_image_black_pointer_comment;
	}
	if ( ( style == STYLE_WHITE ) && ( pointer_type == POINTER_MENU ) )
	{
		return _border_image_white_menu;
	}
	if ( style == STYLE_BLACK )
	{
		return _border_image_black;
	}
	if ( style == STYLE_WHITE )
	{
		return _border_image_white;
	}
	return _border_image_none;
}

int roadmap_display_border( int style, int header, int pointer_type, RoadMapGuiPoint *bottom, RoadMapGuiPoint *top, const char* background, RoadMapPosition *position )
{
	RoadMapPen fill_pen;
	int screen_width, screen_height;
	int count;
	RoadMapGuiPoint sign_bottom, sign_top;
	RoadMapImage image = NULL;
    OGLBorderImageType border_type = _border_image_none;

	screen_width = roadmap_canvas_width();
	screen_height = roadmap_canvas_height();

	if ( top == NULL )
	{
		sign_top.x = 1;
		sign_top.y = roadmap_bar_top_height();
	}
	else
	{
		sign_top.x = top->x ;
		sign_top.y = top->y ;
	}

	if ( bottom == NULL )
	{
		sign_bottom.x = screen_width - 1;
		sign_bottom.y = screen_height - roadmap_bar_bottom_height();
	}
	else{
		sign_bottom.x = bottom->x;
		sign_bottom.y = bottom->y;
	}

	/*
	 * Get the appropriate file name
	 */
	border_type = get_border_type( style, header, pointer_type );
	if ( border_type == _border_image_none )
	{
		roadmap_log( ROADMAP_ERROR, "Cannot find appropriate border for Style: %d, Header: %d, Pointer: %d, Background: %s",
				style, header, pointer_type, background );
		return FALSE;
	}

	/*
	 * Load the image
	 */
	image = roadmap_res_get( RES_BITMAP, RES_SKIN, OGLBorderImageFiles[border_type] );
	roadmap_canvas_draw_image_stretch( image, &sign_top, &sign_bottom, &OGLBorderStretchPoints[border_type], 0, IMAGE_NORMAL );

	/*
	 * Draw the pointer
	 */
	if ( pointer_type == POINTER_POSITION )
	{
		int visible = roadmap_math_point_is_visible ( position );
		if ( visible )
		{
			int count = 3;
			RoadMapGuiPoint points[3];
			RoadMapPen pointer_pen;
			roadmap_math_coordinate ( position, points );
			roadmap_math_rotate_project_coordinate ( points );

			if ( points[0].y > sign_bottom.y )
			{
				int pointer_draw_offset =   ( ( sign_bottom.x - sign_top.x ) * 100 ) >> 7; /* The offset for the pointer drawing - 80% of width */
				points[1].x = sign_top.x + pointer_draw_offset;
				points[1].y = sign_bottom.y - 3;
				points[2].x = points[1].x+42;
				points[2].y = sign_bottom.y - 3;
				pointer_pen = roadmap_canvas_create_pen( "fill_pop_up_pen" );
				roadmap_canvas_set_foreground("#000000");
				roadmap_canvas_set_opacity(181);
				count = 3;
				roadmap_canvas_draw_multiple_polygons (1, &count, points, 1, 0);
			}
		}
	}
	return TRUE;
}





