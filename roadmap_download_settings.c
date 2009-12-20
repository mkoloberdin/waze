/*  roadmap_download_settings.c
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

#include "roadmap_lang.h"
#include "roadmap_skin.h"
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_button.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_choice.h"
#include "ssd/ssd_checkbox.h"
#include "ssd/ssd_separator.h"
#include "roadmap_config.h"
#include "Realtime/Realtime.h"
#include "roadmap_map_settings.h"
#include "roadmap_map_download.h"
#include <string.h>

#ifdef IPHONE
#include "iphone/roadmap_download_settings_dialog.h"
#endif //IPHONE
static const char* title = "Data Usage";
static const char *yesno_label[2];
static const char *yesno[2];
static void on_close_dialog (int exit_code, void* context);
static int on_ok( SsdWidget this, const char *new_value);
static int on_ok_softkey(SsdWidget this, const char *new_value, void *context);
static void updateVisibilityDescriptors(RoadMapConfigDescriptor descriptor,const char * labelName);
static BOOL needToNotifyServer = FALSE;
static const char * NOTE1 = "* Changes won't affect routing. Your route is always calculated based on real-time traffic conditions.";         
static const char * NOTE2 = "* Traffic and updates will not be seen on the map if you disable their download.";                                           
static SsdWidget space(int height);


static RoadMapConfigDescriptor RoadMapConfigDownloadTraffic =
                        ROADMAP_CONFIG_ITEM("Download", "Download traffic jams");    
                        
static RoadMapConfigDescriptor RoadMapConfigDisplayDownload =
                  ROADMAP_CONFIG_ITEM("Download", "Display data download options");                        

#if 0   
static RoadMapConfigDescriptor RoadMapConfigDownloadWazers =
                        ROADMAP_CONFIG_ITEM("Download", "Download other wazers");
                        
static RoadMapConfigDescriptor RoadMapConfigDownloadUserReports =
                        ROADMAP_CONFIG_ITEM("Download", "Download user reports");
                        
static RoadMapConfigDescriptor RoadMapConfigDownloadTrafficJams =
                        ROADMAP_CONFIG_ITEM("Download", "Download traffic jams");    

                     
static RoadMapConfigDescriptor RoadMapConfigDownloadHouseNumbers =
                        ROADMAP_CONFIG_ITEM("Download", "Download house numbers");    
                        
static RoadMapConfigDescriptor RoadMapConfigDownloadMapProblems =
                        ROADMAP_CONFIG_ITEM("Download", "Download map problems");   
                       
#endif               
                                 


static int initialized = 0;
void roadmap_download_settings_init(void){
	  roadmap_log (ROADMAP_DEBUG, "intialiazing map settings");
	
	  initialized = 1;
	  roadmap_config_declare_enumeration
         ("user", &RoadMapConfigDownloadTraffic, NULL, "yes", "no", NULL);
      roadmap_config_declare_enumeration
         ("preferences", &RoadMapConfigDisplayDownload, NULL, "no", "yes", NULL);         
#if 0             
      roadmap_config_declare_enumeration
         ("user", &RoadMapConfigDownloadWazers, NULL, "yes", "no", NULL);
      roadmap_config_declare_enumeration
         ("user", &RoadMapConfigDownloadUserReports, NULL, "yes", "no", NULL);
      roadmap_config_declare_enumeration
         ("user", &RoadMapConfigDownloadTrafficJams, NULL, "yes", "no", NULL);
         

      roadmap_config_declare_enumeration
         ("user", &RoadMapConfigDownloadHouseNumbers, NULL, "no", "yes", NULL);
      roadmap_config_declare_enumeration
         ("user", &RoadMapConfigDownloadMapProblems, NULL, "no", "yes", NULL);
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
BOOL roadmap_download_settings_isEnabled(RoadMapConfigDescriptor descriptor){
	if(roadmap_config_match(&descriptor, "yes")){
		return TRUE;
	}
	return FALSE;
}

/*
 * Show the map settings dialog
 */
void roadmap_download_settings_show(void){
   char *icon[2];
   int tab_flag = SSD_WS_TABSTOP;
   const char * notesColor;
   roadmap_log (ROADMAP_DEBUG, "creatting download settings menu");
   if (!initialized) {
      roadmap_download_settings_init();
   }
    
#ifdef IPHONE
   roadmap_download_settings_dialog_show();
#else
    if (!ssd_dialog_activate (title, NULL)) {
      SsdWidget dialog;
      SsdWidget box;
	  SsdWidget box2,text;
	  SsdWidget space_container;

      SsdWidget container;
      dialog = ssd_dialog_new (title, roadmap_lang_get(title), on_close_dialog,
                               SSD_CONTAINER_TITLE);

#ifdef TOUCH_SCREEN
	  ssd_widget_add(dialog, space(5));
#endif
      

       // add header
       box = ssd_container_new ("Download Heading group", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE,
            SSD_WIDGET_SPACE | SSD_END_ROW);
   	   
  	   ssd_widget_add (box, ssd_text_new ("Download_Heading_text_cont",
            roadmap_lang_get ("Reduce data usage:"), 16, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE));
       ssd_widget_set_color (box, NULL, NULL);
       ssd_widget_add (dialog, box);
       
       ssd_widget_add(dialog, space(2));

       // download map group
      box = ssd_container_new ("download map group", NULL,
                  SSD_MAX_SIZE,47,
		  SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE|SSD_POINTER_NONE|SSD_CONTAINER_BORDER);
	  ssd_widget_set_color (box, "#000000", "#ffffff");
	

	  icon[0] = "download_map";
      space_container =  ssd_container_new ("space", NULL, 47, SSD_MIN_SIZE, SSD_ALIGN_VCENTER);
	  ssd_widget_set_color(space_container, NULL, NULL);
	  ssd_widget_add (space_container,
         ssd_button_new ("Download map button", "Download map",
                         (const char **)&icon[0], 1,SSD_START_NEW_ROW|SSD_ALIGN_VCENTER,
                        roadmap_map_download));
	  box->callback = roadmap_map_download;
      
    
	  ssd_widget_add (box,space_container );
	  ssd_widget_add (box,
         ssd_text_new ("Download map text", roadmap_lang_get("Download map of my area"), 16, SSD_ALIGN_VCENTER));	  
	  ssd_widget_add(dialog,box);
	  ssd_widget_add(dialog, space(3));
	  
	  // disable traffic download group
	   container = ssd_container_new ("Download prefs", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
            SSD_WIDGET_SPACE|SSD_END_ROW|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE|SSD_POINTER_NONE|SSD_CONTAINER_BORDER);
		
	   box = ssd_container_new ("Download Traffic Group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
              SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag);
       ssd_widget_set_color (box, "#000000", "#ffffff");
       box2 = ssd_container_new ("download traffic text group", NULL, roadmap_canvas_width()/2, SSD_MIN_SIZE,
	                            SSD_ALIGN_VCENTER);
	   ssd_widget_set_color (box2, NULL, NULL);
       ssd_widget_add (box2,
         ssd_text_new ( "Download traffic Label",
                        roadmap_lang_get ("Download traffic info"),
                        -1, SSD_TEXT_LABEL|SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE ) );
 	   ssd_widget_add(box, box2);
       ssd_widget_add (box,
            ssd_checkbox_new ( "DownloadTraffic", roadmap_download_settings_isEnabled(RoadMapConfigDownloadTraffic)
            , SSD_ALIGN_RIGHT, NULL,NULL, NULL,CHECKBOX_STYLE_ON_OFF ) );
            
    

       ssd_widget_add(box, space(1));
      
       ssd_widget_add (container, box);	
		
		
	   	
		
#if 0  // right now user can only choose to toggle a general traffic download settings
       // Download wazers group
	   box = ssd_container_new ("Download Wazers Group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
              SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag);
	   ssd_widget_add(box, ssd_separator_new("separator", SSD_ALIGN_BOTTOM));
       ssd_widget_set_color (box, "#000000", "#ffffff");
       box2 = ssd_container_new ("download Wazers text group", NULL, roadmap_canvas_width()/2, SSD_MIN_SIZE,
	                            SSD_ALIGN_VCENTER);
	   ssd_widget_set_color (box, "#000000", "#ffffff");
       ssd_widget_add (box2,
         ssd_text_new ( "Download Wazer Label",
                        roadmap_lang_get ("Download wazers info "),
                        -1, SSD_TEXT_LABEL|SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE ) );
 	   ssd_widget_add(box, box2);
       ssd_widget_add (box,
            ssd_checkbox_new ( "DownloadWazers", roadmap_download_settings_isEnabled(RoadMapConfigDownloadWazers)
            , SSD_ALIGN_RIGHT, NULL,NULL, NULL,CHECKBOX_STYLE_ON_OFF ) );

       ssd_widget_add(box, space(1));
      
       ssd_widget_add (container, box);
      
	   //download user reports
	   box = ssd_container_new ("Download user reports Group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
              SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag);

       ssd_widget_set_color (box, "#000000", "#ffffff");

 	   ssd_widget_add(box,     ssd_text_new ( "Download user reports Label",
                        roadmap_lang_get ("Download user reports"),
                        -1, SSD_TEXT_LABEL|SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE ));
 	   ssd_widget_add (box,
            ssd_checkbox_new ( "DownloadReports", roadmap_download_settings_isEnabled(RoadMapConfigDownloadUserReports)
            , SSD_ALIGN_RIGHT, NULL,NULL, NULL,CHECKBOX_STYLE_ON_OFF ) );

       ssd_widget_add(box, space(1));
       ssd_widget_add(box, ssd_separator_new("separator", SSD_ALIGN_BOTTOM));
       ssd_widget_add (container, box);
	   
	   
	   //download automatic traffic events on roads
	   box = ssd_container_new ("Download automatic traffic Group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
              SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag);

       ssd_widget_set_color (box, "#000000", "#ffffff");
		
		
		box2 = ssd_container_new ("download automatic traffic text group", NULL, roadmap_canvas_width()/2, SSD_MIN_SIZE,
	                            SSD_ALIGN_VCENTER);
	   ssd_widget_set_color (box, "#000000", "#ffffff");
       ssd_widget_add (box2,
         ssd_text_new ( "Download automatic traffic Label",
                        roadmap_lang_get ("Download traffic jams"),
                        -1, SSD_TEXT_LABEL|SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE ) );
 	   ssd_widget_add(box, box2);
       
 
       ssd_widget_add (box,
            ssd_checkbox_new ( "DownloadAutoTraffic", roadmap_download_settings_isEnabled(RoadMapConfigDownloadTrafficJams)
            , SSD_ALIGN_RIGHT, NULL,NULL, NULL,CHECKBOX_STYLE_ON_OFF ) );

       ssd_widget_add(box, space(1));
      
       ssd_widget_add (container, box);

	   
	   //Download house number 
	   box = ssd_container_new ("Download house numbers", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
              SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag);
	   ssd_widget_add(box, ssd_separator_new("separator", SSD_ALIGN_BOTTOM));
       ssd_widget_set_color (box, "#000000", "#ffffff");
	   
	   
	   box2 = ssd_container_new ("Download house number group", NULL, roadmap_canvas_width()/2, SSD_MIN_SIZE,
	                            SSD_ALIGN_VCENTER);
	   ssd_widget_set_color (box, "#000000", "#ffffff");
       ssd_widget_add (box2,
         ssd_text_new ( "Download house numbers Label",
                        roadmap_lang_get ("Download house numbers"),
                        -1, SSD_TEXT_LABEL|SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE ) );
 	   ssd_widget_add(box, box2);
 	   
       

       ssd_widget_add (box,
            ssd_checkbox_new ( "DownloadHouseNumbers", roadmap_download_settings_isEnabled(RoadMapConfigDownloadHouseNumbers)
            , SSD_ALIGN_RIGHT, NULL,NULL, NULL,CHECKBOX_STYLE_ON_OFF ) );

       ssd_widget_add(box, space(1));
       ssd_widget_add(box, ssd_separator_new("separator", SSD_ALIGN_BOTTOM));
       ssd_widget_add (container, box);
	   
	   
	   
	   // Download map problems group
	   box = ssd_container_new ("Download map problems Group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
              SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag);

       ssd_widget_set_color (box, "#000000", "#ffffff");
		
	   box2 = ssd_container_new ("Download map problems group", NULL, roadmap_canvas_width()/2, SSD_MIN_SIZE,
	                            SSD_ALIGN_VCENTER);
	   ssd_widget_set_color (box, "#000000", "#ffffff");
       ssd_widget_add (box2,
         ssd_text_new ( "Download map problems Label",
                        roadmap_lang_get ("Download map problems"),
                        -1, SSD_TEXT_LABEL|SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE ) );
 
 	   ssd_widget_add(box, box2);	
		
       ssd_widget_add (box,
            ssd_checkbox_new ( "DownloadMapProblems", roadmap_download_settings_isEnabled(RoadMapConfigDownloadMapProblems)
            , SSD_ALIGN_RIGHT, NULL,NULL, NULL,CHECKBOX_STYLE_ON_OFF ) );
            
       ssd_widget_add(box, space(1));
       ssd_widget_add(box, ssd_separator_new("separator", SSD_ALIGN_BOTTOM));
       ssd_widget_add (container, box);
#endif   
	   // add the container to the dialog
	   ssd_widget_add(dialog, container);
	  
	  // add notes
	   notesColor = "#3b3838"; // some sort of gray
       box = ssd_container_new ("Note group", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE,
            SSD_WIDGET_SPACE | SSD_END_ROW);
   	   text = ssd_text_new ("Note_text_cont",
            roadmap_lang_get ("Note:"), 14, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE);
       ssd_text_set_color(text,notesColor);
  	   ssd_widget_add (box,text );
       ssd_widget_set_color (box, NULL, NULL);
       ssd_widget_add (dialog, box);      
       box = ssd_container_new ("Note1 group", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE,
            SSD_WIDGET_SPACE | SSD_END_ROW|SSD_WS_TABSTOP);   
       text = ssd_text_new ("Note1_text_cont",
            roadmap_lang_get (NOTE1), 14, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE);
       ssd_widget_add (box,text );
       ssd_text_set_color(text,notesColor);
       ssd_widget_set_color (box, NULL, NULL);
       ssd_widget_add (dialog, box);
       box = ssd_container_new ("Note2 group", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE,
            SSD_WIDGET_SPACE | SSD_END_ROW|SSD_WS_TABSTOP); 
       text = ssd_text_new ("Note2_text_cont",
            roadmap_lang_get (NOTE2), 14, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE);
       ssd_text_set_color(text,notesColor);
       ssd_widget_add (box, text);
       ssd_widget_set_color (box, NULL, NULL);
       ssd_widget_add (dialog, box);
	  
	  
#ifndef TOUCH_SCREEN
  	   ssd_widget_set_left_softkey_text       ( dialog, roadmap_lang_get("Ok"));
       ssd_widget_set_left_softkey_callback   ( dialog, on_ok_softkey);
#endif
       ssd_dialog_activate (title, NULL);
     }
     
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
    roadmap_log (ROADMAP_DEBUG, "exitting map settings");
	needToNotifyServer = FALSE;   
	updateVisibilityDescriptors(RoadMapConfigDownloadTraffic,"DownloadTraffic");
#if 0   
   updateVisibilityDescriptors(RoadMapConfigDownloadWazers,"DownloadWazers");
   updateVisibilityDescriptors(RoadMapConfigDownloadUserReports,"DownloadReports");
   updateVisibilityDescriptors(RoadMapConfigDownloadTrafficJams,"DownloadAutoTraffic");
   updateVisibilityDescriptors(RoadMapConfigDownloadHouseNumbers,"DownloadHouseNumbers");                         
   updateVisibilityDescriptors(RoadMapConfigDownloadMapProblems,"DownloadMapProblems");
#endif   

   if (needToNotifyServer){
   		OnSettingsChanged_VisabilityGroup(); // notify server of visibilaty settings change
   }
   needToNotifyServer = FALSE;
   roadmap_config_save(TRUE);
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


// update the visibilaty descriptors  ( right now only a general traffic descriptor )
// Need to notify server of any have changed
static void updateVisibilityDescriptors(RoadMapConfigDescriptor descriptor,const char * labelName){
	const char * data = (const char *)ssd_dialog_get_data(labelName);
	if (!(roadmap_config_match(&descriptor, data))){ // descriptor changed
		 roadmap_config_set (&descriptor,data);
		 needToNotifyServer = TRUE;
	}
}


// linked to the show wazers on map in map settings, since it doesn't make sense
// to download wazers if you don't show them ( right now at least  ). 
BOOL roadmap_download_settings_isDownloadWazers(){
	return roadmap_map_settings_isShowWazers(); 
}

BOOL roadmap_download_settings_isDownloadReports(){
	return roadmap_download_settings_isEnabled(RoadMapConfigDownloadTraffic);
}

BOOL roadmap_download_settings_isDownloadTraffic(){
	return roadmap_download_settings_isEnabled(RoadMapConfigDownloadTraffic);
}

void roadmap_download_settings_setDownloadTraffic(BOOL is_enabled){
   int i = (is_enabled ? 0 : 1);
   
   if (!(is_enabled && roadmap_download_settings_isDownloadTraffic())){ // descriptor changed
      roadmap_config_set (&RoadMapConfigDownloadTraffic,yesno[i]);
      roadmap_config_save(TRUE);
      OnSettingsChanged_VisabilityGroup(); // notify server of visibilaty settings change
	}
}