/* roadmap_street.c - Handle streets operations and attributes.
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
 *   see roadmap_street.h.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "roadmap_db_street.h"
#include "roadmap_db_range.h"

#include "roadmap.h"
#include "roadmap_dbread.h"
#include "roadmap_tile_model.h"

#include "roadmap_locator.h"

#include "roadmap_math.h"
#include "roadmap_square.h"
#include "roadmap_street.h"
#include "roadmap_shape.h"
#include "roadmap_point.h"
#include "roadmap_line.h"
#include "roadmap_layer.h"
#include "roadmap_dictionary.h"
#include "roadmap_city.h"
#include "roadmap_line_route.h"
#include "roadmap_string.h"
#include "roadmap_tile.h"

static char *RoadMapStreetType = "RoadMapStreetContext";
//static char *RoadMapZipType    = "RoadMapZipContext";

typedef struct {

   RoadMapString prefix;
   int city;

} StreetSearchContext;

typedef struct {

   char *type;

   RoadMapStreet        *RoadMapStreets;
   int                   RoadMapStreetsCount;

	RoadMapCity			*RoadMapCities;
	int					 RoadMapCitiesCount;

	RoadMapDictionary RoadMapCityNames;
	RoadMapDictionary RoadMapStreetPrefix;
	RoadMapDictionary RoadMapStreetNames;
	RoadMapDictionary RoadMapText2Speech;
	RoadMapDictionary RoadMapStreetType;
	RoadMapDictionary RoadMapStreetSuffix;
} RoadMapStreetContext;

typedef struct {

	int							rc;
	RoadMapNeighbour			*result;
	int							cfcc;
	const RoadMapPosition 	*position;
} RoadMapStreetSearch;


static RoadMapStreetContext *RoadMapStreetActive = NULL;

#define MAX_SEARCH_NAMES 100
static int RoadMapStreetSearchCount;
static char *RoadMapStreetSearchNames[MAX_SEARCH_NAMES];

enum t2s_types_tag {
   STREET_EXTENDED_TYPE_SHIELD_TEXT = 0,
   STREET_EXTENDED_TYPE_SHIELD_TYPE
};

static const char *roadmap_street_get_street_name_from_id (int street);

static void *roadmap_street_map (const roadmap_db_data_file *file) {

   RoadMapStreetContext *context;

   context = malloc (sizeof(RoadMapStreetContext));
   roadmap_check_allocated(context);

   context->type = RoadMapStreetType;
   context->RoadMapStreetPrefix = NULL;
   context->RoadMapStreetNames  = NULL;
   context->RoadMapText2Speech  = NULL;
   context->RoadMapStreetType   = NULL;
   context->RoadMapStreetSuffix = NULL;
   context->RoadMapCityNames    = NULL;

   if (!roadmap_db_get_data (file,
   								  model__tile_street_name,
   								  sizeof (RoadMapStreet),
   								  (void**)&(context->RoadMapStreets),
   								  &(context->RoadMapStreetsCount))) {
      roadmap_log (ROADMAP_FATAL, "invalid street/name structure");
   }

   if (!roadmap_db_get_data (file,
   								  model__tile_street_city,
   								  sizeof (RoadMapCity),
   								  (void**)&(context->RoadMapCities),
   								  &(context->RoadMapCitiesCount))) {
      roadmap_log (ROADMAP_FATAL, "invalid street/city structure");
   }
   if (context->RoadMapCitiesCount > 0)
   	context->RoadMapCitiesCount--;

   return context;
}

static void roadmap_street_activate (void *context) {

   RoadMapStreetContext *this = (RoadMapStreetContext *) context;

   if ((this != NULL) && (this->type != RoadMapStreetType)) {
      roadmap_log (ROADMAP_FATAL, "cannot activate (bad context type)");
   }

   if (this->RoadMapStreetPrefix == NULL) {
	  this->RoadMapStreetPrefix = roadmap_dictionary_open ("prefix");
   }
   if (this->RoadMapStreetNames == NULL) {
	  this->RoadMapStreetNames = roadmap_dictionary_open ("street");
   }
   if (this->RoadMapText2Speech == NULL) {
	  this->RoadMapText2Speech = roadmap_dictionary_open ("text2speech");
   }
   if (this->RoadMapStreetType == NULL) {
	  this->RoadMapStreetType = roadmap_dictionary_open ("type");
   }
   if (this->RoadMapStreetSuffix == NULL) {
	  this->RoadMapStreetSuffix = roadmap_dictionary_open ("suffix");
   }
   if (this->RoadMapCityNames == NULL) {
	  this->RoadMapCityNames = roadmap_dictionary_open ("city");
   }
   if (this->RoadMapStreetNames  == NULL ||
		 this->RoadMapStreetPrefix == NULL ||
		 this->RoadMapStreetType   == NULL ||
		 this->RoadMapStreetSuffix == NULL ||
		 this->RoadMapCityNames    == NULL) {
	  roadmap_log (ROADMAP_FATAL, "cannot open dictionary");
   }

   RoadMapStreetActive = this;
}

static void roadmap_street_unmap (void *context) {

   RoadMapStreetContext *this = (RoadMapStreetContext *) context;

   if (this->type != RoadMapStreetType) {
      roadmap_log (ROADMAP_FATAL, "cannot unmap (bad context type)");
   }
   if (RoadMapStreetActive == this) {
      RoadMapStreetActive = NULL;
   }
   free (this);
}

roadmap_db_handler RoadMapStreetHandler = {
   "street",
   roadmap_street_map,
   roadmap_street_activate,
   roadmap_street_unmap
};




typedef struct roadmap_street_identifier {

   RoadMapString prefix;
   RoadMapString name;
   RoadMapString suffix;
   RoadMapString type;

} RoadMapStreetIdentifier;

#if 0
static int roadmap_street_city_from_id(int city_id)
{
	int i;

	if (!RoadMapStreetActive) {
		return -1;
	}

	 //todo: binary search
	for (i = 0; i < RoadMapStreetActive->RoadMapCitiesCount; i++) {
		 if (RoadMapStreetActive->RoadMapCities[i].city == city_id) {
		 	return i;
		 }
	}

	return -1;
}
#endif

static void roadmap_street_locate (const char *name,
                                   RoadMapStreetIdentifier *street) {

	int index;

	street->prefix = ROADMAP_INVALID_STRING;
	street->name = ROADMAP_INVALID_STRING;
	street->suffix = ROADMAP_INVALID_STRING;
	street->type = ROADMAP_INVALID_STRING;

	for (index = 0; index < RoadMapStreetActive->RoadMapStreetsCount; index++) {
		const char *candidate = roadmap_street_get_street_name_from_id (index);
		if (name && !strcmp (name, candidate)) {

			RoadMapStreet *one_street = RoadMapStreetActive->RoadMapStreets + index;
			street->prefix = one_street->fedirp;
			street->name = one_street->fename;
			street->suffix = one_street->fedirs;
			street->type = one_street->fetype;
			break;
		}
	}

}


#if 0
static void roadmap_street_locate (const char *name,
                                   RoadMapStreetIdentifier *street) {

   char *space;
   char  buffer[128];

   street->prefix = 0;
   street->suffix = 0;
   street->type   = 0;

   /* Search for a prefix. If found, remove the prefix from the name
    * by shifting the name's pointer.
    */
   space = strchr (name, ' ');

   if (space != NULL) {

      unsigned  length;

      length = (unsigned)(space - name);

      if (length < sizeof(buffer)) {

         strncpy (buffer, name, length);
         buffer[length] = 0;

         street->prefix =
            roadmap_dictionary_locate
               (RoadMapStreetActive->RoadMapStreetPrefix, buffer);

         if (street->prefix != ROADMAP_INVALID_STRING) {
            name = name + length;
            while (*name == ' ') ++name;
         }
      }

      /* Search for a street type. If found, extract the street name
       * and stor it in the temporary buffer, which will be substituted
       * for the name.
       */
      space = strrchr (name, ' ');

      if (space != NULL) {

         char *trailer;

         trailer = space + 1;

         street->type =
            roadmap_dictionary_locate
               (RoadMapStreetActive->RoadMapStreetType, trailer);

         if (street->type != ROADMAP_INVALID_STRING) {

            length = (unsigned)(space - name);

            if (length < sizeof(buffer)) {

               strncpy (buffer, name, length);
               buffer[length] = 0;

               while (buffer[length-1] == ' ') buffer[--length] = 0;

               space = strrchr (buffer, ' ');

               if (space != NULL) {

                  trailer = space + 1;

                  street->suffix =
                     roadmap_dictionary_locate
                        (RoadMapStreetActive->RoadMapStreetSuffix, trailer);

                  if (street->suffix != ROADMAP_INVALID_STRING) {

                     while (*space == ' ') {
                        *space = 0;
                        --space;
                     }
                  }
               }

               name = buffer;
            }
         }
      }
   }

   street->name =
      roadmap_dictionary_locate (RoadMapStreetActive->RoadMapStreetNames, name);
   if (street->prefix == ROADMAP_INVALID_STRING) street->prefix = 0;
   if (street->suffix == ROADMAP_INVALID_STRING) street->suffix = 0;
   if (street->type   == ROADMAP_INVALID_STRING) street->type   = 0;
}
#endif

static int roadmap_street_block_by_county_subdivision
              (RoadMapStreetIdentifier *street,
               int city,
               RoadMapBlocks *blocks, int size) {
   int i;
   int j;
   int count = 0;


   for (i = RoadMapStreetActive->RoadMapCities[city].first_street;
   		i < RoadMapStreetActive->RoadMapCities[city+1].first_street; i++) {

      RoadMapStreet *this_street = RoadMapStreetActive->RoadMapStreets + i;

      if (this_street->fename == street->name &&
          (street->prefix == 0 || this_street->fedirp == street->prefix) &&
          (street->suffix == 0 || this_street->fedirs == street->suffix) &&
          (street->type   == 0 || this_street->fetype == street->type  )) {

		 	for (j = roadmap_line_count() - 1; j >= 0; j--) {

				if (roadmap_line_get_street(j) == i) {

					if (count >= size)
						break;
					blocks[count].square = roadmap_square_active ();
               		blocks[count].street = i;
               		blocks[count].line   = j;
               		blocks[count].city   = city;
					roadmap_range_get_address (roadmap_line_get_range (j),
														blocks[count].range + 0,
														blocks[count].range + 1);
				   ++count;
				}
		 	}
      }
   }
   return count;
}


int roadmap_street_blocks_by_city
       (const char *street_name, const char *city_name,
        RoadMapBlocks *blocks,
        int size) {

   int count;
   int total = 0;

   int city;
   RoadMapStreetIdentifier street;

   const RoadMapCityId *id;
   RoadMapCityEntry entry;

	city = roadmap_city_find (city_name);
	if (city < 0) return ROADMAP_STREET_NOCITY;

	id = roadmap_city_first (city, &entry);
	while (id) {

		if (roadmap_square_set_current (id->square_id)) {

		   roadmap_street_locate (street_name, &street);
		   if (street.name != ROADMAP_INVALID_STRING) {

			   count = roadmap_street_block_by_county_subdivision
		                                (&street, id->city_id, blocks + total, size - total); // advance the pointer by total, and decrease the left space
			   if (count > 0) {
			      total += count;
			   }
		   }
		}

		id = roadmap_city_next (city, &entry);
	}

   return total;
}


int roadmap_street_blocks_by_zip
       (const char *street_name, int zip,
        RoadMapBlocks *blocks,
        int size) {

#if 1
   return 0;
#else

   int i;
   int j;
   int zip_index = 0;
   int zip_count;
   int range_count;
   int count = 0;

   RoadMapStreetIdentifier street;

   if ((RoadMapRangeActive == NULL) || (RoadMapZipActive == NULL)) return 0;

   zip_count   = RoadMapZipActive->RoadMapZipCodeCount;
   range_count = RoadMapRangeActive->RoadMapByStreetCount;

   roadmap_street_locate (street_name, &street);

   if (street.name == ROADMAP_INVALID_STRING) {
      return ROADMAP_STREET_NOSTREET;
   }

   for (i = 1; i < zip_count; i++) {
      if (RoadMapZipActive->RoadMapZipCode[i] == zip) {
         zip_index = i;
         break;
      }
   }
   if (zip_index <= 0) {
      return ROADMAP_STREET_NOSTREET;
   }

   for (i = 0; i < range_count; i++) {

      RoadMapStreet *this_street = RoadMapStreetActive->RoadMapStreets + i;
      RoadMapRangeByStreet *by_street = RoadMapRangeActive->RoadMapByStreet + i;

      if (this_street->fename == street.name && by_street->count_range >= 0) {

         int range_index;
         int range_end;

         range_index = by_street->first_range;
         range_end  = by_street->first_range + by_street->count_range;

         for (j = by_street->first_zip; range_index < range_end; j++) {

            RoadMapRangeByZip *by_zip = RoadMapRangeActive->RoadMapByZip + j;

            if (by_zip->zip == zip_index) {

               blocks[count].street = i;
               blocks[count].first  = range_index;
               blocks[count].count  = by_zip->count;

               if (++count >= size) {
                  return count;
               }
               break;
            }

            range_index += by_zip->count;
         }
      }
   }
   return count;
#endif
}


int roadmap_street_get_ranges
       (RoadMapBlocks *blocks, int count, RoadMapStreetRange *ranges) {

   int total = 0;

#if 0

   int i;
   int end;

   int fradd;
   int toadd;


   if (RoadMapRangeActive == NULL) return 0;

   for (i = blocks->first, end = blocks->first + blocks->count; i < end; ++i) {

      RoadMapRange *this_addr = RoadMapRangeActive->RoadMapAddr + i;

      if (HAS_CONTINUATION(this_addr)) {

         fradd = ((int)(this_addr->fradd) & 0xffff)
                    + (((int)(this_addr[1].fradd) & 0xffff) << 16);
         toadd = ((int)(this_addr->toadd) & 0xffff)
                    + (((int)(this_addr[1].toadd) & 0xffff) << 16);

         ++i; /* Because this range occupies two entries. */

      } else {

         fradd = this_addr->fradd;
         toadd = this_addr->toadd;
      }

      if (total >= count) return total;

      ranges->fradd = fradd;
      ranges->toadd = toadd;
      ranges += 1;
      total  += 1;
   }
#endif

   return total;
}


static void roadmap_street_distance_position (int line, int distance,
                                              RoadMapPosition *position) {

   RoadMapPosition from;
   RoadMapPosition to;
   int first_shape;
   int last_shape;
   int segment_length;

   roadmap_line_from (line, &from);

   if (distance == 0) {
      *position = from;
      return;
   }

   roadmap_line_to (line, &to);
   roadmap_line_shapes (line, &first_shape, &last_shape);

   if (first_shape >= 0) {
      int i;
      to = from;
      for (i=first_shape; i<=last_shape; i++) {

         roadmap_shape_get_position (i, &to);
         segment_length = roadmap_math_distance (&from, &to);

         if (segment_length >= distance) break;
         distance -= segment_length;

         from = to;
      }

      if (i > last_shape) roadmap_line_to (line, &to);
   }

   segment_length = roadmap_math_distance (&from, &to);
   if (distance >= segment_length) {
      *position = to;
      return;
   }

   position->longitude = from.longitude +
         distance * (to.longitude - from.longitude) / segment_length;

   position->latitude = from.latitude +
         distance * (to.latitude - from.latitude) / segment_length;
}


int roadmap_street_get_position (RoadMapBlocks *blocks,
                                 int number,
                                 RoadMapPosition *position) {

	int length;
	int distance;
	int side;

	roadmap_square_set_current (blocks->square);

	if (number == -1) {

		length = roadmap_line_length (blocks->line);
		distance = length / 2;
		roadmap_street_distance_position (blocks->line, distance, position);
		return 1;
	}

	for (side = 0; side < 2; side++) {

		RoadMapStreetRange *range = &blocks->range[side];
		if (range->fradd >= 0 &&
			 range->fradd % 2 == number % 2) {

			 if ((number >= range->fradd && number <= range->toadd) ||
			 	  (number >= range->toadd && number <= range->fradd)) {

				length = roadmap_line_length (blocks->line);
				if (range->fradd <= range->toadd) {
					distance = length * (number - range->fradd + 1) / (range->toadd - range->fradd + 2);
				} else {
					distance = length * (number - range->fradd - 1) / (range->toadd - range->fradd - 2);
				}
				roadmap_street_distance_position (blocks->line, distance, position);
				return 1;
			 }
		}
	}

	return 0;
}


static int roadmap_street_get_distance_with_shape
              (const RoadMapPosition *position,
               int line, int cfcc, int first_shape, int last_shape,
               RoadMapNeighbour *neighbours, int max) {

   int i;
   RoadMapNeighbour current;
   int fips = roadmap_locator_active();
	int square = roadmap_square_active ();
	int found = 0;

   roadmap_plugin_set_line
      (&current.line, ROADMAP_PLUGIN_ID, line, cfcc, square, fips);

   /* Note: the position of a shape point is relative to the position
    * of the previous point, starting with the from point.
    * The logic here takes care of this property.
    */
   roadmap_line_from (line, &current.from);

   current.to = current.from; /* Initialize the shape position (relative). */

   for (i = first_shape; i <= last_shape; i++) {

      roadmap_shape_get_position (i, &current.to);

      if (roadmap_math_get_visible_coordinates (&current.from, &current.to,
                                                NULL, NULL)) {

         current.distance =
            roadmap_math_get_distance_from_segment
               (position, &current.from, &current.to,
                &current.intersection, NULL);

			found = roadmap_street_replace (neighbours, found, max, &current);
      }

      current.from = current.to;
   }

   roadmap_line_to (line, &current.to);

   if (roadmap_math_get_visible_coordinates (&current.from, &current.to,
                                                NULL, NULL)) {

      current.distance =
         roadmap_math_get_distance_from_segment
            (position, &current.to, &current.from, &current.intersection, NULL);

		found = roadmap_street_replace (neighbours, found, max, &current);
   }

   return found;
}


static int roadmap_street_get_distance_no_shape
              (const RoadMapPosition *position, int line, int cfcc,
               RoadMapNeighbour *neighbour) {

   roadmap_line_from (line, &neighbour->from);
   roadmap_line_to   (line, &neighbour->to);

   if (roadmap_math_get_visible_coordinates (&neighbour->from, &neighbour->to,
                                             NULL, NULL)) {

      neighbour->distance =
         roadmap_math_get_distance_from_segment
            (position, &neighbour->from, &neighbour->to,
             &neighbour->intersection, NULL);

      roadmap_plugin_set_line
         (&neighbour->line,
          ROADMAP_PLUGIN_ID,
          line,
          cfcc,
			 roadmap_square_active (),
          roadmap_locator_active ());

      return 1;
   }

   return 0;
}


int roadmap_street_replace
              (RoadMapNeighbour *neighbours, int count, int max,
               const RoadMapNeighbour *this) {

   int i;

   for (i = 0; i < count; i++) {

      if (roadmap_plugin_same_line (&neighbours[i].line, &this->line) &&
      	 neighbours[i].from.longitude == this->from.longitude &&
      	 neighbours[i].from.latitude == this->from.latitude &&
      	 neighbours[i].to.longitude == this->to.longitude &&
      	 neighbours[i].to.latitude == this->to.latitude) {
         return count;
      }
   }

	/* keep the list sorted */
	for (i = count; i > 0 && neighbours[i - 1].distance > this->distance; i--) {
		if (i < max) {
			neighbours[i] = neighbours[i - 1];
		}
	}

	if (i < max) neighbours[i] = *this;

	return count < max ? count + 1 : max;
}


static int roadmap_street_get_distance_from_line
        (const RoadMapPosition *position, int line, int cfcc,
         RoadMapNeighbour *result) {

   int first_shape;
   int last_shape;

   if (RoadMapStreetActive == NULL) return 0;

   if (roadmap_line_shapes (line, &first_shape, &last_shape) > 0) {

      return roadmap_street_get_distance_with_shape
                  (position, line, cfcc, first_shape, last_shape, result, 1);
   } else {

      return roadmap_street_get_distance_no_shape
                              (position, line, cfcc, result);
   }
}


static int roadmap_street_check_distance (const PluginLine *line, void *context, int extend_flags) {

	RoadMapStreetSearch *street_search = (RoadMapStreetSearch *)context;
	RoadMapNeighbour	  neighbour;

	roadmap_square_set_current (line->square);
	if (roadmap_street_get_distance_from_line (street_search->position,
															 line->line_id,
															 street_search->cfcc,
															 &neighbour)) {

		if (street_search->rc == 0 ||
			 street_search->result->distance > neighbour.distance) {

			*street_search->result = neighbour;
			street_search->result->line.square = line->square;
			street_search->result->line.line_id = line->line_id;
			street_search->rc = 1;
		}
	}

	return 1;
}


int roadmap_street_get_distance
        (const RoadMapPosition *position, int line, int cfcc,
         RoadMapNeighbour *result) {


	RoadMapStreetSearch		street_search;
	PluginLine					search_line;

	street_search.rc = 0;
	street_search.result = result;
	street_search.cfcc = cfcc;
	street_search.position = position;

	search_line.plugin_id = ROADMAP_PLUGIN_ID;
	search_line.square = roadmap_square_active ();
	search_line.line_id = line;

	roadmap_street_check_distance (&search_line, &street_search, 0);
	roadmap_street_extend_line_ends (&search_line, NULL, NULL, FLAG_EXTEND_BOTH,
												  roadmap_street_check_distance, &street_search);

	return street_search.rc;
}


static int roadmap_street_get_closest_in_square
              (const RoadMapPosition *position, int square, int cfcc,
               int max_shapes, RoadMapNeighbour *neighbours,
               int count, int max) {

   int line;
   int found;
   int first_line;
   int last_line;
   int first_shape;
   int last_shape;
   int i;
   int fips;
   RoadMapNeighbour this[3];
   int max_possible_shapes = (int)(sizeof(this) / sizeof(this[0]));

   if (max_shapes > max_possible_shapes) {
   	max_shapes = max_possible_shapes;
   }

   fips = roadmap_locator_active ();


   if (roadmap_line_in_square (square, cfcc, &first_line, &last_line) > 0) {

      if (roadmap_square_has_shapes (square)) {

         for (line = first_line; line <= last_line; line++) {

            if (roadmap_plugin_override_line (line, cfcc, fips)) continue;

            if (roadmap_line_shapes (line, &first_shape, &last_shape) > 0) {

               found =
                  roadmap_street_get_distance_with_shape
                     (position, line, cfcc, first_shape, last_shape, this, max_shapes);

            } else {
               found =
                   roadmap_street_get_distance_no_shape
                           (position, line, cfcc, this);
            }
            for (i = 0; i < found; i++) {
               count = roadmap_street_replace (neighbours, count, max, this + i);
            }
         }

      } else {

         for (line = first_line; line <= last_line; line++) {

            if (roadmap_street_get_distance_no_shape
                        (position, line, cfcc, this)) {
               count = roadmap_street_replace (neighbours, count, max, this);
            }
         }
      }
   }

   return count;
}


int roadmap_street_get_closest
       (const RoadMapPosition *position, int scale,
        int *categories, int categories_count, int max_shapes,
        RoadMapNeighbour *neighbours, int max) {

   static int *fips = NULL;

   int i;
   int j;
   int county;
   int county_count;
   int square[9];
   int count_squares;

   int count = 0;


   if (RoadMapStreetActive == NULL) return 0;

   county_count = roadmap_locator_by_position (position, &fips);

   /* - For each candidate county: */

   for (county = county_count - 1; county >= 0; --county) {

      /* -- Access the county's database. */

      if (roadmap_locator_activate (fips[county]) != ROADMAP_US_OK) continue;

      /* -- Look for the square the current location fits in. */

		count_squares = roadmap_square_find_neighbours (position, scale, square);
		for (j = 0; j < count_squares; j++) {

      /* The current location fits in one of the county's squares.
       * We might be in that county, search for the closest streets.
       */
         for (i = 0; i < categories_count; ++i) {

            count =
               roadmap_street_get_closest_in_square
                  (position, square[j], categories[i], max_shapes, neighbours, count, max);
			}
		}
   }

   return count;
}

#if 0
static int roadmap_street_check_street (int street, int line) {

   int i;
   int range_end;
   RoadMapRange *range;
   RoadMapRangeByStreet *by_street;


   if (RoadMapRangeActive == NULL) return -1;

   range     = RoadMapRangeActive->RoadMapAddr;
   by_street = RoadMapRangeActive->RoadMapByStreet + street;
   range_end = by_street->first_range + by_street->count_range;

   for (i = by_street->first_range; i < range_end; i++) {

      if ((range[i].line & (~CONTINUATION_FLAG)) == (unsigned)line) {
         return i;
      }
   }
   return -1;
}
#endif


#if 0
static int roadmap_street_check_other_side (int street,
                                            int line,
                                            int first_range) {

   int range_end;
   RoadMapRange *range;
   RoadMapRangeByStreet *by_street;


   if (RoadMapRangeActive == NULL) return -1;

   range     = RoadMapRangeActive->RoadMapAddr;
   by_street = RoadMapRangeActive->RoadMapByStreet + street;
   range_end = by_street->first_range + by_street->count_range;

   if (HAS_CONTINUATION ((range + first_range))) {
      first_range++;
   }

   first_range++;

   if (first_range >= range_end) return -1;

   if ((range[first_range].line & (~CONTINUATION_FLAG)) == (unsigned)line) {
         return first_range;
   }
   return -1;
}
#endif

#if 0
static void roadmap_street_extract_range (int range_index,
                                          RoadMapStreetRange *range) {

   RoadMapRange *this_address =
      &(RoadMapRangeActive->RoadMapAddr[range_index]);

   if (HAS_CONTINUATION(this_address)) {

      range->fradd =
         ((int)(this_address->fradd) & 0xffff)
         + (((int)(this_address[1].fradd) & 0xffff) << 16);
      range->toadd =
         ((int)(this_address->toadd) & 0xffff)
         + (((int)(this_address[1].toadd) & 0xffff) << 16);

   } else {

      range->fradd = this_address->fradd;
      range->toadd = this_address->toadd;
   }
}
#endif

static int roadmap_street_get_city (int street/*, int range*/) {

   int i;

	if (street == -1) return -1;

	//todo: binary search
   for (i = RoadMapStreetActive->RoadMapCitiesCount; i >= 0 &&
   		RoadMapStreetActive->RoadMapCities[i].first_street > street; i--)
   		;

   return RoadMapStreetActive->RoadMapCities[i].city;
}

#if 0
#define STREET_HASH_MODULO  513

typedef struct {

   RoadMapRange  *range;
   unsigned short next;
   unsigned short side;

} RoadMapRangesHashEntry;

typedef struct {

   RoadMapRangesHashEntry *list;
   int list_size;
   int list_cursor;

   unsigned short head[STREET_HASH_MODULO];

} RoadMapRangesHash;

static int roadmap_street_hash_code (RoadMapPosition *position) {

   int hash_code =
          (position->latitude - position->longitude) % STREET_HASH_MODULO;

   if (hash_code < 0) return 0 - hash_code;

   return hash_code;
}

static RoadMapRange *roadmap_street_hash_search (RoadMapRangesHash *hash,
                                                 int erase,
                                                 RoadMapPosition *position) {

   int i;
   int line;
   RoadMapPosition position2;
   RoadMapRangesHashEntry *entry;

   int hash_code = roadmap_street_hash_code (position);


   for (i = hash->head[hash_code]; i < 0xffff; i = entry->next) {

       entry = hash->list + i;

       if (entry->side == 0xffff) continue; /* Already matched. */

       line = entry->range->line & (~CONTINUATION_FLAG);

       if (entry->side == 0) {
          roadmap_line_from(line, &position2);
       } else {
          roadmap_line_to(line, &position2);
       }

       if (position2.latitude == position->latitude &&
           position2.longitude == position->longitude) {
          if (erase) {
             entry->side = 0xffff;
          }
          return entry->range;
       }
   }
   return NULL;
}

static void roadmap_street_hash_add (RoadMapRangesHash *hash,
                                     RoadMapRange *this_addr,
                                     int side,
                                     RoadMapPosition *position) {

   if (roadmap_street_hash_search (hash, 0, position) == NULL) {

       int hash_code = roadmap_street_hash_code (position);

       hash->list[hash->list_cursor].range = this_addr;
       hash->list[hash->list_cursor].side  = side;
       hash->list[hash->list_cursor].next = hash->head[hash_code];
       hash->head[hash_code] = hash->list_cursor;
       hash->list_cursor += 1;
   }
}

static int roadmap_street_intersection_county
               (int fips,
                RoadMapStreetIdentifier *street1,
                RoadMapStreetIdentifier *street2,
                RoadMapStreetIntersection *intersection,
                int count) {

   static int Initialized = 0;
   static RoadMapRangesHash Hash;

   int i, j;
   int line;
   int results = 0;
   int range_end;

   RoadMapPosition position;


   /* First loop: build a hash table of all the street block endpoint
    * positions for the 1st street.
    */
   if (! Initialized) {
      Hash.list = NULL;
      Hash.list_size = 0;
      Hash.list_cursor = 1; /* Force an initial reset. */
      Initialized = 1;
   }

   if (Hash.list_cursor > 0) {

      /* This hash has been used before: reset it. */

      for (i = STREET_HASH_MODULO - 1; i >= 0; --i) {
         Hash.head[i] = 0xffff;
      }
      Hash.list_cursor = 0;
   }

   for (i = RoadMapRangeActive->RoadMapByStreetCount - 1; i >= 0; --i) {

       RoadMapStreet *this_street = RoadMapStreetActive->RoadMapStreets + i;

       if (this_street->fename == street1->name &&
           (street1->prefix == 0 || this_street->fedirp == street1->prefix) &&
           (street1->suffix == 0 || this_street->fedirs == street1->suffix) &&
           (street1->type   == 0 || this_street->fetype == street1->type)) {

           RoadMapRangeByStreet *by_street;

           by_street = RoadMapRangeActive->RoadMapByStreet + i;

           if (by_street->count_range > 0) {

               int required = 2 * (Hash.list_cursor + by_street->count_range);

               range_end = by_street->first_range + by_street->count_range;

               if (required > Hash.list_size) {

                  Hash.list_size = required * 2;
                  if (Hash.list_size >= 0xffff) break;

                  Hash.list =
                      realloc (Hash.list,
                               Hash.list_size * sizeof(RoadMapRangesHashEntry));
                  roadmap_check_allocated(Hash.list);
               }

               for (j = by_street->first_range; j < range_end; ++j) {

                   RoadMapRange *this_addr =
                      RoadMapRangeActive->RoadMapAddr + j;

                   if (HAS_CONTINUATION(this_addr)) ++j;

                   line = this_addr->line & (~CONTINUATION_FLAG);

                   roadmap_line_from(line, &position);
                   roadmap_street_hash_add (&Hash, this_addr, 0, &position);

                   roadmap_line_to(line, &position);
                   roadmap_street_hash_add (&Hash, this_addr, 1, &position);
               }
           }
       }
   }
   if (Hash.list_cursor == 0) return 0;


   /* Second loop: match each street block endpoint positions for the 2nd
    * street with the positions for the 1st street.
    */
   for (i = RoadMapRangeActive->RoadMapByStreetCount - 1; i >= 0; --i) {

       RoadMapStreet *this_street = RoadMapStreetActive->RoadMapStreets + i;

       if (this_street->fename == street2->name &&
           (street2->prefix == 0 || this_street->fedirp == street2->prefix) &&
           (street2->suffix == 0 || this_street->fedirs == street2->suffix) &&
           (street2->type   == 0 || this_street->fetype == street2->type)) {

           RoadMapRangeByStreet *by_street;

           by_street = RoadMapRangeActive->RoadMapByStreet + i;

           if (by_street->count_range > 0) {

               range_end = by_street->first_range + by_street->count_range;

               for (j = by_street->first_range; j < range_end; ++j) {

                   RoadMapRange *this_addr1;
                   RoadMapRange *this_addr2 =
                      RoadMapRangeActive->RoadMapAddr + j;

                   if (HAS_CONTINUATION(this_addr2)) ++j;

                   line = this_addr2->line & (~CONTINUATION_FLAG);

                   roadmap_line_from(line, &position);
                   this_addr1 =
                       roadmap_street_hash_search (&Hash, 1, &position);

                   if (this_addr1 == NULL) {
                      roadmap_line_to(line, &position);
                      this_addr1 =
                          roadmap_street_hash_search (&Hash, 1, &position);
                   }

                   if (this_addr1 != NULL) {

                      intersection[results].fips = fips;
                      intersection[results].line1 =
                          this_addr1->line & (~CONTINUATION_FLAG);
                      intersection[results].line2 =
                          this_addr2->line & (~CONTINUATION_FLAG);
                      intersection[results].position = position;

                      ++results;
                      if (results >= count) return results;
                   }
               }
           }
       }
   }

   return results;
}
#endif

int roadmap_street_intersection (const char *state,
                                 const char *street1_name,
                                 const char *street2_name,
                                 RoadMapStreetIntersection *intersection,
                                 int count) {

#if 1
	  return 0;
#else
   static int *fips = NULL;

   int i;
   int results = 0;
   int county_count;

   RoadMapStreetIdentifier street1, street2;


   if (RoadMapRangeActive == NULL) return 0;

   county_count = roadmap_locator_by_state (state, &fips);

   for (i = county_count - 1; i >= 0; --i) {

      if (roadmap_locator_activate (fips[i]) != ROADMAP_US_OK) continue;

      roadmap_street_locate (street1_name, &street1);
      if (street1.name == ROADMAP_INVALID_STRING) continue;

      roadmap_street_locate (street2_name, &street2);
      if (street2.name == ROADMAP_INVALID_STRING) continue;

      results += roadmap_street_intersection_county
                     (fips[i], &street1, &street2,
                      intersection + results,
                      count - results);
   }

   return results;
#endif
}


void roadmap_street_get_properties (
        int line, RoadMapStreetProperties *properties) {

   int range = roadmap_line_get_range (line);
   int street;


   memset (properties, -1, sizeof(RoadMapStreetProperties));
   properties->street = -1;

   if (RoadMapStreetActive == NULL) return;

   roadmap_range_get_address (range, &properties->first_range, &properties->second_range);

   street = roadmap_range_get_street (range);
   properties->city = roadmap_street_get_city (street);
   properties->street = street;
}


void roadmap_street_get_street (int line, RoadMapStreetProperties *properties) {

   if (RoadMapStreetActive == NULL) return;

   properties->street = roadmap_line_get_street (line);
}


static void roadmap_street_append (char *name, int max_size, char *item) {

	if (item != NULL && item[0] != 0) {
   	int len;

        //strcat (name, item);
   	len = strlen (name);
      strncpy_safe (name + len, item, max_size - len);
        //strcat (name, " ");
   	len = strlen (name);
      strncpy_safe (name + len, " ", max_size - len);
   }
}


const char *roadmap_street_get_street_address (
                const RoadMapStreetProperties *properties) {

    static char RoadMapStreetAddress [32];
    int min = RANGE_ADDR_MAX_VALUE;
    int max = 0;

    if (properties->first_range.fradd == -1 &&
    	  properties->second_range.fradd == -1) {
        return "";
    }

    if (properties->first_range.fradd != -1) {

	    if (properties->first_range.fradd > properties->first_range.toadd) {
	       min = properties->first_range.toadd;
	       max = properties->first_range.fradd;
	    } else {
	       min = properties->first_range.fradd;
	       max = properties->first_range.toadd;
	    }
    }

    if (properties->second_range.fradd != -1) {

       if (properties->second_range.fradd < min) {
          min = properties->second_range.fradd;
       }
       if (properties->second_range.fradd > max) {
          max = properties->second_range.fradd;
       }
       if (properties->second_range.toadd < min) {
          min = properties->second_range.toadd;
       }
       if (properties->second_range.toadd > max) {
          max = properties->second_range.toadd;
       }
    }

    sprintf (RoadMapStreetAddress, "%d - %d", min, max);

    return RoadMapStreetAddress;
}


static const char *roadmap_street_get_street_name_from_id (int street) {

    static char RoadMapStreetName [512];

    char *p;
    RoadMapStreet *this_street;


    if (street < 0) {

        return "";
    }

    this_street =
        RoadMapStreetActive->RoadMapStreets + street;

    RoadMapStreetName[0] = 0;

    roadmap_street_append
        (RoadMapStreetName, sizeof (RoadMapStreetName),
         roadmap_dictionary_get
            (RoadMapStreetActive->RoadMapStreetPrefix, this_street->fedirp));

    roadmap_street_append
        (RoadMapStreetName, sizeof (RoadMapStreetName),
         roadmap_dictionary_get
            (RoadMapStreetActive->RoadMapStreetNames, this_street->fename));

    roadmap_street_append
        (RoadMapStreetName, sizeof (RoadMapStreetName),
         roadmap_dictionary_get
            (RoadMapStreetActive->RoadMapStreetType, this_street->fetype));

    roadmap_street_append
        (RoadMapStreetName, sizeof (RoadMapStreetName),
         roadmap_dictionary_get
            (RoadMapStreetActive->RoadMapStreetSuffix, this_street->fedirs));

    /* trim the resulting string on both sides (right then left): */

    p = RoadMapStreetName + strlen(RoadMapStreetName) - 1;
    while (*p == ' ') *(p--) = 0;

    for (p = RoadMapStreetName; *p == ' '; p++) ;

    return p;
}


const char *roadmap_street_get_street_name (
                const RoadMapStreetProperties *properties) {

	return roadmap_street_get_street_name_from_id (properties->street);
}


const char *roadmap_street_get_city_name (
                const RoadMapStreetProperties *properties) {

   if (RoadMapStreetActive == NULL) return 0;

	if (properties->city == (RoadMapString)-1) {
	 	return "";
	}
   return roadmap_dictionary_get
                (RoadMapStreetActive->RoadMapCityNames, properties->city);
}


const char *roadmap_street_get_full_name (
                const RoadMapStreetProperties *properties) {

    static char RoadMapStreetName [512];

    const char *address;
    const char *city;


    if (properties->street < 0) {
        return "";
    }

    address = roadmap_street_get_street_address (properties);
    if (properties->city != ROADMAP_INVALID_STRING) {
       city    = roadmap_street_get_city_name      (properties);
    } else {
       city = "";
    }

    snprintf (RoadMapStreetName, sizeof(RoadMapStreetName),
              "%s%s%s%s%s",
              address,
              (address[0])? " " : "",
              roadmap_street_get_street_name (properties),
              (city[0])? ", " : "",
              city);

    return RoadMapStreetName;
}


const char *roadmap_street_get_street_fename (
                const RoadMapStreetProperties *properties) {

    RoadMapStreet *this_street;

    if (properties->street < 0) {

        return "";
    }

    this_street =
        RoadMapStreetActive->RoadMapStreets + properties->street;

    return
         roadmap_dictionary_get
            (RoadMapStreetActive->RoadMapStreetNames, this_street->fename);
}


const char *roadmap_street_get_street_fetype (
                const RoadMapStreetProperties *properties) {

    RoadMapStreet *this_street;

    if (properties->street < 0) {

        return "";
    }

    this_street =
        RoadMapStreetActive->RoadMapStreets + properties->street;

    return
         roadmap_dictionary_get
            (RoadMapStreetActive->RoadMapStreetType, this_street->fetype);
}


const char *roadmap_street_get_street_fedirs (
                const RoadMapStreetProperties *properties) {

    RoadMapStreet *this_street;

    if (properties->street < 0) {

        return "";
    }

    this_street =
        RoadMapStreetActive->RoadMapStreets + properties->street;

    return
         roadmap_dictionary_get
            (RoadMapStreetActive->RoadMapStreetType, this_street->fedirs);
}


const char *roadmap_street_get_street_fedirp (
                const RoadMapStreetProperties *properties) {

    RoadMapStreet *this_street;

    if (properties->street < 0) {

        return "";
    }

    this_street =
        RoadMapStreetActive->RoadMapStreets + properties->street;

    return
         roadmap_dictionary_get
            (RoadMapStreetActive->RoadMapStreetType, this_street->fedirp);
}

static const char *parse_extended_data(const RoadMapStreetProperties *properties, int type) {
#ifdef J2MEMAP
   return "";
#else
   RoadMapStreet *this_street;
   char *data;
   static char text[256];
   int i;
   
   if (RoadMapStreetActive == NULL ||
       RoadMapStreetActive->RoadMapText2Speech == NULL) return "";
   
   if (properties->street < 0) {
      return "";
   }
   
   this_street = RoadMapStreetActive->RoadMapStreets + properties->street;
   
   data = roadmap_dictionary_get (RoadMapStreetActive->RoadMapText2Speech, this_street->t2s);
   text[0] = '\0';
   
   for (i = 0; i <= type && data && data[0] >= '0' && data[0] <= '9'; i++) {
      int count = 0;
      
      // read char count of row
      while (data && data[0] >= '0' && data[0] <= '9') {
         count = count * 10 + (data[0] - '0');
         data++;
      }
      if (!data || !data[0] == ',')
         return "";
      
      data++;
      
      if (strlen(data) < count)
         return "";
      
      if (count + 1 > 256) {
         roadmap_log(ROADMAP_DEBUG, "Data too long, cannot handle more than (256)");
         return "";
      }
      
      if (i == type) {
         strncpy_safe (text, data, count + 1);
      }
      
      data += count;
   }
   
   return text;
#endif
   
}


const char *roadmap_street_get_street_t2s (
                const RoadMapStreetProperties *properties) {

#ifdef J2MEMAP
    return "";
#else
    return ""; //t2s field is currently used as extended data. t2s value is not used currently
#endif
}

const char *roadmap_street_get_street_shield_text (
                const RoadMapStreetProperties *properties) {
   /*
   const char *street_t2s = roadmap_street_get_street_t2s(properties);
   
   if (street_t2s && street_t2s[0] >= '0' && street_t2s[0] <= '9')
      printf("(square: %d) '%s'\n", RoadMapSquareCurrent, street_t2s);
   else
      printf("NOT new format (square: %d) '%s'\n", RoadMapSquareCurrent, street_t2s);

   
   const char *street_name = roadmap_street_get_street_name(properties);
   if (street_name && street_name[0] >= '0' && street_name[0] <= '9' &&
       strlen(street_name) <= 4)
      return street_name;
   else
      return "";
    */
   
   static char shield_text[256];
   
   strncpy_safe (shield_text, parse_extended_data(properties, STREET_EXTENDED_TYPE_SHIELD_TEXT), sizeof(shield_text));
   
   return shield_text;
}

const char *roadmap_street_get_street_shield_type (
                const RoadMapStreetProperties *properties) {
   /*
   const char *street_name = roadmap_street_get_street_name(properties);
   if (street_name && street_name[0] >= '0' && street_name[0] <= '9' &&
       strlen(street_name) <= 4) {
      switch (strlen(street_name)) {
         case 1:
            return "1";
            break;
         case 2:
            return "2";
            break;
         case 3:
            return "3";
            break;
         case 4:
            return "4";
            break;
         default:
            break;
      }
      return "1";
   } else
      return "";
    */
   
   static char shield_type[256];
   
   strncpy_safe (shield_type, parse_extended_data(properties, STREET_EXTENDED_TYPE_SHIELD_TYPE), sizeof(shield_type));
   
   return shield_type;
}

const char *roadmap_street_get_street_city (
                const RoadMapStreetProperties *properties, int side) {

//TODO add street sides search

    return roadmap_street_get_city_name (properties);
}


const char *roadmap_street_get_street_zip (
                const RoadMapStreetProperties *properties, int side) {
//TODO add implement get_street_zip
   return "";
}


void roadmap_street_get_street_range
   (const RoadMapStreetProperties *properties, int range, int *from, int *to) {

   if (range == 1) {
      *from = properties->first_range.fradd;
      *to = properties->first_range.toadd;

   } else if (range == 2) {
      *from = properties->second_range.fradd;
      *to = properties->second_range.toadd;

   } else {
      roadmap_log (ROADMAP_ERROR, "Illegal range number: %d", range);
   }
}


static const char *roadmap_street_get_name_string (int search_city, int city, int street) {
	static char full_name[512];

	if (search_city < 0) {
		snprintf (full_name, sizeof (full_name), "%s, %s",
					roadmap_street_get_street_name_from_id (street),
					roadmap_dictionary_get
               	(RoadMapStreetActive->RoadMapCityNames,
               	 RoadMapStreetActive->RoadMapCities[city].city));
	} else {
		strncpy_safe (full_name, roadmap_street_get_street_name_from_id (street), sizeof (full_name));
	}

	return full_name;
}


int roadmap_street_search (const char *city, const char *str,
									int max_results,
                           RoadMapDictionaryCB cb,
                           void *data) {

   int index = roadmap_city_find (strlen (city) ? city : NULL);
	RoadMapCityEntry entry;
	const RoadMapCityId *id;
	int from;
	int to;
	int street;
	int prev;

   RoadMapStreetSearchCount = 0;

	id = roadmap_city_first (index, &entry);
	while (id) {

		roadmap_square_set_current (id->square_id);
		from = RoadMapStreetActive->RoadMapCities[id->city_id].first_street;
		to = RoadMapStreetActive->RoadMapCities[id->city_id + 1].first_street;

		for (street = from; street < to; street++) {

			const char *name = roadmap_street_get_street_name_from_id (street);
			if (name && roadmap_string_is_sub_ignore_case (name, str)) {

				if (index < 0) {
					name = roadmap_street_get_name_string (index, id->city_id, street);
				}

				for (prev = RoadMapStreetSearchCount - 1; prev >= 0; prev--) {
					if (!strcmp (RoadMapStreetSearchNames[prev], name))
						break;
				}
				if (prev >= 0) continue;

		      if (RoadMapStreetSearchNames[RoadMapStreetSearchCount]) {
		         free (RoadMapStreetSearchNames[RoadMapStreetSearchCount]);
		      }
		      RoadMapStreetSearchNames[RoadMapStreetSearchCount] = strdup(name);
				if (cb) {
		      	if (!(*cb) (street, RoadMapStreetSearchNames[RoadMapStreetSearchCount], data)) {
		      		return RoadMapStreetSearchCount;
		      	}
				}
				RoadMapStreetSearchCount++;
		      if (RoadMapStreetSearchCount == MAX_SEARCH_NAMES ||
		      	 RoadMapStreetSearchCount == max_results) {
		      	return RoadMapStreetSearchCount;
		      }
			}
		}

		id = roadmap_city_next (index, &entry);
	}

   return RoadMapStreetSearchCount;
}

int roadmap_street_search_city (const char *str, RoadMapDictionaryCB cb,
                                void *data) {

	return roadmap_city_search (str, cb, data);
}


void roadmap_street_update_city_index (void) {

	int index;
	int square = roadmap_square_active ();

	roadmap_city_unload (square);
	for (index = 0; index < RoadMapStreetActive->RoadMapCitiesCount; index++) {
		roadmap_city_add (roadmap_dictionary_get
                			(RoadMapStreetActive->RoadMapCityNames,
                			 RoadMapStreetActive->RoadMapCities[index].city),
                			square, index);
	}
}


static int roadmap_street_find_line_cont (int line, int scale, int extend_from,
												 const char *name, int route, int max_dist,
												 int *cont_square, int *cont_line) {

   int edge;
   int square;
   int first_broken;
   int last_broken;
   int broken_id;
   int broken_line;
   RoadMapPosition pos;
   RoadMapPosition new_pos;
   int dist;
   int best_dist = max_dist + 1;
   RoadMapStreetProperties street;

	if (extend_from) {
		roadmap_line_from (line, &pos);
	} else {
		roadmap_line_to (line, &pos);
	}

	edge = roadmap_square_cross_pos (&pos);
	if (edge >= 0) {
   	square = roadmap_square_search (&pos, scale);
   	if (square != ROADMAP_SQUARE_GLOBAL) {
   		roadmap_square_set_current (square);
   		if (roadmap_line_broken_range (edge, extend_from, &first_broken, &last_broken)) {
   			for (broken_id = first_broken; broken_id <= last_broken; broken_id++) {
   				broken_line = roadmap_line_get_broken (broken_id);
   				if (extend_from) {
   					roadmap_line_to (broken_line, &new_pos);
   				} else {
   					roadmap_line_from (broken_line, &new_pos);
   				}
   				if (edge == ROADMAP_DIRECTION_NORTH || edge == ROADMAP_DIRECTION_SOUTH) {
   					dist = new_pos.longitude - pos.longitude;
   				} else {
   					dist = new_pos.latitude - pos.latitude;
   				}
   				if (dist < 0) dist = -dist;
   				if (dist >= best_dist) continue;

				   roadmap_street_get_properties (broken_line, &street);
				   route = roadmap_line_route_get_direction (broken_line, ROUTE_DIRECTION_ANY);
				   if (route == roadmap_line_route_get_direction (broken_line, ROUTE_DIRECTION_ANY) &&
				   	 !strcmp (name, roadmap_street_get_full_name (&street))) {
				   	*cont_square = square;
				   	*cont_line = broken_line;
				   	best_dist = dist;
				   }

   			}
   		}
   	}
  	else
   	{
   		*cont_square = roadmap_tile_get_id_from_position( scale, &pos );
   		*cont_line = -1;
   		roadmap_log( ROADMAP_DEBUG, "Tile %d is not available", *cont_square );
   	}
 }

	return best_dist <= max_dist;
}


int roadmap_street_extend_line_ends
			(const PluginLine *line, RoadMapPosition *from, RoadMapPosition *to,
			 int flags, RoadMapStreetIterCB cb, void *context) {

	PluginLine line_ext;
	RoadMapPosition pos;
   int count = 0;
   char name[512];
   RoadMapStreetProperties street;
   int route = 0;
   int filled_props = 0;
   int scale;
            
   roadmap_log_push("roadmap_street_extend_line_ends");
	if (from) roadmap_plugin_line_from (line, from);
	if (to) roadmap_plugin_line_to (line, to);
   roadmap_log_pop();

	if (line->plugin_id != ROADMAP_PLUGIN_ID) return 0;

	line_ext = *line;
	roadmap_square_set_current (line->square);
	scale = roadmap_square_scale (line->square);

	if (flags & FLAG_EXTEND_FROM) {
	   while (roadmap_line_from_is_fake (line_ext.line_id)) {

	   	if (!filled_props) {
			   roadmap_square_set_current (line_ext.square);
			   roadmap_street_get_properties (line_ext.line_id, &street);
			   strncpy_safe (name, roadmap_street_get_full_name (&street), 512);
			   route = roadmap_line_route_get_direction (line_ext.line_id, ROUTE_DIRECTION_ANY);
			   filled_props = 1;
	   	}

	   	if (!roadmap_street_find_line_cont (line_ext.line_id, scale, 1, name, route, 16,
	   													  &line_ext.square, &line_ext.line_id))
	   	  {
			if ( cb && ( flags & FLAG_EXTEND_CB_NO_SQUARE ) &&
					line_ext.line_id == -1 /* The square not found */ )
			{
				cb ( &line_ext, context, FLAG_EXTEND_CB_NO_SQUARE );
			}
			break;
	      }
	      if (roadmap_plugin_same_line (line, &line_ext)) {
	      	return count;
	      }
	      count++;
	      if (from) roadmap_plugin_line_from (&line_ext, from);
	      if (cb) {
	      	if (cb (&line_ext, context, FLAG_EXTEND_FROM)) break;
	      }

	      roadmap_square_set_current (line_ext.square);
	   }

		line_ext = *line;
	    roadmap_square_set_current (line_ext.square);
	}

	if (flags & FLAG_EXTEND_TO) {
	   while (roadmap_line_to_is_fake (line_ext.line_id)) {

	   	if (!filled_props) {
			   roadmap_square_set_current (line_ext.square);
			   roadmap_street_get_properties (line_ext.line_id, &street);
			   strncpy_safe (name, roadmap_street_get_full_name (&street), 512);
			   route = roadmap_line_route_get_direction (line_ext.line_id, ROUTE_DIRECTION_ANY);
			   filled_props = 1;
	   	}

	      roadmap_line_to (line_ext.line_id, &pos);
	      roadmap_square_cross_pos (&pos);
	   	if (!roadmap_street_find_line_cont (line_ext.line_id, scale, 0, name, route, 16,
	   													  &line_ext.square, &line_ext.line_id)) {
			if ( cb && ( flags & FLAG_EXTEND_CB_NO_SQUARE ) &&
					( line_ext.line_id == -1 /* The square not found */ ) )
			{
				cb ( &line_ext, context, FLAG_EXTEND_CB_NO_SQUARE );
			}
			break;
	      }
	      if (roadmap_plugin_same_line (line, &line_ext)) {
	      	return count;
	      }
	      count++;
	      if (to) roadmap_plugin_line_to (&line_ext, to);
	      if (cb) {
	      	if (cb (&line_ext, context, FLAG_EXTEND_TO)) break;
	      }

	      roadmap_square_set_current (line_ext.square);
	   }
	}

	return count;
}


int roadmap_street_line_has_predecessor (PluginLine *line) {

	return roadmap_street_extend_line_ends (line, NULL, NULL, FLAG_EXTEND_FROM, NULL, NULL);
}
