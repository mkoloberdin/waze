/* tts_db_files.c - implementation of the Text To Speech (TTS) files based database layer
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
 *   See tts_db_files.h
 *       tts.h
 *       tts_db.h
 */

#include <stdlib.h>
#include <string.h>
#include "tts_db_files.h"
#include "roadmap_file.h"
#include "roadmap_path.h"

//======================== Local defines ========================

//======================== Local types ========================

//======================== Globals ========================


//======================== Local Declarations ========================
static BOOL _check_parent( const char* path, BOOL is_create );

/*
 ******************************************************************************
 */
BOOL tts_db_files_store( const TtsPath* db_path, const TtsData* db_data )
{
   RoadMapFile file;
   BOOL result = FALSE;
   const char* path;
   if ( db_path == NULL )
   {
      roadmap_log( ROADMAP_ERROR, TTS_LOG_STR( "Path was not supplied!" ) );
      return FALSE;
   }

   if ( db_data && db_data->data )
   {
      path = db_path->path;
      // Save the resource to the file
      roadmap_log( ROADMAP_INFO, "Storing file at %s", path );

      file = roadmap_file_open( path, "w" );
      if ( !ROADMAP_FILE_IS_VALID( file ) )
      {
         // Last chance - try to create the parent
         _check_parent( path, TRUE );
         file = roadmap_file_open( path, "w" );
      }

      if ( ROADMAP_FILE_IS_VALID( file ) )
      {
         roadmap_file_write( file, db_data->data, db_data->data_size );
         roadmap_file_close( file );
         result = TRUE;
      }
      else
      {
         result = FALSE;
         roadmap_log( ROADMAP_ERROR, "Error opening file: %s", path );
      }
   }

   return result;
}

/*
 ******************************************************************************
 */
void tts_db_files_remove( const TtsPath* db_path )
{
   if ( db_path == NULL )
   {
      roadmap_log( ROADMAP_ERROR, TTS_LOG_STR( "Path was not supplied!" ) );
      return;
   }

   roadmap_file_remove( db_path->path, NULL );
}

/*
 ******************************************************************************
 */
BOOL tts_db_files_get( const TtsPath* db_path, TtsData* db_data )
{
   BOOL result = FALSE;

   RoadMapFile file;

   if ( db_path == NULL )
   {
      roadmap_log( ROADMAP_ERROR, TTS_LOG_STR( "Path was not supplied!" ) );
      return FALSE;
   }


   if ( db_data && ( file = roadmap_file_open( db_path->path, "r" ) ) )
   {
     db_data->data_size = roadmap_file_length( db_path->path, NULL );
     db_data->data = malloc( db_data->data_size );
     roadmap_check_allocated( db_data->data );
     roadmap_file_read( file, db_data->data, db_data->data_size );
     result = TRUE;
   }

   return result;
}

/*
 ******************************************************************************
 */
BOOL tts_db_files_exists( const TtsPath* db_path )
{
   BOOL result;

   if ( db_path == NULL )
   {
      roadmap_log( ROADMAP_ERROR, TTS_LOG_STR( "Path was not supplied!" ) );
      return FALSE;
   }

   result = roadmap_file_exists( db_path->path, NULL );

   return result;
}


/*
 ******************************************************************************
 * Auxiliary
 * Checks if parent directory exists
 * Params: is_create - TRUE create the parent if necessary
 */
static BOOL _check_parent( const char* path, BOOL is_create )
{
   const char* parent = roadmap_path_parent( path, NULL );
   BOOL result = roadmap_file_exists( parent, NULL );

   if ( !result )
   {
      roadmap_log( ROADMAP_WARNING, "Path %s doesn't exist!. Creating: %d", parent, is_create );
      if ( is_create )
      {
         roadmap_path_create( parent );
         result = TRUE;
      }
   }
   roadmap_path_free( parent );

   return result;
}


