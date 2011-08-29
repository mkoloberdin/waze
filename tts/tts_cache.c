/* tts_cache.c - Text To Speech (TTS) interface layer implementation
 *                       Cache management.
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
 *       tts.c
 *
 */

#include <stdlib.h>
#include <string.h>
#include "roadmap.h"
#include "tts_voices.h"
#include "tts_cache.h"
#include "tts_defs.h"
#include "tts_db.h"
#include "roadmap_config.h"
#include "roadmap_hash.h"

//======================== Local defines ========================

#define TTS_CACHE_DEBUG

#define TTS_CACHE_SIZE                    (32)

//======================== Local types ========================

typedef struct
{
   int prev;
   int next;
   int key;

   char* text;
   TtsData tts_data;
   TtsPath tts_path;
} TtsCacheEntry;


typedef struct
{
   TtsCacheEntry cache[TTS_CACHE_SIZE];
   RoadMapHash* hash;
   int head;
   int count;
   const char* voice_id;
} TtsCacheContext;

static TtsCacheContext sgTtsCache;

//======================== Globals ========================

static TtsDbDataStorageType sgTtsDbType = __tts_db_data_storage__file;

static RoadMapConfigDescriptor RMConfigTTSCacheEnabled =
      ROADMAP_CONFIG_ITEM( TTS_CFG_CATEGORY, TTS_CFG_CACHE_ENABLED );
static RoadMapConfigDescriptor RMConfigTTSDbVersion =
      ROADMAP_CONFIG_ITEM( TTS_CFG_CATEGORY, TTS_CFG_DB_VERSION );

//======================== Local Declarations ========================

static void _reset_context( TtsCacheContext* context, const char* voice_id );
static int _find_entry( const char* text, TtsCacheContext* context );
static void _set_MRU ( TtsCacheContext* context, int slot );
static void _remove_entry_data ( TtsCacheContext* context, int slot );
static void _remove_entry ( TtsCacheContext* context, int slot );
static const TtsVoice* _tts_voice( void );
static int _find_empty( void );
static void _check_db_version( void );

/*
 ******************************************************************************
 */
void tts_cache_initialize( void )
{
   roadmap_config_declare("preferences", &RMConfigTTSCacheEnabled,
            TTS_CFG_CACHE_ENABLED_DEFAULT, NULL);

   roadmap_config_declare("session", &RMConfigTTSDbVersion, "0", NULL);

   sgTtsCache.hash = roadmap_hash_new( "TTS CACHE", TTS_CACHE_SIZE );
}

/*
 ******************************************************************************
 */
int tts_cache_enabled( void )
{
   int enabled = !strcmp( roadmap_config_get( &RMConfigTTSCacheEnabled ), "yes" );
   return enabled;
}

TtsCacheEntry* _cache_set_entry( int slot, const char* text, const TtsData* tts_data, const TtsPath* tts_path )
{
   TtsCacheEntry* cache_entry = &sgTtsCache.cache[slot];

#ifdef TTS_CACHE_DEBUG
   roadmap_log( ROADMAP_DEBUG, "Adding text '%s' to the cache. Slot: %d", text, slot );
#endif // TTS_CACHE_DEBUG

   if ( TTS_CACHE_BUFFERS_ENABLED && tts_data )
   {
      cache_entry->tts_data.data_size = tts_data->data_size;
      cache_entry->tts_data.data = tts_data->data;
   }
   else
   {
      cache_entry->tts_data.data = NULL;
   }

   strncpy_safe( cache_entry->tts_path.path, tts_path->path, sizeof( tts_path->path ) );

   cache_entry->text = strdup( text );
   cache_entry->key = roadmap_hash_string( text );

   roadmap_hash_add( sgTtsCache.hash, cache_entry->key, slot );

   return cache_entry;
}

/*
 ******************************************************************************
 */
static TtsCacheEntry* _cache_add( const char* text, const TtsData* tts_data, const TtsPath* tts_path )
{
   int slot;
   TtsCacheEntry *cache_entry = NULL;

   slot = _find_entry( text, &sgTtsCache );
   if ( slot >= 0 )
   {
      // AGA DEBUG Reduce level
#ifdef TTS_CACHE_DEBUG
     roadmap_log( ROADMAP_DEBUG, "The text '%s' is already in the cache. Slot: %d", slot );
#endif // TTS_CACHE_DEBUG
     return &sgTtsCache.cache[slot];
   }
   if ( sgTtsCache.count < TTS_CACHE_SIZE )
   {
      slot = _find_empty();
      sgTtsCache.count++;
   }
   else  // Replace the element
   {
      int head = sgTtsCache.head;
      slot = sgTtsCache.cache[head].prev;

      _remove_entry_data( &sgTtsCache, slot );
   }

   _cache_set_entry( slot, text, tts_data, tts_path );

   if ( sgTtsCache.count > 1 )
      _set_MRU( &sgTtsCache, slot );

   return cache_entry;
}


/*
 ******************************************************************************
 */
void tts_cache_add( const char* voice_id, const char* text, TtsTextType text_type, TtsDbDataStorageType storage_type,
      const TtsData* tts_data, const TtsPath* tts_path )
{
   TtsDbEntry db_entry;
   const TtsVoice* voice = _tts_voice();

   if ( !tts_cache_enabled() )
      return;

   if ( strcmp( voice_id, voice->voice_id ) )
      return;

   tts_db_entry(voice->voice_id, text, &db_entry );
   db_entry.text_type = text_type;

   _cache_add( text, tts_data, tts_path );

   /*
    * Store to the database
    */
   tts_db_store( &db_entry, sgTtsDbType, tts_data, tts_path );
}

/*
 ******************************************************************************
 */
void tts_cache_remove( const char* text, const char* voice_id, TtsDbDataStorageType storage_type )
{
   TtsDbEntry db_entry;
   int slot = -1;

   if ( !tts_cache_enabled() )
      return;

   if ( !voice_id || !strcmp( voice_id, sgTtsCache.voice_id ) )
   {
      voice_id = sgTtsCache.voice_id;
      slot = _find_entry( text, &sgTtsCache );
   }

   _remove_entry( &sgTtsCache, slot );

   // AGA DEBUG - Reduce level
   roadmap_log( ROADMAP_WARNING, TTS_LOG_STR( "Removing entry from the database: (%s, %s)" ),
         text, voice_id );
   tts_db_entry( voice_id, text, &db_entry );
   tts_db_remove( &db_entry );   // Cache can be defined for another voice. Remove from database for the voice in parameter
}

/*
 ******************************************************************************
 */
BOOL tts_cache_get_path( const char* text, TtsPath* cached_path )
{
   return tts_cache_get( text,  NULL, NULL, cached_path );
}

/*
 ******************************************************************************
 */
BOOL tts_cache_get_data( const char* text, TtsData* cached_data )
{
   return tts_cache_get( text, NULL, cached_data, NULL );
}

/*
 ******************************************************************************
 */
void tts_cache_clear( const char* voice_id )
{
   // If current voice - reset the cache context
   if ( sgTtsCache.voice_id )
   {
      if ( !voice_id || !strcmp( voice_id, sgTtsCache.voice_id ) )
         _reset_context( &sgTtsCache, sgTtsCache.voice_id );
   }

   tts_db_clear( sgTtsDbType, voice_id );
}

/*
 ******************************************************************************
 */
BOOL tts_cache_get( const char* text, const char* voice_id, TtsData* cached_data, TtsPath* cached_path )
{
   int slot = -1;
   BOOL result = FALSE;
   TtsDbEntry db_entry;
   TtsCacheEntry *cache_entry;
   const TtsData* tts_data;

   if ( !tts_cache_enabled() )
      return FALSE;

   if ( !text )
      return FALSE;

   if ( !voice_id || !strcmp( voice_id, sgTtsCache.voice_id ) )
   {
      voice_id = sgTtsCache.voice_id;
      slot = _find_entry( text, &sgTtsCache );
   }

   // Update cache activity
#ifdef TTS_CACHE_DEBUG
   if ( slot >= 0 )
   {
      roadmap_log( ROADMAP_DEBUG, "The text '%s' is found in the cache in slot: %d", text, slot );
   }
   else
   {
      roadmap_log( ROADMAP_DEBUG, "The text '%s' is not found in the cache", text );
   }
#endif // TTS_CACHE_DEBUG

   // Load the data
   if ( cached_data || cached_path )
   {
      if ( slot >= 0 )
      {
         _set_MRU( &sgTtsCache, slot );

         cache_entry = &sgTtsCache.cache[slot];
         tts_data = &cache_entry->tts_data;
         if ( cached_data && tts_data )
         {
            cached_data->data = malloc( tts_data->data_size );
            cached_data->data_size = tts_data->data_size;
            memcpy( cached_data->data, tts_data->data, tts_data->data_size );
         }
         if ( cached_path && cache_entry->tts_path.path[0] )
         {
            strncpy_safe( cached_path->path, cache_entry->tts_path.path, sizeof( cached_path->path ) );
         }
         return TRUE;
      }
      else
      {
         TtsData _data;
         TtsPath _path;

         tts_db_entry( voice_id, text, &db_entry );

         if ( TTS_CACHE_BUFFERS_ENABLED )
            result = tts_db_get( &db_entry, &_data, &_path );
         else
            result = tts_db_get( &db_entry, NULL, &_path );

         if ( result )
         {
            if ( cached_path )
            {
               strncpy_safe( cached_path->path, _path.path, sizeof( cached_path->path ) );
            }
            if ( cached_data )
            {
               cached_data->data = _data.data;
               cached_data->data_size = _data.data_size;
            }
            // Add to cache if the voice id is the cache one
            if ( !strcmp( voice_id, sgTtsCache.voice_id ) )
               _cache_add( text, &_data, &_path );
         }
      }
   }


   return result;
}

/*
 ******************************************************************************
 */
BOOL tts_cache_exists( const char* text, const char* voice_id )
{
   BOOL result = FALSE;
   int slot = -1;

   if ( !tts_cache_enabled() )
      return FALSE;

   if ( !voice_id || !strcmp( voice_id, sgTtsCache.voice_id ) )
   {
      voice_id = sgTtsCache.voice_id;
      slot = _find_entry( text, &sgTtsCache );
   }

   slot = _find_entry( text, &sgTtsCache );

   if ( slot >= 0 )
   {
      result = TRUE;
   }
   else
   {
      const TtsDbEntry* db_entry = tts_db_entry( voice_id, text, NULL );

      if ( tts_db_exists( db_entry ) )
      {
         result = TRUE;
      }
   }

   return result;
}
/*
 * Current TTS voice
 * Auxiliary
 */
static const TtsVoice* _tts_voice( void )
{
   static TtsVoice voice = TTS_VOICE_INITIALIZER;

   // Check if voice was changed
   if ( !voice.voice_id || strcmp( voice.voice_id, sgTtsCache.voice_id ) )
      tts_voices_get( sgTtsCache.voice_id, &voice );

   return &voice;
}

/*
 ******************************************************************************
 */
void tts_cache_set_voice( const char* voice_id, TtsDbDataStorageType db_type )
{
   // Don't reset cache if same voice as current.
   if ( sgTtsCache.voice_id &&
         ( !voice_id || !strcmp( voice_id, sgTtsCache.voice_id ) ) )
      return;

   sgTtsDbType = db_type;

   _reset_context( &sgTtsCache, voice_id );

   _check_db_version();
}


/*
 ******************************************************************************
 * Resets the cache context
 * Auxiliary
 */
static void _reset_context( TtsCacheContext* context, const char* voice_id )
{
   int i;
   TtsCacheEntry* cache = context->cache;
   context->voice_id = voice_id;

   context->count = 0;
   context->head = 0;

   cache[0].prev = 0;
   cache[0].next = 0;
   cache[0].key = -1;
   cache[0].text = NULL;

   for ( i = 1; i < TTS_CACHE_SIZE; ++i )
   {
      if ( cache[i].tts_data.data )
         free( cache[i].tts_data.data );
      if ( cache[i].text )
         free( cache[i].text );

      cache[i].key = -1;
      cache[i].prev = -1;
      cache[i].next = -1;
      cache[i].text = NULL;
      cache[i].tts_data.data = NULL;
      cache[i].tts_path.path[0] = 0;
   }

   roadmap_hash_clean( context->hash );
}


/*
 ******************************************************************************
 * Tries to find the entry matching the text
 * Auxiliary
 */
static int _find_entry( const char* text, TtsCacheContext* context )
{
   int slot, key;

   key = roadmap_hash_string( text );

   slot = roadmap_hash_get_first( context->hash, key );

   while (slot >= 0)
   {
     if ( !strcmp( context->cache[slot].text, text ) )
        break;

     slot = roadmap_hash_get_next( context->hash, slot );
   }

   return slot;
}
/*
 ******************************************************************************
 * Sets the element to be the MRU element of the cache
 * Auxiliary
 */
static void _set_MRU ( TtsCacheContext* context, int slot )
{
   TtsCacheEntry* cache = context->cache;

   int prev = cache[slot].prev;
   int next = cache[slot].next;
   int head = context->head;
   int tail = cache[head].prev;

   // Already the head - nothing to do
   if ( slot == head )
      return;


   // Set the MRU to be the head
   if ( slot != tail ) // if( slot == tail ) all the connections are already ok
   {
      cache[slot].prev = cache[head].prev;
      cache[slot].next = head;
      cache[tail].next = slot;

      // Fix the connections
      if ( prev >= 0 )
         cache[prev].next = next;
      if ( next >= 0 )
         cache[next].prev = prev;
   }

   // Set the current head to be the next
   cache[head].prev = slot;

   // Update head pointer
   context->head = slot;
}


/*
 ******************************************************************************
 * Removes cache entry related data
 * Auxiliary
 */
static void _remove_entry_data ( TtsCacheContext* context, int slot )
{
   TtsCacheEntry* cache_entry = &context->cache[slot];
   TtsData* tts_data = &cache_entry->tts_data;
   TtsPath* tts_path = &cache_entry->tts_path;

#ifdef TTS_CACHE_DEBUG
   roadmap_log( ROADMAP_DEBUG, "Removing entry with text '%s' from the cache. Slot: %d", cache_entry->text, slot );
#endif // TTS_CACHE_DEBUG

   roadmap_hash_remove( context->hash, cache_entry->key, slot );

   if ( tts_data->data )
      free( tts_data->data );

   tts_path->path[0] = 0;
   tts_data->data = NULL;

   free( cache_entry->text );
}

/*
 ******************************************************************************
 * Removes cache entry. Fixes the connections
 * Auxiliary
 */
static void _remove_entry ( TtsCacheContext* context, int slot )
{
   if ( slot >= 0 )
   {
      int head = sgTtsCache.head;
      int next = sgTtsCache.cache[slot].next;
      int prev = sgTtsCache.cache[slot].prev;
      TtsCacheEntry* cache_entry = &context->cache[slot];

      _remove_entry_data( &sgTtsCache, slot );

      if ( sgTtsCache.count == 1 )
      {
         _reset_context( &sgTtsCache, sgTtsCache.voice_id );
      }
      else
      {
         sgTtsCache.cache[prev].next = next;
         sgTtsCache.cache[next].prev = prev;
         if ( slot == head )
         {
            sgTtsCache.head = next;
         }
         sgTtsCache.count--;

         cache_entry->key = -1;
         cache_entry->prev = -1;
         cache_entry->next = -1;
         cache_entry->text = NULL;
      }
   }
}

/*
 ******************************************************************************
 * Tries to find the entry matching the text
 * Auxiliary
 */
static int _find_empty( void )
{
   int i;

   for ( i = 0; i < TTS_CACHE_SIZE; ++i )
   {
      TtsCacheEntry* entry = &sgTtsCache.cache[i];
      if ( entry->key == -1 )
         break;
   }

   if ( i == TTS_CACHE_SIZE )
   {
      return -1;
   }

   return i;
}

/*
 ******************************************************************************
 * Check DB version. If increased - clear the database
 * Auxiliary
 */
static void _check_db_version( void )
{
   // Destroy database if version is incremented
   if ( roadmap_config_get_integer( &RMConfigTTSDbVersion ) < TTS_DB_VERSION )
   {
      tts_db_clear( sgTtsDbType, NULL );
      roadmap_config_set_integer( &RMConfigTTSDbVersion, TTS_DB_VERSION );
   }
}
