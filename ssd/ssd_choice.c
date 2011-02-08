/* ssd_choice.c - combo box widget
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
 *   See ssd_choice.h.
 */

#include <string.h>
#include <stdlib.h>

#include "ssd_dialog.h"
#include "ssd_container.h"
#include "ssd_button.h"
#include "ssd_text.h"
#include "ssd_generic_list_dialog.h"

#include "ssd_choice.h"

struct ssd_choice_data {
   const char *title;
   SsdCallback callback;
   int num_values;
   const char **labels;
   const void **values;
};

#define CHOICE_CLICK_OFFSETS_DEFAULT   {-800, -20, 800, 20 };

static SsdClickOffsets sgChoiceOffsets = CHOICE_CLICK_OFFSETS_DEFAULT;


static int list_callback(SsdWidget widget, const char* selection, const void *value, void* context)
{
   SsdWidget selected_line      = (SsdWidget)context;
   struct ssd_choice_data* data = (struct ssd_choice_data *)selected_line->data;

   ssd_widget_set_value( selected_line, "Label", selection);
   ssd_generic_list_dialog_hide ();

   if( data->callback){
        data->callback( selected_line, selection);
   }

   return 1;
}


static int choice_callback (SsdWidget widget, const char *new_value) {

   struct ssd_choice_data *data;

   widget = widget->parent;

   data = (struct ssd_choice_data *)widget->data;

   ssd_generic_list_dialog_show (data->title, data->num_values, data->labels,
                                 NULL, list_callback, NULL, widget, ssd_container_get_row_height() );
   return 1;
}


static const char *get_value (SsdWidget widget) {
   return ssd_widget_get_value (widget, "Label");
}


static const void *get_data (SsdWidget widget) {
   struct ssd_choice_data *data = (struct ssd_choice_data *)widget->data;
   const char *value = get_value (widget);
   int i;

   for (i=0; i<data->num_values; i++) {
      if (!strcmp(value, data->labels[i])) break;
   }

   if (i == data->num_values) return NULL;

   return data->values[i];
}


static int set_value (SsdWidget widget, const char *value) {
   struct ssd_choice_data *data = (struct ssd_choice_data *)widget->data;

   if ((data->callback) && !(*data->callback) (widget, value)) {
      return 0;
   }

   return ssd_widget_set_value (widget, "Label", value);
}


static int set_data (SsdWidget widget, const void *value) {
   struct ssd_choice_data *data = (struct ssd_choice_data *)widget->data;
   int i;

   for (i=0; i<data->num_values; i++) {
      if (data->values[i] == value) break;
   }

   if (i == data->num_values) return -1;

   return ssd_widget_set_value (widget, "Label", data->labels[i]);
}


SsdWidget ssd_choice_new (const char *name, const char *title, int count,
                          const char **labels,
                          const void **values,
                          int flags,
                          SsdCallback callback) {

   const char *edit_button[] = {"edit_right", "edit_left"};
   const char *buttons[2];
   int rtl_flag = 0;
   SsdWidget text_box, text;
   SsdWidget choice;
   int txt_box_height = 40;
   struct ssd_choice_data *data =
      (struct ssd_choice_data *)calloc (1, sizeof(*data));

   SsdWidget button;

#ifndef TOUCH_SCREEN
   txt_box_height = 23;
#endif

   choice =
      ssd_container_new (name, NULL, SSD_MIN_SIZE, txt_box_height, SSD_ALIGN_VCENTER| flags);
   ssd_widget_set_color(choice, NULL, NULL);
   ssd_widget_set_pointer_force_click( choice );

   text_box =
      ssd_container_new ("text_box", NULL, SSD_MIN_SIZE,
                         SSD_MIN_SIZE,
                         SSD_ALIGN_VCENTER);
   ssd_widget_set_pointer_force_click( text_box );
   data->callback = callback;
   data->num_values = count;
   data->labels = labels;
   data->values = values;
   data->title = title;

   choice->get_value = get_value;
   choice->get_data = get_data;
   choice->set_value = set_value;
   choice->set_data = set_data;
   choice->data = data;
   choice->bg_color = NULL;

   text_box->callback = choice_callback;
   text_box->bg_color = NULL;

   if (!ssd_widget_rtl(NULL))
   	rtl_flag = SSD_ALIGN_RIGHT;

   text = ssd_text_new ("Label", labels[0], SSD_SECONDARY_TEXT_SIZE, SSD_ALIGN_VCENTER);
   ssd_text_set_color(text, "#206892");
   ssd_widget_add (text_box, text);
   if (!ssd_widget_rtl(NULL)){
   	   buttons[0] = edit_button[0];
   	   buttons[1] = edit_button[0];
   }else{
   	   buttons[0] = edit_button[1];
   	   buttons[1] = edit_button[1];
   }

   button = ssd_button_new ("edit_button", "", &buttons[0], 2,
        	                 SSD_ALIGN_VCENTER|SSD_ALIGN_RIGHT, choice_callback);
   ssd_widget_add (choice, button);
   ssd_widget_set_click_offsets( button, &sgChoiceOffsets );

   ssd_widget_set_click_offsets( choice, &sgChoiceOffsets );

   ssd_widget_add (choice, text_box);

   return choice;
}

