/* generic_search.c
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

#include "../roadmap.h"
#include "../roadmap_config.h"
#include "../roadmap_start.h"
#include "../roadmap_lang.h"
#include "../roadmap_history.h"
#include "../roadmap_tile.h"
#include "../roadmap_square.h"
#include "../roadmap_tile_manager.h"
#include "../roadmap_tile_status.h"
#include "../roadmap_map_download.h"
#include "../roadmap_messagebox.h"
#include "../roadmap_navigate.h"
#include "../roadmap_utf8.h"
#include "../roadmap_trip.h"
#include "generic_search.h"
#include "../Realtime/Realtime.h"
#include "../ssd/ssd_keyboard_dialog.h"

#ifdef IPHONE
#include "roadmap_editbox.h"
#endif //IPHONE

extern void convert_int_coordinate_to_float_string(char* buffer, int value);

extern void navigate_main_stop_navigation();
extern int main_navigator( const RoadMapPosition *point,
                           address_info_ptr       ai);
extern void convert_int_coordinate_to_float_string(char* buffer, int value);


static CB_OnAddressResolved      s_cbOnAddressResolved= NULL;
static void*                     s_context            = NULL;
static address_candidate         s_results[ADSR_MAX_RESULTS];
static int                       s_results_count      = 0;


const address_candidate* generic_search_results()
{
   if( s_results_count < 1)
      return NULL;

   return s_results;
}

const address_candidate* generic_search_result( int index)
{
   if( (index < 0) || (s_results_count <= index))
   {
      assert(0);
      return NULL;
   }

   return s_results + index;
}


int generic_search_result_count()
{ return s_results_count;}

void address_candidate_init( address_candidate* this)
{
   memset( this,0,sizeof(address_candidate));
   this->house = -1;
}

static void address_append_current_location( char* buffer)
{
   char  float_string_longitude[32];
    char  float_string_latitude [32];
    PluginLine line;
    int direction;

    RoadMapGpsPosition   MyLocation;

    if( roadmap_navigate_get_current( &MyLocation, &line, &direction) != -1)
    {
       convert_int_coordinate_to_float_string( float_string_longitude, MyLocation.longitude);
       convert_int_coordinate_to_float_string( float_string_latitude , MyLocation.latitude);

       sprintf( buffer, "&longtitude=%s&latitude=%s", float_string_longitude, float_string_latitude);
    }
    else{
       const RoadMapPosition *Location;
       Location = roadmap_trip_get_position( "Location" );
       if ( (Location != NULL) && !IS_DEFAULT_LOCATION( Location ) ){
          convert_int_coordinate_to_float_string( float_string_longitude, Location->longitude);
          convert_int_coordinate_to_float_string( float_string_latitude , Location->latitude);

          sprintf( buffer, "&longtitude=%s&latitude=%s", float_string_longitude, float_string_latitude);
       }
       else{
          roadmap_log( ROADMAP_DEBUG, "address_search::no location used");
          sprintf( buffer, "&longtitude=0&latitude=0");
       }
    }
}

void generic_address_add(address_candidate ac){
   if (s_results_count == ADSR_MAX_RESULTS)
      return;

   s_results[s_results_count++] = ac;
}
static const char* address__prepare_query( const char* address, const char* custom_query )
{
   static char s_current_location[1024];

   /* Main attributes */
   sprintf( s_current_location,
            "q=%s&mobile=true&max_results=%d&server_cookie=%s&version=%s&lang=%s",
             address, ADSR_MAX_RESULTS,Realtime_GetServerCookie(), roadmap_start_version(),
             roadmap_lang_get_system_lang() );
   /* Append custom query */
   if ( custom_query )
   {
	   sprintf ( s_current_location + strlen(s_current_location), "&%s", custom_query );
   }
   /* Append current location */
   address_append_current_location( s_current_location + strlen(s_current_location));

   return s_current_location;
}


static void on_completed( void* ctx, roadmap_result res)
{
   if(s_cbOnAddressResolved)
      s_cbOnAddressResolved( ctx, s_results, s_results_count, res);

   s_cbOnAddressResolved = NULL;
   s_context             = NULL;
}

// q=beit a&mobile=true&max_results=10&longtitude=10.943983489&latitude=23.984398
roadmap_result generic_search_resolve_address(
                  wst_handle           websvc,
                  wst_parser           *data_parser,
                  int                  parser_count,
                  const char           *service_name,
                  void*                context,
                  CB_OnAddressResolved cbOnAddressResolved,
                  const char*          address, const char* custom_query )
{
   transaction_state tstate;
   const char*       query = NULL;

//   assert( address && (*address));

   if( INVALID_WEBSVC_HANDLE == websvc)
   {
      roadmap_log( ROADMAP_ERROR, "address_search_resolve_address() - MODULE NOT INITIALIZED");
      assert(0);  // 'address_search_init()' was not called
      return err_internal_error;
   }

   if( !address || (utf8_strlen(address)<ADSR_ADDRESS_MIN_SIZE) || (ADSR_ADDRESS_MAX_SIZE<utf8_strlen(address)))
   {
      roadmap_log(ROADMAP_ERROR,
                  "address_search_resolve_address() - Size of 'Address to resolve' is wrong (%d)",
                  utf8_strlen(address));
      return err_as_wrong_input_string_size;
   }

   tstate = wst_get_trans_state( websvc);

   if( trans_idle != tstate)
   {
      roadmap_log( ROADMAP_DEBUG, "address_search_resolve_address() - Cannot start transaction: Transaction is not idle yet");
      wst_watchdog( websvc);
      return err_as_already_in_transaction;
   }

   s_cbOnAddressResolved = cbOnAddressResolved;
   s_context             = context;

   query = address__prepare_query( address, custom_query );
   s_results_count = 0;
   
   roadmap_log (ROADMAP_INFO, "Local search query: %s", query);

   // Perform WebService Transaction:
   if( wst_start_trans( websvc,
                        service_name,
                        data_parser,
                        parser_count,
                        on_completed,
                        context,
                        query))
      return succeeded;

   s_cbOnAddressResolved = NULL;
   s_context             = NULL;

   roadmap_log( ROADMAP_DEBUG, "address_search_resolve_address() - Transaction failed...");
   return err_failed;
}


const char* get_house_number__str( int i)
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

char* favorites_address_info[ahi__count];
BOOL on_favorites_name( int         exit_code,
                        const char* name,
                        void*       context)
{
   int i;

   if( dec_ok == exit_code)
   {
      if( !name || !(*name))
         roadmap_messagebox ( roadmap_lang_get( "Add to favorites"),
                              roadmap_lang_get( "ERROR: Invalid name specified"));
      else
      {
         RoadMapPosition coordinates;
         coordinates.latitude = atoi(favorites_address_info[5]);
         coordinates.longitude = atoi(favorites_address_info[6]);
         Realtime_TripServer_CreatePOI(name, &coordinates, TRUE);

         favorites_address_info[ ahi_name] = strdup( name);
         roadmap_history_add( ADDRESS_FAVORITE_CATEGORY, (const char **)favorites_address_info);
#ifdef IPHONE
         roadmap_main_show_root(0);
#endif //IPHONE
         ssd_dialog_hide_all( dec_close);

         if( !roadmap_screen_refresh())
            roadmap_screen_redraw();
      }
   }

   for( i=0; i<ahi__count; i++)
      FREE( favorites_address_info[i])

   return TRUE;
}

void generic_search_add_to_favorites()
{

#if ((defined(__SYMBIAN32__) && !defined(TOUCH_SCREEN)) || defined(IPHONE))
   ShowEditbox(roadmap_lang_get("Name"), "",
            on_favorites_name, NULL, EEditBoxStandard | EEditBoxAlphaNumeric );
#else
   ssd_show_keyboard_dialog(  roadmap_lang_get( "Name"),
                              NULL,
                              on_favorites_name,
                              NULL);
#endif
}

void generic_search_add_address_to_history( int               category,
                                    const char*       city,
                                    const char*       street,
                                    const char*       house,
                                    const char*       state,
                                    const char*       name,
                                    RoadMapPosition*  position)
{
   const char* address[ahi__count];
   char        latitude[32];
   char        longtitude[32];

   address[ahi_city]          = city;
   address[ahi_street]        = street;
   address[ahi_house_number]  = house;
   address[ahi_state]         = state;
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

      generic_search_add_to_favorites();
   }
   else
   {
      roadmap_history_add( category, address);
      roadmap_history_save();
   }
}

BOOL navigate_with_coordinates( BOOL take_me_there, search_types type, int   selected_list_item)
{
   address_info               ai;

   const address_candidate*   selection         = generic_search_result( selected_list_item);
   RoadMapPosition            position;
   const char* name = NULL;

   if( !selection)
   {
      assert(0);
      return FALSE;
   }

   position.longitude= (int)(selection->longtitude* 1000000);;
   position.latitude = (int)(selection->latitude  * 1000000);;

   roadmap_trip_set_point ("Selection",&position);
   roadmap_trip_set_point ("Address",  &position);

   ai.state    = selection->state;
   ai.country  = NULL;
   ai.city     = selection->city;
   ai.street   = selection->street;
   ai.house    = get_house_number__str( selection->house);

   if (type == search_local)
      name = selection->name;
   generic_search_add_address_to_history( ADDRESS_HISTORY_CATEGORY,
                                          selection->city,
                                          selection->street,
                                          get_house_number__str( selection->house),
                                          selection->state,
                                          name,
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

