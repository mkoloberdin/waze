/* roadmap_county_model.h - definitions for county data format.
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

#ifndef _ROADMAP_COUNTY_MODEL__H_
#define _ROADMAP_COUNTY_MODEL__H_

enum {

	model__county_global_first,
	
	model__county_global_data				= model__county_global_first,
	model__county_global_scale,
	model__county_global_grid,
	model__county_global_bitmask,
	
	model__county_global_last				= model__county_global_bitmask,
	
	
	model__county
};


#endif // _ROADMAP_COUNTY_MODEL__H_

