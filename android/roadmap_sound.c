/* roadmap_sound.c - a low level module to manage sound for RoadMap.
 *
 * LICENSE:
 *
 *   Copyright 2008 Alex Agranovich
 *   Copyright 2008 Ehud Shabtai
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
 *   See roadmap_library.h
 */

#include <stdlib.h>
#include <string.h>
#include "JNI/FreeMapJNI.h"
#include "roadmap.h"
#include "roadmap_config.h"
#include "roadmap_path.h"
#include "roadmap_file.h"
#include "roadmap_sound.h"
#include "roadmap_prompts.h"
#include "roadmap_lang.h"

static RoadMapConfigDescriptor RoadMapConfigVolControl =
                        ROADMAP_CONFIG_ITEM( "Voice", "Volume Control" );

/* Defined in C file */
#define __SND_VOLUME_LVLS_COUNT__ (4)

const int SND_VOLUME_LVLS_COUNT = __SND_VOLUME_LVLS_COUNT__;
const int SND_VOLUME_LVLS[] = {0, 1, 2, 3};
const char* SND_VOLUME_LVLS_LABELS[__SND_VOLUME_LVLS_COUNT__];
const char* SND_DEFAULT_VOLUME_LVL = "2";

static BOOL 			sgInitialized = FALSE;

static const char* get_full_name( const char* name );

void roadmap_sound_initialize ()
{

   int curLvl;
   char sound_dir[MAX_SOUND_NAME];

   // Initialize the volume labels for GUI
   SND_VOLUME_LVLS_LABELS[0] = roadmap_lang_get( "Silent" );
   SND_VOLUME_LVLS_LABELS[1] = roadmap_lang_get( "Low" );
   SND_VOLUME_LVLS_LABELS[2] = roadmap_lang_get( "Medium" );
   SND_VOLUME_LVLS_LABELS[3] = roadmap_lang_get( "High" );

   // Set current volume from the configuration
   roadmap_config_declare("user", &RoadMapConfigVolControl, SND_DEFAULT_VOLUME_LVL, NULL );
   curLvl = roadmap_config_get_integer( &RoadMapConfigVolControl );
//   FreeMapNativeManager_SetVolume( curLvl, SND_VOLUME_LVLS[0], SND_VOLUME_LVLS[SND_VOLUME_LVLS_COUNT-1] );

   // Preload the sound resources
   snprintf( sound_dir, sizeof( sound_dir ), "%s//%s//%s",
				roadmap_path_downloads(), "sound", roadmap_prompts_get_name() );
   FreeMapNativeSoundManager_LoadSoundData( sound_dir );

   sgInitialized = TRUE;
   // Log the operation
   roadmap_log( ROADMAP_DEBUG, "Current volume initialized to level : %d.", curLvl );
}


void roadmap_sound_shutdown ()
{
//  CRoadMapNativeSound::FreeInstance();
}

RoadMapSoundList roadmap_sound_list_create ( int flags )
{
  RoadMapSoundList list =
        (RoadMapSoundList) calloc ( 1, sizeof( struct roadmap_sound_list_t ) );

  list->flags = flags;

  return list;
}

int roadmap_sound_list_add ( RoadMapSoundList list, const char *name )
{
  const char* full_name;
  if ( list->count == MAX_SOUND_LIST ) return SND_LIST_ERR_LIST_FULL;

  full_name = get_full_name( name );

  if ( !roadmap_file_exists( full_name, NULL ) )
  {
     roadmap_log( ROADMAP_ERROR, "File %s doesn't exist! Cannot add to the list.", full_name );
     return SND_LIST_ERR_NO_FILE;
  }

  strncpy (list->list[list->count], name, sizeof(list->list[0]));
  list->list[list->count][sizeof(list->list[0])-1] = '\0';
  list->count++;

  return list->count - 1;
}

int roadmap_sound_list_add_buf (RoadMapSoundList list, void* buf, size_t size )
{
   char path[512];
   int file_num = list->count;
   RoadMapFile file;

   if (list->count == MAX_SOUND_LIST) return SND_LIST_ERR_LIST_FULL;

   list->buf_list[list->count] = buf;
   list->buf_list_sizes[list->count] = size;


   /*
    * Temporary solution - write the buffer to the file for further playing
    * AGA
    */
   sprintf( path, "%s/tmp/%d", roadmap_path_tts(), file_num );
   if ( file_num == 0 )
   {
      roadmap_path_create( roadmap_path_parent( path, NULL ) );
   }

   file = roadmap_file_open( path, "w" );
   roadmap_file_write( file, buf, size );
   roadmap_file_close( file );

   strncpy_safe( list->list[list->count], path, 512 );


   list->count++;

   return list->count - 1;
}


/*************************************************************************************************
 * roadmap_sound_play_list()
 * Playing the sound files in the list by calling the JNI layer
 * FreeMapNativeSoundManager_PlaySoundFile utility
 */
int roadmap_sound_play_list( const RoadMapSoundList list )
{
	if ( sgInitialized )
	{
		int listSize = roadmap_sound_list_count( list );
		const char *full_name;
		int i;

		for( i = 0; i < roadmap_sound_list_count(list); i++ )
		{
			const char *name = roadmap_sound_list_get ( list, i );
			// !!!! Resources are not supported !!!!
			// RoadMapSound sound = (RoadMapSound) roadmap_res_get (RES_SOUND, RES_NOCREATE, name);

			// AGA NOTE :: TODO :: Temporary solution. All the resources are extracted now.
			// The unnecessary sounds should be cleared/not extracted (?)
             if ( (list->flags & SOUND_LIST_BUFFERS) == 0 )
             {
                 full_name = get_full_name( name );
                 // Calling the JNI layer
                 FreeMapNativeSoundManager_PlayFile( full_name );
             }
             else
             {
                /*
                 * Temporary solution - write the buffer to the file for further playing
                 * AGA
                 */
    //            FreeMapNativeSoundManager_PlayFile( roadmap_sound_list_get ( list, i ) );
             //			   FreeMapNativeSoundManager_PlayBuffer( list->buf_list[i], list->buf_list_sizes[i] );
    //            free( list->buf_list[i] );
             }
	}
	}
	// Deallocation
	if ( (list->flags & SOUND_LIST_NO_FREE) == 0x0 )
	{
		free (list);
	}
	return 0;
}




RoadMapSound roadmap_sound_load (const char *path, const char *file, int *mem)
{

   char *full_name = roadmap_path_join (path, file);
   const char *seq;
   RoadMapFileContext sound;
   char sound_filename[MAX_SOUND_NAME];

   return NULL;

   snprintf( sound_filename, sizeof(sound_filename), "%s.mp3", full_name);

   seq = roadmap_file_map (NULL, sound_filename, NULL, "r", &sound);

   roadmap_path_free (full_name);

   if (seq == NULL) {
      *mem = 0;
      return NULL;
   }

   *mem = roadmap_file_size (sound);

   return (RoadMapSound) sound;
}

int roadmap_sound_free (RoadMapSound sound)
{

   roadmap_file_unmap ((RoadMapFileContext*)(&sound));

   return 0;
}

int roadmap_sound_list_count (const RoadMapSoundList list) {

   return list->count;
}

const char *roadmap_sound_list_get (const RoadMapSoundList list, int i)
{

   if (i >= MAX_SOUND_LIST) return NULL;

   return list->list[i];
}

void roadmap_sound_list_free (RoadMapSoundList list) {

   free(list);
}

int roadmap_sound_play_file (const char *file_name) {
   return -1;
}

int roadmap_sound_record (const char *file_name, int seconds) {   
   return WazeSoundRecorder_Start( file_name, seconds*1000 );
}

void roadmap_sound_stop_recording (void){
   WazeSoundRecorder_Stop();
}

/***********************************************************
 *	Name 		: roadmap_sound_set_volume
 *	Purpose 	: Sets the user volume setting to the native sound object
 * 					with configuration update
 */
void roadmap_sound_set_volume ( int volLvl )
{
	// Update the device
//	FreeMapNativeManager_SetVolume( volLvl, SND_VOLUME_LVLS[0], SND_VOLUME_LVLS[SND_VOLUME_LVLS_COUNT-1] );

	// Update the configuration
	roadmap_config_set_integer( &RoadMapConfigVolControl, volLvl );

	// Log the operation
	roadmap_log( ROADMAP_DEBUG, "Current volume is set to level : %d.", volLvl );

}


static const char* get_full_name( const char* name )
{
   static char full_name[256];
   const char *suffix = "";

   if ( !strchr( name, '.' ) )
   {
      suffix = ".bin";
   }

   if ( roadmap_path_is_full_path( name ) )
   {
      strncpy_safe( full_name, name, sizeof( full_name ) );
   }
   else
   {
      snprintf( full_name, sizeof( full_name ), "%s//%s//%s//%s%s",
            roadmap_path_downloads(), "sound", roadmap_prompts_get_name(), name, suffix );
   }
   return full_name;
}
