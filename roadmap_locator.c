/* roadmap_locator.c - Locate the map to which a specific place belongs.
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
 *  int roadmap_locator_by_position
 *         (RoadMapPosition *position, int *fips, int count);
 *  int  roadmap_locator_by_city     (const char *city, const char *state);
 *  int  roadmap_locator_activate    (int fips);
 *
 * These functions are used to retrieve which map the given entity belongs to.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "roadmap.h"
#include "roadmap_config.h"
#include "roadmap_types.h"
#include "roadmap_dbread.h"
#include "roadmap_tile_model.h"
#include "roadmap_county_model.h"
#include "roadmap_dictionary.h"
#include "roadmap_point.h"
#include "roadmap_square.h"
#include "roadmap_shape.h"
#include "roadmap_alert.h"
#include "roadmap_turns.h"
#include "roadmap_line.h"
#include "roadmap_line_route.h"
#include "roadmap_line_speed.h"
#include "roadmap_street.h"
#include "roadmap_polygon.h"
#include "roadmap_county.h"
#include "roadmap_path.h"
#include "roadmap_db_square.h"
#include "roadmap_tile_manager.h"
#include "roadmap_gzm.h"

#include "roadmap_locator.h"


#define ROADMAP_CACHE_SIZE 8

struct roadmap_cache_entry {

   int          fips;
   unsigned int last_access;
   int			 gzm_id;
};

static struct roadmap_cache_entry *RoadMapCountyCache = NULL;

static int RoadMapCountyCacheSize = 0;

static int RoadMapActiveCounty = -2;
static int RoadMapActiveMap = -1;

#if 0
static int RoadMapUsdirActive = 0;
#endif

//static roadmap_db_model *RoadMapUsModel;
static roadmap_db_model *RoadMapCountyModel;
static roadmap_db_model *RoadMapTileModel;

//static RoadMapDictionary RoadMapUsCityDictionary = NULL;
//static RoadMapDictionary RoadMapUsStateDictionary = NULL;


static int roadmap_locator_no_download (int fips) {return 0;}

static RoadMapInstaller  RoadMapDownload = roadmap_locator_no_download;

static RoadMapConfigDescriptor RoadMapConfigStaticCounty =
                        ROADMAP_CONFIG_ITEM("Map", "Static County");

static void roadmap_locator_configure (void) {

   roadmap_config_declare
       ("preferences", &RoadMapConfigStaticCounty, "0", NULL);

   if (RoadMapCountyCache == NULL) {

		roadmap_db_sector sector_global = {model__county_global_first, model__county_global_last};
		roadmap_db_sector sector_square = {model__tile_square_first, model__tile_square_last};
		
      RoadMapCountyModel =
         roadmap_db_register
            (RoadMapCountyModel, &sector_global, &RoadMapSquareHandler);
/*
      RoadMapUsModel =
         roadmap_db_register
            (RoadMapUsModel, "county", &RoadMapCountyHandler);
*/
      RoadMapTileModel =
         roadmap_db_register
            (RoadMapTileModel, &sector_square, &RoadMapSquareOneHandler);

      RoadMapCountyCacheSize = roadmap_option_cache ();
      if (RoadMapCountyCacheSize < ROADMAP_CACHE_SIZE) {
         RoadMapCountyCacheSize = ROADMAP_CACHE_SIZE;
      }
      RoadMapCountyCache = (struct roadmap_cache_entry *)
         calloc (RoadMapCountyCacheSize, sizeof(struct roadmap_cache_entry));
      roadmap_check_allocated (RoadMapCountyCache);
   }

   if (roadmap_config_get_integer(&RoadMapConfigStaticCounty) != 0) return;
/*
   if (!RoadMapUsdirActive) {
      if (! roadmap_db_open ("usdir", NULL, RoadMapUsModel, "r")) {
         roadmap_log (ROADMAP_ERROR, "cannot open directory database (usdir)");
         return;
      }

      RoadMapUsCityDictionary   = roadmap_dictionary_open ("city");
      RoadMapUsStateDictionary  = roadmap_dictionary_open ("state");

      RoadMapUsdirActive = 1;
   }
*/
}


static void roadmap_locator_remove (int index) {

   RoadMapCountyCache[index].fips = 0;
   RoadMapCountyCache[index].last_access = 0;
   if (RoadMapCountyCache[index].gzm_id >= 0) {
   	roadmap_gzm_close (RoadMapCountyCache[index].gzm_id);
   }
}


static unsigned int roadmap_locator_new_access (void) {

   static unsigned int RoadMapLocatorAccessCount = 0;

   int i;


   RoadMapLocatorAccessCount += 1;

   if (RoadMapLocatorAccessCount == 0) { /* Rollover. */

      for (i = 0; i < RoadMapCountyCacheSize; i++) {

         if (RoadMapCountyCache[i].fips > 0) {
            roadmap_locator_remove (i);
         }
      }
      RoadMapActiveCounty = 0;
      RoadMapActiveMap = -1;

      RoadMapLocatorAccessCount += 1;
   }

   return RoadMapLocatorAccessCount;
}


static int roadmap_locator_open (int fips) {

   int i;
   int access;
   int oldest = 0;
   char gzm_name[256];

   if (fips <= 0) {
      return ROADMAP_US_NOMAP;
   }
   
   if (fips == RoadMapActiveCounty) {
   	return ROADMAP_US_OK;
   }
   
   /* Look for the oldest entry in the cache. */

   for (i = RoadMapCountyCacheSize-1; i >= 0; --i) {

      if (RoadMapCountyCache[i].fips == fips) {

         roadmap_db_activate (fips, -1);
         RoadMapActiveCounty = fips;
         RoadMapActiveMap = RoadMapCountyCache[i].gzm_id;

         return ROADMAP_US_OK;
      }

      if (RoadMapCountyCache[i].last_access
             < RoadMapCountyCache[oldest].last_access) {

         oldest = i;
      }
   }

   if (RoadMapCountyCache[oldest].fips > 0) {
       roadmap_locator_remove (oldest);
       if (RoadMapCountyCache[oldest].fips == RoadMapActiveCounty) {
           RoadMapActiveCounty = 0;
           RoadMapActiveMap = -1;
       }
   }

   access = roadmap_locator_new_access ();

   while (! roadmap_db_open (fips, -1, RoadMapCountyModel, "r")) {

      if (! RoadMapDownload (fips)) {
         return ROADMAP_US_NOMAP;
      }
   }

   RoadMapCountyCache[oldest].fips = fips;
   RoadMapCountyCache[oldest].last_access = access;

	snprintf (gzm_name, sizeof(gzm_name), "map%05d%s", fips, ROADMAP_GZM_TYPE);
	RoadMapCountyCache[oldest].gzm_id = roadmap_gzm_open (gzm_name);
	
   RoadMapActiveCounty = fips;
   RoadMapActiveMap = RoadMapCountyCache[oldest].gzm_id;
   
   roadmap_square_load_index ();
   return ROADMAP_US_OK;
}


int roadmap_locator_refresh (int fips) {

	int n_tiles;
	int i_tile;
	int rc;
	
	rc = roadmap_locator_open (fips);
	if (rc != ROADMAP_US_OK) {
		return rc;
	}
	
	n_tiles = roadmap_map_get_num_tiles (RoadMapActiveMap);
	for (i_tile = 0; i_tile < n_tiles; i_tile++) {
		int tile_id = roadmap_map_get_tile_id (RoadMapActiveMap, i_tile);
		roadmap_db_remove (fips, tile_id);
	}

   return ROADMAP_US_OK;
}


void roadmap_locator_close (int fips) {

   int i;

   for (i = RoadMapCountyCacheSize-1; i >= 0; --i) {

      if (RoadMapCountyCache[i].fips == fips) {
         roadmap_locator_remove (i);
      }
   }

   if (RoadMapActiveCounty == fips) {
      RoadMapActiveCounty = 0;
      RoadMapActiveMap = -1;
   }
}


#if 0
void roadmap_locator_close_dir (void) {

   if (roadmap_config_get_integer(&RoadMapConfigStaticCounty) != 0) return;

   if (RoadMapUsdirActive) {
      roadmap_db_close ("usdir");
      RoadMapUsdirActive = 0;
   }
}
#endif


static int roadmap_locator_allocate (int **fips) {

   int count;

   roadmap_locator_configure();

   if (roadmap_config_get_integer(&RoadMapConfigStaticCounty) != 0) {
      count = 1;
   } else {
      count = roadmap_county_count();
   }

   if (*fips == NULL) {
      *fips = calloc (count, sizeof(int));
      roadmap_check_allocated(*fips);
   }

   return count;
}


void roadmap_locator_declare (RoadMapInstaller download) {

   if (download == NULL) {
       RoadMapDownload = roadmap_locator_no_download;
   } else {
       RoadMapDownload = download;
   }
}


int roadmap_locator_by_position
        (const RoadMapPosition *position, int **fips) {

   int count;

   count = roadmap_locator_allocate (fips);

   return roadmap_county_by_position (position, *fips, count);
}

/*
int roadmap_locator_by_state (const char *state_symbol, int **fips) {

   int count;
   RoadMapString state;

   count = roadmap_locator_allocate (fips);

   if (RoadMapUsStateDictionary == NULL) state = 0;
   else state =
         roadmap_dictionary_locate (RoadMapUsStateDictionary, state_symbol);
   if (state == ROADMAP_INVALID_STRING) {
       return 0;
   }
   return roadmap_county_by_state (state, *fips, count);
}
*/

int roadmap_locator_by_city (const char *city_name, const char *state_symbol) {

//   RoadMapString city;
//   RoadMapString state;

   roadmap_locator_configure();

   if (roadmap_config_get_integer(&RoadMapConfigStaticCounty) != 0) {
      return roadmap_locator_open
                (roadmap_config_get_integer(&RoadMapConfigStaticCounty));
   }

/*
   state = roadmap_dictionary_locate (RoadMapUsStateDictionary, state_symbol);
   if (state == ROADMAP_INVALID_STRING) {
      return ROADMAP_US_NOSTATE;
   }

   while (city_name[0] == '?') {
      ++city_name;
      while (city_name[0] == ' ') ++city_name;
   }
   city = roadmap_dictionary_locate (RoadMapUsCityDictionary, city_name);
   if (city == ROADMAP_INVALID_STRING) {
      return ROADMAP_US_NOCITY;
   }

   return roadmap_locator_open (roadmap_county_by_city (city, state));
*/
   return ROADMAP_US_NOSTATE;
}


int roadmap_locator_activate (int fips) {

   if (RoadMapActiveCounty == fips) {
       return ROADMAP_US_OK;
   }

   roadmap_locator_configure();

   return roadmap_locator_open (fips);
}


int roadmap_locator_active (void) {
    return RoadMapActiveCounty;
}

/*
RoadMapString roadmap_locator_get_state (const char *state) {

   return roadmap_dictionary_locate (RoadMapUsStateDictionary, state);
}
*/

int roadmap_locator_search_city (const char *str, RoadMapDictionaryCB cb,
                                 void *data) {

   if (roadmap_config_get_integer(&RoadMapConfigStaticCounty) != 0) {
      return roadmap_street_search_city (str, cb, data);
   }
/*
   return roadmap_dictionary_search_all
            (RoadMapUsCityDictionary, NULL, str, 1, cb, data);
*/
   return ROADMAP_US_NOCITY;
}


int roadmap_locator_static_county (void) {

   return roadmap_config_get_integer(&RoadMapConfigStaticCounty);
}


int roadmap_locator_load_tile (int index) {
	
   int rc;

   if (RoadMapActiveCounty <= 0) {
      return ROADMAP_US_NOMAP;
   }
   
   rc = roadmap_db_open (RoadMapActiveCounty, index, RoadMapTileModel, "r");
   
   if (! rc) {

		if (RoadMapActiveMap >= 0) {
			void *data;
			int length;
			
			if (0 == roadmap_gzm_get_section (RoadMapActiveMap, index, &data, &length)) {
   			rc = roadmap_db_open_mem (RoadMapActiveCounty, index, RoadMapTileModel, data, length);
			}
		}
		
		if (!rc)
      	return ROADMAP_US_NOMAP;
   }
   
   return ROADMAP_US_OK;
}


int roadmap_locator_load_tile_mem (int index, void *data, size_t size) {
	
   int rc;

   if (RoadMapActiveCounty <= 0) {
      return ROADMAP_US_NOMAP;
   }
   
   rc = roadmap_db_open_mem (RoadMapActiveCounty, index, RoadMapTileModel, data, size);
   
   if (! rc) {

      return ROADMAP_US_NOMAP;
   }
   
   return ROADMAP_US_OK;
}


int roadmap_locator_unload_tile (int index) {
	
   if (RoadMapActiveCounty <= 0) {
      return 0;
   }

   return roadmap_db_close (RoadMapActiveCounty, index);
}
