/* editor_screen.h - manage screen drawing
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
 */

#ifndef INCLUDE__EDITOR_SCREEN__H
#define INCLUDE__EDITOR_SCREEN__H

#include "../roadmap_canvas.h"
#include "../roadmap_plugin.h"

typedef struct selected_line_s {

   PluginLine line;
} SelectedLine;

void editor_screen_adjust_layer (int layer, int thickness, int pen_count);
void editor_screen_initialize (void);

int editor_screen_override_pen (int line,
                                int cfcc,
                                int fips,
                                int pen_type,
                                RoadMapPen *override_pen);

void editor_screen_set (int status);
void editor_screen_update_fips (int fips);
void editor_screen_repaint (int max_pen);
void editor_screen_reset_selected (void);
void report_accident_at_screen_point(void);
void report_accident_opposite_side_at_screen_point(void);

int editor_screen_show_candidates (void);

int editor_screen_gray_scale(void);
char *editor_screen_overide_car();
void editor_screen_set_override_car(const char *name);
#endif // INCLUDE__EDITOR_SCREEN__H

