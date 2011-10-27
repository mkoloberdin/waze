/*  ssd_segmented_control.c- segmented control
 *
 * LICENSE:
 *
 *   Copyright 2006 Avi Ben-Shoshan
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
 */

#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_res.h"
#include "roadmap_canvas.h"
#include "roadmap_sound.h"
#include "roadmap_screen.h"
#include "roadmap_main.h"

#include "roadmap_keyboard.h"

#include "ssd_widget.h"
#include "ssd_text.h"
#include "ssd_button.h"
#include "ssd_segmented_control.h"
#include "ssd_container.h"

#define MAX_BUTTONS 3

struct ssd_segmented_control_data {
   SsdSegmentedControlCallback callbacks[MAX_BUTTONS];
   SsdWidget                   buttons[MAX_BUTTONS];
   int num_buttons;

};

static int on_button_pressed (SsdWidget widget, const char *new_value){
   struct ssd_segmented_control_data *data = (struct ssd_segmented_control_data *) widget->parent->data;
   SsdWidget text;
   int i;
   int index;

   for (i = 0; i<data->num_buttons;i++){
      if (data->buttons[i] == widget){
         index = i;
         break;
      }
   }

   for (i = 0; i<data->num_buttons;i++){
      ssd_widget_loose_focus(data->buttons[i]);
   }

   if (data->callbacks[index])
      (*data->callbacks[index])(widget, widget->value, widget->parent->context);

   ssd_widget_set_focus(widget);
   text = ssd_widget_get(widget, "label");
   ssd_widget_set_focus(text);
   return 1;
}

void ssd_segmented_control_set_focus(SsdWidget tab, int item){
   struct ssd_segmented_control_data *data = (struct ssd_segmented_control_data *) tab->data;
   int i;
   SsdWidget text;

   if (!data)
      return;

   if ((item < 0) || (item > data->num_buttons+1))
      return;

   for (i = 0; i<data->num_buttons;i++){
      ssd_widget_loose_focus(data->buttons[i]);
   }

   ssd_widget_set_focus(data->buttons[item]);
   text = ssd_widget_get(data->buttons[item], "label");
   if (text)
      ssd_widget_set_focus(text);

}

SsdWidget ssd_segmented_control_new (const char *name, int count, const char **labels,
                          const void **values,
                          int flags,
                          SsdSegmentedControlCallback *callback, void *context, int default_button) {
   SsdWidget container;
   SsdWidget button;
   const char *icons[3];
   int i;

   struct ssd_segmented_control_data *data =
      (struct ssd_segmented_control_data *)calloc (1, sizeof(*data));

   for (i = 0; i< MAX_BUTTONS; i++){
      data->callbacks[i] = NULL;
      data->buttons[i] = NULL;
   }


   container = ssd_container_new ("Tab.Container", NULL,
                                  SSD_MIN_SIZE,SSD_MIN_SIZE,flags);
   container->_typeid = "SegmentedControl";
   ssd_widget_set_color(container, NULL, NULL);
   container->context = context;
   data->num_buttons = count;

   for (i = 0; i< count; i++){
      data->callbacks[i] = callback[i];
   }

   if (count == 2){
      if (ssd_widget_rtl(NULL)){
          icons[0] = "button_sc_2_right";
          icons[1] = "button_sc_2_right_s";
          icons[2] = NULL;
      }
      else{
          icons[0] = "button_sc_2_left";
          icons[1] = "button_sc_2_left_s";
          icons[2] = NULL;
      }

      button = ssd_button_label_custom("Button1", labels[0], SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER|SSD_WS_TABSTOP, on_button_pressed, icons , 2, "#373737", "#ffffff", 14);
      ssd_widget_set_offset(button, 0, 2);
      button->value = (char *)values[0];
      data->buttons[0] = button;

      if (default_button == 0)
         ssd_widget_set_focus(button);
      ssd_widget_add(container, button);

      if (ssd_widget_rtl(NULL)){
          icons[0] = "button_sc_2_left";
          icons[1] = "button_sc_2_left_s";
          icons[2] = NULL;
      }
      else{
          icons[0] = "button_sc_2_right";
          icons[1] = "button_sc_2_right_s";
          icons[2] = NULL;
      }
      button = ssd_button_label_custom("Button2",labels[1], SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER|SSD_WS_TABSTOP, on_button_pressed, icons , 2,"#373737", "#ffffff",14);
      ssd_widget_set_offset(button, 0, 2);
      button->value = (char *)values[1];
      data->buttons[1] = button;

      if (default_button == 1)
         ssd_widget_set_focus(button);
   }
   else if (count == 3){
      if (ssd_widget_rtl(NULL)){
          icons[0] = "button_sc_3_right";
          icons[1] = "button_sc_3_right_s";
          icons[2] = NULL;
      }
      else{
          icons[0] = "button_sc_3_left";
          icons[1] = "button_sc_3_left_s";
          icons[2] = NULL;
      }
      button = ssd_button_label_custom("Button1", labels[0], SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER|SSD_WS_TABSTOP, on_button_pressed, icons , 2,"#373737", "#ffffff",12);
      button->value = (char *)values[0];
      if (default_button == 0)
         ssd_widget_set_focus(button);
      ssd_widget_add(container, button);
      data->buttons[0] = button;


      icons[0] = "button_sc_3_mid";
      icons[1] = "button_sc_3_mid_s";
      icons[2] = NULL;
      button = ssd_button_label_custom("Button2",labels[1], SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER|SSD_WS_TABSTOP, on_button_pressed, icons , 2,"#373737", "#ffffff",12);
      button->value = (char *)values[1];
      if (default_button == 1)
         ssd_widget_set_focus(button);
      ssd_widget_add(container, button);
      data->buttons[1] = button;

      if (ssd_widget_rtl(NULL)){
          icons[0] = "button_sc_3_left";
          icons[1] = "button_sc_3_left_s";
          icons[2] = NULL;
      }
      else{
          icons[0] = "button_sc_3_right";
          icons[1] = "button_sc_3_right_s";
          icons[2] = NULL;
      }
      button = ssd_button_label_custom("Button3",labels[2], SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER|SSD_WS_TABSTOP, on_button_pressed, icons , 2,"#373737", "#ffffff",12);
      button->value = (char *)values[2];
      data->buttons[2] = button;

      if (default_button == 2)
         ssd_widget_set_focus(button);
   }
   ssd_widget_add(container, button);
   container->data = data;
   return container;
}

static void AddtoButton(SsdWidget button,  const char *label, const char *button_icons){
   const char *button_icon_names[3];
   SsdWidget icon;
   SsdWidget text;

   button_icon_names[0] =button_icons;
   button_icon_names[1] = NULL;
   icon = ssd_button_new
                           ("butoon", "button", button_icon_names, 1, SSD_ALIGN_CENTER|SSD_END_ROW|SSD_ALIGN_VCENTER,
                            NULL);
   ssd_widget_set_offset(icon, 0, ADJ_SCALE(-12));
   ssd_widget_add(button, icon);
   text = ssd_text_new ("label",
                          label,
                          14, SSD_ALIGN_CENTER|SSD_ALIGN_BOTTOM);
   ssd_widget_set_color(text, "#373737", "#ffffff");
   ssd_widget_set_offset(text, 0, ADJ_SCALE(-10));
   ssd_widget_add (button, text);
}

SsdWidget ssd_segmented_icon_control_new (const char *name, int count, const char **labels,
                          const void **values, const char **button_icons,
                          int flags,
                          SsdSegmentedControlCallback *callback, void *context, int default_button) {
   SsdWidget container;
   SsdWidget button;
   const char *icons[3];
   int i;

   struct ssd_segmented_control_data *data =
      (struct ssd_segmented_control_data *)calloc (1, sizeof(*data));

   for (i = 0; i< MAX_BUTTONS; i++){
      data->callbacks[i] = NULL;
      data->buttons[i] = NULL;
   }


   container = ssd_container_new ("Tab.Container", NULL,
                                  SSD_MIN_SIZE,SSD_MIN_SIZE,flags);
   container->_typeid = "SegmentedControl";
   ssd_widget_set_color(container, NULL, NULL);
   container->context = context;
   data->num_buttons = count;

   for (i = 0; i< count; i++){
      data->callbacks[i] = callback[i];
   }

   if (count == 2){
      if (ssd_widget_rtl(NULL)){
          icons[0] = "button_sc_icon_right";
          icons[1] = "button_sc_icon_right_s";
          icons[2] = NULL;
      }
      else{
          icons[0] = "button_sc_icon_left";
          icons[1] = "button_sc_icon_left_s";
          icons[2] = NULL;
      }
      button = ssd_button_new("Button1", values[0], icons, 2, SSD_WS_TABSTOP, on_button_pressed);
      data->buttons[0] = button;
      AddtoButton(button, labels[0], button_icons[0] );
      if (default_button == 0)
         ssd_widget_set_focus(button);
      ssd_widget_add(container, button);

      if (ssd_widget_rtl(NULL)){
          icons[0] = "button_sc_icon_left";
          icons[1] = "button_sc_icon_left_s";
          icons[2] = NULL;
      }
      else{
          icons[0] = "button_sc_icon_right";
          icons[1] = "button_sc_icon_right_s";
          icons[2] = NULL;
      }
      button = ssd_button_new("Button2", values[1], icons, 2, SSD_WS_TABSTOP, on_button_pressed);
      data->buttons[1] = button;
      AddtoButton(button, labels[1], button_icons[1] );

      if (default_button == 1)
         ssd_widget_set_focus(button);
   }
   else if (count == 3){
      if (ssd_widget_rtl(NULL)){
          icons[0] = "button_sc_icon_right";
          icons[1] = "button_sc_icon_right_s";
          icons[2] = NULL;
      }
      else{
          icons[0] = "button_sc_icon_left";
          icons[1] = "button_sc_icon_left_s";
          icons[2] = NULL;
      }
      button = ssd_button_new("Button1", values[0], icons, 2, SSD_WS_TABSTOP, on_button_pressed);
      AddtoButton(button, labels[0], button_icons[0] );

      if (default_button == 0)
         ssd_widget_set_focus(button);
      ssd_widget_add(container, button);
      data->buttons[0] = button;


      icons[0] = "button_sc_icon_mid";
      icons[1] = "button_sc_icon_mid_s";
      icons[2] = NULL;
      button = ssd_button_new("Button2",values[1], icons, 2, SSD_WS_TABSTOP, on_button_pressed);
      AddtoButton(button, labels[1], button_icons[1] );

      if (default_button == 1)
         ssd_widget_set_focus(button);
      ssd_widget_add(container, button);
      data->buttons[1] = button;

      if (ssd_widget_rtl(NULL)){
          icons[0] = "button_sc_icon_left";
          icons[1] = "button_sc_icon_left_s";
          icons[2] = NULL;
      }
      else{
          icons[0] = "button_sc_icon_right";
          icons[1] = "button_sc_icon_right_s";
          icons[2] = NULL;
      }
      button = ssd_button_new("Button3", values[2], icons, 2, SSD_WS_TABSTOP, on_button_pressed);
      AddtoButton(button, labels[2], button_icons[2] );
      data->buttons[2] = button;

      if (default_button == 2)
         ssd_widget_set_focus(button);
   }
   ssd_widget_add(container, button);
   container->data = data;
   return container;
}



