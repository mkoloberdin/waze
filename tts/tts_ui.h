/* tts.h - The interface for the text to speech UI related functionality
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


#ifndef INCLUDE__TTS_UI__H
#define INCLUDE__TTS_UI__H
#ifdef __cplusplus
extern "C" {
#endif

void tts_ui_initialize( void );

const char** tts_ui_voices_labels( void );

const void** tts_ui_voices_values( void );

int tts_ui_count( void );

const void* tts_ui_voice_value( const char* voice_id );

const char* tts_ui_voice_label( const char* voice_id );

#ifdef __cplusplus
}
#endif
#endif // INCLUDE__TTS_UI__H
