/* single_search_dlg.c - iPhone single search dialog
 *
 * LICENSE:
 *
 *   Copyright 2010 Avi R.
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
#include "ssd/ssd_container.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_button.h"
#include "ssd/ssd_list.h"
#include "ssd/ssd_contextmenu.h"
#include "roadmap_input_type.h"
#include "roadmap_lang.h"
#include "roadmap_messagebox.h"
#include "roadmap_geocode.h"
#include "roadmap_trip.h"
#include "roadmap_history.h"
#include "roadmap_locator.h"
#include "roadmap_display.h"
#include "roadmap_main.h"
#include "roadmap_start.h"
#include "roadmap_geo_config.h"
#include "../roadmap_square.h"
#include "../roadmap_tile.h"
#include "../roadmap_tile_manager.h"
#include "../roadmap_tile_status.h"
#include "../roadmap_map_download.h"
#include "ssd_progress_msg_dialog.h"
#include "address_search.h"
#include "address_search_dlg.h"
#include "local_search.h"
#include "single_search_dlg.h"
#include "roadmap_searchbox.h"
#include "roadmap_list_menu.h"
#include "generic_search.h"
#include "Realtime.h"

#include "roadmap_analytics.h"


static   SsdWidget            s_dlg    = NULL;
static   BOOL                 s_1st    = TRUE;
static   BOOL                 s_menu   = FALSE;
static   BOOL                 s_history_was_loaded = FALSE;

static	int					selected_item;
static   char              searched_text[256];


#define  ASD_DIALOG_NAME            "address-search-dialog"
#define  ASD_DIALOG_TITLE           "New address"
#define  ASD_INPUT_CONT_NAME        "address-search-dialog.input-container"
#define  ASD_IC_EDITBOX_NAME        "address-search-dialog.input-container.editbox"
#define  ASD_IC_EDITBOX_CNT_NAME    "address-search-dialog.input-container.editbox.container"
#define  ASD_IC_BUTTON_NAME         "address-search-dialog.input-container.button"
#define  ASD_IC_BUTTON_LABEL        "Search"

#define  ASD_RESULTS_CONT_NAME      "address-search-dialog.results-container"
#define  ASD_RC_LIST_NAME           "address-search-dialog.results-container.list"
#define  ASD_RC_BACK_BTN_NAME       "address-search-dialog.results-container.back-btn"
#define  ASD_RC_BACK_BTN_LABEL      "Back"

#define  COULDNT_FIND_INDEX 1000
#define  COULDNT_FIND_ADDRESS_TEXT    "Couldn't find the address I wanted. Notify waze."


#define MAX_FAVORITES   3
#define MAX_AB          5

extern void navigate_main_stop_navigation();
extern int main_navigator( const RoadMapPosition *point,
						  address_info_ptr       ai);


static BOOL navigate( BOOL take_me_there);

typedef enum tag_contextmenu_items
{
   cm_navigate,
   cm_show,
   cm_add_to_favorites,
   cm_tel,
   cm_url,

   cm__count,
   cm__invalid

}  contextmenu_items;

// Context menu items:
static ssd_cm_item main_menu_items[] =
{
   //                  Label     ,           Item-ID
   SSD_CM_INIT_ITEM  ( "Navigate",           cm_navigate),
   SSD_CM_INIT_ITEM  ( "Show on map",        cm_show),
   SSD_CM_INIT_ITEM  ( "Add to favorites",   cm_add_to_favorites)
};

static ssd_cm_item tel_item = SSD_CM_INIT_ITEM  ( "Tel: ",   cm_tel);
//static ssd_cm_item url_item = SSD_CM_INIT_ITEM  ( "Web: ",   cm_url);

// Context menu:
static ssd_contextmenu  context_menu = SSD_CM_INIT_MENU( main_menu_items);


#define MAX_MENU_ITEMS 20

//Address events
static const char* ANALYTICS_EVENT_SSEARCHFAIL_NAME    = "SINGLE_SEARCH_FAILED";
static const char* ANALYTICS_EVENT_SSEARCHFAIL_INFO    = "SINGLE_SEARCH_STRING";
static const char* ANALYTICS_EVENT_SSEARCHNORES_NAME   = "SINGLE_SEARCH_NO_RESULTS";
static const char* ANALYTICS_EVENT_SSEARCHNORES_INFO   = "SINGLE_SEARCH_STRING";
static const char* ANALYTICS_EVENT_SSEARCHSUCCESS_NAME = "SINGLE_SEARCH_SUCCESS";


static int on_options(SsdWidget widget, const char *new_value, void *context);
static int send_error_report(){
   roadmap_result rc;

   //roadmap_analytics_log_event(ANALYTICS_EVENT_ADDRREPORT_NAME, ANALYTICS_EVENT_ADDRREPORT_INFO, searched_text);

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

   roadmap_main_pop_view(YES);

   return 0;
}

static int on_list_item_selected(SsdWidget widget, const char *new_value, const void *value, void *context)
{
	selected_item = (int)value;

   if (strcmp(new_value, roadmap_lang_get(COULDNT_FIND_ADDRESS_TEXT))==0) {
      send_error_report();
   }
   else
   {
      navigate(1);
   }

   return 0;
}

/* Callback for the error message box */
static void on_search_error_message( int exit_code )
{

}

static void on_address_resolved( void*                context,
                                 address_candidate*   array,
                                 int                  size,
                                 roadmap_result       rc)
{
   static   const char* results[MAX_FAVORITES + ADSR_MAX_RESULTS + MAX_AB];//3 max favorites ; 5 max adress book
   static   void*       indexes[MAX_FAVORITES + ADSR_MAX_RESULTS + MAX_AB];
   static   const char* icon_adr;
   static   const char* icon_ls;
   static   const char* icon_ab = NULL;
   int      count_adr = 0;
   int      count_ls = 0;
   int      count_ab = 0;
   int      total_count = 0;
   int       i;
   
   /* Close the progress message */
   ssd_progress_msg_dialog_hide();

   if( succeeded != rc)
   {
      if( is_network_error( rc))
         roadmap_messagebox_cb ( roadmap_lang_get( "Oops"),
                                roadmap_lang_get( "Search requires internet connection.\r\nPlease make sure you are connected."), on_search_error_message );
      
      
      
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
      roadmap_analytics_log_event(ANALYTICS_EVENT_SSEARCHFAIL_NAME, ANALYTICS_EVENT_SSEARCHFAIL_INFO, searched_text);
      return;
   }
   
   if( !size)
   {
      roadmap_log(ROADMAP_DEBUG,
                  "single_search_dlg::on_address_resolved() - NO RESULTS for the address-resolve process");
      roadmap_analytics_log_event(ANALYTICS_EVENT_SSEARCHNORES_NAME, ANALYTICS_EVENT_SSEARCHNORES_INFO, searched_text);
      return;
   }
   
   roadmap_analytics_log_event(ANALYTICS_EVENT_SSEARCHSUCCESS_NAME, NULL, NULL);
   
   assert( size <= ADSR_MAX_RESULTS);
   
   //addresses
   icon_adr = "search_address";
   for( i=0; i<size; i++)
   {
      if (array[i].type == ADDRESS_CANDIDATE_TYPE_ADRESS){
         results[total_count] = array[i].address;
         indexes[total_count++] = (void*)i;
         count_adr++;
      }
   }
   
   //local search
   icon_ls = local_search_get_icon_name();
   for( i=0; i<size; i++)
   {
      if (array[i].type != ADDRESS_CANDIDATE_TYPE_ADRESS){
         results[total_count] = array[i].address;
         indexes[total_count++] = (void*)i;
         count_ls++;
      }
   }
   
   //address book
   
   single_search_resolved_dlg_show(results, indexes,
                                   count_adr,
                                   count_ls,
                                   count_ab,
                                   on_list_item_selected, on_options);
   /*
   if (count_adr > 3){
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
   
   
   
   */
   
   
}


static int on_search(const char *text, void *list_cont)
{
  roadmap_result rc;

	if ( !strcmp( DEBUG_LEVEL_SET_PATTERN, text ) )
	{
		roadmap_start_reset_debug_mode();
		roadmap_main_show_root(NO);
		return 0;
	}
   if ( !strcmp( "##@coord", text ) )
   {
      roadmap_gps_reset_show_coordinates();
      roadmap_main_show_root(NO);
      return 0;
   }
   if ( !strcmp( "##@il", text ) )
   {
      roadmap_geo_config_il(NULL);
      roadmap_main_show_root(NO);
		return 0;
   }
   if ( !strcmp( "##@usa", text ) )
   {
      roadmap_geo_config_usa(NULL);
      roadmap_main_show_root(NO);
		return 0;
   }
   if ( !strcmp( "##@other", text ) )
   {
      roadmap_geo_config_other(NULL);
      roadmap_main_show_root(NO);
      return 0;
   }
   if ( !strcmp( "##@stg", text ) )
   {
      roadmap_geo_config_stg(NULL);
      roadmap_main_show_root(NO);
      return 0;
   }

   if ( !strcmp( "##@heb", text ) )
   {
      roadmap_lang_set_system_lang("heb");
      roadmap_messagebox("", "Language changed to Hebrew, please restart waze");
      return 0;
   }
   if ( !strcmp( "##@eng", text ) )
   {
      roadmap_messagebox("","Language changed to English, please restart waze");
      roadmap_lang_set_system_lang("eng");
      return 0;
   }


   strncpy_safe (searched_text, text, sizeof(searched_text));

	rc = ( single_search_resolve_address( list_cont, on_address_resolved, text));
	if( succeeded == rc)
	{
		//roadmap_main_set_cursor( ROADMAP_CURSOR_WAIT);
      ssd_progress_msg_dialog_show( roadmap_lang_get( "Searching . . . " ) );
		roadmap_log(ROADMAP_DEBUG,
					"single_search_dlg::on_search() - Started Web-Service transaction: Resolve address");
	}
	else
	{
		const char* err = roadmap_result_string( rc);

		roadmap_log(ROADMAP_ERROR,
					"single_search_dlg::on_search() - Resolve process transaction failed to start");
      /* Close the progress message */
      ssd_progress_msg_dialog_hide();
		roadmap_messagebox ( roadmap_lang_get( "Resolve Address"),
							roadmap_lang_get( err));
	}


   return 0;
}

static int get_selected_list_item()
{
   SsdWidget list;

   if( s_1st)
      return -1;

   list = ssd_widget_get( s_dlg, ASD_RC_LIST_NAME);
   assert(list);

   return (int)ssd_list_selected_value( list);
}

/*
static const char* get_house_number__str( int i)
{
   static char s_str[12];
   static int  s_i = -1;

   if( i < 0)
      s_str[0] = '\0';
   else
   {
      if( s_i != i)
         sprintf( s_str, "%d", i);
   }

   s_i = i;

   return s_str;
}

*/

char* favorites_address_info[ahi__count];
/*
BOOL on_favorites_name( int         exit_code,
					   const char* name,
					   void*       context)
{
	int i;

	roadmap_main_show_root(0);

	if( dec_ok == exit_code)
	{
		if( !name || !(*name))
			roadmap_messagebox ( roadmap_lang_get( "Add to favorites"),
								roadmap_lang_get( "ERROR: Invalid name specified"));
		else
		{
			favorites_address_info[ ahi_name] = strdup( name);
			roadmap_history_add( ADDRESS_FAVORITE_CATEGORY, (const char **)favorites_address_info);

			ssd_dialog_hide_all( dec_close);

			if( !roadmap_screen_refresh())
				roadmap_screen_redraw();
		}
	}

	for( i=0; i<ahi__count; i++)
		FREE( favorites_address_info[i])

		return TRUE;
}


static void add_to_favorites()
{
	ShowEditbox(roadmap_lang_get( "Name"), NULL, on_favorites_name, NULL, EEditBoxStandard);
}

static void add_address_to_history( int               category,
								   const char*       city,
								   const char*       street,
								   const char*       house,
								   const char*       name,
								   RoadMapPosition*  position)
{
	const char* address[ahi__count];
	char        latitude[32];
	char        longtitude[32];

	address[ahi_city]          = city;
	address[ahi_street]        = street;
	address[ahi_house_number]  = house;
	address[ahi_state]         = ADDRESS_HISTORY_STATE;
	if(name)
		address[ahi_name]   = name;
	else
		address[ahi_name]   = "";

	sprintf(latitude, "%d", position->latitude);
	sprintf(longtitude, "%d", position->longitude);
	address[ahi_latitude]   = latitude;
	address[ahi_longtitude] = longtitude;

	if( ADDRESS_FAVORITE_CATEGORY == category)
	{
		int i;

		for( i=0; i<ahi__count; i++)
			favorites_address_info[i] = strdup( address[i]);

		add_to_favorites();
	}
	else
		roadmap_history_add( category, address);
}


static BOOL navigate_with_coordinates_int( BOOL take_me_there)
{
	address_info               ai;
	int                        selected_list_item= selected_item;
	const address_candidate*   selection         = generic_search_result( selected_list_item);
	RoadMapPosition            position;

	if( !selection)
	{
		assert(0);
		return FALSE;
	}

	position.longitude= (int)(selection->longtitude* 1000000);
	position.latitude = (int)(selection->latitude  * 1000000);

	roadmap_trip_set_point ("Selection",&position);
	roadmap_trip_set_point ("Address",  &position);

	ai.state    = NULL;
	ai.country  = NULL;
	ai.city     = selection->city;
	ai.street   = selection->street;
	ai.house    = get_house_number__str( selection->house);

	add_address_to_history( ADDRESS_HISTORY_CATEGORY,
                           selection->city,
                           selection->street,
                           get_house_number__str( selection->house),
                           NULL,
                           &position);

	if( take_me_there)
	{
		// Cancel previous navigation:
		navigate_main_stop_navigation();

		if( -1 == main_navigator( &position, &ai))
			return FALSE;
	}
	else
	{
		int square = roadmap_tile_get_id_from_position (0, &position);
   	roadmap_tile_request (square, ROADMAP_TILE_STATUS_PRIORITY_ON_SCREEN, 1, NULL);

      roadmap_trip_set_focus ("Address");

		roadmap_square_force_next_update ();
      roadmap_screen_redraw ();
	}

	return TRUE;
}
*/


static BOOL navigate( BOOL take_me_there)
//{ return navigate_with_coordinates_int( take_me_there);}
{ return navigate_with_coordinates( take_me_there, search_address, selected_item);}


static int on_option_selected (SsdWidget widget, const char* sel ,const void *value, void* context) {

   contextmenu_items item_id = cm__invalid;
   BOOL              close     = FALSE;
   BOOL              do_nav    = FALSE;
	RoadMapPosition position;
   char url[256];
   int                        selected_list_item   = selected_item;
   const address_candidate*   selection            = generic_search_result( selected_list_item);

   s_menu = FALSE;

   item_id = ((ssd_cm_item_ptr)value)->id;

   switch( item_id)
   {
      case cm_navigate:
         do_nav = TRUE;
         // Roll down...
      case cm_show:
		   	roadmap_main_show_root(0);
         close = navigate( do_nav);
         break;

      case cm_add_to_favorites:
         position.longitude= (int)(selection->longtitude* 1000000);
         position.latitude = (int)(selection->latitude  * 1000000);

         generic_search_add_address_to_history( ADDRESS_FAVORITE_CATEGORY,
                                               selection->city,
                                               selection->street,
                                               get_house_number__str( selection->house),
                                               selection->state,
                                               NULL,
                                               &position);

         break;

      case cm_tel:
         sprintf(url, "tel:%s", selection->phone);
         roadmap_main_open_url(url);
         break;
         
      case cm_url:
         roadmap_main_open_url(selection->url);
         break;
      default:
         assert(0);
         break;
   }

   return 0;
}

int on_options(SsdWidget widget, const char *new_value, void *context)
{
	static   const char* labels[MAX_MENU_ITEMS];
	static   void*       values[MAX_MENU_ITEMS];
   static   char        phone[100];
	int i;
   int count;

   selected_item = (int)new_value;
   const address_candidate*   selection = generic_search_result( selected_item);

   if (selected_item != COULDNT_FIND_INDEX) {


      for (i = 0; i < context_menu.item_count; ++i) {
         labels[i] = context_menu.item[i].label;
         values[i] = (void *)&context_menu.item[i];
      }
      count = context_menu.item_count;
      
      if (selection->phone[0] != 0) {
         snprintf(phone, sizeof(phone), "%s: %s",roadmap_lang_get("Phone number"), selection->phone);
         labels[count] = phone;
         values[count] = (void *)&tel_item;
         count++;
      }
   }

	roadmap_list_menu_generic("Options", count, labels, (const void**)values, NULL, NULL, NULL,
                             on_option_selected, NULL, NULL, NULL, 60, FALSE, NULL);



   return 0;
}

static BOOL on_searchbox_done (int type, const char *new_value, void *context)
{
	on_search (new_value, context);
	return 0;
}

static void verify_init ()
{
   if( !s_history_was_loaded)
   {
      roadmap_history_declare( ADDRESS_HISTORY_CATEGORY, ahi__count);
	   roadmap_history_declare( ADDRESS_FAVORITE_CATEGORY, ahi__count);
      s_history_was_loaded = TRUE;
   }
}

BOOL single_search_auto_search( const char* address)
{
   verify_init();

	on_search(address, NULL);

	return TRUE;
}

void single_search_dlg_show( PFN_ON_DIALOG_CLOSED cbOnClosed,
                              void*                context)
{
   verify_init();

	ShowSearchbox (roadmap_lang_get (ASD_DIALOG_TITLE), roadmap_lang_get ("Enter your search"), on_searchbox_done, NULL);
}