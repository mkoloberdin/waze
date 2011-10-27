/* WazeMenuManager_JNI.c - JNI STUB for the WazeMenuManager class
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
 *   See WazeMenuManager_JNI.h
 */
#include "WazeMenuManager_JNI.h"
#include "FreeMapJNI.h"
#include "roadmap.h"
// The JNI object representing the current class
static android_jni_obj_type gJniObj;


#define JNI_CALL_WazeMenuManager_AddOptionsMenuItem 		"AddOptionsMenuItem"
#define JNI_CALL_WazeMenuManager_AddOptionsMenuItem_Sig 	"(I[B[BIIII)V"

#define JNI_CALL_WazeMenuManager_ResetOptionsMenu      "ResetOptionsMenu"
#define JNI_CALL_WazeMenuManager_ResetOptionsMenu_Sig  "()V"

#define JNI_CALL_WazeMenuManager_SubmitOptionsMenu      "SubmitOptionsMenu"
#define JNI_CALL_WazeMenuManager_SubmitOptionsMenu_Sig  "()V"

/*************************************************************************************************
 * Java_com_waze_WazeMenuManager_InitTimerManagerNTV()
 * Initializes the JNI environment object for the WazeMenuManager class
 *
 */
JNIEXPORT void JNICALL Java_com_waze_WazeMenuManager_InitMenuManagerNTV
  ( JNIEnv * aJNIEnv, jobject aJObj )
{
    JNI_LOG( ROADMAP_INFO, "Initializing JNI object WazeMenuManager" );
	// JNI object initialization
	InitJNIObject( &gJniObj, aJNIEnv, aJObj, "WazeMenuManager" );
}

/*************************************************************************************************
 * WazeMenuManager_AddOptionsMenuItem()
 * Schedules the new timer
 * aItemId - the item id in the menu index
 * aLabel - the item label
 * aIcon - the icon name
 * aIsNative - is native icon
 * aPortraitOrder - order in the portrait mode
 * aLandscapeOrder - order in the landscape mode
 * aItemType -Type bitmask (See roadmap_androidmenu.c for description)
 */
void WazeMenuManager_AddOptionsMenuItem( int aItemId, const char* aLabel, const char* aIcon,
		int aIsNative, int aPortraitOrder, int aLandscapeOrder, int aItemType )
{
	JNIEnv *env;
	jmethodID mid;
	android_method_context_type lMthdContext;
	jbyteArray label;
	jbyteArray icon;

	JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_WazeMenuManager_AddOptionsMenuItem );
	mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_WazeMenuManager_AddOptionsMenuItem,
																JNI_CALL_WazeMenuManager_AddOptionsMenuItem_Sig );
	if ( !mid || !lMthdContext.env )
	{
		roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
		return;
	}

	label = (*lMthdContext.env)->NewByteArray( lMthdContext.env, strlen( aLabel ) );
	(*lMthdContext.env)->SetByteArrayRegion( lMthdContext.env, label, 0, strlen( aLabel ), aLabel );

	icon = (*lMthdContext.env)->NewByteArray( lMthdContext.env, strlen( aIcon ) );
	(*lMthdContext.env)->SetByteArrayRegion( lMthdContext.env, icon, 0, strlen( aIcon ), aIcon );


	// Call the method
	(*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid,
			aItemId, label, icon, aIsNative, aPortraitOrder, aLandscapeOrder, aItemType );
}

/*************************************************************************************************
 * WazeMenuManager_ResetOptionsMenu()
 * Resets the options menu to be rebuild
 */
void WazeMenuManager_ResetOptionsMenu( void )
{
   JNIEnv *env;
   jmethodID mid;
   android_method_context_type lMthdContext;

   JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_WazeMenuManager_ResetOptionsMenu );
   mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_WazeMenuManager_ResetOptionsMenu,
         JNI_CALL_WazeMenuManager_ResetOptionsMenu_Sig );
   if ( !mid || !lMthdContext.env )
   {
      roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
      return;
   }

   // Call the method
   (*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid );
}
/*************************************************************************************************
 * WazeMenuManager_SubmitOptionsMenu()
 * Submits the options menu to be ready
 */
void WazeMenuManager_SubmitOptionsMenu( void )
{
   JNIEnv *env;
   jmethodID mid;
   android_method_context_type lMthdContext;

   JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_WazeMenuManager_SubmitOptionsMenu );
   mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_WazeMenuManager_SubmitOptionsMenu,
         JNI_CALL_WazeMenuManager_SubmitOptionsMenu_Sig );
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
 * Dispose the JNI object of the WazeMenuManager module
 *
 */
void WazeMenuManager_DisposeRefs()
{
    JNI_LOG( ROADMAP_INFO, "Disposing the references for the JNI object %s", gJniObj.name );
	DisposeJNIObject( &gJniObj );
}
