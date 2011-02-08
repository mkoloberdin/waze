/*  roadmap_general_settings.c
 *
 * LICENSE:
 *
 *   Copyright 2008 Ehud Shabtai
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
 */


#include <assert.h>
#include <string.h>
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_button.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_confirm_dialog.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_choice.h"
#include "roadmap_keyboard.h"
#include "roadmap_config.h"
#include "roadmap_lang.h"
#include "Realtime/Realtime.h"
#include "Realtime/RealtimeDefs.h"
#include "ssd/ssd_checkbox.h"
#include "roadmap_login.h"
#include "ssd/ssd_bitmap.h"
#include "ssd/ssd_separator.h"
#include "roadmap_device.h"
#include "roadmap_math.h"
#include "roadmap_sound.h"
#include "roadmap_skin.h"
#include "roadmap_car.h"
#include "roadmap_path.h"
#include "roadmap_skin.h"
#include "roadmap_mood.h"
#include "roadmap_main.h"
#include "editor/editor_screen.h"
#include "navigate/navigate_main.h"
#include "roadmap_prompts.h"
#include "roadmap_messagebox.h"
#include "roadmap_alternative_routes.h"
#include "roadmap_analytics.h"

#define CLOCK_SETTINGS_12_HOUR "12 hr."
#define CLOCK_SETTINGS_24_HOUR "24 hr."

static const char*   title = "General settings";
static int DialogShowsShown = 0;
static const char *yesno_label[2];
static const char *yesno[2];
static RoadMapConfigDescriptor RoadMapConfigConnectionAuto =
                        ROADMAP_CONFIG_ITEM("Connection", "AutoConnect");
static RoadMapConfigDescriptor RoadMapConfigBackLight =
                        ROADMAP_CONFIG_ITEM("Display", "BackLight");
static RoadMapConfigDescriptor RoadMapConfigVolControl =
                        ROADMAP_CONFIG_ITEM( "Voice", "Volume Control" );
static RoadMapConfigDescriptor RoadMapConfigGeneralUnit =
                            ROADMAP_CONFIG_ITEM("General", "Unit");

static RoadMapConfigDescriptor RoadMapConfigGeneralUserUnit =
                        ROADMAP_CONFIG_ITEM("General", "Unit");

static RoadMapConfigDescriptor RoadMapConfigShowTicker =
                        ROADMAP_CONFIG_ITEM("User", "Show points ticker");

RoadMapConfigDescriptor RoadMapConfigUseNativeKeyboard =
                        ROADMAP_CONFIG_ITEM("Keyboard", "Use native");

static RoadMapConfigDescriptor RoadMapConfigEventsRadius =
                        ROADMAP_CONFIG_ITEM("Events", "Radius");

static RoadMapConfigDescriptor RoadMapConfigClockFormat =
						ROADMAP_CONFIG_ITEM("Clock","Format");

extern RoadMapConfigDescriptor NavigateConfigAutoZoom;
extern RoadMapConfigDescriptor NavigateConfigNavigationGuidance;

static const char *autozoom_label[3];
static const char *autozoom_value[3];


void Delayed_messagebox(void){
   roadmap_messagebox("","Please restart waze");
   roadmap_main_remove_periodic (Delayed_messagebox);
}

static void lang_changed_delayed_message(void){
   roadmap_main_remove_periodic(lang_changed_delayed_message);
   roadmap_messagebox_timeout("","Language changed, Please restart waze",5);
}

static void update_events_radius(){
   const char * data = (const char *)ssd_dialog_get_data("event_radius");
   if (!(roadmap_config_match(&RoadMapConfigEventsRadius, data))){ // descriptor changed
       roadmap_config_set (&RoadMapConfigEventsRadius,data);
       OnSettingsChanged_VisabilityGroup(); // notify server of visibilaty settings change
   }
}
static int on_ok( SsdWidget this, const char *new_value) {

   const char *current_lang = roadmap_lang_get_system_lang();
   const char *new_lang = ssd_dialog_get_data("lang");
   const char *prompts = ssd_dialog_get_data("Prompts");

   if (prompts)
      roadmap_prompts_set_name(prompts);

#ifdef __SYMBIAN32__
   roadmap_config_set(&RoadMapConfigConnectionAuto, ( const char* ) ssd_dialog_get_data("AutoConnect"));
#endif

#if (defined(__SYMBIAN32__) || defined(ANDROID))
   roadmap_device_set_backlight( !( strcasecmp( ( const char* ) ssd_dialog_get_data("BackLight"), yesno[0] ) ) );

   roadmap_sound_set_volume( ( int ) ssd_dialog_get_data( "Volume Control" ) );
#endif // Symbian or android

   roadmap_config_set (&NavigateConfigAutoZoom,
                           (const char *)ssd_dialog_get_data ("autozoom"));
#ifndef TOUCH_SCREEN
   roadmap_config_set (&NavigateConfigNavigationGuidance,
                           (const char *)ssd_dialog_get_data ("navigationguidance"));

   roadmap_config_set (&RoadMapConfigShowTicker,
                              (const char *)ssd_dialog_get_data ("show_ticker"));


#endif
   roadmap_config_set ( &RoadMapConfigShowTicker,
                              (const char *)ssd_dialog_get_data ("show_ticker") );

#if (defined(_WIN32) || defined(ANDROID))
   roadmap_config_set ( &RoadMapConfigUseNativeKeyboard,
                              (const char *) ssd_dialog_get_data ( "Native keyboard" ) );
#endif


   if(!strcasecmp( ( const char* ) ssd_dialog_get_data("use_metric"), yesno[0] )){
   	  roadmap_config_set (&RoadMapConfigGeneralUserUnit,"metric");
   	  roadmap_math_use_metric();
   }
   else{
	  roadmap_config_set (&RoadMapConfigGeneralUserUnit,"imperial");
	  roadmap_math_use_imperial();
   }

   if(!strcasecmp( ( const char* ) ssd_dialog_get_data("ClockFormatChk"), yesno[0] ))
   	  roadmap_config_set (&RoadMapConfigClockFormat,CLOCK_SETTINGS_12_HOUR);
   else
	 roadmap_config_set (&RoadMapConfigClockFormat,CLOCK_SETTINGS_24_HOUR);
   if (roadmap_alternative_feature_enabled()){
      if(!strcasecmp( ( const char* ) ssd_dialog_get_data("AutoPromptRouteSuggestions"), yesno[0] ))
         roadmap_alternative_routes_set_suggest_routes(TRUE);
      else
         roadmap_alternative_routes_set_suggest_routes(FALSE);
   }

   update_events_radius();

   roadmap_config_save(TRUE);
   DialogShowsShown = 0;
   if (new_lang && strcmp(current_lang,new_lang)){
      // Language changed
      roadmap_lang_download_lang_file(new_lang, NULL);
      roadmap_lang_set_system_lang( new_lang);
      ssd_dialog_hide_all(dec_close);
      roadmap_screen_redraw();
      roadmap_main_set_periodic(500,lang_changed_delayed_message);
   }
   return 0;
}

#ifndef TOUCH_SCREEN
static int on_ok_softkey(SsdWidget this, const char *new_value, void *context){
	on_ok(this, new_value);
	ssd_dialog_hide_all(dec_ok);
	return 0;
}
#endif

static void on_close_dialog (int exit_code, void* context){
#ifdef TOUCH_SCREEN
	if (exit_code == dec_ok)
		on_ok(NULL, NULL);
#endif
}

/////////////////////////////////////////////////////////////////////
static SsdWidget space(int height){
	SsdWidget space;
	space = ssd_container_new ("spacer", NULL, SSD_MAX_SIZE, height, SSD_WIDGET_SPACE|SSD_END_ROW);
	ssd_widget_set_color (space, NULL,NULL);
	return space;
}

void quick_settins_exit(int exit_code, void* context){
   const char* view_val;

   if (exit_code != dec_ok)
		return;

   yesno[0] = "Yes";
   yesno[1] = "No";

   if(!strcasecmp( ( const char* ) ssd_dialog_get_data("light"), yesno[0] )){
   	roadmap_skin_set_subskin ("day");
   } else {
      roadmap_skin_set_subskin ("night");

   }

   view_val = ( const char* ) ssd_dialog_get_data("view");
   if( !view_val || !strcasecmp( view_val, yesno[0] )){
      if (roadmap_screen_get_view_mode() == VIEW_MODE_3D){
         roadmap_analytics_log_event(ANALYTICS_EVENT_VIEWMODESET, ANALYTICS_EVENT_INFO_NEW_MODE, ANALYTICS_EVENT_2D);
         roadmap_screen_set_view (VIEW_MODE_2D);
      }

   } else {
      if (roadmap_screen_get_view_mode() == VIEW_MODE_2D){
         roadmap_analytics_log_event(ANALYTICS_EVENT_VIEWMODESET, ANALYTICS_EVENT_INFO_NEW_MODE, ANALYTICS_EVENT_3D);
         roadmap_screen_set_view (VIEW_MODE_3D);
      }
   }

   if (!strcmp((const char *)ssd_dialog_get_data ("navigationguidance"), "yes")){
      if (!roadmap_config_match(&NavigateConfigNavigationGuidance, "yes"))
         roadmap_analytics_log_event(ANALYTICS_EVENT_MUTE, ANALYTICS_EVENT_INFO_CHANGED_TO, ANALYTICS_EVENT_ON);
   }
   else{
      if (roadmap_config_match(&NavigateConfigNavigationGuidance, "yes"))
         roadmap_analytics_log_event(ANALYTICS_EVENT_MUTE, ANALYTICS_EVENT_INFO_CHANGED_TO, ANALYTICS_EVENT_OFF);
   }

   roadmap_config_set (&NavigateConfigNavigationGuidance,
                        (const char *)ssd_dialog_get_data ("navigationguidance"));

   roadmap_config_save(TRUE);
}

int callback (SsdWidget widget, const char *new_value){
   yesno[0] = "Yes";
   yesno[1] = "No";
   if(!strcasecmp( ( const char* ) ssd_dialog_get_data("light"), yesno[0] )){
      roadmap_analytics_log_event(ANALYTICS_EVENT_DAYNIGHTSET, ANALYTICS_EVENT_INFO_NEW_MODE, ANALYTICS_EVENT_DAY);
        roadmap_skin_set_subskin ("day");
   } else {
      roadmap_analytics_log_event(ANALYTICS_EVENT_DAYNIGHTSET, ANALYTICS_EVENT_INFO_NEW_MODE, ANALYTICS_EVENT_NIGHT);
      roadmap_skin_set_subskin ("night");
   }
   ssd_dialog_draw();
   return 1;
}

void on_download_lang_confirm(int exit_code, void *context){
   if (exit_code == dec_yes){
      const char *lang = (const char *)context;
      roadmap_prompts_download(lang);
   }
}

int on_prompts_selected (SsdWidget widget, const char *new_value){
   if (!roadmap_prompts_exist(roadmap_prompts_get_prompt_value_from_name(new_value))){
      char msg[256];
      snprintf(msg, sizeof(msg),"%s %s, %s", roadmap_lang_get("Prompt set"), new_value, roadmap_lang_get("is not installed on your device, Do you want to download prompt files?") );
      ssd_confirm_dialog("", msg, FALSE, on_download_lang_confirm, (void *)roadmap_prompts_get_prompt_value_from_name(new_value));
   }
   return 0;
}


SsdWidget create_quick_setting_menu(){
#ifdef TOUCH_SCREEN
   int tab_flag = SSD_WS_TABSTOP;
#else
   int tab_flag = SSD_WS_TABSTOP;
#endif
   BOOL checked = FALSE;
   SsdWidget box;
   SsdWidget quick_container;
   int height = ssd_container_get_row_height();
   int width = ssd_container_get_width();

   //Quick Setting Container
   quick_container = ssd_container_new ("__quick_settings", NULL, width, SSD_MIN_SIZE,
                                        SSD_ALIGN_CENTER|SSD_WIDGET_SPACE|SSD_END_ROW|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE|SSD_POINTER_NONE|SSD_CONTAINER_BORDER);

   // add title
   // ssd_widget_add (container, ssd_text_new ("Quick_actions_title_text_cont",
   // roadmap_lang_get ("Quick actions"), 16, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE|tab_flag));

   //Mute
   box = ssd_container_new ("Mute group", NULL, SSD_MAX_SIZE, height,
                            SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag);
   ssd_widget_set_color (box, NULL, NULL);

   if ( navgiate_main_voice_guidance_enabled() )
   {
      if  ( roadmap_config_match(&NavigateConfigNavigationGuidance, "yes"))
      {
         checked = TRUE;
      }

      box = ssd_checkbox_row_new ("navigationguidance", roadmap_lang_get ("Navigation guidance"),
                                  checked, NULL,NULL,NULL,CHECKBOX_STYLE_ON_OFF);

      ssd_widget_add (quick_container, box);
      ssd_widget_add (quick_container, ssd_separator_new("separator", SSD_END_ROW));
   }

   //View 2D/3D
#ifndef OPENGL
   if ( !roadmap_screen_is_hd_screen() )
   {
#endif
      if (roadmap_screen_get_view_mode() == VIEW_MODE_2D)
         checked = TRUE;
      else
         checked = FALSE;

      box = ssd_checkbox_row_new ("view", roadmap_lang_get ("Display"),
                                  checked, NULL, "button_2d", "button_3d", CHECKBOX_STYLE_ON_OFF);

      ssd_widget_add (quick_container, box);
      ssd_widget_add (quick_container, ssd_separator_new("separator", SSD_END_ROW));
#ifndef OPENGL
   }
#endif

   //Light day/night
   checked = (roadmap_skin_state() == 0);

   box = ssd_checkbox_row_new ("light", roadmap_lang_get ("Mode"),
                               checked, callback ,"button_day", "button_night",CHECKBOX_STYLE_ON_OFF);

   ssd_widget_add (quick_container, box);

   return quick_container;

}


void roadmap_general_settings_show(void) {

   static int initialized = 0;
   int height = ssd_container_get_row_height();
   int i;
   char temp[6][100];
   static const char *distance_labels[6];
   static const char *distance_values[6];

   const char *pVal;
#ifdef TOUCH_SCREEN
   int tab_flag = SSD_WS_TABSTOP;
#else
   int tab_flag = SSD_WS_TABSTOP;
#endif

   int width = ssd_container_get_width();

   if (!initialized) {
      initialized = 1;

      // Define the labels and values
	yesno_label[0] = roadmap_lang_get ("Yes");
	 yesno_label[1] = roadmap_lang_get ("No");
	 yesno[0] = "Yes";
	 yesno[1] = "No";
    autozoom_value[0] = "speed";
	 autozoom_value[1] = "yes";
	 autozoom_value[2] = "no";
    autozoom_label[0] = roadmap_lang_get ("According to speed");
	 autozoom_label[1] = roadmap_lang_get ("According to distance");
	 autozoom_label[2] = roadmap_lang_get ("No");

   }

   if (!ssd_dialog_activate (title, NULL)) {

      SsdWidget dialog;
      SsdWidget box, box2;
      SsdWidget container;

      const void ** lang_values = roadmap_lang_get_available_langs_values();
      const char ** lang_labels = roadmap_lang_get_available_langs_labels();
      int lang_count = roadmap_lang_get_available_langs_count();


      const void ** prompts_values = roadmap_prompts_get_values();
      const char ** prompts_labels = roadmap_prompts_get_labels();
      int prompts_count = roadmap_prompts_get_count();

      dialog = ssd_dialog_new (title, roadmap_lang_get(title), on_close_dialog,
                               SSD_CONTAINER_TITLE);

#ifdef TOUCH_SCREEN
      ssd_widget_add(dialog, space(5));
#endif

      container = ssd_container_new ("Conatiner Group", NULL, width, SSD_MIN_SIZE,
                                     SSD_WIDGET_SPACE|SSD_END_ROW|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE|SSD_POINTER_NONE|SSD_CONTAINER_BORDER|SSD_ALIGN_CENTER);

      //////////// Language /////////////
      if (lang_count > 1){
         box = ssd_container_new ("lang group", NULL, SSD_MAX_SIZE, height,
                                  SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag);
         ssd_widget_set_color (box, NULL, NULL);

         box2 = ssd_container_new ("box2", NULL, width/2, height,
                                   SSD_ALIGN_VCENTER);
         ssd_widget_set_color (box2, NULL, NULL);

         ssd_widget_add (box2,
                         ssd_text_new ("lang_label",
                                       roadmap_lang_get ("Language"),
                                       SSD_MAIN_TEXT_SIZE, SSD_TEXT_NORMAL_FONT|SSD_TEXT_LABEL|SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE));
         ssd_widget_add(box, box2);

         ssd_widget_add (box,
                         ssd_choice_new ("lang", roadmap_lang_get ("Language"),lang_count,
                                         (const char **)lang_labels,
                                         (const void **)lang_values,
                                         SSD_ALIGN_RIGHT, NULL));
         ssd_widget_add(box, space(1));
         ssd_widget_add(box, ssd_separator_new("separator", SSD_ALIGN_BOTTOM));
         ssd_widget_add (container, box);
      }
      //////////// Prompts /////////////
      if (prompts_count > 0){
         box = ssd_container_new ("prompts group", NULL, SSD_MAX_SIZE, height,
                                  SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag);
         ssd_widget_set_color (box, NULL, NULL);

         box2 = ssd_container_new ("box2", NULL, width/2, height,
                                   SSD_ALIGN_VCENTER);
         ssd_widget_set_color (box2, NULL, NULL);

         ssd_widget_add (box2,
                         ssd_text_new ("prompts_label",
                                       roadmap_lang_get ("Prompts"),
                                       SSD_MAIN_TEXT_SIZE, SSD_TEXT_NORMAL_FONT|SSD_TEXT_LABEL|SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE));
         ssd_widget_add(box, box2);

         ssd_widget_add (box,
                         ssd_choice_new ("Prompts", roadmap_lang_get ("Prompts"), prompts_count,
                                         (const char **)prompts_labels,
                                         (const void **)prompts_values,
                                         SSD_ALIGN_RIGHT, on_prompts_selected));
         ssd_widget_add (container, box);
      }
      ssd_widget_add(dialog, container);

      container = ssd_container_new ("Conatiner Group", NULL, width, SSD_MIN_SIZE,
                                     SSD_WIDGET_SPACE|SSD_END_ROW|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE|SSD_POINTER_NONE|SSD_CONTAINER_BORDER|SSD_ALIGN_CENTER);

      //General Units
      box = ssd_checkbox_row_new ("use_metric", roadmap_lang_get ("Measurement system"),
                                  TRUE, NULL, "button_meters", "button_miles", CHECKBOX_STYLE_ON_OFF);

      ssd_widget_add (container, box);

      ssd_widget_add(dialog, container);

      container = ssd_container_new ("Conatiner Group", NULL, width, SSD_MIN_SIZE,
                                     SSD_WIDGET_SPACE|SSD_END_ROW|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE|SSD_POINTER_NONE|SSD_CONTAINER_BORDER|SSD_ALIGN_CENTER);

#ifdef __SYMBIAN32__


      //////////// Automatic connection selection box /////////////
      // TODO :: Move to another settings directory
      box = ssd_checkbox_row_new ("AutoConnect", roadmap_lang_get ("Auto Connection"),
                                  TRUE, NULL, NULL, NULL, CHECKBOX_STYLE_ON_OFF);

      ssd_widget_add (container, box);
      ssd_widget_add (container, ssd_separator_new("separator", SSD_END_ROW));
#endif

#if (defined(__SYMBIAN32__) || defined(ANDROID) )
      //////////////////////////////////////////////////////////

      ////////////  Backlight control  /////////////
      // TODO :: Move to another settings directory
      box = ssd_checkbox_row_new ("BackLight", roadmap_lang_get ("Back Light On"),
                                  TRUE, NULL, NULL, NULL, CHECKBOX_STYLE_ON_OFF);

      ssd_widget_add (container, box);
      ssd_widget_add (container, ssd_separator_new("separator", SSD_END_ROW));

      //////////////////////////////////////////////////////////
#endif // defined(__SYMBIAN32__) || defined(ANDROID)

#ifdef __SYMBIAN32__
      ////////////  Volume control  /////////////
      // TODO :: Move to another settings directory
      box = ssd_container_new ("Volume Control Group", NULL, SSD_MAX_SIZE, height,
                               SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag);

      ssd_widget_set_color (box, "#000000", NULL);
      ssd_widget_set_color (box, "#000000", "#ffffff");

      ssd_widget_add (box,
                      ssd_text_new ( "VolumeCtrlLabel",
                                    roadmap_lang_get ("Volume Control"),
                                    SSD_MAIN_TEXT_SIZE, SSD_TEXT_NORMAL_FONT|SSD_TEXT_LABEL|SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE ) );

      ssd_widget_add (box,
                      ssd_choice_new ( "Volume Control", roadmap_lang_get ("Volume Control"), SND_VOLUME_LVLS_COUNT,
                                      SND_VOLUME_LVLS_LABELS,
                                      ( const void** ) SND_VOLUME_LVLS,
                                      SSD_ALIGN_RIGHT|SSD_ALIGN_VCENTER, NULL) );
      ssd_widget_add(box, space(1));
      ssd_widget_add(box, ssd_separator_new("separator", SSD_ALIGN_BOTTOM));
      ssd_widget_add (container, box);

      /////////////////////////////////////////////////////////
#endif // Symbian only

   box = ssd_container_new ("autozoom group", NULL, SSD_MAX_SIZE, height,
                            SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag);
   ssd_widget_set_color (box, "#000000", "#ffffff");

   box2 = ssd_container_new ("box2", NULL, roadmap_canvas_width()/3, SSD_MIN_SIZE,
                              SSD_ALIGN_VCENTER);
    ssd_widget_set_color (box2, NULL, NULL);

    ssd_widget_add (box2,
       ssd_text_new ("autozoom_label",
                      roadmap_lang_get ("Auto zoom"),
                     -1, SSD_TEXT_NORMAL_FONT|SSD_TEXT_LABEL|SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE));
    ssd_widget_add(box, box2);

    ssd_widget_add (box,
          ssd_choice_new ("autozoom", roadmap_lang_get ("Auto zoom"),3,
                          (const char **)autozoom_label,
                          (const void **)autozoom_value,
                          SSD_ALIGN_RIGHT, NULL));


   ssd_widget_add(box, space(1));
   ssd_widget_add(box, ssd_separator_new("separator", SSD_ALIGN_BOTTOM));
   ssd_widget_add (container, box);

#ifndef TOUCH_SCREEN
      //Navigation guidance
      box = ssd_checkbox_row_new ("navigationguidance", roadmap_lang_get ("Navigation guidance"),
                                  TRUE, NULL, NULL, NULL, CHECKBOX_STYLE_ON_OFF);

      ssd_widget_add (container, box);
      ssd_widget_add (container, ssd_separator_new("separator", SSD_END_ROW));
#endif

      //Show Ticker
      box = ssd_checkbox_row_new ("show_ticker", roadmap_lang_get ("Show points ticker"),
                                  TRUE, NULL, NULL, NULL, CHECKBOX_STYLE_ON_OFF);

      ssd_widget_add (container, box);
      ssd_widget_add (container, ssd_separator_new("separator", SSD_END_ROW));

      // Native keyboard - for android only at this time
#if defined(_WIN32)
      box = ssd_checkbox_row_new ("Native keyboard", roadmap_lang_get ("Use native keyboard"),
                                  TRUE, NULL, NULL, NULL, CHECKBOX_STYLE_ON_OFF);

      ssd_widget_add (container, box);
      ssd_widget_add (container, ssd_separator_new("separator", SSD_END_ROW));
#endif

      //Clock format
      box = ssd_checkbox_row_new ("ClockFormatChk", roadmap_lang_get ("24 hour clock"),
                                  TRUE, NULL, "checkbox_off", "checkbox_on", CHECKBOX_STYLE_ON_OFF);

      ssd_widget_add (container, box);

      //Auto learn
      if (roadmap_alternative_feature_enabled()){
         ssd_widget_add (container, ssd_separator_new("separator", SSD_END_ROW));
         box = ssd_checkbox_row_new ("AutoPromptRouteSuggestions", roadmap_lang_get ("Auto-learn routes to your frequent destination"),
                                     TRUE, NULL, NULL, NULL, CHECKBOX_STYLE_ON_OFF);

         ssd_widget_add (container, box);
      }
      ssd_widget_add(dialog, container);

      //Events Radius
      container = ssd_container_new ("Events Conatiner Group", NULL, width, SSD_MIN_SIZE,
                                     SSD_WIDGET_SPACE|SSD_END_ROW|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE|SSD_POINTER_NONE|SSD_CONTAINER_BORDER|SSD_ALIGN_CENTER);

      box = ssd_container_new ("lang group", NULL, SSD_MAX_SIZE, height,
                               SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag);
      ssd_widget_set_color (box, NULL, NULL);

      box2 = ssd_container_new ("box2", NULL, width/2, height,
                                SSD_ALIGN_VCENTER);
      ssd_widget_set_color (box2, NULL, NULL);

      ssd_widget_add (box2,
                      ssd_text_new ("events_radius_label",
                                    roadmap_lang_get ("Events Radius"),
                                    SSD_MAIN_TEXT_SIZE, SSD_TEXT_NORMAL_FONT|SSD_TEXT_LABEL|SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE));
      ssd_widget_add(box, box2);

      snprintf(temp[0], 100, "5 ");
      snprintf(temp[1], 100, "25 ");
      snprintf(temp[2], 100, "50 ");
      snprintf(temp[3], 100, "100 ");
      snprintf(temp[4], 100, "200 ");

      if (roadmap_math_is_metric()){
         int i;
         distance_values[0] = "5";
         distance_values[1] = "25";
         distance_values[2] = "50";
         distance_values[3] = "100";
         distance_values[4] = "200";
         distance_values[5] = "-1";
         for (i=0; i<5; i++){
            strcat(temp[i], roadmap_lang_get("Km"));
            distance_labels[i] = strdup(temp[i]);
         }
      }
      else{
         int i;
         distance_values[0] = "8";
         distance_values[1] = "40";
         distance_values[2] = "80";
         distance_values[3] = "160";
         distance_values[4] = "320";
         distance_values[5] = "-1";
         for (i=0; i<5; i++){
            strcat(temp[i], roadmap_lang_get("miles"));
            distance_labels[i] = strdup(temp[i]);
         }
      }
      distance_labels[5] = roadmap_lang_get("All");

      ssd_widget_add (box,
                      ssd_choice_new ("event_radius", roadmap_lang_get ("Events Radius"),6,
                                      (const char **)distance_labels,
                                      (const void **)distance_values,
                                      SSD_ALIGN_RIGHT, NULL));
      ssd_widget_add(container, box);
      ssd_widget_add(dialog, container);

#ifndef TOUCH_SCREEN
      ssd_widget_set_left_softkey_text       ( dialog, roadmap_lang_get("Ok"));
      ssd_widget_set_left_softkey_callback   ( dialog, on_ok_softkey);
#endif
      ssd_dialog_activate (title, NULL);
   }

   if (!DialogShowsShown) {
      // Case insensitive comparison
#ifdef __SYMBIAN32__
      pVal = roadmap_config_match( &RoadMapConfigConnectionAuto, yesno[0] ) ? yesno[0] : yesno[1];
      ssd_dialog_set_data("AutoConnect", pVal );
#endif

#if (defined(__SYMBIAN32__) || defined(ANDROID))
      pVal = roadmap_config_match( &RoadMapConfigBackLight, yesno[0] ) ? yesno[0] : yesno[1];
      ssd_dialog_set_data("BackLight", pVal );
      ssd_dialog_set_data("Volume Control", ( void* ) roadmap_config_get_integer( &RoadMapConfigVolControl ) );
#endif // Symbian or android

	  if (roadmap_config_match(&NavigateConfigAutoZoom, "yes")) pVal = autozoom_value[1];
	  else if (roadmap_config_match(&NavigateConfigAutoZoom, "no")) pVal = autozoom_value[2];
	  else pVal =  autozoom_value[0];
	  ssd_dialog_set_data ("autozoom", (void *) pVal);

#ifndef TOUCH_SCREEN
      if (roadmap_config_match(&NavigateConfigNavigationGuidance, "yes")) pVal = yesno[0];
      else pVal = yesno[1];
      ssd_dialog_set_data ("navigationguidance", (void *) pVal);

#endif

      if (roadmap_config_match(&RoadMapConfigGeneralUserUnit, "default")){
         if (roadmap_config_match(&RoadMapConfigGeneralUnit, "metric")) pVal = yesno[0];
         else pVal = yesno[1];
         ssd_dialog_set_data ("use_metric", (void *) pVal);
      }
      else{
         if (roadmap_config_match(&RoadMapConfigGeneralUserUnit, "metric")) pVal = yesno[0];
         else pVal = yesno[1];
         ssd_dialog_set_data ("use_metric", (void *) pVal);
      }

      if (roadmap_config_match(&RoadMapConfigClockFormat, CLOCK_SETTINGS_12_HOUR)) pVal = yesno[0];
      else pVal = yesno[1];
      ssd_dialog_set_data ("ClockFormatChk", (void *) pVal);

      if (roadmap_alternative_feature_enabled()){
         if (roadmap_alternative_routes_suggest_routes()) pVal = yesno[0];
         else pVal = yesno[1];
         ssd_dialog_set_data ("AutoPromptRouteSuggestions", (void *) pVal);
      }

      if (roadmap_config_match(&RoadMapConfigShowTicker, "yes")) pVal = yesno[0];
      else pVal = yesno[1];
      ssd_dialog_set_data ("show_ticker", (void *) pVal);

      ssd_dialog_set_data ("lang", (void *) roadmap_lang_get_lang_value(roadmap_lang_get_system_lang()));
      ssd_dialog_set_data ("Prompts", (void *) roadmap_prompts_get_prompt_value(roadmap_prompts_get_name()));
#if (defined(_WIN32) || defined(ANDROID))
      if ( roadmap_config_match(&RoadMapConfigUseNativeKeyboard, "yes")) pVal = yesno[0];
      else pVal = yesno[1];

      ssd_dialog_set_data( "Native keyboard", (void *) pVal );
#endif


   }

   for (i = 0; i < 6; i++){
      if (!strcmp(distance_values[i], roadmap_config_get( &RoadMapConfigEventsRadius )))
         break;
   }
   ssd_dialog_set_data("event_radius", distance_values[i] );

   DialogShowsShown = 1;
   ssd_dialog_draw ();
}

BOOL roadmap_general_settings_is_24_hour_clock(){
	return roadmap_config_match(&RoadMapConfigClockFormat, CLOCK_SETTINGS_24_HOUR);
}

int roadmap_general_settings_events_radius(void){
   return roadmap_config_get_integer(&RoadMapConfigEventsRadius);
}

void roadmap_general_settings_init(void){
   roadmap_config_declare
      ("user", &RoadMapConfigConnectionAuto, "yes", NULL);
   roadmap_config_declare
      ("user", &RoadMapConfigBackLight, "yes", NULL);
   roadmap_config_declare
      ("user", &RoadMapConfigVolControl, SND_DEFAULT_VOLUME_LVL, NULL);
   roadmap_config_declare_enumeration
      ("preferences", &RoadMapConfigGeneralUnit, NULL, "imperial", "metric", NULL);

   roadmap_config_declare_enumeration
      ("user", &RoadMapConfigGeneralUserUnit, NULL, "default", "imperial", "metric", NULL);


   roadmap_config_declare_enumeration
      ("user", &RoadMapConfigShowTicker, NULL, "yes", "no", NULL);

   roadmap_config_declare_enumeration
      ("user", &RoadMapConfigEventsRadius, NULL, "-1", "5", "25", "50", "100", "200", NULL);

   roadmap_config_declare_enumeration
      ("user", &RoadMapConfigClockFormat, NULL, CLOCK_SETTINGS_12_HOUR, CLOCK_SETTINGS_24_HOUR, NULL);

}

