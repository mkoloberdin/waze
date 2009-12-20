/* roadmap_geocode.c - Find the possible positions of a street address.
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
 * SYNOPSYS:
 *
 *   See roadmap_geocode.h
 */

#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_math.h"
#include "roadmap_config.h"
#include "roadmap_street.h"
#include "roadmap_locator.h"
#include "roadmap_trip.h"
#include "roadmap_lang.h"
#include "roadmap_preferences.h"
#include "roadmap_square.h"

#include "roadmap_geocode.h"


#define ROADMAP_MAX_STREETS  256


static const char*            RoadMapGeocodeLastErrorString = NULL;
static roadmap_geocode_error  RoadMapGeocodeLastErrorCode   = geo_error_none;


int roadmap_geocode_address (RoadMapGeocode **selections,
                             const char *number_image,
                             const char *street_name,
                             const char *city_name,
                             const char *state_name) {

   int i, j;
   int fips;
   int count;
   int number;

   RoadMapBlocks blocks[ROADMAP_MAX_STREETS];

   RoadMapGeocode *results;


   RoadMapGeocodeLastErrorString = "No error";
   RoadMapGeocodeLastErrorCode   = geo_error_none;
   *selections = NULL;

   switch (roadmap_locator_by_city (city_name, state_name)) {

   case ROADMAP_US_OK:

      count = roadmap_street_blocks_by_city
                  (street_name, city_name, blocks, ROADMAP_MAX_STREETS);
      break;

   case ROADMAP_US_NOSTATE:

      RoadMapGeocodeLastErrorString =
         roadmap_lang_get ("No state with that name could be found");
      RoadMapGeocodeLastErrorCode = geo_error_no_state;
      return 0;

   case ROADMAP_US_NOCITY:

      RoadMapGeocodeLastErrorString =
         roadmap_lang_get ("No city with that name could be found");
      RoadMapGeocodeLastErrorCode = geo_error_no_city;
      return 0;

   default:

      RoadMapGeocodeLastErrorString =
         roadmap_lang_get ("No related map could be found");
      RoadMapGeocodeLastErrorCode = geo_error_no_map;
      return 0;
   }

   if (count <= 0) {

      switch (count) {
      case ROADMAP_STREET_NOADDRESS:
         RoadMapGeocodeLastErrorString =
            roadmap_lang_get ("Address could not be found");
         RoadMapGeocodeLastErrorCode = geo_error_no_address;
         break;
      case ROADMAP_STREET_NOCITY:
         RoadMapGeocodeLastErrorString =
            roadmap_lang_get ("No city with that name could be found");
         RoadMapGeocodeLastErrorCode = geo_error_no_city;
         break;
      case ROADMAP_STREET_NOSTREET:
         RoadMapGeocodeLastErrorString =
            roadmap_lang_get ("No street with that name could be found");
         RoadMapGeocodeLastErrorCode = geo_error_no_street;
      default:
         RoadMapGeocodeLastErrorString =
            roadmap_lang_get ("The address could not be found");
         RoadMapGeocodeLastErrorCode = geo_error_no_address;
      }
      return 0;
   }

   if (count > ROADMAP_MAX_STREETS) {
      roadmap_log (ROADMAP_ERROR, "too many blocks");
      RoadMapGeocodeLastErrorCode = geo_error_general;
      count = ROADMAP_MAX_STREETS;
   }

   /* If the street number was not provided, retrieve all street blocks to
    * expand the match list, else decode that street number and update the
    * match list.
    */
    
   if (number_image[0] == 0) {
   	
   	number = -1;
   } else {
   	
      number = roadmap_math_street_address (number_image, strlen(number_image));
   } 

   results = (RoadMapGeocode *)
       calloc (count, sizeof(RoadMapGeocode));

   fips = roadmap_locator_active();

   for (i = 0, j = 0; i< count; ++i) {

      if (roadmap_street_get_position
      			(blocks + i, number, &results[j].position)) {

	      RoadMapStreetProperties properties;
	          
	      roadmap_street_get_properties (blocks[i].line, &properties);
       	results[j].fips = fips;
        	results[j].square = blocks[i].square; // roadmap_square_active ();
        	results[j].line = blocks[i].line;
        	results[j].name = strdup (roadmap_street_get_full_name (&properties));
        	j += 1;
      }
   }

   if (j <= 0) {

      free (results);
      if (number_image[0] != 0) {
         RoadMapGeocodeLastErrorString =
            roadmap_lang_get ("House number was not found in that street. Please enter a valid house number, or erase the house number entry");
         RoadMapGeocodeLastErrorCode = geo_error_no_house_number;
      } else {
         RoadMapGeocodeLastErrorString = roadmap_lang_get ("No valid street was found");
         RoadMapGeocodeLastErrorCode = geo_error_no_street;
      }

   } else {

      *selections = results;
   }

   return j;
}


const char* roadmap_geocode_last_error_string(void) {

   return RoadMapGeocodeLastErrorString;
}

roadmap_geocode_error roadmap_geocode_last_error_code(void) {

   return RoadMapGeocodeLastErrorCode;
}
