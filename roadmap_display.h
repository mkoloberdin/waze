/* roadmap_display.h - Manage screen signs.
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

#ifndef INCLUDE__ROADMAP_DISPLAY__H
#define INCLUDE__ROADMAP_DISPLAY__H

#include "roadmap_plugin.h"

void roadmap_display_initialize (void);

void roadmap_display_page (const char *name);

int roadmap_display_activate
        (const char *title,
         const PluginLine *line,
         const RoadMapPosition *position,
         PluginStreet *street);

void roadmap_display_update_points
        (const char *title,
         RoadMapPosition *from,
         RoadMapPosition *to);
         
int roadmap_display_pop_up
        (const char *title,
         const char *image,
         const RoadMapPosition *position,
         const char *format, ...) ;

int roadmap_activate_image_sign(const char *title,
         				   		const char *image);
         				   		
void roadmap_display_text (const char *title, const char *format, ...);
int  roadmap_display_is_sign_active (const char *title);

void roadmap_display_show (const char *title);
void roadmap_display_hide (const char *title);

void roadmap_display_signs (void);

const char *roadmap_display_get_id (const char *title);

#endif // INCLUDE__ROADMAP_DISPLAY__H
