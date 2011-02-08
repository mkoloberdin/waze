/*
 * LICENSE:
 *
 *   Copyright 2010 Avi Ben-Shoshan
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "roadmap.h"
#include "roadmap_lang.h"
#include "roadmap_config.h"
#include "roadmap_main.h"
#include "roadmap_groups.h"
#include "ssd/ssd_widget.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_choice.h"
#include "ssd/ssd_separator.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
#define GROUP_SETTING_DLG_NAME         "groups_settings.dlg"
#define GROUP_SETTING_CONTAINER_NAME   "groups_settings.container"

#define GROUP_SETTING_POPUP_CHOICE     "popup"
#define GROUP_SETTING_WAZER_CHOICE     "wazers"

#define GROUP_SETTING_DLG_TITLE        "Groups"

//////////////////////////////////////////////////////////////////////////////////////////////////
static const char *popup_labels[3];
static const char *popup_values[3];
static const char *wazer_labels[3];
static const char *wazer_values[3];

static BOOL g_initialized = FALSE;


//////////////////////////////////////////////////////////////////////////////////////////////////
static void init(void){

   if (g_initialized)
      return;

   popup_values[0] = POPUP_REPORT_VAL_ONLY_MAIN_GROUP;
   popup_values[1] = POPUP_REPORT_VAL_FOLLOWING_GROUPS;
   popup_values[2] = POPUP_REPORT_VAL_NONE;
   popup_labels[0] = roadmap_lang_get ("From my main group");
   popup_labels[1] = roadmap_lang_get ("From all groups I follow");
   popup_labels[2] = roadmap_lang_get ("None");

   wazer_values[0] = SHOW_WAZER_GROUP_VAL_MAIN;
   wazer_values[1] = SHOW_WAZER_GROUP_VAL_FOLLOWING;
   wazer_values[2] = SHOW_WAZER_GROUP_VAL_ALL;
   wazer_labels[0] = roadmap_lang_get ("My main group only");
   wazer_labels[1] = roadmap_lang_get ("The groups I follow");
   wazer_labels[2] = roadmap_lang_get ("All groups");

   g_initialized = TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void save_changes(){
   roadmap_groups_set_popup_config((const char *)ssd_dialog_get_data (GROUP_SETTING_POPUP_CHOICE));

   roadmap_groups_set_show_wazer_config((const char *)ssd_dialog_get_data (GROUP_SETTING_WAZER_CHOICE));

   roadmap_config_save(FALSE);
}

#ifndef TOUCH_SCREEN
//////////////////////////////////////////////////////////////////////////////////////////////////
static int on_save(SsdWidget widget, const char *new_value, void *context){
   save_changes();
   ssd_dialog_hide_current(dec_close);
   return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////
static void set_softkeys( SsdWidget dialog)
{
   ssd_widget_set_left_softkey_text       ( dialog, roadmap_lang_get("Ok"));
   ssd_widget_set_left_softkey_callback   ( dialog, on_save);
}
#endif
//////////////////////////////////////////////////////////////////////////////////////////////////
static void on_close_dialog (int exit_code, void* context){
   if (exit_code == dec_ok){
      save_changes();
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void create_ssd_dialog (void) {
   SsdWidget box, box2;
   SsdWidget container;
   SsdWidget dialog;
   int height = ssd_container_get_row_height();

   dialog = ssd_dialog_new (GROUP_SETTING_DLG_NAME,
                            roadmap_lang_get (GROUP_SETTING_DLG_TITLE),
                            on_close_dialog,
                            SSD_CONTAINER_TITLE);

   ssd_dialog_add_vspace (dialog, 5, 0);

   container = ssd_container_new (GROUP_SETTING_CONTAINER_NAME, NULL, ssd_container_get_width(), SSD_MIN_SIZE,
                            SSD_WIDGET_SPACE|SSD_END_ROW|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE|SSD_CONTAINER_BORDER|SSD_POINTER_NONE|SSD_ALIGN_CENTER);

   // PopUp settings
   box = ssd_container_new ("popup group", NULL, SSD_MAX_SIZE, height,
                             SSD_WIDGET_SPACE|SSD_END_ROW|SSD_WS_TABSTOP);
   ssd_widget_set_color (box, NULL, NULL);

   //box2 = ssd_container_new ("box2", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
   //      SSD_ALIGN_VCENTER);
   //ssd_widget_set_color (box2, NULL, NULL);

   ssd_widget_add (box,
       ssd_text_new ("popup_label",
                      roadmap_lang_get ("Pop-up reports"),
                      SSD_MAIN_TEXT_SIZE, SSD_TEXT_NORMAL_FONT|SSD_TEXT_LABEL|SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE));
   //ssd_widget_add(box, box2);

   ssd_widget_add (box,
          ssd_choice_new (GROUP_SETTING_POPUP_CHOICE, roadmap_lang_get ("Pop-up reports"), 3,
                          (const char **)popup_labels,
                          (const void **)popup_values,
                          SSD_ALIGN_VCENTER|SSD_ALIGN_RIGHT, NULL));

    ssd_widget_add (container, box);
    ssd_widget_add(container, ssd_separator_new("separator", SSD_END_ROW));

    // Wazer settings
    box = ssd_container_new ("wazer group", NULL, SSD_MAX_SIZE, height,
                             SSD_WIDGET_SPACE|SSD_END_ROW|SSD_WS_TABSTOP);
    ssd_widget_set_color (box, NULL, NULL);

    box2 = ssd_container_new ("box2", NULL, roadmap_canvas_width()/2, SSD_MIN_SIZE,
          SSD_ALIGN_VCENTER);
    ssd_widget_set_color (box2, NULL, NULL);

    ssd_widget_add (box2,
       ssd_text_new ("wazer_label",
                      roadmap_lang_get ("Wazers group icons"),
                      SSD_MAIN_TEXT_SIZE, SSD_TEXT_NORMAL_FONT|SSD_TEXT_LABEL|SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE));
    ssd_widget_add(box, box2);

    ssd_widget_add (box,
          ssd_choice_new (GROUP_SETTING_WAZER_CHOICE, roadmap_lang_get ("Wazers group icons"),3,
                          (const char **)wazer_labels,
                          (const void **)wazer_values,
                          SSD_ALIGN_VCENTER|SSD_ALIGN_RIGHT, NULL));
    ssd_widget_add (container, box);

    ssd_widget_add(dialog, container);
#ifndef TOUCH_SCREEN
    set_softkeys(dialog);
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void set_choice_values(){
   int popup_settings;
   int wazer_settings;
   const char *value;

   popup_settings = roadmap_groups_get_popup_config();
   if (popup_settings == POPUP_REPORT_NONE) value = popup_values[2];
   else if (popup_settings == POPUP_REPORT_FOLLOWING_GROUPS) value = popup_values[1];
   else if (popup_settings == POPUP_REPORT_ONLY_MAIN_GROUP) value = popup_values[0];
   else value = popup_values[2];
   ssd_dialog_set_data (GROUP_SETTING_POPUP_CHOICE, (void *) value);

   wazer_settings = roadmap_groups_get_show_wazer_config();
   if (wazer_settings == SHOW_WAZER_GROUP_ALL) value = wazer_values[2];
   else if (wazer_settings == SHOW_WAZER_GROUP_FOLLOWING) value = wazer_values[1];
   else if (wazer_settings == SHOW_WAZER_GROUP_MAIN) value = wazer_values[0];
   else value = wazer_values[0];
   ssd_dialog_set_data (GROUP_SETTING_WAZER_CHOICE, (void *) value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_groups_settings(void){
   init();

   if (!ssd_dialog_activate (GROUP_SETTING_DLG_NAME, NULL)) {
      create_ssd_dialog ();
      ssd_dialog_activate (GROUP_SETTING_DLG_NAME, NULL);
   }

   set_choice_values();
}
