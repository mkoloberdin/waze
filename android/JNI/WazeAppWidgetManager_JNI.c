/* WazeAppWidgetManager_JNI.c - JNI STUB for the WazeAppWidgetManager class
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
 *   See WazeAppWidgetManager_JNI.h
 */
#include <string.h>
#include "WazeAppWidgetManager_JNI.h"
#include "FreeMapJNI.h"
#include "roadmap.h"
#include "roadmap_appwidget.h"

// The JNI object representing the current class
static android_jni_obj_type gJniObj;

#define JNI_CALL_WazeAppWidgetManager_RouteRequestCallback               "RouteRequestCallback"
#define JNI_CALL_WazeAppWidgetManager_RouteRequestCallback_Sig           "(ILjava/lang/String;Ljava/lang/String;I)V"

#define JNI_CALL_WazeAppWidgetManager_RequestRefresh      "RequestRefresh"
#define JNI_CALL_WazeAppWidgetManager_RequestRefresh_Sig     "()V"

/*************************************************************************************************
 * Java_com_waze_WazeAppWidgetManager_InitNTV()
 * Initializes the JNI environment object for the WazeAppWidgetManager class
 *
 */
JNIEXPORT void JNICALL Java_com_waze_widget_WazeAppWidgetManager_InitNTV
( JNIEnv* aJNIEnv, jobject aJObj )
{
   JNI_LOG( ROADMAP_INFO, "Initializing JNI object WazeAppWidgetManager" );
   // JNI object initialization
   InitJNIObject( &gJniObj, aJNIEnv, aJObj, "WazeAppWidgetManager" );
}



/*************************************************************************************************
 * Java_com_waze_WazeAppWidgetManager_RouteRequestNTV()
 * Route request from the widget
 *
 */
JNIEXPORT void JNICALL Java_com_waze_widget_WazeAppWidgetManager_RouteRequestNTV
( JNIEnv* aJNIEnv, jobject aJObj )
{
   roadmap_appwidget_request_route();
}


/*************************************************************************************************
 * WazeAppWidgetManager_RouteRequestCallback
 * Calls java method. Passes the destination description and time to destination
 *
 */
void WazeAppWidgetManager_RouteRequestCallback(  int aStatus, const char* aErrDesc, const char* aDestDescription, int aTimeToDest )
{
    android_method_context_type lMthdContext;
    int retVal = -1;
    jmethodID mid;
    jstring description, err_desc;
    JNIEnv* env;

    JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_WazeAppWidgetManager_RouteRequestCallback );
    mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_WazeAppWidgetManager_RouteRequestCallback,
                                                          JNI_CALL_WazeAppWidgetManager_RouteRequestCallback_Sig );
    if ( !mid || !lMthdContext.env )
    {
        roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
        return;
    }

    env = lMthdContext.env;

    description = (*lMthdContext.env)->NewStringUTF( lMthdContext.env, aDestDescription );

    err_desc = (*lMthdContext.env)->NewStringUTF( lMthdContext.env, aErrDesc );
    // Calling the method
    (*env)->CallVoidMethod( env, gJniObj.obj, lMthdContext.mid, aStatus, err_desc, description, aTimeToDest );
}

/*************************************************************************************************
 * WazeAppWidgetManager_RequestRefresh()
 *
 *
 */
void WazeAppWidgetManager_RequestRefresh()
{
    android_method_context_type lMthdContext;
    jmethodID mid;
    JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_WazeAppWidgetManager_RequestRefresh );
    mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_WazeAppWidgetManager_RequestRefresh,
                                            JNI_CALL_WazeAppWidgetManager_RequestRefresh_Sig );
    if ( !mid || !lMthdContext.env )
    {
        roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
        return;
    }

    // Calling the method
    (*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid );
}

/*************************************************************************************************
 * WazeAppWidgetManager_DisposeRefs()
 * Dispose the JNI object of the WazeAppWidgetManager module
 *
 */
void WazeAppWidgetManager_DisposeRefs()
{
   JNI_LOG( ROADMAP_INFO, "Disposing the references for the JNI object %s", gJniObj.name );
   DisposeJNIObject( &gJniObj );
}
