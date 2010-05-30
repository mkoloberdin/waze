/* roadmap_wicnecanvas.h - manage the roadmap canvas in WinCE.
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
 *   This defines a toolkit-independent interface for the canvas used
 *   to draw the map. Even while the canvas implementation is highly
 *   toolkit-dependent, the interface must not.
 */

#ifndef INCLUDE__ROADMAP_WINCE_CANVAS__H
#define INCLUDE__ROADMAP_WINCE_CANVAS__H

/* This function is called by roadmap_window.c, a GUI toolkit
 * dependent module.
 */
HWND roadmap_canvas_new (HWND main, HWND tool_bar);
void roadmap_canvas_button_pressed(POINT *data);
void roadmap_canvas_button_released(POINT *data);
void roadmap_canvas_mouse_moved(POINT *data);

#endif // INCLUDE__ROADMAP_WINCE_CANVAS__H

