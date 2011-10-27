/* WazeMsgBox_JNI.c - JNI STUB for the WazeMsgBox class
 *
 *
 * LICENSE:
 *
 *   Copyright 2010 Alex Agranovich (AGA), Waze Ltd
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
 *   See WazeMsgBox_JNI.h
 */
#include "WazeSoundRecorder_JNI.h"
#include "FreeMapJNI.h"
#include "roadmap.h"
// The JNI object representing the current class
static android_jni_obj_type gJniObj;


#define JNI_CALL_WazeSoundRecorder_Start      "Start"
#define JNI_CALL_WazeSoundRecorder_Start_Sig  "(Ljava/lang/String;I)I"
#define JNI_CALL_WazeSoundRecorder_Stop      "Stop"
#define JNI_CALL_WazeSoundRecorder_Stop_Sig  "()V"


/*************************************************************************************************
 * Java_com_waze_WazeSoundRecorder_InitSoundRecorderNTV()
 * Initializes the JNI environment object for the SoundRecorder class
 *
 */
JNIEXPORT void JNICALL Java_com_waze_WazeSoundRecorder_InitSoundRecorderNTV
  ( JNIEnv * aJNIEnv, jobject aJObj )
{
   JNI_LOG( ROADMAP_INFO, "Initializing JNI object SoundRecorder" );
   // JNI object initialization
   InitJNIObject( &gJniObj, aJNIEnv, aJObj, "WazeSoundRecorder" );
}

/*************************************************************************************************
 * WazeSoundRecorder_Start()
 * Starts the audio recorder
 * aPath - path to
 * aTimeout - timeout for recording in ms
 */
int WazeSoundRecorder_Start( const char* aPath, int aTimeout )
{
   jmethodID mid;
   android_method_context_type lMthdContext;
   jstring path = NULL;
   int res;

   JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_WazeSoundRecorder_Start );
   mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_WazeSoundRecorder_Start,
         JNI_CALL_WazeSoundRecorder_Start_Sig );
   if ( !mid || !lMthdContext.env )
   {
      roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
      return -1;
   }

   if ( aPath != NULL )
   {
      path = (*lMthdContext.env)->NewStringUTF( lMthdContext.env, aPath );
   }

   // Call the method
   res = (*lMthdContext.env)->CallIntMethod( lMthdContext.env, gJniObj.obj, mid, path, aTimeout );


   // Release local references
   if ( path )
      (*lMthdContext.env)->DeleteLocalRef( lMthdContext.env, path );

   return res;
}

/*************************************************************************************************
 * WazeSoundRecorder_Start()
 * Stops the audio recorder
 *
 *
 */
void WazeSoundRecorder_Stop( void )
{
   JNIEnv *env;
   jmethodID mid;
   android_method_context_type lMthdContext;

   JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_WazeSoundRecorder_Stop );
   mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_WazeSoundRecorder_Stop,
         JNI_CALL_WazeSoundRecorder_Stop_Sig );
   if ( !mid || !lMthdContext.env )
   {
      roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
      return;
   }

   // Call the method
   (*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, mid );
}


/*************************************************************************************************
 * WazeSoundRecorder_DisposeRefs()
 * Dispose the JNI object of the WazeMsgBox module
 *
 */
void WazeSoundRecorder_DisposeRefs()
{
   JNI_LOG( ROADMAP_INFO, "Disposing the references for the JNI object %s", gJniObj.name );
   DisposeJNIObject( &gJniObj );
}
