/* roadmap_label.h - Manage map labels.
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
 *
 *   This code was mostly taken from UMN Mapserver
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

#ifndef __ROADMAP_LABEL__H
#define __ROADMAP_LABEL__H

#include "roadmap.h"
#include "roadmap_gui.h"
#include "roadmap_plugin.h"


int roadmap_label_add (const RoadMapGuiPoint *point, int angle,
                       int featuresize, const PluginLine *line);

int roadmap_label_add_place (const RoadMapGuiPoint *point, int angle,
                             int featuresize_sq, const char *name);

void roadmap_label_activate (void);
int roadmap_label_initialize (void);

int roadmap_label_draw_cache (int angles);

void roadmap_label_start (void);
void roadmap_label_clear (int square);

#endif // __ROADMAP_LABEL__H
