/* tts_apptext.c - implementation of the TTS for the common application texts
 *
 * LICENSE:
 *   Copyright 2011, Waze Ltd      Alex Agranovich (AGA)
 *
 *
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License V2 as published by
 *   the Free Software Foundation.
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
 *   See tts.h
 *
 */
#include <stdlib.h>
#include <string.h>
#include "tts/tts.h"
#include "tts_apptext_defs.h"
#include "roadmap_config.h"
#include "navigate/navigate_main.h"
#include "Realtime/Realtime.h"
#include "roadmap_lang.h"

//======================== Local defines ========================
#define TTS_APPTEXT_APPLY_LANG                    (0)


//======================== Local types ========================



//======================== Globals ========================

static const char* sgAppTextAlerts[] =
                                             {
                                                TTS_APPTEXT_APPROACH_SPEED_CAM,
                                                TTS_APPTEXT_APPROACH_TRAFFIC,
                                                TTS_APPTEXT_APPROACH_ACCIDENT,
                                                TTS_APPTEXT_APPROACH_HAZARD,
                                                TTS_APPTEXT_POLICE,
                                                TTS_APPTEXT_APPROACH_REDLIGHT_CAM
                                             };

static const char* sgAppTextStartDrive[] =
                                             {
                                                TTS_APPTEXT_LETS_GO,
                                                TTS_APPTEXT_ALL_SET,
                                                TTS_APPTEXT_ALL_SET_2,
                                                TTS_APPTEXT_LETS_GET_STARTED,
                                                TTS_APPTEXT_LETS_GET_STARTED_DRIVE_SAFE
                                             };

//static const char* sgAppTextStatus[] = {};
//
//static const char* sgAppTextCommon[] = {};

/*
 * Status attributes
 */
static RoadMapCallback sgOnLoginNextCallback = NULL;

extern RoadMapConfigDescriptor NavigateConfigNavigationGuidanceType;

//======================== Local Declarations ========================
static void _on_login( void );
static void _on_voice_changed( const char* voice_id, BOOL force_recommit );
static void _preload_list( const char* preload_list[], int list_size, int apply_lang );
static void _preload_cb( const void* user_context, int res_status, const char* text );
static void _preload( int apply_lang );

/*
 ******************************************************************************
 */
void tts_apptext_init( void )
{
   sgOnLoginNextCallback = Realtime_NotifyOnLogin ( _on_login );
}

/*
 ******************************************************************************
 */
BOOL tts_apptext_available( const char* text )
{
   BOOL tts_on = roadmap_config_match( &NavigateConfigNavigationGuidanceType, NAV_CFG_GUIDANCE_TYPE_TTS );

   return tts_on && tts_text_available( text, NULL );
}


/*
 ******************************************************************************
 */
void tts_apptext_play( const char* text )
{
   TtsPlayList pl = tts_playlist_create( NULL );
   tts_playlist_add( pl, text );
   tts_playlist_play( pl );
}

/*
 ******************************************************************************
 */
RoadMapSoundList tts_apptext_get_sound( const char* text )
{
   TtsPlayList tts_pl = tts_playlist_create( NULL );
   RoadMapSoundList sl = NULL;
   if ( tts_pl )
   {
      if ( tts_playlist_add( tts_pl, text ) )
      {
         sl = tts_playlist_export_list( tts_pl, TRUE );
      }
   }
   return sl;
}

/*
 ******************************************************************************
 */
const char* tts_apptext_get_start_drive( int index )
{
   const char* text = NULL;
   int size_start_drive = sizeof( sgAppTextStartDrive ) / sizeof( *sgAppTextStartDrive );

   if ( ( index >= 0 ) )
   {
      index %= size_start_drive;
      text = sgAppTextStartDrive[index];
   }
   else
   {
      roadmap_log( ROADMAP_ERROR, "Cannot access negative indexes!" );
   }

   return text;
}

/*
 ******************************************************************************
 * On login initialization callback
 */
static void _on_login( void )
{
   if ( sgOnLoginNextCallback )
      sgOnLoginNextCallback();

   if ( tts_feature_enabled() )
   {
      tts_register_on_voice_changed( ( TtsOnVoiceChangedCb ) _on_voice_changed );

      _preload( TTS_APPTEXT_APPLY_LANG );
   }
}

/*
 ******************************************************************************
 * Resubmits the texts with the new voice
 * Auxiliary
 */
static void _on_voice_changed( const char* voice_id, BOOL force_recommit )
{
   _preload( TTS_APPTEXT_APPLY_LANG );
}


/*
 ******************************************************************************
 * Preloader
 * Auxiliary
 */
static void _preload( int apply_lang )
{
   int size_alerts = sizeof( sgAppTextAlerts ) / sizeof( *sgAppTextAlerts );
   int size_start_drive = sizeof( sgAppTextStartDrive ) / sizeof( *sgAppTextStartDrive );

   // Alerts
   _preload_list( sgAppTextAlerts, size_alerts, apply_lang );

   // Start drive
   _preload_list( sgAppTextStartDrive, size_start_drive, apply_lang );

   // Commit all
   tts_commit();
}

/*
 ******************************************************************************
 * Prepares the text in the list
 * Auxiliary
 */
static void _preload_list( const char* preload_list[], int list_size, int apply_lang )
{
   const char* text;
   int i;

   for ( i = 0; i < list_size; ++i )
   {
      if ( apply_lang )
         text = roadmap_lang_get( preload_list[i] );
      else
         text = preload_list[i];
      tts_request_ex( text, TTS_TEXT_TYPE_DEFAULT, _preload_cb, (void*) text, TTS_FLAG_NONE );
   }
}

/*
 ******************************************************************************
 * Preload callback
 * Auxiliary
 */
static void _preload_cb( const void* user_context, int res_status, const char* text )
{
   roadmap_log( ROADMAP_DEBUG, "Preload status %d for application text: %s", res_status, (const char*) user_context );
}
