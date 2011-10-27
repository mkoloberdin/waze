/* tts_queue.h - The interface for the text to speech module queue management
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


#ifndef INCLUDE__TTS_QUEUE__H
#define INCLUDE__TTS_QUEUE__H
#ifdef __cplusplus
extern "C" {
#endif
#include <time.h>

#include "tts_defs.h"

#define TTS_QUEUE_SIZE                             (TTS_ACTIVE_REQUESTS_LIMIT)
#define TTS_QUEUE_STATUS_UNDEFINED                -0xFFFFFFFF
#define TTS_QUEUE_STATUS_IDLE                      0
#define TTS_QUEUE_STATUS_CACHED                    1
#define TTS_QUEUE_STATUS_COMMITTED                 2


/*
 * Initializes the queue engine
 * Params:  void
 *
 * Returns: void
 */
void tts_queue_init( void );

/*
 * Adds element to the TTS requests queue
 * Params:  context - entry data to store
 *          key - the unique key string for the data in the entry
 *
 * Returns: index of the entry in the queue
 */
int tts_queue_add( void* context, const char* key );

/*
 * Gets index of the queue entry according to the text
 * Params:  key - string to look for
 *
 * Returns: index of the entry in the queue. (-1) - not found
 */
int tts_queue_get_index( const char* key );

/*
 * Remove the entry from the queue
 * Params:  index of the entry in the queue
 *
 * Returns: void
 */
void tts_queue_remove( int index );

/*
 * Remove the entry from the head of the queue
 * Params:  void
 *
 * Returns: void
 */
void tts_queue_remove_head( void );
/*
 * Sets the status of index element in the queue
 * Params:  index - index of the entry in the queue
 *          status - status value to update
 * Returns: void
 */
void tts_queue_set_status( int index, int status );
/*
 * Sets the context of index element in the queue
 * Params:  index - index of the entry in the queue
 *          context - context pointer to set
 * Returns: void
 */
void tts_queue_set_context( int index, void* context );
/*
 * Returns the data of the element in the queue
 * Params:  index - index of the entry in the queue
 *
 * Returns: data (void*)
 */
void* tts_queue_get_context( int index );
/*
 * Returns the status of the element in the queue
 * Params:  index - index of the entry in the queue
 *
 * Returns: status (int)
 */
int tts_queue_get_status( int index );
/*
 * Returns the key of the element in the queue
 * Params:  index - index of the entry in the queue
 *
 * Returns: key (const char*)
 */
const char* tts_queue_get_key( int index );
/*
 * Returns 1 if the queue entry is empty (removed/not assigned)
 * Params:  index - index of the entry in the queue
 *
 * Returns: is emtpy status (int)
 */
int tts_queue_is_empty( int index );
/*
 * Returns the indexes of the elements matching the provided status
 * Params:  indexes - array of indexes to store to
 *          indexes_size - the size of the indexes array
 *          status - the status to look for
 *
 * Returns: Count of the found entries
 */
int tts_queue_get_indexes( int indexes[], int indexes_size, int status );


/*
 * Saves configuration, frees all dynamic allocations
 * Params:  void
 *
 * Returns: void
 */
void tts_queue_shutdown( void );


#ifdef __cplusplus
}
#endif
#endif // INCLUDE__TTS_QUEUE__H
