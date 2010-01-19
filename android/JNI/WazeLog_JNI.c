/* WazeLog_JNI.c - JNI STUB for the WazeLog class
 *
 *
 * LICENSE:
 *
 *   Copyright 2008 Alex Agranovich
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
 * SYNOPSYS:
 *
 *   See WazeLog_JNI.h
 */

#include "FreeMapJNI.h"
#include "roadmap.h"

// The JNI object representing the current class
static android_jni_obj_type gJniObj;


/*************************************************************************************************
 * Java_com_waze_WazeLog_WazeLogNTV
 * Logger interface for the java layer
 *
 */
JNIEXPORT void JNICALL Java_com_waze_WazeLog_WazeLogNTV
 ( JNIEnv* aJNIEnv, jobject aJObj, jint aLogLevel, jstring aLogStr )
{

	jboolean isCopy;
	const char* strLog = (*aJNIEnv)->GetStringUTFChars( aJNIEnv, aLogStr, &isCopy );

	switch ( aLogLevel )
	{
		case ROADMAP_MESSAGE_DEBUG:
		{
			roadmap_log( ROADMAP_DEBUG, strLog );
			break;
		}
		case ROADMAP_MESSAGE_INFO:
		{
			roadmap_log( ROADMAP_INFO, strLog );
			break;
		}
		case ROADMAP_MESSAGE_WARNING:
		{
			roadmap_log( ROADMAP_WARNING, strLog );
			break;
		}
		case ROADMAP_MESSAGE_ERROR:
		{
			roadmap_log( ROADMAP_ERROR, strLog );
			break;
		}
		case ROADMAP_MESSAGE_FATAL:
		{
			roadmap_log( ROADMAP_FATAL, strLog );
			break;
		}
		default:
		{
			roadmap_log( ROADMAP_ERROR, "Undefined logger level: %d", aLogLevel );
			break;
		}
	}

	(*aJNIEnv)->ReleaseStringUTFChars( aJNIEnv, aLogStr, strLog );
}

