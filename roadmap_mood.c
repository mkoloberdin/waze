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
#include "ssd/ssd_list.h"
#include "ssd/ssd_text.h"
#include "Realtime/Realtime.h"

#ifdef IPHONE
#include "roadmap_bar.h"
#endif //IPHONE


#define MAX_MOOD_ENTRIES 35
#define MAX_EXCLUSIVE_ICONS 3
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

static int g_exclusiveMoods = 0;

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
      case Mood_Bronze:           return strdup(MOOD_Name_Bronze);
      case Mood_Silver:           return strdup(MOOD_Name_Silver);
      case Mood_Gold:             return strdup(MOOD_Name_Gold);
      case Mood_Busy:             return roadmap_path_join("moods",MOOD_Name_Busy);
      case Mood_Busy_Female:      return roadmap_path_join("moods",MOOD_Name_Busy_Female);
      case Mood_In_a_Hurry:       return roadmap_path_join("moods",MOOD_Name_In_A_Hurry);
      case Mood_In_a_Hurry_Female:return roadmap_path_join("moods",MOOD_Name_In_A_Hurry_Female);

      default:             	    return roadmap_path_join("moods",MOOD_Name_Happy);

   }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static const char* roadmap_exclusive_mood_string(MoodType mood){

   switch(mood)
   {
      case 1:           return MOOD_Name_Bronze;
      case 2:           return MOOD_Name_Silver;
      case 3:           return MOOD_Name_Gold;

      default:                    return MOOD_Name_Happy;
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
   if( !strcmp( szMood, MOOD_Name_Bronze))           return Mood_Bronze;
   if( !strcmp( szMood, MOOD_Name_Silver))           return Mood_Silver;
   if( !strcmp( szMood, MOOD_Name_Gold))             return Mood_Gold;
   if( !strcmp( szMood, MOOD_Name_Busy))             return Mood_Busy;
   if( !strcmp( szMood, MOOD_Name_Busy_Female))      return Mood_Busy_Female;
   if( !strcmp( szMood, MOOD_Name_In_A_Hurry))       return Mood_In_a_Hurry;
   if( !strcmp( szMood, MOOD_Name_In_A_Hurry_Female))return Mood_In_a_Hurry_Female;

   return Mood_Happy;
}

///////////////////////////////////////////////////////////////////////////////////////////
int roadmap_mood_get_exclusive_moods(void){
//Also called from iPhone dialog
   return g_exclusiveMoods;
}

///////////////////////////////////////////////////////////////////////////////////////////
void roadmap_mood_set_exclusive_moods(int mood){
   int iMood = roadmap_mood_from_string(roadmap_mood_get_name());
   g_exclusiveMoods = mood;

   if (mood != 0){
      roadmap_mood_set(roadmap_exclusive_mood_string(mood));
   }
   else if ((roadmap_mood_actual_state() >= Mood_Bronze ) && (roadmap_mood_actual_state() <= Mood_Gold )  && ((iMood - Mood_Bronze) > mood -1)){
      roadmap_mood_set("happy");
   }
}

///////////////////////////////////////////////////////////////////////////////////////////
const char *roadmap_mood_get(){
	const char * mood_cfg;
	roadmap_config_declare
        ("user", &MoodCfg, "happy", NULL);
    mood_cfg = roadmap_config_get (&MoodCfg);
    if (strstr(mood_cfg, "wazer"))
       return strdup(mood_cfg);
    else
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

	return roadmap_mood_actual_state();
}

///////////////////////////////////////////////////////////////////////////////////////////
int roadmap_mood_actual_state(void){

   if (gState == -1){
      gState = roadmap_mood_from_string(roadmap_mood_get_name());
   }
   return gState;
}

///////////////////////////////////////////////////////////////////////////////////////////
static int roadmap_mood_call_back (SsdWidget widget, const char *new_value, const void *value) {

   RoadMapCallback cb = (RoadMapCallback)widget->parent->context;
   roadmap_mood_set(value);
   ssd_dialog_hide ( "MoodDlg", dec_close );

   if (cb)
   		(*cb)();

   return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////
static int roadmap_exclusive_mood_call_back (SsdWidget widget, const char *new_value, const void *value) {

   RoadMapCallback cb = (RoadMapCallback)widget->parent->context;
   int iMood = roadmap_mood_from_string( value);

   if ( (iMood <= Mood_Gold) && (iMood >= Mood_Bronze) && (iMood - Mood_Bronze) > roadmap_mood_get_exclusive_moods() -1) {
      ssd_dialog_set_focus(NULL);
      ssd_dialog_draw();

      return 0;
   }

   roadmap_mood_set(value);
   ssd_dialog_hide ( "MoodDlg", dec_close );

   if (cb)
         (*cb)();

   return 1;
}
#ifndef IPHONE
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
    SsdWidget moodDlg;
    SsdWidget list;
    SsdWidget exclusive_list;
    SsdWidget text;
    int i;
    int row_height = 40;
    SsdListCallback exclusive_callback = NULL;
    int flags = 0;

    static roadmap_mood_list_dialog context = {"roadmap_mood", NULL};
    static char *labels[MAX_MOOD_ENTRIES] ;
	 static void *values[MAX_MOOD_ENTRIES] ;
	 static void *icons[MAX_MOOD_ENTRIES];

	 static char *exclusive_labels[MAX_EXCLUSIVE_ICONS] ;
	 static void *exclusive_values[MAX_EXCLUSIVE_ICONS] ;
	 static void *exclusive_icons[MAX_EXCLUSIVE_ICONS];

#ifdef OPENGL
    flags |= SSD_ALIGN_CENTER|SSD_CONTAINER_BORDER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE;
#endif

    if (roadmap_screen_is_hd_screen())
       row_height = 70;

    moodDlg   = ssd_dialog_new ( "MoodDlg", roadmap_lang_get ("Select your mood"), NULL, SSD_CONTAINER_TITLE);
	 moodDlg->context = (void *)callback;
	 exclusive_list = ssd_list_new ("list", SSD_MAX_SIZE, SSD_MAX_SIZE, inputtype_none, flags, NULL);

    ssd_list_resize ( exclusive_list, row_height );

    exclusive_labels[0] = (char *)roadmap_lang_get("wazer_gold");
    exclusive_values[0] = "wazer_gold";
    exclusive_icons[0] = "wazer_gold";

    exclusive_labels[1] = (char *)roadmap_lang_get("wazer_silver");
    exclusive_values[1] = "wazer_silver";
    exclusive_icons[1] = "wazer_silver";

    exclusive_labels[2] = (char *)roadmap_lang_get("wazer_bronze");
    exclusive_values[2] = "wazer_bronze";
    exclusive_icons[2] = "wazer_bronze";

    if (roadmap_mood_get_exclusive_moods() > 0){
       exclusive_callback = roadmap_exclusive_mood_call_back;
    }

    ssd_list_populate (exclusive_list, 3, (const char **)exclusive_labels, (const void **)exclusive_values, (const char **)exclusive_icons, NULL, exclusive_callback, NULL, FALSE);

    text = ssd_text_new ("Gold Mood Txt", roadmap_lang_get("Exclusive moods"), 14, SSD_END_ROW);
    ssd_widget_add(moodDlg, text);
    text = ssd_text_new ("Gold Mood Txt", roadmap_lang_get("(Available only to top weekly scoring wazers)"), 12, SSD_END_ROW|SSD_TEXT_NORMAL_FONT);
    ssd_widget_add(moodDlg, text);
    ssd_widget_add (moodDlg, exclusive_list);

    for (i = 0; i < (3 - roadmap_mood_get_exclusive_moods()); i++){
           SsdWidget row = ssd_list_get_row(exclusive_list, i);
           if (row){
              SsdWidget label = ssd_widget_get(row,"label");
              if (label)
                 ssd_text_set_color(label,"#999999");
           }
    }



    list = ssd_list_new ("list", SSD_MAX_SIZE, SSD_MAX_SIZE, inputtype_none, flags, NULL);
    exclusive_list->key_pressed = NULL;
    text = ssd_text_new ("Gold Mood Txt", roadmap_lang_get("Everyday moods"), 14, SSD_END_ROW);
    ssd_widget_add(moodDlg, text);
    text = ssd_text_new ("Gold Mood Txt", roadmap_lang_get("(Available to all)"), 12, SSD_END_ROW|SSD_TEXT_NORMAL_FONT);
    ssd_widget_add(moodDlg, text);
    ssd_widget_add (moodDlg, list);
    ssd_list_resize ( list, row_height );

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
    ssd_list_populate (list, count, (const char **)labels, (const void **)values, (const char **)icons, NULL, roadmap_mood_call_back, NULL, FALSE);

    exclusive_list->key_pressed = NULL;
    ssd_dialog_activate ("MoodDlg", NULL);
    ssd_dialog_draw ();

}

///////////////////////////////////////////////////////////////////////////////////////////
void roadmap_mood(void){

	roadmap_mood_dialog(NULL);
}
#endif //IPHONE
