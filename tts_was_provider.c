/* tts_was_provider.c - implementation of the WAS based tts service provider.
 *
 * LICENSE:
 *   Copyright 2011, Waze Ltd      Alex Agranovich (AGA)
 *
 *
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
 * SYNOPSYS:
 *
 *   See tts.h
 *       tts_provider.h
 *
 */
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "tts/tts.h"
#include "tts/tts_utils.h"
#include "tts/tts_provider.h"
#include "roadmap_config.h"
#include "roadmap_time.h"
#include "roadmap_httpcopy_async.h"
#include "roadmap_path.h"
#include "Realtime/Realtime.h"
#include "websvc_trans/web_date_format.h"
#include "tts_was_provider.h"

//======================== Local defines ========================

#define WAS_TTS_PROTOCOL_VERSION                3        /* Version number for the API compatability with WAS */

#define WAS_TTS_PROVIDER                        "was_tts"
#define WAS_TTS_PROVIDER_VOICES                 "voices_was_tts.csv"

#define WAS_TTS_BATCH_REQUESTS_LIMIT            4
#define WAS_TTS_CONCURRENT_REQUESTS_LIMIT       8
#define WAS_TTS_URL_MAXSIZE                     512

#define WAS_TTS_CATEGORY                        ("TTS WAS Provider")
#define WAS_TTS_CFG_WS_ADDRESS                  ("Web-Service Address")
#define WAS_TTS_CFG_WS_ADDRESS_DEFAULT          ("http://174.129.223.121:80/WAS/text2speach")
#define WAS_TTS_CFG_UPDATE_TIME                 ("Voices Update Time")
#define WAS_TTS_CFG_VOICES_URL                  ("Voices Url")
#define WAS_TTS_CFG_VOICES_SET                  ("Voices Set")
#define WAS_TTS_CFG_VOICES_URL_DEFAULT          ("http://174.129.223.121:80/WAS/text2speach_capabilities_csv")

#define WAS_TTS_CFG_PROTOCOL_VERSION            ("Protocol Ver")
#define WAS_TTS_CFG_PROTOCOL_VERSION_DEFAULT    ("0")

//#define WAS_TTS_CFG_AUDIO_CONTENT_TYPE                     ("audio/mpeg")
#define WAS_TTS_CFG_AUDIO_CONTENT_TYPE          ("Audio Content Type")
#define WAS_TTS_CFG_AUDIO_CONTENT_TYPE_DEFAULT  ("audio/x-wav")

#define WAS_PREALLOC_DATA_SIZE                  48000

//======================== Local types ========================

typedef struct {

   void* data; /* Received data */
   int size; /* Received data size */
   int alloc_size;

   char* query;
   int flags;

   const void* cb_context;
   TtsSynthResponseCb response_cb;
   TtsSynthResponseData response_data;

   BOOL        busy;
   int         index;
} WasRequestContext;

typedef struct {
   const char* text; // Pointer to the null terminated text string in the received buffer
   void* data; // Pointer to the synthesized audio in the received buffer
   int data_size; // Size of the chunk of pointed by data

   int total_size; // Total size in buffer occupied by this entry (including size fields)

   // Debug
   int text_size;

} WasResponseEntry;

typedef struct {
   size_t data_size;
   void* data;
   size_t data_size_max;
} WasVoicesHttpCtx;


//======================== Globals ========================

/*
 * Configuration
 */
static RoadMapConfigDescriptor WasProviderConfigWebService =
      ROADMAP_CONFIG_ITEM( WAS_TTS_CATEGORY, WAS_TTS_CFG_WS_ADDRESS );
static RoadMapConfigDescriptor WasProviderConfigVoicesUpdateTime =
                           ROADMAP_CONFIG_ITEM( WAS_TTS_CATEGORY, WAS_TTS_CFG_UPDATE_TIME );
static RoadMapConfigDescriptor WasProviderConfigVoicesUrl =
                           ROADMAP_CONFIG_ITEM( WAS_TTS_CATEGORY, WAS_TTS_CFG_VOICES_URL );
static RoadMapConfigDescriptor WasProviderConfigVoicesSet =
                           ROADMAP_CONFIG_ITEM( WAS_TTS_CATEGORY, WAS_TTS_CFG_VOICES_SET );

static RoadMapConfigDescriptor WasProviderConfigAudioContentType =
                           ROADMAP_CONFIG_ITEM( WAS_TTS_CATEGORY, WAS_TTS_CFG_AUDIO_CONTENT_TYPE );

static RoadMapConfigDescriptor WasProviderConfigProtocolVersion =
                           ROADMAP_CONFIG_ITEM( WAS_TTS_CATEGORY, WAS_TTS_CFG_PROTOCOL_VERSION );

/*
 * Status attributes
 */
static WasRequestContext   sgCtxPool[TTS_ACTIVE_REQUESTS_LIMIT];
static char                sgVoicesPath[TTS_PATH_MAXLEN];
static TtsProvider         sgWasProvider;
static BOOL                sgWaitingConfig  =  FALSE;               // Indicates that we are waiting for the updated configuration

//======================== Local Declarations ========================


static void _on_response( WasRequestContext* ctx, int res_status );
static int _get_int32( unsigned char* src_data, int src_big_endian );

static void* _response_entry( void* this, int size, WasResponseEntry* entry );
static void _buffer_from_entry( TtsSynthResponseData* response_data, const WasResponseEntry* entry );
static void _voices_init( void );
static int _process_received( WasRequestContext* context );
static WasRequestContext* _allocate_context( void );

static void _synth_request( const void* context, TtsTextList text_list, const TtsSynthRequestParams* params, TtsSynthResponseCb response_cb );

static int _request_size_cb( void *context_cb, size_t size );
static void _request_progress_cb( void *context_cb, char *data, size_t size );
static void _request_error_cb( void *context_cb, int connection_failure, const char *format, ... );
static void _request_done_cb( void *context_cb, char *last_modified, const char *format, ...  );

static int _voices_cfg_size_cb( void *context_cb, size_t size );
static void _voices_cfg_progress_cb( void *context_cb, char *data, size_t size );
static void _voices_cfg_error_cb( void *context_cb, int connection_failure, const char *format, ... );
static void _voices_cfg_done_cb( void *context_cb, char *last_modified, const char *format, ...  );
static const char* _voices_cfg_url ( void );

static RoadMapHttpAsyncCallbacks sgHttpTtsRequestCBs =
         {
            _request_size_cb,
            _request_progress_cb,
            _request_error_cb,
            _request_done_cb
         };

static RoadMapHttpAsyncCallbacks sgHttpTtsVoicesCfgCBs =
         {
            _voices_cfg_size_cb,
            _voices_cfg_progress_cb,
            _voices_cfg_error_cb,
            _voices_cfg_done_cb
         };


void tts_was_provider_init( void )
{
   int i;
   int protocol_ver_cfg;
   roadmap_path_format( sgVoicesPath, TTS_PATH_MAXLEN, roadmap_path_tts(), WAS_TTS_PROVIDER_VOICES );
   roadmap_config_declare("preferences", &WasProviderConfigWebService,
         WAS_TTS_CFG_WS_ADDRESS_DEFAULT, NULL);
   roadmap_config_declare( "preferences", &WasProviderConfigAudioContentType,
         WAS_TTS_CFG_AUDIO_CONTENT_TYPE_DEFAULT, NULL );
   roadmap_config_declare( "session", &WasProviderConfigProtocolVersion,
         WAS_TTS_CFG_PROTOCOL_VERSION_DEFAULT, NULL );

   // Provider initialization
   sgWasProvider.batch_request_limit = WAS_TTS_BATCH_REQUESTS_LIMIT;
   sgWasProvider.provider_name = WAS_TTS_PROVIDER;
   sgWasProvider.storage_type = __tts_db_data_storage__file;
   sgWasProvider.request_cb = _synth_request;
   sgWasProvider.voices_cfg = sgVoicesPath;
   sgWasProvider.concurrent_limit = WAS_TTS_CONCURRENT_REQUESTS_LIMIT;

   protocol_ver_cfg = roadmap_config_get_integer( &WasProviderConfigProtocolVersion );
   if ( roadmap_file_exists( sgVoicesPath, NULL ) &&
         ( protocol_ver_cfg == 0 || protocol_ver_cfg ==  WAS_TTS_PROTOCOL_VERSION ) )
   {
      // Register the android provider
      tts_register_provider( &sgWasProvider );
      roadmap_config_set_integer( &WasProviderConfigProtocolVersion, WAS_TTS_PROTOCOL_VERSION );
   }
   else
   {
      roadmap_log( ROADMAP_WARNING, TTS_LOG_STR( "WAS PROVIDER. The protocol version doesn't match the configuration. Trying to retrieve from the server" ) );
      sgWaitingConfig = TRUE;
   }
   // Voices initialization
   _voices_init();

   // Initialize the context pool
   for ( i = 0; i < TTS_ACTIVE_REQUESTS_LIMIT; ++i )
   {
      sgCtxPool[i].busy = FALSE;
      sgCtxPool[i].index = i;
   }
}
/*
 ******************************************************************************
 */
void tts_was_provider_apply_voices_set( const char* new_set )
{
   BOOL changed = FALSE;

   if ( !new_set )
      return;

   changed = !roadmap_config_match( &WasProviderConfigVoicesSet, new_set );

   if ( changed )
   {
      roadmap_config_set( &WasProviderConfigVoicesSet, new_set );
      /*
       * Reset the modified time for this set of capabilities
       */
      roadmap_config_set( &WasProviderConfigVoicesUpdateTime, "" );
   }


}
/*
 ******************************************************************************
 */
const char* tts_was_provider_voices_set( void )
{
   return roadmap_config_get( &WasProviderConfigVoicesSet );
}
/*
 ******************************************************************************
 */
static int _request_size_cb( void *context_cb, size_t size ) {
   WasRequestContext *context = (WasRequestContext *) context_cb;

   roadmap_log( ROADMAP_DEBUG, TTS_LOG_STR( "Size Callback. for context index %d. Requested size: %d" ), context->index, size );

   // Allocate data storage
/*
   if ( size > 0 )
   {
      context->data = malloc(size);
      context->alloc_size = size;
      context->data_cursor = context->data;
      context->size = 0;
   }

   return size;
*/

   return 1;
}

/*
 ******************************************************************************
 */
static void _request_progress_cb( void *context_cb, char *data, size_t size )
{
   WasRequestContext* context = (WasRequestContext*) context_cb;

   // Allocate the buffer. Should occur only once in common case. If occurs more than one time frequently
   // the prealloc data size should be increased
   if ( context->alloc_size < ( context->size + (int) size ) )
   {
      int multiplier = size/WAS_PREALLOC_DATA_SIZE + 1;
      if ( context->alloc_size > 0 )
      {
         roadmap_log( ROADMAP_INFO, TTS_LOG_STR( "Received more bytes ( %d ) than allocated ( %d ) for request. Reallocating..." ),
               context->alloc_size, ( context->size + (int) size ) );
      }
      context->alloc_size += multiplier*WAS_PREALLOC_DATA_SIZE;
      context->data = realloc( context->data, context->alloc_size );
   }

   if ( context->data )
   {
      memcpy( context->data + context->size, data, size );
      context->size += size;
   }
}

/*
 ******************************************************************************
 */
static void _request_error_cb(void *context_cb, int connection_failure, const char *format, ... )
{
   WasRequestContext* context = (WasRequestContext*) context_cb;

   va_list ap;
   char err_string[1024] = {0};

   if ( context->data && context->size )
   {
      int to_copy = ( context->size > 1024 ) ? 1024 : context->size;
      strncpy_safe( err_string, context->data, to_copy );
   }
   // Load the arguments
   va_start( ap, format );
   vsnprintf( err_string, 1024 - strlen( err_string ), format, ap );
   va_end( ap );

   roadmap_log( ROADMAP_ERROR, TTS_LOG_STR( "TTS request error. Http request was failed with error = %s. Allocated: %d. Received: %d" ),
         err_string, context->alloc_size, context->size );

   _on_response( context, TTS_RES_STATUS_ERROR );

   // Free allocated space
   if ( context->data )
      free( context->data );
}

/*
 ******************************************************************************
 */
static void _request_done_cb(void *context_cb, char *last_modified, const char *format, ...  ) {
   WasRequestContext* context = (WasRequestContext*) context_cb;

   _on_response( context, TTS_RES_STATUS_SUCCESS );

   free( context->data );
}


/*
 ******************************************************************************
 * Record parser. Gets pointer to the data and tries to parse it to one record entry
 * If succeeded return the pointer to the next record. (Note can point to garbage)
 */
static void* _response_entry( void* this, int size, WasResponseEntry* entry )
{
   int text_size;
   int data_size;
   int remaining = size;
   char* p_next = (char*) this;

   if (this == NULL)
      return NULL;

   // Text size
   if ( remaining < 4 )
      return NULL;
   else
      remaining -= 4;
   // TODO:: Check endianess
   text_size = _get_int32( (unsigned char*) p_next, 1 );
   entry->text_size = text_size;

   // Text data
   if ( remaining < text_size )
      return NULL; // Not all the text is ready
   else
      remaining -= text_size;

   p_next += 4;   // Jump to text start
   entry->text = p_next; // Assign the text

   p_next += text_size;

   // Data buffer size and body
   if ( remaining < 4 )
      return NULL; // Data size is not ready
   else
      remaining -= 4;

   data_size = _get_int32( (unsigned char*) p_next, 1 );
   *p_next = 0; // Ensure null terminated text

   // Data body
   if ( remaining < data_size )
      return NULL; // Not all the text is ready

   p_next += 4;
   entry->data = p_next;
   p_next += data_size;

   entry->data_size = data_size;
   entry->total_size = text_size + data_size + 8;

   return p_next;
}

/*
 ******************************************************************************
 */
static void _buffer_from_entry( TtsSynthResponseData* response_data, const WasResponseEntry* entry )
{
   TtsData *data_ptr = &response_data->tts_data[response_data->count];
   data_ptr->data = malloc( entry->data_size );
   data_ptr->data_size = entry->data_size;
   memcpy( data_ptr->data, entry->data, entry->data_size );
}

/*
 ******************************************************************************
 */
static void _on_response( WasRequestContext* ctx, int res_status )
{
   if ( res_status == TTS_RES_STATUS_SUCCESS )
   {
      int received;
      // Call the process received one more time - for sure ( if progress was not called for the last chunk )
      received = _process_received( ctx );

      roadmap_log( ROADMAP_DEBUG, TTS_LOG_STR( "Received %d elements in http response." ), received );

      if ( ctx->response_cb )
         ctx->response_cb( ctx->cb_context, res_status, &ctx->response_data );

   }
   else
   {
      if ( ctx->response_cb )
        ctx->response_cb( ctx->cb_context, res_status, &ctx->response_data );
   }
   if ( ctx->query )
      free( ctx->query );
   ctx->busy = FALSE;
}

/*
 ******************************************************************************
 */
static int _process_received( WasRequestContext* context )
{
    void* next_data = context->data;
    int remaining = context->size;
    WasResponseEntry entry;
    TtsSynthResponseData *response_data = &context->response_data;
    int received_before = response_data->count;
    int count = 0;

#ifdef TTS_DEBUG
    {
       // Dump the buffer
       static const char* directory = NULL;
       char path[256];
       char path_suffix[TTS_PATH_MAXLEN];
       if ( !directory )
       {
          directory = roadmap_path_join( roadmap_path_tts(), "debug" );
          roadmap_path_create( directory );
       }

       // File name
       snprintf( path_suffix, sizeof( path_suffix ), "%s.dump", tts_utils_uid_str() );

       roadmap_path_format( path, sizeof( path ), directory, path_suffix );
       RoadMapFile file = roadmap_file_open( path, "w" );
       if ( ROADMAP_FILE_IS_VALID( file ) )
       {
          roadmap_file_write( file, context->data, context->size );
          roadmap_file_close( file );
          roadmap_log( ROADMAP_WARNING, TTS_LOG_STR( "Dumping buffer of %d bytes for query: %s. Path: %s" ), context->size, context->query, path );
       }
       else
       {
          roadmap_log( ROADMAP_ERROR, TTS_LOG_STR( "Error Dumping buffer of %d bytes for query: %s. Path: %s" ), context->size, context->query, path );
       }

    }
#endif
    // Parse the response
    while ( remaining > 0 )
    {
       next_data = _response_entry( next_data, remaining, &entry );

       // AGA DEBUG - Remove
//       roadmap_log( ROADMAP_WARNING, TTS_LOG_STR( "ENTRY DEBUG. Remaining: %d. Current entry (%d) text: %s, Text size: %d. Data size: %d. Total: %d. Next data: 0x%x" ),
//             remaining, count+1, entry.text, entry.text_size, entry.data_size, entry.total_size, next_data );

       if ( !next_data ) // Stop working - not enough data for one record
          break;

       count++;

       remaining -= entry.total_size;

       /*
        * Fill buffer
        */
       _buffer_from_entry( response_data, &entry );
       /*
        * Texts' list for reference
        */
       response_data->text_list[response_data->count] = entry.text;
       response_data->count++;

       roadmap_log( ROADMAP_DEBUG, TTS_LOG_STR( "Successfully received audio for text: %s" ), entry.text );

//       context->data_cursor = next_data;
    } // while

    // AGA DEBUG - Remove
//    roadmap_log( ROADMAP_WARNING, TTS_LOG_STR( "Parsed %d entries. Buffer size: %d. Query: %s" ), count, context->size, context->query );

    return ( response_data->count - received_before );
}

/*
 ******************************************************************************
 * Returns 32bit integer from the four bytes buffer.
 * Considers the endianess
 * TODO:: Move this to the utilities
 *
 */
static int _get_int32( unsigned char* src_data, int src_big_endian )
{
   int result = 0;
   int i = 0;
   int next_byte;
   for ( ; i < 4; ++i )
   {
      if ( src_big_endian )
         next_byte = *(src_data + 3 - i);
      else
         next_byte = *(src_data + i);

      next_byte <<= (8*i);
      result |= next_byte;
   }
   return result;
}

/*
 ******************************************************************************
 * Synth request callback. See TtsSynthRequestCb prototype
*/
static void _synth_request( const void* context, TtsTextList text_list, const TtsSynthRequestParams* params, TtsSynthResponseCb response_cb )
{
   WasRequestContext* ctx = _allocate_context();
   char query_prefix[1024];
   char types_list[(TTS_TEXT_TYPE_MAXLEN + 1 )*TTS_BATCH_REQUESTS_LIMIT + 6] = {0}; /* for types=type1|type2. 6 chars for type= and '\0' */
   int  query_size;
   int  len, i, strings_count = 0;
   const char *header;
   const char *text_type_str;
   const char* audio_content = roadmap_config_get( &WasProviderConfigAudioContentType );
   if ( !ctx )
   {
      if ( response_cb )
         response_cb( ctx->cb_context, TTS_RES_STATUS_ERROR, NULL );
      return;
   }

   ctx->cb_context = context;
   ctx->response_cb = response_cb;
   ctx->flags = params->flags;
   ctx->size = 0;

   query_prefix[0] = 0;

#ifdef TTS_DEBUG
   snprintf( query_prefix, sizeof( query_prefix ), "userid=%s&", RealTime_GetUserName() );
#endif

   /* Build the post query prefix */
   snprintf( query_prefix + strlen( query_prefix ), sizeof( query_prefix ) - strlen( query_prefix ),
             "request_id=%s&version=%d&sessionid=%d&cookie=%s&content_type=%s&id=%s&text=",
             tts_utils_uid_str(), WAS_TTS_PROTOCOL_VERSION, Realtime_GetServerId(), Realtime_GetServerCookie(), audio_content, params->voice_id );

   /* Build the text types list */
   for ( i = 0; i < TTS_BATCH_REQUESTS_LIMIT; ++i )
   {
      if ( !text_list[i] )
         continue;
      // Build types string
      text_type_str = tts_text_type_string( params->types_list[i] );
      len = strlen( types_list );
      if ( types_list[0] == 0 )
         snprintf( types_list, sizeof( types_list ), "&type=%s", text_type_str );
      else
         snprintf( types_list + len, sizeof( types_list ) - len, "|%s", text_type_str );
   }

   /*
    * Calculate the size of the query
    */
   query_size = ( strlen( query_prefix ) + strlen( types_list ) );
   for ( i = 0; i < TTS_BATCH_REQUESTS_LIMIT; ++i )
   {
      if ( text_list[i] )
      {
         query_size += ( strlen( text_list[i] ) + 1 /* For | or \0 */ );
         strings_count++;
      }
   }


   /*
    * Allocate and fill the query
    */
   if ( strings_count > 0 )
   {
      for ( i = 0; i < TTS_BATCH_REQUESTS_LIMIT; ++i )
      {
         if ( text_list[i] )
         {
            if ( ctx->query == NULL ) // First string
            {
               ctx->query = malloc( query_size );
               strcpy( ctx->query, query_prefix );
            }
            else
            {
               strcat( ctx->query, "|" );
            }
            strcat( ctx->query, text_list[i] );
         }
      } // for
      // Add the types
      strcat( ctx->query, types_list );

      roadmap_log( ROADMAP_DEBUG, TTS_LOG_STR( "Querying list of %d tts strings. Query body %s. Url: %s. Context index: %d" ),
            strings_count, ctx->query, roadmap_config_get( &WasProviderConfigWebService ), ctx->index );

      /*
       * Start the transaction "binary/octet-stream"
       * application/x-www-form-urlencoded; charset=utf-8
       *
       */
      header = roadmap_http_async_get_simple_header( "binary/octet-stream", strlen( ctx->query ) );
      roadmap_http_async_post( &sgHttpTtsRequestCBs, ctx,
                              roadmap_config_get( &WasProviderConfigWebService ), header, ctx->query, strlen( ctx->query ), HTTPCOPY_FLAG_IGNORE_CONTENT_LEN );
   }
   else
   {
      if ( response_cb )
         response_cb( ctx->cb_context, TTS_RES_STATUS_NO_INPUT_STRINGS, NULL );
      return;
   }
}

/*
 ******************************************************************************
 * Allocates context from the contexts pool
 */
static void _reset_context( WasRequestContext* ctx )
{
   memset( ctx, 0, sizeof( WasRequestContext ) );
}

/*
 ******************************************************************************
 * Allocates context from the contexts pool
 */
static WasRequestContext* _allocate_context( void )
{
   int i;
   WasRequestContext* ctx = NULL;

   for ( i = 0; i < TTS_ACTIVE_REQUESTS_LIMIT; ++i )
   {
      if ( !sgCtxPool[i].busy )
      {
         ctx = &sgCtxPool[i];
         _reset_context( ctx );

         ctx->busy = TRUE;
         break;
      }
   }

   if ( i == TTS_ACTIVE_REQUESTS_LIMIT )
   {
      roadmap_log( ROADMAP_ERROR, "There is on available context in WAS TTS request context pool!" );
   }
   return ctx;
}

/*
 ******************************************************************************
 * Initialization of the wass provider voices configuration
 * Auxiliary
 */
static void _voices_init( void )
{
   const char* update_time_str;
   time_t update_time = 0;
   WasVoicesHttpCtx *http_ctx = calloc( sizeof( WasVoicesHttpCtx ), 1 );

   /*
    * Configuration declarations
    */
   roadmap_config_declare ( "session", &WasProviderConfigVoicesUpdateTime, "", NULL );
   roadmap_config_declare ( "preferences", &WasProviderConfigVoicesUrl, WAS_TTS_CFG_VOICES_URL_DEFAULT, NULL );
   roadmap_config_declare ( "preferences", &WasProviderConfigVoicesSet, TTS_WAS_VOICES_SET_PRODUCTION, NULL );

   /*
    * Download configuration - if there is newer one. Start async download
    * If waiting for the configuration - force download
    */
   if ( !sgWaitingConfig )
   {
      update_time_str = roadmap_config_get( &WasProviderConfigVoicesUpdateTime );
      if ( update_time_str && update_time_str[0] )
         update_time = WDF_TimeFromModifiedSince( update_time_str );
   }

   roadmap_log( ROADMAP_DEBUG, "Posting download request for capabilities. Url: %s", _voices_cfg_url() );

   roadmap_http_async_copy( &sgHttpTtsVoicesCfgCBs, http_ctx, _voices_cfg_url(), update_time );
}

/*
******************************************************************************
* Called upon successful download of the voices configuration
* Auxiliary
*/
static BOOL _voices_save( void *data, size_t size, const char* last_modified )
{
   RoadMapFile file;
   char path[TTS_PATH_MAXLEN];
   const char *directory = roadmap_path_tts();
   BOOL res = TRUE;

   roadmap_log( ROADMAP_DEBUG, "Saving the voices configuration. Writing %d bytes, last modified = %s",
                                 size, last_modified );

   /*
    * Save last modified
    */
   roadmap_config_set ( &WasProviderConfigVoicesUpdateTime, last_modified );

   if ( !roadmap_file_exists( directory, "" ) )
   {
      roadmap_path_create ( directory );
   }

   roadmap_path_format( path, TTS_PATH_MAXLEN, directory, WAS_TTS_PROVIDER_VOICES );

   // Save the resource to the file
   file = roadmap_file_open( path, "w" );
   if ( file ) {
      roadmap_file_write( file, data, size );
      roadmap_file_close( file );
      res = TRUE;
   }
   else
   {
      roadmap_log( ROADMAP_ERROR, "Error opening file: %s", path );
      res = FALSE;
   }
   return res;
}

/*
******************************************************************************
* Voices download http size callback
* Auxiliary
*/
static int _voices_cfg_size_cb ( void *context_cb, size_t size )
{
   WasVoicesHttpCtx* context = (WasVoicesHttpCtx*) context_cb;

  // Allocate data storage
  if ( size > 0 )
  {
     context->data = malloc ( size );
     context->data_size_max = size;
     context->data_size = 0;
  }

  return size;
}

/*
******************************************************************************
* Voices download http progress callback
* Auxiliary
*/
static void _voices_cfg_progress_cb ( void *context_cb, char *data, size_t size )
{
   WasVoicesHttpCtx* context = (WasVoicesHttpCtx*) context_cb;

  // Store data
  if ( context->data )
  {
     if ( context->data_size + size > context->data_size_max )
     {
        size_t to_truncate = context->data_size + size - context->data_size_max;
        roadmap_log( ROADMAP_ERROR, "Cannot copy more bytes than allocated. Truncating %d bytes",
              to_truncate );
        size -= to_truncate;
     }
     memcpy( context->data + context->data_size, data, size );
     context->data_size += size;
  }
}

/*
******************************************************************************
* Voices download http error callback
* Auxiliary
*/
static void _voices_cfg_error_cb (void *context_cb,
              int connection_failure,
              const char *format,
              ...)
{
   WasVoicesHttpCtx* context = (WasVoicesHttpCtx*) context_cb;
   va_list ap;
   char err_string[1024];

   // Load the arguments
   va_start( ap, format );
   vsnprintf (err_string, 1024, format, ap);
   va_end( ap );

   if ( strstr( err_string, "304" ) )
      roadmap_log ( ROADMAP_WARNING, "TTS Voices download. Http response contains 'not modified' state" );
   else
      roadmap_log ( ROADMAP_ERROR, "TTS Voices download error. Http request was failed. Error: %s" , err_string );

   // Free allocated space
   if ( context->data )
   {
     free( context->data );
     context->data = NULL;
   }
   free( context );
}

/*
******************************************************************************
*/
static void _voices_cfg_done_cb ( void *context_cb, char *last_modified, const char *format, ...  )
{
  WasVoicesHttpCtx* context = (WasVoicesHttpCtx*) context_cb;
  char status[256];
  BOOL res;
  char* p = strstr( context->data, "\n" );

  size_t status_line_len = p - ( ( char* ) context->data );

  strncpy_safe( status, context->data, status_line_len );

  roadmap_log( ROADMAP_DEBUG, "TTS Voices download status: %s", status );

  res = _voices_save( p + 1, context->data_size - status_line_len - 1 /* '\n' */, last_modified );

  if ( res )
  {
     if ( sgWaitingConfig )
     {
        tts_register_provider( &sgWasProvider );
        roadmap_config_set_integer( &WasProviderConfigProtocolVersion, WAS_TTS_PROTOCOL_VERSION );
        sgWaitingConfig = FALSE;
     }
     else
     {
        // Load the voices file
        tts_update_provider( &sgWasProvider );
     }
  }

  free( context->data );
  free( context );
}


/*
******************************************************************************
*/
static const char* _voices_cfg_url ( void )
{
  static char url[WAS_TTS_URL_MAXSIZE];
  snprintf( url, WAS_TTS_URL_MAXSIZE, "%s?version=%d&set=%s", roadmap_config_get( &WasProviderConfigVoicesUrl ), WAS_TTS_PROTOCOL_VERSION,
        roadmap_config_get( &WasProviderConfigVoicesSet ) );
  return url;
}
