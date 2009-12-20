/* editor_report.c - export db for reporting
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
 *   See editor_export.h
 */

#include "time.h"

#include "roadmap.h"


#include "../db/editor_line.h"
#include "../db/editor_marker.h"

#include "Realtime/Realtime.h"

#include "editor_report.h"


static int EditorReportMarkersId = -1;
static int EditorReportSegmentsId = -1;
static int EditorReportMarkersInProgress = 0;
static int EditorReportSegmentsInProgress = 0;


static void editor_report_markers_cb (BOOL bDetailsVerified, roadmap_result rc) {

	EditorReportMarkersInProgress = 0;
	if (bDetailsVerified) {
		editor_marker_confirm_commit (EditorReportMarkersId);	
		editor_report_markers ();
	}
}


void editor_report_markers (void) {
	
	if (EditorReportMarkersInProgress || !editor_marker_items_pending ()) return;
	
	EditorReportMarkersId = editor_marker_begin_commit ();
	EditorReportMarkersInProgress = Realtime_Editor_ExportMarkers (editor_report_markers_cb);
}


static void editor_report_segments_cb (BOOL bDetailsVerified, roadmap_result rc) {

	EditorReportSegmentsInProgress = 0;
	if (bDetailsVerified) {
		editor_line_confirm_commit (EditorReportSegmentsId);	
		editor_report_segments ();
	}
}


void editor_report_segments (void) {
	
	if (EditorReportSegmentsInProgress || !editor_line_items_pending ()) return;
	
	EditorReportSegmentsId = editor_line_begin_commit ();
	EditorReportSegmentsInProgress = Realtime_Editor_ExportSegments (editor_report_segments_cb);
}
