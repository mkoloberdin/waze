/* tts_cache.h - The interface for the text to speech module cache management
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


#ifndef INCLUDE__TTS_CACHE__H
#define INCLUDE__TTS_CACHE__H
#ifdef __cplusplus
extern "C" {
#endif
#include <time.h>
#include "tts_defs.h"



#define TTS_CACHE_BUFFERS_ENABLED         (0)   // Indicates if audio buffers are cached in memory

/*
 * Initializes the queue engine
 * Params: void
 * Returns: void
 */
void tts_cache_initialize( void );

/*
 * Enabled indicator
 * Params:  void
 *
 * Returns: 1 if enabled
 */
int tts_cache_enabled( void );
/*
 * Adds the entry to the cache
 * If voice is different from the current cache voice the data only stored in the database
 * ( No pre-caching of buffers in the memory)
 * Params:  voice_id - voice id of the current text
 *          text - synthesized text
 *          text_type - the type of the text
 *          tts_path - synthesized audio file path ( can be NULL )
 *
 * Returns: void
 */
//void tts_cache_add( const char* voice_id, const char* text, const void* data, size_t data_size );
void tts_cache_add( const char* voice_id, const char* text, TtsTextType text_type, TtsDbDataStorageType storage_type,
      const TtsData* tts_data, const TtsPath* tts_path );


/*
 * Tries to load the entry from the cache
 * Params:  text - requested text
 *          voice_id - voice to retrieve text of
 *          [out] cached_data - synthesized audio
 *          [out] cached_path - the path of the file if exists
 *
 * Returns: void
 */
BOOL tts_cache_get( const char* text, const char* voice_id, TtsData* cached_data, TtsPath* cached_path );

/*
 * Tries to load the entry from the cache - requests path only
 * Params:  text - requested text
 *          [out] cached_path - the path of the file if exists
 *
 * Returns: true if succeeded
 */
BOOL tts_cache_get_path( const char* text, TtsPath* cached_path );

/*
 * Tries to load the entry from the cache - requests path only
 * Params:  text - requested text
 *          [out] data - synthesized audio data
 *
 * Returns: true if succeeded
 */
BOOL tts_cache_get_data( const char* text, TtsData* cached_data );

/*
 * Checks if the text is in cache
 * Params:  text - requested text
 *          voice_id - the voice for which to check existance
 *
 * Returns: void
 */
BOOL tts_cache_exists( const char* text, const char* voice_id );

/*
 * Sets the new voice for the cache. Clears the cache from all entries of the previous
 * Nothing done if voice id is the same
 * Params:  voice_id - new voice id
 *          db_type - storage type for the new voice_id
 *
 * Returns: void
 */
void tts_cache_set_voice( const char* voice_id, TtsDbDataStorageType db_type );

/*
 * Removes the entry from the cache and database
 * Params:  text - requested text
 *          voice_id - voice to retrieve text of
 *          storage_type - storage type of the voice
 *
 * Returns: void
 */
void tts_cache_remove( const char* text, const char* voice_id, TtsDbDataStorageType storage_type );

/*
 * Clears all the TTS cache
 *
 * Params:  voice_id - the voice to clear cached data for ( If NULL - all voices are cleared)
 *
 *
 * Returns: void
 */
void tts_cache_clear( const char* voice_id );

#ifdef __cplusplus
}
#endif
#endif // INCLUDE__TTS_CACHE__H
