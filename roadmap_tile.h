/* roadmap_tile.h - conversion between coordinates and tile indices.
 *
 * LICENSE:
 *
 *   Copyright 2008 Israel Disatnik
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
#ifndef ROADMAP_TILE_H_
#define ROADMAP_TILE_H_

#include "roadmap_types.h"

int	roadmap_tile_get_max_scale (void);

int	roadmap_tile_get_scale_factor (int scale);

int	roadmap_tile_get_size (int scale);

void	roadmap_tile_get_origin (int scale, const RoadMapPosition *position, RoadMapPosition *origin);

int	roadmap_tile_get_id_from_position (int scale, const RoadMapPosition *position);

int	roadmap_tile_get_id_from_index (int scale, int lon_index, int lat_index);

void	roadmap_tile_get_index_from_position (int scale, const RoadMapPosition *position, 
														  int *lon_index, int *lat_index);

int	roadmap_tile_get_square_from_index (int scale, int lon_index, int lat_index,
//														int lon_min, int lon_max, int lat_min, int lat_max,
														int *west, int *east, int *south, int *north);

int	roadmap_tile_get_scale (int tile);

void 	roadmap_tile_edges (int tile, int *west, int *east, int *south, int *north);

int	roadmap_tile_is_adjacent (int tile1, int tile2);

#endif /*ROADMAP_TILE_H_*/
