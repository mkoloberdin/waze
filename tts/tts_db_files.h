/* tts_db_files.h - The interface for the text to speech files database management module
 *
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


#ifndef INCLUDE__TTS_DB_FILES__H
#define INCLUDE__TTS_DB_FILES__H
#ifdef __cplusplus
extern "C" {
#endif

#include "roadmap.h"
#include "tts_db.h"

#define TTS_DB_FILES_NAME_MAXLEN          (255)            // Maximum name of file
#define TTS_DB_FILES_ROOT_DIR             ("database")     // Root directory for the database files under tts directory

/*
 * Stores the voice data to the files database
 *
 * Params: db_path - the file path to store the data in
 *         db_data - the pointer to the audio data structure
 *
 */
BOOL tts_db_files_store( const TtsPath* db_path, const TtsData* db_data );

/*
 * Removes the voice from the files database
 *
 * Params: db_path - the file path to remove
 *
 */
void tts_db_files_remove( const TtsPath* db_path );

/*
 * Loads the voice from the database
 *
 * Params: [in] db_path - the file path to load
 *       : [out] db_data - the data structure to assign the loaded data
 */
BOOL tts_db_files_get( const TtsPath* db_path, TtsData* db_data );

/*
 * Checks if the synthesized text is in the db
 *
 * Params: db_path - the file path to load
 * Returns true - if exists
 */
BOOL tts_db_files_exists( const TtsPath* db_path );

/*
 * File name for the text
 *
 * Params: entry - the TTS db entry description
 *         buf - output buffer
 * Returns void
 */
void tts_db_files_get_path( const TtsDbEntry* entry, char buf[] );

#ifdef __cplusplus
}
#endif
#endif // INCLUDE__TTS_DB_FILES__H
