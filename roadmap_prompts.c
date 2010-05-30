/* roadmap_prompts.c
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi Ben-Shoshan
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
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "roadmap.h"
#include "roadmap_main.h"
#include "roadmap_prompts.h"
#include "roadmap_messagebox.h"
#include "roadmap_config.h"
#include "roadmap_file.h"
#include "roadmap_lang.h"
#include "roadmap_path.h"
#include "roadmap_res.h"
#include "roadmap_res_download.h"
#include "roadmap_warning.h"
#include "Realtime/Realtime.h"
#include "websvc_trans/mkgmtime.h"
#include "websvc_trans/web_date_format.h"
#include "ssd/ssd_confirm_dialog.h"
#include "ssd/ssd_dialog.h"

#define MAX_PROMPT_SETS 30
#define MAX_PROMPTS_FILE 50
#define PROMPTS_DOWNLOAD_TIMER    (1000*60*4)

static const char *prompts_list[MAX_PROMPTS_FILE];

static const char *prompts_labels[MAX_PROMPT_SETS];

static const char *prompts_values[MAX_PROMPT_SETS];

static int prompt_set_count = 0;

static int num_prompts = 0;

static int num_downloaded;

static BOOL initialized = FALSE;

static RoadMapCallback PromptsNextLoginCb = NULL;

static RoadMapConfigDescriptor RoadMapConfigPromptName = ROADMAP_CONFIG_ITEM("Prompts", "Name");

static RoadMapConfigDescriptor RoadMapConfigPromptDownloadingLang = ROADMAP_CONFIG_ITEM("Prompts", "Downloading lang");

static RoadMapConfigDescriptor RoadMapConfigPromptQueuedLang = ROADMAP_CONFIG_ITEM("Prompts", "Queued lang");

static RoadMapConfigDescriptor RoadMapConfigPromptUpdateTime = ROADMAP_CONFIG_ITEM("Prompts", "Update time");


//////////////////////////////////////////////////////////////////
static void roadmap_prompts_init_params (void) {
   roadmap_config_declare ("user", &RoadMapConfigPromptName, "", NULL);

   roadmap_config_declare ("session", &RoadMapConfigPromptUpdateTime, "", NULL);

   roadmap_config_declare ("session", &RoadMapConfigPromptDownloadingLang, "", NULL);

   roadmap_config_declare ("session", &RoadMapConfigPromptQueuedLang, "", NULL);

   initialized = TRUE;
}

//////////////////////////////////////////////////////////////////
const char *roadmap_prompts_get_name (void) {
   if (!initialized)
      roadmap_prompts_init_params ();

   return roadmap_config_get (&RoadMapConfigPromptName);
}

//////////////////////////////////////////////////////////////////
void roadmap_prompts_set_name (const char *name) {
   if (!initialized)
      roadmap_prompts_init_params ();

   roadmap_config_set (&RoadMapConfigPromptName, name);
}

//////////////////////////////////////////////////////////////////
static const char *roadmap_prompts_get_update_time (void) {
   if (!initialized)
      roadmap_prompts_init_params ();

   return roadmap_config_get (&RoadMapConfigPromptUpdateTime);
}

//////////////////////////////////////////////////////////////////
void roadmap_prompts_set_update_time (const char *update_time) {

   if (!initialized)
      roadmap_prompts_init_params ();

   roadmap_config_set (&RoadMapConfigPromptUpdateTime, update_time);
}

//////////////////////////////////////////////////////////////////
static const char *roadmap_prompts_get_downloading_lang_name (void) {

   if (!initialized)
      roadmap_prompts_init_params ();

   return roadmap_config_get (&RoadMapConfigPromptDownloadingLang);
}

//////////////////////////////////////////////////////////////////
static void roadmap_prompts_set_downloading_lang_name (const char *lang) {

   if (!initialized)
      roadmap_prompts_init_params ();

   roadmap_config_set (&RoadMapConfigPromptDownloadingLang, lang);
}


//////////////////////////////////////////////////////////////////
static const char *roadmap_prompts_get_queued_lang (void) {

   if (!initialized)
      roadmap_prompts_init_params ();

   return roadmap_config_get (&RoadMapConfigPromptQueuedLang);
}

//////////////////////////////////////////////////////////////////
static void roadmap_prompts_set_queued_lang (const char *lang) {

   if (!initialized)
      roadmap_prompts_init_params ();

   roadmap_config_set (&RoadMapConfigPromptQueuedLang, lang);
}


//////////////////////////////////////////////////////////////////
static BOOL prompts_downloads_warning_fn (char* dest_string) {
   if (num_prompts == 0){
      snprintf (dest_string, ROADMAP_WARNING_MAX_LEN, " ");
      return FALSE;
   }

   snprintf (dest_string, ROADMAP_WARNING_MAX_LEN, "%s: %d%%%%", roadmap_lang_get (
                  "Downloading new prompts"), num_downloaded * 100 / num_prompts);
   return TRUE;
}

//////////////////////////////////////////////////////////////////
static void roadmap_prompts_download_watchdog_timer(void){
   const char * queued_lang = roadmap_prompts_get_queued_lang();

   roadmap_log (ROADMAP_ERROR,"roadmap_prompts_download_watchdog_timer - Timer reached. Downloading lang %s (downloaded %d of %d)", roadmap_prompts_get_downloading_lang_name(), num_downloaded, num_prompts );

   roadmap_messagebox("Oops", "Downloading new voice files failed. Please try again later");
   roadmap_warning_unregister (prompts_downloads_warning_fn);
   roadmap_prompts_set_downloading_lang_name("");
   if (queued_lang[0] != 0){
      roadmap_prompts_set_queued_lang("");
      roadmap_prompts_download(queued_lang);
   }
   roadmap_main_remove_periodic(roadmap_prompts_download_watchdog_timer);
}

//////////////////////////////////////////////////////////////////
static int load_prompt_list (void) {
   char *p;
   FILE *file;
   char line[1024];
   char file_name[20];
   char *name;
   char *value;
   const char *path;
#ifdef IPHONE
   path = roadmap_path_bundle();
#elif (!defined(J2ME))
   path = roadmap_path_user ();
#else
   path = NULL;
#endif


   if (num_prompts != 0)
      return 1;

   sprintf (file_name, "prompt_list.txt");
   file = roadmap_file_fopen (path, file_name, "sr");
   if (file == NULL) {
      roadmap_log (ROADMAP_ERROR,"prompt_list.txt not found." );
      return 0;
   }

   while (!feof(file)) {

      /* Read the next line, skip empty lines and comments. */

      if (fgets (line, sizeof(line), file) == NULL) break;

      p = roadmap_config_extract_data (line, sizeof(line));
      if (p == NULL) continue;

      /* Decode the line (name= value). */

      name = p;

      p = roadmap_config_skip_spaces (p);
      value = p;

      p = roadmap_config_skip_until (p, 0);
      *p = 0;

      prompts_list[num_prompts] = strdup (value);
      num_prompts++;
   }

   fclose (file);

   return 1;
}

//////////////////////////////////////////////////////////////////
void on_loaded_prompt_file (const char* res_name, int success, void *context, char *last_modified) {
   if (success) {
      num_downloaded++;
      if (num_downloaded == num_prompts){
		 const char * queued_lang;
         roadmap_main_remove_periodic(roadmap_prompts_download_watchdog_timer);
         queued_lang = roadmap_prompts_get_queued_lang();
         roadmap_warning_unregister (prompts_downloads_warning_fn);
         roadmap_prompts_set_downloading_lang_name("");
         if (queued_lang[0] != 0){
            roadmap_prompts_set_queued_lang("");
            roadmap_prompts_download(queued_lang);
         }
      }
   }
}

//////////////////////////////////////////////////////////////////
void roadmap_prompts_download (const char *lang) {
   int i;
   const char *downloading_lang;

   downloading_lang = roadmap_prompts_get_downloading_lang_name();
   if (downloading_lang[0] != 0){
      // we care currently dowloading a lang
      if (!strcmp(downloading_lang, lang)){
         //we are now downloading this lang
         return;

      }else{
         const char * queued_lang = roadmap_prompts_get_queued_lang();
         if (queued_lang[0] == 0){
            roadmap_prompts_set_queued_lang(lang);
         }
         else{
            //todo add messagebox
         }
         return;
      }
   }


   load_prompt_list ();
   num_downloaded = 0;
   roadmap_main_set_periodic(PROMPTS_DOWNLOAD_TIMER, roadmap_prompts_download_watchdog_timer);
   roadmap_warning_register (prompts_downloads_warning_fn, "prompts");
   roadmap_prompts_set_downloading_lang_name(lang);
   for (i = 0; i < num_prompts; i++) {
      roadmap_res_download (RES_DOWNLOAD_SOUND, prompts_list[i], NULL, lang, FALSE, 0,
                     on_loaded_prompt_file, NULL);
   }
}


//////////////////////////////////////////////////////////////////
static int roadmap_prompts_conf_load (const char *path) {

   char *p;
   FILE *file;
   char line[1024];
   char file_name[20];
   char *name;
   char *value;

   prompt_set_count = 0;

   sprintf (file_name, "prompts.conf");
   file = roadmap_file_fopen (path, file_name, "sr");
   if (file == NULL) {
      roadmap_log (ROADMAP_ERROR,"prompts.conf not found." );
      return 0;
   }

   while (!feof(file)) {

      /* Read the next line, skip empty lines and comments. */

      if (fgets (line, sizeof(line), file) == NULL) break;

      p = roadmap_config_extract_data (line, sizeof(line));
      if (p == NULL) continue;

      name = p;

      p = roadmap_config_skip_until (p, ',');
      if (*p != ',') continue;
      *(p++) = 0;

      p = roadmap_config_skip_spaces (p);
      value = p;

      p = roadmap_config_skip_until (p, 0);
      *p = 0;

      prompts_labels[prompt_set_count] = strdup (value);
      prompts_values[prompt_set_count] = strdup (name);
      prompt_set_count++;
   }

   fclose (file);

   return 1;
}

//////////////////////////////////////////////////////////////////
static BOOL prompt_set_exist(const char *value){
   int i;
   if (!value)
       return FALSE;

    for (i = 0; i < prompt_set_count; i++) {
       if (prompts_values[i] && !strcmp (prompts_values[i], value))
          return TRUE;
    }
    return FALSE;
}

//////////////////////////////////////////////////////////////////
static void on_download_lang_confirm(int exit_code, void *context){
   if (exit_code == dec_yes){
      const char *lang = (const char *)context;
      roadmap_prompts_download(lang);
   }

   if (PromptsNextLoginCb) {
      PromptsNextLoginCb ();
      PromptsNextLoginCb = NULL;
   }

}

//////////////////////////////////////////////////////////////////
static void on_conf_file_downloaded (const char* res_name, int success, void *context, char *last_modified) {

   const char *system_prompts;
   if (success){
      if (last_modified && *last_modified)
         roadmap_prompts_set_update_time(last_modified);

      roadmap_prompts_conf_load (roadmap_path_downloads ());
   }

   system_prompts=roadmap_prompts_get_name();
   if (!prompt_set_exist(system_prompts)){
      roadmap_log (ROADMAP_ERROR,"Prompt %s is not defined, switching to english", system_prompts );
      roadmap_prompts_set_name("eng");
      system_prompts=roadmap_prompts_get_name();
   }
   else if (!roadmap_prompts_exist(system_prompts)){
      char msg[256];
      snprintf(msg, sizeof(msg),"%s %s, %s", roadmap_lang_get("Prompt set"), system_prompts, roadmap_lang_get("is not installed on your device, Do you want to download prompt files?") );
      ssd_confirm_dialog("", msg, FALSE, on_download_lang_confirm, (void *)roadmap_prompts_get_prompt_value_from_name(system_prompts));
      return;
   }

}

//////////////////////////////////////////////////////////////////
static void download_conf_file () {
   const char* last_save_time = roadmap_prompts_get_update_time();
   time_t update_time;

   if (last_save_time[0] == 0) {
      update_time = 0;
   }
   else {
      update_time = WDF_TimeFromModifiedSince(last_save_time);
    }
   roadmap_res_download (RES_DOWNLOAD_CONFIFG, "prompts.conf", NULL, "", TRUE, update_time,
                  on_conf_file_downloaded, NULL);
}
//////////////////////////////////////////////////////////////////
int roadmap_prompts_get_count (void) {
   return prompt_set_count;
}

//////////////////////////////////////////////////////////////////
const void **roadmap_prompts_get_values (void) {
   return (const void **) &prompts_values[0];
}

//////////////////////////////////////////////////////////////////
const char **roadmap_prompts_get_labels (void) {
   return (const char **) &prompts_labels[0];
}

//////////////////////////////////////////////////////////////////
const void *roadmap_prompts_get_prompt_value (const char *value) {
   int i;

   if (!value)
      return NULL;

   for (i = 0; i < prompt_set_count; i++) {
      if (prompts_values[i] && !strcmp (prompts_values[i], value))
         return (const void *) prompts_values[i];
   }
   return value;
}

//////////////////////////////////////////////////////////////////
const void *roadmap_prompts_get_prompt_value_from_name (const char *name) {
   int i;
   if (!name)
      return NULL;

   for (i = 0; i < prompt_set_count; i++) {
      if (prompts_labels[i] && !strcmp (prompts_labels[i], name))
         return (const void *) prompts_values[i];
   }
   return name;
}

//////////////////////////////////////////////////////////////////
const char *roadmap_prompts_get_label (const char *value) {
   int i;
   if (!value)
      return NULL;

   for (i = 0; i < prompt_set_count; i++) {
      if (prompts_values[i] && !strcmp (prompts_values[i], value))
         return (const char *) prompts_labels[i];
   }
   return value;
}

//////////////////////////////////////////////////////////////////
void roadmap_prompts_login_cb(void){
   const char *last_download;

   download_conf_file ();

   last_download = roadmap_prompts_get_downloading_lang_name();
   if(*last_download){
      const char *name=strdup(last_download);
      roadmap_log (ROADMAP_ERROR,"Downloading of lang %s did not complete the last run, resuming download", last_download );
      roadmap_prompts_set_downloading_lang_name("");
      roadmap_prompts_download(name);

   }

   if (PromptsNextLoginCb) {
      PromptsNextLoginCb ();
      PromptsNextLoginCb = NULL;
   }

}


//////////////////////////////////////////////////////////////////
void roadmap_prompts_init (void) {
   const char *prompt;

   roadmap_prompts_init_params();

   prompt = roadmap_prompts_get_name ();
   if (prompt[0] == 0) {
      roadmap_prompts_set_name (roadmap_lang_get_system_lang ());
   }
   roadmap_prompts_conf_load (roadmap_path_downloads ());

   PromptsNextLoginCb = Realtime_NotifyOnLogin (roadmap_prompts_login_cb);
}

//////////////////////////////////////////////////////////////////
BOOL roadmap_prompts_exist (const char *name) {
   BOOL exist;
   char path[256];
   roadmap_path_format (path, sizeof (path), roadmap_path_downloads (), "sound");
   roadmap_path_format (path, sizeof (path), path, name);
#ifdef ANDROID
   exist = roadmap_file_exists (path, "click.bin");
#else
   exist = roadmap_file_exists (path, "click.mp3");
#endif
   return exist;
}
