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

//   Module initialization/termination - Called once, when the process starts/terminates

#include "../roadmap.h"
#include "../roadmap_config.h"
#include "../websvc_trans/websvc_trans.h"
#include "../roadmap_navigate.h"
#include "../roadmap_utf8.h"
#include "../roadmap_start.h"
#include "../roadmap_trip.h"
#include "generic_search.h"
#include "address_search.h"
#include "local_search.h"
#include "single_search.h"
#include "../Realtime/Realtime.h"
#include "../ssd/ssd_progress_msg_dialog.h"

#include "../roadmap_lang.h"

static wst_handle                s_websvc             = INVALID_WEBSVC_HANDLE;
static BOOL                      s_initialized_once   = FALSE;
static RoadMapConfigDescriptor   s_web_service_name   =
            ROADMAP_CONFIG_ITEM(
               SSR_WEBSVC_CFG_TAB,
               SSR_WEBSVC_ADDRESS);

extern const char* VerifyStatus( /* IN  */   const char*       pNext,
                                 /* IN  */   void*             pContext,
                                 /* OUT */   BOOL*             more_data_needed,
                                 /* OUT */   roadmap_result*   rc);

static const char* on_single_search_address_candidate(   /* IN  */   const char*       data,
                                 /* IN  */   void*             context,
                                 /* OUT */   BOOL*             more_data_needed,
                                 /* OUT */   roadmap_result*   rc);

static wst_parser data_parser[] =
{
   { "RC",                 VerifyStatus},
   { "AddressCandidate",   on_single_search_address_candidate}
};


/////////////////////////////////////////////////////////////////////////////////
static const char* get_webservice_address()
{ return roadmap_config_get( &s_web_service_name);}

/////////////////////////////////////////////////////////////////////////////////
BOOL single_search_init()
{
   const char* address;

   if( INVALID_WEBSVC_HANDLE != s_websvc)
   {
      assert(0);  // Called twice?
      return TRUE;
   }

   if( !s_initialized_once)
   {
      //   Web-service address
      roadmap_config_declare( SSR_WEBSVC_CFG_FILE,
                              &s_web_service_name,
                              SSR_WEBSVC_DEFAULT_ADDRESS,
                              NULL);
      s_initialized_once = TRUE;
   }

   address  = get_webservice_address();
   s_websvc = wst_init( address, NULL, NULL, "application/x-www-form-urlencoded; charset=utf-8");

   if( INVALID_WEBSVC_HANDLE != s_websvc)
   {
      roadmap_log(ROADMAP_DEBUG,
                  "single_search_init() - Web-Service Address: '%s'",
                  address);
      address_search_init();
      local_search_init();
      return TRUE;
   }

   roadmap_log(ROADMAP_ERROR, "single_search_init() - 'wst_init()' failed");
   return FALSE;
}

static const char* on_single_search_address_candidate(   /* IN  */   const char*       data,
                                 /* IN  */   void*             context,
                                 /* OUT */   BOOL*             more_data_needed,
                                 /* OUT */   roadmap_result*   rc)
{
   address_candidate ac;
   int               size;
   char              temp[100];

   address_candidate_init( &ac);

   // Default error for early exit:
   (*rc) = err_parser_unexpected_data;

   // Expected data:
   //    <provider><longtitude>,<latitude>,[state],[county],<city>,<street>,<house number>\n

   size  = 100;
   data  = ExtractNetworkString(
                data,       // [in]     Source string
                &temp[0],   // [out,opt]Output buffer
                &size,      // [in,out] Buffer size / Size of extracted string
                ",",        // [in]     Array of chars to terminate the copy operation
                1);         // [in]     Remove additional termination chars

    if( !data)
    {
       roadmap_log( ROADMAP_ERROR, "single_search::on_single_search_option() - Failed to read 'provider'");
       return NULL;
    }

    if (!strcmp("waze", temp)){
       return on_address_option(data, context, more_data_needed, rc);
    }
    else{
       return on_local_option(data, context, more_data_needed, rc);
    }

}

/////////////////////////////////////////////////////////////////////////////////
void single_search_term()
{
   if( INVALID_WEBSVC_HANDLE == s_websvc)
      return;

   roadmap_log( ROADMAP_DEBUG, "single_search_term() - TERM");

   wst_term( s_websvc);
   s_websvc = INVALID_WEBSVC_HANDLE;
   address_search_term();
   local_search_term();
}

roadmap_result single_search_resolve_address(
                  void*                context,
                  CB_OnAddressResolved cbOnAddressResolved,
                  const char*          address)

{
   char custom_query[GS_CUSTOM_QUERY_MAX_SIZE];

   snprintf( custom_query, GS_CUSTOM_QUERY_MAX_SIZE, "provider=%s&old_mobile_format=false", local_search_get_provider() );

   return generic_search_resolve_address(s_websvc, data_parser,sizeof(data_parser)/sizeof(wst_parser),
         "mozi_combo",context, cbOnAddressResolved, address, custom_query );
}
