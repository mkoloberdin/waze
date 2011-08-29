/* tts_db.c - implementation of the Text To Speech (TTS) database layer
 *                       SQlite based
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
 *   See tts_db.h
 *       tts.h
 */

#include <stdlib.h>
#include "roadmap.h"
#include "roadmap_path.h"
#include "roadmap_time.h"
#include "roadmap_file.h"
#include "tts_db.h"
#include "tts_db_sqlite.h"
#include "tts_db_files.h"

//======================== Local defines ========================


//======================== Local types ========================


//======================== Globals ========================


//======================== Local Declarations ========================


/*
 ******************************************************************************
 */
BOOL tts_db_store( const TtsDbEntry* entry, TtsDbDataStorageType storage_type, const TtsData* db_data, const TtsPath* db_path )
{
   BOOL result = FALSE;

   if ( storage_type & __tts_db_data_storage__none )
      return FALSE;

   if ( storage_type & __tts_db_data_storage__blob_record )
   {
      result = tts_db_sqlite_store( entry, storage_type, db_data, db_path );
   }
   else
   {
      // Entry only
      result = tts_db_sqlite_store( entry, storage_type, NULL, db_path );
   }

   if ( ( storage_type & __tts_db_data_storage__file ) && db_data )
   {
      // Store data to file
      tts_db_files_store( db_path, db_data );
   }

   return TRUE;
}

/*
 ******************************************************************************
 */
void tts_db_remove( const TtsDbEntry* entry )
{
   TtsPath db_path;
   TtsDbDataStorageType storage_type;

   tts_db_sqlite_get_info( entry, NULL, &storage_type, &db_path );

   tts_db_sqlite_remove( entry );

   if ( storage_type & __tts_db_data_storage__file )
   {
      tts_db_files_remove( &db_path );
   }
}

/*
 ******************************************************************************
 */
BOOL tts_db_get( const TtsDbEntry* entry, TtsData* db_data, TtsPath *db_path )
{
   BOOL result = FALSE;
   TtsDbDataStorageType storage_type;

   result = tts_db_sqlite_get( entry, &storage_type, db_data, db_path );

   if ( ( storage_type & __tts_db_data_storage__file ) && db_data )
   {
      result = tts_db_files_get( db_path, db_data );
   }

   return result;
}

/*
 ******************************************************************************
 */
BOOL tts_db_get_path( const TtsDbEntry* entry, TtsPath* db_path )
{
   TtsDbDataStorageType storage_type;

   BOOL result = FALSE;

   result = tts_db_sqlite_get_info( entry, NULL, &storage_type, db_path );

   if ( !result )
   {
      roadmap_log( ROADMAP_WARNING, TTS_LOG_STR( "Error retrieving the path!" ) );
   }
   if ( !( storage_type & __tts_db_data_storage__file ) )
   {
      roadmap_log( ROADMAP_WARNING, TTS_LOG_STR( "Request for path for the non fs storage record!" ) );
   }

   result = ( result && db_path->path[0] );

   return result;
}


/*
 ******************************************************************************
 */
void tts_db_generate_path( const char* voice_id, TtsPath* db_path )
{
   static char s_voice_id[TTS_VOICE_MAXLEN] = {0};
   static unsigned long counter = 0;
   char path_suffix[TTS_PATH_MAXLEN];
   const EpochTimeMicroSec* time_now = roadmap_time_get_epoch_us( NULL );


   if ( !db_path )
      return;

   // File name
   snprintf( path_suffix, sizeof( path_suffix ), "%s//%s//%lu-%lu-%lu.%s", TTS_DB_FILES_ROOT_DIR, voice_id,
         time_now->epoch_sec, time_now->usec, counter++, TTS_RESULT_FILE_NAME_EXT );
   // Full path
   roadmap_path_format( db_path->path, TTS_PATH_MAXLEN, roadmap_path_tts(), path_suffix );

   if ( strcmp( s_voice_id, voice_id ) ) // Save io time
   {
      char* parent = roadmap_path_parent( db_path->path, NULL );
      strncpy_safe( s_voice_id, voice_id, TTS_VOICE_MAXLEN );
      // Create path if not exists
      roadmap_path_create( parent );
      free( parent );
   }
}

/*
 ******************************************************************************
 */
BOOL tts_db_exists( const TtsDbEntry* entry )
{
   BOOL result = FALSE;

   result = tts_db_sqlite_exists( entry );

   return result;
}

/*
 ******************************************************************************
 */
const TtsDbEntry* tts_db_entry( const char* voice_id, const char* text, TtsDbEntry* entry )
{
   static TtsDbEntry s_entry;

   s_entry.voice_id = voice_id;
   s_entry.text = text;

   if ( entry != NULL )
   {
      *entry = s_entry;
   }

   return &s_entry;
}

/*
 ******************************************************************************
 */
void tts_db_clear( TtsDbDataStorageType storage_type, const char* voice_id )
{
   if ( voice_id )
   {
      tts_db_sqlite_destroy_voice( voice_id );
   }
   else
   {
      // Remove the database
      tts_db_sqlite_destroy();
   }

   // Remove the files if exist
   if ( storage_type & __tts_db_data_storage__file )
   {
      char path[TTS_PATH_MAXLEN];
      if ( voice_id )
         snprintf( path, sizeof( path ), "%s//%s//%s", roadmap_path_tts(), TTS_DB_FILES_ROOT_DIR, voice_id );
      else
         snprintf( path, sizeof( path ), "%s//%s", roadmap_path_tts(), TTS_DB_FILES_ROOT_DIR );

      roadmap_log( ROADMAP_INFO, "Removing the TTS fs database at %s", path );
      roadmap_file_rmdir( path, NULL );
   }
}

/*
 ******************************************************************************
 */
TtsTextType tts_db_text_type( const TtsDbEntry* entry )
{
   TtsTextType text_type;
   tts_db_sqlite_get_info( entry, &text_type, NULL, NULL );
   return text_type;
}
