/* editor_square.h - database layer
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

#ifndef INCLUDE__EDITOR_SQUARE__H
#define INCLUDE__EDITOR_SQUARE__H

#include "roadmap_types.h"
#include "roadmap_dbread.h"

#include "editor/db/editor_db.h"

#define MAX_BLOCKS_PER_SQUARE 50

#define EDITOR_DB_LONGITUDE_STEP 10000
#define EDITOR_DB_LATITUDE_STEP 10000

#define ED_SQUARE_NEAR 1

typedef struct editor_db_square_s {
   int cfccs; /* bitmap */
   editor_db_section section;
   int lines[MAX_BLOCKS_PER_SQUARE];
} editor_db_square;

extern roadmap_db_handler EditorSquaresHandler;

void editor_square_add_line
         (int line_id,
          int p_from,
          int p_to,
          int trkseg,
          int cfcc);

void editor_square_view (int *x0, int *y0, int *x1, int *y1);
int editor_square_find (int x, int y);

int editor_square_get_num_lines (int square);
int editor_square_get_line (int square, int i);
int editor_square_get_cfccs (int square);

int editor_square_find_by_position
      (const RoadMapPosition *position, int *squares, int size, int distance);

#endif // INCLUDE__EDITOR_SQUARE__H

