/* editor_marker.h - database layer
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
 */

#ifndef INCLUDE__EDITOR_MARKER__H
#define INCLUDE__EDITOR_MARKER__H

#include <time.h>
#include "roadmap_types.h"
#include "editor/db/editor_db.h"
#include "editor/db/editor_dictionary.h"

#define ED_MARKER_DIRTY   				0x1
#define ED_MARKER_UPLOAD  			 	0x2
#define ED_MARKER_DELETED 			 	0x4
#define ED_MARKER_MAX_ATTRS 		 	5
#define ED_MARKER_MAX_STRING_SIZE 	100

typedef struct editor_marker_type {
   const char *name;
   int (*export_marker)(int          marker,
                        const char **name,
                        const char **note,
                        const char  *keys[ED_MARKER_MAX_ATTRS],
                        char        *values[ED_MARKER_MAX_ATTRS],
                        int         *count);
   int (*update_marker)(int marker, unsigned char *flags, const char **note);
} EditorMarkerType;

typedef struct editor_db_marker_s {
	int				tile_timestamp;
   int	         time;
   int            longitude;
   int            latitude;
   EditorString   note;
   EditorString   icon;
   short          steering;
   unsigned char  type;
   unsigned char  flags;
} editor_db_marker;


int editor_marker_add (int longitude,
                       int latitude,
                       int steering,
                       time_t time,
                       unsigned char type,
                       unsigned char flags,
                       const char *note,
                       const char *icon);
   
int  editor_marker_count (void);

void editor_marker_position(int marker,
                            RoadMapPosition *position,
                            int *steering);

int			editor_marker_is_obsolete(int marker);
time_t      editor_marker_time(int marker);
const char *editor_marker_type(int marker);
const char *editor_marker_note(int marker);
const char *edit_marker_icon(int marker);

unsigned char editor_marker_flags(int marker);
int editor_marker_committed (int marker);

void editor_marker_update(int marker, unsigned char flags, const char *note);

int editor_marker_reg_type(EditorMarkerType *type);

void editor_marker_voice_file (int marker, char *file, int size);

int editor_marker_export(int marker, const char **name, const char **description,
                         const char *keys[ED_MARKER_MAX_ATTRS],
                         char       *values[ED_MARKER_MAX_ATTRS],
                         int *count);

int editor_marker_verify(int marker, unsigned char *flags, const char **note);

int editor_marker_begin_commit (void);
void editor_marker_confirm_commit (int id);
int editor_marker_item_committed (int item_id);
int editor_marker_items_pending (void);

extern editor_db_handler EditorMarkersHandler;

#endif // INCLUDE__EDITOR_MARKER__H

