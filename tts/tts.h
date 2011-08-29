/* tts.h - The interface for the text to speech module
 *                        The launcher activates external text to speech synthesis routine. The result callback should is called
 *                        upon recognition process.
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


#ifndef INCLUDE__TTS__H
#define INCLUDE__TTS__H
#ifdef __cplusplus
extern "C" {
#endif


#include "roadmap.h"
#include "tts_defs.h"
#include "roadmap_sound.h"

#define TTS_FILTER_PREFIX_MAX_NUM                  32       // Maximum number of prefixes that can be registered for filtering

#define TTS_DEFAULT_TIMEOUT                          60     // Default timeout for the response waiting (seconds)
                                                            // By passing this period the request is aborted if not completed

/* Interface wrappers for the text type */
#define TTS_TEXT_TYPE_DEFAULT                        (__tts_text_type_default)
#define TTS_TEXT_TYPE_STREET                         (__tts_text_type_street)

typedef struct _TtsPlayList* TtsPlayList;


/*
 * Triggers voice changed event for interested clients.
 * Registration on this event through tts_register_on_voice_changed
 *
 * Params:   new_voice_id - New voice id
 *           force_recommit - indicates that all the requests should be re-committed due to this voice update
 * Returns: void
 */
typedef void (*TtsOnVoiceChangedCb) ( const char* new_voice_id, BOOL force_recommit );


/*
 * Supplied by the client (service consumer). Notifies of the completion of the text synthesis.
 * When ready user can access the audio data of the text through tts_get_path, tts_get_data functions
 * User can access the data
 * Params:   user_context - user context
 *           res_status - result status of the tts process
 *           text - requested text
 *
 */
typedef void (*TtsRequestCompletedCb)( const void* user_context, int res_status, const char* text );


/*
 * Registers new TTS provider. Doesn't check if the provider exists
 * Params:  tts_provider - parameters structure.
 *
 * Returns: true - success
 */
BOOL tts_register_provider( const TtsProvider* tts_provider );
/*
 * Updates registered TTS provider.
 * Params:  tts_provider - parameters structure.
 *
 * Returns: true - success, false - not found
 */
BOOL tts_update_provider( const TtsProvider* tts_provider );

/*
 * Initialization procedure (called at the first TTS request)
 */
void tts_initialize( void );

/*
 * Posts the simple tts request. Uses defaults for all parameters (see tts_request_ex )
 * Params:  text - text to synthesize
 * Returns: void
 */
void tts_request( const char* text );
/*
 * Posts the extended tts request. Supplies the callback, binds the context for the callback
 * Params:  text - text to synthesize
 *          text_type - text type to request ( see TtsTextType ). Default: __tts_text_type_default
 *          completed_cb - the callback to be called upon completion
 *          user_context - the context to be supplied to the callback
 *          flags - flag parameters
 * Returns: void
 */
void tts_request_ex( const char* text, TtsTextType text_type, TtsRequestCompletedCb completed_cb, void* user_context, int flags );

 /*
  * Commits the prepared items and posts the requests to the server for those which are not in cache
  * Params:  void
  *
  *
  * Returns: true if commit was successful
  */
 BOOL tts_commit( void );

 /*
  * Clears all the cache on device
  * Params:  void
  *
  *
  * Returns: void
  */
 void tts_clear_cache( void );

 /*
  * Returns path of the audio file for the supplied text and voice (if voice is NULL - current voice is applied)
  *
  * Params:  text - text to look for
  *          voice_id - voice of the audio to look for
  * Returns: Path if text is synthesized, NULL otherwise
  */
 const TtsPath* tts_get_path( const char* text, const char* voice_id );

 /*
  * Returns data buffer of the audio for the supplied text and voice (if voice is NULL - current voice is applied)
  *
  * Params:  text - text to look for
  *          voice_id - voice of the audio to look for
  * Returns: Pointer to the TtsData if text is synthesized, NULL otherwise
  */
 const TtsData* tts_get_data( const char* text, const char* voice_id );

 /*
  * Returns true if text audio is in cache or database for the supplied voice (can be null - using the current voice)
  * Params:  text to check if available
  *          voice_id - voice to check for
  * Returns: true if available
  */
 BOOL tts_text_available( const char* text, const char* voice_id );
 /*
  * Creates the playlist for the tts items
  * Params:  voice_id - voice_id of the playlist
  *
  * Returns: pointer to the TtsPlayList structure
  */
 TtsPlayList tts_playlist_create( const char* voice_id );
 /*
  * Adds the text to the tts playlist
  * Params:  list - playlist to add to
  *          text - the text to add synthesis of
  * Returns: TRUE - success ( the text is available )
  */
 BOOL tts_playlist_add( TtsPlayList list, const char* text );

 /*
  * Plays the list. After the play it's no longer available
  * Params:  list - playlist to play
  *
  * Returns: TRUE - success
  */
 BOOL tts_playlist_play( TtsPlayList list );

 /*
  * Exports the sound list in the RoadMapSoundList format
  * Params:  list - tts playlist to export.
  *          free_tts_playlist - TRUE, free the tts list upon export
  *
  * Returns: exported list in RoadMapSoundList format
  */
 RoadMapSoundList tts_playlist_export_list( TtsPlayList list, BOOL free_tts_playlist );


 /*
  * Destroys the list. Only call this if list has not been played
  * Params:  list - playlist
  *
  * Returns: TRUE - success
  */
 void tts_playlist_free( TtsPlayList list );
/*
 * Stops the recognition process. Supplied callback ( in start function ) is called
 * with the results till that point
 */
void tts_abort( void );

/*
 * Indicates whether the TTS is initialized and can be used
 */
int tts_enabled( void );

/*
 * Indicates whether the TTS feature is enabled in configuration
 */
int tts_feature_enabled( void );

/*
 * Overrides feature enabled configuration
 */
void tts_set_feature_enabled( BOOL value );

/*
 * Forces configuration loading. Updates current state with this from the configuration
 */
void tts_load_config( void );

/*
 * Set voice for the TTS engine
 * Params:  voice_id - new voice id
 *
 * Returns: void
 */
void tts_set_voice( const char* voice_id );

/*
 * Returns voice id
 *
 * Returns: void
 */
const char* tts_voice_id( void );

/*
 * Registers the callback to be called upon voice changing
 *
 * Returns: void
 */
void tts_register_on_voice_changed( TtsOnVoiceChangedCb cb );

/*
 * Saves configuration, frees all dynamic allocations
 * Params:  void
 *
 * Returns: void
 */
void tts_shutdown( void );

/*
 * Converts the text type value to the string definition
 * Params:  text_type - the value of the text type
 *
 * Returns: string format of the text type
 */
const char* tts_text_type_string( TtsTextType text_type );

/*
 * Registers prefix for filtering. This prefixes are dropped and are do not appear in the tts request
 * Params:  prefix
 *
 * Returns: TRUE - if prefix not NULL and added successfully (list is not overflowed )
 */
//BOOL tts_register_fiter_prefix( const char* prefix );

#ifdef __cplusplus
}
#endif
#endif // INCLUDE__TTS__H
