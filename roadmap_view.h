/* roadmap_view.h
 *
 * LICENSE:
 *
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
#ifndef ROADMAP_VIEW_H_
#define ROADMAP_VIEW_H_

void roadmap_view_refresh (void);
int roadmap_view_get_orientation (void);
int roadmap_view_get_scale (void);
void roadmap_view_menu (void);
void roadmap_view_commute (void);
void roadmap_view_navigation (void);
int roadmap_view_show_labels (int cfcc, RoadMapPen *pens, int num_projs);
void roadmap_view_auto_zoom_suspend (void);
void roadmap_view_reset (void);
int roadmap_view_hold (void);

#endif /*ROADMAP_VIEW_H_*/
