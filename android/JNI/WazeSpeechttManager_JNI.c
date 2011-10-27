/* WazeSpeechttManager_JNI.c - JNI STUB for the WazeSpeechttManager class
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
 *   See WazeSpeechttManager_JNI.h
 */
#include <string.h>
#include "WazeSpeechttManager_JNI.h"
#include "WazeSpeechttManagerBase_JNI.h"
#include "FreeMapJNI.h"
#include "roadmap.h"
#include "roadmap_androidspeechtt.h"

// The JNI object representing the current class
static android_jni_obj_type gJniObj;


#define JNI_CALL_WazeSpeechttManager_StartNative      "StartNative"
#define JNI_CALL_WazeSpeechttManager_StartNative_Sig  "(JI[B[BI)V"

#define JNI_CALL_WazeSpeechttManager_Stop      "Stop"
#define JNI_CALL_WazeSpeechttManager_Stop_Sig  "()V"

#define JNI_CALL_WazeSpeechttManager_IsAvailable      "IsAvailable"
#define JNI_CALL_WazeSpeechttManager_IsAvailable_Sig  "()Z"


/*************************************************************************************************
 * Java_com_waze_WazeSpeechttManager_InitMsgBoxNTV()
 * Initializes the JNI environment object for the WazeSpeechttManager class
 *
 */
JNIEXPORT void JNICALL Java_com_waze_WazeSpeechttManagerBase_InitSpeechttManagerNTV
( JNIEnv* aJNIEnv, jobject aJObj )
{
   JNI_LOG( ROADMAP_INFO, "Initializing JNI object WazeSpeechttManager" );
   // JNI object initialization
   InitJNIObject( &gJniObj, aJNIEnv, aJObj, "WazeSpeechttManager" );
}

/*************************************************************************************************
 * Java_com_waze_WazeSpeechttManager_SpeechttManagerCallbackNTV
 * Callback
 *
 */
JNIEXPORT void JNICALL Java_com_waze_WazeSpeechttManagerBase_SpeechttManagerCallbackNTV
  (JNIEnv * aJNIEnv, jobject aJObj, jlong aCbContext, jstring aBuffer, jint res_status )
{
   char* bufferOut = NULL;
   AndrSpeechttCbContext* ctx =  (AndrSpeechttCbContext*) aCbContext;
   roadmap_log( ROADMAP_DEBUG, "Calling the callback 0x%x", ctx->callback );

   if ( res_status == SPEECHTT_RES_STATUS_SUCCESS )
   {
     // Retrieve the string (should be null terminated
     jboolean isCopy;
     int length = (*aJNIEnv)->GetStringUTFLength( aJNIEnv, aBuffer );
     bufferOut = calloc( length + 1, 1 ); // Zero initialized. One byte to ensure zero termination
     const char* text = (*aJNIEnv)->GetStringUTFChars( aJNIEnv, aBuffer, &isCopy );

     memcpy( bufferOut, text, length );

     (*aJNIEnv)->ReleaseStringUTFChars( aJNIEnv, aBuffer, text );
  }

  if ( ctx->callback )
  {
     ctx->callback( ctx->context, res_status, bufferOut );
  }

  free( ctx );
}
/*************************************************************************************************
 * WazeSpeechttManager_StartNative()
 * Starts the speech to text engine
 * aContext - callback context
 * aTimeout - timeout
 *
 */
void WazeSpeechttManager_StartNative( void* aContext, int aTimeout, const char* aLang, const char* aExtraPrompt, int aFlags )
{
    android_method_context_type lMthdContext;
    jmethodID mid;
    jbyteArray langTag = NULL;
    jbyteArray extraPrompt = NULL;

    JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_WazeSpeechttManager_StartNative );
    mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_WazeSpeechttManager_StartNative,
                                                                JNI_CALL_WazeSpeechttManager_StartNative_Sig );
    if ( !mid || !lMthdContext.env )
    {
        roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
        return;
    }

    jlong ptrCtx = (jlong) aContext;

    if ( aLang != NULL )
    {
       langTag = (*lMthdContext.env)->NewByteArray( lMthdContext.env, strlen( aLang ) );
       (*lMthdContext.env)->SetByteArrayRegion( lMthdContext.env, langTag, 0, strlen( aLang ), aLang );
    }

    if ( aExtraPrompt != NULL )
    {
       extraPrompt = (*lMthdContext.env)->NewByteArray( lMthdContext.env, strlen( aExtraPrompt ) );
       (*lMthdContext.env)->SetByteArrayRegion( lMthdContext.env, extraPrompt, 0, strlen( aExtraPrompt ), aExtraPrompt );
    }

    // Calling the method
    (*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid,
          ptrCtx, aTimeout, langTag, extraPrompt, aFlags );
}

/*************************************************************************************************
 * WazeSpeechttManager_Stop()
 * Starts the speech to text engine
 * aExecCallback - if the callback should be executed
 *
 *
 *
 */
void WazeSpeechttManager_Stop( void )
{
    android_method_context_type lMthdContext;
    jmethodID mid;
    JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_WazeSpeechttManager_Stop );
    mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_WazeSpeechttManager_Stop,
                                                                JNI_CALL_WazeSpeechttManager_Stop_Sig );
    if ( !mid || !lMthdContext.env )
    {
        roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
        return;
    }

    // Calling the method
    (*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid );
}

/*************************************************************************************************
 * WazeSpeechttManager_IsAvailable()
 * Checks if the speech to text available
 */
int WazeSpeechttManager_IsAvailable( void )
{
    android_method_context_type lMthdContext;
    jmethodID mid;
    JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_WazeSpeechttManager_IsAvailable );
    mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_WazeSpeechttManager_IsAvailable,
                                                                JNI_CALL_WazeSpeechttManager_IsAvailable_Sig );
    if ( !mid || !lMthdContext.env )
    {
        roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
        return 0;
    }

    // Calling the method
    jboolean res = (*lMthdContext.env)->CallBooleanMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid );
    return ( res == JNI_TRUE );
}

/*************************************************************************************************
 * WazeSpeechttManager_DisposeRefs()
 * Dispose the JNI object of the WazeSpeechttManager module
 *
 */
void WazeSpeechttManager_DisposeRefs()
{
   JNI_LOG( ROADMAP_INFO, "Disposing the references for the JNI object %s", gJniObj.name );
   DisposeJNIObject( &gJniObj );
}
