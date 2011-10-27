/* tts_utils.c - Implementation of the Text To Speech (TTS) common utilities
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
 *   See tts_utils.h
 *
 */

#include <stdio.h>
#include "roadmap_time.h"

//======================== Local defines ========================


//======================== Local types ========================


//======================== Globals ========================



//======================== Local Declarations ========================


/*
 ******************************************************************************
 */
const char* tts_utils_uid_str( void )
{
   static char s_uid[128] = {0};
   static unsigned long s_counter = 0;
   const EpochTimeMicroSec* time_now = roadmap_time_get_epoch_us( NULL );

   snprintf( s_uid, sizeof( s_uid ), "%lu-%lu-%lu", time_now->epoch_sec, time_now->usec, s_counter++ );

   return s_uid;
}



