/* ssd_widget.h - defines a generic widget
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
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
 */

#ifndef __SSD_WIDGET_H_
#define __SSD_WIDGET_H_

#include "../roadmap_gui.h"
#include "../roadmap_keyboard.h"
#include "../roadmap_input_type.h"

#define SSD_MIN_SIZE -1
#define SSD_MAX_SIZE -2

#define  SSD_X_SCREEN_LEFT      (-1)
#define  SSD_X_SCREEN_RIGHT     (-2)
#define  SSD_Y_SCREEN_TOP       (-1)
#define  SSD_Y_SCREEN_BOTTOM    (-2)

#define SSD_ALIGN_CENTER    0x1
#define SSD_ALIGN_RIGHT     0x2
#define SSD_START_NEW_ROW   0x4
#define SSD_END_ROW         0x8
#define SSD_ALIGN_VCENTER   0x10
#define SSD_ALIGN_GRID      0x20
#define SSD_VAR_SIZE        0x40
#define SSD_WIDGET_SPACE    0x80
#define SSD_WIDGET_HIDE     0x100
#define SSD_ALIGN_BOTTOM    0x200
#define SSD_ALIGN_LTR       0x400
#define SSD_PERSISTENT      0x800				/* This flag is generic for widgets and dialogs.
												 * defines if widget should be deallocated in the cleanup process
												 */
#define SSD_WS_TABSTOP      0x00100000
#define SSD_WS_DEFWIDGET    0x00200000
#define SSD_TAB_CONTROL     0x00400000

#define SSD_POINTER_LOCATION 0x00800000
#define SSD_POINTER_NONE	  0x01000000
#define SSD_POINTER_COMMENT  0x02000000
#define SSD_POINTER_MENU	  0x04000000
#define SSD_HEADER_BLACK     0x08000000
#define SSD_ROUNDED_BLACK    0x10000000
#define SSD_ROUNDED_WHITE    0x40000000

/* Draw flags */
#define SSD_GET_SIZE           0x1
#define SSD_GET_CONTAINER_SIZE 0x2

/* Buttons flags */
#define SSD_BUTTON_KEY           0x1000
#define SSD_BUTTON_TEXT          0x2000
#define SSD_BUTTON_TEXT_BELOW    0x4000
#define SSD_BUTTON_NO_TEXT       0x8000

/* Dialogs */
#define SSD_DIALOG_FLOAT          0x10000
#define SSD_DIALOG_TRANSPARENT    0x20000
#define SSD_DIALOG_GUI_TAB_ORDER  0x40000
#define SSD_DIALOG_VERTICAL       0x80000
#define SSD_DIALOG_NO_SCROLL      0x100000
#define SSD_DIALOG_NO_BACK        0x200000
#define SSD_DIALOG_ADDITION_BELOW 0x400000


/* Container flags */
#define SSD_CONTAINER_BORDER 	    0x1000
#define SSD_CONTAINER_TITLE  	    0x2000
#define SSD_ROUNDED_CORNERS	    0x4000
#define SSD_NO_BG_IMAGE 		    0x8000
#define SSD_CONTAINER_TXT_BOX     0x20000000
#define SSD_CONTAINER_SEARCH_BOX  0x80000000

/* Text flags */
#define SSD_TEXT_LABEL        0x1000 /* Adds a ':' sign */
#define SSD_TEXT_PASSWORD     0x2000
#define SSD_TEXT_READONLY     0x4000
#define SSD_TEXT_NORMAL_FONT  0x8000
#define SSD_TEXT_INPUT        0x10000
#define SSD_TEXT_SINGLE_LINE  0x20000

/* Context-menu flags   */
#define SSD_CONTEXTMENU_ROUNDED              0x0000
#define SSD_CONTEXTMENU_SIMPLE_LIST          0x0001
#define SSD_CONTEXTMENU_DROP_DOWN            0x0002
#define SSD_CONTEXTMENU_USE_SIZE             0x0004

#define  SSD_ROUNDED_CORNER_WIDTH            (7)
#define  SSD_ROUNDED_CORNER_HEIGHT           (7)

#define  SOFT_MENU_BAR_HEIGHT                (24)
#define  TITLE_BAR_HEIGHT                    (24)

/** TODO:: These have to be configurable because the PPI differs for various screens !!! * AGA */
#define SSD_DIALOG_OBJ_FRAME_OFFSET_X		35
#define SSD_DIALOG_OBJ_FRAME_OFFSET_Y		35

typedef enum {

   FOCUS_BACK,
   FOCUS_FORWARD,
   FOCUS_LEFT,
   FOCUS_RIGHT,
   FOCUS_UP,
   FOCUS_DOWN

} SSD_FOCUS_DIRECTION;

typedef struct ssd_size {
   short width;
   short height;
} SsdSize;

typedef struct ssd_point {
   short x;
   short y;
} SsdPoint;

typedef struct {
   int left;
   int top;
   int right;
   int bottom;
} SsdClickOffsets;

struct ssd_widget;
typedef struct ssd_widget *SsdWidget;

/* Widget callback. If it returns non zero, the new_value should not be
 * applied.
 */
typedef int (*SsdCallback) (SsdWidget widget, const char *new_value);
typedef int (*SsdSoftKeyCallback) (SsdWidget widget, const char *new_value, void *context);
typedef BOOL(*CB_OnWidgetKeyPressed) (SsdWidget widget, const char* utf8char, uint32_t flags);
typedef void(*SsdDrawCallback)(SsdWidget widget, RoadMapGuiRect *rect, int flags);

struct ssd_widget {

   char* _typeid;

   SsdWidget parent;
   SsdWidget next;
   SsdWidget children;

   const char *name;

   char *value;
   SsdSize size;
   SsdSize cached_size;

   int offset_x;
   int offset_y;

   int         flags;
   BOOL        tab_stop;        	//  Can we receive the focus?
   BOOL        default_widget;  	//  First this to receive the focus in the dialog
   BOOL        in_focus;        	//  Keyboard focus
   BOOL        focus_highlight;  	//  Keyboard focus highlight ( TRUE by default )
   BOOL        background_focus;	//  Visability-only focus (nested context menus)
   int         tab_sequence;

   SsdWidget   prev_tabstop;    //  Previous this in the tab order
   SsdWidget   next_tabstop;    //  Next     this in the tab order
   SsdWidget   tabstop_left;    //  GUI location tab-stop:   Left
   SsdWidget   tabstop_right;   //  GUI location tab-stop:   Right
   SsdWidget   tabstop_above;   //  GUI location tab-stop:   Up
   SsdWidget   tabstop_below;   //  GUI location tab-stop:   Down
   void*       parent_dialog;   //  Parent dialog

   const char *fg_color;
   const char *bg_color;

   SsdCallback callback;
   void *context;  /* References a caller-specific context. */

   RoadMapGuiPoint position;
   const char *right_softkey;
   const char *left_softkey;

   BOOL force_click; /* Force click in case of motion in the click area */
   SsdClickOffsets click_offsets;	/* Offsets added to the widget borders in order
									 * to determine that the object was clicked
									 */

   SsdSoftKeyCallback right_softkey_callback;
   SsdSoftKeyCallback left_softkey_callback;

   void *data; /* Widget specific data */

   const char * (*get_value)(SsdWidget widget);
   const void * (*get_data)(SsdWidget widget);
   int  (*set_value)       (SsdWidget widget, const char *value);
   int  (*set_data)        (SsdWidget widget, const void *value);
   void (*draw)            (SsdWidget widget, RoadMapGuiRect *rect, int flags);
   int  (*pointer_down)    (SsdWidget widget, const RoadMapGuiPoint *point);
   int  (*pointer_up)      (SsdWidget widget, const RoadMapGuiPoint *point);
   int  (*short_click)     (SsdWidget widget, const RoadMapGuiPoint *point);
   int  (*long_click)      (SsdWidget widget, const RoadMapGuiPoint *point);
   int  (*drag_start)      (SsdWidget widget, const RoadMapGuiPoint *point);
   int  (*drag_end)        (SsdWidget widget, const RoadMapGuiPoint *point);
   int  (*drag_motion)     (SsdWidget widget, const RoadMapGuiPoint *point);
   BOOL (*key_pressed)     (SsdWidget widget, const char* utf8char, uint32_t flags);
   void (*release)     	   (SsdWidget widget );
   roadmap_input_type
        (*get_input_type)  (SsdWidget widget);
};

SsdWidget ssd_widget_new (const char *name, CB_OnWidgetKeyPressed key_pressed, int flags);
SsdWidget ssd_widget_get (SsdWidget child, const char *name);
void ssd_widget_draw (SsdWidget w, const RoadMapGuiRect *rect,
                      int parent_flags);
void ssd_widget_set_callback (SsdWidget widget, SsdCallback callback);
int  ssd_widget_rtl (SsdWidget parent);

int ssd_widget_pointer_down (SsdWidget widget, const RoadMapGuiPoint *point);
int ssd_widget_pointer_up   (SsdWidget widget, const RoadMapGuiPoint *point);
int ssd_widget_short_click  (SsdWidget widget, const RoadMapGuiPoint *point);
int ssd_widget_long_click   (SsdWidget widget, const RoadMapGuiPoint *point);

void        ssd_widget_set_backgroundfocus( SsdWidget w, BOOL set);
BOOL        ssd_widget_set_focus   (SsdWidget widget);
void        ssd_widget_loose_focus (SsdWidget widget);
SsdWidget   ssd_widget_move_focus (SsdWidget w, SSD_FOCUS_DIRECTION direction);
SsdWidget   ssd_widget_get_next_focus( SsdWidget w);

SsdWidget ssd_widget_sort_tab_order   (void* parent_dialog, SsdWidget head);
SsdWidget ssd_widget_sort_gui_tab_order(SsdWidget first_tab_stop);
void      ssd_widget_reset_tab_order   (SsdWidget head);
void      ssd_widget_reset_position (SsdWidget w);
BOOL      ssd_widget_on_key_pressed (SsdWidget widget, const char* utf8char, uint32_t flags);

const char *ssd_widget_get_value (const SsdWidget widget, const char *name);
const void *ssd_widget_get_data (const SsdWidget widget, const char *name);
int ssd_widget_set_value (const SsdWidget widget, const char *name,
                          const char *value);
int ssd_widget_set_data (const SsdWidget widget, const char *name,
                          const void *value);

void        ssd_widget_add    (SsdWidget parent, SsdWidget child);
SsdWidget   ssd_widget_remove (SsdWidget parent, SsdWidget child);
SsdWidget   ssd_widget_replace(SsdWidget parent, SsdWidget old_child, SsdWidget new_child);


void ssd_widget_set_flags   (SsdWidget widget, int flags);
void ssd_widget_set_size    (SsdWidget widget, int width, int height);
void ssd_widget_set_offset  (SsdWidget widget, int x, int y);
void ssd_widget_set_context (SsdWidget widget, void *context);
void ssd_widget_get_size    (SsdWidget w, SsdSize *size, const SsdSize *max);
void ssd_widget_set_color   (SsdWidget w, const char *fg_color,
                             const char *bg_color);

void ssd_widget_container_size (SsdWidget dialog, SsdSize *size);

void *ssd_widget_get_context (SsdWidget w);

void ssd_widget_reset_cache (SsdWidget w);

void ssd_widget_hide (SsdWidget w);
void ssd_widget_show (SsdWidget w);

SsdWidget ssd_widget_find_by_pos (SsdWidget widget,
                                  const RoadMapGuiPoint *point, BOOL use_offsets );

BOOL ssd_widget_check_point_location(SsdWidget widget,
                                  const RoadMapGuiPoint *point, int frame_offset_x, int frame_ofset_y );
BOOL ssd_widget_find_clickable_by_pos (SsdWidget widget, const RoadMapGuiPoint *point,
											SsdWidget* widget_short_click, SsdWidget* widget_long_click );


int ssd_widget_set_left_softkey_text(SsdWidget widget, const char *value);
int ssd_widget_set_right_softkey_text(SsdWidget widget, const char *value);
void ssd_widget_set_right_softkey_callback (SsdWidget widget, SsdSoftKeyCallback callback);
void ssd_widget_set_left_softkey_callback (SsdWidget widget, SsdSoftKeyCallback callback);
int ssd_widget_pointer_down_force_click (SsdWidget widget, const RoadMapGuiPoint *point);
int ssd_widget_pointer_up_force_click (SsdWidget widget, const RoadMapGuiPoint *point);
void ssd_widget_set_pointer_force_click( SsdWidget widget );
void ssd_widget_set_click_offsets( SsdWidget widget, const SsdClickOffsets* offsets );
void ssd_widget_set_click_offsets_ext( SsdWidget widget, int left, int top, int right, int bottom );
BOOL ssd_widget_contains_point(  SsdWidget widget, const RoadMapGuiPoint *point, BOOL use_offsets );
void ssd_widget_set_focus_highlight( SsdWidget widget, BOOL is_highlight );
void ssd_widget_free( SsdWidget widget, BOOL force, BOOL update_parent );
SsdWidget ssd_widget_set_recalculate( BOOL value );
#endif // __SSD_WIDGET_H_
