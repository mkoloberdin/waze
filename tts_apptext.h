/* tts_apptext.h - The interface for the TTS for the common application texts module
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


#ifndef INCLUDE__TTS_APPTEXT__H
#define INCLUDE__TTS_APPTEXT__H
#ifdef __cplusplus
extern "C" {
#endif

#include "tts_apptext_defs.h"

/*
 * Initializes the TTS application texts module
 *
 * Params:  void
 *
 * Returns: void
 */
void tts_apptext_init( void );

/*
 * Plays the requested text. Assumes audio in the cache
 *
 * Params:  text - requested text
 *
 * Returns: void
 */
void tts_apptext_play( const char* text );


/*
 * Returns the sound list (self-free on play) for the given text
 *
 * Params:  text - requested text
 *
 * Returns: sound list ( Type: RoadMapSoundList )
 */
RoadMapSoundList tts_apptext_get_sound( const char* text );

/*
 * Returns true if audio is available
 *
 * Params:  text - requested text
 *
 * Returns: true/false
 */
BOOL tts_apptext_available( const char* text );

/*
 * Returns the "Start Drive" text according to the index.
 * If index exceeds the size of available strings the modulo value of the index is used
 *
 * Params:  index - non-negative index of the text
 *
 * Returns: Text. Null if index is negative.
 */
const char* tts_apptext_get_start_drive( int index );


#ifdef __cplusplus
}
#endif
#endif // INCLUDE__TTS_APPTEXT__H
