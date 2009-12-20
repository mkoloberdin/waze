/* roadmap_geocode.h - Find the possible positions of a street address.
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
 *
 * DESCRIPTION:
 *
 *   This module retrieve the possible positions of a given street address.
 *   There can be several positions returned, because the address provided
 *   may be incomplete or ambiguous.
 *
 *   The function roadmap_geocode_address() supports the following ambiguities:
 *
 *   - The street number may not be provided. In that cases, all the blocks
 *     for the given street (or streets) will be returned.
 *   - The street name may be missing the prefix (North, South, etc..) or
 *     suffixes (Street, Boulevard, North, South, etc..).
 *   - The city name may be prefixed with character '?' (such as "?dallas").
 *     In this case the city name will be used to retrieve the county, but
 *     will not be used when selecting the street later on (this is a kind
 *     of neihborough search).
 *
 *   All of these options can be used simultaneously.
 *
 *   If no position can be found, roadmap_geocode_address() returns 0 and
 *   a description of the error can be obtained by calling the function
 *   roadmap_geocode_last_error().
 */

#ifndef INCLUDE__ROADMAP_GEOCODE__H
#define INCLUDE__ROADMAP_GEOCODE__H

typedef enum tag_roadmap_geocode_error{

	geo_error_none,
	geo_error_general,
	geo_error_no_map,
	geo_error_no_address,
	geo_error_no_state,
	geo_error_no_city,
	geo_error_no_street,
	geo_error_no_house_number
	
}	roadmap_geocode_error;

typedef struct {

   int fips;
   int square;
   int line;
   char *name;
   RoadMapPosition position;

} RoadMapGeocode;


int roadmap_geocode_address (RoadMapGeocode **selections,
                             const char *number_image,
                             const char *street_name,
                             const char *city_name,
                             const char *state_name);

const char* roadmap_geocode_last_error_string(void);
roadmap_geocode_error roadmap_geocode_last_error_code (void);

#endif // INCLUDE__ROADMAP_GEOCODE__H

