/* tts_db.h - The interface for the TTS database
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


#ifndef INCLUDE__TTS_DB__H
#define INCLUDE__TTS_DB__H
#ifdef __cplusplus
extern "C" {
#endif

#include "roadmap.h"
#include "tts_defs.h"


/*
 * Database version number for the current Application version. If greater than the saved one
 * the database should be destroyed
 */
#define TTS_DB_VERSION        2

/*
 * Tts database entry
 */
typedef struct
{
   const char* voice_id;
   const char* text;
   TtsTextType text_type;
} TtsDbEntry;

/*
 * Stores the voice data to the database according to the db type
 *
 * Params:
 *         entry - the TTS db entry description
 *         storage_type - data storage type
 *         db_data -  audio data structure
 *         db_path - the full path to the audio file
 */
BOOL tts_db_store( const TtsDbEntry* entry, TtsDbDataStorageType storage_type, const TtsData* db_data, const TtsPath* db_path );

/*
 * Removes the voice from the database according to the db type
 *
 * Params:  entry - the TTS db entry description
 *
 */
void tts_db_remove( const TtsDbEntry* entry );

/*
 * Returns the text type of the requested entry
 *
 * Params:  entry - the TTS db entry description
 *
 * Returns: type of the text entry
 *
 */
TtsTextType tts_db_text_type( const TtsDbEntry* entry );

/*
 * Loads the voice from the database according to the db type
 *
 * Params:
 *       entry - the TTS db entry description
 *       [out] db_data - the data structure
 *       [out] path - the array to store the path into
 */
BOOL tts_db_get( const TtsDbEntry* entry, TtsData* db_data, TtsPath *db_path );

/*
 * Checks if the synthesized text is in the db
 *
 * Params:
 *          entry - the TTS db entry description
 *
 *          Returns true - if exists
 */
BOOL tts_db_exists( const TtsDbEntry* entry );

/*
 * This function returns fs path for the entry if exists and stored in file
 *
 * Params:
 *          entry - the TTS db entry description
 *          [out] db_path - the pointer to the path variable to store into
 *
 *          Returns true - if entry and path exists
 */
BOOL tts_db_get_path( const TtsDbEntry* entry, TtsPath* db_path );

/*
 * This function generates unique filesystem path for the synthesized file
 *
 * Params:
 *          [out] db_path - the path variable to store into
 *
 *          Returns void
 */
void tts_db_generate_path( const char* voice_id, TtsPath* db_path );

/*
 * Builds db entry structure. Returns statically allocated one and fills "entry" if provided
 *      Used for easier extension in the future of the interface
 *
 * Params:
 *         voice_id - the string defining the voice of the audio data
 *         text - the text vocalized in the audio
 */
const TtsDbEntry* tts_db_entry( const char* voice_id, const char* text, TtsDbEntry* entry );

/*
 * Removes the database
 *
 * Params:  storage_type - the TTS storage type of the database to be cleared
 *          voice_id     - the voice id to clear the db for
 *
 */
void tts_db_clear( TtsDbDataStorageType storage_type, const char* voice_id );


#ifdef __cplusplus
}
#endif
#endif // INCLUDE__TTS_DB__H
