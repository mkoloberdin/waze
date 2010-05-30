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
#include "roadmap_lang.h"

static RoadMapConfigDescriptor RoadMapConfigVolControl =
                        ROADMAP_CONFIG_ITEM( "Voice", "Volume Control" );

/* Defined in C file */
#define __SND_VOLUME_LVLS_COUNT__ (4)

const int SND_VOLUME_LVLS_COUNT = __SND_VOLUME_LVLS_COUNT__;
const int SND_VOLUME_LVLS[] = {0, 1, 2, 3};
const char* SND_VOLUME_LVLS_LABELS[__SND_VOLUME_LVLS_COUNT__];
const char* SND_DEFAULT_VOLUME_LVL = "2";

#define MAX_SOUND_NAME_ANDROID  256
static BOOL 			sgInitialized = FALSE;


void roadmap_sound_initialize ()
{

   int curLvl;
   char sound_dir[MAX_SOUND_NAME_ANDROID];

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

int roadmap_sound_list_add (RoadMapSoundList list, const char *name)
{
  if (list->count == MAX_SOUND_LIST) return -1;

  strncpy (list->list[list->count], name, sizeof(list->list[0]));
  list->list[list->count][sizeof(list->list[0])-1] = '\0';
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
		int i;
		const char *suffix = "";

		for( i = 0; i < roadmap_sound_list_count(list); i++ )
		{
			const char *name = roadmap_sound_list_get ( list, i );
			// !!!! Resources are not supported !!!!
			// RoadMapSound sound = (RoadMapSound) roadmap_res_get (RES_SOUND, RES_NOCREATE, name);

			// AGA NOTE :: TODO :: Temporary solution. All the resources are extracted now.
			// The unnecessary sounds should be cleared/not extracted (?)
			char sound_filename[MAX_SOUND_NAME_ANDROID];
			if ( !strchr( name, '.' ) )
			{
				suffix = ".bin";
			}

			snprintf( sound_filename, sizeof( sound_filename ), "%s//%s//%s//%s%s",
					roadmap_path_downloads(), "sound", roadmap_prompts_get_name(), name, suffix );
			// Calling the JNI layer
			FreeMapNativeSoundManager_PlayFile( sound_filename );
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
   char sound_filename[MAX_SOUND_NAME_ANDROID];

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
   return -1;
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
