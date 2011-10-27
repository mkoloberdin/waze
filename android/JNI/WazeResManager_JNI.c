/* WazeResManager_JNI.c - JNI STUB for the WazeResManager class
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
 *   See WazeResManager_JNI.h
 */
#include <string.h>
#include "WazeResManager_JNI.h"
#include "FreeMapJNI.h"
#include "roadmap.h"

// The JNI object representing the current class
static android_jni_obj_type gJniObj;

#define JNI_CALL_WazeResManager_LoadSkin               "LoadSkin"
#define JNI_CALL_WazeResManager_LoadSkin_Sig           "(Ljava/lang/String;)Lcom/waze/WazeResManager$ResData;"

#define JNI_CALL_WazeResManager_LoadResList               "LoadResList"
#define JNI_CALL_WazeResManager_LoadResList_Sig           "(Ljava/lang/String;)[Ljava/lang/String;"


/*************************************************************************************************
 * Java_com_waze_WazeResManager_InitMsgBoxNTV()
 * Initializes the JNI environment object for the WazeResManager class
 *
 */
JNIEXPORT void JNICALL Java_com_waze_WazeResManager_InitResManagerNTV
( JNIEnv* aJNIEnv, jobject aJObj )
{
   JNI_LOG( ROADMAP_INFO, "Initializing JNI object WazeResManager" );
   // JNI object initialization
   InitJNIObject( &gJniObj, aJNIEnv, aJObj, "WazeResManager" );
}



/*************************************************************************************************
 * WazeResManager_LoadSkin
 * Loads the image bytes from the Java code
 *
 */
char* WazeResManager_LoadSkin( const char* aName, int* size )
{
    android_method_context_type lMthdContext;
    int retVal = -1;
    jmethodID mid;
    jobject imageData;
    jclass class;
    jbyteArray byteArray;
    jstring name = NULL;
    jobject byteArrayMem;
    char* bufPtr;
    unsigned char *buf = NULL;
    JNIEnv* env;

    JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_WazeResManager_LoadSkin );
    mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_WazeResManager_LoadSkin,
                                                          JNI_CALL_WazeResManager_LoadSkin_Sig );
    if ( !mid || !lMthdContext.env )
    {
        roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
        return NULL;
    }

    env = lMthdContext.env;

    name = (*lMthdContext.env)->NewStringUTF( lMthdContext.env, aName );

    // Calling the method
    imageData = ( jobject ) (*env)->CallObjectMethod( env, gJniObj.obj, lMthdContext.mid, name );

    if ( imageData != NULL )
    {
       //Get the class of object
       class = (*env)->GetObjectClass( env, imageData );
       //Obtaining the Fields data from the returned object
       byteArray = (jbyteArray) (*env)->GetObjectField( env, imageData, (*env)->GetFieldID( env, class, "buf", "[B" ) );
       if ( byteArray != NULL )
       {
          //*size = (*env)->GetIntField( env, imageData, (*env)->GetFieldID( env, class, "size", "I" ) );
          *size = (*env)->GetArrayLength( env, byteArray );
   //       size = (*env)->GetDirectBufferCapacity( env, byteArrayMem );

          buf = malloc( *size );
          (*env)->GetByteArrayRegion( env, byteArray, 0, *size, buf );
       }
       else
       {
          roadmap_log( ROADMAP_ERROR, "Error obtaining image buffer from the object for: %s", aName );
       }
    }
    else
    {
       // DEBUG because some resources will be downloaded later
       roadmap_log( ROADMAP_DEBUG, "Error obtaining image data object for: %s", aName );
    }

    // Release local references
    if ( name )
       (*env)->DeleteLocalRef( env, name );

    return buf;
}

/*************************************************************************************************
 * WazeResManager_LoadResList
 * Loads the resources list
 *
 */
char** WazeResManager_LoadResList( const char* aPath )
{
    android_method_context_type lMthdContext;
    int retVal = -1;
    jmethodID mid;
    jobjectArray resArray;
    jstring path = NULL;
    JNIEnv* env;
    char **listOut = NULL;
    JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_WazeResManager_LoadResList );
    mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_WazeResManager_LoadResList,
                                                          JNI_CALL_WazeResManager_LoadResList_Sig );
    if ( !mid || !lMthdContext.env )
    {
        roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
        return NULL;
    }

    env = lMthdContext.env;

    path = (*lMthdContext.env)->NewStringUTF( lMthdContext.env, aPath );


    // Calling the method
    resArray = ( jobjectArray ) (*env)->CallObjectMethod( env, gJniObj.obj, lMthdContext.mid, path );

    if ( resArray != NULL )
    {
       int length = (*env)->GetArrayLength( env, resArray );
       int i = 0;
       listOut = calloc( length + 1, sizeof( char* ) );

       for( ;i < length; ++i )
       {
          jstring javaString = (jstring) (*env)->GetObjectArrayElement( env, resArray, i );
          const char* next = (*env)->GetStringUTFChars( env, javaString, 0 );
          listOut[i] = strdup( next );
          (*env)->ReleaseStringUTFChars( env, javaString, next );
       }
    }
    else
    {
       roadmap_log( ROADMAP_WARNING, "Error obtaining array data object" );
    }

    // Release local references
    if ( path )
       (*env)->DeleteLocalRef( env, path );

    return listOut;
}


/*************************************************************************************************
 * WazeResManager_DisposeRefs()
 * Dispose the JNI object of the WazeResManager module
 *
 */
void WazeResManager_DisposeRefs()
{
   JNI_LOG( ROADMAP_INFO, "Disposing the references for the JNI object %s", gJniObj.name );
   DisposeJNIObject( &gJniObj );
}
