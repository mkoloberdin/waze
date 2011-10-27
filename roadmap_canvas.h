/* roadmap_canvas.h - manage the roadmap canvas that is used to draw the map.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
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
 *
 * DESCRIPTION:
 *
 *   This defines a toolkit-independent interface for the canvas used
 *   to draw the map. Even while the canvas implementation is highly
 *   toolkit-dependent, this interface is not.
 */

#ifndef INCLUDE__ROADMAP_CANVAS__H
#define INCLUDE__ROADMAP_CANVAS__H

#include "roadmap_gui.h"

#define MAX_CORDING_POINTS 2


enum { IMAGE_NORMAL,
       IMAGE_SELECTED,
       IMAGE_NOBLEND
};

struct roadmap_canvas_pen;
typedef struct roadmap_canvas_pen *RoadMapPen;

struct roadmap_canvas_image;
typedef struct roadmap_canvas_image *RoadMapImage;

typedef void (*RoadMapCanvasMouseHandler)     (RoadMapGuiPoint *point);
typedef void (*RoadMapCanvasConfigureHandler) (void);

void roadmap_canvas_register_button_pressed_handler
                    (RoadMapCanvasMouseHandler handler);

void roadmap_canvas_register_button_released_handler
                    (RoadMapCanvasMouseHandler handler);

void roadmap_canvas_register_mouse_move_handler
                    (RoadMapCanvasMouseHandler handler);

void roadmap_canvas_register_configure_handler
                    (RoadMapCanvasConfigureHandler handler);


/* This call is used to get the pixel's size of a given text (if it
 * was going to be drawn on the screen).
 * It is used to compute the positioning of things on the screen,
 * according to the current font.
 */
void roadmap_canvas_get_text_extents
        (const char *text, int size, int *width,
            int *ascent, int *descent, int *can_tilt);

/* This call is used to get the pixel's size of a given text (if it
 * was going to be drawn on the screen).
 * It is used to compute the positioning of things on the screen,
 * according to the current font.
 */
void roadmap_canvas_get_formated_text_extents
        (const char *text, int size, int *width,
            int *ascent, int *descent, int *can_tilt, int font_type);

/* This call creates a new pen. If the pen already exists,
 * it should not be re-created.
 * The new (or existing) pen becomes selected (see below).
 */
RoadMapPen roadmap_canvas_create_pen (const char *name);


/* This calls make the given pen the default context when drawing
 * on the canvas.
 */
RoadMapPen roadmap_canvas_select_pen (RoadMapPen pen);


/* Set properties for the current pen:
 * This is mostly performed once for each pen (at initialization),
 * althrough the thickness may be changed when the zoom level changes.
 */
void roadmap_canvas_set_foreground (const char *color);
void roadmap_canvas_set_background (const char *color);
void roadmap_canvas_set_thickness  (int thickness);
int  roadmap_canvas_get_thickness  (RoadMapPen pen);
void roadmap_canvas_set_opacity (int opacity);


/* The functions below draw in the selected buffer using the selected pen: */

void roadmap_canvas_erase (void);
void roadmap_canvas_erase_area (const RoadMapGuiRect *rect);

/* Draw text on the screen at a given position.
 * A text can be aligned in 4 different ways, defined by which text "corner"
 * is the position related to.
 * The idea here is to align text relative to any of the four corners of
 * the canvas: we need to align any side of the text with the canvas'
 * border.
 */
#define ROADMAP_CANVAS_LEFT        0
#define ROADMAP_CANVAS_RIGHT       1
#define ROADMAP_CANVAS_MIDDLE      8
#define ROADMAP_CANVAS_TOP         0
#define ROADMAP_CANVAS_BOTTOM      2
#define ROADMAP_CANVAS_CENTER      4
#define ROADMAP_CANVAS_TOPMIDDLE    (ROADMAP_CANVAS_TOP|ROADMAP_CANVAS_MIDDLE)
#define ROADMAP_CANVAS_TOPLEFT      (ROADMAP_CANVAS_TOP|ROADMAP_CANVAS_LEFT)
#define ROADMAP_CANVAS_TOPRIGHT     (ROADMAP_CANVAS_TOP|ROADMAP_CANVAS_RIGHT)
#define ROADMAP_CANVAS_BOTTOMRIGHT  (ROADMAP_CANVAS_BOTTOM|ROADMAP_CANVAS_RIGHT)
#define ROADMAP_CANVAS_BOTTOMLEFT   (ROADMAP_CANVAS_BOTTOM|ROADMAP_CANVAS_LEFT)
#define ROADMAP_CANVAS_BOTTOMMIDDLE (ROADMAP_CANVAS_BOTTOM|ROADMAP_CANVAS_MIDDLE)
#define ROADMAP_CANVAS_CENTERRIGHT  (ROADMAP_CANVAS_CENTER|ROADMAP_CANVAS_RIGHT)
#define ROADMAP_CANVAS_CENTERLEFT   (ROADMAP_CANVAS_CENTER|ROADMAP_CANVAS_LEFT)
#define ROADMAP_CANVAS_CENTERMIDDLE (ROADMAP_CANVAS_CENTER|ROADMAP_CANVAS_MIDDLE)

#define CANVAS_DRAW_FAST 0x1
#define CANVAS_NO_ROTATE 0x2

#define CANVAS_COPY_NORMAL 0x1
#define CANVAS_COPY_BLEND  0x2

#define FONT_TYPE_NORMAL   0x1
#define FONT_TYPE_BOLD     0x2
#define FONT_TYPE_OUTLINE  0x4

void roadmap_canvas_draw_string  (RoadMapGuiPoint *position,
                                  int corner,
                                  const char *text);

void roadmap_canvas_draw_string_size (RoadMapGuiPoint *position,
                                 int corner,
                                 int size,
                                 const char *text);

void roadmap_canvas_draw_formated_string_size (RoadMapGuiPoint *position,
                                                int corner,
                                                int size,
                                                int font_type,
                                                const char *text);

void roadmap_canvas_draw_string_angle (const RoadMapGuiPoint *position,
                                       RoadMapGuiPoint *center,
                                       int angle, int size,
                                       const char *text);

void roadmap_canvas_draw_formated_string_angle (const RoadMapGuiPoint *position,
                                                RoadMapGuiPoint *center,
                                                int angle, int size, int font_type,
                                                const char *text);

void roadmap_canvas_draw_multiple_points (int count, RoadMapGuiPoint *points);

void roadmap_canvas_draw_multiple_lines
         (int count, int *lines, RoadMapGuiPoint *points, int fast_draw);

void roadmap_canvas_draw_multiple_tex_lines (int count, int *lines, RoadMapGuiPoint *points, int fast_draw,
                                             RoadMapImage image, int opposite, int offset);

void roadmap_canvas_draw_multiple_polygons
         (int count, int *polygons, RoadMapGuiPoint *points, int filled,
                int fast_draw);

void roadmap_canvas_draw_multiple_circles
        (int count, RoadMapGuiPoint *centers, int *radius, int filled,
                int fast_draw);

void roadmap_canvas_draw_rounded_rect(RoadMapGuiPoint *bottom, RoadMapGuiPoint *top, int radius);
int roadmap_canvas_width (void);
int roadmap_canvas_height (void);

int roadmap_canvas_is_landscape();

/* This primitive causes the "exposed" drawing buffer to appear on the screen.
 */
void roadmap_canvas_refresh (void);

void roadmap_canvas_save_screenshot (const char* filename);

int  roadmap_canvas_image_width  (const RoadMapImage image);
int  roadmap_canvas_image_height (const RoadMapImage image);

RoadMapImage roadmap_canvas_load_image (const char *path,
                                        const char* file_name);

void roadmap_canvas_image_set_mutable (RoadMapImage src);

void roadmap_canvas_draw_image (RoadMapImage image, const RoadMapGuiPoint *pos,
                                int opacity, int mode);

void roadmap_canvas_draw_image_scaled( RoadMapImage image, const RoadMapGuiPoint *top_left_pos, const RoadMapGuiPoint *bottom_right_pos,
                                int opacity, int mode );

void roadmap_canvas_draw_image_stretch( RoadMapImage image, const RoadMapGuiPoint *top_left_pos, const RoadMapGuiPoint *bottom_right_pos,
											const RoadMapGuiPoint *pivot_pos, int opacity, int mode );

void roadmap_canvas_draw_image_middle_stretch( RoadMapImage image, const RoadMapGuiPoint *top_left_pos, const RoadMapGuiPoint *bottom_right_pos,
                                int opacity, int mode );

void roadmap_canvas_draw_image_scaled_angle ( RoadMapImage image, const RoadMapGuiPoint *top_left_pos, const RoadMapGuiPoint *bottom_right_pos,
                                             int opacity, int angle, int mode );

RoadMapImage roadmap_canvas_new_image (int width, int height);


void roadmap_canvas_copy_image (RoadMapImage dst_image,
                                const RoadMapGuiPoint *pos,
                                const RoadMapGuiRect  *rect,
                                RoadMapImage src_image, int mode);

void roadmap_canvas_draw_image_text (RoadMapImage image,
                                     const RoadMapGuiPoint *position,
                                     int size, const char *text);

void roadmap_canvas_draw_image_formated_text (RoadMapImage image,
                                     const RoadMapGuiPoint *position,
                                     int size, const char *text, int font_type);

RoadMapImage roadmap_canvas_image_from_buf( unsigned char* buf, int width, int height, int stride );

void roadmap_canvas_free_image (RoadMapImage image);

int roadmap_canvas_get_generic_screen_type( int width, int height );
void roadmap_canvas_image_invalidate( RoadMapImage image );
void roadmap_canvas_unmanaged_list_add( RoadMapImage image );
void roadmap_canvas_shutdown();
void roadmap_canvas_begin_draw_to_image (RoadMapImage image);
void roadmap_canvas_stop_draw_to_image (void);

#if defined(IPHONE) || defined(ANDROID)
void roadmap_canvas_get_cording_pt (RoadMapGuiPoint points[MAX_CORDING_POINTS]);
int roadmap_canvas_is_cording();
#if defined(IPHONE)
void roadmap_canvas_cancel_touches();
void roadmap_canvas_should_accept_layout (int bAcceptLayout);
#endif
#endif

#endif // INCLUDE__ROADMAP_CANVAS__H

