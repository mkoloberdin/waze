/* FreeMapNativeManager_JNI.c - JNI STUB for the FreeMapNativeManager class
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
 *   See FreeMapNativeManager_JNI.h
 */

#include "FreeMapNativeManager_JNI.h"
#include "FreeMapJNI.h"
#include "roadmap_start.h"
#include "roadmap.h"
#include "ssd/ssd_dialog.h"
#include "roadmap_editbox.h"
#include "roadmap_androidmain.h"
#include "roadmap_androidogl.h"
#include "roadmap_urlscheme.h"
#include <string.h>
#include "roadmap_browser.h"
#include "WazeSpeechttManagerBase_JNI.h"
#include "roadmap_keyboard.h"
#include "roadmap_device_events.h"

// The JNI object representing the current class
static android_jni_obj_type gJniObj;

#define JNI_CALL_FreeMapNativeManager_ShowEditBox      "ShowEditBox"
#define JNI_CALL_FreeMapNativeManager_ShowEditBox_Sig     "(II[BJII)V"

#define JNI_CALL_FreeMapNativeManager_HideEditBox      "HideEditBox"
#define JNI_CALL_FreeMapNativeManager_HideEditBox_Sig     "()V"

#define JNI_CALL_FreeMapNativeManager_ShowSoftKeyboard  		"ShowSoftKeyboard"
#define JNI_CALL_FreeMapNativeManager_ShowSoftKeyboard_Sig 		"(II)V"

#define JNI_CALL_FreeMapNativeManager_HideSoftKeyboard  		"HideSoftKeyboard"
#define JNI_CALL_FreeMapNativeManager_HideSoftKeyboard_Sig 		"()V"

#define JNI_CALL_FreeMapNativeManager_MarketRate      "MarketRate"
#define JNI_CALL_FreeMapNativeManager_MarketRate_Sig     "()V"

#define JNI_CALL_FreeMapNativeManager_ShowContacts        "ShowContacts"
#define JNI_CALL_FreeMapNativeManager_ShowContacts_Sig       "()V"

#define JNI_CALL_FreeMapNativeManager_ShutDown 			"ShutDown"
#define JNI_CALL_FreeMapNativeManager_ShutDown_Sig 		"()V"

#define JNI_CALL_FreeMapNativeManager_Flush 			"Flush"
#define JNI_CALL_FreeMapNativeManager_Flush_Sig 		"()V"

#define JNI_CALL_FreeMapNativeManager_getBatteryLevel 			"getBatteryLevel"
#define JNI_CALL_FreeMapNativeManager_getBatteryLevel_Sig 		"()I"

#define JNI_CALL_FreeMapNativeManager_SetBackLightOn 			"SetBackLightOn"
#define JNI_CALL_FreeMapNativeManager_SetBackLightOn_Sig 		"(I)V"

#define JNI_CALL_FreeMapNativeManager_SetIsMenuEnabled 			"SetIsMenuEnabled"
#define JNI_CALL_FreeMapNativeManager_SetIsMenuEnabled_Sig 		"(I)V"

#define JNI_CALL_FreeMapNativeManager_SetVolume 			"SetVolume"
#define JNI_CALL_FreeMapNativeManager_SetVolume_Sig 		"(III)V"


#define JNI_CALL_FreeMapNativeManager_PostNativeMessage 			"PostNativeMessage"
#define JNI_CALL_FreeMapNativeManager_PostNativeMessage_Sig 		"(I)V"

#define JNI_CALL_FreeMapNativeManager_MinimizeApplication 			"MinimizeApplication"
#define JNI_CALL_FreeMapNativeManager_MinimizeApplication_Sig 		"(I)V"

#define JNI_CALL_FreeMapNativeManager_MaximizeApplication 			"MaximizeApplication"
#define JNI_CALL_FreeMapNativeManager_MaximizeApplication_Sig 		"()V"

#define JNI_CALL_FreeMapNativeManager_ShowDilerWindow               "ShowDilerWindow"
#define JNI_CALL_FreeMapNativeManager_ShowDilerWindow_Sig           "()V"

#define JNI_CALL_FreeMapNativeManager_TakePicture               "TakePicture"
#define JNI_CALL_FreeMapNativeManager_TakePicture_Sig           "(III[B[B)I"

#define JNI_CALL_FreeMapNativeManager_TakePictureAsync               "TakePictureAsync"
#define JNI_CALL_FreeMapNativeManager_TakePictureAsync_Sig           "(III[B[B)I"


#define JNI_CALL_FreeMapNativeManager_GetThumbnail               "GetThumbnail"
#define JNI_CALL_FreeMapNativeManager_GetThumbnail_Sig           "(II[I)I"

#define JNI_CALL_FreeMapNativeManager_ShowWebView               "ShowWebView"
#define JNI_CALL_FreeMapNativeManager_ShowWebView_Sig           "([BIIIII)V"

#define JNI_CALL_FreeMapNativeManager_ResizeWebView               "ResizeWebView"
#define JNI_CALL_FreeMapNativeManager_ResizeWebView_Sig           "(IIII)V"

#define JNI_CALL_FreeMapNativeManager_HideWebView               "HideWebView"
#define JNI_CALL_FreeMapNativeManager_HideWebView_Sig           "()V"

#define JNI_CALL_FreeMapNativeManager_LoadUrl               "LoadUrl"
#define JNI_CALL_FreeMapNativeManager_LoadUrl_Sig           "([B)V"

#define JNI_CALL_FreeMapNativeManager_EditBoxCheckTypingLockCb               "EditBoxCheckTypingLockCb"
#define JNI_CALL_FreeMapNativeManager_EditBoxCheckTypingLockCb_Sig           "(I)V"

#define JNI_CALL_FreeMapNativeManager_OpenExternalBrowser               "OpenExternalBrowser"
#define JNI_CALL_FreeMapNativeManager_OpenExternalBrowser_Sig           "(Ljava/lang/String;)V"



extern void roadmap_path_initialize( const char* aCfgPath );
extern void roadmap_device_backlight_monitor_reset( void );
extern void roadmap_main_set_first_run( BOOL value );
extern void roadmap_camera_take_picture_async_cb( int res );
/*************************************************************************************************
 * Java_com_waze_FreeMapNativeManager_InitManagerNTV
 * Initializes the JNI object for the FreeMapNativeManager class
 * Initializes the application resources path as provided from the Java layer
 */
JNIEXPORT void JNICALL Java_com_waze_FreeMapNativeManager_InitNativeManagerNTV
  ( JNIEnv* aJNIEnv, jobject aJObj, jstring aCfgPath, jint aBuildSdkVersion,
        jstring aDeviceName, jstring aDeviceModel, jstring aDeviceManufacturer )
{

	jboolean isCopy;
	const char* cfgPath = (*aJNIEnv)->GetStringUTFChars( aJNIEnv, aCfgPath, &isCopy );
	const char* deviceName = (*aJNIEnv)->GetStringUTFChars( aJNIEnv, aDeviceName, &isCopy );
	const char* deviceModel = (*aJNIEnv)->GetStringUTFChars( aJNIEnv, aDeviceModel, &isCopy );
	const char* deviceManufacturer = (*aJNIEnv)->GetStringUTFChars( aJNIEnv, aDeviceManufacturer, &isCopy );

	roadmap_path_initialize( cfgPath );

	(*aJNIEnv)->ReleaseStringUTFChars( aJNIEnv, aCfgPath, cfgPath );

	// The logger can be started only after the path initialization
	JNI_LOG( ROADMAP_INFO, "Initializing JNI object FreeMapNativeManager" );
	InitJNIObject( &gJniObj, aJNIEnv, aJObj, "FreeMapNativeManager" );

	roadmap_main_set_build_sdk_version( aBuildSdkVersion );

	roadmap_main_set_device_name( deviceName );
	roadmap_main_set_device_model( deviceModel );
	roadmap_main_set_device_manufacturer( deviceManufacturer );

   roadmap_log( ROADMAP_WARNING, "JNI LIB DEBUG: Version: %s, OS Version: %d. Device: %s. Model: %s. Manufacturer: %s. roadmap_start address: 0x%x.\n",
			roadmap_start_version(),
			aBuildSdkVersion,
			roadmap_main_get_device_name(),
			deviceModel,
			deviceManufacturer,
			roadmap_start );

   (*aJNIEnv)->ReleaseStringUTFChars( aJNIEnv, aDeviceName, deviceName );
   (*aJNIEnv)->ReleaseStringUTFChars( aJNIEnv, aDeviceModel, deviceModel );
   (*aJNIEnv)->ReleaseStringUTFChars( aJNIEnv, aDeviceManufacturer, deviceManufacturer );
}


/*************************************************************************************************
 * Java_com_waze_FreeMapNativeManager_AppStartNTV
 * Starts the application
 *
 */
JNIEXPORT void JNICALL Java_com_waze_FreeMapNativeManager_AppStartNTV
  ( JNIEnv* aJNIEnv, jobject aJObj, jstring aUrl, int aAppMode )
{
	roadmap_main_start_init();

	const char* url = NULL;
	if ( aUrl )
	{
	   char query[URL_MAX_LENGTH];
	   jboolean isCopy;
		url = (*aJNIEnv)->GetStringUTFChars( aJNIEnv, aUrl, &isCopy );
		roadmap_urlscheme_remove_prefix( query, url );
      roadmap_urlscheme_init( query );
      (*aJNIEnv)->ReleaseStringUTFChars( aJNIEnv, aUrl, url );
	}

  roadmap_log( ROADMAP_WARNING, "Applicaiton started with URL string: %s", url );

  roadmap_main_set_app_mode( aAppMode );

  // Widget Mode
  if ( aAppMode == 1 )
  {
     roadmap_screen_set_background_run( TRUE );
  }

  roadmap_start(0, NULL);
}

/*************************************************************************************************
 * Java_com_waze_FreeMapNativeManager_AppShutDownNTV
 * Starts the Shutdown process on the native side
 *
 */
JNIEXPORT void JNICALL Java_com_waze_FreeMapNativeManager_AppShutDownNTV
( JNIEnv* aJNIEnv, jobject aJObj )
{
	roadmap_main_shutdown();
}

/*************************************************************************************************
 * FreeMapNativeManager_ShutDown()
 * Calls the ExitManager function on the Java layer
 *
 */
void FreeMapNativeManager_ShutDown()
{
	android_method_context_type lMthdContext;
	jmethodID mid;
	JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_FreeMapNativeManager_ShutDown );
	mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_FreeMapNativeManager_ShutDown,
													JNI_CALL_FreeMapNativeManager_ShutDown_Sig );
	if ( !mid || !lMthdContext.env )
	{
		roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
		return;
	}

	// Calling the method
	(*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid );
}

/*************************************************************************************************
 * FreeMapNativeManager_ShowContacts()
 * Shows the contacts list
 *
 */
void FreeMapNativeManager_ShowContacts()
{
   android_method_context_type lMthdContext;
   jmethodID mid;
   JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_FreeMapNativeManager_ShowContacts );
   mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_FreeMapNativeManager_ShowContacts,
                                       JNI_CALL_FreeMapNativeManager_ShowContacts_Sig );
   if ( !mid || !lMthdContext.env )
   {
      roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
      return;
   }

   // Calling the method
   (*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid );
}


/*************************************************************************************************
 * Java_com_waze_FreeMapNativeManager_UrlHandlerNTV
 * JNI wrapper for
 *
 */
JNIEXPORT jboolean JNICALL Java_com_waze_FreeMapNativeManager_UrlHandlerNTV
( JNIEnv* aJNIEnv, jobject aJObj, jstring aUrl )
{
   jboolean isCopy;
   jboolean res = JNI_FALSE;
   const char* url = (*aJNIEnv)->GetStringUTFChars( aJNIEnv, aUrl, &isCopy );

   /*
    * Try to handle by browser
    */
   roadmap_log( ROADMAP_INFO, "Processing url: %s", url );
   if ( roadmap_browser_url_handler( url ) == TRUE )
   {
      res = JNI_TRUE;
   }

   /*
     * Try to handle by url scheme
     */
   if ( ( res == JNI_FALSE ) && roadmap_urlscheme_valid( url ) )
   {
      char query[URL_MAX_LENGTH];
      roadmap_urlscheme_remove_prefix( query, url );
      roadmap_urlscheme_init( query );
      res = JNI_TRUE;
   }

   (*aJNIEnv)->ReleaseStringUTFChars( aJNIEnv, aUrl, url );

   return res;
}



/*************************************************************************************************
 * FreeMapNativeManager_PostNativeMessage( int aMsg )
 * Calls the PostNativeMessage on the Java side to post the message to the main native thread
 *
 */
void FreeMapNativeManager_PostNativeMessage( int aMsg )
{
	android_method_context_type lMthdContext;
	jmethodID mid;
	JNIEnv* env;
	JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_FreeMapNativeManager_PostNativeMessage );
	// !!! This method can be called from the another thread!!! It has to be attached to the VM
	(*gJniObj.jvm)->AttachCurrentThread( gJniObj.jvm, (void**) &env, NULL );

	mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_FreeMapNativeManager_PostNativeMessage,
														JNI_CALL_FreeMapNativeManager_PostNativeMessage_Sig );
	if ( !mid || !lMthdContext.env )
	{
		roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
		return;
	}

	// Calling the method
	(*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid, aMsg );
	// Detach the thread
	(*gJniObj.jvm)->DetachCurrentThread( gJniObj.jvm );
}


/*************************************************************************************************
 * FreeMapNativeManager_SetBackLightOn( int aAlwaysOn )
 * Sets the backlight to be always on or restores the system level
 *
 */
void FreeMapNativeManager_SetBackLightOn( int aAlwaysOn )
{
	android_method_context_type lMthdContext;
	jmethodID mid;
	JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_FreeMapNativeManager_SetBackLightOn );
	mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_FreeMapNativeManager_SetBackLightOn,
															JNI_CALL_FreeMapNativeManager_SetBackLightOn_Sig );
	if ( !mid || !lMthdContext.env )
	{
		roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
		return;
	}

	// Calling the method
	(*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid, aAlwaysOn );
}


/*************************************************************************************************
 * FreeMapNativeManager_SetIsMenuEnabled( int aAlwaysOn )
 * Enables the menu
 *
 */
void FreeMapNativeManager_SetIsMenuEnabled( int aEnabled )
{
	android_method_context_type lMthdContext;
	jmethodID mid;
	JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_FreeMapNativeManager_SetIsMenuEnabled );
	mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_FreeMapNativeManager_SetIsMenuEnabled,
															JNI_CALL_FreeMapNativeManager_SetIsMenuEnabled_Sig );
	if ( !mid || !lMthdContext.env )
	{
		roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
		return;
	}

	// Calling the method
	(*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid, aEnabled );
}


/*************************************************************************************************
 * FreeMapNativeManager_SetVolume( int aAlwaysOn )
 * Sets the new volume level for the media stream
 *
 */
void FreeMapNativeManager_SetVolume( int aLvl, int aMinLvl, int aMaxLvl )
{
	android_method_context_type lMthdContext;
	jmethodID mid;
	JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_FreeMapNativeManager_SetVolume );
	mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_FreeMapNativeManager_SetVolume,
															JNI_CALL_FreeMapNativeManager_SetVolume_Sig );
	if ( !mid || !lMthdContext.env )
	{
		roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
		return;
	}

	// Calling the method
	(*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid, aLvl, aMinLvl, aMaxLvl );
}


/*************************************************************************************************
 * void FreeMapNativeManager_Flush()
 * Causes the main loop run
 *
 */
void FreeMapNativeManager_Flush()
{
	android_method_context_type lMthdContext;
	jmethodID mid;
	JNIEnv* env;
	JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_FreeMapNativeManager_Flush );

	mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_FreeMapNativeManager_Flush,
																JNI_CALL_FreeMapNativeManager_Flush_Sig );
	if ( !mid || !lMthdContext.env )
	{
		roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
		return;
	}

	// Calling the method
	(*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid );

}

/*************************************************************************************************
 * int FreeMapNativeManager_getBatteryLevel()
 * Returns the current battery level
 *
 */
int FreeMapNativeManager_getBatteryLevel()
{
	int retVal;
	android_method_context_type lMthdContext;
	jmethodID mid;
	JNIEnv* env;
	JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_FreeMapNativeManager_getBatteryLevel );

	mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_FreeMapNativeManager_getBatteryLevel,
															JNI_CALL_FreeMapNativeManager_getBatteryLevel_Sig );
	if ( !mid || !lMthdContext.env )
	{
		roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
		return 0;
	}

	// Calling the method
	retVal = (*lMthdContext.env)->CallIntMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid );

	return retVal;
}

/*************************************************************************************************
 * FreeMapNativeManager_MinimizeApplication( int aDelay )
 * Minimizes the application and returns to the main window (if aDelay>= 0 ) after aDelay msecs
 *
 */
void FreeMapNativeManager_MinimizeApplication( int aDelay )
{
	android_method_context_type lMthdContext;
	jmethodID mid;
	JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_FreeMapNativeManager_MinimizeApplication );
	mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_FreeMapNativeManager_MinimizeApplication,
											JNI_CALL_FreeMapNativeManager_MinimizeApplication_Sig );
	if ( !mid || !lMthdContext.env )
	{
		roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
		return;
	}

	// Calling the method
	(*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid, aDelay );
}

/*************************************************************************************************
 * FreeMapNativeManager_MaximizeApplication()
 * Minimizes the application from the background
 *
 */
void FreeMapNativeManager_MaximizeApplication()
{
	android_method_context_type lMthdContext;
	jmethodID mid;
	JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_FreeMapNativeManager_MaximizeApplication );
	mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_FreeMapNativeManager_MaximizeApplication,
											JNI_CALL_FreeMapNativeManager_MaximizeApplication_Sig );
	if ( !mid || !lMthdContext.env )
	{
		roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
		return;
	}

	// Calling the method
	(*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid );
}

/*************************************************************************************************
 * FreeMapNativeManager_ShowDilerWindow()
 * Shows the dialer window
 *
 */
void FreeMapNativeManager_ShowDilerWindow()
{
    android_method_context_type lMthdContext;
    jmethodID mid;
    JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_FreeMapNativeManager_ShowDilerWindow );
    mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_FreeMapNativeManager_ShowDilerWindow,
                                            JNI_CALL_FreeMapNativeManager_ShowDilerWindow_Sig );
    if ( !mid || !lMthdContext.env )
    {
        roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
        return;
    }

    // Calling the method
    (*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid );
}

/*************************************************************************************************
 * FreeMapNativeManager_ShowSoftKeyboard()
 * Shows the native keyboard
 *
 */
void FreeMapNativeManager_ShowSoftKeyboard( int aAction, int aCloseOnAction )
{
    android_method_context_type lMthdContext;
    jmethodID mid;
    JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_FreeMapNativeManager_ShowSoftKeyboard );
    mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_FreeMapNativeManager_ShowSoftKeyboard,
                                            JNI_CALL_FreeMapNativeManager_ShowSoftKeyboard_Sig );
    if ( !mid || !lMthdContext.env )
    {
        roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
        return;
    }

    // Calling the method
    (*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid, aAction, aCloseOnAction );
}

/*************************************************************************************************
 * FreeMapNativeManager_ShowSoftKeyboard()
 * Hides the native keyboard
 *
 */
void FreeMapNativeManager_HideSoftKeyboard()
{
    android_method_context_type lMthdContext;
    jmethodID mid;
    JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_FreeMapNativeManager_HideSoftKeyboard );
    mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_FreeMapNativeManager_HideSoftKeyboard,
                                            JNI_CALL_FreeMapNativeManager_HideSoftKeyboard_Sig );
    if ( !mid || !lMthdContext.env )
    {
        roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
        return;
    }

    // Calling the method
    (*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid );
}

/*************************************************************************************************
 * FreeMapNativeManager_MarketRate()
 * Brings to front market rating window
 *
 */
void FreeMapNativeManager_MarketRate( void )
{
    android_method_context_type lMthdContext;
    jmethodID mid;
    JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_FreeMapNativeManager_MarketRate );
    mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_FreeMapNativeManager_MarketRate,
                                            JNI_CALL_FreeMapNativeManager_MarketRate_Sig );
    if ( !mid || !lMthdContext.env )
    {
        roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
        return;
    }

    // Calling the method
    (*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid );
}

/*************************************************************************************************
 * FreeMapNativeManager_ShowEditBox()
 * Shows the native edit box
 *
 */
void FreeMapNativeManager_ShowEditBox( int aAction, int aStayOnAction, const char* aText, void* aContext, int aTopMargin, int aFlags )
{
    android_method_context_type lMthdContext;
    jmethodID mid;
    jbyteArray text;
    JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_FreeMapNativeManager_ShowEditBox );
    mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_FreeMapNativeManager_ShowEditBox,
                                            JNI_CALL_FreeMapNativeManager_ShowEditBox_Sig );
    if ( !mid || !lMthdContext.env )
    {
        roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
        return;
    }

    text = (*lMthdContext.env)->NewByteArray( lMthdContext.env, strlen( aText ) );
    (*lMthdContext.env)->SetByteArrayRegion( lMthdContext.env, text, 0, strlen( aText ), aText );

    jlong ptrCtx = (jlong) aContext;

    // Calling the method
    (*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid,
          aAction, aStayOnAction, text, ptrCtx, aTopMargin, aFlags );
}

/*************************************************************************************************
 * FreeMapNativeManager_HideEditBox()
 * Hides the native edit box
 *
 */
void FreeMapNativeManager_HideEditBox()
{
    android_method_context_type lMthdContext;
    jmethodID mid;
    JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_FreeMapNativeManager_HideEditBox );
    mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_FreeMapNativeManager_HideEditBox,
                                            JNI_CALL_FreeMapNativeManager_HideEditBox_Sig );
    if ( !mid || !lMthdContext.env )
    {
        roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
        return;
    }

    // Calling the method
    (*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid );
}


/*************************************************************************************************
 * FreeMapNativeManager_TakePicture()
 * Shows the camera capture preview and saves the taken image
 *
 */
int FreeMapNativeManager_TakePicture( int aImageWidth, int aImageHeight, int aImageQuality,
        char* aImageFolder, char* aImageFile )
{
    android_method_context_type lMthdContext;
    int retVal = -1;
    jmethodID mid;
    jbyteArray imageFolder, imageFile;
      JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_FreeMapNativeManager_TakePicture );
    mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_FreeMapNativeManager_TakePicture,
                                            JNI_CALL_FreeMapNativeManager_TakePicture_Sig );
    if ( !mid || !lMthdContext.env )
    {
        roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
        return retVal;
    }

    imageFolder = (*lMthdContext.env)->NewByteArray( lMthdContext.env, strlen( aImageFolder ) );
    (*lMthdContext.env)->SetByteArrayRegion( lMthdContext.env, imageFolder, 0, strlen( aImageFolder ), aImageFolder );

    imageFile = (*lMthdContext.env)->NewByteArray( lMthdContext.env, strlen( aImageFile ) );
    (*lMthdContext.env)->SetByteArrayRegion( lMthdContext.env, imageFile, 0, strlen( aImageFile ), aImageFile );

    // Calling the method
    retVal = (*lMthdContext.env)->CallIntMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid,
            aImageWidth, aImageHeight, aImageQuality, imageFolder, imageFile );

    return retVal;
}

/*************************************************************************************************
 * Java_com_waze_FreeMapNativeManager_ConnectivityChangedNTV
 * Starts the application
 *
 */
JNIEXPORT void JNICALL Java_com_waze_FreeMapNativeManager_ConnectivityChangedNTV
  ( JNIEnv* aJNIEnv, jobject aJObj, jboolean aConnected, jint aConnType, jstring aConnTypeStr )
{
   const char* conn_type_str = (*aJNIEnv)->GetStringUTFChars( aJNIEnv, aConnTypeStr, NULL );
   static int conn_state = -1;

   roadmap_log( ROADMAP_WARNING, "Android Connectivity Event. Connected: %d (Previous: %d). Network Type: %s ( Code: %d )",
         aConnected == JNI_TRUE, conn_state, conn_type_str, aConnType );

   if ( conn_state != (int) aConnected )
   {
      if ( aConnected == JNI_TRUE )
         roadmap_device_event_notification( device_event_network_connected );
      else
         roadmap_device_event_notification( device_event_network_disconnected );

      conn_state = ( int ) aConnected;
   }

   (*aJNIEnv)->ReleaseStringUTFChars( aJNIEnv, aConnTypeStr, conn_type_str );
}

/*************************************************************************************************
 * FreeMapNativeManager_TakePictureAsync()
 * Shows the camera capture preview and saves the taken image
 *
 */
int FreeMapNativeManager_TakePictureAsync( int aImageWidth, int aImageHeight, int aImageQuality,
        char* aImageFolder, char* aImageFile )
{
    android_method_context_type lMthdContext;
    int retVal = -1;
    jmethodID mid;
    jbyteArray imageFolder, imageFile;
      JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_FreeMapNativeManager_TakePictureAsync );
    mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_FreeMapNativeManager_TakePictureAsync,
                                            JNI_CALL_FreeMapNativeManager_TakePictureAsync_Sig );
    if ( !mid || !lMthdContext.env )
    {
        roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
        return retVal;
    }

    imageFolder = (*lMthdContext.env)->NewByteArray( lMthdContext.env, strlen( aImageFolder ) );
    (*lMthdContext.env)->SetByteArrayRegion( lMthdContext.env, imageFolder, 0, strlen( aImageFolder ), aImageFolder );

    imageFile = (*lMthdContext.env)->NewByteArray( lMthdContext.env, strlen( aImageFile ) );
    (*lMthdContext.env)->SetByteArrayRegion( lMthdContext.env, imageFile, 0, strlen( aImageFile ), aImageFile );

    // Calling the method
    retVal = (*lMthdContext.env)->CallIntMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid,
            aImageWidth, aImageHeight, aImageQuality, imageFolder, imageFile  );

    return retVal;
}

/*************************************************************************************************
 * Java_com_waze_FreeMapNativeManager_TakePictureCallbackNTV
 * Callback for the asynchronous image capturing
 *
 */
JNIEXPORT void JNICALL Java_com_waze_FreeMapNativeManager_TakePictureCallbackNTV
( JNIEnv* aJNIEnv, jobject aJObj, jint aRes )
{
   roadmap_camera_take_picture_async_cb( aRes );
}

/*************************************************************************************************
 * FreeMapNativeManager_GetThumbnail
 * Returns the thumbnail in the supplied byte buffer with the last taken image
 *
 */
int FreeMapNativeManager_GetThumbnail( int aThumbWidth, int aThumbHeight, int bytePP, int* aBuf )
{
    android_method_context_type lMthdContext;
    int retVal = -1;
    int buf_size;
    jmethodID mid;
    jintArray bufJNI;

    JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_FreeMapNativeManager_GetThumbnail );

    mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_FreeMapNativeManager_GetThumbnail,
                                            JNI_CALL_FreeMapNativeManager_GetThumbnail_Sig );
    if ( !mid || !lMthdContext.env )
    {
        roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
        return retVal;
    }

    buf_size = aThumbHeight*aThumbWidth;
    bufJNI = (*lMthdContext.env)->NewIntArray( lMthdContext.env, buf_size );

    // Calling the method
    retVal = (*lMthdContext.env)->CallIntMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid,
            aThumbWidth, aThumbHeight, bufJNI );

    // Copy the elements
    (*lMthdContext.env)->GetIntArrayRegion( lMthdContext.env, bufJNI, 0, buf_size, aBuf );

    return retVal;
}


/*************************************************************************************************
 * Java_com_waze_FreeMapNativeManager_NativeMsgDispatcherNTV
 * JNI wrapper for the roadmap_main_message_dispatcher (will be called from the Java layer)
 *
 */
JNIEXPORT void JNICALL Java_com_waze_FreeMapNativeManager_NativeMsgDispatcherNTV
( JNIEnv* aJNIEnv, jobject aJObj, jint aMsgId )
{
	roadmap_main_message_dispatcher( aMsgId );
}

/*************************************************************************************************
 * Java_com_waze_FreeMapNativeManager_ShowGpsDisabledWarning
 * JNI wrapper for the roadmap_main_show_gps_disabled_warning (will be called from the Java layer)
 *
 */
JNIEXPORT void JNICALL Java_com_waze_FreeMapNativeManager_ShowGpsDisabledWarningNTV
( JNIEnv* aJNIEnv, jobject aJObj )
{
	roadmap_main_show_gps_disabled_warning();
}
/*************************************************************************************************
 * Java_com_waze_FreeMapNativeManager_BackLightMonitorResetNTV
 * JNI wrapper for the roadmap_device_backlight_monitor_reset (will be called from the Java layer)
 *
 */
JNIEXPORT void JNICALL Java_com_waze_FreeMapNativeManager_BackLightMonitorResetNTV
( JNIEnv* aJNIEnv, jobject aJObj )
{
	roadmap_device_backlight_monitor_reset();
}

/*************************************************************************************************
 * Java_com_waze_FreeMapNativeManager_SetUpgradeRunNTV
 * JNI wrapper for the roadmap_main_set_first_run (will be called from the Java layer)
 *
 */
JNIEXPORT void JNICALL Java_com_waze_FreeMapNativeManager_SetUpgradeRunNTV
( JNIEnv* aJNIEnv, jobject aJObj, jbyte aValue )
{
	roadmap_main_set_first_run( aValue );
}


/*************************************************************************************************
 * FreeMapNativeManager_HideWebView
 * Hides android web view inside the dialog
 *
 */
void FreeMapNativeManager_HideWebView( void )
{
   android_method_context_type lMthdContext;
   int retVal = -1;
   jmethodID mid;

   JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_FreeMapNativeManager_HideWebView );
   mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_FreeMapNativeManager_HideWebView,
                                                         JNI_CALL_FreeMapNativeManager_HideWebView_Sig );
   if ( !mid || !lMthdContext.env )
   {
       roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
       return;
   }

      // Calling the method
   (*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid );
}
/*************************************************************************************************
 * FreeMapNativeManager_ShowWebView
 * Shows android web view inside the dialog
 *
 */
void FreeMapNativeManager_ShowWebView( const char* aUrl, int aMinX, int aMinY, int aMaxX, int aMaxY, int aFlags )
{
    android_method_context_type lMthdContext;
    int retVal = -1;
    jmethodID mid;
    jbyteArray url;
      JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_FreeMapNativeManager_ShowWebView );
    mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_FreeMapNativeManager_ShowWebView,
                                                                            JNI_CALL_FreeMapNativeManager_ShowWebView_Sig );
    if ( !mid || !lMthdContext.env )
    {
        roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
        return;
    }

    url = (*lMthdContext.env)->NewByteArray( lMthdContext.env, strlen( aUrl ) );
    (*lMthdContext.env)->SetByteArrayRegion( lMthdContext.env, url, 0, strlen( aUrl ), aUrl );

    // Calling the method
    (*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid,
            url, aMinX, aMinY, aMaxX, aMaxY, aFlags );
}
/*************************************************************************************************
 * FreeMapNativeManager_ResizeWebView
 * Resize android web view to fit the dialog
 *
 */
void FreeMapNativeManager_ResizeWebView( int aMinX, int aMinY, int aMaxX, int aMaxY )
{
    android_method_context_type lMthdContext;
    int retVal = -1;
    jmethodID mid;
    JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_FreeMapNativeManager_ResizeWebView );
    mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_FreeMapNativeManager_ResizeWebView,
                                                                            JNI_CALL_FreeMapNativeManager_ResizeWebView_Sig );
    if ( !mid || !lMthdContext.env )
    {
        roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
        return;
    }

    // Calling the method
    (*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid,
            aMinX, aMinY, aMaxX, aMaxY );
}

/*************************************************************************************************
 * FreeMapNativeManager_LoadUrl
 * Loads the url to the android web view
 *
 */
void FreeMapNativeManager_LoadUrl( const char* aUrl )
{
    android_method_context_type lMthdContext;
    int retVal = -1;
    jmethodID mid;
    jbyteArray url;
      JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_FreeMapNativeManager_LoadUrl );
    mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_FreeMapNativeManager_LoadUrl,
                                                                            JNI_CALL_FreeMapNativeManager_LoadUrl_Sig );
    if ( !mid || !lMthdContext.env )
    {
        roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
        return;
    }

    url = (*lMthdContext.env)->NewByteArray( lMthdContext.env, strlen( aUrl ) );
    (*lMthdContext.env)->SetByteArrayRegion( lMthdContext.env, url, 0, strlen( aUrl ), aUrl );

    // Calling the method
    (*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid,
            url );
}

/*************************************************************************************************
 * FreeMapNativeManager_OpenExternalBrowser
 * Loads the url to the external browser
 *
 */
void FreeMapNativeManager_OpenExternalBrowser( const char* aUrl )
{
    android_method_context_type lMthdContext;
    int retVal = -1;
    jmethodID mid;
    jstring url = NULL;

    JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_FreeMapNativeManager_OpenExternalBrowser );
    mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_FreeMapNativeManager_OpenExternalBrowser,
                                                                            JNI_CALL_FreeMapNativeManager_OpenExternalBrowser_Sig );
    if ( !mid || !lMthdContext.env )
    {
        roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
        return;
    }

    if ( aUrl != NULL )
    {
       url = (*lMthdContext.env)->NewStringUTF( lMthdContext.env, aUrl );
    }

    // Calling the method
    (*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid, url );

    // Release local references
    if ( url )
       (*lMthdContext.env)->DeleteLocalRef( lMthdContext.env, url );
}



/*************************************************************************************************
 * FreeMapNativeManager_EditBoxCheckTypingLockCb
 * Callback for the check typing lock message box
 *
 */
void FreeMapNativeManager_EditBoxCheckTypingLockCb( int aRes )
{
    android_method_context_type lMthdContext;
    jmethodID mid;

    JNI_LOG( ROADMAP_INFO, "Trying to call method %s through JNI", JNI_CALL_FreeMapNativeManager_EditBoxCheckTypingLockCb );
    mid = InitJNIMethodContext( &gJniObj, &lMthdContext, JNI_CALL_FreeMapNativeManager_EditBoxCheckTypingLockCb,
                                                                   JNI_CALL_FreeMapNativeManager_EditBoxCheckTypingLockCb_Sig );
    if ( !mid || !lMthdContext.env )
    {
        roadmap_log( ROADMAP_ERROR, "Failed to obtain method context!" );
        return;
    }


    // Calling the method
    (*lMthdContext.env)->CallVoidMethod( lMthdContext.env, gJniObj.obj, lMthdContext.mid, aRes );
}

/*************************************************************************************************
 * Java_com_waze_FreeMapNativeManager_SetBackgroundRunNTV
 * JNI wrapper for the roadmap_main_set_first_run (will be called from the Java layer)
 *
 */
JNIEXPORT void JNICALL Java_com_waze_FreeMapNativeManager_SetBackgroundRunNTV
( JNIEnv* aJNIEnv, jobject aJObj, jint aValue )
{
	if ( aValue )
	{
		roadmap_start_app_run_bg();
	}
	else
	{
		roadmap_start_app_run_fg();
	}
}

/*************************************************************************************************
 * Java_com_waze_FreeMapNativeManager_EditBoxCallbackNTV
 * Edit box callback
 *
 */
JNIEXPORT void JNICALL Java_com_waze_FreeMapNativeManager_EditBoxCallbackNTV
 ( JNIEnv* aJNIEnv, jobject aJObj, jint aRes, jstring aText, jlong aCbContext )
 {
    EditBoxContextType *ctx = ( EditBoxContextType* ) aCbContext;
    if ( aRes > 0 )
    {
       jboolean isCopy;
       const char* text = (*aJNIEnv)->GetStringUTFChars( aJNIEnv, aText, &isCopy );

       ctx->callback( dec_ok, text, ctx->cb_context );

       (*aJNIEnv)->ReleaseStringUTFChars( aJNIEnv, aText, text );
    }
    else
    {
       roadmap_editbox_confirm_warning_hide();
       ctx->callback( dec_cancel, "", ctx->cb_context );
    }
    roadmap_editbox_dlg_hide();

    free( ctx );
}

/*************************************************************************************************
 * Java_com_waze_FreeMapNativeManager_EditBoxCheckTypingLockNTV
 * Checks if the editbox typing lock is enabled
 *
 */
 JNIEXPORT jboolean JNICALL Java_com_waze_FreeMapNativeManager_EditBoxCheckTypingLockNTV
   ( JNIEnv *aJNIEnv, jobject aJObj )
 {
    jboolean res = JNI_FALSE;
    if ( roadmap_keyboard_typing_locked( TRUE ) )
    {
       res = JNI_TRUE;
    }    
    return res;
 }
/*************************************************************************************************
 * FreeMapNativeManager_DisposeRefs()
 * Dispose the JNI object of the FreeMapNativeManager module
 *
 */
void FreeMapNativeManager_DisposeRefs()
{
    JNI_LOG( ROADMAP_INFO, "Disposing the references for the JNI object %s", gJniObj.name );
	DisposeJNIObject( &gJniObj );
}



