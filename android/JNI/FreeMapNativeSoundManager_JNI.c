/* FreeMapNativeSoundManager_JNI.c - Sound management
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
 *   See FreeMapNativeSoundManager_JNI.h
 */

#include "FreeMapNativeSoundManager_JNI.h"
#include "FreeMapJNI.h"
#include "roadmap.h"
#include <string.h>
// The JNI object representing the current class
static android_jni_obj_type gJniObj;


#define JNI_CALL_FreeMapNativeSoundManager_PlayFile                 "PlayFile"
#define JNI_CALL_FreeMapNativeSoundManager_PlayFile_Sig             "([B)V"

#define JNI_CALL_FreeMapNativeSoundManager_PlayBuffer                 "PlayBuffer"
#define JNI_CALL_FreeMapNativeSoundManager_PlayBuffer_Sig             "([B)V"

#define JNI_CALL_FreeMapNativeSoundManager_LoadSoundData                 "LoadSoundData"
#define JNI_CALL_FreeMapNativeSoundManager_LoadSoundData_Sig             "([B)V"


/*************************************************************************************************
 * Java_com_waze_FreeMapNativeSoundManager_InitSoundManagerNTV()
 * Initializes the JNI environment object for the FreeMapNativeSoundManager class
 *
 */
JNIEXPORT void JNICALL Java_com_waze_FreeMapNativeSoundManager_InitSoundManagerNTV
  ( JNIEnv * aJNIEnv, jobject aJObj )
{
	roadmap_log( ROADMAP_INFO, "Initializing JNI object FreeMapNativeSoundManager" );
	// JNI object initialization
	InitJNIObject( &gJniObj, aJNIEnv, aJObj, "FreeMapNativeSoundManager" );
}


/*************************************************************************************************
 * FreeMapNativeSoundManager_PlayFile()
 * Plays the sound file synchronously by calling the Java layer function JNI_CALL_FreeMapNativeSoundManager_PlayFile
 * aFileName - the full path to the sound file
 *
 */
void FreeMapNativeSoundManager_PlayFile( const char* aFileName )
{
	JNIEnv *env;
	jmethodID mid;
	jbyteArray byteArray;
	android_method_context_type lMthdContext;
	JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_FreeMapNativeSoundManager_PlayFile );
	mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_FreeMapNativeSoundManager_PlayFile,
														JNI_CALL_FreeMapNativeSoundManager_PlayFile_Sig );
	if ( !mid || !lMthdContext.env )
	{
		roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
		return;
	}

	byteArray = (*lMthdContext.env)->NewByteArray( lMthdContext.env, strlen( aFileName ) );
	(*lMthdContext.env)->SetByteArrayRegion( lMthdContext.env, byteArray, 0, strlen( aFileName ), aFileName );
	// Call the method
	(*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid, byteArray );
}


/*************************************************************************************************
 * FreeMapNativeSoundManager_PlayBuffer()
 * Plays the sound buffer
 * aFileName - the full path to the sound file
 *
 */
void FreeMapNativeSoundManager_PlayBuffer( void* aBuf, int aSize )
{
   JNIEnv *env;
   jmethodID mid;
   jbyteArray byteArray;
   android_method_context_type lMthdContext;
   JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_FreeMapNativeSoundManager_PlayBuffer );
   mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_FreeMapNativeSoundManager_PlayBuffer,
                                                               JNI_CALL_FreeMapNativeSoundManager_PlayBuffer_Sig );
   if ( !mid || !lMthdContext.env )
   {
      roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
      return;
   }

   byteArray = (*lMthdContext.env)->NewByteArray( lMthdContext.env, aSize );
   (*lMthdContext.env)->SetByteArrayRegion( lMthdContext.env, byteArray, 0, aSize, aBuf );
   // Call the method
   (*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid, byteArray );
}

/*************************************************************************************************
 * FreeMapNativeSoundManager_LoadSoundData()
 * PreLoads the sounds files to the memory
 * aDataDir - the full path to the sound resources directory
 *
 */
void FreeMapNativeSoundManager_LoadSoundData( char* aDataDir )
{
	JNIEnv *env;
	jmethodID mid;
	jbyteArray byteArray;
	android_method_context_type lMthdContext;
	JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_FreeMapNativeSoundManager_LoadSoundData );
	mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_FreeMapNativeSoundManager_LoadSoundData,
														JNI_CALL_FreeMapNativeSoundManager_LoadSoundData_Sig );
	if ( !mid || !lMthdContext.env )
	{
		roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
		return;
	}

	byteArray = (*lMthdContext.env)->NewByteArray( lMthdContext.env, strlen( aDataDir ) );
	(*lMthdContext.env)->SetByteArrayRegion( lMthdContext.env, byteArray, 0, strlen( aDataDir ), aDataDir );
	// Call the method
	(*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid, byteArray );
}


/*************************************************************************************************
 * FreeMapNativeSoundManager_DisposeRefs()
 * Dispose the JNI object of the FreeMapNativeSoundManager module
 *
 */
void FreeMapNativeSoundManager_DisposeRefs()
{
    JNI_LOG( ROADMAP_INFO, "Disposing the references for the JNI object %s", gJniObj.name );
	DisposeJNIObject( &gJniObj );
}
