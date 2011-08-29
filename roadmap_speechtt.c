/* roadmap_speechtt.c - implementation of the speech to text interface layer.
 *                      Provides control for the external speech to text engine launching
 *
 * LICENSE:
 *   Copyright 2010, Waze Ltd      Alex Agranovich (AGA)
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
 *   See roadmap_speechtt.h
 *
 */

#include "roadmap.h"
#include "roadmap_speechtt.h"
#include <stdlib.h>


typedef enum
{
   _speechtt_idle = 0x0,
   _speechtt_listening,
   _speechtt_stop_request,
   _speechtt_abort_request
}RMSpeechttState;




//======================== Globals ========================

static RMSpeechttLauncherCb sgSpeechttLauncherCb = NULL;       // Launcher of the engine
static RMSpeechttCloseCb    sgSpeechttCloseCb = NULL;          // Engine stop function
static RMSpeechttState      sgSpeechttState = _speechtt_idle;


//======================== Local Declarations ========================
static void  speechtt_result_callback( RMSpeechttContext* context, int res_status, char *buffer_out );

void roadmap_speechtt_register_launcher( RMSpeechttLauncherCb launcher_cb )
{
   sgSpeechttLauncherCb = launcher_cb;
}

void roadmap_speechtt_register_close( RMSpeechttCloseCb close_cb )
{
   sgSpeechttCloseCb = close_cb;
}


void roadmap_speechtt_start( RMSpeechttResultCb on_result, int flags )
{
   RMSpeechttContext *context;
   if ( !sgSpeechttLauncherCb )
   {
      roadmap_log( ROADMAP_WARNING, "SpeechTT engine is not initialized!" );
      return;
   }
   if ( sgSpeechttState != _speechtt_idle )
   {
      roadmap_log( ROADMAP_WARNING, "SpeechTT engine state is not idle ( %d ). Aborting ...", sgSpeechttState );
      roadmap_speechtt_abort();
   }

   sgSpeechttState = _speechtt_listening;

   context = malloc( sizeof( RMSpeechttContext ) );
   context->flags = flags;
   context->result_cb = on_result;
   context->timeout = SPEECHTT_DEFAULT_TIMEOUT;
   context->extra_prompt = NULL;
   if ( !sgSpeechttLauncherCb( (RMSpeechttResponseCb) speechtt_result_callback, context ) )
   {
      roadmap_log( ROADMAP_ERROR, "SpeechTT engine is failed to start!" );
      sgSpeechttState = _speechtt_idle;
      return;
   }
}

void roadmap_speechtt_stop( void )
{
   int execute_callback = ( sgSpeechttState == _speechtt_listening );

   roadmap_log( ROADMAP_DEBUG, "Trying to stop speech to text engine process. State: %d", sgSpeechttState );

   if ( sgSpeechttState != _speechtt_listening )
   {
      roadmap_log( ROADMAP_WARNING, "STT Invalid state while stopping the engine. State: %d", sgSpeechttState );
   }

   sgSpeechttCloseCb();
}


void roadmap_speechtt_abort( void )
{
   roadmap_log( ROADMAP_DEBUG, "Trying to abort speech to text engine process. State: %d", sgSpeechttState );
   if ( sgSpeechttState != _speechtt_listening )
   {
      roadmap_log( ROADMAP_WARNING, "STT Invalid state while aborting the engine. State: %d", sgSpeechttState );
   }
   sgSpeechttCloseCb();
}

int roadmap_speechtt_enabled( void )
{
   return ( sgSpeechttLauncherCb != NULL );
}


/*
 * Callback wrapper
 */
static void  speechtt_result_callback( RMSpeechttContext* context, int res_status, char *buffer_out )
{
   roadmap_log( ROADMAP_WARNING, "Speech to text result callback called. Status: %d", res_status );

   if ( context == NULL )
   {
      roadmap_log( ROADMAP_ERROR, "Speech to text context is corrupted!!!" );
      return;
   }
   /*
    * Client side is responsible to free the buffer memory !
    */
   if ( sgSpeechttState == _speechtt_abort_request )
   {
      if ( buffer_out != NULL)
         free( buffer_out );
   }
   else
   {
      context->result_cb( buffer_out );
   }

   free( context );

   sgSpeechttState = _speechtt_idle;

}

