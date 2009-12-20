/* roadmap_twitter.c - Manages Twitter accont
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi B.S
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

#include "roadmap_main.h"
#include "roadmap_config.h"
#include "roadmap_twitter.h"
#include "roadmap_dialog.h"
#include "roadmap_messagebox.h"
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_choice.h"
#include "ssd/ssd_button.h"
#include "ssd/ssd_entry.h"
#include "ssd/ssd_checkbox.h"
#include "ssd/ssd_separator.h"
#include "ssd/ssd_bitmap.h"

#include "Realtime/Realtime.h"
#include "Realtime/RealtimeDefs.h"


static const char *yesno[2];

static SsdWidget CheckboxDestination[ROADMAP_TWITTER_DESTINATION_MODES_COUNT];


//   User name
RoadMapConfigDescriptor TWITTER_CFG_PRM_NAME_Var = ROADMAP_CONFIG_ITEM(
      TWITTER_CONFIG_TAB,
      TWITTER_CFG_PRM_NAME_Name);

//   Password
RoadMapConfigDescriptor TWITTER_CFG_PRM_PASSWORD_Var = ROADMAP_CONFIG_ITEM(
      TWITTER_CONFIG_TAB,
      TWITTER_CFG_PRM_PASSWORD_Name);

//   Enable / Disable Tweeting road reports
RoadMapConfigDescriptor TWITTER_CFG_PRM_SEND_REPORTS_Var = ROADMAP_CONFIG_ITEM(
      TWITTER_CONFIG_TAB,
      TWITTER_CFG_PRM_SEND_REPORTS_Name);

//   Enable / Disable Tweeting destination
RoadMapConfigDescriptor TWITTER_CFG_PRM_SEND_DESTINATION_Var = ROADMAP_CONFIG_ITEM(
      TWITTER_CONFIG_TAB,
      TWITTER_CFG_PRM_SEND_DESTINATION_Name);

//   Enable / Disable Road goodie munching destination
RoadMapConfigDescriptor TWITTER_CFG_PRM_SEND_MUNCHING_Var = ROADMAP_CONFIG_ITEM(
      TWITTER_CONFIG_TAB,
      TWITTER_CFG_PRM_SEND_MUNCHING_Name);
RoadMapConfigDescriptor TWITTER_CFG_PRM_SHOW_MUNCHING_Var = ROADMAP_CONFIG_ITEM(
      TWITTER_CONFIG_TAB,
      TWITTER_CFG_PRM_SHOW_MUNCHING_Name);

//    Logged in status
RoadMapConfigDescriptor TWITTER_CFG_PRM_LOGGED_IN_Var = ROADMAP_CONFIG_ITEM(
      TWITTER_CONFIG_TAB,
      TWITTER_CFG_PRM_LOGGED_IN_Name);

/////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_twitter_initialize(void) {

   // Name
   roadmap_config_declare(TWITTER_CONFIG_TYPE, &TWITTER_CFG_PRM_NAME_Var,
         TWITTER_CFG_PRM_NAME_Default, NULL);

   // Password
   roadmap_config_declare_password(TWITTER_CONFIG_TYPE,
         &TWITTER_CFG_PRM_PASSWORD_Var, TWITTER_CFG_PRM_PASSWORD_Default);

   // Road reports
   roadmap_config_declare_enumeration(TWITTER_CONFIG_TYPE,
         &TWITTER_CFG_PRM_SEND_REPORTS_Var, NULL,
         TWITTER_CFG_PRM_SEND_REPORTS_Disabled, TWITTER_CFG_PRM_SEND_REPORTS_Enabled, NULL);

   // Destination
   roadmap_config_declare_enumeration(TWITTER_CONFIG_TYPE,
         &TWITTER_CFG_PRM_SEND_DESTINATION_Var, NULL,
         TWITTER_CFG_PRM_SEND_DESTINATION_Disabled, TWITTER_CFG_PRM_SEND_DESTINATION_City,
         TWITTER_CFG_PRM_SEND_DESTINATION_Street, TWITTER_CFG_PRM_SEND_DESTINATION_Full, NULL);

   // Road goodies munching
   roadmap_config_declare_enumeration(TWITTER_CONFIG_TYPE,
         &TWITTER_CFG_PRM_SEND_MUNCHING_Var, NULL,
         TWITTER_CFG_PRM_SEND_MUNCHING_Disabled, TWITTER_CFG_PRM_SEND_MUNCHING_Enabled, NULL);
   roadmap_config_declare_enumeration(TWITTER_CONFIG_PREF_TYPE,
            &TWITTER_CFG_PRM_SHOW_MUNCHING_Var, NULL,
            TWITTER_CFG_PRM_SHOW_MUNCHING_No, TWITTER_CFG_PRM_SHOW_MUNCHING_Yes, NULL);

   // Logged in status
   roadmap_config_declare_enumeration(TWITTER_CONFIG_TYPE,
            &TWITTER_CFG_PRM_LOGGED_IN_Var, NULL,
            TWITTER_CFG_PRM_LOGGED_IN_No, TWITTER_CFG_PRM_LOGGED_IN_Yes, NULL);

   yesno[0] = "Yes";
   yesno[1] = "No";

   return TRUE;

}



void roadmap_twitter_set_logged_in (BOOL is_logged_in) {
   if (is_logged_in)
      roadmap_config_set(&TWITTER_CFG_PRM_LOGGED_IN_Var, TWITTER_CFG_PRM_LOGGED_IN_Yes);
   else
      roadmap_config_set(&TWITTER_CFG_PRM_LOGGED_IN_Var, TWITTER_CFG_PRM_LOGGED_IN_No);
   roadmap_config_save(TRUE);
}

/////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_twitter_logged_in(void) {
   if (0 == strcmp(roadmap_config_get(&TWITTER_CFG_PRM_LOGGED_IN_Var),
         TWITTER_CFG_PRM_LOGGED_IN_Yes))
      return TRUE;
   return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_twitter_login_failed(int status) {
   if (roadmap_twitter_logged_in()) {
      if (status == 701)
         roadmap_messagebox("Error", "Updating your twitter account details failed. Please ensure you entered the correct user name and password");
      else
         roadmap_messagebox("Error", "Twitter error");
      roadmap_twitter_set_logged_in (FALSE);
   }
   roadmap_log (ROADMAP_DEBUG, "Twitter status error (%d)", status);
}

/////////////////////////////////////////////////////////////////////////////////////
const char *roadmap_twitter_get_username(void) {
   return roadmap_config_get(&TWITTER_CFG_PRM_NAME_Var);
}

/////////////////////////////////////////////////////////////////////////////////////
const char *roadmap_twitter_get_password(void) {
   return roadmap_config_get(&TWITTER_CFG_PRM_PASSWORD_Var);
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_twitter_set_username(const char *user_name) {
   roadmap_config_set(&TWITTER_CFG_PRM_NAME_Var, user_name); //AR: we save only username
   roadmap_config_save(TRUE);
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_twitter_set_password(const char *password) {
   roadmap_config_set(&TWITTER_CFG_PRM_PASSWORD_Var, TWITTER_CFG_PRM_PASSWORD_Default/*password*/); //AR: we don't want to save login details
   roadmap_config_save(TRUE);
}

/////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_twitter_is_sending_enabled(void) {
   if (0 == strcmp(roadmap_config_get(&TWITTER_CFG_PRM_SEND_REPORTS_Var),
         TWITTER_CFG_PRM_SEND_REPORTS_Enabled))
      return TRUE;
   return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_twitter_enable_sending() {
   roadmap_config_set(&TWITTER_CFG_PRM_SEND_REPORTS_Var,
         TWITTER_CFG_PRM_SEND_REPORTS_Enabled);
   roadmap_config_save(TRUE);
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_twitter_disable_sending() {
   roadmap_config_set(&TWITTER_CFG_PRM_SEND_REPORTS_Var,
         TWITTER_CFG_PRM_SEND_REPORTS_Disabled);
   roadmap_config_save(TRUE);
}

/////////////////////////////////////////////////////////////////////////////////////
int roadmap_twitter_destination_mode(void) {
   if (0 == strcmp(roadmap_config_get(&TWITTER_CFG_PRM_SEND_DESTINATION_Var),
         TWITTER_CFG_PRM_SEND_DESTINATION_Full))
      return ROADMAP_TWITTER_DESTINATION_MODE_FULL;
   else if (0 == strcmp(roadmap_config_get(&TWITTER_CFG_PRM_SEND_DESTINATION_Var),
         TWITTER_CFG_PRM_SEND_DESTINATION_City))
      return ROADMAP_TWITTER_DESTINATION_MODE_CITY;
   else
      return ROADMAP_TWITTER_DESTINATION_MODE_DISABLED;
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_twitter_set_destination_mode(int mode) {
   switch (mode) {
      case ROADMAP_TWITTER_DESTINATION_MODE_FULL:
         roadmap_config_set(&TWITTER_CFG_PRM_SEND_DESTINATION_Var,
               TWITTER_CFG_PRM_SEND_DESTINATION_Full);
         break;
      case ROADMAP_TWITTER_DESTINATION_MODE_CITY:
         roadmap_config_set(&TWITTER_CFG_PRM_SEND_DESTINATION_Var,
                TWITTER_CFG_PRM_SEND_DESTINATION_City);
         break;
      default:
         roadmap_config_set(&TWITTER_CFG_PRM_SEND_DESTINATION_Var,
                         TWITTER_CFG_PRM_SEND_DESTINATION_Disabled);
         break;
   }

   roadmap_config_save(TRUE);
}

/////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_twitter_is_munching_enabled(void) {
   if (0 == strcmp(roadmap_config_get(&TWITTER_CFG_PRM_SEND_MUNCHING_Var),
         TWITTER_CFG_PRM_SEND_MUNCHING_Enabled))
      return TRUE;
   return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_twitter_enable_munching() {
   roadmap_config_set(&TWITTER_CFG_PRM_SEND_MUNCHING_Var,
         TWITTER_CFG_PRM_SEND_MUNCHING_Enabled);
   roadmap_config_save(TRUE);
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_twitter_disable_munching() {
   roadmap_config_set(&TWITTER_CFG_PRM_SEND_MUNCHING_Var,
         TWITTER_CFG_PRM_SEND_MUNCHING_Disabled);
   roadmap_config_save(TRUE);
}

/////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_twitter_is_show_munching(void) {
   if (0 == strcmp(roadmap_config_get(&TWITTER_CFG_PRM_SHOW_MUNCHING_Var),
         TWITTER_CFG_PRM_SHOW_MUNCHING_Yes))
      return TRUE;
   return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////
static void twitter_un_empty(void){
   roadmap_main_remove_periodic (twitter_un_empty);
   //roadmap_twitter_setting_dialog();
   roadmap_messagebox("Error", "Twitter user name is empty. You are not logged in");
}

static void twitter_pw_empty(void){
   roadmap_main_remove_periodic (twitter_pw_empty);
   //roadmap_twitter_setting_dialog();
   roadmap_messagebox("Error", "Twitter password is empty. You are not logged in");
}

/////////////////////////////////////////////////////////////////////////////////////
static void twitter_network_error(void){
   roadmap_main_remove_periodic (twitter_network_error);
   roadmap_messagebox("Error", roadmap_lang_get("There is no network connection.Updating your twitter account details failed."));
   roadmap_twitter_set_logged_in (FALSE);
   roadmap_twitter_setting_dialog();
}
/////////////////////////////////////////////////////////////////////////////////////
static int on_ok() {
   BOOL success;
   BOOL send_reports = FALSE;
   int  destination_mode = 0;
   int i;
   BOOL send_munch = FALSE;
   const char *selected;
   BOOL logged_in;

   const char * user_name = ssd_dialog_get_value("TwitterUserName");
   const char * password = ssd_dialog_get_value("TwitterPassword");

   logged_in = roadmap_twitter_logged_in();


   if (!strcasecmp((const char*) ssd_dialog_get_data("TwitterSendTwitts"),
         yesno[0]))
      send_reports = TRUE;

   for (i = 0; i < ROADMAP_TWITTER_DESTINATION_MODES_COUNT; i++) {
      selected = ssd_dialog_get_data (CheckboxDestination[i]->name);
      if (!strcmp (selected, "yes")) {
         break;
      }
   }
   if (i == ROADMAP_TWITTER_DESTINATION_MODES_COUNT)
      i = 0;
   destination_mode = i;

   if (roadmap_twitter_is_show_munching())
      if (!strcasecmp((const char*) ssd_dialog_get_data("TwitterSendMunch"), yesno[0]))
         send_munch = TRUE;

   if (user_name[0] != 0 && password[0] != 0) {
      roadmap_twitter_set_username(user_name);
      roadmap_twitter_set_password(password);
      success = Realtime_TwitterConnect(user_name, password);
      if (!success) {
         roadmap_main_set_periodic (100, twitter_network_error);
         return 1;
      }
      roadmap_twitter_set_logged_in (TRUE);
   } else if (!logged_in && (send_reports || destination_mode > 0 || send_munch)){
      if (user_name[0] == 0) {
         roadmap_main_set_periodic (100, twitter_un_empty);
         return 1;
      }

      if (password[0] == 0) {
         roadmap_main_set_periodic (100, twitter_pw_empty);
         return 1;
      }
   }

   if (send_reports)
      roadmap_twitter_enable_sending();
   else
      roadmap_twitter_disable_sending();

   roadmap_twitter_set_destination_mode(destination_mode);

   if (send_munch)
      roadmap_twitter_enable_munching();
   else
      roadmap_twitter_disable_munching();


   return 0;
}

/////////////////////////////////////////////////////////////////////////////////////
static void on_dlg_close(int exit_code, void* context) {
   if (exit_code == dec_ok)
      on_ok();
}


#ifndef TOUCH_SCREEN
/////////////////////////////////////////////////////////////////////////////////////
static int on_ok_softkey(SsdWidget widget, const char *new_value, void *context) {
   ssd_dialog_hide_current(dec_ok);
   return 0;
}
#endif
/////////////////////////////////////////////////////////////////////
static SsdWidget space(int height) {
   SsdWidget space;
   space = ssd_container_new("spacer", NULL, SSD_MAX_SIZE, height,
         SSD_WIDGET_SPACE | SSD_END_ROW);
   ssd_widget_set_color(space, NULL, NULL);
   return space;
}

/////////////////////////////////////////////////////////////////////////////////////
int dest_checkbox_callback (SsdWidget widget, const char *new_value) {
   int i;
   for (i = 0; i < ROADMAP_TWITTER_DESTINATION_MODES_COUNT; i++) {
      if (CheckboxDestination[i] && strcmp (widget->parent->name,
            CheckboxDestination[i]->name))
         CheckboxDestination[i]->set_data (CheckboxDestination[i], "no");
      else
         CheckboxDestination[i]->set_data (CheckboxDestination[i], "yes");
   }
   return 1;
}

/////////////////////////////////////////////////////////////////////////////////////
static void create_dialog(void) {

   SsdWidget dialog;
   SsdWidget group;
   SsdWidget box;
   SsdWidget destination;
   SsdWidget hor_space;
   SsdWidget text;
   SsdWidget bitmap;
   int width;
   int tab_flag = SSD_WS_TABSTOP;
   BOOL checked = FALSE;
   const char * notesColor = "#3b3838"; // some sort of gray

   width = roadmap_canvas_width()/2;
   
   dialog = ssd_dialog_new(TWITTER_DIALOG_NAME,
         roadmap_lang_get(TWITTER_TITTLE), on_dlg_close, SSD_CONTAINER_TITLE);

#ifdef TOUCH_SCREEN
   ssd_widget_add (dialog, space(5));
#endif

   box = ssd_container_new("UN/PW group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
         SSD_WIDGET_SPACE | SSD_END_ROW | SSD_ROUNDED_CORNERS
               | SSD_ROUNDED_WHITE | SSD_POINTER_NONE | SSD_CONTAINER_BORDER);

   //Accound details header
   group = ssd_container_new("Twitter Account Header group", NULL, SSD_MAX_SIZE,SSD_MIN_SIZE,
         SSD_WIDGET_SPACE | SSD_END_ROW);
   ssd_widget_set_color(group, "#000000", "#ffffff");
   ssd_widget_add(group, ssd_text_new("Label", roadmap_lang_get("Account details"),
         16, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER));
   bitmap = ssd_bitmap_new ("twitter_icon", "Tweeter-logo",SSD_ALIGN_RIGHT|SSD_WS_TABSTOP);
   ssd_widget_add(group, bitmap);
   ssd_widget_add (group, ssd_separator_new ("separator", SSD_ALIGN_BOTTOM));
   ssd_widget_add(box, group);
   //Accound login status
   group = ssd_container_new("Twitter Account Login group", NULL, SSD_MAX_SIZE,SSD_MIN_SIZE,
         SSD_WIDGET_SPACE | SSD_END_ROW);
   ssd_widget_set_color(group, "#000000", "#ffffff");
   ssd_widget_add(group, ssd_text_new("Login Status Label", "",
            16, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER));
   ssd_widget_add (group, ssd_separator_new ("separator", SSD_ALIGN_BOTTOM));
   ssd_widget_add(box, group);
   //User name
   group = ssd_container_new("Twitter Name group", NULL, SSD_MAX_SIZE,SSD_MIN_SIZE,
         SSD_WIDGET_SPACE | SSD_END_ROW);
   ssd_widget_set_color(group, "#000000", "#ffffff");
   ssd_widget_add(group, ssd_text_new("Label", roadmap_lang_get("User name"),
         -1, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER));
   ssd_widget_add(group, ssd_entry_new("TwitterUserName", "", SSD_ALIGN_VCENTER
         | SSD_ALIGN_RIGHT | tab_flag, 0, width, SSD_MIN_SIZE,
         roadmap_lang_get("User name")));
   ssd_widget_add(box, group);

   //Password
   group = ssd_container_new("Twitter PW group", NULL, SSD_MAX_SIZE, 40,
         SSD_WIDGET_SPACE | SSD_END_ROW);
   ssd_widget_set_color(group, "#000000", "#ffffff");

   ssd_widget_add(group, ssd_text_new("Label", roadmap_lang_get("Password"),
         -1, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER));
   ssd_widget_add(group, ssd_entry_new("TwitterPassword", "", SSD_ALIGN_VCENTER
         | SSD_ALIGN_RIGHT | tab_flag, SSD_TEXT_PASSWORD, width, SSD_MIN_SIZE,
         roadmap_lang_get("Password")));
   ssd_widget_add(box, group);

   ssd_widget_add(dialog, box);


   //tweet settings header
   ssd_widget_add(dialog, space(5));
   box = ssd_container_new ("Tweeter auto settings header group", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE,
            SSD_WIDGET_SPACE | SSD_END_ROW);

   ssd_widget_add (box, ssd_text_new ("tweeter_auto_settings_header",
         roadmap_lang_get ("Automatically tweet to my followers:"), 16, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE));
   ssd_widget_set_color (box, NULL, NULL);
   ssd_widget_add (dialog, box);
   ssd_widget_add(dialog, space(5));

   //Send Reports Yes/No
   group = ssd_container_new("Send_Reports group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
         SSD_START_NEW_ROW | SSD_WIDGET_SPACE | SSD_END_ROW
                              | SSD_ROUNDED_CORNERS | SSD_ROUNDED_WHITE
                              | SSD_POINTER_NONE | SSD_CONTAINER_BORDER);
   ssd_widget_set_color(group, "#000000", "#ffffff");

   ssd_widget_add(group, ssd_text_new("Send_reports_label", roadmap_lang_get(
         "My road reports"), -1, SSD_TEXT_LABEL
         | SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE));

   ssd_widget_add(group, ssd_checkbox_new("TwitterSendTwitts", TRUE,
         SSD_END_ROW | SSD_ALIGN_RIGHT | SSD_ALIGN_VCENTER, NULL, NULL, NULL,
         CHECKBOX_STYLE_ON_OFF));
   ssd_widget_add(dialog, group);

   //example
   box = ssd_container_new ("report_example group", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE,
      SSD_WIDGET_SPACE | SSD_END_ROW);
   text = ssd_text_new ("report_example_text_cont",
      roadmap_lang_get ("e.g:  Just reported a traffic jam on Geary St. SF, CA using @waze Social GPS."),
                           14, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE);
   ssd_text_set_color(text,notesColor);
   ssd_widget_add (box,text );
   ssd_widget_set_color (box, NULL, NULL);
   ssd_widget_add (dialog, box);

   //Send Destination
   ssd_widget_add(dialog, space(5));
   destination = ssd_container_new("Send_Destination group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
            SSD_START_NEW_ROW | SSD_WIDGET_SPACE | SSD_END_ROW
                                 | SSD_ROUNDED_CORNERS | SSD_ROUNDED_WHITE
                                 | SSD_POINTER_NONE | SSD_CONTAINER_BORDER);
   box = ssd_container_new ("Destination Heading group", NULL, SSD_MAX_SIZE,SSD_MIN_SIZE,
         SSD_WIDGET_SPACE | SSD_END_ROW);
   ssd_widget_add (box, ssd_text_new ("destination_heading_label",
            roadmap_lang_get ("My destination and ETA"), 16, SSD_TEXT_LABEL
                     | SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE | SSD_END_ROW));

   ssd_widget_add (box, ssd_separator_new ("separator", SSD_ALIGN_BOTTOM));
   ssd_widget_set_color (box, NULL, NULL);
   ssd_widget_add (destination, box);
   //Disabled
   box = ssd_container_new ("Destination disabled Group", NULL, SSD_MAX_SIZE,SSD_MIN_SIZE,
         SSD_WIDGET_SPACE | SSD_END_ROW);
   if (roadmap_twitter_destination_mode() == ROADMAP_TWITTER_DESTINATION_MODE_DISABLED)
      checked = TRUE;
   else
      checked = FALSE;

   CheckboxDestination[ROADMAP_TWITTER_DESTINATION_MODE_DISABLED] = ssd_checkbox_new (TWITTER_CFG_PRM_SEND_DESTINATION_Disabled,
               checked, SSD_ALIGN_VCENTER, dest_checkbox_callback, NULL, NULL,
               CHECKBOX_STYLE_ROUNDED);
   ssd_widget_add (box, CheckboxDestination[ROADMAP_TWITTER_DESTINATION_MODE_DISABLED]);

   hor_space = ssd_container_new ("spacer1", NULL, 10, 14, 0);
   ssd_widget_set_color (hor_space, NULL, NULL);
   ssd_widget_add (box, hor_space);

   ssd_widget_add (box, ssd_text_new ("Destination disabled", roadmap_lang_get (
            "Disabled"), 14, SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE
            | SSD_END_ROW));

   ssd_widget_add (box, ssd_separator_new ("separator", SSD_ALIGN_BOTTOM));
   ssd_widget_set_color (box, NULL, NULL);
   ssd_widget_add (destination, box);


   // State and City
   box = ssd_container_new ("Destination city Group", NULL, SSD_MAX_SIZE,SSD_MIN_SIZE,
         SSD_WIDGET_SPACE | SSD_END_ROW);
   if (roadmap_twitter_destination_mode() == ROADMAP_TWITTER_DESTINATION_MODE_CITY)
      checked = TRUE;
   else
      checked = FALSE;

   CheckboxDestination[ROADMAP_TWITTER_DESTINATION_MODE_CITY] = ssd_checkbox_new (TWITTER_CFG_PRM_SEND_DESTINATION_City,
               checked, SSD_ALIGN_VCENTER, dest_checkbox_callback, NULL, NULL,
               CHECKBOX_STYLE_ROUNDED);
   ssd_widget_add (box, CheckboxDestination[ROADMAP_TWITTER_DESTINATION_MODE_CITY]);

   hor_space = ssd_container_new ("spacer1", NULL, 10, 14, 0);
   ssd_widget_set_color (hor_space, NULL, NULL);
   ssd_widget_add (box, hor_space);

   ssd_widget_add (box, ssd_text_new ("Destination city", roadmap_lang_get (
            "City & state only"), 14, SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE
            | SSD_END_ROW));

   ssd_widget_add (box, ssd_separator_new ("separator", SSD_ALIGN_BOTTOM));
   ssd_widget_set_color (box, NULL, NULL);
   ssd_widget_add (destination, box);

   // Street, State and City (future option)
   box = ssd_container_new ("Destination street Group", NULL, SSD_MAX_SIZE,SSD_MIN_SIZE,
         SSD_WIDGET_SPACE | SSD_END_ROW);
   if (roadmap_twitter_destination_mode() == ROADMAP_TWITTER_DESTINATION_MODE_STREET)
      checked = TRUE;
   else
      checked = FALSE;

   CheckboxDestination[ROADMAP_TWITTER_DESTINATION_MODE_STREET] = ssd_checkbox_new (TWITTER_CFG_PRM_SEND_DESTINATION_Street,
               checked, SSD_ALIGN_VCENTER, dest_checkbox_callback, NULL, NULL,
               CHECKBOX_STYLE_ROUNDED);

   ssd_widget_add (box, CheckboxDestination[ROADMAP_TWITTER_DESTINATION_MODE_STREET]);

   hor_space = ssd_container_new ("spacer1", NULL, 10, 14, 0);
   ssd_widget_set_color (hor_space, NULL, NULL);
   ssd_widget_add (box, hor_space);

   ssd_widget_add (box, ssd_text_new ("Destination street", roadmap_lang_get (
            "Street, City & State"), 14, SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE
            | SSD_END_ROW));

   ssd_widget_add (box, ssd_separator_new ("separator", SSD_ALIGN_BOTTOM));
   ssd_widget_set_color (box, NULL, NULL);
   ssd_widget_add (destination, box);
   ssd_widget_hide(box);//future option


   // Full address
   box = ssd_container_new ("Destination full Group", NULL, SSD_MAX_SIZE,SSD_MIN_SIZE,
         SSD_WIDGET_SPACE | SSD_END_ROW);
   if (roadmap_twitter_destination_mode() == ROADMAP_TWITTER_DESTINATION_MODE_FULL)
      checked = TRUE;
   else
      checked = FALSE;

   CheckboxDestination[ROADMAP_TWITTER_DESTINATION_MODE_FULL] = ssd_checkbox_new (TWITTER_CFG_PRM_SEND_DESTINATION_Full,
               checked, SSD_ALIGN_VCENTER, dest_checkbox_callback, NULL, NULL,
               CHECKBOX_STYLE_ROUNDED);
   ssd_widget_add (box, CheckboxDestination[ROADMAP_TWITTER_DESTINATION_MODE_FULL]);

   hor_space = ssd_container_new ("spacer1", NULL, 10, 14, 0);
   ssd_widget_set_color (hor_space, NULL, NULL);
   ssd_widget_add (box, hor_space);

   ssd_widget_add (box, ssd_text_new ("Destination full", roadmap_lang_get (
            "House #, Street, City, State"), 14, SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE
            | SSD_END_ROW));

   ssd_widget_set_color (box, NULL, NULL);
   ssd_widget_add (destination, box);

   ssd_widget_add (dialog, destination);

   //example
   box = ssd_container_new ("destination_example group", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE,
      SSD_WIDGET_SPACE | SSD_END_ROW);
   text = ssd_text_new ("destination_example_text_cont",
      roadmap_lang_get ("e.g:  Driving to Greary St. SF, using @waze social GPS. ETA 2:32pm."),
                           14, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE);
   ssd_text_set_color(text,notesColor);
   ssd_widget_add (box,text );
   ssd_widget_set_color (box, NULL, NULL);
   ssd_widget_add (dialog, box);

   //Send road goodie munch Yes/No
   if (roadmap_twitter_is_show_munching()) {
      ssd_widget_add(dialog, space(5));
      group = ssd_container_new("Send_Munch group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
            SSD_START_NEW_ROW | SSD_WIDGET_SPACE | SSD_END_ROW
                                 | SSD_ROUNDED_CORNERS | SSD_ROUNDED_WHITE
                                 | SSD_POINTER_NONE | SSD_CONTAINER_BORDER);
      ssd_widget_set_color(group, "#000000", "#ffffff");

      ssd_widget_add(group, ssd_text_new("Send_munch_label", roadmap_lang_get(
            "My road munching"), -1, SSD_TEXT_LABEL
            | SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE));

      ssd_widget_add(group, ssd_checkbox_new("TwitterSendMunch", TRUE,
            SSD_END_ROW | SSD_ALIGN_RIGHT | SSD_ALIGN_VCENTER, NULL, NULL, NULL,
            CHECKBOX_STYLE_ON_OFF));
      ssd_widget_add(dialog, group);

      //example
      box = ssd_container_new ("munch_example group", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE,
         SSD_WIDGET_SPACE | SSD_END_ROW);
      text = ssd_text_new ("munch_example_text_cont",
         roadmap_lang_get ("e.g:  Just munched a 'waze road goodie' worth 200 points on Geary St. SF driving with @waze social GPS"),
                              14, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE);
      ssd_text_set_color(text,notesColor);
      ssd_widget_add (box,text );
      ssd_widget_set_color (box, NULL, NULL);
      ssd_widget_add (dialog, box);
   }

#ifndef TOUCH_SCREEN      
   ssd_widget_set_left_softkey_text ( dialog, roadmap_lang_get("Ok"));
   ssd_widget_set_left_softkey_callback ( dialog, on_ok_softkey);
#endif

}

#ifndef IPHONE_NATIVE
/////////////////////////////////////////////////////////////////////////////////////
void roadmap_twitter_setting_dialog(void) {
   const char *pVal;

   if (!ssd_dialog_activate(TWITTER_DIALOG_NAME, NULL)) {
      create_dialog();
      ssd_dialog_activate(TWITTER_DIALOG_NAME, NULL);
   }

   if (roadmap_twitter_logged_in())
      ssd_dialog_set_value("Login Status Label", roadmap_lang_get("Status: logged in"));
   else
      ssd_dialog_set_value("Login Status Label", roadmap_lang_get("Status: not logged in"));

   ssd_dialog_set_value("TwitterUserName", roadmap_twitter_get_username());
   ssd_dialog_set_value("TwitterPassword", roadmap_twitter_get_password());

   if (roadmap_twitter_is_sending_enabled())
      pVal = yesno[0];
   else
      pVal = yesno[1];
   ssd_dialog_set_data("TwitterSendTwitts", (void *) pVal);

   if (roadmap_twitter_is_munching_enabled())
         pVal = yesno[0];
      else
         pVal = yesno[1];
      ssd_dialog_set_data("TwitterSendMunch", (void *) pVal);
}
#endif //IPHONE_NATIVE
