/* roadmap_reminder.c
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi Ben-Shoshan
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
#include "roadmap_main.h"
#include "roadmap_config.h"
#include "roadmap_trip.h"
#include "roadmap_reminder.h"
#include "roadmap_screen.h"
#include "roadmap_street.h"
#include "roadmap_math.h"
#include "roadmap_layer.h"
#include "roadmap_lang.h"
#include "roadmap_string.h"
#include "roadmap_messagebox.h"
#include "roadmap_object.h"
#include "roadmap_history.h"
#include "roadmap_res.h"
#include "roadmap_sound.h"

#include "ssd/ssd_widget.h"
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_entry.h"
#include "ssd/ssd_button.h"
#include "ssd/ssd_popup.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_bitmap.h"
#include "ssd/ssd_choice.h"
#include "ssd/ssd_separator.h"
#include "ssd/ssd_checkbox.h"

#define REMINDER_POP_DLG_NAME "ReminderPopUp"
#define REMINDER_DLG_NAME     "ReminderDlg"
#define REMINDER_DLG_TITLE    "New Geo-Reminder"
#define REMINDER_SOUND        "reminder"

#define REMINDER_ID_MAXSIZE   25
#define MAX_REMINDER_TITLE    100
#define MAX_REMINDER_DESC     256
#define MAX_REMINDER_ENTRIES  100

#define REMINDER_CATEGORY 'S'

static RoadMapConfigDescriptor RoadMapConfigFeatureEnabled =
ROADMAP_CONFIG_ITEM("Geo-Reminder", "Feature enabled");

typedef struct reminder_context_st{
   PluginStreetProperties properties;
   RoadMapPosition position;
}reminder_context;

typedef enum reminder_repeat
   {
      repeat_once,
      repeat_every_time
   }  reminder_repeat;

static reminder_context gContext;

static void register_gps_listener(void);
static int  get_id(const char *id);
static void remider_add_pin(int iID, const RoadMapPosition *position);
static void OnReminderShortClick (const char *name,
                                  const char *sprite,
                                  RoadMapDynamicString *images,
                                  int  image_count,
                                  const RoadMapGpsPosition *gps_position,
                                  const RoadMapGuiPoint    *offset,
                                  BOOL is_visible,
                                  int scale,
                                  int opacity,
                                  int scale_y,
                                  const char *id,
                                  ObjectText *texts,
                                  int        text_count,
                                  int rotation);

//////////////////////////////////////////////////////////////////////////////////////////////////
// Reminder
typedef struct
   {
      int              iID;
      RoadMapPosition  position;
      char             title[MAX_REMINDER_TITLE];
      char             description[MAX_REMINDER_DESC];
      int              distance;
      void             *history;
      BOOL             displayed;
      reminder_repeat  repeat;
      BOOL             in_use;
   } Reminder;

typedef struct
   {
      Reminder reminder[MAX_REMINDER_ENTRIES];
      int iCount;
   } ReminderTableStr;

static ReminderTableStr ReminderTable;

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_reminder_feature_enabled (void) {
#ifdef IPHONE_NATIVE
   return FALSE; // Feature not ready yet for iPhone
#endif //IPHONE_NATIVE
   if (0 == strcmp (roadmap_config_get (&RoadMapConfigFeatureEnabled), "yes_new"))
      return TRUE;
   return FALSE;

}

//////////////////////////////////////////////////////////////////
static RoadMapDynamicString generate_id(int iID){
   char guid_reminder[REMINDER_ID_MAXSIZE];
   RoadMapDynamicString GUI_ID_REMINDER;

   snprintf(guid_reminder, sizeof(guid_reminder), "%d_reminder",iID);
   GUI_ID_REMINDER = roadmap_string_new(guid_reminder);

   return GUI_ID_REMINDER;
}

//////////////////////////////////////////////////////////////////
static RoadMapDynamicString generate_name(int iID){
   char guid_reminder[REMINDER_ID_MAXSIZE];
   RoadMapDynamicString dName;

   snprintf(guid_reminder, sizeof(guid_reminder), "reminder_object_%d",iID);
   dName = roadmap_string_new(guid_reminder);

   return dName;
}

//////////////////////////////////////////////////////////////////
static void reminder_remove_pin(char *ID){
   RoadMapDynamicString GUI_ID_REMINDER = roadmap_string_new(ID);

   roadmap_object_remove(GUI_ID_REMINDER);

   roadmap_string_release(GUI_ID_REMINDER);
}

//////////////////////////////////////////////////////////////////
static void remider_add_pin(int iID, const RoadMapPosition *position){
   RoadMapDynamicString dID = generate_id(iID);
   RoadMapDynamicString dName = generate_name(iID);
   RoadMapDynamicString dSprite= roadmap_string_new( "Reminder");
   RoadMapDynamicString dGroup = roadmap_string_new( "Reminders");
   RoadMapDynamicString dImage_Reminder = roadmap_string_new( "reminder_pin");
   RoadMapGpsPosition   Pos;

   Pos.longitude  = position->longitude;
   Pos.latitude   = position->latitude;
   Pos.altitude   = 0;
   Pos.speed      = 0;
   Pos.steering   = 0;

   roadmap_object_add( dGroup, dID, dName, dSprite, dImage_Reminder, &Pos, NULL, 0, NULL);
   roadmap_object_set_action(dID, OnReminderShortClick);

   roadmap_string_release(dID);
   roadmap_string_release(dName);
   roadmap_string_release(dGroup);
   roadmap_string_release(dSprite);
   roadmap_string_release(dImage_Reminder);
}

//////////////////////////////////////////////////////////////////
void remider_remover_from_history(char *ID){
   int iID = get_id(ID);
   ReminderTable.reminder[iID].in_use = FALSE;
   roadmap_history_delete_entry(ReminderTable.reminder[iID].history);
}

void roadmap_reminder_delete(void *history){
   int i;

   for (i = 0 ; i < ReminderTable.iCount; i++){
      if (ReminderTable.reminder[i].history == history){
         char id[REMINDER_ID_MAXSIZE];
         snprintf(id, sizeof(id), "%d_reminder",ReminderTable.reminder[i].iID);
         reminder_remove_pin(id);
         ReminderTable.reminder[i].in_use = FALSE;
         return;
      }
   }

}
//////////////////////////////////////////////////////////////////
void roadmap_reminder_init(void){
   void *history;
   void *prev;
   int count = 0;

   roadmap_config_declare_enumeration ("preferences", &RoadMapConfigFeatureEnabled, NULL, "no",
                                       "yes", NULL);

   ReminderTable.iCount = 0;

   roadmap_history_declare (REMINDER_CATEGORY, reminder_hi__count);

   if (!roadmap_reminder_feature_enabled())
      return;

   history = roadmap_history_latest (REMINDER_CATEGORY);

   while (history && (count < MAX_REMINDER_ENTRIES)) {
      char *argv[reminder_hi__count];
      roadmap_history_get (REMINDER_CATEGORY, history, argv);
      prev = history;
      if (!strcmp(argv[reminder_hi_add_reminder], "1")){
         ReminderTable.reminder[count].position.latitude = atoi(argv[reminder_hi_latitude]);
         ReminderTable.reminder[count].position.longitude = atoi(argv[reminder_hi_longtitude]);
         ReminderTable.reminder[count].distance= atoi(argv[reminder_hi_distance]);
         ReminderTable.reminder[count].history = history;
         ReminderTable.reminder[count].displayed = FALSE;
         ReminderTable.reminder[count].in_use = TRUE;
         ReminderTable.reminder[count].repeat = atoi(argv[reminder_hi_repeat]);
         snprintf (ReminderTable.reminder[count].title, MAX_REMINDER_TITLE, "%s", argv[reminder_hi_title]);
         snprintf (ReminderTable.reminder[count].description, MAX_REMINDER_DESC, "%s", argv[reminder_hi_description]);

         remider_add_pin(count, &ReminderTable.reminder[count].position);

         count++;
      }

      history = roadmap_history_before (REMINDER_CATEGORY, history);
      if (history == prev) break;
   }

   ReminderTable.iCount = count;
   if(count > 0)
      register_gps_listener();
}

//////////////////////////////////////////////////////////////////
void roadmap_reminder_term(void){

}

//////////////////////////////////////////////////////////////////
static int on_close (SsdWidget widget, const char *new_value){

   int id = -1;

   if (widget->context)
      id = get_id((char *)widget->context);


   if ((id != -1) && (!ReminderTable.reminder[id].repeat)){
      reminder_remove_pin((char *)widget->context);
      remider_remover_from_history((char *)widget->context);
   }

   if (ssd_dialog_is_currently_active() && (!strcmp(ssd_dialog_currently_active_name(), REMINDER_POP_DLG_NAME)))
      ssd_dialog_hide_current(dec_close);

   return 0;
}

//////////////////////////////////////////////////////////////////
static int on_delete (SsdWidget widget, const char *new_value){
   reminder_remove_pin((char *)widget->context);
   remider_remover_from_history((char *)widget->context);
   if (ssd_dialog_is_currently_active() && (!strcmp(ssd_dialog_currently_active_name(), REMINDER_POP_DLG_NAME)))
      ssd_dialog_hide_current(dec_close);

   return 1;
}

//////////////////////////////////////////////////////////////////
static int get_id(const char *id){
   char * TempStr = strdup(id);
   char * tempC;
   tempC  = strtok((char *)TempStr,"_");
   if (tempC!=NULL)
      return(atoi(tempC));
   else
      return -1;
}

//////////////////////////////////////////////////////////////////
static void play_remider_sound(void){
   static RoadMapSoundList list;
   if (!list) {
      list = roadmap_sound_list_create (SOUND_LIST_NO_FREE);
      roadmap_sound_list_add (list, REMINDER_SOUND);
      roadmap_res_get (RES_SOUND, 0, REMINDER_SOUND);
   }
   roadmap_sound_play_list (list);
}

//////////////////////////////////////////////////////////////////
static SsdWidget create_reminder_dlg(){
   SsdWidget dlg;
   SsdWidget bg;
   SsdWidget spacer;
   dlg = ssd_dialog_new (REMINDER_POP_DLG_NAME,
                         "",
                         NULL,
                         SSD_DIALOG_FLOAT|SSD_ALIGN_CENTER);
   ssd_widget_set_color(dlg, NULL, NULL);

   bg = ssd_bitmap_new("sticky", "sticky", SSD_ALIGN_CENTER);
   ssd_widget_add(dlg, bg);

   spacer = ssd_container_new( "space", "", 10, 60, SSD_END_ROW );
   ssd_widget_set_color( spacer, NULL, NULL );
   ssd_widget_add( bg, spacer );
   return bg;
}

//////////////////////////////////////////////////////////////////
static void show_reminder_dlg(){
   ssd_dialog_activate(REMINDER_POP_DLG_NAME, NULL);
   if (!roadmap_screen_refresh()){
      roadmap_screen_redraw();
   }
}

//////////////////////////////////////////////////////////////////
static SsdWidget create_close_button(){
   char *icons[3];
   SsdWidget button;
   icons[0] = "GeoReminderButtonUp";
   icons[1] = "GeoReminderButtonDown";
   icons[2] = NULL;
   button = ssd_button_label_custom("Close_button", roadmap_lang_get("Close"), SSD_WS_DEFWIDGET|SSD_WS_TABSTOP|SSD_ALIGN_CENTER|SSD_ALIGN_BOTTOM, on_close, (const char**) &icons[0], 2, "#ab5939", "#fff88d", 14);
   ssd_widget_set_offset(button, 0, -20);
   return button;
}

//////////////////////////////////////////////////////////////////
static SsdWidget create_del_button(){
   char *icons[3];
   SsdWidget button;
   icons[0] = "DeleteReminder";
   icons[1] = "DeleteReminder_s";
   icons[2] = NULL;
   button = ssd_button_new("Delete_button", "Delete", (const char**) &icons[0], 2, SSD_ALIGN_CENTER|SSD_WS_TABSTOP|SSD_ALIGN_BOTTOM, on_delete);
   ssd_widget_set_offset(button, 0, -20);
   return button;
}

//////////////////////////////////////////////////////////////////
static void show_reminder(int id, int distance){
   SsdWidget button;
   SsdWidget spacer;
   SsdWidget txt;
   SsdWidget bg;

   if (ssd_dialog_is_currently_active() && (!strcmp(ssd_dialog_currently_active_name(), REMINDER_POP_DLG_NAME)))
      ssd_dialog_hide_current(dec_close);


   bg = create_reminder_dlg();

   if (id != -1){
      Reminder reminder;
      SsdWidget container;
      char msg[250];
      int rem_distance;

      container = ssd_container_new( "container_txt", "", SSD_MIN_SIZE, SSD_MIN_SIZE, 0 );
      ssd_widget_set_color( container, NULL, NULL );

      reminder = ReminderTable.reminder[id];
      txt = ssd_text_new("reminder_title",&reminder.title[0] ,22, SSD_END_ROW);
      ssd_text_set_color(txt, "#ab5939");
      ssd_widget_set_offset(txt, 25, 0);
      ssd_widget_add(container, txt);

      spacer = ssd_container_new( "space", "", SSD_MIN_SIZE, 10, SSD_END_ROW );
      ssd_widget_set_color( spacer, NULL, NULL );
      ssd_widget_add( container, spacer );

      txt = ssd_text_new("reminder_desc",&reminder.description[0] ,18, SSD_END_ROW);
      ssd_text_set_color(txt, "#ab5939");
      ssd_widget_set_offset(txt, 30, 0);
      ssd_widget_add(container, txt);

      spacer = ssd_container_new( "space", "", SSD_MIN_SIZE, 10, SSD_END_ROW );
      ssd_widget_set_color( spacer, NULL, NULL );
      ssd_widget_add( container, spacer );

      rem_distance = roadmap_math_to_trip_distance_tenths(reminder.distance);
      sprintf(msg, "%s: %.1f %s",roadmap_lang_get("Distance"), rem_distance/10.0, roadmap_lang_get(roadmap_math_trip_unit()) );
      txt = ssd_text_new("reminder_dist",&msg[0] ,14, SSD_END_ROW);
      ssd_text_set_color(txt, "#ab5939");
      ssd_widget_set_offset(txt, 30, 0);
      ssd_widget_add(container, txt);

      ssd_widget_add( bg, container );
   }

#ifdef TOUCH_SCREEN
   spacer = ssd_container_new( "space", "", 10, 30, SSD_END_ROW );
   ssd_widget_set_color( spacer, NULL, NULL );
   ssd_widget_add( bg, spacer );

   button = create_close_button();
   button->context = (void *)roadmap_string_get(generate_id(id));
   ssd_widget_add(bg, button);

   if (ReminderTable.reminder[id].repeat == 1){
      button = create_del_button();
      button->context = (void *)roadmap_string_get(generate_id(id));
      ssd_widget_add(bg, button);
   }

#endif //TOUCH_SCREEN

   show_reminder_dlg();

}

//////////////////////////////////////////////////////////////////
static void OnReminderShortClick (const char *name,
                                  const char *sprite,
                                  RoadMapDynamicString *images,
                                  int  image_count,
                                  const RoadMapGpsPosition *gps_position,
                                  const RoadMapGuiPoint    *offset,
                                  BOOL is_visible,
                                  int scale,
                                  int opacity,
                                  int scale_y,
                                  const char *id,
                                  ObjectText *texts,
                                  int        text_count,
                                  int rotation){

   SsdWidget button;
   SsdWidget spacer;
   SsdWidget txt;
   SsdWidget bg;
   RoadMapPosition Pos;

   Pos.latitude = gps_position->latitude;
   Pos.longitude = gps_position->longitude;
   if (ssd_dialog_is_currently_active() && (!strcmp(ssd_dialog_currently_active_name(), REMINDER_POP_DLG_NAME)))
      ssd_dialog_hide_current(dec_close);
   bg = create_reminder_dlg();

   if (get_id(id) != -1){
      char msg[250];
      int rem_distance;
      Reminder reminder;
      SsdWidget container;

      container = ssd_container_new( "container_txt", "", SSD_MIN_SIZE, SSD_MIN_SIZE, 0 );
      ssd_widget_set_color( container, NULL, NULL );

      reminder = ReminderTable.reminder[get_id(id)];
      txt = ssd_text_new("reminder_title",&reminder.title[0] ,22, SSD_END_ROW);
      ssd_text_set_color(txt, "#ab5939");
      ssd_widget_set_offset(txt, 25, 0);
      ssd_widget_add(container, txt);

      spacer = ssd_container_new( "space", "", SSD_MIN_SIZE, 10, SSD_END_ROW );
      ssd_widget_set_color( spacer, NULL, NULL );
      ssd_widget_add( container, spacer );

      txt = ssd_text_new("reminder_desc",&reminder.description[0] ,18, SSD_END_ROW);
      ssd_text_set_color(txt, "#ab5939");
      ssd_widget_set_offset(txt, 30, 0);
      ssd_widget_add(container, txt);

      spacer = ssd_container_new( "space", "", SSD_MIN_SIZE, 10, SSD_END_ROW );
      ssd_widget_set_color( spacer, NULL, NULL );
      ssd_widget_add( container, spacer );

      rem_distance = roadmap_math_to_trip_distance_tenths(reminder.distance);
      sprintf(msg, roadmap_lang_get("Distance to remind: %.1f %s"),rem_distance/10.0, roadmap_lang_get(roadmap_math_trip_unit()) );
      txt = ssd_text_new("reminder_dist",&msg[0] ,14, SSD_END_ROW);
      ssd_text_set_color(txt, "#ab5939");
      ssd_widget_set_offset(txt, 30, 0);
      ssd_widget_add(container, txt);

      ssd_widget_add( bg, container );
   }

#ifdef TOUCH_SCREEN

   button = create_close_button();
   ssd_widget_add(bg, button);

   button = create_del_button();
   ssd_widget_set_offset(button, 0, -20);

   button->context = strdup(id);
   ssd_widget_add(bg, button);
#endif //TOUCH_SCREEN

   show_reminder_dlg();
}


//////////////////////////////////////////////////////////////////
void roadmap_reminder_add_entry(const char **argv, BOOL add_reminder) {
   roadmap_history_add (REMINDER_CATEGORY, (const char **)argv);

   if (add_reminder){
      ReminderTable.reminder[ReminderTable.iCount].position.latitude = atoi(argv[reminder_hi_latitude]);
      ReminderTable.reminder[ReminderTable.iCount].position.longitude = atoi(argv[reminder_hi_longtitude]);
      ReminderTable.reminder[ReminderTable.iCount].distance= atoi(argv[reminder_hi_distance]);
      ReminderTable.reminder[ReminderTable.iCount].history = roadmap_history_latest(REMINDER_CATEGORY);
      ReminderTable.reminder[ReminderTable.iCount].displayed = FALSE;
      ReminderTable.reminder[ReminderTable.iCount].in_use = TRUE;
      ReminderTable.reminder[ReminderTable.iCount].repeat = atoi(argv[reminder_hi_repeat]);;
      snprintf (ReminderTable.reminder[ReminderTable.iCount].title, MAX_REMINDER_TITLE, "%s", argv[reminder_hi_title]);
      snprintf (ReminderTable.reminder[ReminderTable.iCount].description, MAX_REMINDER_DESC, "%s", argv[reminder_hi_description]);

      remider_add_pin(ReminderTable.iCount ,&ReminderTable.reminder[ReminderTable.iCount].position);

      if(ReminderTable.iCount == 0)
         register_gps_listener();

      ReminderTable.iCount++;
   }
}

#ifndef IPHONE_NATIVE
//////////////////////////////////////////////////////////////////
static int reminder_add_dlg_buttons_callback (SsdWidget widget, const char *new_value) {
   SsdWidget container = widget->parent;
   const char *argv[reminder_hi__count];
   char  temp[15];
   if (!strcmp(widget->name, "Save")){
      const char *title_txt = ssd_widget_get_value(ssd_widget_get(container, "TitleEntry"),"TitleEntry");
      const char *description = ssd_widget_get_value(ssd_widget_get(container, "DescriptionEntry"),"DescriptionEntry");
      const char *distance = (const char *)ssd_dialog_get_data("distance");
      const char *repeat = (const char *)ssd_dialog_get_data("repeat");
      const char *add_reminder         = ssd_dialog_get_data("add_reminder");


      argv[reminder_hi_house_number] = strdup(gContext.properties.address);
      argv[reminder_hi_street] = strdup(gContext.properties.street);
      argv[reminder_hi_city] = strdup(gContext.properties.city);
      argv[reminder_state] = ""; //state
      sprintf(temp, "%d", gContext.position.latitude);
      argv[reminder_hi_latitude] = strdup(temp);
      sprintf(temp, "%d", gContext.position.longitude);
      argv[reminder_hi_longtitude] = strdup(temp);
      argv[reminder_hi_title] = strdup(title_txt);
      if (add_reminder && !strcmp( add_reminder, "yes" )){
         argv[reminder_hi_add_reminder] = "1";
         argv[reminder_hi_distance] = strdup(distance);
         argv[reminder_hi_description] = strdup(description);
         argv[reminder_hi_repeat] = strdup(repeat);
      }
      else{
         argv[reminder_hi_add_reminder] = "0";
         argv[reminder_hi_distance] = strdup("");
         argv[reminder_hi_description] = strdup("");
         argv[reminder_hi_repeat] = strdup("");
      }
      roadmap_reminder_add_entry (argv, add_reminder && !strcmp( add_reminder, "yes" ));
      ssd_dialog_hide_all(dec_close);

      if (gContext.properties.address && *gContext.properties.address != 0)
         free((void *)gContext.properties.address);

      if (gContext.properties.city && *gContext.properties.city != 0)
         free((void *)gContext.properties.city);

      if (gContext.properties.street && *gContext.properties.street != 0)
         free((void *)gContext.properties.street);

      free((void *)argv[reminder_hi_house_number]);
      free((void *)argv[reminder_hi_street]);
      free((void *)argv[reminder_hi_city]);
      free((void *)argv[reminder_hi_latitude]);
      free((void *)argv[reminder_hi_longtitude]);
      free((void *)argv[reminder_hi_distance]);
      free((void *)argv[reminder_hi_description]);
      free((void *)argv[reminder_hi_repeat]);
   }
   else{
      ssd_dialog_hide_current(dec_close);
   }

   return 1;
}

static int Save_sk_cb(SsdWidget widget, const char *new_value, void *context){
   SsdWidget container = widget;
   const char *argv[reminder_hi__count];
   char  temp[15];

   const char *title_txt = ssd_widget_get_value(ssd_widget_get(container, "TitleEntry"),"TitleEntry");
   const char *description = ssd_widget_get_value(ssd_widget_get(container, "DescriptionEntry"),"DescriptionEntry");
   const char *distance = (const char *)ssd_dialog_get_data("distance");
   const char *repeat = (const char *)ssd_dialog_get_data("repeat");
   const char *add_reminder         = ssd_dialog_get_data("add_reminder");


   argv[reminder_hi_house_number] = strdup(gContext.properties.address);
   argv[reminder_hi_street] = strdup(gContext.properties.street);
   argv[reminder_hi_city] = strdup(gContext.properties.city);
   argv[reminder_state] = ""; //state
   sprintf(temp, "%d", gContext.position.latitude);
   argv[reminder_hi_latitude] = strdup(temp);
   sprintf(temp, "%d", gContext.position.longitude);
   argv[reminder_hi_longtitude] = strdup(temp);
   argv[reminder_hi_title] = strdup(title_txt);
   if (add_reminder && !strcmp( add_reminder, "yes" )){
      argv[reminder_hi_add_reminder] = "1";
      argv[reminder_hi_distance] = strdup(distance);
      argv[reminder_hi_description] = strdup(description);
      argv[reminder_hi_repeat] = strdup(repeat);
   }
   else{
      argv[reminder_hi_add_reminder] = "0";
      argv[reminder_hi_distance] = strdup("");
      argv[reminder_hi_description] = strdup("");
      argv[reminder_hi_repeat] = strdup("");
   }
   roadmap_reminder_add_entry (argv, add_reminder && !strcmp( add_reminder, "yes" ));
   ssd_dialog_hide_all(dec_close);
   free((void *)argv[reminder_hi_house_number]);
   free((void *)argv[reminder_hi_street]);
   free((void *)argv[reminder_hi_city]);
   free((void *)argv[reminder_hi_latitude]);
   free((void *)argv[reminder_hi_longtitude]);
   free((void *)argv[reminder_hi_distance]);
   free((void *)argv[reminder_hi_description]);
   free((void *)argv[reminder_hi_repeat]);

   return 1;
}


int on_checkbox_selected (SsdWidget widget, const char *new_value){
   SsdWidget container = widget->parent->parent;
   const char *add_reminder         = ssd_dialog_get_data("add_reminder");
   if (!container)
      return 0;

   if (!strcmp( add_reminder, "no" )){
        ssd_widget_hide(ssd_widget_get(container,"DescriptionEntry"));
        ssd_widget_hide(ssd_widget_get(container,"Repeat"));
        ssd_widget_hide(ssd_widget_get(container,"Distance"));
   }
   else{
        ssd_widget_show(ssd_widget_get(container,"DescriptionEntry"));
        ssd_widget_show(ssd_widget_get(container,"Repeat"));
        ssd_widget_show(ssd_widget_get(container,"Distance"));
   }
   ssd_dialog_draw();
   return 1;
}

//////////////////////////////////////////////////////////////////
static void reminder_add_dlg(PluginStreetProperties *properties, RoadMapPosition *position, BOOL isReminder, BOOL default_reminder){
   SsdWidget dialog, dialog_cont;
   SsdWidget group;
   SsdWidget text_box;
   SsdWidget spacer;
   SsdWidget text;
   SsdWidget container, box2;
   static const char *distance_labels[6];
   static const char *distance_values[6];
   const char * dlg_name;

   static const char *repeat_labels[2] ;
   static const char *repeat_values[2] = {"0", "1"};


   if (properties){
      gContext.properties = *properties;
      gContext.properties.address = strdup(properties->address);
      gContext.properties.street = strdup(properties->street);
      gContext.properties.city = strdup(properties->city);
   }else{
      gContext.properties.address = "";
      gContext.properties.street = "";
      gContext.properties.city = "";
   }
   gContext.position = *position;


   repeat_labels[0] = roadmap_lang_get("Once");
   repeat_labels[1] = roadmap_lang_get("Every time");

   if (roadmap_math_is_metric()){
      distance_values[0] = "100";
      distance_values[1] = "500";
      distance_values[2] = "1000";
      distance_values[3] = "5000";
      distance_values[4] = "10000";
      distance_values[5] = "20000";
      distance_labels[0] = "100 m";
      distance_labels[1] = "500 m";
      distance_labels[2] = "1 km";
      distance_labels[3] = "5 km";
      distance_labels[4] = "10 km";
      distance_labels[5] = "20 km";
   }
   else{
      distance_values[0] = "30";
      distance_values[1] = "152";
      distance_values[2] = "1609";
      distance_values[3] = "8046";
      distance_values[4] = "16090";
      distance_values[5] = "32186";
      distance_labels[0] = "100 ft";
      distance_labels[1] = "500 ft";
      distance_labels[2] = "1 mi";
      distance_labels[3] = "5 mi";
      distance_labels[4] = "10 mi";
      distance_labels[5] = "20 mi";
   }


   if (default_reminder)
      dlg_name = roadmap_lang_get(REMINDER_DLG_TITLE);
   else
      dlg_name = roadmap_lang_get("Save location");
   dialog = ssd_dialog_new (REMINDER_DLG_NAME,
                            dlg_name,
                            NULL,
                            SSD_CONTAINER_TITLE);

   dialog_cont = ssd_container_new ("Reminder_DLg_Cont", "", SSD_MAX_SIZE, SSD_MAX_SIZE, SSD_END_ROW);
   ssd_widget_set_color(dialog_cont, NULL, NULL);

   text = ssd_text_new("AddressTitle", roadmap_lang_get("Address"), 18, SSD_END_ROW);
   ssd_widget_add(dialog_cont, text);
   group = ssd_container_new ("Report", NULL,
                              SSD_MAX_SIZE,SSD_MIN_SIZE,SSD_WIDGET_SPACE|SSD_END_ROW|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE|SSD_POINTER_NONE|SSD_CONTAINER_BORDER);

   if (properties){
      if (properties->street){
         text = ssd_text_new( "Street Text", properties->street,-1,SSD_END_ROW);
         ssd_widget_add(group, text);
      }
      if (properties->street){
         text = ssd_text_new( "city Text", properties->city,-1,SSD_END_ROW);
         ssd_widget_add(group, text);
      }
   }
   ssd_widget_add(dialog_cont, group);

   ssd_dialog_add_hspace(dialog_cont, 5, SSD_END_ROW);

   text = ssd_text_new("Name", roadmap_lang_get("Name"), 18, SSD_END_ROW);
   ssd_widget_add(dialog_cont, text);
   group = ssd_container_new ("Report", NULL,
                              SSD_MAX_SIZE,SSD_MIN_SIZE,SSD_WIDGET_SPACE|SSD_END_ROW|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE|SSD_POINTER_NONE|SSD_CONTAINER_BORDER);


   text_box = ssd_entry_new( "TitleEntry","",SSD_WIDGET_SPACE|SSD_END_ROW|SSD_WS_TABSTOP,0, SSD_MAX_SIZE, SSD_MIN_SIZE,roadmap_lang_get("Name"));
   ssd_widget_add(group, text_box);
   ssd_widget_add(dialog_cont, group);



   ssd_dialog_add_hspace(dialog_cont, 5, SSD_END_ROW);

   if (roadmap_reminder_feature_enabled()){
      text = ssd_text_new("Geo-ReminderTitle", roadmap_lang_get("Reminder"), 18, SSD_END_ROW);
      ssd_widget_add(dialog_cont, text);
      group = ssd_container_new ("Report", NULL,
                                 SSD_MAX_SIZE,SSD_MIN_SIZE,SSD_WIDGET_SPACE|SSD_END_ROW|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE|SSD_POINTER_NONE|SSD_CONTAINER_BORDER);

      container = ssd_container_new ("CheckboxContainer", "", SSD_MAX_SIZE, SSD_MIN_SIZE, SSD_END_ROW);
      ssd_widget_set_color(container, NULL, NULL);
      ssd_widget_add ( container, ssd_text_new ("Label", roadmap_lang_get ( "Add Geo-Reminder"), 14, SSD_TEXT_LABEL|SSD_WIDGET_SPACE|SSD_ALIGN_VCENTER ) );
      ssd_widget_add (container,
                     ssd_checkbox_new ("add_reminder", default_reminder,  SSD_ALIGN_RIGHT, on_checkbox_selected,NULL,NULL,CHECKBOX_STYLE_ON_OFF));
      ssd_widget_add(group, container);

      container = ssd_container_new ("Distance", "", SSD_MIN_SIZE, SSD_MIN_SIZE, 0);
      ssd_widget_set_color(container, NULL, NULL);
      box2 = ssd_container_new ("box2", NULL, roadmap_canvas_width()/2, SSD_MIN_SIZE,
                                 SSD_ALIGN_VCENTER);
      ssd_widget_set_color (box2, NULL, NULL);
      text = ssd_text_new( "Distance Text", roadmap_lang_get("Distance to alert"),-1, SSD_ALIGN_VCENTER);
      ssd_widget_add(box2, text);
      ssd_widget_add(container, box2);
      ssd_dialog_add_hspace(container, 5, 0);

      ssd_widget_add (container,
                     ssd_choice_new ("distance", roadmap_lang_get("Distance to alert"), 5,
                                   (const char **)distance_labels,
                                   (const void **)distance_values,
                                   SSD_ALIGN_VCENTER|SSD_WS_TABSTOP|SSD_END_ROW, NULL));
      spacer = ssd_container_new( "space", "", SSD_MIN_SIZE, 1, SSD_END_ROW );
      ssd_widget_set_color( spacer, NULL, NULL );
      ssd_widget_add( group, spacer );
      ssd_widget_add(group, container);

      container = ssd_container_new ("Repeat", "", SSD_MIN_SIZE, SSD_MIN_SIZE, 0);
      ssd_widget_set_color(container, NULL, NULL);
      box2 = ssd_container_new ("box2", NULL, roadmap_canvas_width()/2, SSD_MIN_SIZE,
                                SSD_ALIGN_VCENTER);
      ssd_widget_set_color (box2, NULL, NULL);
      text = ssd_text_new( "Repeat Text", roadmap_lang_get("Repeat reminder"),-1, SSD_ALIGN_VCENTER);
      ssd_widget_add(box2, text);
      ssd_widget_add(container, box2);
      ssd_dialog_add_hspace(container, 5, 0);


      ssd_widget_add (container,
                      ssd_choice_new ("repeat", roadmap_lang_get("Repeat reminder"), 2,
                                      (const char **)repeat_labels,
                                      (const void **)repeat_values,
                                      SSD_ALIGN_VCENTER|SSD_WS_TABSTOP, NULL));
      ssd_widget_add(group, container);
      text_box = ssd_entry_new( "DescriptionEntry","",SSD_WIDGET_SPACE|SSD_END_ROW|SSD_WS_TABSTOP,0, SSD_MAX_SIZE, SSD_MIN_SIZE,roadmap_lang_get("Description"));
      ssd_widget_add(group, text_box);
      ssd_widget_add(dialog_cont, group);

      if (!default_reminder){
         ssd_widget_hide(ssd_widget_get(group,"DescriptionEntry"));
         ssd_widget_hide(ssd_widget_get(group,"Repeat"));
         ssd_widget_hide(ssd_widget_get(group,"Distance"));
      }

   }


   spacer = ssd_container_new( "space", "", SSD_MIN_SIZE, 5, SSD_END_ROW );
   ssd_widget_set_color( spacer, NULL, NULL );
   ssd_widget_add( dialog_cont, spacer );

#ifdef TOUCH_SCREEN
   ssd_widget_add (dialog_cont,
                   ssd_button_label ("Save", roadmap_lang_get ("Save"),
                                     SSD_WS_TABSTOP|SSD_ALIGN_CENTER|SSD_ALIGN_BOTTOM, reminder_add_dlg_buttons_callback));

   ssd_widget_add (dialog_cont,
                   ssd_button_label ("Cancel", roadmap_lang_get ("Cancel"),
                                     SSD_WS_TABSTOP|SSD_ALIGN_CENTER|SSD_ALIGN_BOTTOM, reminder_add_dlg_buttons_callback));
#else
   ssd_widget_set_right_softkey_callback(dialog, Save_sk_cb);
   ssd_widget_set_right_softkey_text(dialog, roadmap_lang_get("Save"));
#endif
   ssd_widget_add(dialog, dialog_cont);

   ssd_dialog_activate(REMINDER_DLG_NAME, NULL);

}
#endif //IPHONE


//////////////////////////////////////////////////////////////////
void roadmap_reminder_add_at_position(RoadMapPosition *position,  BOOL isReminder, BOOL default_reminder){
   int number;
   int layers[128];
   int layers_count;
   PluginStreetProperties properties;
   RoadMapNeighbour neighbours[2];
   RoadMapPosition context_save_pos;
   zoom_t context_save_zoom;

   if (!position)
      return;

   roadmap_math_get_context(&context_save_pos, &context_save_zoom);
   layers_count = roadmap_layer_all_roads(layers, 128);
   roadmap_math_set_context(position, 20);
   number = roadmap_street_get_closest(position, 0, layers, layers_count,
                                       1, &neighbours[0], 1);
   roadmap_math_set_context(&context_save_pos, context_save_zoom);
   if (number > 0){
      roadmap_plugin_get_street_properties (&neighbours[0].line, &properties, 0);
      reminder_add_dlg(&properties, position, isReminder, default_reminder);
   }
   else{
      reminder_add_dlg(NULL, position, isReminder,default_reminder);
   }
}

//////////////////////////////////////////////////////////////////
void roadmap_reminder_add(void){
   RoadMapPosition *position =  (RoadMapPosition *)roadmap_trip_get_position("Selection");
   if (!position)
      return;
   roadmap_reminder_add_at_position(position, TRUE, FALSE);
}

//////////////////////////////////////////////////////////////////
void roadmap_reminder_save_location(void){
   RoadMapPosition *position =  (RoadMapPosition *)roadmap_trip_get_position("Selection");
    if (!position)
       return;
    roadmap_reminder_add_at_position(position, FALSE, FALSE);
}
//////////////////////////////////////////////////////////////////
static void roadmap_gps_update
(time_t gps_time,
 const RoadMapGpsPrecision *dilution,
 const RoadMapGpsPosition *gps_position){
   int i, distance;
   RoadMapPosition gps_pos;

   gps_pos.latitude = gps_position->latitude;
   gps_pos.longitude = gps_position->longitude;


   for (i=0; i<ReminderTable.iCount; i++) {
      if (!ReminderTable.reminder[i].in_use){
         continue;
      }

      if (ReminderTable.reminder[i].displayed){
         continue;
      }

      // check that the reminder distanc
      distance = roadmap_math_distance(&ReminderTable.reminder[i].position, &gps_pos);

      if (distance <= ReminderTable.reminder[i].distance){
         play_remider_sound();
         show_reminder(i, distance);
         ReminderTable.reminder[i].displayed = TRUE;
      }
   }

}

//////////////////////////////////////////////////////////////////
static void register_gps_listener(void){
   roadmap_gps_register_listener (&roadmap_gps_update);
}



