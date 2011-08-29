/* tts_utils.h - The interface for the Text To Speech (TTS) common utilities
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


#ifndef INCLUDE__TTS_UTILS__H
#define INCLUDE__TTS_UTILS__H
#ifdef __cplusplus
extern "C" {
#endif


/*
 * Returns unique id string based on the current time
 * Params:  void
 * Returns: uid string ( need to be duplicate )
 */
const char* tts_utils_uid_str( void );


#ifdef __cplusplus
}
#endif
#endif // INCLUDE__TTS_UTILS__H
