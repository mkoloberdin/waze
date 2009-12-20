/* editor_dialog.c - editor navigation
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
 *   See editor_dialog.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "roadmap.h"
#ifdef SSD
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_choice.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_entry.h"
#include "ssd/ssd_button.h"
#else
#include "roadmap_dialog.h"
#endif
#include "roadmap_lang.h"
#include "roadmap_layer.h"
#include "roadmap_line.h"
#include "roadmap_line_route.h"
#include "roadmap_line_speed.h"
#include "roadmap_shape.h"
#include "roadmap_square.h"
#include "roadmap_locator.h"
#include "roadmap_preferences.h"

#include "../db/editor_db.h"
#include "../db/editor_line.h"
#include "../db/editor_shape.h"
#include "../db/editor_point.h"
#include "../db/editor_street.h"
#include "../db/editor_route.h"
#include "../db/editor_override.h"
#include "../editor_main.h"
#include "../editor_log.h"
#include "../export/editor_report.h"

#include "editor_dialog.h"


typedef struct dialog_selected_lines {
   SelectedLine *lines;
   int           count;
} DialogSelectedLines;

static const char *def_values[3] = {"", "", ""};




#ifdef SSD
static int editor_dialog_city_cb (SsdWidget widget, const char *new_value) {
   def_values[0] = new_value;
   return 1;
}

#else

static void editor_dialog_city_cb (const char *name, void *context) {
   if (!strcmp(roadmap_dialog_get_data ("General", "City"), def_values[1])) {

      roadmap_dialog_set_data ("General", "City", def_values[0]);
      roadmap_dialog_hide ("Segment Properties");

      roadmap_address_search_dialog
         (NULL, editor_dialog_city_result, context);
   }
}

#endif


#ifndef SSD
static void editor_segments_cancel (const char *name, void *context) {

   free (context);
   roadmap_dialog_hide (name);
}
#endif

#ifdef SSD
static int editor_segments_apply (SsdWidget widget, const char *new_value) {
   DialogSelectedLines *selected_lines =
      (DialogSelectedLines *) ssd_dialog_context ();
#else
static void editor_segments_apply (const char *name, void *context) {
   DialogSelectedLines *selected_lines = (DialogSelectedLines *)context;
#endif


   int i;
#ifdef SSD
   long type = (long) ssd_dialog_get_data ("Road type");
   int cfcc = type + ROADMAP_ROAD_FIRST;
   
   char *street_type = (char *) ssd_dialog_get_value ("Street type");
   
   char *street_name = (char *) ssd_dialog_get_value ("Name");
   
   char *t2s = (char *) ssd_dialog_get_value ("Text to Speech");
   
   char *city = (char *) ssd_dialog_get_value ("City");
#else   
   int type = (int) roadmap_dialog_get_data ("General", "Road type");
   int cfcc = type + ROADMAP_ROAD_FIRST;
   int direction = (int) roadmap_dialog_get_data ("General", "Direction");
   
   char *street_type =
      (char *) roadmap_dialog_get_data ("General", "Street type");
   
   char *street_name =
      (char *) roadmap_dialog_get_data ("General", "Name");
   
   char *t2s =
      (char *) roadmap_dialog_get_data ("General", "Text to Speech");
   
/*   char *street_range =
      (char *) roadmap_dialog_get_data ("General", "Street range"); */

   char *city = (char *) roadmap_dialog_get_data ("General", "City");
   char *zip = (char *) roadmap_dialog_get_data ("General", "Zip code");

   char *speed_limit_str =
      (char *) roadmap_dialog_get_data ("General", "Speed Limit");
#endif

   int street_id = editor_street_create (street_name, street_type, "", "", city, t2s);
	
   for (i=0; i<selected_lines->count; i++) {

      SelectedLine *line = &selected_lines->lines[i];

      if (line->line.plugin_id != EditorPluginID) {

			if (roadmap_square_scale (line->line.square) == 0 &&
				 !roadmap_street_line_has_predecessor (&line->line)) {
			
				line->line.cfcc = cfcc;
				editor_line_copy (&line->line, street_id);	
			}
			roadmap_square_set_current (line->line.square);
			editor_override_line_set_flag (line->line.line_id, line->line.square,  ED_LINE_DELETED);
      } else {
      
      	editor_line_modify_properties (line->line.line_id, cfcc, ED_LINE_DIRTY);
      	editor_line_set_street (line->line.line_id, street_id);	
      }

      
   }

   editor_screen_reset_selected ();
   editor_report_segments ();

#ifdef SSD
   ssd_dialog_hide_current (dec_close);
   return 1;
#else
   free (context);
   roadmap_dialog_hide (name);
#endif
}


#ifdef SSD
static void activate_dialog (const char *name,
                             DialogSelectedLines *selected_lines) {

   if (!ssd_dialog_activate (name, selected_lines)) {

      long *values;
      int count = ROADMAP_ROAD_LAST - ROADMAP_ROAD_FIRST + 1;
      int i;
      char **categories;
      static const char *lang_categories[ROADMAP_ROAD_LAST - ROADMAP_ROAD_FIRST + 1];
      SsdWidget dialog;
      SsdWidget group;
      SsdWidget container;

      if (!def_values[1][0]) {
         def_values[1] = roadmap_lang_get ("Search");
      }

      dialog = ssd_dialog_new (name, roadmap_lang_get (name), NULL,
                               SSD_CONTAINER_TITLE);

      container = ssd_container_new ("Conatiner Group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
              SSD_WIDGET_SPACE|SSD_END_ROW|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE|SSD_CONTAINER_BORDER|SSD_POINTER_NONE);

      group = ssd_container_new ("Length group", NULL,
                  SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_WIDGET_SPACE|SSD_END_ROW);
      ssd_widget_set_color (group, NULL, NULL);
      ssd_widget_add (group,
         ssd_text_new ("Label", roadmap_lang_get("Length"), -1,
                        SSD_TEXT_LABEL));
      ssd_widget_add (group, ssd_text_new ("Length", "", -1, 0));
      ssd_widget_add (container, group);

      group = ssd_container_new ("Speed group", NULL,
                  SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_WIDGET_SPACE|SSD_END_ROW);
      ssd_widget_set_color (group, NULL, NULL);
      ssd_widget_add (group,
         ssd_text_new ("Label", roadmap_lang_get("Speed"), -1,
                        SSD_TEXT_LABEL));
      ssd_widget_add (group, ssd_text_new ("Speed", "", -1, 0));
      ssd_widget_add (container, group);

      group = ssd_container_new ("Time group", NULL,
                  SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_WIDGET_SPACE|SSD_END_ROW);
      ssd_widget_set_color (group, NULL, NULL);
      ssd_widget_add (group,
         ssd_text_new ("Label", roadmap_lang_get("Time"), -1,
                        SSD_TEXT_LABEL));
      ssd_widget_add (group, ssd_text_new ("Time", "", -1, SSD_END_ROW));
      ssd_widget_add (container, group);

      roadmap_layer_get_categories_names (&categories, &i);
      values = malloc ((count+1) * sizeof (long));

      for (i=0; i<count; i++) {
         values[i] = i;
         lang_categories[i] = roadmap_lang_get (categories[i]);
      }

      group = ssd_container_new ("Road type group", NULL,
                  SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_WIDGET_SPACE|SSD_END_ROW);

      ssd_widget_add (group,
         ssd_text_new ("Label", roadmap_lang_get("Road type"), -1,
                        SSD_TEXT_LABEL|SSD_ALIGN_VCENTER));

      ssd_widget_add (group,
         ssd_choice_new ("Road type",roadmap_lang_get("Road type"), count,
                                 (const char **)lang_categories,
                                 (const void**)values,
                                 SSD_END_ROW|SSD_ALIGN_VCENTER, NULL));
      ssd_widget_add (container, group);

      group = ssd_container_new ("Name group", NULL,
                  SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_WIDGET_SPACE|SSD_END_ROW);
      ssd_widget_add (group,
         ssd_text_new ("Label", roadmap_lang_get("Name"), -1,
                        SSD_TEXT_LABEL|SSD_ALIGN_VCENTER));

      ssd_widget_add (group,
         ssd_entry_new ("Name", "", SSD_ALIGN_VCENTER, 0,180, SSD_MIN_SIZE,""));
      ssd_widget_add (container, group);

      group = ssd_container_new ("Text to Speech group", NULL,
                  SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_WIDGET_SPACE|SSD_END_ROW);
      ssd_widget_add (group,
         ssd_text_new ("Label", roadmap_lang_get("Text to Speech"), -1,
                        SSD_TEXT_LABEL|SSD_ALIGN_VCENTER));

      ssd_widget_add (group,
         ssd_entry_new ("Text to Speech", "", SSD_ALIGN_VCENTER, 0,180, SSD_MIN_SIZE,""));
      ssd_widget_add (container, group);

      group = ssd_container_new ("City group", NULL,
                  SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_WIDGET_SPACE|SSD_END_ROW);

      ssd_widget_add (group,
         ssd_text_new ("Label", roadmap_lang_get("City"), -1,
                        SSD_TEXT_LABEL|SSD_ALIGN_VCENTER));

      ssd_widget_add (group,
         ssd_choice_new ("City",roadmap_lang_get("City"),
                         sizeof(def_values) / sizeof(char **),
                         def_values,
                         (const void **)def_values,
                          SSD_END_ROW|SSD_ALIGN_VCENTER,
                          editor_dialog_city_cb));

      ssd_widget_add (container, group);

      ssd_widget_add (container,
         ssd_button_label ("confirm", roadmap_lang_get ("Ok"),
                        SSD_ALIGN_CENTER|SSD_START_NEW_ROW,
                        editor_segments_apply));

      ssd_widget_add (dialog, container);

      ssd_dialog_activate (name, selected_lines);
   }
}

#else

static void activate_dialog (const char *name,
                             DialogSelectedLines *selected_lines) {

   if (roadmap_dialog_activate ("Segment Properties", selected_lines, 1)) {

      long *values;
      int count = ROADMAP_ROAD_LAST - ROADMAP_ROAD_FIRST + 1;
      int i;
      char **categories;
      static const char *lang_categories[ROADMAP_ROAD_LAST - ROADMAP_ROAD_FIRST + 1];

      if (!def_values[1][0]) {
         def_values[1] = roadmap_lang_get ("Search");
      }

      /*
      char *direction_txts[] =
         { "Unknown", "With road", "Against road", "Both directions"};
      int direction_values[] = {0, 1, 2, 3};
      */
      
      roadmap_dialog_new_label ("Info", "Length");
      roadmap_dialog_new_label ("Info", "Time");

#if 0      
      roadmap_dialog_new_entry ("Right", "Street range", NULL);
      roadmap_dialog_new_entry ("Right", "City", NULL);
      roadmap_dialog_new_entry ("Right", "Zip code", NULL);

      roadmap_dialog_new_entry ("Left", "Street range", NULL);
      roadmap_dialog_new_entry ("Left", "City", NULL);
      roadmap_dialog_new_entry ("Left", "Zip code", NULL);
#endif
      roadmap_layer_get_categories_names (&categories, &i);
      values = malloc ((count+1) * sizeof (long));

      for (i=0; i<count; i++) {
         values[i] = i;
         lang_categories[i] = roadmap_lang_get (categories[i]);
      }

      roadmap_dialog_new_choice ("General", "Road type", count,
                                 (const char **)lang_categories,
                                 (void**)values, NULL);
      free (values);

/*      roadmap_dialog_new_entry ("General", "Street type", NULL); */
      roadmap_dialog_new_entry ("General", "Name", NULL);
      roadmap_dialog_new_entry ("General", "Text to Speech", NULL);
/*      roadmap_dialog_new_entry ("General", "Street range", NULL); */
      roadmap_dialog_new_choice ("General", "City",
         sizeof(def_values) / sizeof(char **),
         def_values, (void **)def_values, editor_dialog_city_cb);

/*      roadmap_dialog_new_choice ("General", "Direction", 4, direction_txts,
                                 (void**)direction_values, NULL);
      roadmap_dialog_new_entry ("General", "Zip code", NULL);
      roadmap_dialog_new_entry ("General", "Speed Limit", NULL); */

      roadmap_dialog_add_button ("Cancel", editor_segments_cancel);
      roadmap_dialog_add_button ("Ok", editor_segments_apply);

      roadmap_dialog_complete (roadmap_preferences_use_keyboard ());
   }
}
#endif


/* NOTE: This function modifies the street_range parameter */
#if 0
static void decode_range (char *street_range,
                          int *from1,
                          int *to1,
                          int *from2,
                          int *to2) {

   int i = 0;
   int max = strlen(street_range);
   char *ranges[4];
   int num_ranges=0;

   while (i <= max) {

      while (!isdigit (street_range[i]) && (i <= max)) i++;
      if (i>max) break;

      ranges[num_ranges++] = street_range+i;
      if (num_ranges == (sizeof(ranges) / sizeof(ranges[0]) - 1)) break;

      while (isdigit (street_range[i]) && (i <= max)) i++;

      street_range[i] = '\0';
      i++;
   }
 
   if (num_ranges == 4) {
      *from1 = atoi (ranges[0]);
      *to1   = atoi (ranges[1]);

      if (from2 && to2) {
         *from2 = atoi (ranges[2]);
         *to2   = atoi (ranges[3]);
      }

   } else if (num_ranges > 1) {
      *from1 = atoi (ranges[0]);
      *to1   = atoi (ranges[1]);

      if (((*from1 & 1) && !(*to1 & 1)) ||
          (!(*from1 & 1) && (*to1 & 1))) {
         (*to1)--;
      }

      if (from2 && to2) {
         *from2 = (*from1) + 1;
         *to2   = (*to1) + 1;
      }
   }
}
#endif


static const char *editor_segments_find_city
         (int line, int plugin_id, int square) {

   int i;
   int count;
   int layers_count;
   int layers[128];
   RoadMapPosition point;
   RoadMapNeighbour neighbours[50];

   if (plugin_id == EditorPluginID) {
      editor_line_get (line, &point, NULL, NULL, NULL, NULL);
   } else {
   	  roadmap_square_set_current (square);
      roadmap_line_from (line, &point);
   }

   layers_count = roadmap_layer_all_roads (layers, 128);

   count = roadmap_street_get_closest
           (&point,
            0,
            layers,
            layers_count,
            1,
            neighbours,
            sizeof(neighbours) / sizeof(RoadMapNeighbour));

   count = roadmap_plugin_get_closest
               (&point, layers, layers_count, 1, neighbours, count,
                sizeof(neighbours) / sizeof(RoadMapNeighbour));

   for (i = 0; i < count; ++i) {

      const char *city;
      
      if (roadmap_plugin_get_id (&neighbours[i].line) == EditorPluginID) {
         int street_id = -1;

			editor_line_get_street 
            (roadmap_plugin_get_line_id (&neighbours[i].line),
             &street_id);

         city =
            editor_street_get_street_city
               (street_id);
         
      } else if (roadmap_plugin_get_id (&neighbours[i].line) ==
            ROADMAP_PLUGIN_ID) {
         RoadMapStreetProperties properties;

			roadmap_square_set_current (roadmap_plugin_get_square (&neighbours[i].line));
         roadmap_street_get_properties
            (roadmap_plugin_get_line_id (&neighbours[i].line),
             &properties);

         city =
            roadmap_street_get_street_city
               (&properties, ROADMAP_STREET_LEFT_SIDE);

      } else {
         city = "";
      }

      if (strlen(city)) {
         return city;
      }
   }

   return "";
}


void editor_segments_fill_dialog (SelectedLine *lines, int lines_count) {

   char street_name[512];
   const char *street_type = "";
   const char *t2s = "";
   const char *city = "";
   int cfcc = ROADMAP_ROAD_LAST;
   int same_street = 1;
   int same_city = 1;
   int i;

   strcpy (street_name, "");
   for (i=0; i<lines_count; i++) {
      
      const char *this_name = "";
      const char *this_type = "";
      const char *this_t2s = "";
      const char *this_city = "";

      if (lines[i].line.plugin_id == EditorPluginID) {
         if (editor_db_activate (lines[i].line.fips) != -1) {

            int street_id = -1;
            
            editor_line_get_street (lines[i].line.line_id, &street_id);

            this_name = editor_street_get_street_fename (street_id);
            this_type = editor_street_get_street_fetype (street_id);
            this_t2s = editor_street_get_street_t2s (street_id);
            this_city = editor_street_get_street_city (street_id);
				//editor_line_get_direction (lines[i].line.line_id, &this_direction);
         }
      } else { 

         RoadMapStreetProperties properties;

         if (roadmap_locator_activate (lines[i].line.fips) < 0) {
            continue;
         }

         roadmap_square_set_current (lines[i].line.square);
         roadmap_street_get_properties (lines[i].line.line_id, &properties);
    
         this_name = roadmap_street_get_street_name (&properties);
         this_type = roadmap_street_get_street_fetype (&properties);
         this_t2s = roadmap_street_get_street_t2s (&properties);

         this_city = roadmap_street_get_street_city
               (&properties, ROADMAP_STREET_LEFT_SIDE);
/*
			if (!editor_override_line_get_direction (lines[0].line.line_id, &this_direction)) {

            this_direction =
               roadmap_line_route_get_direction
                  (lines[i].line.line_id, ROUTE_CAR_ALLOWED);
            roadmap_line_route_get_speed_limit
                  (lines[i].line.line_id,
                   &this_speed_limit, &this_speed_limit);
         }
*/         
      }

      if (same_street) {
         if (!strlen(street_type)) {
            street_type = this_type;
         }

         if (!strlen(t2s)) {
            t2s = this_t2s;
         }

         if (!strlen(street_name)) {
            strncpy_safe (street_name, this_name, 512);
         } else {
            if (strlen(this_name) && strcmp(this_name, street_name)) {
               strcpy (street_name, "");
               street_type = "";
               t2s = "";
               same_street = 0;
            }
         }
       }

      if (same_city) {
         if (!strlen(city)) {
            city = this_city;
         } else {
            if (strlen(this_city) && strcmp(this_city, city)) {
               same_city = 0;
               city = "";
            }
         }
      } 
      
      if (lines[i].line.cfcc < cfcc) {
         cfcc = lines[i].line.cfcc;
      }
   }

   if (same_city && !strlen(city)) {
      
      city = editor_segments_find_city
             (lines[0].line.line_id, lines[0].line.plugin_id, lines[0].line.square);
   }

#ifdef SSD
   ssd_dialog_set_data ("Road type", (void *) (long)(cfcc - ROADMAP_ROAD_FIRST));
   ssd_dialog_set_value ("Name", street_name);
   ssd_dialog_set_value ("Text to Speech", t2s);

   ssd_dialog_set_value ("City", city);

   def_values[0] = ssd_dialog_get_value ("City");

#else
   roadmap_dialog_set_data
      ("General", "Road type", (void *) (cfcc - ROADMAP_ROAD_FIRST));
   roadmap_dialog_set_data ("General", "Street type", street_type);
   roadmap_dialog_set_data ("General", "Name", street_name);
   roadmap_dialog_set_data ("General", "Text to Speech", t2s);

   roadmap_dialog_set_data ("General", "City", city);

#endif


}


void editor_segments_properties (SelectedLine *lines, int lines_count) {

   char str[100];
   int total_length = 0;
   int total_time = 0;
   int i;
   DialogSelectedLines *selected_lines = malloc (sizeof(*selected_lines));

   selected_lines->lines = lines;
   selected_lines->count = lines_count;

   activate_dialog("Segment Properties", selected_lines);
   editor_segments_fill_dialog (lines, lines_count);

   /* Collect info data */
   for (i=0; i<selected_lines->count; i++) {
      int line_length;
      int time;

      SelectedLine *line = &selected_lines->lines[i];

      if (line->line.plugin_id == EditorPluginID) {

         line_length = editor_line_length (line->line.line_id);
         time = editor_line_get_cross_time (line->line.line_id, 1);
         
      } else  {
         LineRouteTime from_cross_time;
         LineRouteTime to_cross_time;

	      roadmap_square_set_current (line->line.square);
         line_length = roadmap_line_length (line->line.line_id);
         if (roadmap_line_speed_get_cross_times
               (line->line.line_id, &from_cross_time, &to_cross_time) == -1) {
            time = -1; //TODO get real speed
         } else {
            time = from_cross_time;
         }
      }

      if (time > 0) {
         total_time += time;
      } else {

         if ((total_time == 0) || (total_length == 0)) {
            total_time += (int)(1.0 *line_length / 10);
         } else {
            total_time +=
               (int)(1.0 * line_length / (total_length/total_time)) + 1;
         }
      }

      total_length += line_length;
   }

#ifdef SSD
   if (selected_lines->lines[0].line.plugin_id == ROADMAP_PLUGIN_ID) {
      int line = selected_lines->lines[0].line.line_id;
      int avg_speed = roadmap_line_speed_get_avg_speed (line, 0);
      int cur_speed = roadmap_line_speed_get_speed (line, 0);

      snprintf (str, sizeof(str), " : %d (%d)",
               roadmap_math_to_speed_unit(avg_speed), roadmap_math_to_speed_unit(cur_speed));
      ssd_dialog_set_value ("Speed", str);
   } else {
      ssd_dialog_set_value ("Speed", "");
   }
#endif   

   snprintf (str, sizeof(str), "  : %d %s",
            roadmap_math_distance_to_current(total_length), roadmap_lang_get(roadmap_math_distance_unit()));
#ifdef SSD
   ssd_dialog_set_value ("Length", str);
#else   
   roadmap_dialog_set_data ("Info", "Length", str);
#endif   

   snprintf (str, sizeof(str), "  : %d %s",
             total_time, roadmap_lang_get("seconds"));

#ifdef SSD
   ssd_dialog_set_value ("Time", str);
   ssd_dialog_draw ();
#else   
   roadmap_dialog_set_data ("Info", "Time", str);
#endif   
}

