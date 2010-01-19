/* stubs.c - Some stubs for Symbian
 *
 * LICENSE:
 *
 *   Copyright 2008 Ehud Shabtai
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
 *   #include "roadmap.h"
 *
 *   void roadmap_log (int level, char *format, ...);
 *
 * This module is used to control and manage the appearance of messages
 * printed by the roadmap program. The goals are (1) to produce a uniform
 * look, (2) have a central point of control for error management and
 * (3) have a centralized control for routing messages.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include "roadmap.h"
#include "roadmap_gps.h"
#include "roadmap_download.h"
#include "roadmap_path.h"
#include "roadmap_pointer.h"
#include "roadmap_serial.h"
#include "roadmap_line.h"
#include "roadmap_file.h"
#include "roadmap_trip.h"
#include "roadmap_gps.h"
#include "roadmap_plugin.h"
#include "roadmap_street.h"
#include "roadmap_navigate.h"

#include "roadmap_res.h"
#include "roadmap_canvas.h"
#include "roadmap_camera_defs.h"
#include "roadmap_login.h"
int roadmap_preferences_use_keyboard() { return 0; }

void roadmap_preferences_edit (void) {
	roadmap_login_details_dialog_show ();
}



time_t timegm(struct tm *tm) { return 0; }

