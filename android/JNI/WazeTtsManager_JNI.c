/* WazeTtsManager_JNI.c - JNI STUB for the WazeTtsManager class
 *
 *
 * LICENSE:
 *
 *   Copyright 2011 Alex Agranovich (AGA), Waze Ltd
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
 *   See WazeTtsManager_JNI.h
 */
#include <string.h>
#include "WazeTtsManager_JNI.h"
#include "FreeMapJNI.h"
#include "roadmap.h"
#include "tts_android_provider.h"

// The JNI object representing the current class
static android_jni_obj_type gJniObj;


#define JNI_CALL_WazeTtsManager_Submit          "Submit"
#define JNI_CALL_WazeTtsManager_Submit_Sig      "(Ljava/lang/String;Ljava/lang/String;J)V"
#define JNI_CALL_WazeTtsManager_Prepare         "Prepare"
#define JNI_CALL_WazeTtsManager_Prepare_Sig     "()V"

/*************************************************************************************************
 * Java_com_waze_WazeTtsManagerBase_InitTtsManagerNTV()
 * Initializes the JNI environment object for the WazeTtsManager class
 *
 */
JNIEXPORT void JNICALL Java_com_waze_WazeTtsManager_InitTtsManagerNTV
( JNIEnv* aJNIEnv, jobject aJObj )
{
   JNI_LOG( ROADMAP_INFO, "Initializing JNI object WazeTtsManager" );
   // JNI object initialization
   InitJNIObject( &gJniObj, aJNIEnv, aJObj, "WazeTtsManager" );

//   tts_android_provider_init();
}

/*************************************************************************************************
 * Java_com_waze_WazeTtsManagerBase_TtsManagerCallbackNTV
 * Callback
 *
 */
JNIEXPORT void Java_com_waze_WazeTtsManager_TtsManagerCallbackNTV
  (JNIEnv * aJNIEnv, jobject aJObj, jlong aCbContext, jint res_status )
{
   tts_android_provider_response( (const void*) aCbContext, res_status );
}
/*************************************************************************************************
 * WazeTtsManager_Commit()
 * Submits the synthesis request
 * aContext - callback context
 * aTimeout - timeout
 *
 */
void WazeTtsManager_Commit( const char* aText, const char* aFullPath, void* aContext )
{
    android_method_context_type lMthdContext;
    jmethodID mid;
    jstring text, path;
    jlong ptrCtx;

    JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_WazeTtsManager_Submit );
    mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_WazeTtsManager_Submit,
                                                          JNI_CALL_WazeTtsManager_Submit_Sig );
    if ( !mid || !lMthdContext.env )
    {
        roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
        return;
    }

    ptrCtx = (jlong) aContext;

    text = (*lMthdContext.env)->NewStringUTF( lMthdContext.env, aText );
    path = (*lMthdContext.env)->NewStringUTF( lMthdContext.env, aFullPath );

    // Calling the method
    (*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid,
          text, path, ptrCtx );
}

/*************************************************************************************************
 * WazeTtsManager_Prepare()
 * Prepares the Android TTS provider
 *
 */
void WazeTtsManager_Prepare( void )
{
   android_method_context_type lMthdContext;
   jmethodID mid;

   JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_WazeTtsManager_Prepare );
   mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_WazeTtsManager_Prepare,
                                                            JNI_CALL_WazeTtsManager_Prepare_Sig );
   if ( !mid || !lMthdContext.env )
   {
       roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
       return;
   }

   // Calling the method
   (*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid );

}
/*************************************************************************************************
 * WazeTtsManager_DisposeRefs()
 * Dispose the JNI object of the WazeTtsManager module
 *
 */
void WazeTtsManager_DisposeRefs( void )
{
   JNI_LOG( ROADMAP_INFO, "Disposing the references for the JNI object %s", gJniObj.name );
   DisposeJNIObject( &gJniObj );
}
