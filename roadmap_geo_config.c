/* roadmap_geo_config.c
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
#include "roadmap_main.h"
#include "roadmap_gps.h"
#include "roadmap_config.h"
#include "roadmap_screen.h"
#include "roadmap_lang.h"
#include "roadmap_messagebox.h"
#include "roadmap_geo_config.h"
#include "roadmap_trip.h"
#include "roadmap_prompts.h"
#include "roadmap_splash.h"
#include "roadmap_tile_storage.h"
#include "roadmap_locator.h"
#include "Realtime/Realtime.h"
#include "ssd/ssd_progress_msg_dialog.h"
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_separator.h"
#include "roadmap_general_settings.h"
#include "websvc_trans/websvc_trans.h"
#ifdef IPHONE
#include "iphone/roadmap_location.h"
#endif //IPHONE

#define SYSTEM_DEFAULT_ID ("-1")

#define REQUEST_TIMEOUT     120000
#define RETRY_TIMEOUT       5000
/* TODO:: Check if 3 retries are enough for all the platforms *** AGA *** */
#if defined(ANDROID) || defined(__SYMBIAN32__) || defined (_WIN32)
#define MAX_LOCATION_RETIES 3
#define MAX_NET_RETRIES    3
#else
#define MAX_LOCATION_RETIES 3
#define MAX_NET_RETRIES    3
#endif

static RoadMapConfigDescriptor RoadMapConfigSystemServerId =
      ROADMAP_CONFIG_ITEM("System", "ServerId");

static RoadMapConfigDescriptor RoadMapConfigGeoConfigVersion =
      ROADMAP_CONFIG_ITEM("GeoConfig", "version");

static RoadMapConfigDescriptor   RoadMapConfigWebServiceName   =
            ROADMAP_CONFIG_ITEM("GeoConfig", "Web-Service Address");

static RoadMapConfigDescriptor   RoadMapConfigForceLocation   =
            ROADMAP_CONFIG_ITEM("GeoConfig", "Force Location");

static RoadMapConfigDescriptor   RoadMapConfigLastPosition   =
            ROADMAP_CONFIG_ITEM("GPS", "Position");

static wst_handle  s_websvc = INVALID_WEBSVC_HANDLE;
static BOOL initialized = FALSE;

static void lang_dlg(void);
static void lang_loaded (void);
static BOOL request_geo_config (void);
void roadmap_geo_config_fixed_location(RoadMapPosition *gpsPosition, const char *name, RoadMapCallback callback);


typedef struct {
   int id;
   char name[100];
   int num_results;
   int num_received;
   char lang[6];
   int  version;
   RoadMapCallback callback;
} NavigateRoutingContext;

static NavigateRoutingContext GeoConfigContext;


////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////
static void init_context (void) {
   GeoConfigContext.num_results = 0;
   GeoConfigContext.num_received = 0;
   GeoConfigContext.name[0] = 0;
}

////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////
static const char* get_webservice_address(void)
{
   return roadmap_config_get( &RoadMapConfigWebServiceName);
}

////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////
static RoadMapPosition *get_session_position(){
   static RoadMapPosition pos;
   roadmap_config_get_position(&RoadMapConfigLastPosition, &pos);
   if ( IS_DEFAULT_LOCATION( &pos ) )
      return NULL;
   else
      return &pos;
}

////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////
static const char* get_force_location(void)
{
   return roadmap_config_get( &RoadMapConfigForceLocation);
}


////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////
static void clean_up (void) {

   if( INVALID_WEBSVC_HANDLE != s_websvc){
      wst_term( s_websvc);
      s_websvc = INVALID_WEBSVC_HANDLE;
   }

   init_context ();
}


////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////
static void parse_string ( char* str ) {

	char spacer = '#';
	char *p = str;
	/* Parse for spaces */
	while( ( p = strchr( p, spacer ) ) )
	{
		*p = ' ';
		p++;
	}
}

////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////
void GeoConfigTimer (void) {
   roadmap_log (ROADMAP_ERROR,"GeoServerConfig Timeout!! received %d out of %d", GeoConfigContext.num_received+1, GeoConfigContext.num_results );
   ssd_progress_msg_dialog_hide();
   roadmap_messagebox ("Oops",
            "Cannot configure service. Please try again later");
   clean_up();
   roadmap_screen_refresh();

   //Remove timer
   roadmap_main_remove_periodic (GeoConfigTimer);

   if (GeoConfigContext.callback)
      (*GeoConfigContext.callback)();

}


   
////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////
void roadmap_geo_config_transaction_failed(void){
   ssd_progress_msg_dialog_hide();
   roadmap_messagebox ("Oops",
            "Cannot configure service. Please try again later");
   clean_up();

   //Remove timer
   roadmap_main_remove_periodic (GeoConfigTimer);

   roadmap_screen_refresh();
   if (GeoConfigContext.callback)
      (*GeoConfigContext.callback)();
}


////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////
static void on_lang_conf_downloaded(void){

   ssd_progress_msg_dialog_hide();
   clean_up();
#ifndef IPHONE_NATIVE
   lang_dlg();
#else
   roadmap_general_settings_show_lang_initial(lang_loaded);
#endif // !IPHONE_NATIVE

}

static void on_user_lang_downloaded(void){
   clean_up();
   ssd_progress_msg_dialog_hide();
   roadmap_screen_redraw();

   if (GeoConfigContext.callback)
       (*GeoConfigContext.callback)();
   GeoConfigContext.callback = NULL;
}

////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////
static void restart_msg_cb (int exit_code) {
   roadmap_main_exit();
}

////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////
static void on_recieved_completed (void) {
   char updateText[] = "We've made a few infrastructural changes that require re-start. Please exit and re-start waze.";
   const char *user_lang = roadmap_lang_get_user_lang();

   // compare the old and new server id's - if different, user needs to restart
   int oldServerId = roadmap_config_get_integer(&RoadMapConfigSystemServerId);
   int newServerId = GeoConfigContext.id;

   // Save the RT server ID.
   roadmap_config_set_integer (&RoadMapConfigSystemServerId,
            GeoConfigContext.id);

   //save default language
   roadmap_lang_set_default_lang (GeoConfigContext.lang);


   // Save version
   roadmap_config_set_integer (&RoadMapConfigGeoConfigVersion,
            GeoConfigContext.version);

   //Save new configuration
   roadmap_config_save (TRUE);

   //Remove timer
   roadmap_main_remove_periodic (GeoConfigTimer);

   roadmap_log (ROADMAP_DEBUG,"GeoServerConfig Completed setting all parameters!!" );

   if ((user_lang[0] == 0) && (newServerId != 2)){
      roadmap_lang_download_conf_file(on_lang_conf_downloaded);
      return;
   }

   if ((user_lang[0] == 0) && (newServerId == 2)){
      roadmap_lang_set_system_lang(GeoConfigContext.lang, FALSE);
   }

   if ((oldServerId==-1)){
      ssd_progress_msg_dialog_show("Downloading language");
      roadmap_lang_download_lang_file(roadmap_lang_get_system_lang(), on_user_lang_downloaded);
      return;
   }


   roadmap_lang_download_lang_file(GeoConfigContext.lang, NULL);

   ssd_progress_msg_dialog_hide();

   clean_up();

   roadmap_screen_refresh();

   if (GeoConfigContext.callback)
      (*GeoConfigContext.callback)();

   GeoConfigContext.callback = NULL;
   if ((oldServerId!=newServerId)&&(oldServerId!=-1)){
      roadmap_lang_set_update_time("");
      roadmap_lang_set_lang_file_update_time("heb","");
      roadmap_lang_set_lang_file_update_time("eng","");
      roadmap_prompts_set_update_time ("");
      roadmap_splash_set_update_time ("");
      roadmap_splash_reset_check_time();
      roadmap_config_save(FALSE);
#if (defined (IPHONE) || defined (ANDROID))
      roadmap_tile_remove_all(roadmap_locator_active());
#endif
      roadmap_messagebox_cb(roadmap_lang_get("Please restart Waze"), roadmap_lang_get(updateText), restart_msg_cb);
   }
}

////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////
const char *on_geo_server_config (/* IN  */const char* data,
/* IN  */void* context,
/* OUT */BOOL* more_data_needed,
/* OUT */roadmap_result* rc) {

   int size;
   // Default error for early exit:
   (*rc) = err_parser_unexpected_data;

   init_context ();

   // Expected data:
   //   <id> <name> <RTServerID> <lang> <num_parameters><version>

   // ID
   data = ReadIntFromString (data, //   [in]      Source string
            ",", //   [in,opt]  Value termination
            NULL, //   [in,opt]  Allowed padding
            &GeoConfigContext.id, //   [out]     Output value
            1); //   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

   if (!data) {
      roadmap_log (ROADMAP_ERROR,"on_geo_server_config() - Failed to read 'ID'" );
      return NULL;
   }

   // Name
   size = sizeof (GeoConfigContext.name);
   data = ExtractString (data, GeoConfigContext.name, &size, ",", 1);
   if (!data) {
       roadmap_log (ROADMAP_ERROR, "on_geo_server_config() - Failed to read 'name'");
       return NULL;
   }


   // lang
   size = sizeof (GeoConfigContext.lang);
   data = ExtractString (data, GeoConfigContext.lang, &size, ",", 1);
   if (!data) {
       roadmap_log (ROADMAP_ERROR, "on_geo_server_config() - Failed to read 'lang'");
       return NULL;
   }



   // num_paramerets
   data = ReadIntFromString(  data,                            //   [in]      Source string
                               ",",                        //   [in,opt]  Value termination
                               NULL,                           //   [in,opt]  Allowed padding
                               &GeoConfigContext.num_results,  //   [out]     Output value
                               1);                //   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

   if (!data) {
       roadmap_log (ROADMAP_ERROR, "on_geo_server_config() - Failed to read 'num_parameters'");
       return NULL;
   }


   // version
   data = ReadIntFromString(  data,                            //   [in]      Source string
                               ",\r\n",                        //   [in,opt]  Value termination
                               NULL,                           //   [in,opt]  Allowed padding
                               &GeoConfigContext.version,  //   [out]     Output value
                               TRIM_ALL_CHARS);                //   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

   if (!data) {
       roadmap_log (ROADMAP_ERROR, "on_geo_server_config() - Failed to read 'version'");
       return NULL;
   }

   roadmap_log (ROADMAP_DEBUG, "got GeoServerConfig message - Id=%d, name=%s lang=%s, num_parameters=%d version=%d",GeoConfigContext.id, GeoConfigContext.name, GeoConfigContext.lang, GeoConfigContext.num_results, GeoConfigContext.version );
   if (GeoConfigContext.num_results == 0){
      roadmap_log (ROADMAP_DEBUG, "GeoServerConfig, No Parameters passed. Completing... " );
      on_recieved_completed();
   }

   (*rc) = succeeded;
   return data;
}

////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////
const char *on_server_config (/* IN  */const char* data,
/* IN  */void* context,
/* OUT */BOOL* more_data_needed,
/* OUT */roadmap_result* rc) {

   int serial;
   int size;
   char category[256];
   char key[256];
   char value[256];
   RoadMapConfigDescriptor param;
   // Default error for early exit:
   (*rc) = err_parser_unexpected_data;

   // Expected data:
   // <serial> <category> <key> <value>

   // Serial
   data = ReadIntFromString (data, //   [in]      Source string
            ",", //   [in,opt]  Value termination
            NULL, //   [in,opt]  Allowed padding
            &serial, //   [out]     Output value
            1); //   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

   if (!data) {
      roadmap_log (ROADMAP_ERROR,"on_serve_config() - Failed to read 'serial'" );
      return NULL;
   }

   // Category
   size = sizeof (category);
   data = ExtractString (data, category, &size, ",", 1);
   if (!data) {
      roadmap_log (ROADMAP_ERROR, "on_geo_server_config() - Failed to read 'category' serial = %d", serial);
      return NULL;
   }

   // Key
   size = sizeof (key);
   data = ExtractString (data, key, &size, ",", 1);
   if (!data) {
      roadmap_log (ROADMAP_ERROR, "on_geo_server_config() - Failed to read 'key' serial = %d", serial);
      return NULL;
   }

   // Value
   size = sizeof (value);
   data = ExtractString (data, value, &size, ",\r\n", TRIM_ALL_CHARS);
   if (!data) {
      roadmap_log (ROADMAP_ERROR, "on_geo_server_config() - Failed to read 'value' serial = %d", serial);
      return NULL;
   }

   (*rc) = succeeded;
   roadmap_log (ROADMAP_INFO, "got ServerConfig message - serial=%d out of %d, category=%s, key=%s, value=%s",serial+1, GeoConfigContext.num_results, category, key, value );

   GeoConfigContext.num_received++;

   param.category = strdup(category);
   param.name = strdup(key);
   roadmap_config_declare("preferences",&param, "", NULL);
   parse_string( value );
   roadmap_config_set(&param,value);

   if (GeoConfigContext.num_received == GeoConfigContext.num_results) {
      roadmap_log (ROADMAP_INFO, "GeoServerConfig, Got all results... " );
      on_recieved_completed();
   }

   (*rc) = succeeded;
   return data;
}

////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////
const char *on_update_config (/* IN  */const char* data,
								/* IN  */void* context,
								/* OUT */BOOL* more_data_needed,
								/* OUT */roadmap_result* rc) {

   int size;
   char file[256];
   char category[256];
   char key[256];
   char value[512];
   // Default error for early exit:
   (*rc) = err_parser_unexpected_data;

   // Expected data:
   // <file>, <category>, <key>, <value>


   // File
   size = sizeof ( file );
   data = ExtractString( data, file, &size, ",", 1 );
   if (!data)
   {
      roadmap_log (ROADMAP_ERROR, "on_update_config() - Failed to read 'file'" );
      return NULL;
   }

   // Category
   size = sizeof ( category );
   data = ExtractString( data, category, &size, ",", 1 );
   if ( !data ) {
      roadmap_log (ROADMAP_ERROR, "on_update_config() - Failed to read 'category'");
      return NULL;
   }

   // Key
   size = sizeof ( key );
   data = ExtractString ( data, key, &size, ",", 1 );
   if (!data) {
      roadmap_log (ROADMAP_ERROR, "on_update_config() - Failed to read 'key' " );
      return NULL;
   }

   // Value
   size = sizeof (value);
   data = ExtractString ( data, value, &size, ",\r\n", TRIM_ALL_CHARS );
   if (!data)
   {
      roadmap_log (ROADMAP_ERROR, "on_update_config() - Failed to read 'value' " );
      return NULL;
   }

   (*rc) = succeeded;
   roadmap_log (ROADMAP_INFO, "Successfully got UpdateConfig message - file %s, category=%s, key=%s, value=%s", file, category, key, value );

   /* Set the configuration value */
   {
	   RoadMapConfigDescriptor cfgDesc ;
	   cfgDesc.category = strdup( category);
	   cfgDesc.name  = strdup( key );
	   roadmap_config_declare( file, &cfgDesc, "", NULL );
	   parse_string( value );
	   roadmap_config_set( &cfgDesc, value );
   }
   roadmap_config_save(0);
   return data;
}


////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////
static void retry(void){
   if (!request_geo_config())
      ssd_progress_msg_dialog_hide();
}

////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////
static int on_pointer_down( SsdWidget this, const RoadMapGuiPoint *point)
{

   ssd_widget_pointer_down_force_click(this, point );

   if( !this->tab_stop)
      return 0;

   if( !this->in_focus)
      ssd_dialog_set_focus( this);
   ssd_dialog_draw();
   return 0;
}

////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////
static int lang_on_pointer_down( SsdWidget this, const RoadMapGuiPoint *point)
{

   ssd_widget_pointer_down_force_click(this, point );
   return 0;
}

////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////
static int usa_callback (SsdWidget widget, const char *new_value) {

   ssd_dialog_hide ("Select Country Dialog", dec_ok);
   roadmap_geo_config_usa(GeoConfigContext.callback);
   return 0;
}

////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////
static int il_callback (SsdWidget widget, const char *new_value) {

   roadmap_geo_config_il(GeoConfigContext.callback);
   ssd_dialog_hide ("Select Country Dialog", dec_ok);
   return 0;
}

////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////
static int other_callback (SsdWidget widget, const char *new_value) {

   roadmap_geo_config_other(GeoConfigContext.callback);
   ssd_dialog_hide ("Select Country Dialog", dec_ok);
   return 0;
}

////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////
static void retreis_exhausted(void){
   SsdWidget dialog;
   SsdWidget space;
   SsdWidget container;
   SsdWidget box;
   SsdWidget text;
   int       height = 45;

   if ( roadmap_screen_is_hd_screen() )
   {
	   height = 90;
   }

   roadmap_log (ROADMAP_ERROR,"RoadmapGeoConfig - Retries exhausted." );
   roadmap_main_remove_periodic(retry);

   dialog = ssd_dialog_new ("Select Country Dialog", "Select country", NULL,
          SSD_CONTAINER_BORDER|SSD_DIALOG_FLOAT|
          SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_BLACK);


    ssd_widget_set_color (dialog, "#000000", "#ff0000000");

    space = ssd_container_new ("spacer2", NULL, SSD_MAX_SIZE, 10, SSD_END_ROW);
    ssd_widget_set_color(space, NULL, NULL);
    ssd_widget_add (dialog, space);

    text = ssd_text_new("SelectCountry Text",roadmap_lang_get("Cannot determine location. Please select your location."),-1, SSD_END_ROW);
    ssd_text_set_color(text, "#ffffff");
    ssd_widget_add(dialog, text);

    space = ssd_container_new ("spacer2", NULL, SSD_MAX_SIZE, 10, SSD_END_ROW);
    ssd_widget_set_color(space, NULL, NULL);
    ssd_widget_add (dialog, space);

    container = ssd_container_new("SelectCountryContainer", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE|SSD_CONTAINER_BORDER);

    box = ssd_container_new ("USA-Box", NULL, SSD_MAX_SIZE, height, SSD_END_ROW|SSD_WS_TABSTOP);
    ssd_widget_set_color(box, NULL, NULL);
    ssd_widget_add(box, ssd_text_new ("USA-TXT", roadmap_lang_get("USA & Canada"), 16, SSD_END_ROW|SSD_WIDGET_SPACE|SSD_ALIGN_VCENTER|SSD_ALIGN_CENTER));
    ssd_widget_add(box, ssd_separator_new("Separator",SSD_ALIGN_BOTTOM));
    box->callback = usa_callback ;
    ssd_widget_set_pointer_force_click( box );
    box->pointer_down = on_pointer_down;
    ssd_widget_add(container, box);

    box = ssd_container_new ("IL-Box", NULL, SSD_MAX_SIZE, height, SSD_END_ROW|SSD_WS_TABSTOP);
    ssd_widget_set_color(box, NULL, NULL);
    ssd_widget_add(box, ssd_text_new ("IL-TXT", roadmap_lang_get("ישראל"), 16, SSD_END_ROW|SSD_WIDGET_SPACE|SSD_ALIGN_VCENTER|SSD_ALIGN_CENTER));
    ssd_widget_add(box, ssd_separator_new("Separator",SSD_ALIGN_BOTTOM));
    box->callback = il_callback ;
    ssd_widget_set_pointer_force_click( box );
    box->pointer_down = on_pointer_down;
    ssd_widget_add(container, box);

    box = ssd_container_new ("Other-Box", NULL, SSD_MAX_SIZE, height, SSD_END_ROW|SSD_WS_TABSTOP);
    ssd_widget_set_color(box, NULL, NULL);
#ifndef IPHONE
    ssd_widget_add(box, ssd_text_new ("Other-TXT", roadmap_lang_get("International"), 16, SSD_END_ROW|SSD_WIDGET_SPACE|SSD_ALIGN_VCENTER|SSD_ALIGN_CENTER));
#else
   ssd_widget_add(box, ssd_text_new ("Other-TXT", roadmap_lang_get("International"), 16, SSD_END_ROW|SSD_WIDGET_SPACE|SSD_ALIGN_VCENTER|SSD_ALIGN_CENTER));
#endif //IPHONE
    box->callback = other_callback ;
    ssd_widget_set_pointer_force_click( box );
    box->pointer_down = on_pointer_down;
    ssd_widget_add(container, box);

    ssd_widget_add(dialog, container);
    ssd_dialog_activate("Select Country Dialog", NULL);
    if ( !roadmap_screen_refresh() )
    	roadmap_screen_redraw();
}

static void lang_loaded (void) {
   ssd_progress_msg_dialog_hide();
   roadmap_screen_refresh();

   if (GeoConfigContext.callback)
       (*GeoConfigContext.callback)();
   download_lang_files();
   GeoConfigContext.callback = NULL;

}

////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////
static void lang_selected(void){
   roadmap_main_remove_periodic(lang_selected);
   ssd_dialog_hide ("Select Language Dialog", dec_ok);
   ssd_progress_msg_dialog_show("Downloading language...");
   roadmap_lang_download_lang_file(roadmap_lang_get_system_lang(), lang_loaded);
}

////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////
static int lang_callback (SsdWidget widget, const char *new_value) {

   const char *value = (const char *)widget->context;
   if (!value)
      return 0;

   roadmap_lang_set_system_lang(value, FALSE);
   ssd_dialog_set_focus(widget);
   roadmap_screen_redraw ();
   roadmap_main_set_periodic (300, lang_selected);
   return 1;

}

////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////
static void lang_dlg(void){
   SsdWidget dialog, container,box, text, space;
   int i;
   int       height = 45;
   const void ** lang_values = roadmap_lang_get_available_langs_values();
   const char ** lang_labels = roadmap_lang_get_available_langs_labels();
   int lang_count = roadmap_lang_get_available_langs_count();

   if ( roadmap_screen_is_hd_screen() )
   {
	   height = 90;
   }

   if (lang_count <= 1){
      roadmap_screen_refresh();

      if (GeoConfigContext.callback)
          (*GeoConfigContext.callback)();

      GeoConfigContext.callback = NULL;
      return ;
   }
   dialog = ssd_dialog_new ("Select Language Dialog", "Select Language", NULL,
            SSD_CONTAINER_BORDER| SSD_CONTAINER_TITLE|
            SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER);

   space = ssd_container_new ("spacer2", NULL, SSD_MAX_SIZE, 10, SSD_END_ROW);
   ssd_widget_set_color(space, NULL, NULL);
   ssd_widget_add (dialog, space);

   text = ssd_text_new("SelectLanguage Text","Please select language:",16, SSD_END_ROW|SSD_ALIGN_CENTER);
   ssd_text_set_color(text, "#000000");
   ssd_widget_add(dialog, text);

   space = ssd_container_new ("spacer2", NULL, SSD_MAX_SIZE, 10, SSD_END_ROW);
   ssd_widget_set_color(space, NULL, NULL);
   ssd_widget_add (dialog, space);

   container = ssd_container_new("SelectLangRow", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE|SSD_CONTAINER_BORDER);
   for (i = 0; i< lang_count; i++){
      box = ssd_container_new ("Lang-Box", NULL, SSD_MAX_SIZE, height, SSD_END_ROW|SSD_WS_TABSTOP);
      ssd_widget_set_color(box, NULL, NULL);
      ssd_widget_add(box, ssd_text_new ("LANG-TXT", lang_labels[i], 16, SSD_END_ROW|SSD_WIDGET_SPACE|SSD_ALIGN_VCENTER|SSD_ALIGN_CENTER));
      if (i != lang_count -1)
         ssd_widget_add(box, ssd_separator_new("Separator",SSD_ALIGN_BOTTOM));
      box->callback = lang_callback ;
      box->context = (void *)lang_values[i];
      ssd_widget_set_pointer_force_click( box );
      box->pointer_down = lang_on_pointer_down;
      ssd_widget_add(container, box);
   }
   ssd_widget_add(dialog, container);
   ssd_dialog_activate("Select Language Dialog", NULL);
    if ( !roadmap_screen_refresh() )
      roadmap_screen_redraw();

}
////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////
static BOOL request_geo_config (void) {

  static int retries = 0;
   static int net_retries = 0;

  BOOL has_reception = (roadmap_gps_reception_state () != GPS_RECEPTION_NONE)
            && (roadmap_gps_reception_state () != GPS_RECEPTION_NA);
   const RoadMapPosition *Location;

   roadmap_log (ROADMAP_INFO,"Requesting Geo Configuration" );

#ifdef IPHONE
   if (roadmap_location_denied ()) {
      roadmap_log (ROADMAP_ERROR,"RoadmapGeoConfig - Location denied");
      retreis_exhausted();
      return FALSE;
   }
#endif //IPHONE

   Location = get_session_position();
   if (Location == NULL){
      if (has_reception)
         Location = roadmap_trip_get_position ("GPS");
      else
         Location = roadmap_trip_get_position( "Location" );

      if ( (Location == NULL) || IS_DEFAULT_LOCATION( Location ) ){
         retries++;
         if (retries == MAX_LOCATION_RETIES){
            retreis_exhausted();
            return FALSE;
         }

         roadmap_log (ROADMAP_ERROR,"RoadmapGeoConfig - Location not found. Going to retry num %d in %d seconds.", retries, (int)(RETRY_TIMEOUT/1000) );
         if (retries == 1)
            roadmap_main_set_periodic(RETRY_TIMEOUT, retry);
         return TRUE;
      }
   }
   else{
      roadmap_log (ROADMAP_INFO,"RoadmapGeoConfig - Using location from session! (lat=%d, lon=%d)", Location->latitude, Location->longitude);
   }

   if (retries > 0) {
      roadmap_main_remove_periodic(retry);
      retries = 0;
   }


   if(!Realtime_GetGeoConfig(Location,"", s_websvc)){
      net_retries++;
      if (net_retries == MAX_NET_RETRIES) {
         roadmap_main_remove_periodic(retry);
         roadmap_log (ROADMAP_ERROR,"Failed to send GetGeoConfig request" );
         roadmap_messagebox("Oops","Failed to initialize. No network connection");
         roadmap_main_remove_periodic (GeoConfigTimer);
         if (GeoConfigContext.callback)
            (*GeoConfigContext.callback)();

         return FALSE;
      } else {
         roadmap_log (ROADMAP_ERROR,"RoadmapGeoConfig - Network connection problem. Going to retry num %d in %d seconds.", net_retries, (int)(RETRY_TIMEOUT/1000) );
         if (net_retries == 1)
            roadmap_main_set_periodic(RETRY_TIMEOUT, retry);
         return TRUE;
      }
   }
   else{
      if (net_retries > 0)
         roadmap_main_remove_periodic(retry);
      return TRUE;
   }

}

////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////
static BOOL need_to_ask_server (void) {
   const char *ServerId = roadmap_config_get (&RoadMapConfigSystemServerId);
   if (0 == strcmp (ServerId,SYSTEM_DEFAULT_ID))
      return TRUE;
   return FALSE;
}

const char *roadmap_geo_config_get_version(void){
   return roadmap_config_get( &RoadMapConfigGeoConfigVersion);
}
////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////
static void roadmap_geo_config_init (void) {
   const char* address;

   init_context ();

   if (!initialized)
   {
      //   Web-service address
      roadmap_config_declare( "preferences",
                              &RoadMapConfigWebServiceName,
                              "",
                              NULL);
      roadmap_config_declare ("preferences",
                              &RoadMapConfigSystemServerId,
                              SYSTEM_DEFAULT_ID, NULL);

      roadmap_config_declare ("preferences",
                              &RoadMapConfigGeoConfigVersion,
                              "0", NULL);

      roadmap_config_declare ("preferences",
                              &RoadMapConfigForceLocation,
                              "", NULL);

      roadmap_config_declare ("session",
                              &RoadMapConfigLastPosition,
                              "", NULL);
      initialized = TRUE;
   }

   address = get_webservice_address();
   if (INVALID_WEBSVC_HANDLE == s_websvc)
      s_websvc = wst_init(address, NULL, NULL, "application/x-www-form-urlencoded; charset=utf-8");

   if( INVALID_WEBSVC_HANDLE != s_websvc)
   {
      roadmap_log(ROADMAP_DEBUG,
                  "roadmap_geo_config_init() - Web-Service Address: '%s'",
                  address);
      return;
   }

   roadmap_log(ROADMAP_ERROR, "address_search_init() - 'wst_init()' failed");
}

#ifdef _WIN32
static void after_detect_reciever(void){
	roadmap_geo_config(GeoConfigContext.callback);
}
#endif
////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////
void roadmap_geo_config (RoadMapCallback callback) {

   const char *force_location;
   static int has_run;
   GeoConfigContext.callback = callback;
#if (defined(_WIN32) && !defined(__SYMBIAN32__))
   if (!has_run){
	   has_run = TRUE;
	   roadmap_gps_detect_receiver_callback(after_detect_reciever);
	   return;
   }
#endif
   roadmap_geo_config_init ();

   force_location = get_force_location();
   if (need_to_ask_server () && *force_location){
      roadmap_lang_set_update_time("");
      roadmap_prompts_set_update_time ("");
      roadmap_lang_set_lang_file_update_time("heb","");
      roadmap_lang_set_lang_file_update_time("eng","");

      roadmap_screen_redraw ();
      if (!strcmp(force_location, "il")){
         roadmap_log(ROADMAP_INFO, "roadmap_geo_config - force location set to il");
         roadmap_geo_config_il(callback);
         return;
      }
      else if (!strcmp(force_location, "usa")){
         roadmap_log(ROADMAP_INFO, "roadmap_geo_config - force location set to usa");
         roadmap_geo_config_usa(callback);
         return;
      }
      else{
         roadmap_log(ROADMAP_INFO, "roadmap_geo_config - force location invalide value %s, continuing nromally", force_location);
      }
   }

   if (need_to_ask_server ()) {
      roadmap_lang_set_update_time("");
      roadmap_prompts_set_update_time ("");
      roadmap_lang_set_lang_file_update_time("heb","");
      roadmap_lang_set_lang_file_update_time("eng","");
      roadmap_screen_redraw ();
      if (request_geo_config()) {
         ssd_progress_msg_dialog_show("Initializing, please wait...");
         roadmap_main_set_periodic(REQUEST_TIMEOUT, GeoConfigTimer);
      }
   }
   else{
     if (callback)
      (*callback) ();
     GeoConfigContext.callback = NULL;
   }
}

////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////
static void completed(void){
   //roadmap_messagebox("Info", "Please restart waze");

}

////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////
void roadmap_geo_config_fixed_location(RoadMapPosition *gpsPosition, const char *name, RoadMapCallback callback){
   roadmap_geo_config_init ();
   GeoConfigContext.callback = callback;
   roadmap_log (ROADMAP_INFO,"Requesting Geo Configuration name=%s",name );
   if(!Realtime_GetGeoConfig(gpsPosition,name, s_websvc)){
      roadmap_log (ROADMAP_ERROR,"Failed to sent GetGeoConfig request" );
      roadmap_messagebox("Oops","Failed to initialize. No network connection");
      ssd_progress_msg_dialog_hide();
      return;
   }
   roadmap_main_set_periodic(REQUEST_TIMEOUT, GeoConfigTimer);
}

////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////
void roadmap_geo_config_il(RoadMapCallback callback){
   RoadMapPosition gpsPosition;
   ssd_dialog_hide_all(dec_close);
   ssd_progress_msg_dialog_show("Initializing, please wait...");
   gpsPosition.latitude = 32331226;
   gpsPosition.longitude = 35011466;
   if (callback == NULL)
      callback = completed;
   roadmap_geo_config_fixed_location(&gpsPosition, "israel", callback);
}

////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////
void roadmap_geo_config_usa(RoadMapCallback callback){
    RoadMapPosition gpsPosition;
    ssd_dialog_hide_all(dec_close);
    ssd_progress_msg_dialog_show("Initializing, please wait...");
    gpsPosition.latitude = 37421354;
    gpsPosition.longitude = -122088173;
    if (callback == NULL)
       callback = completed;
    roadmap_geo_config_fixed_location(&gpsPosition, "usa", callback);
}

////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////
void roadmap_geo_config_stg(RoadMapCallback callback){
    RoadMapPosition gpsPosition;
    ssd_dialog_hide_all(dec_close);
    ssd_progress_msg_dialog_show("Initializing, please wait...");
    gpsPosition.latitude = 0;
    gpsPosition.longitude = 0;
    if (callback == NULL)
       callback = completed;
    roadmap_geo_config_fixed_location(&gpsPosition, "stg", callback);
}

////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////
void roadmap_geo_config_generic(char * name){
    RoadMapPosition gpsPosition;
    ssd_dialog_hide_all(dec_close);
    ssd_progress_msg_dialog_show("Initializing, please wait...");
    gpsPosition.latitude = 0;
    gpsPosition.longitude = 0;
    roadmap_geo_config_fixed_location(&gpsPosition, name, completed);
}
////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////
void roadmap_geo_config_other(RoadMapCallback callback){
    RoadMapPosition gpsPosition;
    ssd_dialog_hide_all(dec_close);
    ssd_progress_msg_dialog_show("Initializing, please wait...");
    gpsPosition.latitude = 0;
    gpsPosition.longitude = 0;
    if (callback == NULL)
       callback = completed;
    roadmap_geo_config_fixed_location(&gpsPosition, "world", callback);
}

////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////
const char* roadmap_geo_config_get_server_id(void){
   return roadmap_config_get (&RoadMapConfigSystemServerId);
}
