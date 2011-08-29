/*
 * LICENSE:
 *
 *   Copyright 2010 Avi Ben-Shoshan
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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "../ssd/ssd_container.h"
#include "../ssd/ssd_text.h"
#include "../ssd/ssd_button.h"
#include "../ssd/ssd_list.h"
#include "../ssd/ssd_contextmenu.h"
#include "../ssd/ssd_bitmap.h"
#include "../ssd/ssd_keyboard_dialog.h"
#include "../ssd/ssd_progress_msg_dialog.h"
#include "../roadmap_input_type.h"
#include "../roadmap.h"
#include "../roadmap_lang.h"
#include "../roadmap_messagebox.h"
#include "../roadmap_geocode.h"
#include "../roadmap_trip.h"
#include "../roadmap_history.h"
#include "../roadmap_locator.h"
#include "../roadmap_display.h"
#include "../roadmap_main.h"
#include "../roadmap_gps.h"
#include "../roadmap_geo_config.h"
#include "../roadmap_square.h"
#include "../roadmap_tile_manager.h"
#include "../roadmap_tile_status.h"
#include "../roadmap_map_download.h"
#include "../roadmap_reminder.h"
#include "../roadmap_analytics.h"
#include "../roadmap_gps.h"
#include "address_search.h"
#include "local_search.h"
#include "single_search.h"
#include "generic_search_dlg.h"
#include "../roadmap_start.h"
#include "single_search_dlg.h"
#include "../roadmap_bar.h"
#include "../roadmap_device_events.h"
#include "tts/tts.h"
#include "navigate/navigate_main.h"
#include "roadmap_config.h"
#include "tts_was_provider.h"

#ifdef TOUCH_SCREEN
   #define  USE_ONSCREEN_KEYBOARD
#endif   // TOUCH_SCREEN

#ifdef   USE_ONSCREEN_KEYBOARD
   #include "../ssd/ssd_keyboard.h"
#endif   // USE_ONSCREEN_KEYBOARD

static   SsdWidget            s_result_container = NULL;
static   BOOL                 s_searching = FALSE;
static   BOOL                 s_menu   = FALSE;

#define  SSD_DIALOG_NAME            "single-search-dialog"
#define  SSD_DIALOG_TITLE           "Drive to"

#define  SSD_RESULTS_CONT_NAME      "single-search-dialog.results-container"
#define  SSD_RC_ADDRESSLIST_NAME    "single-search-dialog.results-container.list.address"
#define  SSD_RC_ADDRESSLIST_TITLE_NAME "single-search-dialog.results-container.adr.list.title_text"
#define  SSD_RC_LSLIST_NAME         "single-search-dialog.results-container.list.ls"
#define  SSD_RC_LSLIST_TITLE_NAME   "single-search-dialog.results-container.list.ls.title_text"
#define  SSD_RC_BACK_BTN_NAME       "single-search-dialog.results-container.back-btn"
#define  SSD_RC_BACK_BTN_LABEL      "Back"
#define  COULDNT_FIND_INDEX 1000
#define  COULDNT_FIND_ADDRESS_TEXT    "Couldn't find the address I wanted. Notify waze."

static int g_is_address = TRUE;
static BOOL g_is_favorite = FALSE;
static const char *g_favorite_name;

static void on_search_error_message( int exit_code );
static void search_progress_message_delayed(void);
static int on_addr_list_item_selected(SsdWidget widget, const char *new_value, const void *value);
extern RoadMapConfigDescriptor NavigateConfigNavigationGuidanceType;

typedef enum tag_contextmenu_items
{
   cm_navigate,
   cm_show,
   cm_add_to_favorites,
   cm_cancel,
   cm_add_geo_reminder,
   cm__count,
   cm__invalid
}  contextmenu_items;

// Context menu items:
static ssd_cm_item main_menu_items[] =
{
   //                  Label     ,           Item-ID
   SSD_CM_INIT_ITEM  ( "Drive",              cm_navigate),
   SSD_CM_INIT_ITEM  ( "Show on map",        cm_show),
   SSD_CM_INIT_ITEM  ( "Add to favorites",   cm_add_to_favorites),
   SSD_CM_INIT_ITEM  ( "Add Geo-Reminder",   cm_add_geo_reminder),
   SSD_CM_INIT_ITEM  ( "Cancel",             cm_cancel),
};

// Context menu:
static ssd_contextmenu  context_menu = SSD_CM_INIT_MENU( main_menu_items);

static   char              searched_text[256];

static int on_options(SsdWidget widget, const char *new_value, void *context);


static void update_list(SsdWidget list_cont){
   static   const char* results_adr[ADSR_MAX_RESULTS+1];
    static   void*       indexes_adr[ADSR_MAX_RESULTS+1];
    static   const char* icons_adr[ADSR_MAX_RESULTS+1];
    int      count_adr = 0;
    SsdWidget list_address;
    int       i;
    int size = generic_search_result_count();
    address_candidate*   array = generic_search_result(0);
    /* Close the progress message */
    ssd_progress_msg_dialog_hide();

    roadmap_main_set_cursor( ROADMAP_CURSOR_NORMAL);

    assert(list_cont);

    list_address = ssd_widget_get( list_cont, SSD_RC_ADDRESSLIST_NAME);

    for( i=0; i<size; i++)
    {
       if (array[i].type == ADDRESS_CANDIDATE_TYPE_ADRESS){
          results_adr[count_adr] = array[i].address;
          indexes_adr[count_adr] = (void*)i;
          icons_adr[count_adr] = "search_address";
          count_adr++;
       }
    }

    ssd_list_populate(list_address,
                      count_adr,
                      results_adr,
                      (const void **)indexes_adr,
                      icons_adr,
                      0,
                      on_addr_list_item_selected,
                      NULL,
                      FALSE);

    ssd_dialog_invalidate_tab_order();
    ssd_dialog_resort_tab_order();
    roadmap_screen_refresh();
}


static int get_selected_list_item()
{
   SsdWidget list;
   SsdWidget dlg;

   if( generic_search_dlg_is_1st(single_search))
      return -1;

   dlg = generic_search_dlg_get_search_dlg(single_search);
   if (g_is_address)
      list = ssd_widget_get( dlg, SSD_RC_ADDRESSLIST_NAME);
   else
      list = ssd_widget_get( dlg, SSD_RC_LSLIST_NAME);
   assert(list);

   return (int)ssd_list_selected_value( list);
}

static int get_ls_first_index()
{
   SsdWidget list;
   SsdWidget dlg;

   if( generic_search_dlg_is_1st(single_search))
      return -1;

   dlg = generic_search_dlg_get_search_dlg(single_search);
   list = ssd_widget_get( dlg, SSD_RC_LSLIST_NAME);
   if (!list)
      return 0;

   return (int)ssd_list_row_value( list, 0);
}


static int on_addr_list_item_selected(SsdWidget widget, const char *new_value, const void *value)
{
   if (value == -1){
      widget->in_focus = FALSE;
      widget->force_click = FALSE;
      update_list(widget);
      return 1;
   }
   g_is_address = TRUE;

   if (g_is_favorite)
   {
      if (g_favorite_name){
         int                        selected_list_item   = get_selected_list_item();
         const address_candidate*   selection            = generic_search_result( selected_list_item);
         RoadMapPosition position;
         position.latitude = (int)(selection->latitude*1000000);
         position.longitude= (int)(selection->longtitude*1000000);
         generic_search_add_address_to_history( ADDRESS_FAVORITE_CATEGORY,
                                          selection->city,
                                          selection->street,
                                          get_house_number__str( selection->house),
                                          selection->state,
                                          g_favorite_name,
                                          &position);
         roadmap_search_menu();
         ssd_progress_msg_dialog_show_timed("The address was successfully added to your Favorites",3);
         return 0;
      }
      else{
         int                        selected_list_item   = get_selected_list_item();
         const address_candidate*   selection            = generic_search_result( selected_list_item);
         RoadMapPosition position;
         position.latitude = (int)(selection->latitude*1000000);
         position.longitude= (int)(selection->longtitude*1000000);
         generic_search_add_address_to_history( ADDRESS_FAVORITE_CATEGORY,
                                               selection->city,
                                               selection->street,
                                               get_house_number__str( selection->house),
                                               selection->state,
                                               g_favorite_name,
                                               &position);
         return 0;
      }
   }

   on_options( NULL, NULL, NULL);
   return 0;
}

static int on_ls_list_item_selected(SsdWidget widget, const char *new_value, const void *value)
{
    g_is_address = FALSE;

    if (g_is_favorite)
    {
       int                        selected_list_item   = get_selected_list_item();
       const address_candidate*   selection            = generic_search_result( selected_list_item);
       RoadMapPosition position;
       position.latitude = (int)(selection->latitude*1000000);
       position.longitude= (int)(selection->longtitude*1000000);
       generic_search_add_address_to_history( ADDRESS_FAVORITE_CATEGORY,
                                           selection->city,
                                           selection->street,
                                           get_house_number__str( selection->house),
                                           selection->state,
                                           g_favorite_name,
                                           &position);
       return 1;
    }

    on_options( NULL, NULL, NULL);
   return 0;
}

static void on_address_resolved( void*                context,
                                 address_candidate*   array,
                                 int                  size,
                                 roadmap_result       rc)
{
   static   const char* results_adr[ADSR_MAX_RESULTS+1];
   static   void*       indexes_adr[ADSR_MAX_RESULTS+1];
   static   const char* icons_adr[ADSR_MAX_RESULTS+1];
   static   const char* results_ls[ADSR_MAX_RESULTS+1];
   static   void*       indexes_ls[ADSR_MAX_RESULTS+1];
   static   const char* icons_ls[ADSR_MAX_RESULTS+1];
   int count_adr = 0;
   int count_ls = 0;
   const char* provider_icon = NULL;
   SsdWidget list_cont = (SsdWidget)context;
   SsdWidget list_address;
   SsdWidget list_ls;
   int       i;
   int       adr_index = 0 ;
   int       ls_index = 0;

   s_searching = FALSE;

   /* Close the progress message */
   ssd_progress_msg_dialog_hide();

   roadmap_main_set_cursor( ROADMAP_CURSOR_NORMAL);

   assert(list_cont);

   list_address = ssd_widget_get( list_cont, SSD_RC_ADDRESSLIST_NAME);
   list_ls = ssd_widget_get( list_cont, SSD_RC_LSLIST_NAME);

   if( succeeded != rc)
   {
      if( is_network_error( rc))
         roadmap_messagebox_cb ( roadmap_lang_get( "Oops"),
                              roadmap_lang_get( "Search requires internet connection.\nPlease make sure you are connected."), on_search_error_message );



      else if( err_as_could_not_find_matches == rc)
         roadmap_messagebox_cb ( roadmap_lang_get( "Oops"),
                              roadmap_lang_get( "Sorry, no results were found for this search"), on_search_error_message );
      else
      {
         char msg[128];

         snprintf( msg, sizeof(msg), "%s\n%s",roadmap_lang_get("Sorry we were unable to complete the search"), roadmap_lang_get("Please try again later"));

         roadmap_messagebox_cb ( roadmap_lang_get( "Oops"), msg, on_search_error_message );
      }

      roadmap_log(ROADMAP_ERROR,
                  "single_search_dlg::on_address_resolved() - Resolve process failed with error '%s' (%d)",
                  roadmap_result_string( rc), rc);
      roadmap_analytics_log_event(ANALYTICS_EVENT_SEARCH_FAILED, ANALYTICS_EVENT_INFO_VALUE, searched_text);
      return;
   }

   if( !size)
   {
      roadmap_log(ROADMAP_DEBUG,
                  "single_search_dlg::on_address_resolved() - NO RESULTS for the address-resolve process");
      roadmap_analytics_log_event(ANALYTICS_EVENT_SEARCH_NORES, ANALYTICS_EVENT_INFO_VALUE, searched_text);
      return;
   }

   assert( size <= ADSR_MAX_RESULTS);
   provider_icon = local_search_get_icon_name();
   for( i=0; i<size; i++)
   {
      if (array[i].type == ADDRESS_CANDIDATE_TYPE_ADRESS){
         array[i].offset =  adr_index++;
         results_adr[count_adr] = array[i].address;
         indexes_adr[count_adr] = (void*)i;
         icons_adr[count_adr] = "search_address";
         count_adr++;
      }
      else{
         array[i].offset =  ls_index++;
         results_ls[count_ls] = array[i].address;
         indexes_ls[count_ls] = (void*)i;
         icons_ls[count_ls] = provider_icon;
         count_ls++;
      }
   }

   if ( roadmap_native_keyboard_enabled() )
   {
      roadmap_native_keyboard_hide();
   }

   if ((count_adr > 4) && (count_ls > 0)){
      static char label[50];
      if (ssd_widget_rtl(NULL))
         snprintf(label, sizeof(label), "%s %d...", roadmap_lang_get("more"), count_adr - 3);
      else
         snprintf(label, sizeof(label), "%d %s...", count_adr - 3, roadmap_lang_get("more"));
      count_adr = 4;
      results_adr[count_adr-1] = label;
      indexes_adr[count_adr-1] = (void*)(-1);
      icons_adr[count_adr-1] = "search_address";

   }

   if (count_adr == 0){
      ssd_widget_hide(ssd_widget_get(list_cont,SSD_RC_ADDRESSLIST_TITLE_NAME));
   }
   else{
      ssd_widget_show(ssd_widget_get(list_cont,SSD_RC_ADDRESSLIST_TITLE_NAME));
   }

   ssd_list_populate(list_address,
                     count_adr,
                     results_adr,
                     (const void **)indexes_adr,
                     icons_adr,
                     0,
                     on_addr_list_item_selected,
                     NULL,
                     FALSE);

   if (count_ls == 0){
      ssd_widget_hide(ssd_widget_get(list_cont,SSD_RC_LSLIST_TITLE_NAME));
   }
   else{
      ssd_widget_show(ssd_widget_get(list_cont,SSD_RC_LSLIST_TITLE_NAME));
   }

   ssd_list_populate(list_ls,
                     count_ls,
                     results_ls,
                     (const void **)indexes_ls,
                     icons_ls,
                     0,
                     on_ls_list_item_selected,
                     NULL,
                     FALSE);

   generic_search_dlg_switch_gui();
}

static void on_search(void)
{
   SsdWidget      list_cont;
   SsdWidget      edit;
   const char*    text;
   roadmap_result rc;
   const char*    dl_prefix = roadmap_lang_get ("map:");
   SsdWidget dlg = generic_search_dlg_get_search_dlg(single_search);
   const char*    txt ;

   edit = generic_search_dlg_get_search_edit_box(single_search);

   txt = ssd_text_get_text( edit );

   if ( !strcmp( DEBUG_LEVEL_SET_PATTERN, ssd_text_get_text( edit ) ) )
   {
      roadmap_start_reset_debug_mode();
      return;
   }

   if ( !strcmp( "##@coord", txt ) )
   {
      roadmap_gps_reset_show_coordinates();
      return;
   }

   if ( !strcmp( "##@il",  txt ) )
   {
      roadmap_geo_config_il(NULL);
      return;
   }
   if ( !strcmp( "##@usa",  txt ) )
   {
      roadmap_geo_config_usa(NULL);
      return;
   }
   if ( !strcmp( "##@other",  txt ) )
   {
      roadmap_geo_config_other(NULL);
      return;
   }
   if ( !strcmp( "##@stg",  txt ) )
   {
      roadmap_geo_config_stg(NULL);
      return;
   }

   if ( !strcmp( "##@gps", ssd_text_get_text( edit )  ) )
   {
         roadmap_gps_set_show_raw(!roadmap_gps_is_show_raw());
         return ;
   }
   if ( !strcmp( "cc@tts", ssd_text_get_text( edit ) ) )
   {
      tts_clear_cache();
      roadmap_messagebox("","TTS cache has been cleared!");
      return;
   }
   if ( !strcmp( "##@tts", ssd_text_get_text( edit ) ) )
   {
      tts_set_feature_enabled( !tts_feature_enabled() );
      if ( tts_feature_enabled() )
      {
         roadmap_messagebox("","TTS Feature is enabled!\nPlease restart WAZE.");
      }
      else
      {
         roadmap_messagebox("","TTS Feature is disabled!");
      }
      navigate_main_override_nav_settings();
      return;
   }
   if ( !strcmp( "dbg@tts", ssd_text_get_text( edit ) ) )
   {
      if ( !strcmp( tts_was_provider_voices_set(), TTS_WAS_VOICES_SET_PRODUCTION ) )
      {
         tts_was_provider_apply_voices_set( TTS_WAS_VOICES_SET_DEBUG );
         roadmap_messagebox("","TTS Feature is running in debug mode!\nPlease restart WAZE.");
      }
      else
      {
         tts_was_provider_apply_voices_set( TTS_WAS_VOICES_SET_PRODUCTION );
         roadmap_messagebox("","TTS Feature is running in production mode!\nPlease restart WAZE.");
      }
      return;
   }

#ifdef __SYMBIAN32__
   if ( !strcmp( "##@opera",  txt ) )
   {
      roadmap_browser_set_show_external(NULL);
      return;
   }
#endif

   if (strstr(txt, "##@")){
      char *name;
      name = txt+3;
      if (name && name[0] != 0){
         roadmap_geo_config_generic(name);
         return;
      }
   }

   if ( !strncmp( dl_prefix, ssd_text_get_text( edit ), strlen( dl_prefix ) ) )
   {
      roadmap_map_download_region( ssd_text_get_text( edit ) + strlen( dl_prefix ),
                                   roadmap_locator_static_county() );
      ssd_dialog_hide_all( dec_close);

      if (!roadmap_screen_refresh ())
           roadmap_screen_redraw();
      return;
   }

   if (strstr(txt, "@")){

       char txt_buffer[128];
       strcpy (txt_buffer, txt);
       char *name = strtok (txt_buffer, "@");
       char *coord = strtok (NULL, "@");
       if (coord && (isdigit(coord[0]) || coord[0] == '-') && strchr (coord, ',') && strchr (coord, '.')){
             RoadMapPosition position;
              double   longtitude;
              double   latitude;
              char *latitude_ascii;
              char buffer[128];
              char txt_buffer[128];
              char *lon_ascii;

              strcpy (txt_buffer, coord);
              strcpy (buffer, coord);

              latitude_ascii = strchr (buffer, ',');
              if (latitude_ascii != NULL) {
                 *(latitude_ascii++) = 0;
                 lon_ascii = strtok(txt_buffer, ",");
                 if (latitude_ascii[0] == ' ')
                    latitude_ascii++;

                 if ((isdigit(txt_buffer[0]) || txt_buffer[0] == '-') && (isdigit(latitude_ascii[0]) || latitude_ascii[0] == '-')){
                    longtitude= atof(latitude_ascii);
                    latitude= atof(lon_ascii);
                    position.longitude = longtitude*1000000;
                    position.latitude  = latitude*1000000;
                    roadmap_trip_set_point ("Selection", &position);
                    roadmap_trip_set_focus ("Selection");
                    ssd_dialog_hide_all(dec_close);
                    editor_screen_menu(name);
                    return;
                 }
              }
       }
   }
   // Check if lat lon
   if ((isdigit(txt[0]) || txt[0] == '-') && strchr (txt, ',') && strchr (txt, '.')){
      RoadMapPosition position;
      double   longtitude;
      double   latitude;
      char *latitude_ascii;
      char buffer[128];
      char txt_buffer[128];
      char *lon_ascii;

      strcpy (txt_buffer, txt);
      strcpy (buffer, txt);

      latitude_ascii = strchr (buffer, ',');
      if (latitude_ascii != NULL) {
         *(latitude_ascii++) = 0;
         lon_ascii = strtok(txt_buffer, ",");
         if (latitude_ascii[0] == ' ')
            latitude_ascii++;

         if ((isdigit(txt_buffer[0]) || txt_buffer[0] == '-') && (isdigit(latitude_ascii[0]) || latitude_ascii[0] == '-')){
            longtitude= atof(lon_ascii);
            latitude= atof(latitude_ascii);
            position.longitude = longtitude*1000000;
            position.latitude  = latitude*1000000;
//            if (!(position.longitude==0 && position.latitude==0)){
               roadmap_trip_set_point ("Selection", &position);
               roadmap_trip_set_focus ("Selection");
               ssd_dialog_hide_all(dec_close);
               editor_screen_selection_menu();
               return;
//            }
         }
      }
   }

   s_searching = TRUE;

   roadmap_main_set_periodic( 100, search_progress_message_delayed );

   text     = ssd_text_get_text( edit );
   list_cont=  dlg->context;

   strncpy_safe (searched_text, text, sizeof(searched_text));

   rc = single_search_resolve_address( list_cont, on_address_resolved, text );
   if( succeeded == rc)
   {
      roadmap_main_set_cursor( ROADMAP_CURSOR_WAIT);
      roadmap_log(ROADMAP_DEBUG,
                  "single_search_dlg::on_search() - Started Web-Service transaction: Resolve address");
   }
   else
   {
      const char* err = roadmap_result_string( rc);
      s_searching = FALSE;
      roadmap_log(ROADMAP_ERROR,
                  "single_search_dlg::on_search() - Resolve process transaction failed to start");
      /* Close the progress message */
      ssd_progress_msg_dialog_hide();
      roadmap_messagebox_cb ( roadmap_lang_get( "Resolve Address"),
                           roadmap_lang_get( err ), on_search_error_message );
   }
}





static BOOL navigate( BOOL take_me_there)
{
   return navigate_with_coordinates( take_me_there, single_search, get_selected_list_item());
}


static void on_option_selected(  BOOL              made_selection,
                                 ssd_cm_item_ptr   item,
                                 void*             context)
{
   contextmenu_items selection = cm__invalid;
   BOOL              close     = FALSE;
   BOOL              do_nav    = FALSE;

   s_menu = FALSE;

   if( !made_selection)
      return;

   selection = item->id;

   switch( selection)
   {
      case cm_navigate:
         do_nav = TRUE;
         // Roll down...
      case cm_show:
         close = navigate( do_nav);
         break;

      case cm_add_to_favorites:
      {
         int                        selected_list_item   = get_selected_list_item();
         const address_candidate*   selection            = generic_search_result( selected_list_item);
         RoadMapPosition position;
         position.latitude = (int)(selection->latitude*1000000);
         position.longitude= (int)(selection->longtitude*1000000);
         generic_search_add_address_to_history( ADDRESS_FAVORITE_CATEGORY,
                                                selection->city,
                                                selection->street,
                                                get_house_number__str( selection->house),
                                                selection->state,
                                                NULL,
                                                &position);
         break;
      }

      case cm_cancel:
         close = FALSE;
         break;

      case cm_add_geo_reminder:
      {
         int                        selected_list_item   = get_selected_list_item();
         const address_candidate*   selection            = generic_search_result( selected_list_item);
         RoadMapPosition position;
         position.latitude = (int)(selection->latitude*1000000);
         position.longitude= (int)(selection->longtitude*1000000);
         roadmap_reminder_add_at_position(&position, TRUE, TRUE);
         break;
      }

      default:
         assert(0);
         break;
   }

   if( close)
   {
      ssd_dialog_hide_all( dec_close);

      if (!roadmap_screen_refresh ())
           roadmap_screen_redraw();
   }
}

static int on_back(SsdWidget widget, const char *new_value, void *context)
{
   if( generic_search_dlg_is_1st(single_search))
      ssd_dialog_hide_current( dec_cancel);
   else
      generic_search_dlg_switch_gui();

   return 0;
}

int on_options(SsdWidget widget, const char *new_value, void *context)
{
   int menu_x;
   BOOL add_cancel = TRUE;
#ifdef TOUCH_SCREEN
   roadmap_screen_refresh();
#endif

   assert( !s_menu);

   if  (ssd_widget_rtl (NULL))
      menu_x = SSD_X_SCREEN_RIGHT;
   else
      menu_x = SSD_X_SCREEN_LEFT;

   ssd_contextmenu_show_item( &context_menu,
                              cm_navigate,
                              !generic_search_dlg_is_1st(single_search),
                              FALSE);
   ssd_contextmenu_show_item( &context_menu,
                              cm_show,
                              !generic_search_dlg_is_1st(single_search),
                              FALSE);
   ssd_contextmenu_show_item( &context_menu,
                              cm_add_to_favorites,

                              !generic_search_dlg_is_1st(single_search),
                              FALSE);
   ssd_contextmenu_show_item( &context_menu,
                        cm_add_geo_reminder,
                              !generic_search_dlg_is_1st(single_search),
                              FALSE);
   ssd_contextmenu_show_item( &context_menu,
                              cm_add_geo_reminder,

                              !generic_search_dlg_is_1st(single_search)&&roadmap_reminder_feature_enabled(),
                              FALSE);

   ssd_contextmenu_show_item( &context_menu,
                              cm_cancel,
                              add_cancel,
                              FALSE);

   ssd_context_menu_show(  menu_x,              // X
                           SSD_Y_SCREEN_BOTTOM, // Y
                           &context_menu,
                           on_option_selected,
                           NULL,
                           dir_default,
                           0,
                           TRUE);


   s_menu = TRUE;

   return 0;
}

static int send_error_report(){
   roadmap_result rc;
   rc= address_search_report_wrong_address(searched_text);
   if( succeeded == rc)
   {
         roadmap_log(ROADMAP_DEBUG,
                     "address_search_dialog::on_list_item_selected() - Succeeded in sending report");
    }
   else
    {
        const char* err = roadmap_result_string( rc);
        roadmap_log(ROADMAP_ERROR,
              "address_search_dialog::on_list_item_selected() - Resolve process transaction failed to start: '%s'",
              err);
   }
   return 1;
}

static int sending_report_not_listed_cb (SsdWidget widget, const char *new_value){
   send_error_report();
   ssd_dialog_hide_current(dec_close);
   return 1;
}

static int not_listed_cb (SsdWidget widget, const char *new_value){
   SsdWidget dialog;
   SsdWidget box;
   SsdWidget text;
   SsdWidget button;
   int width;
   const char* icons[3];

   if (roadmap_canvas_width() > roadmap_canvas_height())
      width = roadmap_canvas_height();
   else
      width = roadmap_canvas_width();

   width -= 20;

   dialog = ssd_dialog_new ("NotListed",roadmap_lang_get("Not listed"), NULL, SSD_CONTAINER_TITLE);


   ssd_dialog_add_vspace(dialog, 10, 0);
   box = ssd_container_new("NotListedBox","", width, SSD_MIN_SIZE, SSD_ALIGN_CENTER|SSD_CONTAINER_BORDER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE);


   ssd_dialog_add_vspace(box, 20, 0);
   text = ssd_text_new("NotListedTxt", roadmap_lang_get("Can't find what you're"), 16,SSD_ALIGN_CENTER|SSD_END_ROW);
   ssd_text_set_color(text,"#000000");
   ssd_widget_add(box,text);

   text = ssd_text_new("NotListedTxt", roadmap_lang_get("looking for?"), 16,SSD_ALIGN_CENTER);
   ssd_text_set_color(text,"#000000");
   ssd_widget_add(box,text);

   text = ssd_text_new("NotListedtxt2", roadmap_lang_get("If your street is not on the map, note that you can record it and add it yourself!"), 16,SSD_TEXT_NORMAL_FONT|SSD_ALIGN_CENTER);
   ssd_text_set_color(text,"#000000");
   ssd_text_set_lines_space_padding(text, 5);
//   ssd_dialog_add_hspace(box, 10, 0);
   ssd_widget_add(box,text);

   ssd_dialog_add_vspace(box, 10, 0);
   text = ssd_text_new("NotListedtxt2", roadmap_lang_get("Find out how in the forum."), 16,SSD_TEXT_NORMAL_FONT|SSD_ALIGN_CENTER);
   ssd_text_set_color(text,"#000000");
//   ssd_dialog_add_hspace(box, 10, 0);
   ssd_text_set_lines_space_padding(text, 5);
   ssd_widget_add(box,text);
   ssd_dialog_add_vspace(box, 30, 0);
   icons[0] = "welcome_btn";
   icons[1] = "welcome_btn_h";
   icons[2] = NULL;

   // REPORT
   button = ssd_button_label_custom( "Report", roadmap_lang_get ("Report"), SSD_ALIGN_CENTER , sending_report_not_listed_cb,
          icons, 2, "#FFFFFF", "#FFFFFF",14 );
   ssd_widget_add(box, button);
   ssd_widget_add(dialog,box);

   ssd_dialog_activate("NotListed", NULL);
   return 1;
}

static SsdWidget create_results_container()
{
   SsdWidget rcnt = NULL;
   SsdWidget list = NULL;
   SsdWidget text = NULL;
   SsdWidget bitmap = NULL;
   SsdWidget not_listed_cont = NULL;
   int width = ssd_container_get_width();

   rcnt = ssd_container_new(  SSD_RESULTS_CONT_NAME,
                              NULL,
                              SSD_MIN_SIZE,
                              SSD_MIN_SIZE,
                              0);
   ssd_widget_set_color(rcnt, NULL,NULL);


   ssd_dialog_add_vspace(rcnt,5,0);
   ssd_dialog_add_hspace(rcnt,5,0);
   if (g_is_favorite){
      text = ssd_text_new(SSD_RC_ADDRESSLIST_TITLE_NAME, roadmap_lang_get("Tap the correct address below"), SSD_HEADER_TEXT_SIZE, SSD_TEXT_NORMAL_FONT | SSD_END_ROW);
   }else{
      text = ssd_text_new(SSD_RC_ADDRESSLIST_TITLE_NAME, roadmap_lang_get("New address"), SSD_HEADER_TEXT_SIZE, SSD_TEXT_NORMAL_FONT | SSD_END_ROW);
   }
   ssd_widget_add( rcnt, text);
   ssd_dialog_add_vspace(rcnt,1,0);

   list = ssd_list_new(       SSD_RC_ADDRESSLIST_NAME,
                              width,
                              SSD_MAX_SIZE,
                              inputtype_free_text,
                              SSD_ALIGN_CENTER|SSD_CONTAINER_BORDER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE,
                              NULL);
   //ssd_widget_set_color(list, NULL,NULL);
   ssd_list_resize( list, ssd_container_get_row_height());

   ssd_widget_add( rcnt, list);

   ssd_dialog_add_vspace(rcnt,5,0);
   ssd_dialog_add_hspace(rcnt,5,0);
   text = ssd_text_new(SSD_RC_LSLIST_TITLE_NAME, roadmap_lang_get(local_search_get_provider_label()), SSD_HEADER_TEXT_SIZE, SSD_TEXT_NORMAL_FONT | SSD_END_ROW);
   ssd_widget_add( rcnt, text);
   ssd_dialog_add_vspace(rcnt,1,0);

   list = ssd_list_new(       SSD_RC_LSLIST_NAME,
                              width,
                              SSD_MAX_SIZE,
                              inputtype_free_text,
                              SSD_ALIGN_CENTER|SSD_CONTAINER_BORDER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE,
                              NULL);
   //ssd_widget_set_color(list, NULL,NULL);
   ssd_list_resize( list, 80);

   ssd_widget_add( rcnt, list);

   ssd_dialog_add_vspace(rcnt, 20,0);
   not_listed_cont = ssd_container_new ("_not_listed_cont", NULL, width, ADJ_SCALE(60), SSD_ALIGN_CENTER|SSD_CONTAINER_BORDER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE);
   ssd_dialog_add_hspace(not_listed_cont, 10, 0);
   bitmap = ssd_bitmap_new("not_listed", "not_listed", SSD_ALIGN_VCENTER);
   ssd_widget_add( not_listed_cont, bitmap);
   ssd_dialog_add_hspace(not_listed_cont, 10, 0);
   text = ssd_text_new(SSD_RC_LSLIST_TITLE_NAME, roadmap_lang_get("Not listed?"), 16,  SSD_TEXT_NORMAL_FONT|SSD_ALIGN_VCENTER|SSD_END_ROW);
   ssd_widget_add( not_listed_cont, text);
   not_listed_cont->callback = not_listed_cb;
   ssd_widget_add( rcnt, not_listed_cont);


   return rcnt;
}

static SsdWidget get_result_container()
{
   if( !s_result_container)
   {
      s_result_container = create_results_container();
   }
   return s_result_container;
}

static void on_auto_search_completed( int exit_code, void* context)
{ ssd_dialog_hide_all( dec_close);}

void single_search_dlg_show_auto( PFN_ON_DIALOG_CLOSED cbOnClosed,
                              void*                context)
{
   generic_search_dlg_show( single_search,
                            SSD_DIALOG_NAME,
                            SSD_DIALOG_TITLE,
                            on_options,
                            on_back,
                            get_result_container(),
                            cbOnClosed,
                            on_search,
                            single_search_dlg_show,
                            context,
                            TRUE);

}

BOOL single_search_auto_search( const char* address)
{
   SsdWidget edit    = NULL;
   g_is_favorite = FALSE;
   g_favorite_name = NULL;

   generic_search_dlg_update_text( address, single_search );

   single_search_dlg_show_auto( on_auto_search_completed, (void*)address);

   edit  = generic_search_dlg_get_search_edit_box(single_search);

   ssd_text_set_text( edit, address);

   on_search();

   return TRUE;
}


BOOL single_search_add_favorite( const char* address, const char *favorite_name)
{
   SsdWidget edit    = NULL;
   g_is_favorite = TRUE;
   g_favorite_name = favorite_name;

   generic_search_dlg_update_text( address, single_search );
   single_search_dlg_show_auto( on_auto_search_completed, (void*)address);
   edit  = generic_search_dlg_get_search_edit_box(single_search);

   ssd_text_set_text( edit, address);

   on_search();

   return TRUE;
}

void single_search_dlg_repoen_favorite( PFN_ON_DIALOG_CLOSED cbOnClosed,
                                        void*                context)
{
   g_is_favorite = TRUE;
   generic_search_dlg_show( single_search,
                            SSD_DIALOG_NAME,
                            SSD_DIALOG_TITLE,
                            on_options,
                            on_back,
                            get_result_container(),
                            cbOnClosed,
                            on_search,
                            single_search_dlg_repoen_favorite,
                            context,
                            FALSE);

}


void single_search_dlg_show_favorite( const char *favorite_name,
                                      PFN_ON_DIALOG_CLOSED cbOnClosed,
                                      void*                context)
{
   g_is_favorite = TRUE;
   g_favorite_name = favorite_name;
   generic_search_dlg_show( single_search,
                            SSD_DIALOG_NAME,
                            SSD_DIALOG_TITLE,
                            on_options,
                            on_back,
                            get_result_container(),
                            cbOnClosed,
                            on_search,
                            single_search_dlg_repoen_favorite,
                            context,
                            FALSE);

}

void single_search_dlg_show( PFN_ON_DIALOG_CLOSED cbOnClosed,
                             void*                context)
{
   g_is_favorite = FALSE;
   generic_search_dlg_show( single_search,
                            SSD_DIALOG_NAME,
                            SSD_DIALOG_TITLE,
                            on_options,
                            on_back,
                            get_result_container(),
                            cbOnClosed,
                            on_search,
                            single_search_dlg_show,
                            context,
                            FALSE);

}

/* Allows other windows to be closed */
static void search_progress_message_delayed(void)
{
   roadmap_main_remove_periodic( search_progress_message_delayed );
   if( s_searching )
      ssd_progress_msg_dialog_show( roadmap_lang_get( "Searching..." ) );
}

/* Callback for the error message box */
static void on_search_error_message( int exit_code )
{
   generic_search_dlg_reopen_native_keyboard();
}
