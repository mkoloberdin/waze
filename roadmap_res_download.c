/* roadmap_res_download.c
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

#include "roadmap.h"
#include "roadmap_res_download.h"
#include "roadmap_config.h"
#include "roadmap_file.h"
#include "roadmap_res.h"
#include "roadmap_httpcopy_async.h"
#include "roadmap_path.h"
#include "roadmap_geo_config.h"

static RoadMapConfigDescriptor RoadMapConfigDownloadImageUrl =
      ROADMAP_CONFIG_ITEM("Download", "Images");

static RoadMapConfigDescriptor RoadMapConfigDownloadSoundUrl =
      ROADMAP_CONFIG_ITEM("Download", "Sound");

static RoadMapConfigDescriptor RoadMapConfigDownloadConfigUrl =
      ROADMAP_CONFIG_ITEM("Download", "Config");

static RoadMapConfigDescriptor RoadMapConfigDownloadLangUrl =
      ROADMAP_CONFIG_ITEM("Download", "Langs");

static RoadMapConfigDescriptor RoadMapConfigDownloadImageUrl_Ver =
      ROADMAP_CONFIG_ITEM("Download", "Images_Ver");

static RoadMapConfigDescriptor RoadMapConfigDownloadSoundUrl_Ver =
      ROADMAP_CONFIG_ITEM("Download", "Sound_Ver");

static RoadMapConfigDescriptor RoadMapConfigDownloadConfigUrl_Ver =
      ROADMAP_CONFIG_ITEM("Download", "Config_Ver");

static RoadMapConfigDescriptor RoadMapConfigDownloadLangUrl_Ver =
      ROADMAP_CONFIG_ITEM("Download", "Langs_Ver");


#define RES_DOWNLOAD_MAX_QUEUE 100

typedef struct {
   char *name;
   int type;
   char *lang;
   BOOL override;
   time_t update_time;
   RoadMapResDownloadCallback on_loaded_cb;
   void *context;
} ResData;

static ResData RequestQueue[RES_DOWNLOAD_MAX_QUEUE];
static int FirstItem = -1;
static int ItemsCount = 0;
static BOOL Initialized = FALSE;
static int downloading = FALSE;

typedef struct {
   char* res_path;
   char* data;
   size_t data_size;
   ResData res_data;
} DownloadContext;

static int download_size_callback (void *context_cb, size_t size);
static void download_progress_callback (void *context_cb, char *data, size_t size);
static void download_error_callback (void *context_cb,
               int connection_failure,
               const char *format,
               ...);
static void download_done_callback (void *context_cb, char *last_modified);

static RoadMapHttpAsyncCallbacks gHttpAsyncCallbacks = { download_size_callback,
      download_progress_callback, download_error_callback, download_done_callback };

//////////////////////////////////////////////////////////////////
void roadmap_res_download_init (void) {
   
   roadmap_config_declare ("preferences", &RoadMapConfigDownloadImageUrl, "", NULL);

   roadmap_config_declare ("preferences", &RoadMapConfigDownloadSoundUrl, "", NULL);

   roadmap_config_declare ("preferences", &RoadMapConfigDownloadConfigUrl, "", NULL);
   
   roadmap_config_declare ("preferences", &RoadMapConfigDownloadLangUrl, "", NULL);
   
   roadmap_config_declare ("preferences", &RoadMapConfigDownloadImageUrl_Ver, "", NULL);

   roadmap_config_declare ("preferences", &RoadMapConfigDownloadSoundUrl_Ver, "", NULL);

   roadmap_config_declare ("preferences", &RoadMapConfigDownloadConfigUrl_Ver, "", NULL);
   
   roadmap_config_declare ("preferences", &RoadMapConfigDownloadLangUrl_Ver, "", NULL);
   
   
   Initialized = TRUE;
}

//////////////////////////////////////////////////////////////////
void roadmap_res_download_term (void) {
   
}
//////////////////////////////////////////////////////////////////
static char* get_images_url_ver (void) {
   return (char*) roadmap_config_get (&RoadMapConfigDownloadImageUrl_Ver);
}

//////////////////////////////////////////////////////////////////
static char* get_sound_url_ver (void) {
   return (char*) roadmap_config_get (&RoadMapConfigDownloadSoundUrl_Ver);
}

//////////////////////////////////////////////////////////////////
static char* get_config_url_ver (void) {
   return (char*) roadmap_config_get (&RoadMapConfigDownloadConfigUrl_Ver);
}

//////////////////////////////////////////////////////////////////
static char* get_langs_url_ver (void) {
   return (char*) roadmap_config_get (&RoadMapConfigDownloadLangUrl_Ver);
}

//////////////////////////////////////////////////////////////////
static char* get_images_url_prefix (void) {
   return (char*) roadmap_config_get (&RoadMapConfigDownloadImageUrl);
}

//////////////////////////////////////////////////////////////////
static char* get_sound_url_prefix (void) {
   return (char*) roadmap_config_get (&RoadMapConfigDownloadSoundUrl);
}

//////////////////////////////////////////////////////////////////
static char* get_config_url_prefix (void) {
   return (char*) roadmap_config_get (&RoadMapConfigDownloadConfigUrl);
}

//////////////////////////////////////////////////////////////////
static char* get_langs_url_prefix (void) {
   return (char*) roadmap_config_get (&RoadMapConfigDownloadLangUrl);
}

//////////////////////////////////////////////////////////////////
static char* get_download_url (int type, const char *lang, const char* name) {
   char* url;
   const char* url_prefix;
   const char* url_ver;
   
   switch (type) {
      case RES_DOWNLOAD_IMAGE:
         url_prefix = get_images_url_prefix ();
         url_ver = get_images_url_ver();
         break;
      case RES_DOWNLOAD_SOUND:
         url_prefix = get_sound_url_prefix ();
         url_ver = get_sound_url_ver();
         break;
      case RES_DOWNLOAD_LANG:
         url_prefix = get_langs_url_prefix ();
         url_ver = get_langs_url_ver();
         break;
      default:
         url_prefix = get_config_url_prefix ();
         url_ver = get_config_url_ver();
         break;
   }

   if (!*url_prefix){
      roadmap_log (ROADMAP_ERROR,"Failed to download resource. URL prefix is empty  type=%d, lang=%s, name=%s" ,type, lang, name );
      return NULL;
   }
   // Size of prefix +  size of image id + '\0'
   url = malloc (strlen (url_prefix) + strlen(url_ver) + strlen (name) + +strlen (lang) + +strlen (
                  roadmap_geo_config_get_server_id ()) + 5);
   
   roadmap_check_allocated( url );
   
   strcpy (url, url_prefix);
   
   if (url_ver[0] != 0){
      strcat (url, url_ver);
      strcat (url, "/");
   }
   

   if (lang && (lang[0] != 0)) {
      strcat (url, lang);
      strcat (url, "/");
   }

   if (type == RES_DOWNLOAD_CONFIFG) {
      strcat (url, roadmap_geo_config_get_server_id ());
      strcat (url, "/");
   }
   strcat (url, name);
   
   return url;
}
//////////////////////////////////////////////////////////////////
int ResDataQueue_Size () {
   return ItemsCount;
}

//////////////////////////////////////////////////////////////////
BOOL ResDataQueue_IsEmpty (void) {
   return ItemsCount == 0;
}

//////////////////////////////////////////////////////////////////
BOOL ResDataQueue_IsFull (void) {
   return ItemsCount == RES_DOWNLOAD_MAX_QUEUE;
}

//////////////////////////////////////////////////////////////////
static int IncrementIndex (int i) {
   i++;
   
   if (i < RES_DOWNLOAD_MAX_QUEUE)
      return i;
   
   return 0;
}

//////////////////////////////////////////////////////////////////
static int AllocCell () {
   int iNextFreeCell;
   
   if (RES_DOWNLOAD_MAX_QUEUE == ItemsCount)
      return -1;

   if (-1 == FirstItem) {
      FirstItem = 0;
      ItemsCount = 1;
      return 0;
   }

   iNextFreeCell = (FirstItem + ItemsCount);
   ItemsCount++;
   
   if (iNextFreeCell < RES_DOWNLOAD_MAX_QUEUE)
      return iNextFreeCell;
   
   return (iNextFreeCell - RES_DOWNLOAD_MAX_QUEUE);
}

//////////////////////////////////////////////////////////////////
static void ResData_Init (ResData *item) {
   item->on_loaded_cb = NULL;
   item->context = NULL;
   item->name = NULL;
   item->type = -1;
}

//////////////////////////////////////////////////////////////////
static ResData * AllocResData () {
   ResData * pCell = NULL;
   int iCell = AllocCell ();
   if (-1 == iCell)
      return NULL;

   pCell = RequestQueue + iCell;
   ResData_Init (pCell);
   
   return pCell;
}

//////////////////////////////////////////////////////////////////
static void ResData_Free (ResData *this) {
   free (this->name);
   if (this->lang)
      free (this->lang);
   ResData_Init (this);
}

//////////////////////////////////////////////////////////////////
static BOOL PopOldest (ResData *pRD) {
   ResData *pCell;
   
   if (!ItemsCount || (-1 == FirstItem)) {
      if (pRD)
         ResData_Init (pRD);
      return FALSE;
   }

   pCell = RequestQueue + FirstItem;
   
   if (pRD) {
      // Copy item data:
      (*pRD) = (*pCell);
      // Detach item from queue:
      ResData_Init (pCell);
   }
   else
      // Item is NOT being copied; Release item resources:
      ResData_Free (pCell);
   
   if (1 == ItemsCount) {
      ItemsCount = 0;
      FirstItem = -1;
   }
   else {
      ItemsCount--;
      FirstItem = IncrementIndex (FirstItem);
   }
   
   return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void ResDataQueue_Push (ResData *this) {
   ResData * p;
   
   if (ResDataQueue_IsFull ())
      PopOldest (NULL);
   
   p = AllocResData ();
   
   p->name = this->name;
   p->type = this->type;
   p->override = this->override;
   p->update_time = this->update_time;
   p->context = this->context;
   if (this->lang)
      p->lang = this->lang;
   else
      p->lang = NULL;
   p->on_loaded_cb = this->on_loaded_cb;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ResDataQueue_Pop (ResData *this) {
   return PopOldest (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static char * get_sound_output_path (const char *lang, const char *file_name) {
   char path[512];
   char *res_path;
   if (lang && (lang[0] != 0)) {
      roadmap_path_format (path, sizeof (path), roadmap_path_downloads (), "sound");
      roadmap_path_format (path, sizeof (path), path, lang);
      res_path = roadmap_path_join (path, file_name);
   }
   else {
      res_path = roadmap_path_join (roadmap_path_downloads (), file_name);
   }
   
   return res_path;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static char * get_images_output_path (const char *file_name) {
   char path[512];
   char *res_path;
   roadmap_path_format (path, sizeof (path), roadmap_path_downloads (), "skins");
   roadmap_path_format (path, sizeof (path), path, "default");
   res_path = roadmap_path_join (path, file_name);
   
   return res_path;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static char * get_downloads_output_path (const char *file_name) {
   return roadmap_path_join (roadmap_path_downloads (), file_name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void roadmap_download_start (void) {
   ResData resData;
   BOOL exist;
   char *url;
   char *res_file, *res_path;
   DownloadContext* context;
   
   if (downloading) {
      return;
   }

   exist = ResDataQueue_Pop (&resData);
   if (!exist) {
      return;
   }
   downloading = TRUE;
   // Target file name from the image id
   res_file = malloc (strlen (resData.name) + 5);
   strcpy (res_file, resData.name);
   if (resData.type == RES_DOWNLOAD_IMAGE) {
      strcat (res_file, ".bin");
      res_path = get_images_output_path (res_file);
   }
   else if (resData.type == RES_DOWNLOAD_SOUND) {
#ifdef ANDROID
      strcat (res_file, ".bin");
#else      
      strcat (res_file, ".mp3");
#endif      
      res_path = get_sound_output_path (resData.lang, res_file);
   }
   else if (resData.type == RES_DOWNLOAD_LANG) {
      res_path = get_downloads_output_path (res_file);
   }
   else {
      res_path = get_downloads_output_path (res_file);
   }

   if (roadmap_file_exists (NULL, res_path) && !resData.override) {
      if (resData.on_loaded_cb)
         resData.on_loaded_cb (resData.name, 1, resData.context,"");
      free (res_file);
      free (res_path);
      downloading = FALSE;
      roadmap_download_start ();
      return;
   }
   else {
      strcpy (res_file, resData.name);
      if (resData.type == RES_BITMAP)
         strcat (res_file, ".png");
      else if (resData.type == RES_SOUND)
         strcat (res_file, ".mp3");
      
      url = get_download_url (resData.type, resData.lang, res_file);
      if (!url){
         if (resData.on_loaded_cb)
               resData.on_loaded_cb (resData.name, 1, resData.context,"");
         free (res_file);
         free (res_path);
         downloading = FALSE;
         roadmap_download_start ();
         return;
      } 
      // Init the download process context
      context = malloc (sizeof(DownloadContext));
      context->res_path = res_path;
      context->res_data.name = resData.name;
      context->res_data.on_loaded_cb = resData.on_loaded_cb;
      context->res_data.type = resData.type;
      context->res_data.override = resData.override;
      context->res_data.update_time = resData.update_time;
      context->res_data.lang = resData.lang; 
      context->res_data.context = resData.context;
      context->data = NULL;
      roadmap_log (ROADMAP_DEBUG,"Downloading resource.  type=%d, name=%s, lang=%s" ,resData.type, resData.name, resData.lang );
      // Start the process
      roadmap_http_async_copy( &gHttpAsyncCallbacks, context, url ,resData.update_time );
     free( url );
   }

      free( res_file );

}

//////////////////////////////////////////////////////////////////
void roadmap_res_download (int type,
               const char*name,
               const char *lang,
               BOOL override,
               time_t update_time,
               RoadMapResDownloadCallback on_loaded,
               void *context) {
   ResData resData;
   
   if (!Initialized)
      roadmap_res_download_init ();
   
   if (ResDataQueue_IsEmpty ()) {
      resData.on_loaded_cb = on_loaded;
      resData.type = type;
      resData.name = strdup (name);
      resData.lang = strdup (lang);
      resData.override = override;
      resData.context = context;
      resData.update_time = update_time;
      ResDataQueue_Push (&resData);
      roadmap_download_start ();
      return;
   }
   else {
      resData.on_loaded_cb = on_loaded;
      resData.type = type;
      resData.name = strdup (name);
      resData.lang = strdup (lang);
      resData.override = override; 
      resData.update_time = update_time;
      resData.context = context;
      ResDataQueue_Push (&resData);
   }
   
}

//////////////////////////////////////////////////////////////////
static int download_size_callback (void *context_cb, size_t size) {
   DownloadContext* context = (DownloadContext*) context_cb;
   
   // Allocate data storage
   if (size > 0) {
      context->data = malloc (size);
      context->data_size = 0;
   }
   
   return size;
}

//////////////////////////////////////////////////////////////////
static void download_progress_callback (void *context_cb, char *data, size_t size) {
   DownloadContext* context = (DownloadContext*) context_cb;
   
   // Store data
   if (context->data) {
      memcpy (context->data + context->data_size, data, size);
      context->data_size += size;
   }
}

//////////////////////////////////////////////////////////////////
static void download_error_callback (void *context_cb,
               int connection_failure,
               const char *format,
               ...) {
   DownloadContext* context = (DownloadContext*) context_cb;
   va_list ap;
   char err_string[1024];
   
   // Load the arguments
   va_start( ap, format );
   vsnprintf (err_string, 1024, format, ap);
   va_end( ap );
   
   // Log it...
   if (strstr (err_string, " 304 ") == NULL) {
      roadmap_log (ROADMAP_WARNING,"Error dowloading %s - %s" ,context->res_path, err_string );
   }
   else{
      roadmap_log (ROADMAP_DEBUG,"%s was not downloaded error = %s" ,context->res_path, err_string );
   }
   if (context->res_data.on_loaded_cb)
      (*(context->res_data.on_loaded_cb))( context->res_data.name,0, context->res_data.context,"" );
   
   // Free allocated space
   if ( context->data )
   {
      free( context->data );
      context->data = NULL;
   }
   free( context->res_data.name );
   free( context->res_data.lang);
   free( context->res_path );
   free(context);
   downloading = FALSE;
   roadmap_download_start();
}

//////////////////////////////////////////////////////////////////
static void download_done_callback (void *context_cb, char *last_modified) {
   DownloadContext* context = (DownloadContext*) context_cb;
   const char* path = context->res_path;
   RoadMapFile file;
   const char *directory;
   
   roadmap_log (ROADMAP_DEBUG,"Download finished downloading (%s) Writing %d bytes, last_modified = %s" , path, context->data_size ,last_modified);

   directory = roadmap_path_parent (NULL, path);
   if (!roadmap_file_exists(directory, "")){
      roadmap_log (ROADMAP_DEBUG,"download_done_callback, creating directory %s" , directory );
      roadmap_path_create (directory);
   }
   roadmap_path_free (directory);

   // Save the resource to the file
   file = roadmap_file_open( path, "w" );
   if ( file ) {
      roadmap_file_write( file, context->data, context->data_size );

      roadmap_file_close(file);

      if (context->res_data.on_loaded_cb)
         (*(context->res_data.on_loaded_cb))( context->res_data.name, 1,context->res_data.context, last_modified);
      
   }
   else{
      roadmap_log( ROADMAP_ERROR, "File openning error for file: %s", path );
   }
   
   downloading = FALSE;
   
   roadmap_download_start();
   
   // Deallocate the download context
   free( context->res_data.name );
   free( context->res_data.lang );
   free( context->data );
   free( context->res_path );
   free( context );
   
}
