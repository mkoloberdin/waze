/*  roadmap_map_settings.c
 *
 * LICENSE:
 *
 *   Copyright 2008 Dan Friedman
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>

#include "roadmap_lang.h"
#include "roadmap_screen.h"
#include "roadmap_skin.h"
#include "roadmap_analytics.h"
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_button.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_choice.h"
#include "ssd/ssd_checkbox.h"
#include "ssd/ssd_separator.h"
#include "ssd/ssd_generic_list_dialog.h"
#include "roadmap_config.h"
#include "Realtime/Realtime.h"
#include "roadmap_map_download.h"
#include "Realtime/RealtimeAlerts.h"
#include "Realtime/RealtimeBonus.h"
#include <string.h>
#include <stdlib.h>

#ifdef IPHONE
#include "iphone/roadmap_map_settings_dialog.h"
#endif //IPHONE

static const char* title = "Map";
static int DialogShowsAgain = 0;
static const char *yesno_label[2];
static const char *yesno[2];
static void on_close_dialog (int exit_code, void* context);
static int on_ok( SsdWidget this, const char *new_value);
static void addShowAlertsToMenu(SsdWidget container);
#ifndef TOUCH_SCREEN
   static int on_ok_softkey(SsdWidget this, const char *new_value, void *context);
#endif
static void update_reports_dont_show();
RoadMapConfigDescriptor RoadMapConfigShowScreenIconsOnTap =
                        ROADMAP_CONFIG_ITEM("Map Icons", "Show on screen on tap");

RoadMapConfigDescriptor RoadMapConfigShowTopBarOnTap =
                        ROADMAP_CONFIG_ITEM("Map", "Show top bar on tap");

RoadMapConfigDescriptor RoadMapConfigAutoShowStreetBar =
                        ROADMAP_CONFIG_ITEM("Map", "Auto show street bar");

static SsdWidget space(int height);

RoadMapConfigDescriptor RoadMapConfigShowWazers =
                        ROADMAP_CONFIG_ITEM("Map", "Show other wazers");

RoadMapConfigDescriptor RoadMapConfigShowSpeedCams =
                        ROADMAP_CONFIG_ITEM("Map", "Show speed cams");

RoadMapConfigDescriptor RoadMapConfigColorRoads =
                        ROADMAP_CONFIG_ITEM("Map", "Show traffic jams");

RoadMapConfigDescriptor RoadMapConfigReportDontShow =
                        ROADMAP_CONFIG_ITEM("Map", "Dont Show reports types");

RoadMapConfigDescriptor RoadMapConfigEnableToggleConstruction =
                        ROADMAP_CONFIG_ITEM("Map", "Enable Toggle Construction");

RoadMapConfigDescriptor RoadMapConfigShowSpeedometer =
                        ROADMAP_CONFIG_ITEM("Map", "Show speedometer");

RoadMapConfigDescriptor RoadMapConfigRoadGoodies =
                        ROADMAP_CONFIG_ITEM("Map", "Show road goodies");

// wording of different reports, correlates to the report numbers defined in RealtimeAlert.h
static char*  AlertString[] = {"Chit Chats","Police traps","Accidents","Traffic jams ","Traffic info","Hazards",
									   "Other","Road constructions","Parking"};

static int alertsUserCanToggle [] = {0,1,2,3,5,7}; // we will let user decide not to show these reports.

#define MAP_SCHEMES 9
static char *map_labels[MAP_SCHEMES] = {"Vitamin C", "The Blues", "Mochaccino", "Snow Day", "Twilight", "Tutti-fruiti", "Rosebud", "Electrolytes", "Map editors"};
static void *map_values[MAP_SCHEMES]= {"", "1", "2", "3", "4", "5", "6", "7", "8"};
static void *map_icons[MAP_SCHEMES] = {"schema", "schema1", "schema2", "schema3", "schema4", "schema5", "schema6", "schema7", "schema8" };


#if 0
static RoadMapConfigDescriptor RoadMapConfigDisplayHouseNumbers =
                        ROADMAP_CONFIG_ITEM("Map", "Display house numbers");

static RoadMapConfigDescriptor RoadMapConfigDisplayMapProblems =
                        ROADMAP_CONFIG_ITEM("Map", "Display map problems");
#endif

extern RoadMapConfigDescriptor RoadMapConfigAutoNightMode;

static int initialized = 0;
void roadmap_map_settings_init(void){
	  roadmap_log (ROADMAP_DEBUG, "intialiazing map settings");

	  initialized = 1;
	  roadmap_config_declare_enumeration
         ("user", &RoadMapConfigShowScreenIconsOnTap, NULL, "yes", "no", NULL);

      roadmap_config_declare_enumeration
         ("user", &RoadMapConfigShowTopBarOnTap, NULL, "no", "yes", NULL);

   roadmap_config_declare_enumeration
         ("user", &RoadMapConfigAutoShowStreetBar, NULL, "yes", "no", NULL);

      roadmap_config_declare_enumeration
         ("user", &RoadMapConfigShowWazers, NULL, "yes", "no", NULL);

      roadmap_config_declare_enumeration
         ("user", &RoadMapConfigColorRoads, NULL, "yes", "no", NULL);

      roadmap_config_declare_enumeration
         ("user", &RoadMapConfigRoadGoodies, NULL, "yes", "no", NULL);

      roadmap_config_declare ("user", &RoadMapConfigReportDontShow, "", NULL);

      roadmap_config_declare_enumeration
         ("user", &RoadMapConfigShowSpeedCams, NULL, "yes", "no", NULL);

      roadmap_config_declare_enumeration
         ("user", &RoadMapConfigShowSpeedometer, NULL, "yes", "no", NULL);

      roadmap_config_declare_enumeration
         ("preferences", &RoadMapConfigEnableToggleConstruction, NULL, "yes", "no", NULL);


#if 0
      roadmap_config_declare_enumeration
         ("user", &RoadMapConfigDisplayHouseNumbers, NULL, "no", "yes", NULL);
      roadmap_config_declare_enumeration
         ("user", &RoadMapConfigDisplayMapProblems, NULL, "no", "yes", NULL);
#endif


      // Define the labels and values
	 yesno_label[0] = roadmap_lang_get ("Yes");
	 yesno_label[1] = roadmap_lang_get ("No");
	 yesno[0] = "Yes";
	 yesno[1] = "No";
}


/*
 * Returns TRUE iff the descriptor is enabled
 */
BOOL roadmap_map_settings_isEnabled(RoadMapConfigDescriptor descriptor){
	if(roadmap_config_match(&descriptor, "yes")){
		return TRUE;
	}
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////
static int map_scheme_callback (SsdWidget widget, const char *new_value, const void *value, void *context) {

   roadmap_analytics_log_event(ANALYTICS_EVENT_MAPSCHEME, ANALYTICS_EVENT_INFO_CHANGED_TO, value);
   roadmap_skin_set_scheme((const char *)value);
   ssd_dialog_hide_all(dec_close);
   return 1;
}

static int on_map_scheme_select (SsdWidget this, const char *new_value) {
   int i;

   for (i = 0; i < MAP_SCHEMES; i++){
      map_labels[i] = (char *)roadmap_lang_get(map_labels[i]);
   }

   ssd_generic_icon_list_dialog_show (roadmap_lang_get ("Select map color scheme"),
               MAP_SCHEMES,
               (const char **)map_labels,
               (const void **)map_values,
               (const char **)map_icons,
               NULL,
               map_scheme_callback,
               NULL,
               NULL,
               NULL,
               NULL,
               ssd_container_get_row_height(),
               0,
               FALSE);
   return 1;
}

/*
 * Show the map settings dialog
 */
void roadmap_map_settings_show(void){
   const char *pVal;
   const char *edit_button[] = {"edit_right", "edit_left"};
   const char *buttons[2];
   SsdWidget button, text;
   int tab_flag = SSD_WS_TABSTOP;
   int row_height = ssd_container_get_row_height();
   int width = ssd_container_get_width();

   roadmap_analytics_log_event(ANALYTICS_EVENT_MAPSETTINGS, NULL, NULL);

    roadmap_log (ROADMAP_DEBUG, "create map settings menu");
   if (!initialized) {
      roadmap_map_settings_init();
   }
#ifdef IPHONE
   roadmap_map_settings_dialog_show();
#else

    if (!ssd_dialog_activate (title, NULL)) {
      SsdWidget dialog;
      SsdWidget box;
	  SsdWidget box2;

      SsdWidget container;
      dialog = ssd_dialog_new (title, roadmap_lang_get(title), on_close_dialog,
                               SSD_CONTAINER_TITLE);

#ifdef TOUCH_SCREEN
	  ssd_widget_add(dialog, space(5));
#endif


       // add header
       box = ssd_container_new ("Map settings header group", NULL, width, SSD_MIN_SIZE, SSD_WIDGET_SPACE|SSD_ALIGN_CENTER|SSD_END_ROW );
       ssd_widget_set_color( box, NULL, NULL );
  	    ssd_widget_add ( box, ssd_text_new ("map_settings_header_text_cont",
            roadmap_lang_get ("General map settings"), SSD_MAIN_TEXT_SIZE, SSD_TEXT_NORMAL_FONT|SSD_TEXT_LABEL | SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE));
       ssd_widget_add (dialog, box);

       container = ssd_container_new ("General prefs", NULL, width, SSD_MIN_SIZE,
              SSD_WIDGET_SPACE|SSD_END_ROW|SSD_CONTAINER_FLAGS|SSD_POINTER_NONE|SSD_CONTAINER_BORDER|SSD_ALIGN_CENTER);


      if (roadmap_skin_auto_night_feature_enabled()){
	      //Auto Night mode
         box = ssd_checkbox_row_new("auto_night_mode", roadmap_lang_get ("Automatic night mode"),
               roadmap_map_settings_isEnabled(RoadMapConfigAutoNightMode), NULL,NULL,NULL,CHECKBOX_STYLE_ON_OFF);

         ssd_widget_add (container, box);
         ssd_widget_add(container, ssd_separator_new("separator", SSD_END_ROW));
      }
#ifdef TOUCH_SCREEN
      //Show Icons
      box = ssd_checkbox_row_new("show_icons", roadmap_lang_get ("Display map controls on tap"),
            roadmap_map_settings_isEnabled(RoadMapConfigShowScreenIconsOnTap), NULL,NULL,NULL,CHECKBOX_STYLE_ON_OFF);

	   ssd_widget_add (container, box);
	   ssd_widget_add(container, ssd_separator_new("separator", SSD_END_ROW));

	// show top bar
	   box = ssd_checkbox_row_new("show_top_bar", roadmap_lang_get ("Display top bar on tap"),
	         roadmap_map_settings_isEnabled(RoadMapConfigShowTopBarOnTap), NULL,NULL,NULL,CHECKBOX_STYLE_ON_OFF);

	   ssd_widget_add (container, box);
	   ssd_widget_add(container, ssd_separator_new("separator", SSD_END_ROW));

#endif


      // Map Color schem
      //ssd_widget_add(box, space(1));
      box = ssd_container_new ("map schem group", NULL, SSD_MAX_SIZE, row_height,
                                SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag);
      ssd_widget_set_color (box, NULL, NULL);

      box2 = ssd_container_new ("map_schem box2", NULL, roadmap_canvas_width()/3, SSD_MIN_SIZE,
                                  SSD_ALIGN_VCENTER);
      ssd_widget_set_color (box2, NULL, NULL);
      text =  ssd_text_new ("map_scheme_label", roadmap_lang_get ("Map color scheme"),
                            SSD_MAIN_TEXT_SIZE, SSD_TEXT_NORMAL_FONT|SSD_TEXT_LABEL|SSD_ALIGN_VCENTER);
      ssd_text_set_color(text, SSD_CONTAINER_TEXT_COLOR);
      ssd_widget_add (box2, text);

      ssd_widget_add(box, box2);
      if (!ssd_widget_rtl(NULL)){
             buttons[0] = edit_button[0];
             buttons[1] = edit_button[0];
      }else{
             buttons[0] = edit_button[1];
             buttons[1] = edit_button[1];
      }


      text = ssd_text_new ("map_schem_label_value",roadmap_lang_get(map_labels[roadmap_skin_get_scheme()]) , SSD_SECONDARY_TEXT_SIZE, SSD_ALIGN_VCENTER|SSD_ALIGN_RIGHT);
      ssd_text_set_color(text, "#206892");
      ssd_widget_add (box, text);

#ifdef TOUCH_SCREEN
      button = ssd_button_new ("edit_button", "", &buttons[0], 2,
                              SSD_ALIGN_VCENTER|SSD_ALIGN_RIGHT, on_map_scheme_select);
      ssd_widget_set_click_offsets_ext(button, -300, -10, 300, 10);
      ssd_widget_add(box, button);
#else
      box->callback = on_map_scheme_select;
#endif
      ssd_widget_add(box, space(1));

      ssd_widget_add (container, box);

      ssd_widget_add (dialog, container);



	   container = ssd_container_new ("Map GUI prefs", NULL, width, SSD_MIN_SIZE,
              SSD_WIDGET_SPACE|SSD_END_ROW|SSD_CONTAINER_FLAGS|SSD_POINTER_NONE|SSD_CONTAINER_BORDER|SSD_ALIGN_CENTER);



       // add header
       box = ssd_container_new ( "Map GUI prefs Heading group", NULL, width, SSD_MIN_SIZE, SSD_WIDGET_SPACE | SSD_ALIGN_CENTER | SSD_END_ROW );

  	   ssd_widget_add (box, ssd_text_new ("Map_Gui_prefs_header_txt", roadmap_lang_get ("Show on map"),
                     SSD_MAIN_TEXT_SIZE, SSD_TEXT_NORMAL_FONT|SSD_TEXT_LABEL | SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE));
       ssd_widget_set_color (box, NULL, NULL);
       ssd_widget_add (dialog, box);


       // Show speedometer group
       box = ssd_checkbox_row_new("ShowSpeedometer", roadmap_lang_get ("Speedometer"),
             roadmap_map_settings_isEnabled(RoadMapConfigShowSpeedometer), NULL,NULL,NULL,CHECKBOX_STYLE_ON_OFF);

       ssd_widget_add (container, box);
       ssd_widget_add(container, ssd_separator_new("separator", SSD_END_ROW));

       // Show wazers group
       box = ssd_checkbox_row_new("ShowWazers", roadmap_lang_get ("Wazers"),
             roadmap_map_settings_isEnabled(RoadMapConfigShowWazers), NULL,NULL,NULL,CHECKBOX_STYLE_ON_OFF);

       ssd_widget_add (container, box);
       ssd_widget_add(container, ssd_separator_new("separator", SSD_END_ROW));

	    //show automatic traffic events on roads
       box = ssd_checkbox_row_new("ColorRoads", roadmap_lang_get ("Traffic layer (color coded)"),
             roadmap_map_settings_isEnabled(RoadMapConfigColorRoads), NULL,NULL,NULL,CHECKBOX_STYLE_ON_OFF);

       ssd_widget_add (container, box);
       ssd_widget_add(container, ssd_separator_new("separator", SSD_END_ROW));

	   //show speed cams
       box = ssd_checkbox_row_new("SpeedCams", roadmap_lang_get ("Speed cams"),
             roadmap_map_settings_isEnabled(RoadMapConfigShowSpeedCams), NULL,NULL,NULL,CHECKBOX_STYLE_ON_OFF);
       ssd_widget_add (container, box);
       ssd_widget_add(container, ssd_separator_new("separator", SSD_END_ROW));

       //Road goodies
       box = ssd_checkbox_row_new("RoadGoodies", roadmap_lang_get ("Road goodies"),
             roadmap_map_settings_isEnabled(RoadMapConfigRoadGoodies), NULL,NULL,NULL,CHECKBOX_STYLE_ON_OFF);
        ssd_widget_add (container, box);
        ssd_widget_add(container, ssd_separator_new("separator", SSD_END_ROW));

       // add report options
	    addShowAlertsToMenu(container);


#if 0
	   //Display house number
	   box = ssd_container_new ("Display house numbers", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
              SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag);
	   ssd_widget_add(box, ssd_separator_new("separator", SSD_ALIGN_BOTTOM));
       ssd_widget_set_color (box, "#000000", "#ffffff");


	   box2 = ssd_container_new ("Display house number group", NULL, roadmap_canvas_width()/2, SSD_MIN_SIZE,
	                            SSD_ALIGN_VCENTER);
	   ssd_widget_set_color (box, "#000000", "#ffffff");
       ssd_widget_add (box2,
         ssd_text_new ( "Display house numbers Label",
                        roadmap_lang_get ("Display house numbers"),
                        SSD_MAIN_TEXT_SIZE, SSD_TEXT_NORMAL_FONT|SSD_TEXT_LABEL|SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE ) );
	   ssd_widget_set_color (box2, NULL, NULL);
 	   ssd_widget_add(box, box2);



       ssd_widget_add (box,
            ssd_checkbox_new ( "DisplayHouseNumbers", roadmap_map_settings_isEnabled(RoadMapConfigDisplayHouseNumbers)
            , SSD_ALIGN_RIGHT, NULL,NULL, NULL,CHECKBOX_STYLE_ON_OFF ) );

       ssd_widget_add(box, space(1));
       ssd_widget_add(box, ssd_separator_new("separator", SSD_ALIGN_BOTTOM));
       ssd_widget_add (container, box);



	   // Show map problems group
	   box = ssd_container_new ("Show map problems Group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
              SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag);

       ssd_widget_set_color (box, "#000000", "#ffffff");

	   box2 = ssd_container_new ("Display map problems group", NULL, roadmap_canvas_width()/2, SSD_MIN_SIZE,
	                            SSD_ALIGN_VCENTER);
	   ssd_widget_set_color (box, "#000000", "#ffffff");
       ssd_widget_add (box2,
         ssd_text_new ( "Show map problems Label",
                        roadmap_lang_get ("Show map problems"),
                        SSD_MAIN_TEXT_SIZE, SSD_TEXT_NORMAL_FONT|SSD_TEXT_LABEL|SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE ) );
       ssd_widget_set_color (box2, NULL, NULL);

 	   ssd_widget_add(box, box2);

       ssd_widget_add (box,
            ssd_checkbox_new ( "ShowMapProblems", roadmap_map_settings_isEnabled(RoadMapConfigDisplayMapProblems)
            , SSD_ALIGN_RIGHT, NULL,NULL, NULL,CHECKBOX_STYLE_ON_OFF ) );

       ssd_widget_add(box, space(1));
       ssd_widget_add(box, ssd_separator_new("separator", SSD_ALIGN_BOTTOM));
       ssd_widget_add (container, box);
#endif
	   // add the container to the dialog
	   ssd_widget_add(dialog, container);

#ifndef TOUCH_SCREEN
  	   ssd_widget_set_left_softkey_text       ( dialog, roadmap_lang_get("Ok"));
       ssd_widget_set_left_softkey_callback   ( dialog, on_ok_softkey);
#endif
       ssd_dialog_activate (title, NULL);
     }

     if (!DialogShowsAgain) {
#ifdef TOUCH_SCREEN
     	if (roadmap_config_match(&RoadMapConfigShowScreenIconsOnTap, "yes")) pVal = yesno[0];
        else pVal = yesno[1];
        ssd_dialog_set_data ("show_icons", (void *) pVal);

        if (roadmap_config_match(&RoadMapConfigShowTopBarOnTap, "yes")) pVal = yesno[0];
        else pVal = yesno[1];
        ssd_dialog_set_data ("show_top_bar", (void *) pVal);
#endif
      if (roadmap_skin_auto_night_feature_enabled()){
         if (roadmap_config_match(&RoadMapConfigAutoNightMode, "yes")) pVal = yesno[0];
            else pVal = yesno[1];
         ssd_dialog_set_data ("auto_night_mode", (void *) pVal);
      }

     }
     else{
        ssd_dialog_set_value ("map_schem_label_value",roadmap_lang_get(map_labels[roadmap_skin_get_scheme()]));
     }
	 DialogShowsAgain = 1;
	 ssd_dialog_draw ();
#endif //IPHONE
}


static void on_close_dialog (int exit_code, void* context){
#ifdef TOUCH_SCREEN
	if (exit_code == dec_ok)
		on_ok(NULL, NULL);
#endif
}


static int on_ok( SsdWidget this, const char *new_value){
	const char * data;
	BOOL prev_show_icons;
    roadmap_log (ROADMAP_DEBUG, "exitting map settings");
#ifdef TOUCH_SCREEN

    prev_show_icons = roadmap_config_match(&RoadMapConfigShowScreenIconsOnTap, "yes");

    if (prev_show_icons){
       if(strcasecmp( ( const char* ) ssd_dialog_get_data("show_icons"), yesno[0] ))
          roadmap_analytics_log_event(ANALYTICS_EVENT_MAPCONTROLSSET, ANALYTICS_EVENT_INFO_CHANGED_TO, ANALYTICS_EVENT_OFF);
    }
    else{
       if(!strcasecmp( ( const char* ) ssd_dialog_get_data("show_icons"), yesno[0] ))
          roadmap_analytics_log_event(ANALYTICS_EVENT_MAPCONTROLSSET, ANALYTICS_EVENT_INFO_CHANGED_TO, ANALYTICS_EVENT_ON);
    }
    roadmap_config_set (&RoadMapConfigShowScreenIconsOnTap,
                           (const char *)ssd_dialog_get_data ("show_icons"));


    roadmap_config_set (&RoadMapConfigShowTopBarOnTap,
                           (const char *)ssd_dialog_get_data ("show_top_bar"));
#endif
    if (roadmap_skin_auto_night_feature_enabled()){
       BOOL prev_on = roadmap_config_match(&RoadMapConfigAutoNightMode, "yes");

       roadmap_config_set (&RoadMapConfigAutoNightMode,
                              (const char *)ssd_dialog_get_data ("auto_night_mode"));
       if(!strcasecmp( ( const char* ) ssd_dialog_get_data("auto_night_mode"), yesno[0] )){
          if (!prev_on)
             roadmap_analytics_log_event(ANALYTICS_EVENT_NIGHTMODESET, ANALYTICS_EVENT_INFO_CHANGED_TO, ANALYTICS_EVENT_ON);
          roadmap_skin_auto_night_mode();
       }
       else{
          if (prev_on)
             roadmap_analytics_log_event(ANALYTICS_EVENT_NIGHTMODESET, ANALYTICS_EVENT_INFO_CHANGED_TO, ANALYTICS_EVENT_OFF);
          roadmap_skin_auto_night_mode_kill_timer();
       }
    }

   // if changed wazer display data - notify server we don't want the data anymore
   data = (const char *)ssd_dialog_get_data("ShowWazers");
   if (!(roadmap_config_match(&RoadMapConfigShowWazers, data))){ // descriptor changed
		 roadmap_config_set (&RoadMapConfigShowWazers,data);
		 OnSettingsChanged_VisabilityGroup();
   }

   roadmap_config_set (&RoadMapConfigColorRoads,
                           (const char *)ssd_dialog_get_data ("ColorRoads"));

   roadmap_config_set (&RoadMapConfigShowSpeedCams,
                           (const char *)ssd_dialog_get_data ("SpeedCams"));

   roadmap_config_set (&RoadMapConfigShowSpeedometer,
                           (const char *)ssd_dialog_get_data ("ShowSpeedometer"));

   roadmap_config_set (&RoadMapConfigRoadGoodies,
                           (const char *)ssd_dialog_get_data ("RoadGoodies"));
   if (!strcmp((const char *)ssd_dialog_get_data ("RoadGoodies"), "no"))
      RealtimeBonus_Term();

   update_reports_dont_show();
   roadmap_config_save(TRUE);
   DialogShowsAgain = 0;
   return 0;
}

#ifndef TOUCH_SCREEN
static int on_ok_softkey(SsdWidget this, const char *new_value, void *context){
	on_ok(this, new_value);
	ssd_dialog_hide_all(dec_ok);
	return 0;
}
#endif


/////////////////////////////////////////////////////////////////////
static SsdWidget space(int height){
	SsdWidget space;
	space = ssd_container_new ("spacer", NULL, SSD_MAX_SIZE, height, SSD_WIDGET_SPACE|SSD_END_ROW);
	ssd_widget_set_color (space, NULL,NULL);
	return space;
}



BOOL roadmap_map_settings_isShowWazers(){
	return roadmap_map_settings_isEnabled(RoadMapConfigShowWazers);
}

BOOL roadmap_map_settings_isShowSpeedCams(){
	return roadmap_map_settings_isEnabled(RoadMapConfigShowSpeedCams);
}


BOOL roadmap_map_settings_isShowTopBarOnTap(){
	return roadmap_map_settings_isEnabled(RoadMapConfigShowTopBarOnTap);
}

BOOL roadmap_map_settings_isAutoShowStreetBar(){
	return roadmap_map_settings_isEnabled(RoadMapConfigAutoShowStreetBar);
}

BOOL roadmap_map_settings_isShowSpeedometer(){
   return roadmap_map_settings_isEnabled(RoadMapConfigShowSpeedometer);
}

// return true iff report of given type should be shown.
BOOL roadmap_map_settings_show_report(int iType){
	char dontShowReports[100];
	char * tChar;
	int num;
	const char * dontShowReportsConst = (const char *)roadmap_config_get(&RoadMapConfigReportDontShow);
	strcpy(dontShowReports,dontShowReportsConst);
	tChar = strtok(dontShowReports,"-"); // start parsing.
	while (tChar!=NULL){
		num = atoi(tChar);
	    if (num ==iType)
	    	return FALSE;
    	tChar = strtok(NULL,"-");
	}
	return TRUE;
}

BOOL roadmap_map_settings_color_roads(){
	return roadmap_map_settings_isEnabled(RoadMapConfigColorRoads);
}

BOOL roadmap_map_settings_road_goodies(){
   return roadmap_map_settings_isEnabled(RoadMapConfigRoadGoodies);
}



// adds the reports show toggle to the map settings ssd menu
static void addShowAlertsToMenu(SsdWidget container){
	SsdWidget box;
	int count;
	int i ;
	char checkBoxText[100];  // showPoliceChkbox
	char displayText[100]; // police
	count = sizeof(alertsUserCanToggle)/sizeof(int); // number of show alerts to add
	for (i = 0; i < count; i++){

	   if (!roadmap_map_settings_isEnabled(RoadMapConfigEnableToggleConstruction))
	   		if ( alertsUserCanToggle[i] == RT_ALERT_TYPE_CONSTRUCTION ) // don't show construction
	   			continue;
	   //RoadMapConfigEnableToggleConstruction
	   // create the appropriate texts for the Ssd widget creations.
	   strcpy(checkBoxText,"Show"); strcat(checkBoxText,AlertString[alertsUserCanToggle[i]]); strcat(checkBoxText,"Chkbox");
	   strcpy(displayText,AlertString[alertsUserCanToggle[i]]);


	   box = ssd_checkbox_row_new(checkBoxText, roadmap_lang_get (displayText),
	         roadmap_map_settings_show_report(alertsUserCanToggle[i]), NULL,NULL,NULL,CHECKBOX_STYLE_ON_OFF);
     ssd_widget_add (container, box);
     if (i < count -2)
        ssd_widget_add(container, ssd_separator_new("separator", SSD_END_ROW));
	}
}

// updates the reports don't show descriptor, according to the chkboxs' values.
static void update_reports_dont_show(){
	char checkBoxText[100];  // showPoliceChkbox
	char newDescriptorData[100];
	char num[5];
	BOOL firstReport = TRUE;
	int count;
	int i ;
	count = sizeof(alertsUserCanToggle)/sizeof(int);
	strcpy(newDescriptorData,"");
	for (i = 0; i < count; i++){

		if (!roadmap_map_settings_isEnabled(RoadMapConfigEnableToggleConstruction))
	   		if ( alertsUserCanToggle[i] == RT_ALERT_TYPE_CONSTRUCTION ) // don't show construction
	   			continue;

		strcpy(checkBoxText,"Show"); strcat(checkBoxText,AlertString[alertsUserCanToggle[i]]); strcat(checkBoxText,"Chkbox");
		if(strcasecmp( ( const char* ) ssd_dialog_get_data(checkBoxText), yesno[0] )){ // user decided not to show report
		    if (firstReport){
		    	firstReport = FALSE;
		    }else{
		    	strcat(newDescriptorData,"-");
		    }
		    snprintf(num , 5, "%d",alertsUserCanToggle[i]);
		    strcat( newDescriptorData, num );

		}
	}
	roadmap_config_set(&RoadMapConfigReportDontShow,newDescriptorData);
   RTAlerts_RefreshOnMap();

	return;
}


int roadmap_map_settings_allowed_alerts(int outAlertsUserCanToggle[]){
   int i;
   int count;

   if (!initialized)
      return 0;

   count = sizeof(alertsUserCanToggle)/sizeof(int);

   for (i = 0; i < count; i++){
      outAlertsUserCanToggle[i] = alertsUserCanToggle[i];
   }
   return count;
}

void roadmap_map_settings_alert_string(char *outAlertString[]){
   int i;
   int count;

   if (!initialized)
      return;

   count = sizeof(alertsUserCanToggle)/sizeof(int);

   for (i = 0; i <= alertsUserCanToggle[count-1]; i++){
      outAlertString[i] = AlertString[i];
   }
}

