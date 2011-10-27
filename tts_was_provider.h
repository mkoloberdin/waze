/* tts_was_provider.h - The interface for the WAS based text to speech module
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
 *
 */


#ifndef INCLUDE__TTS_WAS_PROVIDER__H
#define INCLUDE__TTS_WAS_PROVIDER__H
#ifdef __cplusplus
extern "C" {
#endif

#define TTS_WAS_VOICES_SET_PRODUCTION     "production"
#define TTS_WAS_VOICES_SET_DEBUG          "debug"


/*
 * Initializes the android native tts provider. Registers the provider
 * on the tts engine
 * Params:  void
 *
 * Returns: void
 */
void tts_was_provider_init( void );

/*
 * Applies provided set for the tts capabilities retrieval
 *
 * Params:  new_set - one of the TTS_WAS_VOICES_SET_*
 *
 * Returns: void
 */
void tts_was_provider_apply_voices_set( const char* new_set );

/*
 * Returns current set for the tts capabilities retrieval
 *
 * Params:  void
 *
 * Returns: const pointer to one of TTS_WAS_VOICES_SET_*
 */
const char* tts_was_provider_voices_set( void );

#ifdef __cplusplus
}
#endif
#endif // INCLUDE__TTS_WAS_PROVIDER__H
