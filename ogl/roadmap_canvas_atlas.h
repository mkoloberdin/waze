/* roadmap_canvas_atlas.h - manage the textures atlas.
 *
 * LICENSE:
 *
 *   Copyright 2010 Avi R.
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
 *   toolkit-dependent, this interface is not.
 */

#ifndef INCLUDE__ROADMAP_CANVAS_ATLAS__H
#define INCLUDE__ROADMAP_CANVAS_ATLAS__H


#define CANVAS_ATLAS_TEX_SIZE    256 //TODO: better make this dynamic

BOOL roadmap_canvas_atlas_insert (const char* hint, RoadMapImage *image);
void roadmap_canvas_atlas_clean (const char* hint);


#endif // INCLUDE__ROADMAP_CANVAS_ATLAS__H

