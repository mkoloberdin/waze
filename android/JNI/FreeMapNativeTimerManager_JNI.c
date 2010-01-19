/* FreeMapNativeTimerManager_JNI.c - JNI STUB for the FreeMapNativeTimerManager class
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
 *   See FreeMapNativeTimerManager_JNI.h
 */
#include "FreeMapNativeTimerManager_JNI.h"
#include "FreeMapJNI.h"
#include "roadmap.h"
// The JNI object representing the current class
static android_jni_obj_type gJniObj;


#define JNI_CALL_FreeMapNativeTimerManager_AddTask 		"AddTask"
#define JNI_CALL_FreeMapNativeTimerManager_AddTask_Sig 	"(III)V"

#define JNI_CALL_FreeMapNativeTimerManager_RemoveTask 		"RemoveTask"
#define JNI_CALL_FreeMapNativeTimerManager_RemoveTask_Sig 	"(I)V"

#define JNI_CALL_FreeMapNativeTimerManager_ShutDown 		"ShutDown"
#define JNI_CALL_FreeMapNativeTimerManager_ShutDown_Sig 	"()V"


/*************************************************************************************************
 * Java_com_waze_FreeMapNativeTimerManager_InitTimerManagerNTV()
 * Initializes the JNI environment object for the FreeMapNativeTimerManager class
 *
 */
JNIEXPORT void JNICALL Java_com_waze_FreeMapNativeTimerManager_InitTimerManagerNTV
  ( JNIEnv * aJNIEnv, jobject aJObj )
{
    JNI_LOG( ROADMAP_INFO, "Initializing JNI object FreeMapNativeTimerManager" );
	// JNI object initialization
	InitJNIObject( &gJniObj, aJNIEnv, aJObj, "FreeMapNativeTimerManager" );
}

/*************************************************************************************************
 * FreeMapNativeTimerManager_AddTask()
 * Schedules the new timer
 * aIndex - the timer index
 * aTimerMsg - the timer message to be posted to the main thread upon expiration
 * aInterval - the expiration interval
 *
 */
void FreeMapNativeTimerManager_AddTask( int aIndex, int aTimerMsg, int aInterval )
{
	JNIEnv *env;
	jmethodID mid;
	android_method_context_type lMthdContext;
	JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_FreeMapNativeTimerManager_AddTask );
	mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_FreeMapNativeTimerManager_AddTask,
																JNI_CALL_FreeMapNativeTimerManager_AddTask_Sig );
	if ( !mid || !lMthdContext.env )
	{
		roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
		return;
	}
	// Call the method
	(*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid, aIndex, aTimerMsg, aInterval );
}
/*************************************************************************************************
 * FreeMapNativeTimerManager_RemoveTask()
 * Removing the timer task according to the id
 *
 */
void FreeMapNativeTimerManager_RemoveTask( int aIndex )
{

	JNIEnv *env;
	jmethodID mid;
	android_method_context_type lMthdContext;

	JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_FreeMapNativeTimerManager_RemoveTask );
	mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_FreeMapNativeTimerManager_RemoveTask,
																	JNI_CALL_FreeMapNativeTimerManager_RemoveTask_Sig );
	if ( !mid || !lMthdContext.env )
	{
		roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
		return;
	}

	// Call the method
	(*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid, aIndex );

}

/*************************************************************************************************
 * FreeMapNativeTimerManager_ShutDown()
 * Stops all the tasks and closes the manager
 *
 */
void FreeMapNativeTimerManager_ShutDown()
{

	JNIEnv *env;
	jmethodID mid;
	android_method_context_type lMthdContext;

	JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_FreeMapNativeTimerManager_ShutDown );
	mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_FreeMapNativeTimerManager_ShutDown,
																	JNI_CALL_FreeMapNativeTimerManager_ShutDown_Sig );
	if ( !mid || !lMthdContext.env )
	{
		roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
		return;
	}

	// Call the method
	(*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid );

}

/*************************************************************************************************
 * FreeMapNativeTimerManager_DisposeRefs()
 * Dispose the JNI object of the FreeMapNativeTimerManager module
 *
 */
void FreeMapNativeTimerManager_DisposeRefs()
{
    JNI_LOG( ROADMAP_INFO, "Disposing the references for the JNI object %s", gJniObj.name );
	DisposeJNIObject( &gJniObj );
}
