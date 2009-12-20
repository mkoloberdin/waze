/* roadmap_square.c - Manage a county area, divided in small squares.
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
 *   See roadmap_square.h.
 *
 * These functions are used to retrieve the squares that make the county
 * area. A special square (ROADMAP_SQUARE_GLOBAL) is used to describe the
 * global county area (vs. a piece of it).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define DECLARE_ROADMAP_SQUARE

#include "roadmap.h"
#include "roadmap_math.h"
#include "roadmap_dbread.h"
#include "roadmap_tile_model.h"
#include "roadmap_county_model.h"
#include "roadmap_tile.h"
#include "roadmap_db_square.h"
#include "roadmap_point.h"
#include "roadmap_shape.h"
#include "roadmap_line.h"
#include "roadmap_line_route.h"
#include "roadmap_street.h"
#include "roadmap_polygon.h"
#include "roadmap_line_speed.h"
#include "roadmap_dictionary.h"
#include "roadmap_city.h"
#include "roadmap_range.h"
#include "roadmap_locator.h"
#include "roadmap_path.h"
#include "roadmap_alert.h"
#include "roadmap_metadata.h"
#include "roadmap_hash.h"
#include "roadmap_tile_manager.h"
#include "roadmap_tile_status.h"

#include "roadmap_square.h"

static char *RoadMapSquareType = "RoadMapSquareContext";

typedef struct {
   roadmap_db_sector    sector;
   roadmap_db_handler   *handler;
} RoadMapSquareSubHandler;

static RoadMapSquareSubHandler SquareHandlers[] = {
   { {model__tile_string_first,model__tile_string_last}, &RoadMapDictionaryHandler },
   { {model__tile_shape_first,model__tile_shape_last}, &RoadMapShapeHandler },
   { {model__tile_line_first,model__tile_line_last}, &RoadMapLineHandler },
   { {model__tile_point_first,model__tile_point_last}, &RoadMapPointHandler },
   { {model__tile_line_route_first,model__tile_line_route_last}, &RoadMapLineRouteHandler },
   { {model__tile_street_first,model__tile_street_last}, &RoadMapStreetHandler },
   { {model__tile_polygon_first,model__tile_polygon_last}, &RoadMapPolygonHandler },
   { {model__tile_line_speed_first,model__tile_line_speed_last}, &RoadMapLineSpeedHandler },
   { {model__tile_range_first,model__tile_range_last}, &RoadMapRangeHandler },
   { {model__tile_alert_first,model__tile_alert_last}, &RoadMapAlertHandler },
	{ {model__tile_metadata_first, model__tile_metadata_last}, &RoadMapMetadataHandler }
};

#define NUM_SUB_HANDLERS ((int) (sizeof (SquareHandlers) / sizeof (SquareHandlers[0])))


typedef struct {
   RoadMapSquare        *square;
   void                 *subs[NUM_SUB_HANDLERS];
   int						attributes;
	RoadMapArea 			edges;
} RoadMapSquareData;


#ifdef J2ME
#define ROADMAP_SQUARE_CACHE_SIZE	64
#else
#define ROADMAP_SQUARE_CACHE_SIZE	512
#endif

#define ROADMAP_SQUARE_UNAVAILABLE	((RoadMapSquareData *)-1)
#define ROADMAP_SQUARE_NOT_LOADED	NULL

typedef struct {
	int	square;
	int	next;
	int	prev;
} SquareCacheNode;

typedef struct RoadMapSquareContext_t {

   char *type;

   RoadMapGlobal     *SquareGlobal;
   RoadMapSquareData **Square;

	SquareCacheNode	SquareCache[ROADMAP_SQUARE_CACHE_SIZE + 1];
	RoadMapHash			*SquareHash;
} RoadMapSquareContext;


RoadMapSquareContext *RoadMapSquareActive = NULL;

int RoadMapScaleCurrent = 0;
int RoadMapSquareCurrent = -1;
static int RoadMapSquareCurrentSlot = -1;
static int RoadMapSquareNextAvailableSlot = ROADMAP_SQUARE_CACHE_SIZE-1;

static int RoadMapSquareForceUpdateMode = 0;

static void *roadmap_square_map (const roadmap_db_data_file *file) {

   RoadMapSquareContext *context;

   int i;

	roadmap_city_init ();

   context = malloc(sizeof(RoadMapSquareContext));
   roadmap_check_allocated(context);

	RoadMapSquareActive = context;

   context->type = RoadMapSquareType;

   if (!roadmap_db_get_data (file,
   								  model__county_global_data,
   								  sizeof (RoadMapGlobal),
   								  (void**)&(context->SquareGlobal),
   								  NULL)) {
      roadmap_log (ROADMAP_FATAL, "invalid global/data structure");
   }

   context->Square = calloc (ROADMAP_SQUARE_CACHE_SIZE, sizeof (RoadMapSquareData *));
   roadmap_check_allocated(context->Square);

	for (i = 0; i <= ROADMAP_SQUARE_CACHE_SIZE; i++) {
		context->SquareCache[i].square = -1;
		context->SquareCache[i].next = (i + 1) % (ROADMAP_SQUARE_CACHE_SIZE + 1);
		context->SquareCache[i].prev = (i + ROADMAP_SQUARE_CACHE_SIZE) % (ROADMAP_SQUARE_CACHE_SIZE + 1);
	}

	context->SquareHash = roadmap_hash_new ("tiles", ROADMAP_SQUARE_CACHE_SIZE);

   RoadMapSquareCurrent = -1;

   RoadMapSquareActive = NULL;
   return context;
}

static void roadmap_square_activate (void *context) {

   RoadMapSquareContext *square_context = (RoadMapSquareContext *) context;

   if ((square_context != NULL) &&
       (square_context->type != RoadMapSquareType)) {
      roadmap_log(ROADMAP_FATAL, "cannot unmap (bad context type)");
   }
   RoadMapSquareActive = square_context;
   RoadMapSquareCurrent = -1;
}

static void roadmap_square_unmap (void *context) {

   RoadMapSquareContext *square_context = (RoadMapSquareContext *) context;

   if (square_context->type != RoadMapSquareType) {
      roadmap_log(ROADMAP_FATAL, "cannot unmap (bad context type)");
   }

   roadmap_city_write_file (roadmap_db_map_path(), "city_index", 0);
   roadmap_city_free ();

   roadmap_square_unload_all ();

   if (RoadMapSquareActive == square_context) {
      RoadMapSquareActive = NULL;
   }

   roadmap_hash_free (square_context->SquareHash);
   free (square_context->Square);
   free (square_context);
}

roadmap_db_handler RoadMapSquareHandler = {
   "global",
   roadmap_square_map,
   roadmap_square_activate,
   roadmap_square_unmap
};

time_t	roadmap_square_global_timestamp (void) {

	if (RoadMapSquareActive == NULL) return 0;

	return RoadMapSquareActive->SquareGlobal->timestamp;
}


#define TIMESTAMP_SAFETY_MARGIN (3 * 60 * 60)
time_t	roadmap_square_timestamp (int square) {

	if (RoadMapSquareActive == NULL) return 0;

	if (roadmap_square_set_current (square)) {
		return RoadMapSquareActive->Square[RoadMapSquareCurrentSlot]->square->timestamp + TIMESTAMP_SAFETY_MARGIN;
	}
	return 0;
}


int	roadmap_square_version (int square) {

	if (RoadMapSquareActive == NULL) return 0;

	if (roadmap_square_set_current (square)) {
		return RoadMapSquareActive->Square[RoadMapSquareCurrentSlot]->square->timestamp;
	}
	return 0;
}


#if 0 // currently not used
static int roadmap_square_distance_score (int square1, int square2) {

	RoadMapSquare *s1 = RoadMapSquareActive->Square[square1]->square;
	RoadMapSquare *s2 = RoadMapSquareActive->Square[square2]->square;
	int coeff1 = 256 / RoadMapSquareActive->SquareScale[s1->scale_index].scale_factor;
	int coeff2 = 256 / RoadMapSquareActive->SquareScale[s2->scale_index].scale_factor;

	return (s1->lon_index * coeff1 - s2->lon_index * coeff2) * (s1->lon_index * coeff1 - s2->lon_index * coeff2) +
			 (s1->lat_index * coeff1 - s2->lat_index * coeff2) * (s1->lat_index * coeff1 - s2->lat_index * coeff2) +
			 (coeff1 - coeff2) * (coeff1 - coeff2);
}
#endif

static void roadmap_square_unload (int slot) {

	if (RoadMapSquareActive->Square[slot] != ROADMAP_SQUARE_NOT_LOADED) {

		RoadMapSquare *s = RoadMapSquareActive->Square[slot]->square;
		roadmap_locator_unload_tile (s->square_id);
	}

}


static int roadmap_square_find (int square) {

	int slot = roadmap_hash_get_first (RoadMapSquareActive->SquareHash, square);

	while (slot >= 0) {

		if (RoadMapSquareActive->SquareCache[slot].square == square)
			return slot;

		slot = roadmap_hash_get_next (RoadMapSquareActive->SquareHash, slot);
	}

	return -1;
}


static void roadmap_square_promote (int slot) {

	SquareCacheNode *cache = RoadMapSquareActive->SquareCache;

	if (cache[ROADMAP_SQUARE_CACHE_SIZE].next != slot) {
		cache[cache[slot].next].prev = cache[slot].prev;
		cache[cache[slot].prev].next = cache[slot].next;

		cache[slot].next = cache[ROADMAP_SQUARE_CACHE_SIZE].next;
		cache[slot].prev = ROADMAP_SQUARE_CACHE_SIZE;

		cache[cache[ROADMAP_SQUARE_CACHE_SIZE].next].prev = slot;
		cache[ROADMAP_SQUARE_CACHE_SIZE].next = slot;
	}
}


void roadmap_square_unload_all (void) {

	int i;

	for (i = 1; i <= ROADMAP_SQUARE_CACHE_SIZE; i++) {

		if (RoadMapSquareActive->SquareCache[i].square >= 0) {
			roadmap_square_unload (i);
			RoadMapSquareActive->SquareCache[i].square = -1;
		}
		RoadMapSquareActive->SquareCache[i].next = (i + 1) % (ROADMAP_SQUARE_CACHE_SIZE + 1);
		RoadMapSquareActive->SquareCache[i].prev = (i + ROADMAP_SQUARE_CACHE_SIZE) % (ROADMAP_SQUARE_CACHE_SIZE + 1);
	}
}


static int roadmap_square_cache (int square) {

	SquareCacheNode *node;
	SquareCacheNode *cache = RoadMapSquareActive->SquareCache;
	int slot = RoadMapSquareNextAvailableSlot;

	if ( slot < 0 )
	{
		slot = cache[ROADMAP_SQUARE_CACHE_SIZE].prev;
	}
	else
	{
		/*
		 * At the end arrives to -1 - no more slots available for filling - replace is necessary
		 */
		RoadMapSquareNextAvailableSlot--;
	}

	// make sure not to unload the current tile
	if ( slot == RoadMapSquareCurrentSlot )
	{
		slot = cache[slot].prev;
	}

	node = cache + slot;
	//printf ("Putting square %d in slot %d\n", square, slot);
	if ( node->square >= 0 )	// Over checking - if RoadMapSquareNextAvailableSlot > 0 - there are still unfilled slots available
	{
		roadmap_square_unload (slot);
	}

	node->square = square;
	return slot;
}




//static int TotalSquares = 0;
static void *roadmap_square_map_one (const roadmap_db_data_file *file) {

   RoadMapSquareData *context;

   int j;
   int index;
   int slot;

   context = malloc(sizeof(RoadMapSquareData));
   roadmap_check_allocated(context);

   if (!roadmap_db_get_data (file,
   								  model__tile_square_data,
   								  sizeof (RoadMapSquare),
   								  (void**)&(context->square),
   								  NULL)) {
      roadmap_log (ROADMAP_FATAL, "invalid square/data structure");
   }

	index = context->square->square_id;
	roadmap_tile_edges (index,
							  &context->edges.west,
							  &context->edges.east,
							  &context->edges.south,
							  &context->edges.north);

	RoadMapSquareCurrent = index;
    slot = roadmap_square_cache (index);
	RoadMapSquareActive->Square[slot] = context;
	RoadMapSquareCurrentSlot = slot;

	//printf ("roadmap_square_map_one: slot %d tile %d\n", RoadMapSquareCurrentSlot, RoadMapSquareCurrent);

	roadmap_hash_add (RoadMapSquareActive->SquareHash, index, slot);


   for (j = 0; j < NUM_SUB_HANDLERS; j++) {
   	if (roadmap_db_exists (file, &(SquareHandlers[j].sector))) {

         context->subs[j] = SquareHandlers[j].handler->map(file);
			SquareHandlers[j].handler->activate (context->subs[j]);
      } else {
         context->subs[j] = NULL;
      }
   }

	context->attributes = 0;

   //printf ("Loaded square %d, total squares = %d\n", index, ++TotalSquares);
   return context;
}

static void roadmap_square_activate_one (void *context) {

}

void roadmap_square_delete_reference (int square) {

	int slot = roadmap_square_find (square);

	if (slot >= 0) {

		roadmap_hash_remove (RoadMapSquareActive->SquareHash, square, slot);
		RoadMapSquareActive->Square[slot] = ROADMAP_SQUARE_NOT_LOADED;
	}
}


static void roadmap_square_unmap_one (void *context) {

   RoadMapSquareData *square_data = (RoadMapSquareData *) context;
   int j;

//   if (square_context->type != RoadMapSquareType) {
//      roadmap_log(ROADMAP_FATAL, "cannot unmap (bad context type)");
//   }

   for (j = 0; j < NUM_SUB_HANDLERS; j++) {
      if (square_data->subs[j]) {
         SquareHandlers[j].handler->unmap (square_data->subs[j]);
      }
   }

	roadmap_square_delete_reference (square_data->square->square_id);

   //printf ("Unloaded square %d, total squares = %d\n", index, --TotalSquares);
   free(square_data);
}

roadmap_db_handler RoadMapSquareOneHandler = {
   "square",
   roadmap_square_map_one,
   roadmap_square_activate_one,
   roadmap_square_unmap_one
};


int roadmap_square_set_attribute (int square, int attribute) {

	int slot = roadmap_square_find (square);

	if (slot < 0) return 0;

	RoadMapSquareActive->Square[slot]->attributes |= attribute;
	return 1;
}


int roadmap_square_reset_attribute (int square, int attribute) {

	int slot = roadmap_square_find (square);

	if (slot < 0) return 0;

	RoadMapSquareActive->Square[slot]->attributes &= ~attribute;
	return 1;
}


int roadmap_square_get_attribute (int square, int attribute) {

	int slot = roadmap_square_find (square);

	if (slot < 0) return 0;

	return RoadMapSquareActive->Square[slot]->attributes & attribute;
}


static int roadmap_square_location (const RoadMapPosition *position, int scale_index) {

	return roadmap_tile_get_id_from_position (scale_index, position);
}


static void roadmap_square_request (int square, int priority, int force_update) {

	int slot = roadmap_square_find (square);

	if (slot < 0) {
		roadmap_tile_request (square, priority, force_update, NULL);
	}
}


void roadmap_square_request_location (const RoadMapPosition *position) {

	static int last_requested[5] = {-1, -1, -1, -1, -1};
	int tile_size;
	int square;
	RoadMapPosition neighbour;

	square = roadmap_square_location (position, 0);

	tile_size = roadmap_tile_get_size (0);
	if (square != last_requested[0]) {
		roadmap_square_request (square, ROADMAP_TILE_STATUS_PRIORITY_GPS, 0); // original position
		last_requested[0] = square;
	}

	neighbour = *position;

	neighbour.longitude += tile_size / 4;
	neighbour.latitude += tile_size / 4;
	square = roadmap_square_location (&neighbour, 0);
	if (square != last_requested[1]) {
		roadmap_square_request (square, ROADMAP_TILE_STATUS_PRIORITY_NEIGHBOURS, 0); // north-east
		last_requested[1] = square;
	}

	neighbour.longitude -= tile_size / 2;
	square = roadmap_square_location (&neighbour, 0);
	if (square != last_requested[2]) {
		roadmap_square_request (square, ROADMAP_TILE_STATUS_PRIORITY_NEIGHBOURS, 0); // north-west
		last_requested[2] = square;
	}

	neighbour.latitude -= tile_size / 2;
	square = roadmap_square_location (&neighbour, 0);
	if (square != last_requested[3]) {
		roadmap_square_request (square, ROADMAP_TILE_STATUS_PRIORITY_NEIGHBOURS, 0); // south-west
		last_requested[3] = square;
	}

	neighbour.longitude += tile_size / 2;
	square = roadmap_square_location (&neighbour, 0);
	if (square != last_requested[4]) {
		roadmap_square_request (square, ROADMAP_TILE_STATUS_PRIORITY_NEIGHBOURS, 0); // south-east
		last_requested[4] = square;
	}
}


int roadmap_square_search (const RoadMapPosition *position, int scale_index) {

   int square;
   int slot;
   int scale = scale_index;

   if (RoadMapSquareActive == NULL) return ROADMAP_SQUARE_OTHER;

	if (scale == -1) scale = RoadMapScaleCurrent;
   square = roadmap_square_location (position, scale);
   slot = roadmap_square_find (square);

	if (slot < 0) {

		if (roadmap_square_set_current (square)) {
			slot = roadmap_square_find (square);
		}
	}

   if (slot < 0) {
      return ROADMAP_SQUARE_GLOBAL;
   }

   return square;
}


int roadmap_square_find_neighbours (const RoadMapPosition *position, int scale_index, int squares[9]) {

	int					count = 0;
	RoadMapPosition	cross;
	int					square;
	int					step;
	RoadMapPosition	origin;

	if (scale_index < 0) {
		scale_index = RoadMapScaleCurrent;
	}

	step = roadmap_tile_get_size (scale_index);
	roadmap_tile_get_origin (RoadMapScaleCurrent, position, &origin);

	// check same square
	square = roadmap_square_search (&origin, scale_index);
	if (square != 	ROADMAP_SQUARE_GLOBAL) squares[count++] = square;

	// check square to the south
	cross.longitude = position->longitude;
	cross.latitude = origin.latitude;
	if (roadmap_math_point_is_visible (&cross)) {
		cross.longitude = origin.longitude;
		cross.latitude = origin.latitude - step;
		square = roadmap_square_search (&cross, scale_index);
		if (square != 	ROADMAP_SQUARE_GLOBAL) squares[count++] = square;
	}

	// check square to the north
	cross.longitude = position->longitude;
	cross.latitude = origin.latitude + step;
	if (roadmap_math_point_is_visible (&cross)) {
		cross.longitude = origin.longitude;
		square = roadmap_square_search (&cross, scale_index);
		if (square != 	ROADMAP_SQUARE_GLOBAL) squares[count++] = square;
	}

	// check square to the east
	cross.longitude = origin.longitude;
	cross.latitude = position->latitude;
	if (roadmap_math_point_is_visible (&cross)) {
		cross.longitude = origin.longitude - step;
		cross.latitude = origin.latitude;
		square = roadmap_square_search (&cross, scale_index);
		if (square != 	ROADMAP_SQUARE_GLOBAL) squares[count++] = square;
	}

	// check square to the west
	cross.longitude = origin.longitude + step;
	cross.latitude = position->latitude;
	if (roadmap_math_point_is_visible (&cross)) {
		cross.latitude = origin.latitude;
		square = roadmap_square_search (&cross, scale_index);
		if (square != 	ROADMAP_SQUARE_GLOBAL) squares[count++] = square;
	}

	// check square to the south-east
	cross.longitude = origin.longitude;
	cross.latitude = origin.latitude;
	if (roadmap_math_point_is_visible (&cross)) {
		cross.longitude = origin.longitude - step;
		cross.latitude = origin.latitude - step;
		square = roadmap_square_search (&cross, scale_index);
		if (square != 	ROADMAP_SQUARE_GLOBAL) squares[count++] = square;
	}

	// check square to the south-west
	cross.longitude = origin.longitude + step;
	cross.latitude = origin.latitude;
	if (roadmap_math_point_is_visible (&cross)) {
		cross.latitude = origin.latitude - step;
		square = roadmap_square_search (&cross, scale_index);
		if (square != 	ROADMAP_SQUARE_GLOBAL) squares[count++] = square;
	}

	// check square to the north-east
	cross.longitude = origin.longitude;
	cross.latitude = origin.latitude + step;
	if (roadmap_math_point_is_visible (&cross)) {
		cross.longitude = origin.longitude - step;
		square = roadmap_square_search (&cross, scale_index);
		if (square != 	ROADMAP_SQUARE_GLOBAL) squares[count++] = square;
	}

	// check square to the north-west
	cross.longitude = origin.longitude + step;
	cross.latitude = origin.latitude + step;
	if (roadmap_math_point_is_visible (&cross)) {
		square = roadmap_square_search (&cross, scale_index);
		if (square != 	ROADMAP_SQUARE_GLOBAL) squares[count++] = square;
	}

	return count;
}


void  roadmap_square_min (int square, RoadMapPosition *position) {

   if (RoadMapSquareActive == NULL) return;

	roadmap_square_set_current (square);

   position->longitude = RoadMapSquareActive->Square[RoadMapSquareCurrentSlot]->edges.west;
   position->latitude  = RoadMapSquareActive->Square[RoadMapSquareCurrentSlot]->edges.south;
}


void  roadmap_square_edges (int square, RoadMapArea *edges) {

	int slot;

   edges->west = 0;
   edges->east = 0;
   edges->north = 0;
   edges->south = 0;

   if (RoadMapSquareActive == NULL) return;

	slot = roadmap_square_find (square);

	if (slot < 0) return;

   *edges = RoadMapSquareActive->Square[slot]->edges;
}


int   roadmap_square_cross_pos (RoadMapPosition *position) {

	/* return the equivalent edge on the next square */
   int scale = roadmap_square_current_scale_factor ();
   RoadMapArea *edges = &RoadMapSquareActive->Square[RoadMapSquareCurrentSlot]->edges;

   if (position->latitude < edges->south + scale) {
      position->latitude -= scale;
      return ROADMAP_DIRECTION_NORTH;
   }
   if (position->latitude > edges->north - scale) {
      position->latitude += scale;
      return ROADMAP_DIRECTION_SOUTH;
   }
   if (position->longitude < edges->west + scale) {
      position->longitude -= scale;
      return ROADMAP_DIRECTION_EAST;
   }
   if (position->longitude > edges->east - scale) {
      position->longitude += scale;
      return ROADMAP_DIRECTION_WEST;
   }

   return -1;
}

static void roadmap_square_get_tiles (RoadMapArea *area, int min_scale) {

   RoadMapPosition position;
   RoadMapPosition corner;
   RoadMapPosition origin;
   int scale;
   int step;
   int max_scale = roadmap_tile_get_max_scale ();

   corner.longitude = area->west;
   corner.latitude = area->south;

   if (max_scale > min_scale + 1) {
   	max_scale = min_scale + 1;
   }

   for (scale = min_scale; scale <= max_scale; scale++) {
		roadmap_tile_get_origin (scale, &corner, &origin);
   	step = roadmap_tile_get_size (scale);

   	for (position.longitude = origin.longitude;
   		  position.longitude <= area->east;
   		  position.longitude += step) {
   		for (position.latitude = origin.latitude;
   			  position.latitude <= area->north;
   			  position.latitude += step) {

   			roadmap_tile_request (roadmap_tile_get_id_from_position (scale, &position), ROADMAP_TILE_STATUS_PRIORITY_NONE, 0, NULL);
   		}
   	}
   }
}


void roadmap_square_force_next_update (void) {

	RoadMapSquareForceUpdateMode = 1;
}


int roadmap_square_view (int *square, int size) {

   RoadMapPosition origin;
   RoadMapPosition position;

   RoadMapArea screen;
   RoadMapArea	peripheral;
   int count;
   int index;
	int step;
	int slot;

   if (RoadMapSquareActive == NULL) return 0;

   roadmap_math_screen_edges (&screen);

	position.longitude = screen.west;
	position.latitude = screen.south;

	roadmap_tile_get_origin (RoadMapScaleCurrent, &position, &origin);
	step = roadmap_tile_get_size (RoadMapScaleCurrent);
	count = 0;

	peripheral.west = (screen.west * 9 - screen.east) / 8;
	peripheral.east = (screen.east * 9 - screen.west) / 8;
	peripheral.south = (screen.south * 9 - screen.north) / 8;
	peripheral.north = (screen.north * 9 - screen.south) / 8;

	for (position.longitude = origin.longitude; position.longitude < screen.east; position.longitude += step) {
		for (position.latitude = origin.latitude; position.latitude <= screen.north; position.latitude += step) {

			index = roadmap_tile_get_id_from_position (RoadMapScaleCurrent, &position);
			slot = roadmap_square_find (index);

			if (slot < 0) {
				roadmap_tile_request (index, ROADMAP_TILE_STATUS_PRIORITY_ON_SCREEN, 0, NULL);
				if (roadmap_square_set_current (index)) {
					slot = roadmap_square_find (index);
				}
			}

         if (slot >= 0) {

				if (RoadMapSquareForceUpdateMode ||
					 ((*roadmap_tile_status_get (index)) & ROADMAP_TILE_STATUS_FLAG_ROUTE)) {
					// force new version of route tiles when on screen
					roadmap_tile_request (index, ROADMAP_TILE_STATUS_PRIORITY_ON_SCREEN, 1, NULL);
				}
				if (count < size) {
            	square[count] = index;
				}
            count += 1;
            if (size > 0 && count > size) {
               roadmap_log (ROADMAP_ERROR,
                            "too many square are visible: %d is not enough",
                            size);
            }
         }
      }
   }

#ifndef J2ME
	roadmap_square_get_tiles (&peripheral, RoadMapScaleCurrent);
#endif
	RoadMapSquareForceUpdateMode = 0;
   return count;
}


int roadmap_square_first_point (int square) {

   roadmap_square_set_current (square);
   return 0;
}


int roadmap_square_points_count (int square) {

   roadmap_square_set_current (square);

   return roadmap_point_count ();
}

int roadmap_square_has_shapes (int square) {

   roadmap_square_set_current (square);

   return roadmap_shape_count ();
}


int roadmap_square_first_shape (int square) {

   roadmap_square_set_current (square);
   return 0;
}


static int roadmap_square_load (int square) {

	return roadmap_locator_load_tile (square);
}


void roadmap_square_load_index (void) {

   /* temporary - force load all hi-res tiles */
//	int i;
	int rc;

   rc = roadmap_city_read_file ("city_index");
	if (!rc) return;
/*
	for (i = RoadMapSquareActive->SquareScale[0].count_latitude * RoadMapSquareActive->SquareScale[0].count_longitude - 1;
			i >= 0; i--) {

		if (RoadMapSquareActive->Square[i] != 	ROADMAP_SQUARE_UNAVAILABLE) {
			roadmap_square_set_current (i);
			roadmap_street_update_city_index ();
		}
	}
*/
}


void roadmap_square_rebuild_index (void) {

   roadmap_file_remove(roadmap_db_map_path(), "city_index");
   roadmap_square_load_index();
   roadmap_city_write_file (roadmap_db_map_path(), "city_index", 0);
}


int roadmap_square_set_current_internal (int square) {

   int j;
   int slot;

   slot = roadmap_square_find (square);
   if (slot < 0) {

		int res;
		int *status = roadmap_tile_status_get (square);

		if (status != NULL) {

			if ((*status) & ROADMAP_TILE_STATUS_FLAG_CHECKED) {
				if (!((*status) & ROADMAP_TILE_STATUS_FLAG_EXISTS)) {
					return 0;
				}
			}
			*status = (*status) | ROADMAP_TILE_STATUS_FLAG_CHECKED;
		}

		res = roadmap_square_load (square);

		switch (res) {
		case ROADMAP_US_OK:

			slot = roadmap_square_find (square);
			if (status != NULL) {
				*status = (*status) | ROADMAP_TILE_STATUS_FLAG_EXISTS;
			}
			break;

		case ROADMAP_US_NOMAP:

			slot = -1;
			break;

		case ROADMAP_US_INPROGRESS:
			//TODO
			slot = -1;
			break;

		default:
			roadmap_log (ROADMAP_FATAL, "Invalid status %d from roadmap_square_load (%08x)", status, square);
		}
	}

	if (slot >= 0) {
		roadmap_square_promote (slot);

		assert (RoadMapSquareActive->Square[slot]->square->square_id == square);
		RoadMapSquareCurrentSlot = slot;
		RoadMapSquareCurrent = square;


      for (j = 0; j < NUM_SUB_HANDLERS; j++) {
         SquareHandlers[j].handler->activate (RoadMapSquareActive->Square[slot]->subs[j]);
      }
	} else {
		return 0;
	}

	return 1;
}

void	roadmap_square_adjust_scale (int zoom_factor) {

	int scale;
	int max_scale;

	max_scale = roadmap_tile_get_max_scale ();
	for (scale = 1;
		  scale <= max_scale &&
		  roadmap_tile_get_scale_factor (scale) <= zoom_factor;
		  scale++)
		  ;

	roadmap_square_set_screen_scale (scale - 1);
}


void	roadmap_square_set_screen_scale (int scale) {

	if (scale < 0) {
		scale = 0;
	}

	if (scale > roadmap_tile_get_max_scale ()) {
		scale = roadmap_tile_get_max_scale ();
	}

	RoadMapScaleCurrent = scale;
}



int	roadmap_square_get_num_scales (void) {

	return roadmap_tile_get_max_scale () + 1;
}


int roadmap_square_screen_scale_factor (void) {

	if (!RoadMapSquareActive) {
		return 1;
	}

	return roadmap_tile_get_scale_factor (RoadMapScaleCurrent);
}


int roadmap_square_current_scale_factor (void) {

	if (!RoadMapSquareActive) {
		return 1;
	}

	return roadmap_tile_get_scale_factor (RoadMapSquareActive->Square[RoadMapSquareCurrentSlot]->square->scale);
}


int	roadmap_square_scale (int square) {

	return roadmap_tile_get_scale (square);
}


int roadmap_square_at_current_scale (int square) {

	return roadmap_square_scale (square) == RoadMapScaleCurrent;
}

