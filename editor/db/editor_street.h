/* editor_street.h - 
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
 */

#ifndef INCLUDE__EDITOR_STREET__H
#define INCLUDE__EDITOR_STREET__H

#include "roadmap.h"
#include "roadmap_street.h"
#include "editor/db/editor_dictionary.h"
#include "editor/db/editor_square.h"

typedef struct editor_db_street_s {
   EditorString fedirp;
   EditorString fename;
   EditorString fetype;
   EditorString fedirs;
   EditorString t2s;
   EditorString city;
} editor_db_street;

int editor_street_create (const char *_name,
                          const char *_type,
                          const char *_prefix,
                          const char *_suffix,
                          const char *_city,
                          const char *_t2s);

int editor_street_get_distance
                 (const RoadMapPosition *position,
                  const PluginLine *line,
                  RoadMapNeighbour *result);

int editor_street_get_closest (const RoadMapPosition *position,
                               int *categories,
                               int categories_count,
                               int max_shapes,
                               RoadMapNeighbour *neighbours,
                               int count,
                               int max);
   
const char *editor_street_get_full_name
                  (int street_id);

const char *editor_street_get_street_address
                  (int street_id);

const char *editor_street_get_street_name
                  (int street_id);

const char *editor_street_get_street_fename
                  (int street_id);

const char *editor_street_get_street_fetype
                  (int street_id);

const char *editor_street_get_street_t2s
                   (int street_id);

const char *editor_street_get_street_city
                   (int street_id);

int editor_street_get_connected_lines (const RoadMapPosition *crossing,
                                       PluginLine *plugin_lines,
                                       int size);

extern editor_db_handler EditorStreetHandler;

#endif // INCLUDE__EDITOR_STREET__H

