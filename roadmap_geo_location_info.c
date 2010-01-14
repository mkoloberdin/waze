/* roadmap_geo_location_info.c
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


#include <stdlib.h>
#include <string.h>

#include "roadmap.h"
#include "roadmap_res.h" 
#include "roadmap_canvas.h"
#include "roadmap_config.h" 
#include "roadmap_lang.h"
#include "roadmap_geo_location_info.h"

#include "ssd/ssd_widget.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_bitmap.h"
#include "ssd/ssd_progress.h"
#include "ssd/ssd_button.h"
#include "ssd/ssd_text.h"

static RoadMapConfigDescriptor GEO_LOCATION_DISPLAYED_Var = 
                           ROADMAP_CONFIG_ITEM( 
                                    GEO_LOCATION_TAB, 
                                    GEO_LOCATION_DISPLAYED_Name);

static RoadMapConfigDescriptor GEO_LOCATION_ENABLED_Var = 
                           ROADMAP_CONFIG_ITEM( 
                                    GEO_LOCATION_TAB, 
                                    GEO_LOCATION_ENABLED_Name);


typedef struct tag_geo_location_info
{
   char    metroplolitan[128];
   char    state[50];
   char    map_score[50];
   char    traffic_score[50];
   char    usage_score[50];
   int     overall_score;

}  geo_location_info, *geo_location_info_ptr;


static geo_location_info g_geo_info; 

/////////////////////////////////////////////////////////////////////
static BOOL is_geo_location_enabled(void){
   roadmap_config_declare_enumeration( GEO_LOCATION_ENABLE_CONFIG_TYPE, 
                                       &GEO_LOCATION_ENABLED_Var, 
                                       NULL, 
                                       GEO_LOCATION_No, 
                                       GEO_LOCATION_Yes, 
                                       NULL);
   if( 0 == strcmp( roadmap_config_get( &GEO_LOCATION_ENABLED_Var), GEO_LOCATION_Yes))
      return TRUE;
   return FALSE;
   
}


/////////////////////////////////////////////////////////////////////
static BOOL geo_location_displayed(void){
   roadmap_config_declare_enumeration( GEO_LOCATION_CONFIG_TYPE, 
                                       &GEO_LOCATION_DISPLAYED_Var, 
                                       NULL, 
                                       GEO_LOCATION_No, 
                                       GEO_LOCATION_Yes, 
                                       NULL);
   if( 0 == strcmp( roadmap_config_get( &GEO_LOCATION_DISPLAYED_Var), GEO_LOCATION_Yes))
      return TRUE;
   return FALSE;
}

/////////////////////////////////////////////////////////////////////
static void set_geo_location_displayed(){
   roadmap_config_set(&GEO_LOCATION_DISPLAYED_Var, GEO_LOCATION_Yes);
   roadmap_config_save(TRUE);
}

/////////////////////////////////////////////////////////////////////
static SsdWidget space(int height){
   SsdWidget space;
   space = ssd_container_new ("spacer", NULL, SSD_MAX_SIZE, height, SSD_WIDGET_SPACE|SSD_END_ROW);
   ssd_widget_set_color (space, NULL,NULL);
   return space;
}

/////////////////////////////////////////////////////////////////////
static int button_callback (SsdWidget widget, const char *new_value) {

   set_geo_location_displayed();
   ssd_dialog_hide_current(dec_ok);

   return 1;
}

/////////////////////////////////////////////////////////////////////
void roadmap_geo_location_set_metropolitan(const char *metro){
   if (!metro)
      return;
   strcpy(g_geo_info.metroplolitan, metro);
}

/////////////////////////////////////////////////////////////////////
void roadmap_geo_location_set_map_score(int map_score){
   sprintf(g_geo_info.map_score, "%d%%", map_score);
}

/////////////////////////////////////////////////////////////////////
void roadmap_geo_location_set_map_score_str(const char* map_score_str){
   strncpy_safe(g_geo_info.map_score,map_score_str, sizeof(g_geo_info.map_score));
}

/////////////////////////////////////////////////////////////////////
void roadmap_geo_location_set_state(const char *state){
   if (!state)
      return;
   strcpy(g_geo_info.state, state);
}


/////////////////////////////////////////////////////////////////////
void roadmap_geo_location_set_traffic_score(int traffic_score){

   sprintf(g_geo_info.traffic_score,"%d%%", traffic_score);
}

/////////////////////////////////////////////////////////////////////
void roadmap_geo_location_set_traffic_score_str(const char* traffic_score_str){

   strncpy_safe(g_geo_info.traffic_score, traffic_score_str, sizeof(g_geo_info.traffic_score));
}

/////////////////////////////////////////////////////////////////////
void roadmap_geo_location_set_usage_score(int usage_score){
   sprintf(g_geo_info.usage_score,"%d%%", usage_score);
}

/////////////////////////////////////////////////////////////////////
void roadmap_geo_location_set_usage_score_str(const char* usage_score_str){
   strncpy_safe(g_geo_info.usage_score, usage_score_str, sizeof(g_geo_info.usage_score));
}

/////////////////////////////////////////////////////////////////////
void roadmap_geo_location_set_overall_score(int overall_score){
   g_geo_info.overall_score = overall_score;
}

#ifndef TOUCH_SCREEN
static int on_softkey(SsdWidget widget, const char *new_value, void *context){
   set_geo_location_displayed();
   ssd_dialog_hide_current(dec_ok);
   return 1;
}
#endif

/////////////////////////////////////////////////////////////////////
static void roadmap_geo_location_dialog(void){
   SsdWidget dialog;
   SsdWidget group;
   SsdWidget progress;
   SsdWidget text;
   SsdWidget container;
   SsdWidget title;
   
   int width = roadmap_canvas_width() - 20;
   dialog = ssd_dialog_new ("geo_location_info",
                            roadmap_lang_get ("waze status in your area"),
                            NULL,
                            SSD_CONTAINER_TITLE|SSD_DIALOG_NO_BACK);
   
   group = ssd_container_new ("geo_location_group", NULL,
            width, SSD_MIN_SIZE,SSD_WIDGET_SPACE|SSD_ALIGN_CENTER|SSD_END_ROW|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE|SSD_CONTAINER_BORDER);
   ssd_widget_set_color (group, NULL,NULL);
   ssd_widget_add (group, space(5));
   text = ssd_text_new ("Metro", g_geo_info.metroplolitan, 28,SSD_ALIGN_CENTER);
   ssd_widget_add (group, text);
   text = ssd_text_new ("Label_label", roadmap_lang_get("Metro"), 28,SSD_ALIGN_CENTER|SSD_END_ROW);
   ssd_widget_add (group, text);
   ssd_widget_add (group, space(5));

   title = ssd_container_new ("title_gr", NULL,
            SSD_MIN_SIZE, SSD_MIN_SIZE,SSD_ALIGN_CENTER|SSD_END_ROW);
   ssd_widget_set_color(title, NULL, NULL);
   
   text = ssd_text_new ("Label_status", roadmap_lang_get("Status:   "), 20, 0);
   ssd_widget_add (title, text);
   text = ssd_text_new ("Status", g_geo_info.state, 20,SSD_END_ROW);
   ssd_widget_set_color(text, "#4886b7", "#4886b7");
   ssd_widget_add (title, text);
   ssd_widget_add (group, title);
   ssd_widget_add (group, space(15));

   text = ssd_text_new ("What you get label", roadmap_lang_get("What you get:"), 18,SSD_ALIGN_CENTER|SSD_END_ROW);
   ssd_widget_add (group, text);
   
   ssd_widget_add (group, space(5));
   text = ssd_text_new ("Map_label", roadmap_lang_get("Navigable map:   "), 14,0);
   ssd_widget_add (group, text);
   text = ssd_text_new ("Map_label_value", g_geo_info.map_score, 14,SSD_END_ROW);
   ssd_widget_set_color(text, "#4886b7", "#4886b7");
   ssd_widget_add (group, text);

   
   ssd_widget_add (group, space(5));
   
   text = ssd_text_new ("Traffic_label", roadmap_lang_get("Real-Time traffic:  "), 14, 0);
   ssd_widget_add (group, text);
   text = ssd_text_new ("Traffic_label_value", g_geo_info.traffic_score, 14,SSD_END_ROW);
   ssd_widget_set_color(text, "#4886b7", "#4886b7");
   ssd_widget_add (group, text);
   ssd_widget_add (group, space(5));
   
   text = ssd_text_new ("Usage_label", roadmap_lang_get("Community alerts:   "), 14,0);
   ssd_widget_add (group, text);
   text = ssd_text_new ("Usage_label", g_geo_info.usage_score, 14,SSD_END_ROW);
   ssd_widget_set_color(text, "#4886b7", "#4886b7");
   ssd_widget_add (group, text);
   ssd_widget_add (group, space(5));
   
   ssd_widget_add (group, space(20));
   progress = ssd_progress_new("progress",SSD_MAX_SIZE, 0);
   ssd_progress_set_value(progress, g_geo_info.overall_score);
   ssd_widget_add(group, progress);
   
   
   //ssd_widget_add (group, space(5));
   
   container = ssd_container_new ("text_cont", NULL,
            80, SSD_MIN_SIZE,SSD_WIDGET_SPACE);
   text = ssd_text_new ("Early building", roadmap_lang_get("Early Building"), -1, SSD_ALIGN_CENTER);
   ssd_widget_add (container, text);
   ssd_widget_add (group, container);
   
   container = ssd_container_new ("text_cont", NULL,
            80, SSD_MIN_SIZE,SSD_WIDGET_SPACE|SSD_ALIGN_CENTER);
   text = ssd_text_new ("Partial Value", roadmap_lang_get("Partial Value"), -1, SSD_ALIGN_CENTER);
   ssd_widget_add (container, text);
   ssd_widget_add (group, container);

   container = ssd_container_new ("text_cont", NULL,
            40, SSD_MIN_SIZE,SSD_WIDGET_SPACE|SSD_END_ROW);
   text = ssd_text_new ("Full Value", roadmap_lang_get("Full Value"), -1, SSD_ALIGN_CENTER);
   ssd_widget_add (container, text);
   ssd_widget_add (group, container);

#ifdef TOUCH_SCREEN   
   ssd_widget_add (group, space(15));
   ssd_widget_add (group,
                   ssd_button_label ("Let me in", roadmap_lang_get ("Let me in"),
                        SSD_WS_TABSTOP|SSD_ALIGN_CENTER|SSD_START_NEW_ROW, button_callback));
#else
   ssd_widget_set_right_softkey_text      ( dialog, roadmap_lang_get("Let me in"));
   ssd_widget_set_right_softkey_callback  ( dialog, on_softkey);

#endif
   ssd_widget_add (group, space(5));
   ssd_widget_add( dialog, group);
   ssd_dialog_activate ("geo_location_info", NULL);
}

/////////////////////////////////////////////////////////////////////
void roadmap_geo_location_info(void){
   if (!is_geo_location_enabled())
       return;
    
  if (geo_location_displayed())
      return;
   
  roadmap_geo_location_dialog();
}
