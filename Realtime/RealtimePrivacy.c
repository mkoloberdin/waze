/* RealtimePrivacy.h - Manages users privacy settings
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi B.S
 *   Copyright 2008 Ehud Shabtai
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

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../roadmap.h"
#include "../roadmap_lang.h"
#include "../roadmap_screen.h"
#include "../roadmap_config.h"
#include "../roadmap_messagebox.h"
#include "../roadmap_social.h"

#include "Realtime.h"
#include "RealtimeDefs.h"
#include "RealtimePrivacy.h"

#include "ssd/ssd_dialog.h"
#include "ssd/ssd_checkbox.h"
#include "ssd/ssd_contextmenu.h"
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_button.h"
#include "ssd/ssd_bitmap.h"
#include "ssd/ssd_separator.h"

#ifndef TOUCH_SCREEN
static BOOL g_context_menu_is_active= FALSE;
#endif

#define NUM_CHECKBOXES_DRIVING   2
#define NUM_CHECKBOXES_REPORTING 2

static SsdWidget CheckboxDriving[NUM_CHECKBOXES_DRIVING];
//static SsdWidget CheckboxReporting[NUM_CHECKBOXES_REPORTING];

static ERTVisabilityGroup gState = VisGrp_Anonymous;
//static ERTVisabilityReport gReportState = VisRep_Anonymous;

extern RoadMapConfigDescriptor RT_CFG_PRM_VISGRP_Var;
extern RoadMapConfigDescriptor RT_CFG_PRM_VISREP_Var;

void save_changes () {
   int i;
   const char *selected;

   for (i = 0; i < NUM_CHECKBOXES_DRIVING; i++) {
      selected = ssd_dialog_get_data (CheckboxDriving[i]->name);
      if (!strcmp (selected, "yes")) {
         break;
      }
   }

   gState = ERTVisabilityGroup_from_string (CheckboxDriving[i]->name);
   roadmap_config_set (&RT_CFG_PRM_VISGRP_Var,
            (const char *) CheckboxDriving[i]->name);

//   for (i = 0; i < NUM_CHECKBOXES_REPORTING; i++) {
//      selected = ssd_dialog_get_data (CheckboxReporting[i]->name);
//      if (!strcmp (selected, "yes")) {
//         break;
//      }
//   }
//   gReportState = ERTVisabilityReport_from_string (CheckboxReporting[i]->name);
//
//   roadmap_config_set (&RT_CFG_PRM_VISREP_Var,
//            (const char *) CheckboxReporting[i]->name);

   roadmap_config_save (FALSE);

   OnSettingsChanged_VisabilityGroup ();
   roadmap_social_send_permissions();
}
#ifndef TOUCH_SCREEN

///////////////////////////////////////////////////////////////////////////////////////////
static void on_option_selected( BOOL made_selection,
         ssd_cm_item_ptr item,
         void* context)
{

   privacy_context_menu_items selection;

   int exit_code = dec_ok;
   g_context_menu_is_active = FALSE;

   if( !made_selection)
   return;

   selection = item->id;
   switch( selection)
   {

      case privacy_cm_save:
      save_changes();
      ssd_dialog_hide_current(dec_close);
      break;

      case privacy_cm_exit:
      exit_code = dec_cancel;
      ssd_dialog_hide_all( exit_code);
      roadmap_screen_refresh ();
      break;

      default:
      break;
   }
}

///////////////////////////////////////////////////////////////////////////////////////////

// Context menu items:
static ssd_cm_item main_menu_items[] =
{
   //                  Label     ,           Item-ID
   SSD_CM_INIT_ITEM ( "Save", privacy_cm_save),
   SSD_CM_INIT_ITEM ( "Exit_key", privacy_cm_exit)
};

// Context menu:
static ssd_contextmenu context_menu = SSD_CM_INIT_MENU( main_menu_items);

static int on_options(SsdWidget widget, const char *new_value, void *context)
{
   int menu_x;

   if(g_context_menu_is_active)
   {
      ssd_dialog_hide_current(dec_ok);
      g_context_menu_is_active = FALSE;
   }

   if (ssd_widget_rtl (NULL))
   menu_x = SSD_X_SCREEN_RIGHT;
   else
   menu_x = SSD_X_SCREEN_LEFT;

   ssd_context_menu_show( menu_x, // X
            SSD_Y_SCREEN_BOTTOM, // Y
            &context_menu,
            on_option_selected,
            NULL,
            dir_default,
            0,
            TRUE);

   g_context_menu_is_active = TRUE;

   return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////
static void set_softkeys( SsdWidget dialog)
{
   ssd_widget_set_left_softkey_text ( dialog, roadmap_lang_get("Options"));
   ssd_widget_set_left_softkey_callback ( dialog, on_options);
}
#endif

static int checkbox_callback (SsdWidget widget, const char *new_value) {
   int i;
   for (i = 0; i < NUM_CHECKBOXES_DRIVING; i++) {
      if (CheckboxDriving[i]) {
         if (strcmp (widget->parent->name,
                     CheckboxDriving[i]->name))
            CheckboxDriving[i]->set_data (CheckboxDriving[i], "no");
         else
            CheckboxDriving[i]->set_data (CheckboxDriving[i], "yes");
      }
   }
   return 1;
}

//int rep_checkbox_callback (SsdWidget widget, const char *new_value) {
//   int i;
//   for (i = 0; i < NUM_CHECKBOXES_REPORTING; i++) {
//      if (CheckboxReporting[i]) {
//         if (strcmp (widget->parent->name,
//                     CheckboxReporting[i]->name))
//            CheckboxReporting[i]->set_data (CheckboxReporting[i], "no");
//         else
//            CheckboxReporting[i]->set_data (CheckboxReporting[i], "yes");
//      }
//   }
//   return 1;
//}

static void on_close_dialog (int exit_code, void* context) {
#ifdef TOUCH_SCREEN
   if (exit_code == dec_ok) save_changes ();
#endif
}
static void create_dialog (void) {
   SsdWidget dialog;
   SsdWidget box, driving, space;
   char *icon[2];
   int i = 0;
   BOOL checked = FALSE;
   int container_height = 24;
   int tab_flag = SSD_WS_TABSTOP;

#ifdef TOUCH_SCREEN
  container_height = ssd_container_get_row_height();
#endif


   dialog = ssd_dialog_new (PRIVACY_DIALOG, roadmap_lang_get (
            PRIVACY_TITLE), on_close_dialog, SSD_CONTAINER_TITLE);

#ifdef TOUCH_SCREEN
   space = ssd_container_new ("spacer", NULL, SSD_MAX_SIZE, 5, SSD_WIDGET_SPACE
            | SSD_END_ROW);
   ssd_widget_set_color (space, NULL, NULL);
   ssd_widget_add (dialog, space);

#endif
   box = ssd_container_new ("Privacy Heading group", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE,
            SSD_WIDGET_SPACE | SSD_END_ROW);

   ssd_widget_add (box, ssd_text_new ("privacy_heading_label",
            roadmap_lang_get ("Display my location on waze mobile and web maps as follows:"), SSD_HEADER_TEXT_SIZE,
            SSD_TEXT_NORMAL_FONT|SSD_TEXT_LABEL | SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE));
   ssd_widget_set_color (box, NULL, NULL);


   ssd_widget_set_color (box, NULL, NULL);

   //////////////////////////////////////////////////
   // * Driving
   //////////////////////////////////////////////////
   driving = ssd_container_new ("Report privacy", NULL, ssd_container_get_width(),
            SSD_MIN_SIZE, SSD_WIDGET_SPACE | SSD_ROUNDED_CORNERS
                     | SSD_ROUNDED_WHITE | SSD_POINTER_NONE
                     | SSD_CONTAINER_BORDER | SSD_ALIGN_CENTER);
   ssd_widget_add (driving, box);
   box = ssd_container_new ("Driving Heading group", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE,
            SSD_WIDGET_SPACE | SSD_END_ROW);

   ssd_widget_add (box, ssd_text_new ("driving_heading_label",
            roadmap_lang_get ("When driving"), SSD_HEADER_TEXT_SIZE, SSD_TEXT_NORMAL_FONT|SSD_TEXT_LABEL
                     | SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE | SSD_END_ROW));

   ssd_widget_add (box, ssd_separator_new ("separator", SSD_ALIGN_BOTTOM));
   ssd_widget_set_color (box, NULL, NULL);
   ssd_widget_add (driving, box);

   //////////////////////////////////////////////////
   // * Nickname
   //////////////////////////////////////////////////
   box = ssd_container_new ("Nickname group", NULL, SSD_MAX_SIZE,
                  container_height, SSD_WIDGET_SPACE | SSD_END_ROW | tab_flag);

   if (gState == VisGrp_NickName)
      checked = TRUE;
   else
      checked = FALSE;

   CheckboxDriving[i] = ssd_checkbox_new (RT_CFG_PRM_VISGRP_Nickname, checked,
            SSD_ALIGN_VCENTER | SSD_ALIGN_RIGHT, checkbox_callback, NULL, NULL,
            CHECKBOX_STYLE_V);
   ssd_widget_add (box, CheckboxDriving[i]);
   i++;

   icon[0] = "privacy_nickname";
   icon[1] = NULL;

   ssd_widget_add (box, ssd_button_new ("privacy_nickname", "privacy_nickname",
            (const char **) &icon[0], 1, SSD_ALIGN_VCENTER, NULL));
   ssd_widget_set_color (box, NULL, NULL);

   ssd_widget_add (box, ssd_text_new ("Nickname", roadmap_lang_get (
            "Nickname"), SSD_MAIN_TEXT_SIZE, SSD_TEXT_NORMAL_FONT | SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE));

   ssd_widget_add (box, ssd_bitmap_new ("On_map_nickname", "On_map_nickname",
            SSD_ALIGN_VCENTER));

   ssd_widget_add (driving, box);
   ssd_widget_add (driving, ssd_separator_new ("separator", SSD_END_ROW));

   //////////////////////////////////////////////////
   // * Anonymous
   //////////////////////////////////////////////////
   box = ssd_container_new ("Anonymous group", NULL, SSD_MAX_SIZE,
                  container_height, SSD_WIDGET_SPACE | SSD_END_ROW | tab_flag);

   if (gState == VisGrp_Anonymous)
      checked = TRUE;
   else
      checked = FALSE;

   CheckboxDriving[i] = ssd_checkbox_new (RT_CFG_PRM_VISGRP_Anonymous, checked,
            SSD_ALIGN_VCENTER | SSD_ALIGN_RIGHT, checkbox_callback, NULL, NULL,
            CHECKBOX_STYLE_V);
   ssd_widget_add (box, CheckboxDriving[i]);
   i++;
   icon[0] = "privacy_anonymous";
   icon[1] = NULL;

   ssd_widget_add (box, ssd_button_new ("privacy_anonymous",
            "privacy_anonymous", (const char **) &icon[0], 1,
            SSD_ALIGN_VCENTER, NULL));

   ssd_widget_add (box, ssd_text_new ("Anonymous text", roadmap_lang_get (
            "Anonymous"), SSD_MAIN_TEXT_SIZE, SSD_TEXT_NORMAL_FONT | SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE));
   ssd_widget_add (box, ssd_bitmap_new ("On_map_anonymous", "On_map_anonymous",
            SSD_ALIGN_VCENTER));

   ssd_widget_set_color (box, NULL, NULL);
   ssd_widget_add (driving, box);
   /*
   //////////////////////////////////////////////////
   // * Invisible
   //////////////////////////////////////////////////

   box = ssd_container_new ("Invisible Group", NULL, SSD_MAX_SIZE,
                  container_height, SSD_WIDGET_SPACE | SSD_END_ROW | tab_flag);
   if (gState == VisGrp_Invisible)
      checked = TRUE;
   else
      checked = FALSE;

   CheckboxDriving[i] = ssd_checkbox_new (RT_CFG_PRM_VISGRP_Invisible, checked,
            SSD_ALIGN_VCENTER, checkbox_callback, NULL, NULL,
            CHECKBOX_STYLE_ROUNDED);
   ssd_widget_add (box, CheckboxDriving[i]);
   i++;
   icon[0] = "privacy_invisible";
   icon[1] = NULL;

   ssd_widget_add (box, ssd_button_new ("privacy_invisible",
            "privacy_invisible", (const char **) &icon[0], 1,
            SSD_ALIGN_VCENTER, NULL));
   ssd_widget_add (box, ssd_text_new ("Invisible Text", roadmap_lang_get (
            "Don't show me"), 14, SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE
            | SSD_END_ROW));

   ssd_widget_set_color (box, NULL, NULL);
   ssd_widget_add (driving, box);
*/
   ssd_widget_add (dialog, driving);
/*
   //////////////////////////////////////////////////
   // * Report
   //////////////////////////////////////////////////

   report = ssd_container_new ("Report privacy", NULL, SSD_MIN_SIZE,
            SSD_MIN_SIZE, SSD_START_NEW_ROW | SSD_WIDGET_SPACE | SSD_END_ROW
                     | SSD_ROUNDED_CORNERS | SSD_ROUNDED_WHITE
                     | SSD_POINTER_NONE | SSD_CONTAINER_BORDER);

   box = ssd_container_new ("Reporting Heading group", NULL, SSD_MIN_SIZE,
                  container_height, SSD_WIDGET_SPACE | SSD_END_ROW);
   ssd_widget_add (box, ssd_text_new ("reporting_heading_label",
            roadmap_lang_get ("When reporting"), 14, SSD_TEXT_LABEL
                     | SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE | SSD_END_ROW));

   ssd_widget_add (box, ssd_separator_new ("separator", SSD_ALIGN_BOTTOM));
   ssd_widget_set_color (box, NULL, NULL);
   ssd_widget_add (report, box);

   i = 0;
   //////////////////////////////////////////////////
   // * Report Nickname
   //////////////////////////////////////////////////
   box = ssd_container_new ("Report Nickname Group", NULL, SSD_MIN_SIZE,
                  container_height, SSD_WIDGET_SPACE | SSD_END_ROW | tab_flag);
   if (gReportState == VisRep_NickName)
      checked = TRUE;
   else
      checked = FALSE;

   CheckboxReporting[i] = ssd_checkbox_new (RT_CFG_PRM_VISREP_Nickname,
            checked, SSD_ALIGN_VCENTER, rep_checkbox_callback, NULL, NULL,
            CHECKBOX_STYLE_ROUNDED);
   ssd_widget_add (box, CheckboxReporting[i]);
   i++;

   space = ssd_container_new ("spacer1", NULL, 10, 14, 0);
   ssd_widget_set_color (space, NULL, NULL);
   ssd_widget_add (box, space);

   ssd_widget_add (box, ssd_text_new ("Report Nickname", roadmap_lang_get (
            "Nickname"), 14, SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE
            | SSD_END_ROW));

   ssd_widget_add (box, ssd_separator_new ("separator", SSD_ALIGN_BOTTOM));
   ssd_widget_set_color (box, NULL, NULL);
   ssd_widget_add (report, box);

   //////////////////////////////////////////////////
   // * Report Anonymous
   //////////////////////////////////////////////////
   box = ssd_container_new ("Report Anonymous Group", NULL, SSD_MAX_SIZE,
                  container_height, SSD_WIDGET_SPACE | SSD_END_ROW | tab_flag);
   if (gReportState == VisRep_Anonymous)
      checked = TRUE;
   else
      checked = FALSE;

   CheckboxReporting[i] = ssd_checkbox_new (RT_CFG_PRM_VISREP_Anonymous,
            checked, SSD_ALIGN_VCENTER, rep_checkbox_callback, NULL, NULL,
            CHECKBOX_STYLE_ROUNDED);
   ssd_widget_add (box, CheckboxReporting[i]);
   i++;

   space = ssd_container_new ("spacer1", NULL, 10, 14, 0);
   ssd_widget_set_color (space, NULL, NULL);
   ssd_widget_add (box, space);

   ssd_widget_add (box, ssd_text_new ("Report Anonymous", roadmap_lang_get (
            "Anonymous"), 14, SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE));

   ssd_widget_set_color (box, NULL, NULL);
   ssd_widget_add (report, box);

   ssd_widget_add (dialog, report);
   */
#ifndef TOUCH_SCREEN
   set_softkeys(dialog);
#endif
}

static void set_state () {
   gState = ERTVisabilityGroup_from_string (roadmap_config_get (
            &RT_CFG_PRM_VISGRP_Var));

//   gReportState = ERTVisabilityReport_from_string (roadmap_config_get (
//            &RT_CFG_PRM_VISREP_Var));
}

void update_checked () {
   int i;
   if (!CheckboxDriving[0]) return;

   for (i = 0; i < NUM_CHECKBOXES_DRIVING; i++) {
      if (!strcmp (ERTVisabilityGroup_to_string (gState),
               CheckboxDriving[i]->name))
         ssd_dialog_set_data (CheckboxDriving[i]->name, "yes");
      else
         ssd_dialog_set_data (CheckboxDriving[i]->name, "no");
   }

//   for (i = 0; i < NUM_CHECKBOXES_REPORTING; i++) {
//      if (!strcmp (ERTVisabilityReport_to_string (gReportState),
//               CheckboxReporting[i]->name))
//         ssd_dialog_set_data (CheckboxReporting[i]->name, "yes");
//      else
//         ssd_dialog_set_data (CheckboxReporting[i]->name, "no");
//   }
}

int RealtimePrivacyState () {

   if (!RealTimeLoginState ()) return 0;

#ifdef IPHONE
	set_state();
#endif //IPHONE

   return gState;
}

void RealtimePrivacyInit () {

   set_state ();

}

void RealtimePrivacySettings (void) {
#ifndef IPHONE

   set_state ();

   if (!ssd_dialog_activate (PRIVACY_DIALOG, NULL)) {

      create_dialog ();
      ssd_dialog_activate (PRIVACY_DIALOG, NULL);
   }
   update_checked ();

#else

	privacy_settings_show();

#endif //IPHONE
}

int RealtimePrivacySettingsWidgetCallBack(SsdWidget widget, const char *new_value){
   RealtimePrivacySettings();
   return 1;
}
