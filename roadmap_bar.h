/* roadmap_bar.h - Handle main bar
 *
 * LICENSE:
 *
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

#ifndef ROADMAP_BAR_H_
#define ROADMAP_BAR_H_
#include "roadmap_gui.h"
typedef const char * (*RoadMapBarTextFn) (void);

int roadmap_bar_bottom_height();
int roadmap_bar_top_height();

void roadmap_bar_initialize(void);
void roadmap_bar_draw_top_bar (BOOL draw_bg);
void roadmap_bar_draw_bottom_bar (BOOL drwa_bg);
void roadmap_bar_draw(void);
void roadmap_bar_draw_objects(void);
int roadmap_bar_short_click (RoadMapGuiPoint *point);
void roadmap_bottom_bar_hide();
void roadmap_bottom_bar_show();
void roadmap_top_bar_hide();
void roadmap_top_bar_show();
BOOL roadmap_bottom_bar_shown();
BOOL roadmap_top_bar_shown();
void roadmap_top_bar_set_clickable( BOOL val );
int roadmap_bar_top_bar_exit_state ( void );
#ifdef IPHONE_NATIVE
void roadmap_more_bar_hide();
void roadmap_more_bar_show();
int roadmap_bar_more_state();
#endif //IPHONE_NATIVE

#endif /*ROADMAP_BAR_H_*/
