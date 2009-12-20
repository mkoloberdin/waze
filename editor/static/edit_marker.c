/* edit_marker.c - Edit / View markers
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
 *   See edit_marker.h
 */

#include <string.h>

#include "roadmap.h"
#include "roadmap_dialog.h"
#include "roadmap_locator.h"
#include "roadmap_trip.h"
#include "roadmap_screen.h"
#include "roadmap_math.h"
#include "roadmap_layer.h"
#include "roadmap_sound.h"
#include "roadmap_file.h"
#include "roadmap_messagebox.h"

#include "../db/editor_db.h"
#include "../db/editor_marker.h"
#include "../editor_log.h"

#include "edit_marker.h"

static const char *yesno[2];

static int fill_dialog (int marker) {

   const char *value;
   RoadMapPosition pos;
   RoadMapGuiPoint point;

   roadmap_dialog_set_data ("Marker", "Type",
                            roadmap_lang_get (editor_marker_type (marker)));

   if (editor_marker_flags (marker) & ED_MARKER_UPLOAD) {
      value = yesno[0];
   } else {
      value = yesno[1];
   }

   roadmap_dialog_set_data ("Marker", "Send to server", value);

   roadmap_dialog_set_data ("Marker", "Note", editor_marker_note (marker));

   editor_marker_position (marker, &pos, NULL);
   
   /* Set zoom to 1:1 */
   roadmap_math_zoom_reset ();
   roadmap_layer_adjust ();

   roadmap_math_coordinate (&pos, &point);
   point.y -= roadmap_canvas_height () / 2 - 15;
   roadmap_math_rotate_coordinates (1, &point);
   roadmap_math_to_position (&point, &pos, 1);
   roadmap_trip_set_point ("Selection", &pos);
   roadmap_trip_set_focus ("Selection");

   roadmap_screen_refresh ();
   return 0;
}


static int edit_marker_update (const char *name, void *context) {

   int *marker = (int *)context;

   const char   *update_server =
                         roadmap_dialog_get_data ("Marker", "Send to server");
   const char   *new_note  = roadmap_dialog_get_data ("Marker", "Note");

   unsigned char new_flags = editor_marker_flags (*marker);
   
   if (!strcmp(update_server, yesno[0])) {
      new_flags |= ED_MARKER_UPLOAD;
   } else {
      new_flags &= ~ED_MARKER_UPLOAD;
   }

   if (editor_marker_verify (*marker, &new_flags, &new_note) == -1) return -1;

   editor_marker_update (*marker, new_flags, new_note);

   return 0;
}


static void edit_marker_prev (const char *name, void *context) {

   int *marker = (int *)context;

   if (*marker > 0) {
      if (edit_marker_update (name, context) == -1) return;

      (*marker)--;
      fill_dialog (*marker);
   }
}


static void edit_marker_next (const char *name, void *context) {

   int *marker = (int *)context;

   if (*marker < (editor_marker_count () - 1)) {
      if (edit_marker_update (name, context) == -1) return;

      (*marker)++;
      fill_dialog (*marker);
   }
}


static void edit_marker_play (const char *name, void *context) {

   char file[256];
   int *marker = (int *)context;

   editor_marker_voice_file (*marker, file, sizeof(file));

   if (!roadmap_file_exists (NULL, file)) {
      roadmap_messagebox ("Error",
                          "There is no recording associated with this note.");
      return;
   } else {

      roadmap_sound_play_file (file);
   }
}


static void edit_marker_close (const char *name, void *context) {

   if (edit_marker_update (name, context) == -1) return;
   roadmap_dialog_hide (name);
}


void edit_marker_dialog (int marker) {

   static int context;
   int fips = roadmap_locator_active ();

   if (!yesno[0]) {
      yesno[0] = roadmap_lang_get ("Yes");
      yesno[1] = roadmap_lang_get ("No");
   }

   if ((editor_db_activate(fips) == -1) ||
         !editor_marker_count ()) {

      roadmap_messagebox ("Info", "There are no markers.");
      return;
   }

   context = marker;

   if (roadmap_dialog_activate ("View markers", &context, 1)) {

      roadmap_dialog_new_label ("Marker", "Type");

      roadmap_dialog_new_choice ("Marker", "Send to server", 2,
                                 (const char **)yesno,
                                 (void**)yesno, NULL);

      roadmap_dialog_new_mul_entry ("Marker", "Note", NULL);

      roadmap_dialog_add_button ("Back", edit_marker_prev);
      roadmap_dialog_add_button ("Next", edit_marker_next);
      roadmap_dialog_add_button ("Play", edit_marker_play);
      roadmap_dialog_add_button ("Close", edit_marker_close);

      roadmap_dialog_complete (0);
   }

   fill_dialog (marker);
}


void edit_markers_dialog (void) {

   edit_marker_dialog (editor_marker_count () - 1);
}

