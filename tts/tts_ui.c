/* tts_ui.c - Implementation of the Text To Speech (TTS) UI related functions
 *
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

#include "tts.h"
#include "tts_voices.h"
#include "tts_ui.h"
#include <string.h>

//======================== Local defines ========================

#define TTS_UI_VOICES_MAX_COUNT        128

//======================== Local types ========================


//======================== Globals ========================


static char sgTtsVoicesLabels[TTS_UI_VOICES_MAX_COUNT][TTS_VOICE_FIELD_MAX_SIZE] = {{0}};

static char sgTtsVoicesIds[TTS_UI_VOICES_MAX_COUNT][TTS_VOICE_FIELD_MAX_SIZE] = {{0}};

static int sgTtsVoicesCount = -1;

//======================== Local Declarations ========================


/*
 ******************************************************************************
 */
void tts_ui_initialize( void )
{
   tts_ui_voices_values();
}

/*
 ******************************************************************************
 */
const char** tts_ui_voices_labels( void )
{
   TtsVoice voices[TTS_UI_VOICES_MAX_COUNT];
   int i, count;
   char *pCh, *voice_label;
   static const char* result_ptr[TTS_UI_VOICES_MAX_COUNT] = {NULL};

   count = tts_voices_get_all( voices, TTS_UI_VOICES_MAX_COUNT );

   for ( i = 0; i < count; ++i )
   {
      voice_label = sgTtsVoicesLabels[i];
      strncpy_safe( voice_label, voices[i].label, TTS_VOICE_FIELD_MAX_SIZE );

//      This parsing is unnecessary for the dedicated label field
//      pCh = voice_label;
//      while( pCh != NULL )
//      {
//         pCh = strchr( pCh, '_' );
//         if ( pCh )
//            *pCh = ' ';
//      }
      result_ptr[i] = voice_label;
   }

   return (const char**) result_ptr;
}


/*
 ******************************************************************************
 */
const void** tts_ui_voices_values( void )
{
   TtsVoice voices[TTS_UI_VOICES_MAX_COUNT];
   int i, count;
   static const char* result_ptr[TTS_UI_VOICES_MAX_COUNT] = {NULL};

   count = tts_voices_get_all( voices, TTS_UI_VOICES_MAX_COUNT );

   for ( i = 0; i < count; ++i )
   {
      strncpy_safe( sgTtsVoicesIds[i], voices[i].voice_id, TTS_VOICE_FIELD_MAX_SIZE );
      result_ptr[i] = sgTtsVoicesIds[i];
   }

   return (const void**) result_ptr;
}

/*
 ******************************************************************************
 */
const void* tts_ui_voice_value( const char* voice_id )
{
   int i;
   const void* value = NULL;

   if ( !voice_id || !sgTtsVoicesIds[0][0] )
      return NULL;

   for ( i = 0; i < sgTtsVoicesCount; ++i )
   {
      if ( !strcmp( sgTtsVoicesIds[i], voice_id ) )
      {
         value = ( const void* ) sgTtsVoicesIds[i];
         break;
      }
   }

   return value;
}

/*
 ******************************************************************************
 */
const char* tts_ui_voice_label( const char* voice_id )
{
   const TtsVoice* tts_voice = NULL;
   const char* label = NULL;
   if ( !voice_id )
      return NULL;


   tts_voice = tts_voices_get( voice_id, NULL );

   if ( tts_voice )
   {
      label = tts_voice->label;
   }

   return label;
}
/*
 ******************************************************************************
 */
int tts_ui_count( void )
{
   TtsVoice voices[TTS_UI_VOICES_MAX_COUNT];

   if ( sgTtsVoicesCount >= 0 )
      return sgTtsVoicesCount;

   sgTtsVoicesCount = tts_voices_get_all( voices, TTS_UI_VOICES_MAX_COUNT );

   return sgTtsVoicesCount;
}
