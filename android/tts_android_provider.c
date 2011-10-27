/* tts_android_provider.c - android native tts implementation
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
 *   See WazeTtsManager_JNI.c
 */

#include <string.h>
#include "roadmap.h"
#include "tts/tts.h"
#include "tts/tts_provider.h"
#include "JNI/FreeMapJNI.h"

//======================== Local defines ========================

#define ANDROID_TTS_PROVIDER                    "android_tts"
#define ANDROID_TTS_PROVIDER_VOICES             "voices_android_tts.csv"

//======================== Local types ========================

typedef struct
{
   const void* cb_context;
   TtsSynthResponseCb response_cb;
   const char* text;
   BOOL        busy;
   int         index;
} AndroidTtsContext;
//======================== Globals ========================
static AndroidTtsContext    sgCtxPool[TTS_ACTIVE_REQUESTS_LIMIT];
static char                 sgVoicesPath[TTS_PATH_MAXLEN];
//======================== Local Declarations ========================

static void _synth_request( const void* context, TtsTextList text_list, const TtsSynthRequestParams* params, TtsSynthResponseCb response_cb );
static AndroidTtsContext* _allocate_context( void );

/*
 ******************************************************************************
 */
void tts_android_provider_init( void )
{
   int i;
   TtsProvider provider;

   roadmap_path_format( sgVoicesPath, TTS_PATH_MAXLEN, roadmap_path_tts(), ANDROID_TTS_PROVIDER_VOICES );
   provider.batch_request_limit = 1;
   provider.provider_name = ANDROID_TTS_PROVIDER;
   provider.storage_type = __tts_db_data_storage__file;
   provider.request_cb = _synth_request;
   provider.voices_cfg = sgVoicesPath;
   provider.prepare_cb = WazeTtsManager_Prepare;
   provider.concurrent_limit = -1;

   // Register the android provider
   tts_register_provider( &provider );

   // Initialize the context pool
   for ( i = 0; i < TTS_ACTIVE_REQUESTS_LIMIT; ++i )
   {
      sgCtxPool[i].busy = FALSE;
      sgCtxPool[i].index = i;
   }
}

/*
 ******************************************************************************
 * Synth request callback. See TtsSynthRequestCb prototype
 */
static void _synth_request( const void* context, TtsTextList text_list, const TtsSynthRequestParams* params, TtsSynthResponseCb response_cb )
{
   AndroidTtsContext* ctx;
   int i, index;
   const TtsPath* tts_path;

   for ( i = 0; i < TTS_BATCH_REQUESTS_LIMIT; ++i )
   {
      if ( text_list[i] )
      {
         if ( ( ctx = _allocate_context() ) )
         {
            tts_path = params->path_list[i];
            ctx->cb_context = context;
            ctx->response_cb = response_cb;
            ctx->text = text_list[i];
            WazeTtsManager_Commit( text_list[i], tts_path->path, ctx );
         }
      }
   }
}

/*
 ******************************************************************************
 * Allocates context from the contexts pool
 */
static AndroidTtsContext* _allocate_context( void )
{
   int i;
   AndroidTtsContext* ctx = NULL;

   for ( i = 0; i < TTS_ACTIVE_REQUESTS_LIMIT; ++i )
   {
      if ( !sgCtxPool[i].busy )
      {
         ctx = &sgCtxPool[i];
         ctx->busy = TRUE;
         break;
      }
   }

   if ( i == TTS_ACTIVE_REQUESTS_LIMIT )
   {
      roadmap_log( ROADMAP_ERROR, "There is on available context in Android TTS request context pool!" );
   }
   return ctx;
}
/*
 ******************************************************************************
 */
void tts_android_provider_response( const void* context, int res_status )
{
   // Break constancy to update the context
   AndroidTtsContext* android_ctx = (AndroidTtsContext*) context;
   TtsSynthResponseData response_data;
   response_data.count = 1;
   response_data.text_list[0] = android_ctx->text;
   /*
    * The data stored externally
    */
   response_data.tts_data[0].data = NULL;

   if ( android_ctx->response_cb )
      android_ctx->response_cb( android_ctx->cb_context, res_status, &response_data );
   android_ctx->busy = FALSE;
}

