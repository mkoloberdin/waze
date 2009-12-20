/* roadmap_street.h - RoadMap streets operations and attributes.
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
 */

#ifndef _ROADMAP_STREET__H_
#define _ROADMAP_STREET__H_

#include "roadmap_types.h"
#include "roadmap_plugin.h"
#include "roadmap_dbread.h"
#include "roadmap_dictionary.h"
#include "roadmap_range.h"

typedef struct {

	int square;
   int line;
   int street;
   RoadMapString city;
	RoadMapStreetRange range[2];
} RoadMapBlocks;

typedef struct {

    RoadMapStreetRange first_range;
    RoadMapStreetRange second_range;

    int street;

    RoadMapString city;

} RoadMapStreetProperties;

#define ROADMAP_STREET_NOPROPERTY {{0, 0}, 0, 0}

typedef struct {

    int fips;
    int square;
    int line1;
    int line2;
    RoadMapPosition position;

} RoadMapStreetIntersection;


typedef struct RoadMapNeighbour_t {

    PluginLine line;
    int distance;
    RoadMapPosition from;
    RoadMapPosition to;
    RoadMapPosition intersection;

} RoadMapNeighbour;

#define	FLAG_EXTEND_FROM			0x0001
#define	FLAG_EXTEND_TO				0x0002
#define	FLAG_EXTEND_BOTH			(FLAG_EXTEND_FROM | FLAG_EXTEND_TO)
#define	FLAG_EXTEND_CB_NO_SQUARE	0x0004		// Indicates if callback has to be called if the square is not available

typedef int (*RoadMapStreetIterCB) (const PluginLine *line, void *context, int extend_flags);

#define ROADMAP_NEIGHBOUR_NULL {PLUGIN_LINE_NULL, -1, {0,0}, {0,0}, {0,0}}


/* The function roadmap_street_blocks either returns a positive count
 * of matching blocks on success, or else an error code (null or negative).
 * The values below are the possible error codes:
 */
#define ROADMAP_STREET_NOADDRESS    0
#define ROADMAP_STREET_NOCITY      -1
#define ROADMAP_STREET_NOSTREET    -2

#define ROADMAP_STREET_LEFT_SIDE    1
#define ROADMAP_STREET_RIGHT_SIDE   2

int roadmap_street_blocks_by_city
       (const char *street_name, const char *city_name,
        RoadMapBlocks *blocks,
        int size);

int roadmap_street_blocks_by_zip
       (const char *street_name, int zip,
        RoadMapBlocks *blocks,
        int size);

int roadmap_street_get_ranges (RoadMapBlocks *blocks,
                               int count,
                               RoadMapStreetRange *ranges);

int roadmap_street_get_position (RoadMapBlocks *blocks,
                                 int number,
                                 RoadMapPosition *position);

int roadmap_street_get_distance
       (const RoadMapPosition *position, int line,
	int cfcc, RoadMapNeighbour *result);

int roadmap_street_get_closest
       (const RoadMapPosition *position, int scale, int *categories, int categories_count,
        int max_shapes, RoadMapNeighbour *neighbours, int max);

int roadmap_street_intersection (const char *state,
                                 const char *street1_name,
                                 const char *street2_name,
                                 RoadMapStreetIntersection *intersection,
                                 int count);


void roadmap_street_get_properties
        (int line, RoadMapStreetProperties *properties);

void roadmap_street_get_street (int line, RoadMapStreetProperties *properties);

const char *roadmap_street_get_street_address
                (const RoadMapStreetProperties *properties);

const char *roadmap_street_get_street_name
                (const RoadMapStreetProperties *properties);

const char *roadmap_street_get_city_name
                (const RoadMapStreetProperties *properties);

const char *roadmap_street_get_full_name
                (const RoadMapStreetProperties *properties);

const char *roadmap_street_get_street_fename
                (const RoadMapStreetProperties *properties);
const char *roadmap_street_get_street_fetype
                (const RoadMapStreetProperties *properties);
const char *roadmap_street_get_street_fedirs
                (const RoadMapStreetProperties *properties);
const char *roadmap_street_get_street_fedirp
                (const RoadMapStreetProperties *properties);
const char *roadmap_street_get_street_t2s
                (const RoadMapStreetProperties *properties);

const char *roadmap_street_get_street_city
                (const RoadMapStreetProperties *properties, int side);

const char *roadmap_street_get_street_zip
                (const RoadMapStreetProperties *properties, int side);

void roadmap_street_get_street_range
    (const RoadMapStreetProperties *properties, int range, int *from, int *to);

int roadmap_street_replace
              (RoadMapNeighbour *neighbours, int count, int max,
               const RoadMapNeighbour *this);

int  roadmap_street_search (const char *city, const char *str,
                            int max_results,
                            RoadMapDictionaryCB cb,
                            void *data);

int roadmap_street_search_city (const char *str, RoadMapDictionaryCB cb,
                                void *data);

void roadmap_street_update_city_index (void);

int roadmap_street_extend_line_ends
			(const PluginLine *line, RoadMapPosition *from, RoadMapPosition *to,
			 int flags, RoadMapStreetIterCB cb, void *context);
int roadmap_street_line_has_predecessor (PluginLine *line);

extern roadmap_db_handler RoadMapStreetHandler;
extern roadmap_db_handler RoadMapZipHandler;
extern roadmap_db_handler RoadMapRangeHandler;

#endif // _ROADMAP_STREET__H_
