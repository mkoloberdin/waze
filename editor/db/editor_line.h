/* editor_line.h - database layer
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

#ifndef INCLUDE__EDITOR_LINE__H
#define INCLUDE__EDITOR_LINE__H

#include "roadmap_types.h"
#include "editor/db/editor_db.h"
#include "roadmap_plugin.h"

#define ED_LINE_DELETED         0x1 /* flag */
#define ED_LINE_EXPLICIT_SPLIT  0x2 /* flag */
#define ED_LINE_DIRTY           0x4 /* flag */
#define ED_LINE_CONNECTION      0x8
#define ED_LINE_NEW_DIRECTION   0x10


typedef struct editor_db_line_s {
	int update_timestamp;
   int point_from;
   int point_to;
   int trkseg;
   int cfcc;
   int flags;
   int direction;
   int street;
} editor_db_line;


int editor_line_add
         (int p_from,
          int p_to,
          int trkseg,
          int cfcc,
          int direction,
          int street,
          int flag);

void editor_line_get (int line,
                      RoadMapPosition *from,
                      RoadMapPosition *to,
                      int *trkseg,
                      int *cfcc,
                      int *flag);

void editor_line_get_points (int line, int *from, int *to);

int editor_line_length (int line);

void editor_line_modify_properties (int line,
                                    int cfcc,
                                    int flags);

void editor_line_set_flag (int line, int flag);
void editor_line_reset_flag (int line, int flag);

int editor_line_get_count (void);

int editor_line_get_timestamp (int line_id);
int editor_line_is_valid (int line_id);
void editor_line_invalidate (int line_id);
int editor_line_get_update_time (int line_id);

int editor_line_get_street (int line, int *street);
int editor_line_set_street (int line, int street);

int editor_line_get_direction (int line, int *direction);
int editor_line_set_direction (int line, int direction);

int editor_line_get_cross_time (int line, int direction);
int editor_line_mark_dirty (int line_id);

int editor_line_committed (int line_id);
int editor_line_begin_commit (void);
void editor_line_confirm_commit (int id);
int editor_line_items_pending (void);

int editor_line_copy (PluginLine *line, int street);


extern editor_db_handler EditorLinesHandler;

#endif // INCLUDE__EDITOR_LINE__H

