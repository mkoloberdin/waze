/* roadmap_canvas_tile.h - manage the canvas tiles
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

#ifndef INCLUDE__ROADMAP_CANVAS_TILE__H
#define INCLUDE__ROADMAP_CANVAS_TILE__H

#include "roadmap_gui.h"
#include "roadmap_math.h"

typedef void (* RoadMapCanvasTileCb) (RoadMapGuiRect *rect);
typedef void (* RoadMapCanvasTileOnDeleteCb) (int tile_id);

void roadmap_canvas_tile_set (int layer_id, zoom_t zoom, RoadMapGuiPointF *new_point, int orientation, int is_3d, RoadMapCanvasTileCb callback, int refresh);
int roadmap_canvas_tile_draw (int layer_id, int fast_refresh);
int roadmap_canvas_tile_cached (int layer_id, RoadMapGuiPoint *point, zoom_t zoom);
void roadmap_canvas_tile_reset_all(int layer_id, int dirty);
void roadmap_canvas_tile_set_horizon (int horizon,int is_projection);
void roadmap_canvas_tile_set_target_zoom (int layer_id, zoom_t zoom);
void roadmap_canvas_tile_register_on_delete (RoadMapCanvasTileOnDeleteCb callback);
int roadmap_canvas_tile_get_id (void);
void roadmap_canvas_tile_free_all (void);


#endif // INCLUDE__ROADMAP_CANVAS_TILE__H

