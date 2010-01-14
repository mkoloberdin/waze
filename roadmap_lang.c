/* roadmap_lang.c - i18n
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
 *   See roadmap_lang.h.
 */

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include "websvc_trans/mkgmtime.h"
#include "roadmap.h"
#include "roadmap_main.h"
#include "roadmap_path.h"
#include "roadmap_file.h"
#include "roadmap_config.h"
#include "roadmap_hash.h"
#include "roadmap_lang.h"
#include "Realtime/Realtime.h"
#include "roadmap_res_download.h"
#include "websvc_trans/string_parser.h"
#include "websvc_trans/web_date_format.h"

#define INITIAL_ITEMS_SIZE 50
#define MAX_LANGUAGES 100
const char *lang_labels[MAX_LANGUAGES];
const char *lang_values[MAX_LANGUAGES];
int languages_count= 0;
struct RoadMapLangItem {

   const char *name;
   const char *value;
};

static RoadMapCallback LangNextLoginCb = NULL;

static BOOL initialized = FALSE;
static struct RoadMapLangItem *RoadMapLangItems;
static int RoadMapLangSize;
static int RoadMapLangCount;
static RoadMapHash *RoadMapLangHash;
static int RoadMapLangLoaded = 0;
static int RoadMapLangRTL = 0;
static RoadMapConfigDescriptor RoadMapConfigSystemLanguage =
                        ROADMAP_CONFIG_ITEM("System", "Language");

static RoadMapConfigDescriptor RoadMapConfigDefaultLanguage =
                        ROADMAP_CONFIG_ITEM("System", "Default Language");

static RoadMapConfigDescriptor RoadMapConfigLangUpdateTime =
                        ROADMAP_CONFIG_ITEM("Lang", "Update time");

//////////////////////////////////////////////////////////////////
void roadmap_lang_initialize_params(void){
   roadmap_config_declare
         ("user", &RoadMapConfigSystemLanguage, "default", NULL);

   roadmap_config_declare
         ("preferences", &RoadMapConfigDefaultLanguage, "eng", NULL);

   roadmap_config_declare
         ("session", &RoadMapConfigLangUpdateTime, "", NULL);
}

//////////////////////////////////////////////////////////////////
const char *roadmap_lang_get_system_lang(){
   const char *lang = roadmap_config_get (&RoadMapConfigSystemLanguage);

   if (!initialized)
      roadmap_lang_initialize_params();

   if (!strcmp(lang,"default"))
      return roadmap_lang_get_default_lang();
   else
      return lang;
}

//////////////////////////////////////////////////////////////////
const char *roadmap_lang_get_user_lang(){
   const char *lang = roadmap_config_get (&RoadMapConfigSystemLanguage);

   if (!initialized)
      roadmap_lang_initialize_params();

   if (!strcmp(lang,"default"))
      return "";
   else
      return lang;
}
//////////////////////////////////////////////////////////////////
void roadmap_lang_set_system_lang(const char *lang){
   if (!initialized)
      roadmap_lang_initialize_params();
   roadmap_lang_download_lang_file( lang, NULL );
   roadmap_config_set(&RoadMapConfigSystemLanguage, lang);
   roadmap_config_save(TRUE);
}

//////////////////////////////////////////////////////////////////
const char *roadmap_lang_get_default_lang(){

   if (!initialized)
      roadmap_lang_initialize_params();

   return roadmap_config_get (&RoadMapConfigDefaultLanguage);
}

//////////////////////////////////////////////////////////////////
void roadmap_lang_set_default_lang(const char *lang){
   if (!initialized)
      roadmap_lang_initialize_params();

   roadmap_config_set(&RoadMapConfigDefaultLanguage, lang);
   roadmap_config_save(TRUE);
}


//////////////////////////////////////////////////////////////////
const char *roadmap_lang_get_lang_file_update_time(const char *lang_value){

   RoadMapConfigDescriptor descriptor;

   descriptor.category = lang_value;
   descriptor.name = "Update time";
   roadmap_config_declare("session",&descriptor, "", NULL);

   return roadmap_config_get (&descriptor);
}

//////////////////////////////////////////////////////////////////
void roadmap_lang_set_lang_file_update_time(char *lang_value, char *update_time){

   RoadMapConfigDescriptor descriptor;

   descriptor.category = lang_value;
   descriptor.name = "Update time";
   roadmap_config_declare("session",&descriptor, "", NULL);

   roadmap_config_set (&descriptor, update_time);
   roadmap_config_save(FALSE);
}

//////////////////////////////////////////////////////////////////
const char *roadmap_lang_get_update_time(void){
   if (!initialized)
      roadmap_lang_initialize_params();

   return roadmap_config_get (&RoadMapConfigLangUpdateTime);
}

//////////////////////////////////////////////////////////////////
void roadmap_lang_set_update_time(const char *update_time){
   if (!initialized)
        roadmap_lang_initialize_params();
   roadmap_config_set(&RoadMapConfigLangUpdateTime, update_time);
}


static void roadmap_lang_allocate (void) {

   if (RoadMapLangSize == 0) {
      RoadMapLangSize = INITIAL_ITEMS_SIZE;
      RoadMapLangItems = calloc(RoadMapLangSize, sizeof(struct RoadMapLangItem));
      RoadMapLangHash = roadmap_hash_new ("lang_hash", RoadMapLangSize);
   } else {
      RoadMapLangSize *= 2;
      RoadMapLangItems =
         realloc(RoadMapLangItems,
                 RoadMapLangSize * sizeof(struct RoadMapLangItem));
      roadmap_hash_resize (RoadMapLangHash, RoadMapLangSize);
   }

   if (RoadMapLangItems == NULL) {
      roadmap_log (ROADMAP_FATAL, "No memory.");
   }
}


static void roadmap_lang_new_item (const char *name, const char *value) {

   int hash = roadmap_hash_string (name);

   if (RoadMapLangCount == RoadMapLangSize) {
      roadmap_lang_allocate ();
   }

   RoadMapLangItems[RoadMapLangCount].name  = name;
   RoadMapLangItems[RoadMapLangCount].value = value;

   roadmap_hash_add (RoadMapLangHash, hash, RoadMapLangCount);

   RoadMapLangCount++;
}


static int roadmap_lang_load (const char *path) {

   char *p;
   FILE *file;
   char  line[1024];
   char file_name[20];

   char *name;
   char *value;

   sprintf(file_name, "lang.%s", roadmap_lang_get_system_lang());
   file = roadmap_file_fopen (path, file_name, "sr");
   if (file == NULL) return 0;

   while (!feof(file)) {

        /* Read the next line, skip empty lines and comments. */

        if (fgets (line, sizeof(line), file) == NULL) break;

        p = roadmap_config_extract_data (line, sizeof(line));
        if (p == NULL) continue;

        /* Decode the line (name= value). */

        name = p;

        p = roadmap_config_skip_until (p, '=');
        if (*p != '=') continue;
        *(p++) = 0;

        p = roadmap_config_skip_spaces (p);
        value = p;

        p = roadmap_config_skip_until (p, 0);
        *p = 0;

        name  = strdup (name);
        value = strdup (value);

        roadmap_lang_new_item (name, value);
    }
    fclose (file);

    return 1;
}

void roadmap_lang_reload(void){
   const char *p;

   RoadMapLangCount = 0;
   RoadMapLangSize = 0;
   roadmap_hash_free(RoadMapLangHash);
   roadmap_lang_allocate ();
#ifndef J2ME
   p = roadmap_path_user ();
#else
   p = NULL;
#endif
   RoadMapLangLoaded = roadmap_lang_load (p);
   if (!RoadMapLangLoaded){
      p = roadmap_path_downloads();
      RoadMapLangLoaded = roadmap_lang_load (p);
   }

   RoadMapLangRTL = (strcasecmp(roadmap_lang_get ("RTL"), "Yes") == 0);
}



static int roadmap_lang_conf_load (const char *path) {

    char *p;
   FILE *file;
   char  line[1024];
   char file_name[20];
   char *name;
   char *value;

   languages_count = 0;

   sprintf(file_name, "lang.conf");
   file = roadmap_file_fopen (path, file_name, "sr");
   if (file == NULL){
      roadmap_log (ROADMAP_ERROR, "lang.conf not found.");
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

           lang_labels[languages_count] = strdup (value);
           lang_values[languages_count] = strdup (name);
           languages_count++;
    }

    fclose (file);

    return 1;
}

void on_lang_file_downloaded (const char* res_name, int success, void *context, char *last_modified){
   RoadMapCallback callback = (RoadMapCallback) context;

   char *lang_value = strrchr  (res_name, '.');
   if (!lang_value){
      if (callback)
        (*callback)();
      return;
   }
   lang_value++;
   if (success)
      roadmap_lang_set_lang_file_update_time(lang_value, last_modified);
   if (callback)
            (*callback)();
}

void roadmap_lang_download_lang_file(const char *lang, RoadMapCallback callback){
   char file_name[256];
   time_t update_time;
   const char* last_save_time = roadmap_lang_get_lang_file_update_time(lang);

   sprintf(file_name, "lang.%s",lang);


   if (last_save_time[0] == 0){
      update_time = 0;
   }
   else{
      update_time = WDF_TimeFromModifiedSince(last_save_time);
   }

   roadmap_res_download (RES_DOWNLOAD_LANG, file_name, "", TRUE, update_time, on_lang_file_downloaded, (void *)callback);
}

void download_lang_files(void){
  int i;
   for (i = 0; i < languages_count; i++){
      if (strcmp(lang_values[i], roadmap_lang_get_system_lang()))
         roadmap_lang_download_lang_file(lang_values[i], NULL);
   }
}

void on_conf_file_downloaded (const char* res_name, int success, void *context, char *last_modified){
   RoadMapCallback callback = (RoadMapCallback) context;
   if (success){ //we download a new conf file. download lang files
      if (last_modified && *last_modified)
         roadmap_lang_set_update_time(last_modified);
      roadmap_lang_conf_load(roadmap_path_downloads());
      if (callback)
         (*callback)();
      download_lang_files();

   }
   else{
      roadmap_lang_conf_load(roadmap_path_downloads());
      if (callback)
         (*callback)();
   }

}

void roadmap_lang_download_conf_file(RoadMapCallback callback){
   time_t update_time;
   static BOOL run_once = FALSE;
   const char* last_save_time = roadmap_lang_get_update_time();

   if (run_once)
      return;
   run_once = TRUE;

   if (last_save_time[0] == 0){
      update_time = 0;
   }
   else{
      update_time = WDF_TimeFromModifiedSince(last_save_time);
   }
   roadmap_res_download (RES_DOWNLOAD_CONFIFG, "lang.conf", "", TRUE, update_time, on_conf_file_downloaded, callback);
}

void roadmap_lang_login_cb(void){
   if (LangNextLoginCb) {
      LangNextLoginCb ();
      LangNextLoginCb = NULL;
   }
   roadmap_lang_download_lang_file(roadmap_lang_get_system_lang(), NULL);
   roadmap_lang_download_conf_file(NULL);
}

void roadmap_lang_initialize (void) {

   const char *p;
   initialized = TRUE;

   roadmap_lang_initialize_params();

   roadmap_lang_allocate ();

   lang_labels[0] = "English";
   lang_values[0] = "eng";

#ifndef J2ME
   p = roadmap_path_user ();
#else
   p = NULL;
#endif

   LangNextLoginCb = Realtime_NotifyOnLogin (roadmap_lang_login_cb);


   roadmap_lang_conf_load(roadmap_path_downloads());

   RoadMapLangLoaded = roadmap_lang_load (p);
   if (!RoadMapLangLoaded){
      p = roadmap_path_downloads();
      RoadMapLangLoaded = roadmap_lang_load (p);
   }
   RoadMapLangRTL = (strcasecmp(roadmap_lang_get ("RTL"), "Yes") == 0);
}


const char* roadmap_lang_get (const char *name) {

   int hash;
   int i;

   if (!RoadMapLangLoaded) return name;

   hash = roadmap_hash_string (name);

   for (i = roadmap_hash_get_first (RoadMapLangHash, hash);
        i >= 0;
        i = roadmap_hash_get_next (RoadMapLangHash, i)) {

      if (!strcmp(name, RoadMapLangItems[i].name)) {

         return RoadMapLangItems[i].value;
      }
   }

   return name;
}


int roadmap_lang_rtl (void) {
   return RoadMapLangRTL;
}


int roadmap_lang_get_available_langs_count(void){
   return languages_count;
}

const void **roadmap_lang_get_available_langs_values(void){
   return (const void **)&lang_values[0];
}

const char **roadmap_lang_get_available_langs_labels(void){
   return (const char **)&lang_labels[0];
}

const void *roadmap_lang_get_lang_value(const char *value){
   int i;
   for (i = 0; i < languages_count; i++){
      if (!strcmp(lang_values[i], value))
         return (const void *)lang_values[i];
   }
   return value;
}

const void *roadmap_lang_get_label(const char *value){
   int i;
   for (i = 0; i < languages_count; i++){
      if (!strcmp(lang_values[i], value))
         return (const void *)lang_labels[i];
   }
   return value;
}

