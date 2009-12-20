/* editor_override.h - database layer
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

#ifndef INCLUDE__EDITOR_OVERRIDE__H
#define INCLUDE__EDITOR_OVERRIDE__H

#include "roadmap_types.h"
#include "editor/db/editor_db.h"

typedef struct editor_db_override_s {
   int square;
   int timestamp;
   int line;
   int flags;
   int direction;
} editor_db_override;

int editor_override_get_count (void);
int editor_override_get (int index, int *line_id, int *square_id, int *direction, int *flags);
int editor_override_line_get_flags (int line, int square, int *flags);
int editor_override_line_set_flag (int line, int square, int flags);
int editor_override_line_reset_flag (int line, int square, int flags);
int editor_override_line_get_direction (int line, int square, int *direction);
int editor_override_line_set_direction (int line, int square, int direction);
int editor_override_exists (int line, int square);

extern editor_db_handler EditorOverrideHandler;

#endif // INCLUDE__EDITOR_OVERRIDE__H

