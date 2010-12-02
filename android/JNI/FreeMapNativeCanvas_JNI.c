/* FreeMapNativeCanvas_JNI.c - JNI STUB for the FreeMapNativeCanvas class
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
 *   See FreeMapNativeCanvas.h
 */

#include "roadmap.h"
#include "FreeMapNativeCanvas_JNI.h"
#include "FreeMapJNI.h"
#include "roadmap_androidcanvas.h"
#include "roadmap_device_events.h"
#include "roadmap_androidmain.h"
#include "roadmap_screen.h"
#include <string.h>

// The JNI object representing the current class
static android_jni_obj_type gJniObj;
// The java side canvas buffer reference
static jintArray gBufObj = 0;

#define JNI_CALL_FreeMapNativeCanvas_RefreshScreen 			"RefreshScreen"
#define JNI_CALL_FreeMapNativeCanvas_RefreshScreen_Sig 		"()V"

#define JNI_CALL_FreeMapNativeCanvas_SwapBuffersEgl 			"SwapBuffersEgl"
#define JNI_CALL_FreeMapNativeCanvas_SwapBuffersEgl_Sig 		"()V"

#define JNI_CALL_FreeMapNativeCanvas_RequestRender 			"RequestRender"
#define JNI_CALL_FreeMapNativeCanvas_RequestRender_Sig 		"(Z)V"

#define JNI_CALL_FreeMapNativeManager_IsActive 			      "IsActive"
#define JNI_CALL_FreeMapNativeManager_IsActive_Sig 			"()I"



extern void roadmap_canvas_init( int aWidth, int aHeight, int aPixelFormat );
extern void roadmap_canvas_mouse_pressed( int aXY[], int aCount );
extern void roadmap_canvas_mouse_released( int aXY[], int aCount );
extern void roadmap_canvas_mouse_moved( int aXY[], int aCount );
extern void roadmap_canvas_new();
extern int roadmap_main_time_interval( int aCallIndex, int aPrintFlag );
extern void roadmap_canvas_prepare();
extern void roadmap_canvas_prepare_main_context( void );
/*************************************************************************************************
 * Java_com_waze_FreeMapNativeCanvas_InitCanvasNTV
 * Initializes the FreeMapNativeCanvas class JNI object.
 *
 */
JNIEXPORT void JNICALL Java_com_waze_FreeMapNativeCanvas_InitCanvasNTV
  ( JNIEnv* aJNIEnv, jobject aJObj )
{
    JNI_LOG( ROADMAP_INFO, "Initializing JNI object FreeMapNativeCanvas" );
	// JNI object initialization
	InitJNIObject( &gJniObj, aJNIEnv, aJObj, "FreeMapNativeCanvas" );
}




/*************************************************************************************************
 * Java_com_waze_FreeMapNativeCanvas_CanvasConfigureNTV
 * Initializes the FreeMapNativeCanvas class JNI object.
  *
 */
JNIEXPORT void JNICALL JNICALL Java_com_waze_FreeMapNativeCanvas_CanvasConfigureNTV
  ( JNIEnv* aJNIEnv, jobject aJObj, jint aWidth, jint aHeight, jint aPixelFormat )
{
    JNI_LOG( ROADMAP_INFO, "Configuring the canvas " );


    // Set the properties for the canvas
    roadmap_canvas_init( ( int ) aWidth , ( int ) aHeight, ( int ) aPixelFormat );
}

/*************************************************************************************************
 * Java_com_waze_FreeMapNativeCanvas_CanvasOrientationUpdateNTV
 * Notifies on the orientation change event
  *
 */
JNIEXPORT void JNICALL JNICALL Java_com_waze_FreeMapNativeCanvas_CanvasOrientationUpdateNTV
  ( JNIEnv* aJNIEnv, jobject aJObj )
{
    JNI_LOG( ROADMAP_INFO, "Canvas orientation has changed" );

    roadmap_log( ROADMAP_WARNING, "AGA DEBUG Handling orientation change ... " );

    roadmap_device_event_notification( device_event_window_orientation_changed );

}
/*************************************************************************************************
 * Java_com_waze_FreeMapNativeCanvas_ForceNewCanvasNTV
 * Force creation of the new canvas
 *
 */
JNIEXPORT void JNICALL Java_com_waze_FreeMapNativeCanvas_CreateCanvasNTV
( JNIEnv* aJNIEnv, jobject aJObj )
{
	 roadmap_canvas_new();
}

#ifdef OPENGL

/*************************************************************************************************
 * FreeMapNativeCanvas_RequestRender()
 * Signals the java layer with the refresh
 *
 */
void FreeMapNativeCanvas_RequestRender()
{
	android_method_context_type lMthdContext;
	jmethodID mid;
	int* lArrPtr;
	roadmap_android_canvas_type *lCanvasPtr;
	JNIEnv *env;
	jboolean is_wait = 1;

	JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_FreeMapNativeCanvas_RequestRender );
	mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_FreeMapNativeCanvas_RequestRender,
															JNI_CALL_FreeMapNativeCanvas_RequestRender_Sig );

	if ( !mid || !lMthdContext.env )
	{
		roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
		return;
	}

	// Calling the method
	(*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid, is_wait );
}
#else
/*************************************************************************************************
 * FreeMapNativeCanvas_RefreshScreen()
 * Signals the java layer with the refresh
 *
 */
void FreeMapNativeCanvas_RefreshScreen()
{
	android_method_context_type lMthdContext;
	jmethodID mid;
	int* lArrPtr;
	roadmap_android_canvas_type *lCanvasPtr;
	JNIEnv *env;

	JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_FreeMapNativeCanvas_RefreshScreen );
	mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_FreeMapNativeCanvas_RefreshScreen,
															JNI_CALL_FreeMapNativeCanvas_RefreshScreen_Sig );

	if ( !mid || !lMthdContext.env )
	{
		roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
		return;
	}

	// Getting the canvas pointer

	GetAndrCanvas( &lCanvasPtr );

	// Checking the buffer
	if ( !lCanvasPtr->pCanvasBuf )
	{
		roadmap_log( ROADMAP_WARNING, "The native canvas is still not initialized!" );
		return;
	}
	env = lMthdContext.env;

	/* Critical section */
	(*env)->MonitorEnter( env, gJniObj.obj );

	// Native side pointer to java array
	lArrPtr = (*lMthdContext.env)->GetIntArrayElements( lMthdContext.env, gBufObj, 0 );
	// Copy the data
	memcpy( lArrPtr, lCanvasPtr->pCanvasBuf, lCanvasPtr->height * lCanvasPtr->width * DEFAULT_BPP );
	(*lMthdContext.env)->ReleaseIntArrayElements( lMthdContext.env, gBufObj, lArrPtr, 0);

	/* End of Critical section */
	(*env)->MonitorExit( env, gJniObj.obj );

	// Calling the method
	(*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid );
}
#endif




/*************************************************************************************************
 * Java_com_waze_FreeMapNativeCanvas_KeyDownHandlerNTV()
 * JNI wrapper for the roadmap_main_key_pressed (will be called from the Java layer)
 *
 */
JNIEXPORT void JNICALL Java_com_waze_FreeMapNativeCanvas_KeyDownHandlerNTV
( JNIEnv *aJNIEnv, jobject aJObj , jint aKeyCode, jboolean aIsSpecial, jbyteArray aUtf8Bytes )
{
	char utf8Bytes[64];
	// Loading the byte array data
	(*aJNIEnv)->GetByteArrayRegion( aJNIEnv, aUtf8Bytes, 0, 64, utf8Bytes );


	JNI_LOG( ROADMAP_DEBUG, "The received characters are %d %d %d %d", utf8Bytes[0], utf8Bytes[1], utf8Bytes[2], utf8Bytes[3] );

	// Calling the hanler
	roadmap_main_key_pressed( aKeyCode, ( int ) aIsSpecial, utf8Bytes );
}

/*************************************************************************************************
 *
 * Auxiliary
 *
 */
static inline int get_coord_data( JNIEnv *aJNIEnv, jintArray aXY, int aDstXY[] )
{
   int count = (*aJNIEnv)->GetArrayLength( aJNIEnv, aXY );
   if ( count > 2*MAX_CORDING_POINTS )
      count = 2*MAX_CORDING_POINTS;

   (*aJNIEnv)->GetIntArrayRegion( aJNIEnv, aXY, 0, count, aDstXY );

   return count;
}
/*************************************************************************************************
 * Java_com_waze_FreeMapNativeCanvas_MousePressedNTV
 * JNI wrapper for the Java_com_waze_FreeMapNativeCanvas_MousePressedNTV (will be called from the Java layer)
 *
 */
JNIEXPORT void JNICALL Java_com_waze_FreeMapNativeCanvas_MousePressedNTV
( JNIEnv *aJNIEnv, jobject aJObj, jintArray aXY )
{
   int XY[2*MAX_CORDING_POINTS];

   int count = get_coord_data( aJNIEnv, aXY, XY )/2;

   JNI_LOG( ROADMAP_DEBUG, "Mouse pressed event (X, Y, Index) = ( %d, %d, %d )", XY[0], XY[1], count );
	roadmap_canvas_mouse_pressed( XY, count );
}

/*************************************************************************************************
 * Java_com_waze_FreeMapNativeCanvas_MouseReleasedNTV()
 * JNI wrapper for the roadmap_canvas_agg_mouse_released (will be called from the Java layer)
 *
 */
JNIEXPORT void JNICALL Java_com_waze_FreeMapNativeCanvas_MouseReleasedNTV
( JNIEnv *aJNIEnv, jobject aJObj, jintArray aXY )
{
   int XY[2*MAX_CORDING_POINTS];

   int count = get_coord_data( aJNIEnv, aXY, XY )/2;


   JNI_LOG( ROADMAP_DEBUG, "Mouse released event (X, Y, Index) = ( %d, %d, %d )", XY[0], XY[1], count );
	roadmap_canvas_mouse_released( XY, count );
}

/*************************************************************************************************
 * Java_com_waze_FreeMapNativeCanvas_MouseMovedNTV()
 * JNI wrapper for the roadmap_canvas_agg_mouse_moved (will be called from the Java layer)
 *
 */
JNIEXPORT void JNICALL Java_com_waze_FreeMapNativeCanvas_MouseMovedNTV
( JNIEnv *aJNIEnv, jobject aJObj, jintArray aXY )
{
   int XY[2*MAX_CORDING_POINTS];

   int count = get_coord_data( aJNIEnv, aXY, XY )/2;

   JNI_LOG( ROADMAP_DEBUG, "Mouse moved event (X, Y, Index) = ( %d, %d, %d )", XY[0], XY[1], count );
	roadmap_canvas_mouse_moved( XY, count );
}

/*************************************************************************************************
 * Java_com_waze_FreeMapNativeCanvas_MouseMovedNTV()
 * JNI wrapper for the roadmap_canvas_prepare (will be called from the Java layer)
 *
 */
JNIEXPORT void JNICALL Java_com_waze_FreeMapNativeCanvas_CanvasPrepareNTV
( JNIEnv *aJNIEnv, jobject aJObj )
{
	JNI_LOG( ROADMAP_DEBUG, "The canvas prepare is called" );
	roadmap_canvas_prepare_main_context();
}

/*************************************************************************************************
 * FreeMapNativeCanvas_DisposeRefs()
 * Dispose the JNI object of the FreeMapNativeCanvas module
 *
 */
void FreeMapNativeCanvas_DisposeRefs()
{
    JNI_LOG( ROADMAP_INFO, "Disposing the references for the JNI object %s", gJniObj.name );
	DisposeCustomJNIObject( &gJniObj, gBufObj );
	DisposeJNIObject( &gJniObj );

}


/*************************************************************************************************
 * int FreeMapNativeManager_IsActive()
 * Returns the current battery level
 *
 */
int FreeMapNativeManager_IsActive()
{
	int retVal;
	android_method_context_type lMthdContext;
	jmethodID mid;
	JNIEnv* env;
	JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_FreeMapNativeManager_IsActive );

	mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_FreeMapNativeManager_IsActive,
															JNI_CALL_FreeMapNativeManager_IsActive_Sig );
	if ( !mid || !lMthdContext.env )
	{
		roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
		return;
	}

	// Calling the method
	retVal = (*lMthdContext.env)->CallIntMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid );

	return retVal;
}



/*************************************************************************************************
 * void FreeMapNativeManager_SwapBuffersEgl()
 * Causes the main loop run
 *
 */
void FreeMapNativeManager_SwapBuffersEgl()
{
	android_method_context_type lMthdContext;
	jmethodID mid;
	JNIEnv* env;
	JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_FreeMapNativeCanvas_SwapBuffersEgl );

	mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_FreeMapNativeCanvas_SwapBuffersEgl,
																JNI_CALL_FreeMapNativeCanvas_SwapBuffersEgl_Sig );
	if ( !mid || !lMthdContext.env )
	{
		roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
		return;
	}

	// Calling the method
	(*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid );

}
