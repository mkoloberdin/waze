/* editor_marker.c - marker databse layer
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
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
 *   See editor_marker.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "roadmap.h"
#include "roadmap_path.h"
#include "roadmap_square.h"
#include "roadmap_tile.h"

#include "../editor_log.h"

#include "editor_db.h"
#include "editor_marker.h"

#define MAX_MARKER_TYPES 10

static EditorMarkerType *MarkerTypes[MAX_MARKER_TYPES];
static int MarkerTypesCount;

static editor_db_section *ActiveMarkersDB;

static void editor_marker_activate (editor_db_section *section) {
   ActiveMarkersDB = section;
}

editor_db_handler EditorMarkersHandler = {
   EDITOR_DB_MARKERS,
   sizeof(editor_db_marker),
   1,
   editor_marker_activate
};

int editor_marker_add(int longitude,
                      int latitude,
                      int steering,
                      time_t time,
                      unsigned char type,
                      unsigned char flags,
                      const char *note,
                      const char *icon) {
   
   editor_db_marker marker;
   int id;
   RoadMapPosition pos;
   
	while (steering < 0) {
		steering += 360;
	}
	while (steering >= 360) {
		steering -= 360;
	}
	
	pos.longitude = longitude;
	pos.latitude = latitude;
	marker.tile_timestamp = (int)roadmap_square_timestamp (roadmap_tile_get_id_from_position (0, &pos));
	
   marker.longitude = longitude;
   marker.latitude  = latitude;
   marker.steering  = steering;
   marker.time      = (int)time;
   marker.type      = type;
   marker.flags     = flags | ED_MARKER_DIRTY;

   if (note == NULL) {
      marker.note = ROADMAP_INVALID_STRING;
   } else {
      
      marker.note = editor_dictionary_add(note);
   }

   if (icon == NULL) {
     marker.icon = ROADMAP_INVALID_STRING;
   } else {
      
      marker.icon = editor_dictionary_add(icon);
   }

   id = editor_db_add_item (ActiveMarkersDB, &marker, 1);

   return id;
}


int  editor_marker_count(void) {

   if (!ActiveMarkersDB) return 0;

   return editor_db_get_item_count (ActiveMarkersDB);
}


void editor_marker_position(int marker,
                            RoadMapPosition *position, int *steering) {

   editor_db_marker *marker_st =
      editor_db_get_item (ActiveMarkersDB, marker, 0, 0);
   assert(marker_st != NULL);

   position->longitude = marker_st->longitude;
   position->latitude = marker_st->latitude;

   if (steering) *steering = marker_st->steering;
}


int editor_marker_is_obsolete(int marker) {
	
   editor_db_marker *marker_st =
      editor_db_get_item (ActiveMarkersDB, marker, 0, 0);
   RoadMapPosition pos;
   
   assert(marker_st != NULL);

	pos.longitude = marker_st->longitude;
	pos.latitude = marker_st->latitude;
	return (int)roadmap_square_timestamp (roadmap_tile_get_id_from_position (0, &pos)) > marker_st->tile_timestamp;
}


const char *edit_marker_icon(int marker){

   editor_db_marker *marker_st =
      editor_db_get_item (ActiveMarkersDB, marker, 0, 0);
   assert(marker_st != NULL);

   if (marker_st->icon == ROADMAP_INVALID_STRING) {
      return "";
   } else {

      return editor_dictionary_get(marker_st->icon);
   }
}

const char *editor_marker_type(int marker) {
   
   editor_db_marker *marker_st =
      editor_db_get_item (ActiveMarkersDB, marker, 0, 0);
   assert(marker_st != NULL);

   return MarkerTypes[marker_st->type]->name;
}
   

const char *editor_marker_note(int marker) {
   
   editor_db_marker *marker_st =
      editor_db_get_item (ActiveMarkersDB, marker, 0, 0);
   assert(marker_st != NULL);

   if (marker_st->note == ROADMAP_INVALID_STRING) {
      return "";
   } else {

      return editor_dictionary_get(marker_st->note);
   }
}


time_t editor_marker_time(int marker) {
   
   editor_db_marker *marker_st =
      editor_db_get_item (ActiveMarkersDB, marker, 0, 0);
   assert(marker_st != NULL);

   return marker_st->time;
}


unsigned char editor_marker_flags(int marker) {
   
   editor_db_marker *marker_st =
      editor_db_get_item (ActiveMarkersDB, marker, 0, 0);
   assert(marker_st != NULL);

   return marker_st->flags;
}


int editor_marker_committed (int marker) {

	return editor_db_item_committed (ActiveMarkersDB, marker);	
}


void editor_marker_update(int marker, unsigned char flags,
                          const char *note) {

   editor_db_marker *marker_st =
      editor_db_get_item (ActiveMarkersDB, marker, 0, 0);

   int dirty = 0;
   assert(marker_st != NULL);

   if (note == NULL) {
      if (marker_st->note != ROADMAP_INVALID_STRING) {
         dirty++;
         marker_st->note = ROADMAP_INVALID_STRING;
      }
   } else if ((marker_st->note == ROADMAP_INVALID_STRING) ||
               strcmp(editor_dictionary_get(marker_st->note),
                      note)) {

      dirty++;
      marker_st->note = editor_dictionary_add(note);
   }

   if ((marker_st->flags & ~ED_MARKER_DIRTY) !=
                  (flags & ~ED_MARKER_DIRTY)) {
      dirty++;
   }

   marker_st->flags = flags;

   if (dirty) editor_db_update_item(ActiveMarkersDB, marker);
}


int editor_marker_reg_type(EditorMarkerType *type) {

   int id = MarkerTypesCount;

   if (MarkerTypesCount == MAX_MARKER_TYPES) return -1;

   MarkerTypes[MarkerTypesCount] = type;
   MarkerTypesCount++;

   return id;
}


void editor_marker_voice_file(int marker, char *file, int size) {
   
   char path[512];
   char file_name[100];

   roadmap_path_format (path, sizeof (path), roadmap_path_user (), "markers");
   roadmap_path_create (path);
   snprintf (file_name, sizeof(file_name), "voice_%d.wav", marker);

   roadmap_path_format (file, size, path, file_name);
}


int editor_marker_export(int marker, const char **name, const char **description,
                         const char *keys[ED_MARKER_MAX_ATTRS],
                         char       *values[ED_MARKER_MAX_ATTRS],
                         int *count) {
   
   editor_db_marker *marker_st =
      editor_db_get_item (ActiveMarkersDB, marker, 0, 0);
   
   assert(marker_st != NULL);
   
   return MarkerTypes[marker_st->type]->export_marker (marker,
                                                       name,
                                                       description,
                                                       keys,
                                                       values,
                                                       count);
}


int editor_marker_verify(int marker, unsigned char *flags, const char **note) {

   editor_db_marker *marker_st =
      editor_db_get_item (ActiveMarkersDB, marker, 0, 0);
   
   assert(marker_st != NULL);
   
   return MarkerTypes[marker_st->type]->update_marker (marker, flags, note);
}


int editor_marker_begin_commit (void) {

	return editor_db_begin_commit (ActiveMarkersDB);	
}


void editor_marker_confirm_commit (int id) {

	editor_db_confirm_commit (ActiveMarkersDB, id);	
}


int editor_marker_item_committed (int item_id) {

	return editor_db_item_committed (ActiveMarkersDB, item_id);	
}


int editor_marker_items_pending (void) {

	return editor_db_items_pending (ActiveMarkersDB);	
}
