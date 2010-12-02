/* FreeMapJNI.c - JNI management module
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
 *   See FreeMapJNI.h
 */

#include "FreeMapJNI.h"
#include <stdio.h>
#include "roadmap.h"


/*************************************************************************************************
 * InitJNI
 * Initializes the android JNI object
 *
 */
void InitJNIObject( android_jni_obj_type* aObj, JNIEnv * aJNIEnv, jobject aJObj, const char* aObjName )
{
	// TODO :: Add log here
	jclass local_cls;
	aObj->obj = (*aJNIEnv)->NewGlobalRef( aJNIEnv, aJObj );
	local_cls = (*aJNIEnv)->GetObjectClass( aJNIEnv, aJObj );
	// Current class
	aObj->obj_class = (*aJNIEnv)->NewGlobalRef( aJNIEnv, local_cls );
	// JNI version
	aObj->ver = (*aJNIEnv)->GetVersion( aJNIEnv );
	// Java VM
	(*aJNIEnv)->GetJavaVM( aJNIEnv, &aObj->jvm );
	// Object name
	strncpy( aObj->name, aObjName, JNI_OBJ_NAME_LEN-1 );
}

/*************************************************************************************************
 * InitJNIMethodContext
 * Initializes the method context
 * Returns the method id if successful. 0 if failure
 */
jmethodID InitJNIMethodContext( android_jni_obj_type* aObj, android_method_context_type* aMthdContext,
													const char* aMethodName, const char* aMethodSig )
{
	JNIEnv *env;
	int res = 0;
	jmethodID mid = 0;

	if ( !aObj->jvm )
	{
		roadmap_log( ROADMAP_ERROR, "Cannot find VM handle for JNI object %s!", aObj->name );
		// TODO :: Add log here
		return mid;
	}
	// Getting the environment
	res = (*aObj->jvm)->GetEnv( aObj->jvm, (void**)&env, aObj->ver );

	if ( res || !env )
	{
		roadmap_log( ROADMAP_ERROR, "Cannot obtain the Java environment for JNI object %s! %d %x", aObj->name, res, env );
		return mid;
	}

	mid = (*env)->GetMethodID( env, aObj->obj_class, aMethodName, aMethodSig );

	if ( !mid )
	{
		roadmap_log( ROADMAP_ERROR, "Cannot obtain the java function pointer for JNI object %s!", aObj->name );
		return mid;
	}
	// Init the context
	aMthdContext->env = env;
	aMthdContext->mid = mid;
	return mid;
}

/*************************************************************************************************
 * DisposeJNIObject
 * Deallocates the jni object
 *
 */
void DisposeJNIObject( android_jni_obj_type* aObj )
{
	JNIEnv *env;
	int res = 0;
	jmethodID mid = 0;

	if ( !aObj->jvm )
	{
		roadmap_log( ROADMAP_ERROR, "Cannot find VM handle for JNI object %s!", aObj->name );
		// TODO :: Add log here
		return;
	}
	// Getting the environment
	res = (*aObj->jvm)->GetEnv( aObj->jvm, (void**)&env, aObj->ver );

	if ( res || !env )
	{
		roadmap_log( ROADMAP_ERROR, "Cannot obtain the Java environment for JNI object %s!", aObj->name );
		return;
	}
	// Dispose the references
	(*env)->DeleteGlobalRef( env, aObj->obj );
	(*env)->DeleteGlobalRef( env, aObj->obj_class );
}


/*************************************************************************************************
 * DisposeCustomJNIObject
 * Deallocates the custom jni object using the environment data passed in the android JNI object
 *  aObj 		- android JNI object
 *  aCustomObj 	- custom object to be deallocated
 */
void DisposeCustomJNIObject( android_jni_obj_type* aObj, jobject aCustomObj )
{
	JNIEnv *env;
	int res = 0;
	jmethodID mid = 0;

	if ( !aObj->jvm )
	{
		roadmap_log( ROADMAP_ERROR, "Cannot find VM handle for JNI object %s!", aObj->name );
		// TODO :: Add log here
		return;
	}
	// Getting the environment
	res = (*aObj->jvm)->GetEnv( aObj->jvm, (void**)&env, aObj->ver );

	if ( res || !env )
	{
		roadmap_log( ROADMAP_ERROR, "Cannot obtain the Java environment for JNI object %s!", aObj->name );
		return;
	}
	// Dispose the references
	(*env)->DeleteGlobalRef( env, aCustomObj );
}


/*************************************************************************************************
 * CloseJNIObjects
 * Disposes all the JNI objects in the system
 *
 */
void CloseJNIObjects()
{
	FreeMapNativeManager_DisposeRefs();
	FreeMapNativeCanvas_DisposeRefs();
	FreeMapNativeSoundManager_DisposeRefs();
	FreeMapNativeTimerManager_DisposeRefs();
	WazeMenuManager_DisposeRefs();
	WazeMsgBox_DisposeRefs();
}

