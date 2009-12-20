/* notes.c - Add notes as markers on map.
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
 *   See notes.h
 */

#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_gps.h"
#include "roadmap_locator.h"
#include "roadmap_county.h"
#include "roadmap_navigate.h"
#include "roadmap_sound.h"
#include "roadmap_config.h"
#include "roadmap_trip.h"
#include "roadmap_lang.h"
#include "roadmap_math.h"
#include "roadmap_adjust.h"
#include "roadmap_layer.h"
#include "roadmap_dialog.h"
#include "roadmap_messagebox.h"

#include "../db/editor_db.h"
#include "../db/editor_marker.h"
#include "../editor_main.h"
#include "../editor_log.h"
#include "edit_marker.h"
#include "../export/editor_report.h"

#include "notes.h"

#define NOTES_MODE_QUICK 1
#define NOTES_MODE_VOICE 2
#define NOTES_MODE_EDIT  3

static const char *yesno[2];

static RoadMapConfigDescriptor ConfigVoiceLength =
                        ROADMAP_CONFIG_ITEM("Notes", "Recording length");

static int note_export(int marker,
                       const char **name,
                       const char **description,
                       const char  *keys[ED_MARKER_MAX_ATTRS],
                       char        *values[ED_MARKER_MAX_ATTRS],
                       int         *count) {
   
   *count = 0;
   *name = NULL;
   *description = editor_marker_note (marker);
   
   return 0;
}


static int note_verify(int marker,
                       unsigned char *flags,
                       const char **note) {
   return 0;
}


static int NotesMarkerType;
static EditorMarkerType NotesMarker = {
   "User note",
   note_export,
   note_verify
};


static void notes_dialog_cancel (const char *name, void *context) {

   roadmap_trip_remove_point ("New note");
   free(context);
   roadmap_dialog_hide (name);
}


static void notes_dialog_save (const char *name, void *context) {

   RoadMapGpsPosition *pos = (RoadMapGpsPosition *)context;

   const char   *update_server =
                         roadmap_dialog_get_data ("Notes", "Send to server");
   const char   *note  = roadmap_dialog_get_data ("Notes", "Note");

   int flags = 0;

   if (!strcmp(update_server, yesno[0])) {
      flags = ED_MARKER_UPLOAD;
   }

   if (editor_marker_add (pos->longitude,
                          pos->latitude,
                          pos->steering,
                          time(NULL),
                          NotesMarkerType,
                          flags, note, NULL) == -1) {

      roadmap_messagebox ("Error", "Can't save note.");
   } else {
		editor_report_markers ();
   }

   roadmap_trip_remove_point ("New note");
   free(context);
   roadmap_dialog_hide (name);
}


static void notes_add_dialog (const RoadMapGpsPosition *pos) {

   RoadMapGpsPosition *note_pos;
   RoadMapPosition     position;
   RoadMapGuiPoint     point;

   if (!yesno[0]) {
      yesno[0] = roadmap_lang_get ("Yes");
      yesno[1] = roadmap_lang_get ("No");
   }

   note_pos = malloc(sizeof(*note_pos));
   roadmap_check_allocated(note_pos);
   *note_pos = *pos;

   if (roadmap_dialog_activate ("Add note", note_pos, 1)) {

      roadmap_dialog_new_label ("Notes", "Type");

      roadmap_dialog_new_choice ("Notes", "Send to server", 2,
                                 (const char **)yesno,
                                 (void**)yesno, NULL);

      roadmap_dialog_new_mul_entry ("Notes", "Note", NULL);

      roadmap_dialog_add_button ("Cancel", notes_dialog_cancel);
      roadmap_dialog_add_button ("Save", notes_dialog_save);

      roadmap_dialog_complete (0);

      roadmap_dialog_set_data ("Notes", "Type",
                            roadmap_lang_get (NotesMarker.name));
   }

   roadmap_dialog_set_data ("Notes", "Send to server", yesno[0]);

   /* Move screen to show the new note location */
   /* Set zoom to 1:1 */
   roadmap_math_zoom_reset ();
   roadmap_layer_adjust ();

   roadmap_adjust_position (pos, &position);
   roadmap_trip_set_point ("New note", &position);
   roadmap_math_coordinate (&position, &point);
   point.y -= roadmap_canvas_height () / 2 - 15;
   roadmap_math_rotate_coordinates (1, &point);
   roadmap_math_to_position (&point, &position, 1);
   roadmap_trip_set_point ("Selection", &position);
   roadmap_trip_set_focus ("Selection");

   roadmap_screen_refresh ();
}


static void notes_add(int mode, RoadMapPosition *point) {

   RoadMapGpsPosition pos;
   PluginLine line;
   int direction;
   int valid_street = 0;
   int fips;
   char file_name[256];
   int marker;

   if (point) {
      pos.longitude = point->longitude;
      pos.latitude  = point->latitude;
      
   } else {
      const char *focus = roadmap_trip_get_focus_name ();

      if (focus && !strcmp(focus, "GPS") &&
         (roadmap_navigate_get_current (&pos, &line, &direction) != -1)) {

         valid_street = 1;
      } else {
         memset (&pos, 0, sizeof(pos));
         roadmap_screen_get_center ((RoadMapPosition *) &pos);
      }
   }

   if (roadmap_county_by_position ((RoadMapPosition *)&pos, &fips, 1) < 1) {
      roadmap_messagebox ("Error", "Can't locate county");
      return;
   }

   if (editor_db_activate (fips) == -1) {

      editor_db_create (fips);

      if (editor_db_activate (fips) == -1) {

         roadmap_messagebox ("Error", "Can't add note");
         return;
      }
   }

   if (mode == NOTES_MODE_EDIT) {
      
      notes_add_dialog (&pos);
      return;
   }

   if ((marker = editor_marker_add (pos.longitude,
                                    pos.latitude,
                                    pos.steering,
                                    time(NULL),
                                    NotesMarkerType,
                                    ED_MARKER_UPLOAD, "", NULL)) == -1) {

      roadmap_messagebox ("Error", "Can't save marker.");
      return;
   }

   switch (mode) {
      case NOTES_MODE_QUICK:
         roadmap_sound_play_file ("rec_end.wav");
         break;
      case NOTES_MODE_VOICE:
         editor_marker_voice_file (marker, file_name, sizeof(file_name));
         roadmap_sound_record (file_name,
                               roadmap_config_get_integer (&ConfigVoiceLength));
         break;
      default:
         roadmap_log (ROADMAP_FATAL, "Invalid note mode: %d", mode);
         break;
   }

	editor_report_markers ();
}


void editor_notes_initialize(void) {
   NotesMarkerType = editor_marker_reg_type (&NotesMarker);

   roadmap_config_declare
       ("preferences", &ConfigVoiceLength,  "10", NULL);
}


void editor_notes_add_quick (void) {
   notes_add (NOTES_MODE_QUICK, NULL);
}


void editor_notes_add_edit (void) {
   notes_add (NOTES_MODE_EDIT, NULL);
}


void editor_notes_add_voice (void) {
   notes_add (NOTES_MODE_VOICE, NULL);
}


