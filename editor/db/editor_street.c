/* editor_street.c - street databse layer
 *
 * LICENSE:
 *
 *   Copyright 2005 Ehud Shabtai
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
 *   See editor_street.h
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "roadmap.h"
#include "roadmap_math.h"
#include "roadmap_locator.h"
#include "roadmap_street.h"

#include "../editor_log.h"
#include "../editor_main.h"
#include "../editor_plugin.h"

#include "editor_db.h"
#include "editor_shape.h"
#include "editor_line.h"
#include "editor_dictionary.h"
#include "editor_trkseg.h"
#include "editor_point.h"

#include "editor_street.h"

static editor_db_section *ActiveStreetDB;

static void editor_street_activate (editor_db_section *context) {

   ActiveStreetDB = context;
}
 
editor_db_handler EditorStreetHandler = {
   EDITOR_DB_STREETS,
   sizeof (editor_db_street),
   0,
   editor_street_activate
};

int editor_street_create (const char *_name,
                          const char *_type,
                          const char *_prefix,
                          const char *_suffix,
                          const char *_city,
                          const char *_t2s) {

   editor_db_street street;

   street.fename = editor_dictionary_add (_name);
   street.fetype = editor_dictionary_add (_type);
   street.fedirp = editor_dictionary_add (_prefix);
   street.fedirs = editor_dictionary_add (_suffix);
   street.city = editor_dictionary_add (_city);
   street.t2s = editor_dictionary_add (_t2s);

   //TODO optimize - this is a linear search

   if (street.fename != -1) {
      int i;

      for (i = editor_db_get_item_count (ActiveStreetDB) - 1; i >= 0; i--) {
         editor_db_street *street_ptr =
            (editor_db_street *) editor_db_get_item
                        (ActiveStreetDB, i, 0, NULL);

         assert (street_ptr != NULL);

         if ((street_ptr != NULL) &&
               !memcmp (street_ptr, &street, sizeof (editor_db_street))) {
            return i;
         }
      }
   }
   
   return editor_db_add_item (ActiveStreetDB, &street, 1);
}


static int editor_street_get_distance_with_shape
              (const RoadMapPosition *position,
               int line,
               int square,
               int fips,
               RoadMapNeighbour *neighbours,
               int max) {

   RoadMapPosition from;
   RoadMapPosition to;
   int first_shape;
   int last_shape;
   int cfcc;
   int trkseg;
   int i;
   RoadMapNeighbour current;
   int found = 0;

   editor_line_get (line, &from, &to, &trkseg, &cfcc, NULL);

   roadmap_plugin_set_line (&current.line, EditorPluginID, line, cfcc, square, fips);
   current.from = from;

   editor_trkseg_get (trkseg, &i, &first_shape, &last_shape, NULL);
   editor_point_position (i, &current.to);

   if (first_shape == -1) {
      /* skip the for loop */
      last_shape = -2;
   }
      
   for (i = first_shape; i <= last_shape; i++) {

      editor_shape_position (i, &current.to);

      if (roadmap_math_line_is_visible (&current.from, &current.to)) {

         current.distance =
            roadmap_math_get_distance_from_segment
               (position, &current.from,
                &current.to, &current.intersection, NULL);

			found = roadmap_street_replace (neighbours, found, max, &current);
      }

      current.from = current.to;
   }

   current.to = to;

   if (roadmap_math_line_is_visible (&current.from, &current.to)) {
      current.distance =
         roadmap_math_get_distance_from_segment
            (position, &current.to, &current.from, &current.intersection, NULL);

		found = roadmap_street_replace (neighbours, found, max, &current);
   }

   return found;
}


int editor_street_get_distance (const RoadMapPosition *position,
                                const PluginLine *line,
                                RoadMapNeighbour *result) {

   return editor_street_get_distance_with_shape
      (position,
       roadmap_plugin_get_line_id (line),
       roadmap_plugin_get_square (line),
       roadmap_plugin_get_fips (line),
       result,
       1);

}


int editor_street_get_closest (const RoadMapPosition *position,
                               int *categories,
                               int categories_count,
                               int max_shapes,
                               RoadMapNeighbour *neighbours,
                               int count,
                               int max) {
   
   int i;
   int min_category = 256;
   int lines_count;
   int found = 0;
   int fips = roadmap_locator_active ();
   int line;
   RoadMapNeighbour this[3];
   int max_possible_shapes = (int)(sizeof(this) / sizeof(this[0]));
   
   if (max_shapes > max_possible_shapes) {
   	max_shapes = max_possible_shapes;
   }


	if (!editor_plugin_get_override ()) return count;
	
   for (i = 0; i < categories_count; ++i) {
        if (min_category > categories[i]) min_category = categories[i];
   }

	lines_count = editor_line_get_count ();

   if (! (-1 << min_category)) {
      return count;
   }

   for (line = 0; line < lines_count; line++) {

      RoadMapPosition from;
      RoadMapPosition to;
      int cfcc;
      int flag;

      editor_line_get (line, &from, &to, NULL, &cfcc, &flag);

      if (flag & ED_LINE_DELETED) continue;
      if (cfcc < min_category) continue;

      found = editor_street_get_distance_with_shape
                           (position, line, -1, fips, this, max_shapes);

      for (i = 0; i < found; i++) {
         count = roadmap_street_replace (neighbours, count, max, this + i);
      }
    }

   return count;
}

int editor_street_get_connected_lines (const RoadMapPosition *crossing,
                                       PluginLine *plugin_lines,
                                       int size) {

   int lines_count;
   int count = 0;
   int line;

   /* FIXME - this is wrong */
   int fips = roadmap_locator_active ();
   
   if (editor_db_activate (fips) == -1) return 0;

   //FIXME:
#if 0
   if (! (square_cfccs && (-1 << ROADMAP_ROAD_FIRST))) {
      return count;
   }
#endif
   lines_count = editor_line_get_count ();
   
   for (line=0; line<lines_count; line++) {

      RoadMapPosition line_from;
      RoadMapPosition line_to;
      int cfcc;
      int flag;

      editor_line_get
         (line, &line_from, &line_to, NULL, &cfcc, &flag);

      if (flag & ED_LINE_DELETED) continue;
      //if (cfcc < ROADMAP_ROAD_FIRST) continue;

      if ((line_from.latitude != crossing->latitude) ||
          (line_from.longitude != crossing->longitude)) {

          if ((line_to.latitude != crossing->latitude) ||
              (line_to.longitude != crossing->longitude)) {

              continue;
          }
      }

      roadmap_plugin_set_line
         (&plugin_lines[count++], EditorPluginID, line, cfcc, -1, fips);

      if (count >= size) return count;
   }

   return count;
}


const char *editor_street_get_full_name
               (int street_id) {

    static char RoadMapStreetName [512];

    const char *address;
    const char *city;


    address = editor_street_get_street_address (street_id);
    city    = editor_street_get_street_city
                  (street_id);
    
    snprintf (RoadMapStreetName, sizeof(RoadMapStreetName),
              "%s%s%s%s%s",
              address,
              (address[0])? " " : "",
              editor_street_get_street_name (street_id),
              (city[0])? ", " : "",
              city);
    
    return RoadMapStreetName;
}


const char *editor_street_get_street_address
               (int street_id) {

#if 0
    static char RoadMapStreetAddress [32];
    int min;
    int max;

    if (properties->first_range.fradd == -1) {
        return "";
    }

    if (properties->first_range.fradd > properties->first_range.toadd) {
       min = properties->first_range.toadd;
       max = properties->first_range.fradd;
    } else {
       min = properties->first_range.fradd;
       max = properties->first_range.toadd;
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
#endif
	return "";    
}


const char *editor_street_get_street_name
               (int street_id) {
   
   static char name[512];
   const char *fename = editor_street_get_street_fename (street_id);
   const char *fetype = editor_street_get_street_fetype (street_id);

	sprintf (name, "%s%s%s",
				fetype,
				*fetype ? " " : "",
				fename);
				
   return name;
}


const char *editor_street_get_street_fename
               (int street_id) {
   
   editor_db_street *street;
   
   if (street_id < 0) return "";
   
   street =
      (editor_db_street *) editor_db_get_item
                              (ActiveStreetDB, street_id, 0, NULL);
   assert (street != NULL);

   if (street->fename < 0) {
      return "";
   }

   return editor_dictionary_get (street->fename);
}


const char *editor_street_get_street_fetype
               (int street_id) {
   
   editor_db_street *street;
   
   if (street_id < 0) return "";
   
   street =
      (editor_db_street *) editor_db_get_item
                              (ActiveStreetDB, street_id, 0, NULL);
   assert (street != NULL);

   if (street->fename < 0) {
      return "";
   }

   return editor_dictionary_get (street->fetype);
}


const char *editor_street_get_street_city
                (int street_id) {

   editor_db_street *street;
   
   if (street_id < 0) return "";
   
   street =
      (editor_db_street *) editor_db_get_item
                              (ActiveStreetDB, street_id, 0, NULL);
   assert (street != NULL);

   if (street->fename < 0) {
      return "";
   }

   return editor_dictionary_get (street->city);
}


const char *editor_street_get_street_t2s
               (int street_id) {

   editor_db_street *street;
   
   if (street_id < 0) return "";
   
   street =
      (editor_db_street *) editor_db_get_item
                              (ActiveStreetDB, street_id, 0, NULL);
   assert (street != NULL);

   if (street->t2s < 0) {
      return "";
   }

   return editor_dictionary_get (street->t2s);
}

