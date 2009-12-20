/* RealtimeOffline.c - Save server data to file
 *
 * LICENSE:
 *
 *   Copyright 2008 Israel Disatnik
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License V2 as published by
 *   the Free Software Foundation.
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
 *
 */

#include "RealtimeOffline.h"
#include "Realtime.h"
#include "roadmap_file.h"
#include "roadmap_path.h"
#include "roadmap_dbread.h"
#include <string.h>

static RoadMapFile	OfflineFile = ROADMAP_INVALID_FILE;
static const char 	*OfflineFileName = NULL;


static const char						*gs_OfflinePrefix[] = {
	"Auth",
	"GPSPath",
	"GPSDisconnect",
	"NodePath",
	"SubmitMarker",
	"SubmitSegment",
	"CreateNewRoads"
};

#define NUM_OFFLINE_PREFIX		((int)(sizeof (gs_OfflinePrefix) / sizeof (gs_OfflinePrefix[0])))

void		Realtime_OfflineOpen (const char *path, const char *filename) {

	Realtime_OfflineClose ();

	if (path) {
		OfflineFileName = roadmap_path_join (path, filename);
	} else {
		OfflineFileName = roadmap_path_join (roadmap_db_map_path(), filename);
	}
}


static void Realtime_OfflineOpenFile (void) {
	
	if (OfflineFileName && !ROADMAP_FILE_IS_VALID (OfflineFile)) {
		
		OfflineFile = roadmap_file_open (OfflineFileName, "a");
		if (ROADMAP_FILE_IS_VALID (OfflineFile)) {
			RealTime_Auth ();
		}
	}
}


void		Realtime_OfflineClose (void) {

	if (ROADMAP_FILE_IS_VALID (OfflineFile)) {
		roadmap_file_close (OfflineFile);
	}
	OfflineFile = ROADMAP_INVALID_FILE;
	
	if (OfflineFileName) {
		roadmap_path_free (OfflineFileName);
		OfflineFileName = NULL;
	}
}


static void	Realtime_OfflineWriteLine (const char *line, int len) {

	int i;
	
	for (i = 0; i < NUM_OFFLINE_PREFIX; i++) {
		if (strncmp (line, gs_OfflinePrefix[i], strlen (gs_OfflinePrefix[i])) == 0) {
		
			Realtime_OfflineOpenFile ();
			if (!ROADMAP_FILE_IS_VALID (OfflineFile)) return;
			roadmap_file_write (OfflineFile, line, len);
			roadmap_file_write (OfflineFile, "\n", 1);
		} 
	}
}


void		Realtime_OfflineWrite (const char *packet) {

	const char *newline;
	
	newline = strchr (packet, '\n');
	
	while (newline) {
		Realtime_OfflineWriteLine (packet, newline - packet);
		packet += newline - packet + 1;
		newline = strchr (packet, '\n');
	}
	
	if (strlen (packet)) {
		Realtime_OfflineWriteLine (packet, strlen (packet));
	}
}
