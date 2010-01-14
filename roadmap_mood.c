/* roadmap_mood.c - Manage mood selection
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
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
 *   See roadmap_mood.h
 */

#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_history.h"
#include "roadmap_locator.h"
#include "roadmap_street.h"
#include "roadmap_lang.h"
#include "roadmap_geocode.h"
#include "roadmap_trip.h"
#include "roadmap_lang.h"
#include "roadmap_display.h"
#include "roadmap_config.h"
#include "roadmap_path.h"
#include "roadmap_navigate.h"
#include "roadmap_mood.h"
#include "ssd/ssd_keyboard_dialog.h"
#include "ssd/ssd_generic_list_dialog.h"
#include "Realtime/Realtime.h"


#define MAX_MOOD_ENTRIES 30
static int gState = -1;

typedef struct {

    const char *title;
    RoadMapCallback callback;

} roadmap_mood_list_dialog;

typedef struct {

  char *value;
  char *label;
  char *icon;

} roadmap_mood_list;
static RoadMapConfigDescriptor MoodCfg =
                        ROADMAP_CONFIG_ITEM("User", "Mood");


//////////////////////////////////////////////////////////////////////////////////////////////////
const char* roadmap_mood_to_string(MoodType mood){

   switch(mood)
   {
      case Mood_Happy:    		    return roadmap_path_join("moods",MOOD_Name_Happy);
      case Mood_Sad: 		  	    return roadmap_path_join("moods",MOOD_Name_Sad);
      case Mood_Mad:     	  	    return roadmap_path_join("moods",MOOD_Name_Mad);
      case Mood_Bored:     	    return roadmap_path_join("moods",MOOD_Name_Bored);
      case Mood_Speedy:     	    return roadmap_path_join("moods",MOOD_Name_Speedy);
      case Mood_Starving:         return roadmap_path_join("moods",MOOD_Name_Starving);
      case Mood_Sleepy:     	    return roadmap_path_join("moods",MOOD_Name_Sleepy);
      case Mood_Cool:             return roadmap_path_join("moods",MOOD_Name_Cool);
      case Mood_InLove:           return roadmap_path_join("moods",MOOD_Name_InLove);
      case Mood_LOL:              return roadmap_path_join("moods",MOOD_Name_LOL);
      case Mood_Peaceful:         return roadmap_path_join("moods",MOOD_Name_Peaceful);
      case Mood_Singing:          return roadmap_path_join("moods",MOOD_Name_Singing);
      case Mood_Wondering:        return roadmap_path_join("moods",MOOD_Name_Wondering);
      case Mood_Happy_Female:     return roadmap_path_join("moods",MOOD_Name_Happy_Female);
      case Mood_Sad_Female:       return roadmap_path_join("moods",MOOD_Name_Sad_Female);
      case Mood_Mad_Female:       return roadmap_path_join("moods",MOOD_Name_Mad_Female);
      case Mood_Bored_Female:     return roadmap_path_join("moods",MOOD_Name_Bored_Female);
      case Mood_Speedy_Female:    return roadmap_path_join("moods",MOOD_Name_Speedy_Female);
      case Mood_Starving_Female:  return roadmap_path_join("moods",MOOD_Name_Starving_Female);
      case Mood_Sleepy_Female:    return roadmap_path_join("moods",MOOD_Name_Sleepy_Female);
      case Mood_Cool_Female:      return roadmap_path_join("moods",MOOD_Name_Cool_Female);
      case Mood_InLove_Female:    return roadmap_path_join("moods",MOOD_Name_InLove_Female);
      case Mood_LOL_Female:       return roadmap_path_join("moods",MOOD_Name_LOL_Female);
      case Mood_Peaceful_Female:  return roadmap_path_join("moods",MOOD_Name_Peaceful_Female);
      case Mood_Singing_Female:   return roadmap_path_join("moods",MOOD_Name_Singing_Female);
      case Mood_Wondering_Female: return roadmap_path_join("moods",MOOD_Name_Wondering_Female);
      default:             	    return roadmap_path_join("moods",MOOD_Name_Happy);

   }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
const char* roadmap_mood_to_string_small(MoodType mood){

   switch(mood)
   {
      case Mood_Happy:    		      return MOOD_Name_Happy_Small;
      case Mood_Sad: 		  	      return MOOD_Name_Sad_Small;
      case Mood_Mad:     	  	      return MOOD_Name_Mad_Small;
      case Mood_Bored:     	      return MOOD_Name_Bored_Small;
      case Mood_Speedy:     	      return MOOD_Name_Speedy_Small;
      case Mood_Starving:           return MOOD_Name_Starving_Small;
      case Mood_Sleepy:     	      return MOOD_Name_Sleepy_Small;
      case Mood_Cool:               return MOOD_Name_Cool_Small;
      case Mood_InLove:             return MOOD_Name_InLove_Small;
      case Mood_LOL:                return MOOD_Name_LOL_Small;
      case Mood_Peaceful:           return MOOD_Name_Peaceful_Small;
      case Mood_Singing:            return MOOD_Name_Singing_Small;
      case Mood_Wondering:          return MOOD_Name_Wondering_Small;
      case Mood_Happy_Female:       return MOOD_Name_Happy_Small_Female;
      case Mood_Sad_Female:         return MOOD_Name_Sad_Small_Female;
      case Mood_Mad_Female:         return MOOD_Name_Mad_Small_Female;
      case Mood_Bored_Female:       return MOOD_Name_Bored_Small_Female;
      case Mood_Speedy_Female:      return MOOD_Name_Speedy_Small_Female;
      case Mood_Starving_Female:    return MOOD_Name_Starving_Small_Female;
      case Mood_Sleepy_Female:      return MOOD_Name_Sleepy_Small_Female;
      case Mood_Cool_Female:        return MOOD_Name_Cool_Small_Female;
      case Mood_InLove_Female:      return MOOD_Name_InLove_Small_Female;
      case Mood_LOL_Female:         return MOOD_Name_LOL_Small_Female;
      case Mood_Peaceful_Female:    return MOOD_Name_Peaceful_Small_Female;
      case Mood_Singing_Female:     return MOOD_Name_Singing_Small_Female;
      case Mood_Wondering_Female:   return MOOD_Name_Wondering_Small_Female;
      default:             	      return MOOD_Name_Happy_Small;

   }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
MoodType roadmap_mood_from_string( const char* szMood){

   if( !strcmp( szMood, MOOD_Name_Happy))            return Mood_Happy;
   if( !strcmp( szMood, MOOD_Name_Sad))              return Mood_Sad;
   if( !strcmp( szMood, MOOD_Name_Mad))              return Mood_Mad;
   if( !strcmp( szMood, MOOD_Name_Bored))            return Mood_Bored;
   if( !strcmp( szMood, MOOD_Name_Speedy))           return Mood_Speedy;
   if( !strcmp( szMood, MOOD_Name_Starving))         return Mood_Starving;
   if( !strcmp( szMood, MOOD_Name_Sleepy))           return Mood_Sleepy;
   if( !strcmp( szMood, MOOD_Name_Cool))             return Mood_Cool;
   if( !strcmp( szMood, MOOD_Name_InLove))           return Mood_InLove;
   if( !strcmp( szMood, MOOD_Name_LOL))              return Mood_LOL;
   if( !strcmp( szMood, MOOD_Name_Peaceful))         return Mood_Peaceful;
   if( !strcmp( szMood, MOOD_Name_Singing))          return Mood_Singing;
   if( !strcmp( szMood, MOOD_Name_Wondering))        return Mood_Wondering;
   if( !strcmp( szMood, MOOD_Name_Happy_Female))     return Mood_Happy_Female;
   if( !strcmp( szMood, MOOD_Name_Sad_Female))       return Mood_Sad_Female;
   if( !strcmp( szMood, MOOD_Name_Mad_Female))       return Mood_Mad_Female;
   if( !strcmp( szMood, MOOD_Name_Bored_Female))     return Mood_Bored_Female;
   if( !strcmp( szMood, MOOD_Name_Speedy_Female))    return Mood_Speedy_Female;
   if( !strcmp( szMood, MOOD_Name_Starving_Female))  return Mood_Starving_Female;
   if( !strcmp( szMood, MOOD_Name_Sleepy_Female))    return Mood_Sleepy_Female;
   if( !strcmp( szMood, MOOD_Name_Cool_Female))      return Mood_Cool_Female;
   if( !strcmp( szMood, MOOD_Name_InLove_Female))    return Mood_InLove_Female;
   if( !strcmp( szMood, MOOD_Name_LOL_Female))       return Mood_LOL_Female;
   if( !strcmp( szMood, MOOD_Name_Peaceful_Female))  return Mood_Peaceful_Female;
   if( !strcmp( szMood, MOOD_Name_Singing_Female))   return Mood_Singing_Female;
   if( !strcmp( szMood, MOOD_Name_Wondering_Female)) return Mood_Wondering_Female;
   return Mood_Happy;
}
///////////////////////////////////////////////////////////////////////////////////////////
const char *roadmap_mood_get(){
	const char * mood_cfg;
	roadmap_config_declare
        ("user", &MoodCfg, "happy", NULL);
    mood_cfg = roadmap_config_get (&MoodCfg);
    return roadmap_path_join("moods", mood_cfg);
}

///////////////////////////////////////////////////////////////////////////////////////////
const char *roadmap_mood_get_name(){
	roadmap_config_declare
        ("user", &MoodCfg, "happy", NULL);
    return roadmap_config_get (&MoodCfg);
}

///////////////////////////////////////////////////////////////////////////////////////////
void roadmap_mood_set(const char *value){
   roadmap_config_declare
        ("user", &MoodCfg, "happy", NULL);
   roadmap_config_set (&MoodCfg, value);
   roadmap_config_save(1);
   gState = roadmap_mood_from_string(value);
   OnMoodChanged();
}

///////////////////////////////////////////////////////////////////////////////////////////
const char *roadmap_mood_get_top_name(){

	static char mood_top[100];
	roadmap_config_declare
        ("user", &MoodCfg, "happy", NULL);
    sprintf(mood_top, "top_mood_%s", roadmap_config_get (&MoodCfg));
    return &mood_top[0];
}

///////////////////////////////////////////////////////////////////////////////////////////
int roadmap_mood_state(void){

   if (!RealTimeLoginState())
      return 0;

   if (gState == -1){
		gState = roadmap_mood_from_string(roadmap_mood_get_name());
	}
	return gState;
}

///////////////////////////////////////////////////////////////////////////////////////////
static int roadmap_mood_call_back (SsdWidget widget, const char *new_value, const void *value, void *context) {

   roadmap_mood_list_dialog *list_context = (roadmap_mood_list_dialog *)context;
   roadmap_mood_set(value);
   ssd_generic_list_dialog_hide ();

   if (list_context->callback)
   		(*list_context->callback)();

   return 1;
}

static int cstring_cmp(const void *a, const void *b)
{
    const char **ia = (const char **)a;
    const char **ib = (const char **)b;
    return strcmp(*ia, *ib);
   /* strcmp functions works exactly as expected from
   comparison function */
}

///////////////////////////////////////////////////////////////////////////////////////////
void roadmap_mood_dialog (RoadMapCallback callback) {

    char **files;
    const char *cursor;
    char **cursor2;
    char *directory = NULL;
    int count = 0;
    int i;

    static roadmap_mood_list_dialog context = {"roadmap_mood", NULL};
   	static char *labels[MAX_MOOD_ENTRIES] ;
	static void *values[MAX_MOOD_ENTRIES] ;
	static void *icons[MAX_MOOD_ENTRIES];


	context.callback = callback;
    for (cursor = roadmap_path_first ("skin");
            cursor != NULL;
            cursor = roadmap_path_next ("skin", cursor)) {

	    directory = roadmap_path_join (cursor, "moods");

    	files = roadmap_path_list (directory, ".png");
    	if ( *files == NULL )
    	{
    	   files = roadmap_path_list (directory, NULL);
    	}

   		for (cursor2 = files; *cursor2 != NULL; ++cursor2) {
   	  			labels[count]  =   (char *)(strtok(*cursor2,"."));
      			count++;
   		}
   }


    qsort((void *) &labels[0], count, sizeof(void *), cstring_cmp);

    for (i = 0; i< count; i++){
       values[i] = labels[i];
       icons[i]   =   roadmap_path_join("moods", labels[i]);
       labels[i] = (char *)roadmap_lang_get(labels[i]);
    }

    free(directory);

    ssd_generic_icon_list_dialog_show (roadmap_lang_get ("Select your mood"),
                  count,
                  (const char **)labels,
                  (const void **)values,
                  (const char **)icons,
                  NULL,
                  roadmap_mood_call_back,
                  NULL,
                  &context,
                  NULL,
                  NULL,
                  70,
                  0,
                  FALSE);

}

///////////////////////////////////////////////////////////////////////////////////////////
void roadmap_mood(void){

	roadmap_mood_dialog(NULL);
}

