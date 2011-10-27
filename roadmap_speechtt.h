/* roadmap_speechtt.h - The interface for the speech to text module
 *                        The launcher activates external recognition routine. The supplied callback should be called
 *                        upon recognition process.
 *
 * LICENSE:
 *
 *   Copyright 2010, Waze Ltd
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


#ifndef INCLUDE__ROADMAP_EDITBOX__H
#define INCLUDE__ROADMAP_EDITBOX__H
#ifdef __cplusplus
extern "C" {
#endif


#include "roadmap.h"

#define SPEECHTT_FLAG_NONE                               0x00000000
#define SPEECHTT_FLAG_TIMEOUT_ENABLED                    0x00000001
#define SPEECHTT_FLAG_INPUT_SEARCH                       0x00000002
#define SPEECHTT_FLAG_INPUT_TEXT                         0x00000004
#define SPEECHTT_FLAG_INPUT_CMD                          0x00000008
#define SPEECHTT_FLAG_EXTERNAL_RECOGNIZER                0x00000010
#define SPEECHTT_FLAG_EXTRA_PROMPT                       0x00000011


#define SPEECHTT_RES_STATUS_ERROR                        0x00000000
#define SPEECHTT_RES_STATUS_SUCCESS                      0x00000001
#define SPEECHTT_RES_STATUS_NO_RESULTS                   0x00000002


#define SPEECHTT_DEFAULT_TIMEOUT                            5    // Default timeout for the voice listening.
                                                                 // By passing this period the results are collected and callback is called

/*
 * The callback is called upon the recognition result. "buffer_out"
 * points to the leaked heap zero-terminated byte buffer.
 * The callback supplier is responsible for the deallocation
 */
typedef void (*RMSpeechttResultCb)( char *buffer_out );

/*
 * Launcher context passed to the native side. Lifetime upon end of the process
 */
typedef struct
{
   RMSpeechttResultCb result_cb;
   int flags;
   int timeout;
   const char* extra_prompt;     // Additional text on the native UI like "speak now" ( used along with SPEECHTT_FLAG_EXTRA_PROMPT )
} RMSpeechttContext;


/*
 * The callback function. Should be called on the end of the process
 * Params: result_cb - passed at the launch by the client
 *         res_status - resulting status
 *         buffer_out - the results buffer
 */
typedef int (*RMSpeechttResponseCb)( RMSpeechttContext* context, int res_status, char *buffer_out );


/*
 * The callback function launches the external speechtt engine
 * Params: resulting callback, timeout, flags
 */
typedef int (*RMSpeechttLauncherCb)( RMSpeechttResponseCb response_cb, RMSpeechttContext* context );


/*
 * The callback function stop the external speechtt engine process
 * Params: execute_callback - if the callback with the partial results should be called
 *
 */
typedef void (*RMSpeechttCloseCb)( void );


/*
 * This should be called upon platform initialization to make recognition
 * engine available for the application
 */
void roadmap_speechtt_register_launcher( RMSpeechttLauncherCb launcher_cb );

/*
 * Recognition process stop function
 */
void roadmap_speechtt_register_close( RMSpeechttCloseCb close_cb );



/*
 * Main entry point for the recognition process. Supplied callback is called
 * upon recognition result
 */
void roadmap_speechtt_start( RMSpeechttResultCb on_result, int flags );

/*
 * Stops the recognition process. Supplied callback ( in start function ) is called
 * with the results till that point
 */
void roadmap_speechtt_stop( void );

/*
 * Stops the recognition without callback execution.
 */
void roadmap_speechtt_abort( void );

/*
 * Indicates whether the speechtt is initialized and can be used
 */
int roadmap_speechtt_enabled( void );



#endif // INCLUDE__ROADMAP_EDITBOX__H
