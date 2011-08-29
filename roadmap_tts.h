/* roadmap_tts.h - The interface for the text to speech module
 *                        The launcher activates external text to speech synthesis routine. The result callback should is called
 *                        upon recognition process.
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


#ifndef INCLUDE__ROADMAP_TTS__H
#define INCLUDE__ROADMAP_TTS__H
#ifdef __cplusplus
extern "C" {
#endif


#include "roadmap.h"

// Should be "no" for platforms without TTS support
#if defined(ANDROID)
#define TTS_FEATURE_ENABLED                         "yes"
#else
#define TTS_FEATURE_ENABLED                         "yes"
#endif

#define TTS_FLAG_NONE                               0x00000000
#define TTS_FLAG_TIMEOUT_ENABLED                    0x00000001 /* Currently not supported  */

#define TTS_RES_STATUS_ERROR                        0x00000000
#define TTS_RES_STATUS_SUCCESS                      0x00000001
#define TTS_RES_STATUS_NO_RESULTS                   0x00000002


#define TTS_DEFAULT_TIMEOUT                          60     // Default timeout for the response waiting (seconds)
                                                            // By passing this period the request is aborted if not completed
#define TTS_RESULT_FILE_NAME_MAXLEN                  512

/*
 * Supplied by the client (service consumer)
 * Returns the file name of the synthesized audio
 */
typedef void (*RMTtsResultCb)( const char* res_file_path );

/*
 * Launcher context passed to the service side. Lifetime upon end of the process
 */
typedef struct
{
   RMTtsResultCb result_cb;
   int flags;
   char* input_text;
   char res_file_name[TTS_RESULT_FILE_NAME_MAXLEN];
   int queue_idx; // Internal interface usage (conceptually should be hidden)
} RMTtsContext;


/*
 * The callback function. Supplied to the service by the interface layer.
 * Intermediate function allowing the interface layer to handle the results before passing them
 * to the client
 * Params: context - the context passed back to the interface
 *         res_status - resulting status
 *         res_file
 *
 */
typedef  (*RMTtsResponseCb)( const void* context, int res_status, const char* res_file );


/*
 * The callback function launches the external speechtt engine
 * Params: resulting callback, flags
 */
typedef int (*RMTtsLauncherCb)( RMTtsResponseCb response_cb, const void* context, int flags );


/*
 * The callback function stop the external TTS engine process (all the requests)
 * Params:
 *
 */
typedef void (*RMTtsCloseCb)( void );


/*
 * This should be called upon platform initialization to make TTS
 * engine available for the application
 */
void roadmap_tts_register_launcher( RMTtsLauncherCb launcher_cb );

/*
 * TTS process stop function
 */
void roadmap_tts_register_close( RMTtsCloseCb close_cb );

/*
 * Initialization procedure (called at the first TTS request)
 */
void roadmap_tts_initialize( void );

/*
 * Posts the synthesis request
 * Supplied callback is called upon synthesis result
 */
void roadmap_tts_post( const char* text, RMTtsResultCb on_result, int flags );

/*
 * Posts the synthesis request
 * The result is played
 */
void roadmap_tts_play( const char* text, int flags );

/*
 * Stops the recognition process. Supplied callback ( in start function ) is called
 * with the results till that point
 */
void roadmap_tts_abort( void );

/*
 * Indicates whether the TTS is initialized and can be used
 */
int roadmap_tts_enabled( void );



#endif // INCLUDE__ROADMAP_TTS__H
