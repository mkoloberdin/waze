/* navigate_bar.h - Navigation bar images
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
 */

#ifndef INCLUDE__NAVIGATE_BAR__H
#define INCLUDE__NAVIGATE_BAR__H

#include "roadmap_canvas.h"
#include "navigate/navigate_main.h"

void navigate_bar_initialize (void);
void navigate_bar_set_mode (int mode);
void navigate_bar_draw (void);
void navigate_bar_set_instruction (enum NavigateInstr instr);
void navigate_bar_set_exit (int exit);
void navigate_bar_set_distance (int distance);
void navigate_bar_set_speed ();
void navigate_bar_set_street (const char *street);
void navigate_bar_set_draw_offsets ( int offset_x, int offset_y );
void navigate_bar_resize( void );
BOOL navigate_bar_is_hidden(void);
void navigate_bar_set_next_instruction (enum NavigateInstr instr);
void navigate_bar_set_next_exit (int exit);
void navigate_bar_set_next_distance (int distance);
int navigate_bar_get_height( void );
#endif // INCLUDE__NAVIGATE_BAR__H

