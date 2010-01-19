/* update_range.c - Street range entry
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
 *   See update_range.h
 */

#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_dialog.h"
#include "roadmap_line.h"
#include "roadmap_line_route.h"
#include "roadmap_gps.h"
#include "roadmap_locator.h"
#include "roadmap_county.h"
#include "roadmap_math.h"
#include "roadmap_adjust.h"
#include "roadmap_plugin.h"
#include "roadmap_preferences.h"
#include "roadmap_navigate.h"
#include "roadmap_messagebox.h"

#include "../db/editor_db.h"
#include "../db/editor_line.h"
#include "../db/editor_street.h"
#include "../db/editor_marker.h"
#include "../editor_main.h"
#include "../editor_log.h"
#include "../export/editor_report.h"

#ifdef SSD
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_button.h"
#include "ssd/ssd_keyboard_dialog.h"
#include "ssd/ssd_text.h"
#endif

#include "update_range.h"

#ifdef _WIN32
#define NEW_LINE "\r\n"
#else
#define NEW_LINE "\n"
#endif

#define STREET_PREFIX "Street"
#define CITY_PREFIX   "City"
#define UPDATE_LEFT   "Update left"
#define UPDATE_RIGHT  "Update right"

static RoadMapGpsPosition CurrentGpsPoint;
static RoadMapPosition    CurrentFixedPosition;

static int extract_field(const char *note, const char *field_name,
                         char *field, int size) {

   const char *lang_field_name = roadmap_lang_get (field_name);

   /* Save space for string termination */
   size--;

   while (*note) {
      if (!strncmp (field_name, note, strlen(field_name)) ||
          !strncmp (lang_field_name, note, strlen(lang_field_name))) {

         const char *end;

         while (*note && (*note != ':')) note++;
         if (!*note) break;
         note++;

         while (*note && (*note == ' ')) note++;

         end = note;

         while (*end && (*end != NEW_LINE[0])) end++;

         if (note == end) {
            field[0] = '\0';
            return 0;
         }

         if ((end - note) < size) size = end - note;
         strncpy(field, note, size);
         field[size] = '\0';
         return 0;
      }

      while (*note && (*note != NEW_LINE[strlen(NEW_LINE)-1])) note++;
      if (*note) note++;
   }

   return -1;
}



static int update_range_export(int marker,
                               const char **name,
                               const char **description,
                               const char  *keys[ED_MARKER_MAX_ATTRS],
                               char        *values[ED_MARKER_MAX_ATTRS],
                               int         *count) {

   char field[255];
   const char *note = editor_marker_note (marker);
   *count = 0;
   *description = NULL;
   *name = NULL;

   if (extract_field (note, STREET_PREFIX, field, sizeof(field)) == -1) {
      roadmap_log (ROADMAP_ERROR, "Update range - Can't find street name.");
      return -1;
   } else {
      keys[*count] = "street";
      values[*count] = strdup(field);
      (*count)++;
   }

   if (extract_field (note, CITY_PREFIX, field, sizeof(field)) == -1) {
      roadmap_log (ROADMAP_ERROR, "Update range - Can't find city name.");
      return -1;
   } else {
      keys[*count] = "city";
      values[*count] = strdup(field);
      (*count)++;
   }

   if (extract_field (note, UPDATE_LEFT, field, sizeof(field)) != -1) {
      if (atoi(field) <= 0) {
         roadmap_log (ROADMAP_ERROR, "Update range - Left range is invalid.");
         return -1;
      } else {
         keys[*count] = "left";
         values[*count] = strdup(field);
         (*count)++;
      }
   }

   if (extract_field (note, UPDATE_RIGHT, field, sizeof(field)) != -1) {
      if (atoi(field) <= 0) {
         roadmap_log (ROADMAP_ERROR, "Update range - Right range is invalid.");
         return -1;
      } else {
         keys[*count] = "right";
         values[*count] = strdup(field);
         (*count)++;
      }
   }

   if (*count < 2) {
      roadmap_log (ROADMAP_ERROR,
                     "Update range - No range updates were found.");
      return -1;
   }

   return 0;
}


static int update_range_verify(int marker,
                               unsigned char *flags,
                               const char **note) {

   char field[255];
   int found_update = 0;

   if (extract_field (*note, STREET_PREFIX, field, sizeof(field)) == -1) {
      roadmap_messagebox ("Error", "Can't find street name.");
      return -1;
   }

   if (extract_field (*note, CITY_PREFIX, field, sizeof(field)) == -1) {
      roadmap_messagebox ("Error", "Can't find city name.");
      return -1;
   }

   if (extract_field (*note, UPDATE_LEFT, field, sizeof(field)) != -1) {
      if (atoi(field) <= 0) {
         roadmap_messagebox ("Error", "Left range is invalid.");
         return -1;
      }

      found_update++;
   }

   if (extract_field (*note, UPDATE_RIGHT, field, sizeof(field)) != -1) {
      if (atoi(field) <= 0) {
         roadmap_messagebox ("Error", "Right range is invalid.");
         return -1;
      }

      found_update++;
   }

   if (!found_update) {
      roadmap_messagebox ("Error", "No range updates were found.");
      return -1;
   }

   return 0;
}


static int UpdateRangeMarkerType;
static EditorMarkerType UpdateRangeMarker = {
   "Street range",
   update_range_export,
   update_range_verify
};


static void update_range (const char *updated_left, const char *updated_right,
                         const char *city, const char *street) {

   char note[500];
   int fips;

   if (!*updated_left && !*updated_right) {
      return;
   }

   if (roadmap_county_by_position (&CurrentFixedPosition, &fips, 1) < 1) {
      roadmap_messagebox ("Error", "Can't locate county");
      return;
   }

   if (editor_db_activate (fips) == -1) {

      editor_db_create (fips);

      if (editor_db_activate (fips) == -1) {

         roadmap_messagebox ("Error", "Can't update range");
         return;
      }
   }

   snprintf(note, sizeof(note), "%s: %s%s",
         roadmap_lang_get (STREET_PREFIX), (char *)street,
         NEW_LINE);

   snprintf(note + strlen(note), sizeof(note) - strlen(note),
         "%s: %s%s", roadmap_lang_get (CITY_PREFIX), (char *)city,
         NEW_LINE);

   if (*updated_left) {

      snprintf(note + strlen(note), sizeof(note) - strlen(note),
            "%s: %s%s", roadmap_lang_get (UPDATE_LEFT), updated_left,
            NEW_LINE);
   }

   if (*updated_right) {

      snprintf(note + strlen(note), sizeof(note) - strlen(note),
            "%s: %s%s", roadmap_lang_get (UPDATE_RIGHT), updated_right,
            NEW_LINE);
   }

   if (editor_marker_add (CurrentFixedPosition.longitude,
            CurrentFixedPosition.latitude,
            CurrentGpsPoint.steering,
            time(NULL),
            UpdateRangeMarkerType,
            ED_MARKER_UPLOAD, note, NULL) == -1) {

      roadmap_messagebox ("Error", "Can't save marker.");
   } else {
		editor_report_markers ();
   }

}

#ifdef SSD
static BOOL keyboard_callback(int         exit_code, 
                              const char* value,
                              void*       context)
{
   SsdWidget button = (SsdWidget)context;

   if( dec_ok != exit_code)
        return TRUE;

   if (!strcmp(button->name, "left_button")) {
      ssd_widget_set_value (button->parent, UPDATE_LEFT, value);
   } else {
      ssd_widget_set_value (button->parent, UPDATE_RIGHT, value);
   }

   return TRUE;
}

static int button_callback (SsdWidget widget, const char *new_value) {

   const char *title;
   const char *value;

   if (!strcmp(widget->name, "OK")) {
      update_range (
         ssd_widget_get_value (widget->parent, UPDATE_LEFT),
         ssd_widget_get_value (widget->parent, UPDATE_RIGHT),
         ssd_dialog_get_value (CITY_PREFIX),
         ssd_dialog_get_value (STREET_PREFIX));

      ssd_dialog_hide_current (dec_cancel);

      return 1;

   } else if (!strcmp(widget->name, "Close")) {
      ssd_dialog_hide_current (dec_close);
      return 1;
   } else if (!strcmp(widget->name, "left_button")) {
      title = roadmap_lang_get (UPDATE_LEFT);
      value = ssd_widget_get_value (widget->parent, UPDATE_LEFT);
   } else {
      title = roadmap_lang_get (UPDATE_RIGHT);
      value = ssd_widget_get_value (widget->parent, UPDATE_RIGHT);
   }


#if (defined(__SYMBIAN32__) && !defined(TOUCH_SCREEN))
    ShowEditbox(title, "",
            keyboard_callback, (void *)widget, EEditBoxStandard | EEditBoxNumeric );
#else

   ssd_show_keyboard_dialog(  title,
                              NULL,
                              keyboard_callback,
                              widget);

#endif
   return 1;
}

static void create_ssd_dialog (void) {

   const char *left_icon[] = {"left_side"};
   const char *right_icon[] = {"right_side"};
   SsdWidget text;
   SsdWidget left;
   SsdWidget right;
   SsdWidget box;
   int align = 0;

   SsdWidget dialog = ssd_dialog_new ("Update house number",
                  roadmap_lang_get ("Update house number"),NULL,
                  SSD_CONTAINER_BORDER|SSD_CONTAINER_TITLE|SSD_DIALOG_FLOAT|
                  SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_BLACK);

   ssd_widget_set_color (dialog, "#000000", "#ffffffdd");

   /* Labels */
   box = ssd_container_new ("labels_box", NULL, -1, -1, SSD_WIDGET_SPACE);
   ssd_widget_set_color (box, "#fffff", NULL);

   text = ssd_text_new ("street_label", roadmap_lang_get (STREET_PREFIX),
                  14, SSD_TEXT_LABEL|SSD_END_ROW);
   ssd_text_set_color(text, "#ffffff");
   ssd_widget_add (box, text);

   text = ssd_text_new ("city_label", roadmap_lang_get (CITY_PREFIX),
                    14, SSD_TEXT_LABEL|SSD_END_ROW);
   ssd_text_set_color(text, "#ffffff");
   ssd_widget_add (box, text);

   ssd_widget_add (dialog, box);

   /* Values */
   box = ssd_container_new ("values_box", NULL, -1, -1, SSD_END_ROW);
   ssd_widget_set_color (box, "#ffffff", NULL);

   text = ssd_text_new (STREET_PREFIX, "", 14, SSD_END_ROW);
   ssd_text_set_color(text, "#ffffff");
   ssd_widget_add (box, text);

   text = ssd_text_new (CITY_PREFIX, "", 14, SSD_END_ROW);
   ssd_text_set_color(text, "#ffffff");
   ssd_widget_add (box, text);

   ssd_widget_add (dialog, box);

   /* Spacer */
   ssd_widget_add (dialog,
      ssd_container_new ("spacer1", NULL, 0, 16, SSD_END_ROW));

   /* Left side */
   if (ssd_widget_rtl (NULL)) {
      align = SSD_ALIGN_RIGHT;
   }

   left = ssd_container_new ("left", NULL, -1, -1, align|SSD_WS_TABSTOP);
   ssd_widget_set_color (left, "#ffffff", NULL);
   ssd_widget_set_offset (left, 2, 0);

   box = ssd_container_new ("left_box", NULL, 50, 20,
                            SSD_ALIGN_CENTER|SSD_END_ROW);
   ssd_widget_set_color (box, "#ffffff", NULL);

   text = ssd_text_new ("estimated_left", "", 16, SSD_END_ROW|SSD_ALIGN_CENTER);
   ssd_text_set_color(text, "#ffffff");
   ssd_widget_add (box, text);

   ssd_widget_add (left, box);

   ssd_widget_add (left,
         ssd_button_new ("left_button", "", left_icon, 1,
                         SSD_ALIGN_CENTER|SSD_END_ROW, button_callback));

   text = ssd_text_new (UPDATE_LEFT, "", 15, SSD_END_ROW|SSD_ALIGN_CENTER);
   ssd_text_set_color(text, "#ffffff");
   ssd_widget_add (left, text);

   /* Right side */
   if (ssd_widget_rtl (NULL)) align = 0;
   else align = SSD_ALIGN_RIGHT;

   right = ssd_container_new ("right", NULL, -1, -1, align|SSD_WS_TABSTOP);
   ssd_widget_set_offset (right, 2, 0);
   ssd_widget_set_color (right, "#fffff", NULL);
   box = ssd_container_new ("right_box", NULL, 50, 20,
                            SSD_ALIGN_CENTER|SSD_END_ROW);
   ssd_widget_set_color (box, "#000000", NULL);

   text = ssd_text_new ("estimated_right", "", 16, SSD_END_ROW|SSD_ALIGN_CENTER);
   ssd_text_set_color(text, "#ffffff");
   ssd_widget_add (box, text);
   
   ssd_widget_add (right, box);

   ssd_widget_add (right,
         ssd_button_new ("right_button", "", right_icon, 1,
                         SSD_ALIGN_CENTER|SSD_END_ROW, button_callback));

   text = ssd_text_new (UPDATE_RIGHT, "", 15, SSD_END_ROW);
   ssd_text_set_color(text, "#ffffff");
   ssd_widget_add (right, text);

   if (ssd_widget_rtl (NULL)) {
      ssd_widget_add (dialog, right);
      ssd_widget_add (dialog, left);
   } else {
      ssd_widget_add (dialog, left);
      ssd_widget_add (dialog, right);
   }

   ssd_widget_add (dialog,
      ssd_button_label ("OK", roadmap_lang_get ("Ok"),
                        SSD_ALIGN_CENTER|SSD_START_NEW_ROW|SSD_WS_TABSTOP, button_callback));

   ssd_widget_add (dialog,
      ssd_button_label ("Close", roadmap_lang_get ("Close"),
                        SSD_ALIGN_CENTER|SSD_WS_TABSTOP, button_callback));
}
#endif

static int get_estimated_range(const PluginLine *line,
                               const RoadMapPosition *pos,
                               int direction,
                               int fraddl, int toaddl,
                               int fraddr, int toaddr,
                               int *left, int *right) {


   int total_length;
   int point_length;
   double rel;

   point_length =
      roadmap_plugin_calc_length (pos, line, &total_length);

   rel = 1.0 * point_length / total_length;

   *left  = fraddl + (int) ((toaddl - fraddl + 1) / 2 * rel) * 2;
   *right = fraddr + (int) ((toaddr - fraddr + 1) / 2 * rel) * 2;

   if (direction == ROUTE_DIRECTION_AGAINST_LINE) {
      int tmp = *left;

      *left = *right;
      *right = tmp;
   }

   return 0;
}


static int fill_dialog(PluginLine *line, RoadMapPosition *pos,
                       int direction) {

   const char *street_name = NULL;
   const char *city_name = NULL;
   int fraddl;
   int toaddl;
   int fraddr;
   int toaddr;
   int left;
   int right;
   char str[100];

   if (line->plugin_id == EditorPluginID) {
#if 0
      EditorStreetProperties properties;

      if (editor_db_activate (line->fips) == -1) return -1;

      editor_street_get_properties (line->line_id, &properties);

      street_name = editor_street_get_street_name (&properties);

      city_name = editor_street_get_street_city
                        (&properties, ED_STREET_LEFT_SIDE);

      editor_street_get_street_range
         (&properties, ED_STREET_LEFT_SIDE, &fraddl, &toaddl);
      editor_street_get_street_range
         (&properties, ED_STREET_RIGHT_SIDE, &fraddr, &toaddr);
#endif
   } else {
      RoadMapStreetProperties properties;

      if (roadmap_locator_activate (line->fips) < 0) return -1;

      roadmap_street_get_properties (line->line_id, &properties);

      street_name = roadmap_street_get_street_name (&properties);

      city_name = roadmap_street_get_street_city
                        (&properties, ROADMAP_STREET_LEFT_SIDE);

      roadmap_street_get_street_range
         (&properties, ROADMAP_STREET_LEFT_SIDE, &fraddl, &toaddl);
      roadmap_street_get_street_range
         (&properties, ROADMAP_STREET_RIGHT_SIDE, &fraddr, &toaddr);
   }

   if (!city_name) city_name = "";

   get_estimated_range
         (line, pos, direction, fraddl, toaddl, fraddr, toaddr, &left, &right);
#ifndef SSD
   roadmap_dialog_set_data ("Update", STREET_PREFIX, street_name);

   roadmap_dialog_set_data ("Update", CITY_PREFIX, city_name);

   snprintf(str, sizeof(str), "%s:%d  %s:%d",
         roadmap_lang_get ("Left"), left,
         roadmap_lang_get ("Right"), right);

   roadmap_dialog_set_data ("Update", "Estimated", str);

   roadmap_dialog_set_data ("Update", UPDATE_LEFT, "");
   roadmap_dialog_set_data ("Update", UPDATE_RIGHT, "");
#else
   ssd_dialog_set_value (STREET_PREFIX, street_name);
   ssd_dialog_set_value (CITY_PREFIX, city_name);
   sprintf(str, "%d", left);
   ssd_dialog_set_value ("estimated_left", str);
   sprintf(str, "%d", right);
   ssd_dialog_set_value ("estimated_right", str);
   ssd_dialog_set_value (UPDATE_LEFT, "");
   ssd_dialog_set_value (UPDATE_RIGHT, "");
   ssd_dialog_draw ();
#endif
   return 0;
}


#ifndef SSD
static void update_range_apply(const char *name, void *context) {

   const char *updated_left =
      roadmap_dialog_get_data ("Update", UPDATE_LEFT);

   const char *updated_right =
      roadmap_dialog_get_data ("Update", UPDATE_RIGHT);

   update_range (updated_left, updated_right,
                 roadmap_dialog_get_data ("Update", CITY_PREFIX),
                 roadmap_dialog_get_data ("Update", STREET_PREFIX));

   roadmap_dialog_hide (name);
}


static void update_range_cancel(const char *name, void *context) {

   roadmap_dialog_hide (name);
}
#endif


void update_range_dialog(void) {

   RoadMapPosition from;
   RoadMapPosition to;
   PluginLine line;
   RoadMapNeighbour result;
   int direction;

   if (roadmap_navigate_get_current
         (&CurrentGpsPoint, &line, &direction) == -1) {

      roadmap_messagebox ("Error", "Can't find current street.");
      return;
   }

   roadmap_plugin_line_from (&line, &from);
   roadmap_plugin_line_to   (&line, &to);

   if (!roadmap_plugin_get_distance
        ((RoadMapPosition *)&CurrentGpsPoint, &line, &result)) {

      roadmap_messagebox ("Error", "Can't find a road near point.");
      return;
   }

   CurrentFixedPosition = result.intersection;

#ifndef SSD
   if (roadmap_dialog_activate ("Update house number", NULL, 1)) {

      roadmap_dialog_new_label ("Update", STREET_PREFIX);
      roadmap_dialog_new_label ("Update", CITY_PREFIX);
      roadmap_dialog_new_label ("Update", "Estimated");

      roadmap_dialog_new_entry ("Update", UPDATE_LEFT, NULL);
      roadmap_dialog_new_entry ("Update", UPDATE_RIGHT, NULL);

      roadmap_dialog_add_button ("Cancel", update_range_cancel);
      roadmap_dialog_add_button ("OK", update_range_apply);

      roadmap_dialog_complete (roadmap_preferences_use_keyboard ());
   }
#else
   if (!ssd_dialog_activate ("Update house number", NULL)) {
      create_ssd_dialog();
      ssd_dialog_activate ("Update house number", NULL);
   }
#endif
   fill_dialog (&line, &CurrentFixedPosition, direction);
}


void update_range_initialize(void) {
   UpdateRangeMarkerType = editor_marker_reg_type (&UpdateRangeMarker);
}

