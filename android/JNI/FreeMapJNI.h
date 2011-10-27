/* FreeMapJNI.h - Interface for the JNI management module
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
 * DESCRIPTION:
 *
 *
 */
#ifndef INCLUDE__FREEMAP_JNI__H
#define INCLUDE__FREEMAP_JNI__H

#include <jni.h>


// JNI debug
#define JNI_DEBUG_ENABLED       0

#define JNI_LOG( ... )   {                  \
    if ( JNI_DEBUG_ENABLED )                \
        roadmap_log( __VA_ARGS__ );         \
}
// This structure represents the JNI object.
// The class and object references have to be destroyed manually
#define JNI_OBJ_NAME_LEN 		64
typedef struct
{
	jclass  obj_class;
	jobject obj;
	JavaVM* jvm;
	jint ver;
	const char name[JNI_OBJ_NAME_LEN];
} android_jni_obj_type;

typedef struct
{
	JNIEnv*		env;
	jmethodID	mid;
} android_method_context_type;


// Pixel formats for the bitmap ( Reflection of the android SDK )
typedef enum
{
	_andr_pix_fmt_A_8   	=   	0x00000008,
	_andr_pix_fmt_JPEG  	=		0x00000100,
	_andr_pix_fmt_LA_88  	= 		0x0000000a,
	_andr_pix_fmt_L_8  	  	=	  	0x00000009,
	_andr_pix_fmt_OPAQUE  	=		0xffffffff,
	_andr_pix_fmt_RGBA_4444 =  	  	0x00000007,
	_andr_pix_fmt_RGBA_5551 =		0x00000006,
	_andr_pix_fmt_RGBA_8888 =		0x00000001,
	_andr_pix_fmt_RGBX_8888 =		0x00000002,
	_andr_pix_fmt_RGB_332  	=		0x0000000b,
	_andr_pix_fmt_RGB_565	=		0x00000004,
	_andr_pix_fmt_RGB_888	=		0x00000003,
	_andr_pix_fmt_TRANSLUCENT  	=	0xfffffffd,
	_andr_pix_fmt_TRANSPARENT  	=	0xfffffffe,
	_andr_pix_fmt_UNKNOWN		=	0x00000000,
	_andr_pix_fmt_YCbCr_420_SP	=	0x00000011,
	_andr_pix_fmt_YCbCr_422_SP	=	0x00000010
} android_pixel_format_type;


// Keycodes ( Reflection of the android SDK )
typedef enum
{
	_andr_keycode_soft_left			= 0x00000001,
	_andr_keycode_soft_right		= 0x00000002,
	_andr_keycode_home				= 0x00000003,
	_andr_keycode_return			= 0x00000004,
	_andr_keycode_call              = 0x00000005,
	_andr_keycode_end_call			= 0x00000006,
	_andr_keycode_dpad_up			= 0x00000013,
	_andr_keycode_dpad_down			= 0x00000014,
	_andr_keycode_dpad_left			= 0x00000015,
	_andr_keycode_dpad_right		= 0x00000016,
	_andr_keycode_dpad_center		= 0x00000017,
	_andr_keycode_camera            = 0x0000001B,
	_andr_keycode_enter				= 0x00000042,
	_andr_keycode_del				= 0x00000043,
	_andr_keycode_menu				= 0x00000052,
	_andr_keycode_search			= 0x00000054
} android_keycode_type;

// Actions for the keyboard
typedef enum
{
	_andr_ime_action_none 			= 0x00000001,
	_andr_ime_action_go 			   = 0x00000002,
	_andr_ime_action_search			= 0x00000003,
	_andr_ime_action_send			= 0x00000004,
	_andr_ime_action_next			= 0x00000005,
	_andr_ime_action_done 			= 0x00000006
} android_kb_action_type;

void InitJNIObject( android_jni_obj_type* aObj, JNIEnv * aJNIEnv, jobject aJObj, const char* aObjName );

jmethodID InitJNIMethodContext( android_jni_obj_type* aObj, android_method_context_type* aMthdContext,
																const char* aMethodName, const char* aMethodSig );

void DisposeJNIObject( android_jni_obj_type* aObj );

void DisposeCustomJNIObject( android_jni_obj_type* aObj, jobject aCustomObj );

void FreeMapNativeTimerManager_AddTask( int aIndex, int aTimerMsg, int aInterval );

void FreeMapNativeTimerManager_RemoveTask( int a_Index );

void FreeMapNativeTimerManager_ShutDown();

void FreeMapNativeManager_ShutDown();

void FreeMapNativeManager_PostNativeMessage( int aMsg );

void FreeMapNativeCanvas_RefreshScreen();

void FreeMapNativeSoundManager_PlayFile( const char* aFileName );

void FreeMapNativeManager_SetBackLightOn( int aAlwaysOn );

void FreeMapNativeManager_SetVolume( int aLvl, int aMinLvl, int aMaxLvl );

int FreeMapNativeManager_getBatteryLevel();

void FreeMapNativeManager_MinimizeApplication( int aDelay );

void FreeMapNativeManager_MaximizeApplication( void );

void FreeMapNativeManager_ShowDilerWindow();

void FreeMapNativeManager_HideSoftKeyboard();

void FreeMapNativeManager_ShowSoftKeyboard( int aAction, int aCloseOnAction );

int FreeMapNativeManager_TakePicture( int aImageWidth, int aImageHeight, int aImageQuality,
                                                            char* aImageFolder, char* aImageFile );

int FreeMapNativeManager_TakePictureAsync( int aImageWidth, int aImageHeight, int aImageQuality,
        char* aImageFolder, char* aImageFile );


int FreeMapNativeManager_GetThumbnail( int aThumbWidth, int aThumbHeight, int bytePP, int* aBuf );

void FreeMapNativeSoundManager_LoadSoundData( char* aDataDir );

void FreeMapNativeManager_SetIsMenuEnabled( int aEnabled );

void FreeMapNativeManager_Flush();

void WazeMenuManager_AddOptionsMenuItem( int aItemId, const char* aLabel, const char* aIcon,
		int aIsNative, int aPortraitOrder, int aLandscapeOrder, int aItemType );

void FreeMapNativeCanvas_RequestRender();

void FreeMapNativeManager_SwapBuffersEgl();

int FreeMapNativeManager_IsActive();

void FreeMapNativeManager_HideWebView( void );

void FreeMapNativeManager_ShowWebView( const char* aUrl, int aMinX, int aMinY, int aMaxX, int aMaxY, int aFlags );

void FreeMapNativeManager_ShowEditBox( int aAction, int aStayOnAction, const char* aText, void* aContext, int aTopMargin, int aFlags );

void FreeMapNativeManager_ResizeWebView( int aMinX, int aMinY, int aMaxX, int aMaxY );

void FreeMapNativeManager_HideEditBox( void );

void FreeMapNativeManager_LoadUrl( const char* aUrl );

void FreeMapNativeManager_ShowContacts( void );

void FreeMapNativeManager_MarketRate( void );

void WazeMsgBox_Show( const char* aTitle, const char* aMessage, const char* aLabelOk, const char* aLabelCancel, void* aCbContext );

void WazeSpeechttManager_StartNative( void* aContext, int aTimeout, const char* aLang, const char* aExtraPrompt, int aFlags );

void WazeSpeechttManager_Stop( void );

int WazeSpeechttManager_IsAvailable( void );

char* WazeResManager_LoadSkin( const char* aName, int* size );

void FreeMapNativeManager_EditBoxCheckTypingLockCb( int aRes );

int WazeSoundRecorder_Start( const char* aPath, int aTimeout );

void WazeSoundRecorder_Stop( void );

void FreeMapNativeManager_OpenExternalBrowser( const char* aUrl );

char** WazeResManager_LoadResList( const char* aPath );

void WazeAppWidgetManager_RouteRequestCallback(  int aStatus, const char* aErrDesc, const char* aDestDescription, int aTimeToDest );

void WazeAppWidgetManager_RequestRefresh( void );

void WazeMenuManager_ResetOptionsMenu( void );

void WazeMenuManager_SubmitOptionsMenu( void );

void WazeTtsManager_Commit( const char* aText, const char* aFullPath, void* aContext );
void WazeTtsManager_Prepare( void );

void FreeMapNativeManager_DisposeRefs( void );
void FreeMapNativeCanvas_DisposeRefs( void );
void FreeMapNativeSoundManager_DisposeRefs( void );
void FreeMapNativeTimerManager_DisposeRefs( void );
void WazeMenuManager_DisposeRefs( void );
void WazeMsgBox_DisposeRefs( void );
void WazeSpeechttManager_DisposeRefs( void );
void WazeSoundRecorder_DisposeRefs( void );
void WazeResManager_DisposeRefs( void );
void WazeTtsManager_DisposeRefs( void );
void WazeAppWidgetManager_DisposeRefs( void );
void CloseJNIObjects();


#endif // INCLUDE__FREEMAP_JNI__H
