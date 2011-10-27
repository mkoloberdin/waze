/* tts_voices.h - The interface for the tts voices management module
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


#ifndef INCLUDE__TTS_VOICES__H
#define INCLUDE__TTS_VOICES__H
#ifdef __cplusplus
extern "C" {
#endif



#define TTS_VOICE_INITIALIZER {{0}, {0}, 0, {0}, {0}, {0}, 1, {0}, 0}

#define  SET_TTS_VOICE_FIELD( ptr, field, val ) \
   ( ptr->##field = (val) )

#define TTS_VOICE_FIELD_MAX_SIZE          128

#define TTS_VOICE_VALID(voice_ptr) ( voice_ptr->status == __tts_voice_status_valid )

typedef struct
{
   char service_provider[TTS_VOICE_FIELD_MAX_SIZE];        // Tts service provider. Should register to the tts engine.
                                                           // Can different voices of different voice providers
   char voice_id[TTS_VOICE_FIELD_MAX_SIZE];
   int  version;                                           // Represents the version number of the current voice
//   char voice_provider[TTS_VOICE_FIELD_MAX_SIZE];          // Tts voice provider - voice information
   char language[TTS_VOICE_FIELD_MAX_SIZE];
   char gender[TTS_VOICE_FIELD_MAX_SIZE];
   char code[TTS_VOICE_FIELD_MAX_SIZE];
   int cacheable;                                          // 1- True, 0- False
   char label[TTS_VOICE_FIELD_MAX_SIZE];
   int status;                                             // TtsVoiceStatus values
} TtsVoice;

typedef enum
{
   __tts_voice_status_invalid,
   __tts_voice_status_valid,
   __tts_voice_status_pending_invalidation
} TtsVoiceStatus;

typedef enum
{
   __tts_voice_field__voice_id = 0x0,
   __tts_voice_field__version,
   __tts_voice_field__label,
//   __tts_voice_field__provider,
   __tts_voice_field__cacheable,
   __tts_voice_field__language,
   __tts_voice_field__gender,
   __tts_voice_field__code,


   __tts_voice_fields__count
} TtsVoiceFieldIndex;


/*
 * Called upon updating voice id
 * Params:   new_voice - New voice data
 *           old_voice - Old voice data (or NULL)
 *
 */
typedef void (*TtsVoiceUpdatedCb)( const TtsVoice* new_voice, const TtsVoice* old_voice );

/*
 * Retrieves the tts voices from the server. Initializes the data structures
 *
 * Params: voice_updated_cb - called upon voice is added/updated
 */
void tts_voices_initialize( TtsVoiceUpdatedCb voice_updated_cb );

/*
 * Frees up all the memory allocations
 *
 * Params: void
 */
void tts_voices_shutdown( void );

/*
 * Retrieves the tts voices from the server. Initializes the data structures
 *
 * Params:  service_provider - the service provider name
 *          path - Path to the csv configuration file
 */
void tts_voices_update(  const char* service_provider, const char* path );

/*
 * Returns the voice information in the voice ( if not null )
 * and statically allocated pointer to the voice matching voice_id (if found)
 *
 * Params: voice_id - the unique id of the voice to find
 *          voice - the output parameter
 * Returns: static pointer to the voice structure if found, NULL otherwise
 */
const TtsVoice* tts_voices_get( const char* voice_id, TtsVoice* voice );

/*
 * Returns the voice information for the requested language
 * Places the data to the preallocated array
 *
 * Params: lang - the requested language
 *          voices - the preallocated array for the output
 *          voices_size - the size of the voices array
 * Returns: the number of the voices placed to the array
 */
int tts_voices_get_lang( const char* lang, TtsVoice voices[], int voices_size );

/*
 * Returns the voice information for all the available voices
 * Places the data to the preallocated array
 *
 * Params:  voices - the preallocated array for the output
 *          voices_size - the size of the voices array
 * Returns: the number of the voices placed to the array
 */
int tts_voices_get_all( TtsVoice voices[], int voices_size );

/*
 * Returns the voice information for the voices of the supplied service provider
 * Places the data to the preallocated array
 *
 * Params:
 *          service_provider - service provider name
 *          voices - the preallocated array for the output
 *          voices_size - the size of the voices array
 * Returns: the number of the voices placed to the array
 */
int tts_voices_get_service_provider( const char* service_provider, TtsVoice voices[], int voices_size );

/*
 * Returns the default valid voice for current service provider
 *
 * Params:
 *          service_provider - service provider name
 * Returns: the default voice id
 */
const char* tts_voices_get_service_default( const char* service_provider );

/*
 * Returns the default valid voice for current service provider
 *
 * Params: void
 *
 * Returns: the default voice id
 */
const char* tts_voices_get_default( void );
#ifdef __cplusplus
}
#endif
#endif // INCLUDE__TTS_VOICES__H
