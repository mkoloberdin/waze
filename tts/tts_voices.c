/* tts_voices.c - This module provides tools for retrieving the information
 *                            regarding the available tts voices
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
 *   See
 *       tts_voices.h
 *       tts_db.h
 *       tts.h
 */

/*
 * TODO:: Check if hashing on voice_id is necessary
 *
 */



#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "roadmap.h"
#include "roadmap_config.h"
#include "roadmap_file.h"
#include "roadmap_path.h"
#include "tts_voices.h"
#include "tts_defs.h"
#include "roadmap_httpcopy_async.h"
#include "websvc_trans/web_date_format.h"

//======================== Local defines ========================
#define TTS_VOICES_INITIAL_MAX_COUNT     64     /* The initial maximum count of available voices
                                                 * Adjusted if the downloaded file has more records */

#define TTS_VOICES_CONFIG_FILE_NAME      "voices.csv"

//======================== Local types ========================

typedef TtsVoice* TtsVoicePtr;

//======================== Globals ========================

static TtsVoicePtr* sgTtsVoices = NULL;
static int sgTtsVoicesCount = 0;
static TtsVoiceUpdatedCb   sgTtsVoiceOnUpdatedCb;
//======================== Local Declarations ========================

static const char* tts_voices_path( void );

static void _voices_load( const char* service_provider, const char* path );
static void _free_voice( TtsVoicePtr ptr );
static int _get_voice_index( const char* voice_id );
static void _prepare_invalidation( const char* service_provider );
static void _commit_invalidation( const char* service_provider );

//static void tts_voices_download( void );
//static void tts_voices_load_file( void );

/*
 ******************************************************************************
 */
void tts_voices_initialize( TtsVoiceUpdatedCb voice_updated_cb )
{
   sgTtsVoiceOnUpdatedCb = voice_updated_cb;
}

/*
 ******************************************************************************
 */
void tts_voices_shutdown( void )
{
   int i = sgTtsVoicesCount;

   for ( ; (i--) && sgTtsVoices; )
   {
      TtsVoicePtr ptr = sgTtsVoices[i];
      _free_voice( ptr );
   }
   if ( sgTtsVoices != NULL )
   {
      free( sgTtsVoices );
   }
}

/*
 ******************************************************************************
 */
void tts_voices_update(  const char* service_provider, const char* path )
{
   _voices_load( service_provider, path );
}

/*
 ******************************************************************************
 */
int tts_voices_get_lang( const char* lang, TtsVoice voices[], int voices_size )
{
   int i, count = 0;
   if ( sgTtsVoicesCount == 0 )
      return 0;

   for ( i = 0; ( i < sgTtsVoicesCount ) &&
                  ( i < voices_size ); ++i )
   {
      if ( TTS_VOICE_VALID( sgTtsVoices[i] ) && !strcmp( sgTtsVoices[i]->language, lang ) )
      {
         voices[count++] = *sgTtsVoices[i];
      }
   }

   return count;
}
/*
 ******************************************************************************
 */
int tts_voices_get_all( TtsVoice voices[], int voices_size )
{
   int i, i_out = 0;
   if ( sgTtsVoicesCount == 0 )
      return 0;

   for ( i = 0; ( i < sgTtsVoicesCount ) &&
                  ( i_out < voices_size ); ++i )
   {
      if ( TTS_VOICE_VALID( sgTtsVoices[i] ) )
      {
         voices[i_out] = *sgTtsVoices[i];
         i_out++;
      }
   }

   return i_out;
}

#if 0
/*
 ******************************************************************************
 */
int tts_voices_get_service_provider( const char* service_provider, TtsVoice voices[], int voices_size )
{
   int i, i_out = 0;
   if ( sgTtsVoicesCount == 0 )
      return 0;

   for ( i = 0; ( i < sgTtsVoicesCount ) &&
                  ( i_out < voices_size ); ++i )
   {
         if ( !strcmp( service_provider, sgTtsVoices[i]->service_provider ) &&
               TTS_VOICE_VALID( sgTtsVoices[i] ) )
         {
            voices[i_out++] = *sgTtsVoices[i];
         }
   }

   return i_out;
}


/*
 ******************************************************************************
 */
const char* tts_voices_get_service_default( const char* service_provider )
{
   const char* voice_id = NULL;
   int i;
   if ( sgTtsVoicesCount == 0 )
      return 0;

   for ( i = 0; i < sgTtsVoicesCount; ++i )
   {
      if ( !strcmp( service_provider, sgTtsVoices[i]->service_provider ) &&
            TTS_VOICE_VALID( sgTtsVoices[i] ) )
      {
         voice_id = sgTtsVoices[i]->voice_id;
         break;
      }
   }

   return voice_id;
}
/*
 ******************************************************************************
 */
const char* tts_voices_get_default( void )
{
   const char* voice_id = NULL;
   int i;

   if ( sgTtsVoicesCount == 0 )
      return 0;

   for ( i = 0; i < sgTtsVoicesCount; ++i )
   {
      if ( TTS_VOICE_VALID( sgTtsVoices[i] ) )
      {
         voice_id = sgTtsVoices[i]->voice_id;
         break;
      }
   }

   return voice_id;
}
#endif // Currently not in use

/*
 ******************************************************************************
 */
const TtsVoice* tts_voices_get( const char* voice_id, TtsVoice* voice )
{
   static TtsVoice tts_voice;
   TtsVoice* ret_ptr = NULL;
   int index;

   index = _get_voice_index( voice_id );

   if ( index == -1 )
      return NULL;

   tts_voice = *sgTtsVoices[index];
   ret_ptr = &tts_voice;
   if ( voice )
      *voice = tts_voice;

   return ret_ptr;
}

/*
 ******************************************************************************
 */
static int _get_voice_index( const char* voice_id )
{
   int i, index = -1;

   for ( i = 0; i < sgTtsVoicesCount; ++i )
   {
      if ( !strcmp( sgTtsVoices[i]->voice_id, voice_id ) )
      {
         index = i;
         break;
      }
   }

   return index;
}

/*
******************************************************************************
* Splits the CSV line. Creates the tts voice data structure
* Auxiliary function
*/
static TtsVoicePtr tts_voices_load_line( char line[] )
{
   TtsVoicePtr tts_voice = malloc( sizeof( *tts_voice ) );
   const char* token_sep = ",";
   char* token;
   int field_index = 0;

   token = strtok( line, token_sep );

   while ( token != NULL )
   {
      switch ( field_index )
      {
         case __tts_voice_field__voice_id:
            strncpy_safe( tts_voice->voice_id, token, TTS_VOICE_FIELD_MAX_SIZE );
            break;
         case __tts_voice_field__version:
            tts_voice->version = atoi( token );
            break;
         case __tts_voice_field__label:
            strncpy_safe( tts_voice->label, token, TTS_VOICE_FIELD_MAX_SIZE );
            break;
//         case __tts_voice_field__provider:
//            strncpy_safe( tts_voice->voice_provider, token, TTS_VOICE_FIELD_MAX_SIZE );
//            break;
         case __tts_voice_field__cacheable:
            tts_voice->cacheable = !strcasecmp( token, "t" );
            break;
         case __tts_voice_field__language:
            strncpy_safe( tts_voice->language, token, TTS_VOICE_FIELD_MAX_SIZE );
            break;
         case __tts_voice_field__gender:
            strncpy_safe( tts_voice->gender, token, TTS_VOICE_FIELD_MAX_SIZE );
            break;
         case __tts_voice_field__code:
            strncpy_safe( tts_voice->code, token, TTS_VOICE_FIELD_MAX_SIZE );
            break;
         default:
            roadmap_log( ROADMAP_WARNING, "Invalid field index in the CSV file: %d", field_index );
      }
      field_index++;
      token = strtok( NULL, token_sep );
   }
   // Check fields number
   if ( field_index < __tts_voice_fields__count )
   {
      roadmap_log( ROADMAP_ERROR, "Incomplete line: %s. Only %d fields found", line, field_index );
      free( tts_voice );
      tts_voice = NULL;
   }
   return tts_voice;
}

/*
******************************************************************************
* Loads the configuration file, allocates memory and fills the data structures
* Auxiliary function
*/
static void _voices_load( const char* service_provider, const char* path )
{
   FILE* file;
   int voices_alloc_count = TTS_VOICES_INITIAL_MAX_COUNT;

   if ( !roadmap_file_exists( path, NULL )  )
   {
      roadmap_log( ROADMAP_WARNING, "TTS voices configuration (%s) does not exist!", path );
      return;
   }

   file = roadmap_file_fopen( path, NULL, "r" );

   if ( file )
   {
      char line[2048];
      TtsVoicePtr tts_voice;
      const TtsVoice* old_voice;
      BOOL initial_load = !sgTtsVoices;
      // Make initial preallocation
      if ( initial_load )
         sgTtsVoices = malloc( sizeof( TtsVoicePtr ) * voices_alloc_count );

      // Invalidate current service provider voices
      _prepare_invalidation( service_provider );

      while ( !feof( (FILE*) file ) )
      {
          /* Read the next line */
          if ( fgets( line, sizeof( line ), (FILE*) file ) == NULL ) break;

          tts_voice = tts_voices_load_line( line );
          if ( tts_voice != NULL )
          {
             int index = -1;
             old_voice = NULL;

             strncpy_safe( tts_voice->service_provider, service_provider, TTS_VOICE_FIELD_MAX_SIZE );
             tts_voice->status = __tts_voice_status_valid;

             if ( !initial_load )
             {
                // Find entry
                index = _get_voice_index( tts_voice->voice_id );
                if ( index >= 0 )
                   old_voice = sgTtsVoices[index];
             }

             // Inform on voice update
             if ( sgTtsVoiceOnUpdatedCb )
                sgTtsVoiceOnUpdatedCb( tts_voice, old_voice );

             if ( index >= 0 )
             {
                // Update the voice
                _free_voice( sgTtsVoices[index] );
                sgTtsVoices[index] = tts_voice;
             }
             else // New entry
             {
                // Realloc if necessary
                if ( sgTtsVoicesCount == voices_alloc_count )
                {
                   voices_alloc_count += TTS_VOICES_INITIAL_MAX_COUNT;
                   sgTtsVoices = realloc( sgTtsVoices, voices_alloc_count );
                }
                sgTtsVoices[sgTtsVoicesCount] = tts_voice;
                sgTtsVoicesCount++;
             }
          }
          else
          {
             roadmap_log( ROADMAP_ERROR, "Error reading line %s in file: %s", line, path );
          }
      } // while
      // Inform on invalidated voices
      _commit_invalidation( service_provider );
   }
   else
   {
      roadmap_log( ROADMAP_ERROR, "Error opening file: %s", path );
   }


   fclose( file );
}

/*
******************************************************************************
*/
static const char* tts_voices_path( void )
{
   static char path[512] = {0};
   if ( path[0] == 0 )
   {
      roadmap_path_format( path, sizeof( path ), roadmap_path_tts(), TTS_VOICES_CONFIG_FILE_NAME );
   }
   return path;
}

///*
//******************************************************************************
//*/
//static void tts_voices_reload( void )
//{
//   tts_voices_shutdown();
//   _voices_load();
//}

/*
 ******************************************************************************
 */
static void _free_voice( TtsVoicePtr ptr )
{
   if ( ptr )
      free( ptr );
}

/*
 ******************************************************************************
 */
static void _prepare_invalidation( const char* service_provider )
{
   int i;

   for ( i = 0; i < sgTtsVoicesCount; ++i )
   {
      if ( !strcmp( sgTtsVoices[i]->service_provider, service_provider ) )
      {
         if ( TTS_VOICE_VALID( sgTtsVoices[i] ) )
            sgTtsVoices[i]->status = __tts_voice_status_pending_invalidation;
      }
   }
}
/*
 ******************************************************************************
 */
static void _commit_invalidation( const char* service_provider )
{
   int i;

   for ( i = 0; i < sgTtsVoicesCount; ++i )
   {
      if ( !strcmp( sgTtsVoices[i]->service_provider, service_provider ) )
      {
         if ( sgTtsVoices[i]->status == __tts_voice_status_pending_invalidation )
         {
            if ( sgTtsVoiceOnUpdatedCb )
               sgTtsVoiceOnUpdatedCb( NULL,  sgTtsVoices[i] );
            sgTtsVoices[i]->status = __tts_voice_status_invalid;
         }
      }
   }
}
