/* tts.c - implementation of the Text To Speech (TTS) interface layer.
 *                      Provides control for the external TTS engine execution
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
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include "roadmap.h"
#include "tts.h"
#include "roadmap_sound.h"
#include "tts_defs.h"
#include "tts_voices.h"
#include "tts_provider.h"
#include "tts_queue.h"
#include "tts_cache.h"
#include "tts_db.h"
#include "tts_ui.h"
#include "roadmap_path.h"
#include "roadmap_time.h"
#include "roadmap_hash.h"
#include "roadmap_config.h"
#include "roadmap_httpcopy_async.h"
#include "Realtime/Realtime.h"

//======================== Local defines ========================

#define TTS_CACHE_ENABLED                          1
#define TTS_VOICE_CHANGED_CB_MAX_NUM               16       //
#define TTS_COMPLETED_CB_MAX_NUM                   16       //
#define TTS_RETRY_COUNT                            3
#define TTS_REQUEST_INITIALIZER                    { {NULL}, {NULL}, 0, NULL, 0, {{0}}, 0, 0 }
//======================== Local types ========================

typedef struct
{
   void* completed_cb_ctx[TTS_COMPLETED_CB_MAX_NUM];                    // The context supplied by the tts engine user, passed to the completed_cb
   TtsRequestCompletedCb completed_cb[TTS_COMPLETED_CB_MAX_NUM];
   int completed_cb_count;
   const char* text;
   TtsTextType text_type;
   TtsPath     tts_path;             // Path if requested to store in file
   int flags;
   int retry_index;                  // Indicates request retry number. (Zero is the first request - not retry )
} TtsRequest;


typedef struct {
   unsigned int start_time;
   const char* voice_id;            // Voice id of the request

   int idx_list[TTS_QUEUE_SIZE]; // The list of queue indexes of the entries in the request
   int idx_list_count; // The count of queue indexes in the list
   BOOL        busy;
   int         index;
} TtsProviderRequest;

struct _TtsPlayList
{
   RoadMapSoundList sound_list;
   const char* voice_id;
};

//// Prefixes list
//static const char* sgFilterPrefixList[TTS_FILTER_PREFIX_MAX_NUM] = {NULL};
//static int sgFilterPrefixCount = 0;

// User request context pool
static TtsRequest sgUserCtxPool[TTS_ACTIVE_REQUESTS_LIMIT];
// Provider request context pool
static TtsProviderRequest sgProviderCtxPool[TTS_ACTIVE_REQUESTS_LIMIT];

// Tts engine state
static TtsProvider sgTtsProviders[TTS_PROVIDERS_MAX_NUM] = {TTS_PROVIDER_INITIALIZER};    // Zero bits
static const TtsProvider* sgActiveProvider = NULL;    // Current provider
static int sgActiveRequestsCount = 0;

//======================== Globals ========================




/*
 * Status attributes
 */
static int sgTtsFeatureEnabled = 0;                         // Feature
static const char* sgTtsVoiceId = NULL;
static const char* sgTtsDefaultVoiceId = NULL;
static TtsOnVoiceChangedCb sgOnVoiceChangedCBs[TTS_VOICE_CHANGED_CB_MAX_NUM] = {NULL};

static RoadMapConfigDescriptor RMConfigTTSEnabled =
      ROADMAP_CONFIG_ITEM( TTS_CFG_CATEGORY, TTS_CFG_FEATURE_ENABLED );
static RoadMapConfigDescriptor RMConfigTTSVoiceId =
      ROADMAP_CONFIG_ITEM( TTS_CFG_CATEGORY, TTS_CFG_VOICE_ID );
static RoadMapConfigDescriptor RMConfigTTSDefaultVoiceId =
      ROADMAP_CONFIG_ITEM( TTS_CFG_CATEGORY, TTS_CFG_DEFAULT_VOICE_ID );



//======================== Local Declarations ========================

//static const char* tts_cache_get(const char* text);
//static const TtsCacheEntry* tts_cache_add( const char* text, const char* file_name );
static void _queue_one( const char* text, TtsTextType text_type, TtsRequestCompletedCb completed_cb, void* user_context, int flags );
static void _on_response( void* context, int res_status, TtsSynthResponseData* response_data );

static BOOL _process_cached( const TtsRequest* request_ctx );
static int _process_received( TtsProviderRequest* provider_request, TtsSynthResponseData* response_data );

static int _process_error( const TtsProviderRequest* provider_request );
static const char* _parse_text( const char* text );
static const TtsProvider* _voice_service_provider( const char* voice_id );
static void _provider_ctx_init( void );
static TtsProviderRequest* _provider_ctx_allocate( void );
static void _provider_ctx_free( TtsProviderRequest* ctx );
static void _voice_updated( const TtsVoice* new_voice, const TtsVoice* old_voice );
static void _voice_changed( const char* voice_id, BOOL force_recommit );

static void _run_request_callbacks( const TtsRequest* request, int status );
static void _add_request_callback( TtsRequest* request, TtsRequestCompletedCb cb, void* cb_ctx );
static void _clear_noncacheable( void );
static BOOL _apply_retry( TtsRequest* tts_request, int queue_idx, int* status );

void tts_initialize( void )
{
   // Load the configuraiton
   tts_load_config();

   // Set the comfiguration to the Server defined
   if ( !sgTtsVoiceId[0] || !strcmp( sgTtsVoiceId, TTS_CFG_VOICE_ID_NOT_DEFINED ) )
   {
      roadmap_config_set( &RMConfigTTSVoiceId, sgTtsDefaultVoiceId );
      sgTtsVoiceId = sgTtsDefaultVoiceId;
   }

   // Voices initialization
   tts_voices_initialize( _voice_updated );

   // Queue initialization
   tts_queue_init();

   // Cache initialization
   tts_cache_initialize();

   // Provider context pool initialization
   _provider_ctx_init();

   if ( !tts_feature_enabled() )
      return;

   // Find active provider
   if ( !( sgActiveProvider = _voice_service_provider( sgTtsVoiceId ) ) )
   {
      roadmap_log( ROADMAP_WARNING, TTS_LOG_STR( "Provider is not registered for current voice id: %s." ), sgTtsVoiceId );
      sgTtsVoiceId = sgTtsDefaultVoiceId;
      if ( !( sgActiveProvider = _voice_service_provider( sgTtsVoiceId ) ) )
      {
         roadmap_log( ROADMAP_ERROR, TTS_LOG_STR( "Critical TTS Engine error. Provider is not registered for default voice id: %s." ), sgTtsVoiceId );
         return;
      }
   }

   if ( sgActiveProvider->prepare_cb )
      sgActiveProvider->prepare_cb();

   sgActiveRequestsCount = 0;

   // Cache voice
   tts_cache_set_voice( sgTtsVoiceId, sgActiveProvider->storage_type );
}

/*
 ******************************************************************************
 */
void tts_load_config( void )
{
   static BOOL initialized = FALSE;

   if ( !initialized )
   {
      /*
       * Configuration declarations
       */
      roadmap_config_declare("preferences", &RMConfigTTSEnabled,
            TTS_CFG_FEATURE_ENABLED_DEFAULT, NULL);
      roadmap_config_declare("preferences", &RMConfigTTSDefaultVoiceId,
            TTS_CFG_VOICE_ID_DEFAULT, NULL);
      roadmap_config_declare( "user", &RMConfigTTSVoiceId, TTS_CFG_VOICE_ID_NOT_DEFINED, NULL );
      initialized = TRUE;
   }

   /*
    * Load state from the configuration
    */
   sgTtsDefaultVoiceId = roadmap_config_get( &RMConfigTTSDefaultVoiceId );
   sgTtsFeatureEnabled = !strcmp( "yes", roadmap_config_get( &RMConfigTTSEnabled ) );
   sgTtsVoiceId = roadmap_config_get( &RMConfigTTSVoiceId );
}

/*
 ******************************************************************************
 */
void tts_set_voice( const char* voice_id )
{
   const TtsProvider* provider;

   if ( !tts_feature_enabled() )
      return;

   if ( !voice_id || ( sgActiveProvider && !strcmp( voice_id, sgTtsVoiceId ) ) )
      return;

   roadmap_log( ROADMAP_INFO, TTS_LOG_STR( "Voice change request: %s => %s" ), SAFE_STR( sgTtsVoiceId ), SAFE_STR( voice_id ) );

   // Find active provider
   if ( !( provider = _voice_service_provider( voice_id ) ) )
   {
      roadmap_log( ROADMAP_ERROR, TTS_LOG_STR( "Critical TTS Engine error. Provider is not registered for voice id: %s." ), sgTtsVoiceId );
      return;
   }

   if ( !TTS_VOICE_VALID( tts_voices_get( voice_id, NULL ) ) )
   {
      roadmap_log( ROADMAP_WARNING, TTS_LOG_STR( "Current voice %s is not valid. Server may not support the requests" ), voice_id );
   }

   sgActiveProvider = provider;
   sgActiveRequestsCount = 0;

   if ( sgActiveProvider->prepare_cb )
      sgActiveProvider->prepare_cb();

   sgTtsVoiceId = voice_id;

   tts_cache_set_voice( sgTtsVoiceId, sgActiveProvider->storage_type );

   roadmap_config_set( &RMConfigTTSVoiceId, sgTtsVoiceId );

   _voice_changed( sgTtsVoiceId, FALSE );

}

/*
 ******************************************************************************
 */
void tts_shutdown( void )
{
   if ( !tts_enabled() )
        return;

   tts_queue_shutdown();

   _clear_noncacheable();
}

/*
 ******************************************************************************
 */
void tts_request( const char* text )
{
   tts_request_ex( text, __tts_text_type_default, NULL, NULL, TTS_FLAG_NONE );
}
/*
 ******************************************************************************
 */
void tts_request_ex( const char* text, TtsTextType text_type, TtsRequestCompletedCb completed_cb, void* user_context, int flags )
{
   const char *parsed_text;

   if ( !tts_enabled() )
   {
      roadmap_log(ROADMAP_WARNING, TTS_LOG_STR( "TTS is disabled cannot post synthesize request for %s." ), text );
      return;
   }

   if ( !text || !text[0] )
   {
      if ( completed_cb )
      {
         completed_cb( user_context, TTS_RES_STATUS_NULL_TEXT, "NULL" );
      }
      roadmap_log( ROADMAP_WARNING, TTS_LOG_STR( "NULL or empty text cannot be requested!" ) );
      return;
   }

   parsed_text = _parse_text( text );

   if ( tts_cache_exists( parsed_text, sgTtsVoiceId ) )
   {
      TtsRequest tts_request = TTS_REQUEST_INITIALIZER;
      tts_request.flags = flags;
      tts_request.text = parsed_text;
      _add_request_callback( &tts_request, completed_cb, user_context );

      _process_cached( &tts_request );
   }
   else
   {
      _queue_one( strdup( parsed_text ), text_type, completed_cb, user_context, flags );

      if ( sgActiveProvider->batch_request_limit == 1 )
      {
         // Commit immediately
         tts_commit();
      }
   }
}
/*
 ******************************************************************************
 */
BOOL tts_register_provider( const TtsProvider* tts_provider )
{
   int i;
   BOOL result = TRUE;
   for( i = 0; i < TTS_PROVIDERS_MAX_NUM; ++i )
   {
      if ( !sgTtsProviders[i].registered )
      {
         sgTtsProviders[i] = *tts_provider; // Bit copy.
         sgTtsProviders[i].registered = TRUE;
         tts_voices_update( sgTtsProviders[i].provider_name, sgTtsProviders[i].voices_cfg );
         tts_ui_initialize();

         roadmap_log( ROADMAP_INFO, TTS_LOG_STR( "Provider %s was registered successfully" ), tts_provider->provider_name );
         break;
      }
   }
   if ( i == TTS_PROVIDERS_MAX_NUM )
   {
      roadmap_log( ROADMAP_ERROR, TTS_LOG_STR( "Cannot register more providers. Maximum: %d" ), TTS_PROVIDERS_MAX_NUM );
      result = FALSE;
   }
   return result;
}
/*
 ******************************************************************************
 */
BOOL tts_update_provider( const TtsProvider* tts_provider )
{
   int i;
   BOOL result = TRUE;
   for( i = 0; i < TTS_PROVIDERS_MAX_NUM; ++i )
   {
      if ( sgTtsProviders[i].registered &&
            !strcmp( sgTtsProviders[i].provider_name, tts_provider->provider_name ) )
      {
         sgTtsProviders[i] = *tts_provider; // Bit copy.
         sgTtsProviders[i].registered = TRUE;
         tts_voices_update( sgTtsProviders[i].provider_name, sgTtsProviders[i].voices_cfg );

         roadmap_log( ROADMAP_INFO, TTS_LOG_STR( "Provider %s was updated successfully" ), tts_provider->provider_name );
         break;
      }
   }
   if ( i == TTS_PROVIDERS_MAX_NUM )
   {
      roadmap_log( ROADMAP_ERROR, TTS_LOG_STR( "Cannot find registered provider with name %s" ), tts_provider->provider_name );
      result = FALSE;
   }
   else
   {
      const char* voice_id = sgTtsVoiceId;
      if ( sgTtsVoiceId )
      {
         const TtsVoice* voice;
         voice = tts_voices_get( sgTtsVoiceId, NULL );
         // If not valid voice - replace to default
         if ( !TTS_VOICE_VALID( voice ) )
         {
            roadmap_log( ROADMAP_WARNING, TTS_LOG_STR( "Voice %s is invalidated. Trying to set the default one" ), voice->voice_id );
            voice_id = sgTtsDefaultVoiceId;
         }
      }
      else
      {
         roadmap_log( ROADMAP_WARNING, TTS_LOG_STR( "There is no voice defined. Setting the default one" ) );
         voice_id = sgTtsDefaultVoiceId;
      }

      tts_set_voice( voice_id );
      tts_ui_initialize();
   }

   return result;
}
/*
 ******************************************************************************
 */
BOOL tts_commit( void )
{
   TtsProviderRequest *provider_request= NULL;
   TtsRequest *tts_request;
   int i;
   const char* text;
   TtsTextList text_list = TTS_GENERIC_LIST_INITIALIZER;
   int queue_idx;
   int idx_list[TTS_QUEUE_SIZE];
   int idx_list_count;
   TtsSynthRequestParams params = TTS_SYNTH_PARMAS_INITIALIZER;

   if ( !tts_enabled() )
   {
      roadmap_log(ROADMAP_WARNING, TTS_LOG_STR( "TTS is not enabled. Cannot commit synthesize requests. Feature: %d, Provider: %d" ),
            sgTtsFeatureEnabled, ( sgActiveProvider != NULL ) );
      return FALSE;
   }

   if ( sgActiveProvider->concurrent_limit >= 0 && sgActiveRequestsCount >= sgActiveProvider->concurrent_limit )
   {
      roadmap_log( ROADMAP_WARNING, TTS_LOG_STR( "Overflow in concurrent requests for the provider. Maximum: %d" ),
                   sgActiveProvider->concurrent_limit );
      return FALSE;
   }

   idx_list_count = tts_queue_get_indexes( idx_list,
         MIN( TTS_QUEUE_SIZE, sgActiveProvider->batch_request_limit ), TTS_QUEUE_STATUS_IDLE );


   if ( idx_list_count == 0 )
   {
      roadmap_log( ROADMAP_DEBUG, TTS_LOG_STR( "There are no strings to commit." ) );
      return TRUE;
   }

   /*
    * Parameters
    */
   params.voice_id = sgTtsVoiceId;

   /*
    * Building the context.
    */
   provider_request = _provider_ctx_allocate();
   if ( provider_request == NULL )
   {
      roadmap_log( ROADMAP_ERROR, TTS_LOG_STR( "Error allocating provider context!" ) );
      return FALSE;
   }
   provider_request->start_time = roadmap_time_get_millis();
   provider_request->voice_id = sgTtsVoiceId;
   memcpy( provider_request->idx_list, idx_list, sizeof( idx_list[0] ) * idx_list_count );
   provider_request->idx_list_count = idx_list_count;

   for ( i = 0; i < idx_list_count ; ++i )
   {
      queue_idx = provider_request->idx_list[i];

      text = tts_queue_get_key( queue_idx );
      // Set status
      tts_queue_set_status( provider_request->idx_list[i], TTS_QUEUE_STATUS_COMMITTED );
      // Fill the text list
      text_list[i] = text;
      // Fill the path list
      if ( sgActiveProvider->storage_type & __tts_db_data_storage__file )
      {
         tts_request = ( TtsRequest * ) tts_queue_get_context( queue_idx );
         tts_db_generate_path( sgTtsVoiceId, &tts_request->tts_path );
         params.types_list[i] = tts_request->text_type;
         params.path_list[i] = &tts_request->tts_path;
      }
   }

   roadmap_log( ROADMAP_DEBUG, TTS_LOG_STR( "Committing list of %d tts strings." ), i );
   // Send request to the provider
   sgActiveProvider->request_cb( provider_request, text_list, &params, ( TtsSynthResponseCb ) _on_response );
   sgActiveRequestsCount++;

   return TRUE;
}

/*
 ******************************************************************************
 */
int tts_enabled(void)
{
   return ( sgTtsFeatureEnabled && sgActiveProvider );
}

/*
 ******************************************************************************
 */
int tts_feature_enabled( void )
{
   return sgTtsFeatureEnabled;
}
/*
 ******************************************************************************
 */
void tts_set_feature_enabled( BOOL value )
{
   sgTtsFeatureEnabled = value;
   if ( sgTtsFeatureEnabled )
      roadmap_config_set( &RMConfigTTSEnabled, "yes" );
   else
      roadmap_config_set( &RMConfigTTSEnabled, "no" );
}

/*
 ******************************************************************************
 */
const char* tts_text_type_string( TtsTextType text_type )
{
   const char* type_string = TTS_TEXT_TYPE_STR_DEFAULT;
   switch ( text_type )
   {
      case __tts_text_type_default:
         type_string = TTS_TEXT_TYPE_STR_DEFAULT;
         break;
      case __tts_text_type_street:
         type_string = TTS_TEXT_TYPE_STR_STREET;
         break;
   }

   return type_string;
}

/*
 ******************************************************************************
 */
void tts_register_on_voice_changed( TtsOnVoiceChangedCb cb )
{
   int i;
   for ( i = 0; i < TTS_VOICE_CHANGED_CB_MAX_NUM; ++i )
   {
      if ( sgOnVoiceChangedCBs[i] == NULL )
         break;
   }
   if ( i == TTS_VOICE_CHANGED_CB_MAX_NUM )
   {
      roadmap_log( ROADMAP_ERROR, TTS_LOG_STR( "Maximum number of callbacks (on voice changed) was exceeded" ) );
   }
   else
   {
      sgOnVoiceChangedCBs[i] = cb;
   }
}
/*
BOOL tts_register_fiter_prefix( const char* prefix )
{
   if ( sgFilterPrefixCount == TTS_FILTER_PREFIX_MAX_NUM || !prefix )
   {
      roadmap_log( ROADMAP_ERROR, TTS_LOG_STR( "Error adding the prefix: %s. Current number: %d(%d)" ),
            SAFE_STR( prefix ), sgFilterPrefixCount, TTS_FILTER_PREFIX_MAX_NUM );
      return FALSE;
   }

   sgFilterPrefixList[sgActiveRequestsCount++] = prefix;

   return TRUE;
}
*/

/*
 ******************************************************************************
 */

void tts_abort(void)
{
   // TODO:: Add the abort implementation
}

/*
 ******************************************************************************
 */
const char* tts_voice_id( void )
{
   return sgTtsVoiceId;
}


/*
 ******************************************************************************
 */
const TtsPath* tts_get_path( const char* text, const char* voice_id )
{
   static TtsPath sCachedPath;

   if ( tts_cache_get( text, voice_id, NULL, &sCachedPath ) )
      return &sCachedPath;
   else
      return NULL;
}

/*
 ******************************************************************************
 */
const TtsData* tts_get_data( const char* text, const char* voice_id )
{
   static TtsData sCachedData;
   if ( tts_cache_get( text, voice_id, &sCachedData, NULL ) )
      return &sCachedData;
   else
      return NULL;
}


/*
 ******************************************************************************
 */
void tts_clear_cache( void )
{
   tts_cache_clear( NULL );
}
/*
 ******************************************************************************
 */
static const TtsProvider* _voice_service_provider( const char* voice_id )
{
   int i;
   const TtsProvider* provider = NULL;
   const TtsVoice* voice = tts_voices_get( voice_id, NULL );

   if ( !voice )
      return NULL;

   for( i = 0; i < TTS_PROVIDERS_MAX_NUM; ++i )
   {
      if ( !strcmp( sgTtsProviders[i].provider_name, voice->service_provider ) )
      {
         provider = &sgTtsProviders[i];
         break;
      }
   }
   return provider;
}

/*
 ******************************************************************************
 */
TtsPlayList tts_playlist_create( const char* voice_id )
{
   RoadMapSoundList sound_list = NULL;
   TtsPlayList play_list = NULL;

   if ( tts_enabled() )
   {
      if ( voice_id == NULL )
         voice_id = sgTtsVoiceId;

      const TtsProvider* provider = _voice_service_provider( voice_id );
      switch ( provider->storage_type )
      {
         case __tts_db_data_storage__file:
         {
            sound_list = roadmap_sound_list_create( 0 );
            break;
         }
         case __tts_db_data_storage__blob_record:
         {
            sound_list = roadmap_sound_list_create( SOUND_LIST_BUFFERS );
            break;
         }
         default:
         {
            roadmap_log( ROADMAP_WARNING, "Data storage type %d is not supported" );
         }
      }
      if ( sound_list )
      {
         play_list = malloc( sizeof( TtsPlayList ) );
         play_list->sound_list = sound_list;
         play_list->voice_id = voice_id;
      }
   }
   return play_list;
}
/*
 ******************************************************************************
*/
BOOL tts_playlist_add( TtsPlayList list, const char* text )
{
   int result = -1;
   const TtsProvider* provider = NULL;

   if ( !tts_enabled() )
      return FALSE;

   if ( !text )
   {
      roadmap_log( ROADMAP_ERROR, TTS_LOG_STR( "Cannot add NULL texts to the playlist" ) );
      return FALSE;
   }

   provider = _voice_service_provider( list->voice_id );
   switch ( provider->storage_type )
   {
      case __tts_db_data_storage__file:
      {
         TtsPath path;
         const char* parsed_text = _parse_text( text );
         if ( tts_cache_get( parsed_text, list->voice_id, NULL, &path ) )
         {
            result = roadmap_sound_list_add( list->sound_list, path.path );
            roadmap_log( ROADMAP_DEBUG, TTS_LOG_STR( "Adding text %s (Voice: %s). Path: %s to the playlist. Status: %d" ),
                  parsed_text, list->voice_id, path.path, result );

            if ( result == SND_LIST_ERR_NO_FILE )
            {
               const TtsDbEntry* db_entry = tts_db_entry( list->voice_id, parsed_text, NULL );
               TtsTextType text_type = tts_db_text_type( db_entry );

               roadmap_log( ROADMAP_WARNING, TTS_LOG_STR( "File %s doesn't exist!!!. Removing the entry!" ), path.path );
               tts_cache_remove( parsed_text, list->voice_id, __tts_db_data_storage__file );
               // Post new request if matches the current voice
               if ( !list->voice_id || !strcmp( list->voice_id, sgTtsVoiceId ) )
               {
                  tts_request_ex( parsed_text, text_type, NULL, NULL, TTS_FLAG_NONE );
                  tts_commit(); // Commit immediately in this case
               }
            }
         }
         break;
      }
      case __tts_db_data_storage__blob_record:
      {
         TtsData data;
         if ( tts_cache_get( _parse_text( text ), list->voice_id, &data, NULL ) )
            result = roadmap_sound_list_add_buf( list->sound_list, data.data, data.data_size );
         break;
      }
      default:
      {
         roadmap_log( ROADMAP_WARNING, "Data storage type %d is not supported" );
      }
   }

   return ( result >= 0 );
}
/*
 ******************************************************************************
 */
BOOL tts_playlist_play( TtsPlayList list )
{
   int result = -1;

   result = roadmap_sound_play_list( list->sound_list );

   free( list );

   return ( result == 0 );
}
/*
 ******************************************************************************
 */
RoadMapSoundList tts_playlist_export_list( TtsPlayList list, BOOL free_tts_playlist )
{
   RoadMapSoundList sl = list->sound_list;

   if ( free_tts_playlist )
      free( list );

   return sl;
}
/*
 ******************************************************************************
 */
void tts_playlist_free( TtsPlayList list )
{
   roadmap_sound_list_free( list->sound_list );
   free( list );
}


/*
 ******************************************************************************
*/
BOOL tts_text_available( const char* text, const char* voice_id )
{
   BOOL result = tts_enabled() && tts_cache_exists( _parse_text( text ), voice_id );

   return result;
}

/*
 ******************************************************************************
 */
static void _on_response( void* context, int res_status, TtsSynthResponseData* response_data )
{
   TtsProviderRequest *provider_request = (TtsProviderRequest*) context;
   unsigned int delta = roadmap_time_get_millis() - provider_request->start_time;
   int matched_count;

   roadmap_log( ROADMAP_DEBUG, TTS_LOG_STR( "Request was completed in %d ms. Status: %d" ), delta, res_status );

   if ( res_status == TTS_RES_STATUS_SUCCESS )
   {
      // Call the process received one more time - for sure ( if progress was not called for the last chunk )
      matched_count = _process_received( provider_request, response_data );

      roadmap_log( ROADMAP_DEBUG, TTS_LOG_STR( "Received %d elements in tts response. Matched: %d. Requested: %d" ),
            response_data->count, matched_count, provider_request->idx_list_count );

   }
   else
   {
      roadmap_log( ROADMAP_ERROR, TTS_LOG_STR( "Error in response" ) );
      _process_error( provider_request );
   }

   // Deallocate the context
   _provider_ctx_free( provider_request );

   sgActiveRequestsCount--;

   // Commit the remaining entries in the queue if existing
   tts_commit();
}

/*
 ******************************************************************************
 */
static int _process_received( TtsProviderRequest* provider_request, TtsSynthResponseData* response_data )
{
    int queue_idx;
    BOOL apply_retry = FALSE;
    int i, j, status, matched_count = 0;
    const char* received_text, *queued_text;
    TtsRequest* tts_request;

    // Parse the response
    for ( i = 0; i < provider_request->idx_list_count; ++i )
    {

       queue_idx = provider_request->idx_list[i];

       if ( tts_queue_is_empty( queue_idx ) )
          continue;

       tts_request = tts_queue_get_context( queue_idx );
       status = tts_queue_get_status( queue_idx );
       /*
        * Test status
        */
       if ( status != TTS_QUEUE_STATUS_COMMITTED )
       {
          roadmap_log( ROADMAP_ERROR, TTS_LOG_STR( "Internal error. Current status %d differs from 'COMMITTED' (%d)" ),
                         status, TTS_QUEUE_STATUS_COMMITTED );
       }

       queued_text = tts_queue_get_key( queue_idx );

       // Look for the text in the list
       for ( j = 0; j < TTS_BATCH_REQUESTS_LIMIT; ++j )
       {
          received_text = response_data->text_list[j];
          if ( received_text && !strcmp( received_text, queued_text ) )
          {
             matched_count++;
             break;
          }
       }

       if ( j == TTS_BATCH_REQUESTS_LIMIT )
       {
          int res_status = TTS_RES_STATUS_ERROR;
          roadmap_log( ROADMAP_ERROR, TTS_LOG_STR( "The text %s was not synthesized." ), queued_text );

          apply_retry = _apply_retry( tts_request, queue_idx, &res_status );

          _run_request_callbacks( tts_request, res_status );
       }
       else
       {
          TtsData* tts_data = &response_data->tts_data[i];
          /*
           * The final result is stored in cache
           */
          tts_cache_add( provider_request->voice_id, queued_text, tts_request->text_type, sgActiveProvider->storage_type,
                   tts_data, &tts_request->tts_path );

          _run_request_callbacks( tts_request, TTS_RES_STATUS_SUCCESS );

          roadmap_log( ROADMAP_DEBUG, TTS_LOG_STR( "Successfully received audio for text: %s. Voice: %s. Path: %s" ),
                queued_text, provider_request->voice_id, tts_request->tts_path.path );

          // Deallocate the buffers if not needed
          if ( !TTS_CACHE_BUFFERS_ENABLED && tts_data->data )
          {
             free( tts_data->data );
          }
       }
       if ( !apply_retry )
       {
          tts_queue_remove( queue_idx );
          free( (char*) queued_text );
       }
    } // for i

    return matched_count;
}

/*
 ******************************************************************************
 */
static BOOL _apply_retry( TtsRequest* tts_request, int queue_idx, int* status )
{
   BOOL apply_retry = FALSE;

   if ( tts_request->flags & TTS_FLAG_RETRY )
   {
      if ( tts_request->retry_index < TTS_RETRY_COUNT )
      {
         apply_retry = TRUE;
         *status |= TTS_RES_STATUS_RETRY_ON;
         tts_request->retry_index++;
         tts_queue_set_status( queue_idx, TTS_QUEUE_STATUS_IDLE );
         roadmap_log( ROADMAP_WARNING, TTS_LOG_STR( "Applying retry #%d ( %d ) for text: %s" ), tts_request->retry_index, TTS_RETRY_COUNT, tts_request->text );
      }
      else
      {
         *status |= TTS_RES_STATUS_RETRY_EXHAUSTED;
         roadmap_log( ROADMAP_WARNING, TTS_LOG_STR( "Retries number exhausted #%d ( %d ) for text: %s. Giving up ... " ), tts_request->retry_index, TTS_RETRY_COUNT, tts_request->text );
      }
   }

   return apply_retry;
}
/*
 ******************************************************************************
 */
static BOOL _process_cached( const TtsRequest* request_ctx )
{
   BOOL result = TRUE;

   roadmap_log( ROADMAP_DEBUG, TTS_LOG_STR( "Processing cached element: %s" ), request_ctx->text );

   _run_request_callbacks( request_ctx, TTS_RES_STATUS_SUCCESS );

   return result;
}

/*
 ******************************************************************************
 */
static int _process_error( const TtsProviderRequest* provider_request )
{
   int queue_idx, i, res_status = TTS_RES_STATUS_ERROR;
   int count = 0;
   TtsRequest* tts_request;
   char* text;
   BOOL apply_retry = FALSE;

   for ( i = 0, count = 0; i < provider_request->idx_list_count; ++i )
   {
      queue_idx = provider_request->idx_list[i];
      if ( tts_queue_is_empty( queue_idx ) )
         continue;

      tts_request = tts_queue_get_context( queue_idx );

      apply_retry = _apply_retry( tts_request, queue_idx, &res_status );

      _run_request_callbacks( tts_request, res_status );

      roadmap_log( ROADMAP_WARNING, TTS_LOG_STR( "Error reported for text: %s" ), tts_request->text );

      if ( !apply_retry )
      {
         text = (char*) tts_queue_get_key( queue_idx );
         tts_queue_remove( queue_idx );

         free( text );
      }
      count++;
   }

   return count;
}

/*
 ******************************************************************************
 * Queue one TTS request.
 * Auxiliary
 */
static void _queue_one( const char* text, TtsTextType text_type, TtsRequestCompletedCb completed_cb, void* user_context, int flags )
{
   // Check if already requested in the queue
   int queue_idx;

   roadmap_log( ROADMAP_DEBUG, TTS_LOG_STR( "Adding text %s to the queue!" ), text );

   if ( text == NULL )
   {
      roadmap_log( ROADMAP_ERROR, TTS_LOG_STR( "Cannot prepare NULL text." ) );
      return;
   }

   queue_idx = tts_queue_get_index( text );
   if (queue_idx >= 0) {
      TtsRequest* request = tts_queue_get_context( queue_idx );

      _add_request_callback( request, completed_cb, user_context );

      return;
   }

   // Queue. Text duplicated in parser
   queue_idx = tts_queue_add( NULL, text );

   // Set the context
   sgUserCtxPool[queue_idx].completed_cb_count = 0;
   sgUserCtxPool[queue_idx].flags = flags;
   sgUserCtxPool[queue_idx].text = text;
   sgUserCtxPool[queue_idx].text_type = text_type;
   sgUserCtxPool[queue_idx].tts_path.path[0] = 0;
   sgUserCtxPool[queue_idx].retry_index = 0;
   _add_request_callback( &sgUserCtxPool[queue_idx], completed_cb, user_context );

   tts_queue_set_context( queue_idx, &sgUserCtxPool[queue_idx] );
}
/*
 ******************************************************************************
 * Queue one TTS request.
 * Auxiliary
 */
static const char* _parse_text( const char* text )
{
   static char parsed_text[TTS_TEXT_MAX_LENGTH];
   char* pCh = parsed_text;

   strncpy_safe( parsed_text, text, TTS_TEXT_MAX_LENGTH );

   for( ; *pCh; pCh++ )
   {
      if ( *pCh == '|' )
      {
         *pCh = ' ';
      }

//      if ( *pCh == ',' || *pCh == '.' )
//         continue;
//      if ( *pCh >= '0' && *pCh <= '9' )
//         continue;
//      if ( *pCh >= 'A' && *pCh <= 'Z' )
//         continue;
//      if ( *pCh >= 'a' && *pCh <= 'z' )
//         continue;
//
//      // Otherwise replace by space
//      *pCh = ' ';
   }
   return parsed_text;
}

#if 0
/*
 ******************************************************************************
 * Queue one TTS request.
 * Auxiliary
 */
static void _free_text_list( TtsTextList text_list )
{
   int i = 0;

   for( ; i < TTS_BATCH_REQUESTS_LIMIT; ++i )
   {
      if ( !text_list[i] )
         break;

      free( (char*) text_list[i] );
   }
}
#endif

/*
 ******************************************************************************
 * Initializes context of the contexts pool
 */
static void _provider_ctx_init( void )
{
   int i;

   for ( i = 0; i < TTS_ACTIVE_REQUESTS_LIMIT; ++i )
   {
      sgProviderCtxPool[i].busy = FALSE;
      sgProviderCtxPool[i].index = i;
      sgProviderCtxPool[i].idx_list_count = 0;
      sgProviderCtxPool[i].start_time = 0;
      sgProviderCtxPool[i].voice_id = NULL;
   }
}

/*
 ******************************************************************************
 * Allocates context from the contexts pool
 */
static TtsProviderRequest* _provider_ctx_allocate( void )
{
   int i;
   TtsProviderRequest* ctx = NULL;

   for ( i = 0; i < TTS_ACTIVE_REQUESTS_LIMIT; ++i )
   {
      if ( !sgProviderCtxPool[i].busy )
      {
         ctx = &sgProviderCtxPool[i];
         ctx->busy = TRUE;
         break;
      }
   }
   if ( i == TTS_ACTIVE_REQUESTS_LIMIT )
   {
      roadmap_log( ROADMAP_ERROR, "The TTS provider request pool is full!" );
   }
   return ctx;
}

/*
 ******************************************************************************
 * Deallocates context from the contexts pool
 */
static void _provider_ctx_free( TtsProviderRequest* ctx )
{
   ctx->busy = TRUE;
}


/*
 ******************************************************************************
 * Handles voice data update
 */
static void _voice_updated( const TtsVoice* new_voice, const TtsVoice* old_voice )
{
   if ( new_voice == NULL )
      return;

   if ( old_voice )
   {
      if ( new_voice->version > old_voice->version )
      {
         roadmap_log( ROADMAP_WARNING, TTS_LOG_STR( "Voice: %s version is incremented ( %d =>> %d ). Clearing the database. Current voice: %s" ),
               new_voice->voice_id, old_voice->version, new_voice->version, sgTtsVoiceId );
         tts_cache_clear( new_voice->voice_id );

         // Force voice change for the current voice
         if ( sgTtsVoiceId && !strcmp( sgTtsVoiceId, new_voice->voice_id ) )
         {
            _voice_changed( sgTtsVoiceId, TRUE );
         }
      }
   }
}

/*
 ******************************************************************************
 * Calls the voice changed callbacks
 */
static void _voice_changed( const char* voice_id, BOOL force_recommit )
{
   int i;
   TtsOnVoiceChangedCb cb;
   for ( i = 0; i < TTS_VOICE_CHANGED_CB_MAX_NUM; ++i )
   {
      cb = sgOnVoiceChangedCBs[i];
      if (  cb != NULL )
         cb( voice_id, force_recommit );
   }
}


/*
 ******************************************************************************
 * Sets the callback for the supplied request
 */
static void _add_request_callback( TtsRequest* request, TtsRequestCompletedCb cb, void* cb_ctx )
{

   if ( request->completed_cb_count == TTS_COMPLETED_CB_MAX_NUM )
   {
      roadmap_log( ROADMAP_WARNING, TTS_LOG_STR( "Can't register more than %d callbacks for text: %s" ), TTS_COMPLETED_CB_MAX_NUM, request->text );
      return;
   }
   if ( cb )
   {
      request->completed_cb[request->completed_cb_count] = cb;
      request->completed_cb_ctx[request->completed_cb_count] = cb_ctx;
      request->completed_cb_count++;
   }

}

/*
 ******************************************************************************
 * Runs the registered callbacks
 */
static void _run_request_callbacks( const TtsRequest* request, int status )
{
   int i;
   TtsRequestCompletedCb cb;

   // If in retry state and retry callback flag is off - don't call callbacks
   if ( ( request->retry_index > 1 ) && !( request->flags & TTS_FLAG_RETRY_CALLBACK ) )
      return;

   for ( i = 0; i < request->completed_cb_count; ++i )
   {
      cb = request->completed_cb[i];
      if ( cb )
      {
         cb( request->completed_cb_ctx[i], status, request->text );
      }
   }
}

/*
 ******************************************************************************
 * Clears the audio data of all non-cacheable voices
 */
static void _clear_noncacheable( void )
{
   TtsVoice voices[128];
   int i, count = tts_voices_get_all( voices, 128 );
   for ( i = 0; i < count; ++i )
   {
      if ( !voices[i].cacheable )
         tts_cache_clear( voices[i].voice_id );
   }
}


