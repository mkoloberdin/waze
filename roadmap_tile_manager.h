/* roadmap_tile_manager.h - Manage loading of map tiles.
 *
 * LICENSE:
 *
 *   Copyright 2009 Israel Disatnik.
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

#ifndef _ROADMAP_TILE_MANAGER__H
#define _ROADMAP_TILE_MANAGER__H

#include "roadmap.h"

typedef void (*RoadMapTileCallback) (int tile_id);

RoadMapTileCallback roadmap_tile_register_callback (RoadMapTileCallback cb);
void roadmap_tile_request (int index, int priority, int force_update, RoadMapCallback on_loaded);
void roadmap_tile_reset_session (void);

#endif // _ROADMAP_TILE_MANAGER__H
