/* roadmap_androidspeechtt.h - the interface to the android speech to text engine
 *
 * LICENSE:
 *
 *   Copyright 2010 Waze Ltd, Alex Agranovich (AGA)
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
 * DESCRIPTION:
 *
 *     See roadmap_speechtt.h
 *
 *
 */

#ifndef INCLUDE__ROADMAP_ANDROID_SPEECHTT__H
#define INCLUDE__ROADMAP_ANDROID_SPEECHTT__H

#include "roadmap_speechtt.h"


/*
 * JNI Callback context
 */
typedef struct
{
   RMSpeechttResponseCb callback;
   RMSpeechttContext* context;
} AndrSpeechttCbContext;

void roadmap_androidspeechtt_init( void );

#endif // INCLUDE__ROADMAP_ANDROID_SPEECHTT__H

