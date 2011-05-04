/* roadmap_screen.h - draw the map on the screen.
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
 */

#ifndef INCLUDE__ROADMAP_SCREEN__H
#define INCLUDE__ROADMAP_SCREEN__H

#include "roadmap_types.h"
#include "roadmap_canvas.h"
#include "roadmap.h"
enum { VIEW_MODE_2D = 0,
       VIEW_MODE_3D
};

enum { ORIENTATION_DYNAMIC = 0,
       ORIENTATION_FIXED
};


/* The base height for which all the absolute values for height values are adjusted */
#define RM_SCREEN_BASE_HEIGHT 	480
/* The base width for which all the absolute values for width values are adjusted */
#define RM_SCREEN_BASE_WIDTH 	320

#define RM_SCREEN_CAR_ANIMATION 1000

void roadmap_screen_initialize (void);
void roadmap_screen_shutdown   (void);

void roadmap_screen_set_initial_position (void);

void roadmap_screen_zoom_in    (void);
void roadmap_screen_zoom_out   (void);
void roadmap_screen_zoom_reset (void);

void roadmap_screen_move       (int dx, int dy);
void roadmap_screen_move_up    (void);
void roadmap_screen_move_down  (void);
void roadmap_screen_move_right (void);
void roadmap_screen_move_left  (void);

void roadmap_screen_move_center (int dy);

void roadmap_screen_toggle_view_mode (void);
void roadmap_screen_set_view(int view_mode);
void roadmap_screen_restore_view(void);
void roadmap_screen_toggle_orientation_mode (void);
void roadmap_screen_set_orientation_fixed (void);
void roadmap_screen_set_orientation_dynamic (void);
void roadmap_screen_increase_horizon (void);
void roadmap_screen_decrease_horizon (void);
int roadmap_screen_get_orientation_mode (void);
int roadmap_screen_get_view_mode (void);
int roadmap_screen_get_nonogl_view_mode (void);

void roadmap_screen_rotate (int delta);

int  roadmap_screen_refresh (void); /* Conditional: only if needed. */
void roadmap_screen_redraw  (void); /* Force a screen redraw, no move. */

void roadmap_screen_hold     (void); /* Hold on at the current position. */
void roadmap_screen_freeze   (void); /* Forbid any screen refresh. */
void roadmap_screen_unfreeze (void); /* Enable screen refresh. */

void roadmap_screen_update_center (const RoadMapPosition *pos);


void roadmap_screen_get_center (RoadMapPosition *center);

typedef void (*RoadMapScreenSubscriber) (void);

RoadMapScreenSubscriber roadmap_screen_subscribe_after_refresh
                                    (RoadMapScreenSubscriber handler);

RoadMapScreenSubscriber roadmap_screen_subscribe_after_flow_control_refresh
                                    (RoadMapScreenSubscriber handler);

int  roadmap_screen_draw_one_line (RoadMapPosition *from,
                                   RoadMapPosition *to,
                                   int fully_visible,
                                   RoadMapPosition *first_shape_pos,
                                   int first_shape,
                                   int last_shape,
                                   RoadMapShapeItr shape_itr,
                                   RoadMapPen *pens,
                                   int num_pens,
                                   int label_min_proj,
                                   int *total_length,
                                   RoadMapGuiPoint *middle,
                                   int *angle);

int roadmap_screen_draw_one_tex_line (RoadMapPosition *from,
                                      RoadMapPosition *to,
                                      int fully_visible,
                                      RoadMapPosition *first_shape_pos,
                                      int first_shape,
                                      int last_shape,
                                      RoadMapShapeItr shape_itr,
                                      RoadMapPen *pens,
                                      int num_pens,
                                      int label_max_proj,
                                      int *total_length_ptr,
                                      RoadMapGuiPoint *middle,
                                      int *angle,
                                      RoadMapImage image,
                                      BOOL oposite);

void roadmap_screen_draw_line_direction (RoadMapPosition *from,
                                         RoadMapPosition *to,
                                         RoadMapPosition *first_shape_pos,
                                         int first_shape,
                                         int last_shape,
                                         RoadMapShapeItr shape_itr,
                                         int width,
                                         int direction,
                                         const char *color);

int roadmap_screen_fast_refresh (void);

void show_me_on_map(void);
void focus_on_me (void);
int  is_screen_wide (void);

void roadmap_screen_add_focus_on_me_softkey(void);

int roadmap_screen_height (void);

int roadmap_screen_touched_state(void);
int roadmap_screen_not_touched_state(void);

void roadmap_screen_touched(void);
void roadmap_screen_touched_off(void);

int roadmap_screen_is_ld_screen( void );
int roadmap_screen_is_hd_screen( void );
void roadmap_screen_set_screen_type( int width, int height );
int roadmap_screen_get_screen_type( void );
int roadmap_screen_get_screen_scale( void );
int roadmap_screen_adjust_height( int orig_height );
int roadmap_screen_adjust_width( int orig_width );


void roadmap_screen_mark_redraw (void);
int roadmap_screen_show_icons_only_when_touched(void);
int roadmap_screen_show_top_bar_only_when_touched(void);
#define DBG_TIME_FULL 0
#define DBG_TIME_DRAW_SQUARE 1
#define DBG_TIME_DRAW_ONE_LINE 2
#define DBG_TIME_SELECT_PEN 3
#define DBG_TIME_DRAW_LINES 4
#define DBG_TIME_CREATE_PATH 5
#define DBG_TIME_ADD_PATH 6
#define DBG_TIME_FLIP 7
#define DBG_TIME_TEXT_FULL 8
#define DBG_TIME_TEXT_CNV 9
#define DBG_TIME_TEXT_LOAD 10
#define DBG_TIME_TEXT_ONE_LETTER 11
#define DBG_TIME_TEXT_GET_GLYPH 12
#define DBG_TIME_TEXT_ONE_RAS 13
#define DBG_TIME_DRAW_LONG_LINES 14
#define DBG_TIME_FIND_LONG_LINES 15
#define DBG_TIME_ADD_SEGMENT 16
#define DBG_TIME_FLUSH_LINES 17
#define DBG_TIME_FLUSH_POINTS 18
#define DBG_TIME_T1 19
#define DBG_TIME_T2 20
#define DBG_TIME_T3 21
#define DBG_TIME_T4 22
#define DBG_TIME_LAST_COUNTER 23

void dbg_time_start(int type);
void dbg_time_end(int type);
int dbg_time_print();
void roadmap_screen_draw_flush(void);
int roadmap_screen_show_top_bar(void);
void roadmap_screen_set_Xicon_state(BOOL state);
BOOL roadmap_screen_is_xicon_open();

void roadmap_screen_set_background_run( BOOL value );
BOOL roadmap_screen_get_background_run( void );

void roadmap_screen_shade_bg(void);
void roadmap_screen_flush_lines_and_points(void);

void roadmap_screen_set_animating( BOOL value );
void roadmap_screen_set_scale (long scale, int use_map_units);
void roadmap_screen_update_center_animated (const RoadMapPosition *pos, int duration, BOOL linear);
void roadmap_screen_set_cording_rotation_enabled( BOOL value );
#define isViewModeAny3D()	\
((RoadMapScreenViewMode == VIEW_MODE_3D) || (RoadMapScreenOGLViewMode == VIEW_MODE_3D))

void roadmap_screen_update_center_animated (const RoadMapPosition *pos, int duration, BOOL linear);
void roadmap_screen_start_glow (RoadMapPosition *position, int max_duraiton, RoadMapGuiPoint *offset);
void roadmap_screen_stop_glow (void);

#define ADJ_SCALE(_size) ( _size * roadmap_screen_get_screen_scale()/100 )
BOOL roadmap_screen_is_any_dlg_active();
#endif // INCLUDE__ROADMAP_SCREEN__H
