/* tts_provider.h - The interface for the tts service provider.
 *                  Service should register on tts manager. See function tts_register_provider
 *
 *
 * LICENSE:
 *
 *   Copyright 2011, Waze Ltd
 *                   Alex Agranovich   (AGA)
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
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


#ifndef INCLUDE__TTS_PROVIDER__H
#define INCLUDE__TTS_PROVIDER__H
#ifdef __cplusplus
extern "C" {
#endif

#include "tts_defs.h"

#define TTS_PROVIDER_NAME_MAXLEN                   64

#define TTS_PROVIDER_INITIALIZER                   {0,0,0,0,-1,0,0,0}

#define TTS_SYNTH_PARMAS_INITIALIZER               { 0, NULL, TTS_GENERIC_LIST_INITIALIZER, TTS_INTEGER_LIST_INITIALIZER }

/*
 * Represents the TTS request parameters values
 * Common for all providers.
 */
typedef struct
{
   int flags;
   const char* voice_id;
   TtsPathList       path_list;
   TtsTextTypeList   types_list;
} TtsSynthRequestParams;

/*
 * Represents the TTS data returned after tts request
 */
typedef struct
{
   int      count;                              // Number of the audio buffers
   TtsData tts_data[TTS_BATCH_REQUESTS_LIMIT];
   TtsTextList text_list;
} TtsSynthResponseData;

/*
 * Supplied by the provider to tts manager to the provider. Prepares the provider.
 * Should be called upon provider's voice set (if supplied)
 * Params:  void
  */
typedef void (*TtsSynthPrepare)( void );

/*
 * Supplied by the tts manager to the provider. Called by the provider upon request completion
 * Params:  context - request context
 *           res_status - result status of the tts process supplied by provider
 *           response_data - response data structure
  */
typedef void (*TtsSynthResponseCb)( const void* context, int res_status, TtsSynthResponseData* response_data );

/*
 * Supplied by the provider
 * Params:  context - request context
 *           text - result status of the tts process
 *           text_list - the array of pointers to the requested strings.
 *           path_list - static list of the full paths of the synthesized audio files
 *           params - request parameters
 *           response_cb - response callback
 *
 */
typedef void (*TtsSynthRequestCb) ( const void* context, TtsTextList text_list, const TtsSynthRequestParams* params, TtsSynthResponseCb response_cb );

/*
 * Represents the TTS Provider attributes
 */
struct _TtsProvider
{
   BOOL                registered;

   const char*         provider_name;

   const char*         voices_cfg;   // This one is mandatory. Represents the path to the provider voices
                                                      // configuration file
   int                 batch_request_limit;          // Represents the number of text strings in one request.
                                                      // If batch request is not supported should be 1

   int                 concurrent_limit;              // Limit for the concurrent requests count; -1 - unlimited

   TtsSynthRequestCb   request_cb;                   // Request callback

   TtsSynthPrepare     prepare_cb;

   TtsDbDataStorageType storage_type;                // Storage type for the provider

};

#ifdef __cplusplus
}
#endif
#endif // INCLUDE__TTS_PROVIDER__H
