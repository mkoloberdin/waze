/* roadmap_pointer.h - Manage mouse/pointer events
 *
 * LICENSE:
 *
 *   Copyright 2005 Ehud Shabtai
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

#ifndef INCLUDE__ROADMAP_POINTER__H
#define INCLUDE__ROADMAP_POINTER__H

#include "roadmap_gui.h"

#define POINTER_DEFAULT 0
#define POINTER_NORMAL  1
#define POINTER_HIGH    2
#define POINTER_HIGHEST 3

typedef int (*RoadMapPointerHandler) (RoadMapGuiPoint *point);
enum POINTER_EVENT {SHORT_CLICK = 0, LONG_CLICK, DOUBLE_CLICK, PRESSED, RELEASED, DRAG_START, DRAG_MOTION, DRAG_END,KEY_BOARD_PRESS, MAX_EVENTS};
void roadmap_pointer_initialize (void);
const RoadMapGuiPoint *roadmap_pointer_position (void);

void roadmap_pointer_register_short_click (RoadMapPointerHandler handler,
                                           int priority);
void roadmap_pointer_register_long_click  (RoadMapPointerHandler handler,
                                           int priority);
void roadmap_pointer_register_double_click(RoadMapPointerHandler handler,
                                           int priority);
void roadmap_pointer_register_drag_start  (RoadMapPointerHandler handler,
                                           int priority);
void roadmap_pointer_register_drag_motion (RoadMapPointerHandler handler,
                                           int priority);
void roadmap_pointer_register_drag_end    (RoadMapPointerHandler handler,
                                           int priority);
void roadmap_pointer_register_pressed     (RoadMapPointerHandler handler,
                                           int priority);
void roadmap_pointer_register_released    (RoadMapPointerHandler handler,
                                           int priority);
                                           
void roadmap_pointer_register_enter_key_press    (RoadMapPointerHandler handler,
                                                  int priority);                                           

void roadmap_pointer_unregister_short_click (RoadMapPointerHandler handler);
void roadmap_pointer_unregister_long_click  (RoadMapPointerHandler handler);
void roadmap_pointer_unregister_double_click  (RoadMapPointerHandler handler);
void roadmap_pointer_unregister_drag_start  (RoadMapPointerHandler handler);
void roadmap_pointer_unregister_drag_motion (RoadMapPointerHandler handler);
void roadmap_pointer_unregister_drag_end    (RoadMapPointerHandler handler);
void roadmap_pointer_unregister_pressed     (RoadMapPointerHandler handler);
void roadmap_pointer_unregister_released    (RoadMapPointerHandler handler);
void roadmap_pointer_cancel_dragging( void );
int roadmap_pointer_force_click(int event,RoadMapGuiPoint *point);
int roadmap_pointer_long_click_expired( void );
void roadmap_pointer_enable_double_click( void );
BOOL roadmap_pointer_is_down( void );

int roadmap_pointer_screen_state(void);

#endif // INCLUDE__ROADMAP_POINTER__H

