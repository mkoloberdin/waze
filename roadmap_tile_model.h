/* roadmap_tile_model.h - definitions for tile data format.
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

#ifndef _ROADMAP_TILE_MODEL__H_
#define _ROADMAP_TILE_MODEL__H_

enum {

	model__tile_square_first			= 0,
	
	model__tile_string_first			= model__tile_square_first,
	model__tile_string_prefix			= model__tile_string_first,
	model__tile_string_street,
	model__tile_string_text2speech,
	model__tile_string_type,
	model__tile_string_suffix,
	model__tile_string_city,
	model__tile_string_attributes,
	model__tile_string_landmark,
	model__tile_string_last				= model__tile_string_landmark,
	
	model__tile_shape_first,
	model__tile_shape_data				= model__tile_shape_first,
	model__tile_shape_last				= model__tile_shape_data,
	
	model__tile_line_first,
	model__tile_line_data				= model__tile_line_first,
	model__tile_line_bysquare1,
	model__tile_line_broken,
	model__tile_line_roundabout,
	model__tile_line_last				= model__tile_line_roundabout,
	
	model__tile_point_first,
	model__tile_point_data				= model__tile_point_first,
	model__tile_point_id,
	model__tile_point_last				= model__tile_point_id,

	model__tile_line_route_first,
	model__tile_line_route_data		= model__tile_line_route_first,
	model__tile_line_route_last		= model__tile_line_route_data,
	
	model__tile_street_first,
	model__tile_street_name				= model__tile_street_first,
	model__tile_street_city,
	model__tile_street_last				= model__tile_street_city,
	
	model__tile_polygon_first,
	model__tile_polygon_head			= model__tile_polygon_first,
	model__tile_polygon_point,
	model__tile_polygon_last			= model__tile_polygon_point,
	
	model__tile_line_speed_first,
	model__tile_line_speed_line_ref	= model__tile_line_speed_first,
	model__tile_line_speed_avg,
	model__tile_line_speed_index,
	model__tile_line_speed_data,
	model__tile_line_speed_last		= model__tile_line_speed_data,
	
	model__tile_range_first,
	model__tile_range_addr				= model__tile_range_first,
	model__tile_range_last				= model__tile_range_addr,
	
	model__tile_alert_first,
	model__tile_alert_data				= model__tile_alert_first,
	model__tile_alert_last				= model__tile_alert_data,
	
	model__tile_square_data,
	model__tile_square_last				= model__tile_square_data,
	
	model__tile_metadata_first,
	model__tile_metadata_attributes	= model__tile_metadata_first,
	model__tile_metadata_last			= model__tile_metadata_attributes,
	
	
	
	model__tile
};


#endif // _ROADMAP_TILE_MODEL__H_

