/* roadmap_tile.c - conversion between coordinates and tile indices.
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

#include "roadmap_tile.h"
#include <stdlib.h>
 
#define	SCALE_STEP	4
#define	SQUARE_SIZE	10000 //30000
#define	MAX_SQUARE_SIZE	30000000

typedef struct {
 
	int		scale_factor;
 	int		tile_size;
 	int		base_index;
 	int		num_rows;
 	int		num_cols;
} RoadMapScaleData;
 
static RoadMapScaleData		*ScaleData = NULL;
static int						MaxScale = -1;
 
static void init_scales (void) {

	int	scale;
	int	size;
	int	factor;
	int	base;
	
	if (ScaleData != NULL) return;
	
	for (scale = 0, size = SQUARE_SIZE; 
		  size <= MAX_SQUARE_SIZE;
		  scale++, size *= SCALE_STEP)
		;
		
	MaxScale = --scale;
	ScaleData = malloc ((MaxScale + 1) * sizeof (RoadMapScaleData));
	
	base = 0;
	factor = 1;
	
	for (scale = 0; scale <= MaxScale; scale++) {
	
		ScaleData[scale].scale_factor = factor;
		ScaleData[scale].tile_size = factor * SQUARE_SIZE;
		ScaleData[scale].base_index = base;
		ScaleData[scale].num_rows = 179999999 / ScaleData[scale].tile_size + 1;
		ScaleData[scale].num_cols = 359999999 / ScaleData[scale].tile_size + 1;
		
		base += ScaleData[scale].num_rows * ScaleData[scale].num_cols;
		factor *= SCALE_STEP; 	
	}
}

 
int	roadmap_tile_get_max_scale (void) {

	init_scales ();
	return MaxScale;
}


int	roadmap_tile_get_scale_factor (int scale) {

	return ScaleData[scale].scale_factor;	
}


int	roadmap_tile_get_size (int scale) {

	return ScaleData[scale].tile_size;	
}


void	roadmap_tile_get_origin (int scale, const RoadMapPosition *position, RoadMapPosition *origin) {

	int tile_size = roadmap_tile_get_size (scale);
	
	origin->longitude = (position->longitude + 180000000) / tile_size * tile_size - 180000000;
	origin->latitude = (position->latitude + 90000000) / tile_size * tile_size - 90000000;
}


int	roadmap_tile_get_id_from_position (int scale, const RoadMapPosition *position) {

	int lon;
	int lat;
	
	roadmap_tile_get_index_from_position (scale, position, &lon, &lat);
	return roadmap_tile_get_id_from_index (scale, lon, lat);	
}


int	roadmap_tile_get_id_from_index (int scale, int lon_index, int lat_index) {
	
	return ScaleData[scale].base_index + lon_index * ScaleData[scale].num_rows + lat_index; 	
}


void	roadmap_tile_get_index_from_position (int scale, const RoadMapPosition *position, 
														  int *lon_index, int *lat_index) {
																  	
	*lon_index = (position->longitude + 180000000) / ScaleData[scale].tile_size;
	*lat_index = (position->latitude + 90000000) / ScaleData[scale].tile_size;																  	
}

int	roadmap_tile_get_square_from_index (int scale, int lon_index, int lat_index,
//														int lon_min, int lon_max, int lat_min, int lat_max,
														int *west, int *east, int *south, int *north) {
																	
	*west = lon_index * ScaleData[scale].tile_size - 180000000;
	*east = *west + ScaleData[scale].tile_size;
	*south = lat_index * ScaleData[scale].tile_size - 90000000;
	*north = *south + ScaleData[scale].tile_size;
/*	
	if (lon_min > *west) *west = lon_min;
	if (lon_max < *east) *east = lon_max;
	if (lat_min > *south) *south = lat_min;
	if (lat_max < *north) *north = lat_max;
*/	
	return 1;
}

int	roadmap_tile_get_scale (int tile) {
	
	int max = roadmap_tile_get_max_scale ();
	int i;
	
	for (i = 1; i <= max; i++) {
		if (ScaleData[i].base_index > tile)
			break;
	}
		
	return i - 1;
}

void 	roadmap_tile_edges (int tile, int *west, int *east, int *south, int *north) {

	int scale = roadmap_tile_get_scale (tile);
	int lon = (tile - ScaleData[scale].base_index) / ScaleData[scale].num_rows;
	int lat = tile - ScaleData[scale].base_index - lon * ScaleData[scale].num_rows;
	
	*west = lon * ScaleData[scale].tile_size - 180000000;
	*east = *west + ScaleData[scale].tile_size;
	*south = lat * ScaleData[scale].tile_size - 90000000;
	*north = *south + ScaleData[scale].tile_size;
}

int	roadmap_tile_is_adjacent (int tile1, int tile2) {

	int west[2], east[2], south[2], north[2];
	
	roadmap_tile_edges (tile1, west, east, south, north);
	roadmap_tile_edges (tile2, west + 1, east + 1, south + 1, north + 1);
	
	return ((south[0] == south[1] && (west[0] == east[1] || east[0] == west[1])) ||
			  (west[0] == west[1] && (south[0] == north[1] || north[0] == south[1])));	
}
