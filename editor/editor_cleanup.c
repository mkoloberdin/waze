/* editor_cleanup.c - cleanup editor db when possible
 *
 * LICENSE:
 *
 *   Copyright 2009 Israel Disatnik
 *
 *   This file is part of Waze.
 *
 *   Waze is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   Waze is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Waze; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdlib.h>

#include "../roadmap.h"
#include "../roadmap_locator.h"
#include "../roadmap_tile_manager.h"
#include "../roadmap_tile.h"

#include "editor_cleanup.h"
#include "editor_main.h"

#include "db/editor_db.h"
#include "db/editor_line.h"
#include "db/editor_marker.h"
#include "db/editor_override.h"


static int editor_has_relevant_lines (int min_time) {
	
	int count = editor_line_get_count ();
	int i;
	int rc = 0;
	
	for (i = 0; i < count; i++) {
		
		if (editor_line_is_valid (i)) {
			int line_timestamp = editor_line_get_timestamp (i);
			if (line_timestamp >= min_time) {
				if (line_timestamp > editor_line_get_update_time (i)) {
					rc = 1;
				}
			} else {
				editor_line_invalidate (i);
			}
		}
	}
	
	return rc;
}

static int editor_has_relevant_markers (void) {
	
	int count = editor_marker_count ();
	int i;
	
	for (i = 0; i < count; i++) {
	
		if (!editor_marker_is_obsolete (i)) {
			RoadMapPosition pos;
			
			// this makes sure we identify an obsolete marker,
			// even if we don't get to its square
			editor_marker_position (i, &pos, NULL);
			roadmap_tile_request (roadmap_tile_get_id_from_position (0, &pos), 0, 1, NULL);
			return 1;
		}	
	}
	
	return 0;
}

static int editor_has_relevant_overrides (void) {
	
	int count = editor_override_get_count ();
	int i;
	int square;
	
	for (i = 0; i < count; i++) {
		
		if (editor_override_get (i, NULL, &square, NULL, NULL)) {
		
			// this makes sure we identify an obsolete override,
			// even if we don't get to its square
			roadmap_tile_request (square, 0, 1, NULL);
			return 1;	
		}
	}
	
	return 0;
}

void	editor_cleanup_test (int timestamp) {

	int fips;
	
	if (editor_has_relevant_lines (timestamp) ||
		 editor_has_relevant_markers () ||
		 editor_has_relevant_overrides ()) {
		 	
		return;
	}
	
	fips = roadmap_locator_active ();
	editor_db_close (fips);
	editor_db_delete (fips);
	editor_db_activate (fips);	
}
