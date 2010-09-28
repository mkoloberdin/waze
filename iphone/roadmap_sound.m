/* roadmap_sound.m - Play sound.
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi R.
 *   Copyright 2009, Waze Ltd
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
 *   See roadmap_sound.h
 */

#include <stdlib.h>
#include <string.h>
#include "roadmap.h"
#include "roadmap_file.h"
#include "roadmap_path.h"
#include "roadmap_res.h"
#include "roadmap_main.h"
#include "roadmap_prompts.h"
#include "roadmap_media_player.h"

#include <UIKit/UIKit.h>
#include <AudioToolbox/AudioServices.h>
#include <AVFoundation/AVAudioPlayer.h>

#include "roadmap_sound.h"
#include "roadmap_iphonesound.h"

const char* SND_DEFAULT_VOLUME_LVL = "2";

static int              current_list  = -1;
static int              current_index = -1;
#define FREE_FLAG -2

#define MAX_LISTS 2

static RoadMapSoundList sound_lists[MAX_LISTS];
static SoundDelegate    *RoadmapSoundDelegate;
static AVAudioPlayer    *dummyPlayer;


static void play_next_file (void);


static void set_duck (int enabled) {
   UInt32 overrideMixing = TRUE;
   UInt32 allowMixing = enabled;
   OSStatus propertySetError = 0;
   
   AudioSessionSetActive(FALSE);
   
   if (enabled) {
      propertySetError |= AudioSessionSetProperty(kAudioSessionProperty_OverrideCategoryMixWithOthers, sizeof(overrideMixing), &overrideMixing);
      propertySetError |= AudioSessionSetProperty(kAudioSessionProperty_OtherMixableAudioShouldDuck, sizeof(allowMixing), &allowMixing);
   } else {      
      propertySetError |= AudioSessionSetProperty(kAudioSessionProperty_OtherMixableAudioShouldDuck, sizeof(allowMixing), &allowMixing);
      propertySetError |= AudioSessionSetProperty(kAudioSessionProperty_OverrideCategoryMixWithOthers, sizeof(overrideMixing), &overrideMixing);
   }
   
   if (propertySetError)
      roadmap_log(ROADMAP_ERROR, "set_duck() failed to set audio session properties, err: %d", propertySetError);
   
   AudioSessionSetActive(TRUE);
}


RoadMapSoundList roadmap_sound_list_create (int flags) {

   RoadMapSoundList list =
            (RoadMapSoundList) calloc (1, sizeof(struct roadmap_sound_list_t));
   list->flags = flags;   
   
   return list;
}


int roadmap_sound_list_add (RoadMapSoundList list, const char *name) {

   if (list->count == MAX_SOUND_LIST) return -1;

   strncpy (list->list[list->count], name, sizeof(list->list[0]));
   list->list[list->count][sizeof(list->list[0])-1] = '\0';
   list->count++;

   return list->count - 1;
}


int roadmap_sound_list_count (const RoadMapSoundList list) {

   return list->count;
}


const char *roadmap_sound_list_get (const RoadMapSoundList list, int i) {

   if (i >= MAX_SOUND_LIST) return NULL;

   return list->list[i];
}


void roadmap_sound_list_free (RoadMapSoundList list) {

   free(list);
}


RoadMapSound roadmap_sound_load (const char *path, const char *file, int *mem) {

   return 0;
}


int roadmap_sound_free (RoadMapSound sound) {

   return 0;
}


int roadmap_sound_play      (RoadMapSound sound) {
/*
   int res;
   void *mem;

   if (!sound) return -1;
   mem = roadmap_file_base ((RoadMapFileContext)sound);

   //TODO: res = PlaySound((LPWSTR)mem, NULL, SND_SYNC | SND_MEMORY);

   if (res) return 0;
   else return -1;*/
   
   return -1;
}

char *get_full_name(const char *file_name) {
   char full_name[MAX_SOUND_NAME];
   
   //check if we have .mp3 extention
   char result[MAX_SOUND_NAME];
   char *p;
   
   strncpy_safe(result, file_name, sizeof(result));
   
   p = roadmap_path_skip_directories (result);
   p = strrchr (p, '.');
   if (p == NULL) {
      strcat (result,".mp3");
   }
   
   file_name = result;
   
   if (roadmap_path_is_full_path (file_name)) {
      strncpy_safe(full_name, file_name, sizeof(full_name));
   } else {
      const char* prompts_name = roadmap_prompts_get_name();
      snprintf (full_name, sizeof(full_name), "%ssound/%s/%s",
                roadmap_path_user(), prompts_name, file_name);
   }
   
   return strdup(full_name);
}

int play_file (AVAudioPlayer* audioPlayer) {
   BOOL success = FALSE;
   
   if (audioPlayer) {
      success = [audioPlayer play];
   }
   
   if (!success) {
      roadmap_log (ROADMAP_WARNING, "Sound play failed 'play' command");
      
      //play_next_file ();
   }
   return success;
}

int prepare_file (const char *file_name, AVAudioPlayer** audioPlayer) {
   char *full_name;
   NSURL *fileURL;
   
   full_name = get_full_name(file_name);
   
   fileURL = [NSURL URLWithString:[[NSString stringWithUTF8String:full_name] stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding]];
   
   if (!fileURL) {
      roadmap_log (ROADMAP_WARNING, "prepare_file(): Can't create file url: %s", full_name);
      free(full_name);
      return 0;
   }
   
   BOOL success = FALSE;
   NSError *err;
   *audioPlayer = [[AVAudioPlayer alloc] initWithContentsOfURL:fileURL error:&err];
   
   if (err == NULL) {
      (*audioPlayer).delegate = RoadmapSoundDelegate;
      success = [*audioPlayer prepareToPlay];
   }
   
   if (!success) {
      if (err) {
         roadmap_log (ROADMAP_WARNING, "prepare_file(): Sound play failed to init; File: '%s' ; Err: %s", 
                      full_name, [[err description] UTF8String]);
      } else {
         roadmap_log (ROADMAP_WARNING, "prepare_file(): Sound play failed 'prepareToPlay' command; File: '%s'",
                      full_name);
      }
      
      if (audioPlayer) {
         [*audioPlayer release];
         *audioPlayer = NULL;
      }
   }
   
   free(full_name);
   
   return success;
}


static void play_next_file (void) {
	
	RoadMapSoundList list;
   static AVAudioPlayer *audioPlayer = NULL;
   const char *next_name;
	
	
	if (current_index == FREE_FLAG) {
		roadmap_sound_list_free (sound_lists[current_list]);
		sound_lists[current_list] = NULL;
		current_index = -1;
	}
	
	
	if (current_list == -1 || current_index == -1) { // move to next list
		current_list = (current_list + 1) % MAX_LISTS;
		current_index = 0;
		
		if (sound_lists[current_list] == NULL) {
         /* nothing to play */
         current_list = -1;
		}
		
		if (current_list == -1) {
         //roadmap_media_player_pause(FALSE);
         //AudioSessionSetActive(FALSE);
         set_duck(0);
         return;
		}
	} else { //same list, next index
		current_index++;
	}
	
	list = sound_lists[current_list];
	
	const char *name = roadmap_sound_list_get (list, current_index);
	/*
	 RoadMapSound sound =
	 roadmap_res_get (RES_SOUND, RES_NOCREATE, name);
	 */
	RoadMapSound sound = NULL;
	
	if (current_index == roadmap_sound_list_count (list) - 1) {
		if (!(list->flags & SOUND_LIST_NO_FREE)) {
            //roadmap_sound_list_free (list);
			current_index = FREE_FLAG; //We want to free the list only after the last file playback
		} else {
			sound_lists[current_list] = NULL;
			current_index = -1;
		}
      next_name = NULL;
	} else {
      //we have another file to play next, so prepare it for playback
      next_name = roadmap_sound_list_get (list, current_index+1);
   }
	
	if (!roadmap_main_should_mute()) {
		if (sound) {
         roadmap_sound_play (sound);
		} else {
         if (!audioPlayer)
            prepare_file(name, &audioPlayer);
         
         if (audioPlayer) {
            play_file (audioPlayer);
            audioPlayer = NULL;
         }
         else
            play_next_file();
         
         if (next_name)
            prepare_file(next_name, &audioPlayer);
		}
	} else { //skip this file, as mute is ON
		play_next_file ();
	}
}

void sound_complete (AVAudioPlayer *player){
   player.delegate = NULL;
   [player release];
   
   play_next_file();
}


int roadmap_sound_play_file (const char *file_name) {

   int res;
   AVAudioPlayer *audioPlayer = NULL;
    
   //roadmap_media_player_pause(TRUE);
   //AudioSessionSetActive(TRUE);
   set_duck(1);

   res = prepare_file(file_name, &audioPlayer);
   if (res)
      res = play_file (audioPlayer); 

   if (res) {
      return 0;
   } else {
      //roadmap_media_player_pause(FALSE); 
      //AudioSessionSetActive(FALSE);
      set_duck(0);
      return -1;
   }
}




int roadmap_sound_play_list (const RoadMapSoundList list) {
   
   if (current_list == -1) {
      /* not playing */
      //roadmap_media_player_pause(TRUE);
      //AudioSessionSetActive(TRUE);
      set_duck(1);
      sound_lists[0] = list;
      play_next_file ();

   } else {

      int next = (current_list + 1) % MAX_LISTS;

      if (sound_lists[next] != NULL) {
		  if (!(sound_lists[next]->flags & SOUND_LIST_NO_FREE)) {
			  roadmap_sound_list_free (sound_lists[next]);
		  }
      }

      sound_lists[next] = list;
   }

   return 0;
}


int roadmap_sound_record (const char *file_name, int seconds) {

   return 0;
}

void startPlayer () {
   //This is a workaround for controlling the correct volume with the hw buttons.
   //Only required for OS 3.0 or below. Can be removed once 3.1 is used
   
   NSError *err;
   NSURL *fileURL;
   fileURL = [NSURL URLWithString:[NSString stringWithFormat:@"%s/sound/eng/m.mp3",roadmap_path_bundle()]];
   
   if (fileURL) {
      dummyPlayer = [[AVAudioPlayer alloc] initWithContentsOfURL:fileURL error:&err];
      if (dummyPlayer)
         [dummyPlayer prepareToPlay];
   }
}

void stopPlayer () {
   //This is a workaround for controlling the correct volume with the hw buttons.
   //Only required for OS 3.0 / 3.0.1
   
   if (dummyPlayer)
      [dummyPlayer release];
}


void roadmap_sound_initialize (void) {
   
   AudioSessionInitialize (NULL, NULL, NULL, NULL);
   //UInt32 sessionCategory = kAudioSessionCategory_AmbientSound;
   UInt32 sessionCategory = kAudioSessionCategory_MediaPlayback;
   AudioSessionSetProperty (kAudioSessionProperty_AudioCategory, sizeof (sessionCategory), &sessionCategory);
   UInt32 overrideMixing = TRUE;
   AudioSessionSetProperty(kAudioSessionProperty_OverrideCategoryMixWithOthers, sizeof(overrideMixing), &overrideMixing);
   
   
   AudioSessionSetActive (TRUE);
   
   RoadmapSoundDelegate = [[SoundDelegate alloc] init];
   
   if (roadmap_main_get_os_ver() == ROADMAP_MAIN_OS_30)
      startPlayer();//workaround for 3.0 OS volume control
   
   roadmap_media_player_init();
}


void roadmap_sound_shutdown   (void) {
   if (roadmap_main_get_os_ver() == ROADMAP_MAIN_OS_30)
      stopPlayer();//workaround for 3.0 OS volume control
   
   roadmap_media_player_shutdown();
}


@implementation SoundDelegate

- (void)audioPlayerDidFinishPlaying:(AVAudioPlayer *)player successfully:(BOOL)flag
{
   if (!flag)
      roadmap_log (ROADMAP_ERROR, "Finished sound playback with error");
   
   sound_complete(player);
}

- (void)audioPlayerBeginInterruption:(AVAudioPlayer *)player
{
   if (player != dummyPlayer)
      roadmap_log(ROADMAP_DEBUG, "Sound player interruption begin");
}

- (void)audioPlayerEndInterruption:(AVAudioPlayer *)player
{
   if (player != dummyPlayer) {
      roadmap_log(ROADMAP_DEBUG, "Sound player interruption end");
      [player play];
   }
}

- (void)audioPlayerDecodeErrorDidOccur:(AVAudioPlayer *)player error:(NSError *)error
{
   roadmap_log (ROADMAP_ERROR, "Sound player decode error code :%d", [error code]);
}


@end

