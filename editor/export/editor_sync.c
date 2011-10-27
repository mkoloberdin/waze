/* editor_sync.c - synchronize data and maps
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
 *   See editor_sync.h
 */

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "roadmap.h"
#include "roadmap_file.h"
#include "roadmap_path.h"
#include "roadmap_locator.h"
#include "roadmap_metadata.h"
#include "roadmap_lang.h"
#ifdef SSD
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_button.h"
#include "ssd/ssd_container.h"
#else
#include "roadmap_dialog.h"
#endif
#include "roadmap_main.h"
#include "roadmap_messagebox.h"
#include "roadmap_label.h"
#include "roadmap_warning.h"

#include "editor_download.h"
#include "../editor_main.h"
#include "editor_sync.h"
#include "editor_upload.h"
#include "roadmap_httpcopy_async.h"

#include "navigate/navigate_graph.h"
#include "Realtime/Realtime.h"
#include "Realtime/RealtimeOffline.h"

#define MAX_MSGS 10
#define MAX_SIZEOF_RESPONSE_MSG 100
#define MIN_FREE_SPACE 5000 //Free space in KB
#define DEFAULT_CONTENT_TYPE        ("application/octet-stream")

static int SyncProgressItems;
static int SyncProgressCurrentItem;
static int SyncProgressTarget;
static int SyncProgressLoaded;
static int SyncUploadNumMessages = 0;
static char SyncUploadMessages[MAX_MSGS][MAX_SIZEOF_RESPONSE_MSG];

static char SyncProgressLabel[100];

static int upload_file_size_callback( void *context, size_t aSize );
static void upload_progress_callback(void *context, char *data, size_t size);
static void upload_error_callback( void *context, int connection_failure, const char *format, ...);
static void upload_done( void *context, char *last_modified, const char *format, ... );


typedef struct tag_upload_context{
	char * full_path;
	char ** files;
	char ** cursor;
}upload_context;

BOOL download_warning_fn ( char* dest_string ) {
     
   int progress;
   
   if (SyncProgressLoaded < 0) {
      return FALSE;
   }
   
   progress = 100 / SyncProgressItems * (SyncProgressCurrentItem - 1) +
         (100 / SyncProgressItems) * SyncProgressLoaded / SyncProgressTarget;
   
   snprintf (dest_string, ROADMAP_WARNING_MAX_LEN, "%s %s: %d%%%%",
               roadmap_lang_get("Progress status"),
               roadmap_lang_get(SyncProgressLabel), 
               progress);
               
   roadmap_main_flush ();
   return TRUE;   
}

static int roadmap_download_request (int size) {

   SyncProgressTarget = size;
   SyncProgressLoaded = 0;
   SyncProgressCurrentItem++;
   return 1;
}

static void roadmap_download_error (const char *format, ...) {

   va_list ap;
   char message[2048];

   va_start(ap, format);
   vsnprintf (message, sizeof(message), format, ap);
   va_end(ap);

   roadmap_log (ROADMAP_ERROR, "Sync error: %s", message);
}



static void roadmap_download_progress (int loaded) {

   if ((SyncProgressLoaded > loaded) || !loaded || !SyncProgressItems) {
      SyncProgressLoaded = loaded;
   } else {

      if (SyncProgressLoaded == loaded) {
         return;

      } else if ((loaded < SyncProgressTarget) &&
            (100 * (loaded - SyncProgressLoaded) / SyncProgressTarget) < 5) {
         return;
      }

      SyncProgressLoaded = loaded;
   }
}



static RoadMapDownloadCallbacks SyncDownloadCallbackFunctions = {
   roadmap_download_request,
   roadmap_download_progress,
   roadmap_download_error
};

static RoadMapHttpAsyncCallbacks gUploadCallbackFunctions =
{
upload_file_size_callback,
upload_progress_callback,
upload_error_callback,
upload_done
};

const char *editor_sync_get_export_path (void) {

	static char path[1024];
	static int initialized = 0; 
	
	if (!initialized) {
#ifdef IPHONE
      roadmap_path_format (path, sizeof (path), roadmap_path_preferred("maps"), "queue");
#else
		roadmap_path_format (path, sizeof (path), roadmap_db_map_path (), "queue");
#endif //IPHONE
	   roadmap_path_create (path);
	   initialized = 1;
	}

	return path;
}


const char *editor_sync_get_export_name (void) {

	static char name[255];
   struct tm *tm;
   time_t now;

   time(&now);
   tm = gmtime (&now);

   snprintf (name, sizeof(name), "rm_%02d_%02d_%02d_%02d%02d%02d.wud",
                     tm->tm_mday, tm->tm_mon+1, tm->tm_year%100,
                     tm->tm_hour, tm->tm_min, tm->tm_sec);

	return name;
}


static int count_upload_files(void) {
   char **files;
   char **cursor;
   const char* directory = editor_sync_get_export_path();
   int count;
   
   files = roadmap_path_list (directory, ".wud");

   count = 0;
   for (cursor = files; *cursor != NULL; ++cursor) {
      count++;
   }
   
   roadmap_path_list_free (files);
   
   return count;
}



static int sync_do_upload () {
   char **files;
   char **cursor;
   const char* directory = editor_sync_get_export_path();
   int count;
   upload_context *  context;
   char * full_path;
   int size;
   const char *header;

   files = roadmap_path_list (directory, ".wud");

   count = 0;
   for (cursor = files; *cursor != NULL; ++cursor) {
      count++;
   }

   //
   cursor = files;
	count = 0;
	//

	context= malloc(sizeof(upload_context));
	context->cursor = cursor;
	context->files = files;
	full_path = roadmap_path_join( directory, *cursor );
	context->full_path = full_path;


   SyncProgressItems = count;
   SyncProgressCurrentItem = 1;
   SyncProgressLoaded = 0;
   SyncUploadNumMessages = 0;

   // this starts the async sending sequence. Further progress is done through the callbacks.
   size = roadmap_file_length (NULL, full_path);
   header = roadmap_http_async_get_upload_header(DEFAULT_CONTENT_TYPE, full_path, size, RealTime_GetUserName(), Realtime_GetPassword());
   if (!roadmap_http_async_post_file(&gUploadCallbackFunctions, (void *)context, editor_upload_get_url(), header, full_path, size))
	{
	  roadmap_log( ROADMAP_ERROR, "File upload error, couldn't start sync socket connect. for file %s ", full_path);
//	  roadmap_path_free(full_path);
//	  roadmap_path_list_free (files);
//	  free(context);
	  return 0;
	}

   return 1;
}


void editor_sync_upload (void) {
   int res;
   
   if (count_upload_files() == 0)
      return;
   
   
   roadmap_download_progress (0);

   snprintf (SyncProgressLabel, sizeof(SyncProgressLabel), "%s",
             roadmap_lang_get ("Uploading data..."));
   
   roadmap_warning_register (download_warning_fn, "edtsync");
   
#ifndef J2ME
   res = sync_do_upload ();
#endif
   
   roadmap_warning_unregister (download_warning_fn);
}

int export_sync (void) {

   int i;
   int res;
   char *messages[MAX_MSGS];
   int num_msgs = 0;
   int fips;

   if (!editor_is_enabled ()) {
      return 0;
   }

   res = roadmap_file_free_space (roadmap_path_user());

#ifndef __SYMBIAN32__
   if ((res >= 0) && (res < MIN_FREE_SPACE)) {
      roadmap_messagebox ("Error",
                  "Please free at least 5MB of space before synchronizing.");
      return -1;
   }
#endif

   roadmap_download_progress (0);

   snprintf (SyncProgressLabel, sizeof(SyncProgressLabel), "%s",
             roadmap_lang_get ("Preparing export data..."));

   roadmap_warning_register (download_warning_fn, "edtsync");
   
   roadmap_main_flush ();
   roadmap_download_progress (0);

   snprintf (SyncProgressLabel, sizeof(SyncProgressLabel), "%s",
             roadmap_lang_get ("Uploading data..."));

   roadmap_main_flush ();

   Realtime_OfflineClose ();
#ifndef J2ME
   res = sync_do_upload ();
#endif
	Realtime_OfflineOpen (editor_sync_get_export_path (),
								 editor_sync_get_export_name ());


   fips = roadmap_locator_active ();

   if (fips < 0) {
      fips = 77001;
   }

#if 0
   if (roadmap_locator_activate (fips) == ROADMAP_US_OK) {
      now_t = time (NULL);
      map_time_t = atoi(roadmap_metadata_get_attribute ("Version", "UnixTime"));

      if ((map_time_t + 3600*24) > now_t) {
         /* Map is less than 24 hours old.
          * A new version may still be available.
          */

         now_tm = *gmtime(&now_t);
         map_time_tm = *gmtime(&map_time_t);

         if (now_tm.tm_mday == map_time_tm.tm_mday) {

            goto end_sync;
         } else {
            /* new day - only download if new maps were already generated. */
            if (now_tm.tm_hour < 2) goto end_sync;
         }
      }
   }
#endif //0

   SyncProgressItems = 1;
   SyncProgressCurrentItem = 0;
   roadmap_download_progress (0);

   snprintf (SyncProgressLabel, sizeof(SyncProgressLabel), "%s",
             roadmap_lang_get ("Downloading new maps..."));

	roadmap_label_clear (-1);
	navigate_graph_clear (-1);
	
   roadmap_main_flush ();
#ifndef J2ME
   res = editor_download_update_map (&SyncDownloadCallbackFunctions);

   if (res == -1) {
      roadmap_messagebox ("Download Error", roadmap_lang_get("Error downloading map update"));
   }
#endif

   for (i=0; i<num_msgs; i++) {
      roadmap_messagebox ("Info", messages[i]);
      free (messages[i]);
   }

   roadmap_warning_unregister (download_warning_fn);

   return 0;
}


static int upload_file_size_callback( void *context, size_t aSize ) {
	SyncProgressTarget = aSize;
   SyncProgressLoaded = 0;
	return 1;
}

static void upload_progress_callback(void *context, char *data, size_t size) {
   SyncProgressLoaded += size;
}


static void upload_error_callback( void *context, int connection_failure, const char *format, ...) {
	upload_context *  uContext = (upload_context *)context;
	roadmap_path_list_free(uContext->files);
	roadmap_path_free(uContext->full_path);
	free(uContext);
}

/*
 * Called once one file was sent successfully. Starts the sending of the next file, if there is one.
 */
static void upload_done( void *context, char *last_modified, const char *format, ... ) {
	upload_context *  uContext = (upload_context *)context;
	char msg[MAX_SIZEOF_RESPONSE_MSG];
	va_list ap;
	char ** new_cursor;
	char * new_full_path;

	if(format){
		va_start(ap, format);
		vsnprintf(msg,sizeof(msg),format,ap);
		va_end(ap);
		roadmap_log(ROADMAP_DEBUG,"done uploading file : %s. Received response : %s",*uContext->cursor,msg);
		strncpy(SyncUploadMessages[SyncUploadNumMessages], msg, MAX_SIZEOF_RESPONSE_MSG);
	}

	SyncProgressCurrentItem ++ ;
	SyncProgressLoaded = 0;
	SyncUploadNumMessages  ++;

	new_cursor = (uContext->cursor)+1;
	roadmap_file_remove(NULL, uContext->full_path); // remove the previous file

	if( (*new_cursor == NULL )  || ( SyncUploadNumMessages == MAX_MSGS ) ) {
		roadmap_path_list_free(uContext->files);
		roadmap_log(ROADMAP_DEBUG, "finished uploading editor_sync files");

	}else{
      int size;
      const char *header;
      
		upload_context * new_context;
		new_full_path = roadmap_path_join( editor_sync_get_export_path(), *new_cursor );
		new_context= malloc(sizeof(upload_context));
		new_context->cursor = new_cursor;
		new_context->files = uContext->files;
		new_context->full_path = new_full_path;
      size = roadmap_file_length (NULL, new_full_path);
      header = roadmap_http_async_get_upload_header(DEFAULT_CONTENT_TYPE, new_full_path, size, RealTime_GetUserName(), Realtime_GetPassword());
      if (!roadmap_http_async_post_file(&gUploadCallbackFunctions, (void *)new_context, editor_upload_get_url(), header, new_full_path, size))
		{
		  roadmap_log( ROADMAP_ERROR, "File upload error, couldn't start sync socket connect. for file %s ", new_full_path);
		  roadmap_path_free(new_full_path);
		  roadmap_path_list_free (new_context->files);
		  free(new_context);
		}
	}

	roadmap_path_free(uContext->full_path);
	free(uContext);
}

