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
#include "WazeMsgBox_JNI.h"
#include "FreeMapJNI.h"
#include "roadmap.h"
#include "roadmap_androidmsgbox.h"
#include <string.h>
// The JNI object representing the current class
static android_jni_obj_type gJniObj;


#define JNI_CALL_WazeMsgBox_Show      "Show"
#define JNI_CALL_WazeMsgBox_Show_Sig  "([B[B[B[BJ)V"


/*************************************************************************************************
 * Java_com_waze_WazeMsgBox_InitMsgBoxNTV()
 * Initializes the JNI environment object for the WazeMsgBox class
 *
 */
JNIEXPORT void JNICALL Java_com_waze_WazeMsgBox_InitMsgBoxNTV
  ( JNIEnv * aJNIEnv, jobject aJObj )
{
   JNI_LOG( ROADMAP_INFO, "Initializing JNI object WazeMsgBox" );
   // JNI object initialization
   InitJNIObject( &gJniObj, aJNIEnv, aJObj, "WazeMsgBox" );
}

/*************************************************************************************************
 * Java_com_waze_WazeMsgBox_MsgBoxCallbackNTV()
 * Message box callback
 *
 */
JNIEXPORT void JNICALL Java_com_waze_WazeMsgBox_MsgBoxCallbackNTV
( JNIEnv * aJNIEnv, jobject aJObj, jint aCbRes, jlong aCbContext )
{

  AndrMsgBoxCbContext* ctx =  (AndrMsgBoxCbContext*) aCbContext;
  roadmap_log( ROADMAP_DEBUG, "Calling the callback 0x%x for the result: %d", ctx->callback, aCbRes );
  if ( ctx->callback )
  {
     // Be aware of the 1 is always "Ok" button result
     ctx->callback( aCbRes, ctx->context );
  }
  free( ctx );
}
/*************************************************************************************************
 * WazeMsgBox_Show()
 * Shows native messagebox
 * aTitle - message box title
 * aMessage - message text
 * aLabelOk - label for Ok button
 * aLabelCancel - label for Cancel button
 * aCbContext - callback context
 */
void WazeMsgBox_Show( const char* aTitle, const char* aMessage, const char* aLabelOk, const char* aLabelCancel, void* aCbContext )
{
   JNIEnv *env;
   jmethodID mid;
   android_method_context_type lMthdContext;
   jbyteArray title = NULL;
   jbyteArray message = NULL;
   jbyteArray label_ok = NULL;
   jbyteArray label_cancel = NULL;

   JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_WazeMsgBox_Show );
   mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_WazeMsgBox_Show,
         JNI_CALL_WazeMsgBox_Show_Sig );
   if ( !mid || !lMthdContext.env )
   {
      roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
      return;
   }

   if ( aTitle != NULL )
   {
      title = (*lMthdContext.env)->NewByteArray( lMthdContext.env, strlen( aTitle ) );
      (*lMthdContext.env)->SetByteArrayRegion( lMthdContext.env, title, 0, strlen( aTitle ), aTitle );
   }
   if ( aMessage != NULL )
   {
      message = (*lMthdContext.env)->NewByteArray( lMthdContext.env, strlen( aMessage ) );
      (*lMthdContext.env)->SetByteArrayRegion( lMthdContext.env, message, 0, strlen( aMessage ), aMessage );
   }
   if ( aLabelOk != NULL )
   {
      label_ok = (*lMthdContext.env)->NewByteArray( lMthdContext.env, strlen( aLabelOk ) );
      (*lMthdContext.env)->SetByteArrayRegion( lMthdContext.env, label_ok, 0, strlen( aLabelOk ), aLabelOk );
   }
   if ( aLabelCancel != NULL )
   {
      label_cancel = (*lMthdContext.env)->NewByteArray( lMthdContext.env, strlen( aLabelCancel ) );
      (*lMthdContext.env)->SetByteArrayRegion( lMthdContext.env, label_cancel, 0, strlen( aLabelCancel ), aLabelCancel );
   }

   // Call the method
   (*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid,
         title, message, label_ok, label_cancel, (jlong) aCbContext );
}

/*************************************************************************************************
 * WazeMsgBox_DisposeRefs()
 * Dispose the JNI object of the WazeMsgBox module
 *
 */
void WazeMsgBox_DisposeRefs()
{
    JNI_LOG( ROADMAP_INFO, "Disposing the references for the JNI object %s", gJniObj.name );
   DisposeJNIObject( &gJniObj );
}
