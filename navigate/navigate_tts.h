/* navigate_tts.h - The interface to the Navigation module client of the Text To Speech (TTS) engine
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


#ifndef INCLUDE__NAVIGATE_TTS__H
#define INCLUDE__NAVIGATE_TTS__H

#ifdef __cplusplus
extern "C" {
#endif

#include "navigate_main.h"
#include "roadmap_plugin.h"
#include "navigate_tts_defs.h"





typedef struct
{
   int announce_state;
   const NavigateSegment *segment;
   const NavigateSegment *next_segment;
   const char* dest_street;
   const char* dest_street_number;
} NavTtsState;

#define NAV_TTS_STATE_INITIALIZER         { NAV_ANNOUNCE_STATE_UNDEFINED, NULL, NULL, NULL, NULL }
/*
 * Navigate TTS initialization
 * Params:  void
 *
 * Returns: void
 */
void navigate_tts_initialize( void );


/*
 * Posts the TTS request for the predefined texts
 * Params:  apply_lang - if apply the _lang_get for the predefined text
 *
 * Returns: Number of the requested strings
 */
int navigate_tts_preload( int apply_lang );

/*
 * indicates if tts is available at this navigation request.
 * Params:  void
 *
 * Returns: BOOL (true - available)
 */
BOOL navigate_tts_available( void );

/*
 * Prepares the TTS request for the given street ( if engine is available )
 * Params:  segment - Segment to prepare
 *
 * Returns: BOOL (true - success)
 */
BOOL navigate_tts_prepare_street( const NavigateSegment* segment );

/*
 * Prepares the TTS request for the given street ( if engine is available )
 * Params:  street - destination street
 *          street_num - destination number
 * Returns: BOOL (true - success)
 */
BOOL navigate_tts_prepare_arrive( const char* street, const char* street_num );


/*
 * Plays the current TTS playlist
 * Params:  void
 *
 * Returns: TRUE - if text is available and TTS engine is available
 */
BOOL navigate_tts_playlist_play( void );


BOOL navigate_tts_playlist_add_within( const char* distance_str );

BOOL navigate_tts_playlist_add_instr( const NavTtsState* state );

BOOL navigate_tts_playlist_add_andthen( NavSmallValue instruction, NavSmallValue exit_no, const NavTtsState* state );

BOOL navigate_tts_playlist_add_arrive( const NavTtsState* state );

/*
 * Adds the text to the playlist
 * Params:  void
 *
 * Returns: TRUE - if text is available and TTS engine is available
 */
BOOL navigate_tts_playlist_add( const char* text );

/*
 * Maps instruction codes to the instruction texts ( see NavigateInstr )
 * Params:  instruction - instruction code
 *
 * Returns: Text of the instruction or NULL
 */
const char* navigate_tts_instruction_text( NavSmallValue instruction );

/*
 * Prepares Navigation TTS context for the route
 * Params:  void
 *
 * Returns: void
 */
void navigate_tts_prepare_context( void );

/*
 * Resets Navigation TTS context
 * Params:  void
 *
 * Returns: void
 */
void navigate_tts_finish_route( void );


/*
 * Submits TTS request for the prepared TTS items till the call time.
 * Params:  void
 *
 * Returns: void
 */
void navigate_tts_commit( void );

/*
 * Returns approximation of the distance in meters that will be passed during the TTS instruction pronounce
 * Params:  state - navigation state for tts module
 *          speed_kph - current speed in km per hour
 *
 * Returns: the distance approximation ( in meters )
 */
int navigate_tts_announce_dist_factor( const NavTtsState* state, int speed_kph );

/*
 * Sets the navigate tts engine as unavailable
 * Params:  void
 *
 * Returns: void
 */
void navigate_tts_set_unavailable( void );

#endif // INCLUDE__NAVIGATE_TTS__H
