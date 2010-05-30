/* roadmap_debug_info.c - Submit debug files
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi R.
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
 *   See roadmap_debug_info.h
 */

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "roadmap.h"
#include "roadmap_main.h"
#include "roadmap_start.h"
#include "roadmap_file.h"
#include "roadmap_path.h"
#include "roadmap_zlib.h"
#include "ssd/ssd_progress_msg_dialog.h"
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_confirm_dialog.h"
#include "roadmap_lang.h"
#include "roadmap_messagebox.h"
#include "editor/export/editor_upload.h"
#include "roadmap_config.h"
#include "roadmap_warning.h"
#include "roadmap_debug_info.h"
#include "Realtime/Realtime.h"


#ifdef RIMAPI
#include "roadmap_time.h"
#include <rimapi.h>
#endif


#define COMPRESSION_LEVEL 6
#define LOG_UPLOAD_CONTENT_TYPE         "log/gzip"

#define GET_2_DIGIT_STRING( num_in, str_out ) \
{ \
str_out[0] = '0'; \
sprintf( &str_out[(num_in < 10)], "%d", num_in ); \
}

static RoadMapConfigDescriptor RMCfgDebugInfoServer =
      ROADMAP_CONFIG_ITEM( CFG_CATEGORY_DEBUG_INFO, CFG_ENTRY_DEBUG_INFO_SERVER );

static char warning_message[ROADMAP_WARNING_MAX_LEN];
static int in_process = 0;

typedef struct tag_upload_context{
	char * full_path;
	char ** files;
	char ** cursor;
	int file_num;
	int total;
}upload_context;

static int upload_file_size_callback( void *context, int aSize );
static void upload_progress_callback( void *context, int aLoaded );
static void upload_error_callback( void *context);
static void upload_done( void *context, const char *format, ... );


#ifdef RIMAPI
static char zipped_log_name[256]; // This is a temporary workaround, until roadmap_path_list is implemented
#endif

static EditorUploadCallbacks gUploadCallbackFunctions =
{
upload_file_size_callback,
upload_progress_callback,
upload_error_callback,
upload_done
};

static int upload_file_size_callback( void *context, int aSize ) {
	return 1;
}

static void upload_progress_callback( void *context, int aLoaded ) {
	//upload_context *  uContext = (upload_context *)context;
	//roadmap_log(ROADMAP_DEBUG,"sent %d bytes of file %s",aLoaded,uContext->full_path);
}

static void upload_error_callback( void *context) {
	upload_context *  uContext = (upload_context *)context;
	ssd_progress_msg_dialog_hide();
	roadmap_messagebox_timeout("Error", "Error sending files",5);
	in_process = 0;
	roadmap_file_remove(NULL, uContext->full_path);
	roadmap_path_list_free(uContext->files);
	roadmap_path_free(uContext->full_path);

	free(uContext);
}

/*
 * Called once one file was sent successfully. Starts the sending of the next file, if there is one.
 */
static void upload_done( void *context, const char *format, ... ) {
	upload_context *  uContext = (upload_context *)context;
	int new_count;
	char ** new_cursor;
	const char * target_url;
	char * new_full_path;
	char msg[500];
	int total;
	va_list ap;
	if(format){
		va_start(ap, format);
		vsnprintf(msg,sizeof(msg),format,ap);
		va_end(ap);
		roadmap_log(ROADMAP_DEBUG,"done uploading log file : %s. Received response : %s",*uContext->cursor,msg);
	}

    new_cursor = (uContext->cursor)+1;
    new_count = (uContext->file_num)+1;
    total = uContext->total;
    roadmap_file_remove(NULL, uContext->full_path); // remove the previous file

	if(new_count==total){ // finished - sent all the files!
		in_process = 0 ;
		roadmap_path_list_free(uContext->files);
		ssd_progress_msg_dialog_hide();
		roadmap_messagebox_timeout("Thank you!!!", "Logs submitted successfully to waze",3);
	}else{ // still more files - call the next one
		upload_context * new_context;
		sprintf (warning_message,"%s\n%d/%d",roadmap_lang_get("Uploading logs..."),new_count+1, total);
		ssd_progress_msg_dialog_show(warning_message);
		roadmap_main_flush();
		new_full_path = roadmap_path_join( roadmap_path_debug(), *new_cursor );
		new_context= malloc(sizeof(upload_context));
		new_context->cursor = new_cursor;
		new_context->files = uContext->files;
		new_context->full_path = new_full_path;
		new_context->file_num = new_count;
		new_context->total = total;
	    target_url = roadmap_config_get ( &RMCfgDebugInfoServer);
		if ( editor_upload_auto( new_full_path, &gUploadCallbackFunctions, target_url, LOG_UPLOAD_CONTENT_TYPE,(void *)new_context) )
	    {
		  roadmap_log( ROADMAP_ERROR, "File upload error. for file %s , number %d", new_full_path, new_count);
		  roadmap_path_free(new_full_path);
		  roadmap_path_list_free (new_context->files);
		  ssd_progress_msg_dialog_hide();
		  roadmap_messagebox_timeout("Error", "Error sending files",5);
		  in_process = 0;
	    }
	}
	roadmap_path_free(uContext->full_path);
	free(uContext);
}



///////////////////////////////////////////////////////
// Compress files and prepare for upload
int prepare_for_upload ()
{
   int res;
   char out_filename[256];
   char **files;
   char **cursor;
   const char* directory;
   int count;
   int total;
   time_t now;
	struct tm *tms;
   char year[5], month[5], day[5];
#ifdef RIMAPI
   timeStruct time_s;
#endif
   sprintf (warning_message,"%s",roadmap_lang_get("Preparing files for upload..."));
   ssd_progress_msg_dialog_show(warning_message);
   roadmap_main_flush();


   //Count files for upload
   directory = roadmap_path_gps();
   files = roadmap_path_list (directory, ".csv");
   count = 1; //Counting also the postmortem
   for (cursor = files; *cursor != NULL; ++cursor) {
      count++;
   }

   total = count;
   count = 0;



   //Prepare log
   count++;
   sprintf (warning_message,"%s\n%d/%d",roadmap_lang_get("Preparing files for upload..."),count, total);
   ssd_progress_msg_dialog_show(warning_message);
   roadmap_main_flush();

   // Building the filename
   time( &now );
   tms = localtime( &now );
#ifdef RIMAPI
   roadmap_time_get_time(&time_s);
   tms->tm_hour = time_s.hours;
   tms->tm_min =  time_s.minutes;
#endif
   GET_2_DIGIT_STRING( tms->tm_mday, day );
   GET_2_DIGIT_STRING( tms->tm_mon+1, month );	// Zero based from January
   GET_2_DIGIT_STRING( tms->tm_year-100, year ); // Year from 1900
   snprintf(out_filename,256, "%s%s%s__%d_%d__%s_%d_%s__%s.gz", day, month, year,
           tms->tm_hour, tms->tm_min, RealTime_GetUserName(), RT_DEVICE_ID, roadmap_start_version(), roadmap_log_filename());
#ifndef RIMAPI
   res = roadmap_zlib_compress(roadmap_log_path(), roadmap_log_filename(), roadmap_path_debug(), out_filename, COMPRESSION_LEVEL,TRUE);
#else
   // 0 = Z_OK = SUCCESS. 1 = failure.
   res = NOPH_ZLib_compress(roadmap_log_path(), roadmap_log_filename(), roadmap_path_debug(),out_filename,COMPRESSION_LEVEL);
   strcpy(zipped_log_name,out_filename); // emporary until path_list is implemented
#endif

   if (res != Z_OK) {
      ssd_progress_msg_dialog_hide();
      return 0;
   }


   //Prepare CSV files
   for (cursor = files; *cursor != NULL; ++cursor) {
      count++;
      sprintf (warning_message,"%s\n%d/%d",roadmap_lang_get("Preparing files for upload..."),count, total);
      ssd_progress_msg_dialog_show(warning_message);
      roadmap_main_flush();

      sprintf(out_filename, "%s%s.gz", *cursor, RealTime_GetUserName());
#ifndef J2ME
      res = roadmap_zlib_compress(directory, *cursor, roadmap_path_debug(), out_filename, COMPRESSION_LEVEL,FALSE);
#else
      // 0 = Z_OK = SUCCESS. 1 = failure.
      res = NOPH_ZLib_compress(directory, *cursor, roadmap_path_debug(),out_filename,COMPRESSION_LEVEL);
#endif
      if (res != Z_OK) {
         ssd_progress_msg_dialog_hide();
         return 0;
      } else {
         roadmap_file_remove(directory, *cursor);
      }
   }

   roadmap_path_list_free (files);



   ssd_progress_msg_dialog_hide();
   return 1;
}

///////////////////////////////////////////////////////
// Compress files and prepare for upload
int upload () {
   const char* directory = roadmap_path_debug();
   const char* target_url;
   char *full_path;
   upload_context * context;

#ifndef RIMAPI
   char **files = roadmap_path_list (directory, ".gz");
#else
   char ** files = malloc(sizeof(char *)*2); // temporary workaround, until path list is implemented
   files[0] = strdup(zipped_log_name);
   files[1]  = NULL;
#endif

   int count;
   int total;
   char **cursor;

   // Set the target to upload to

   sprintf (warning_message,"%s",roadmap_lang_get("Uploading logs..."));
   ssd_progress_msg_dialog_show(warning_message);
   roadmap_main_flush();

   count = 0;
   for (cursor = files; *cursor != NULL; ++cursor) {
      count++;
   }

   cursor = files;
   total = count;
   count = 0;
   target_url = roadmap_config_get ( &RMCfgDebugInfoServer);

   context= malloc(sizeof(upload_context));
   context->cursor = cursor;
   context->files = files;
   context->file_num = count;
   context->total = total;

   full_path = roadmap_path_join( directory, *cursor );

   context->full_path = full_path;

   sprintf (warning_message,"%s\n%d/%d",roadmap_lang_get("Uploading logs..."),count+1, context->total);

   ssd_progress_msg_dialog_show(warning_message);
   roadmap_main_flush();

   // this starts the async sending sequence. Further progress is done through the callbacks.
   if ( editor_upload_auto( full_path, &gUploadCallbackFunctions, target_url, LOG_UPLOAD_CONTENT_TYPE,(void *)context) )
   {
 	  roadmap_log( ROADMAP_ERROR, "File upload error. for file %s ", full_path);

 	  roadmap_path_free(full_path);
 	  roadmap_path_list_free (files);

 	  ssd_progress_msg_dialog_hide();
 	  free(context);
 	  return 0;
   }

   return 1;
}



///////////////////////////////////////////////////////
// Submit all debug info - confirmation callback
static void roadmap_confirmed_debug_info_submit(int exit_code, void *context){
   if (exit_code != dec_yes)
      return;

#ifdef J2ME
	roadmap_log_close_log_file();
#endif

   in_process = 1;

   if (!prepare_for_upload()) {
      roadmap_messagebox_timeout("Oops", "Error sending files",5);
      in_process = 0;
      return;
   }

   if (!upload()) {
      roadmap_messagebox_timeout("Oops", "Error sending files",5);
      in_process = 0;
      return;
   }

}



///////////////////////////////////////////////////////
// Submit all debug info
static void submit (int with_confirmation)
{
   int initialized = 0;

   if (!initialized) {
      roadmap_config_declare( "preferences", &RMCfgDebugInfoServer, CFG_DEBUG_INFO_SERVER_DEFAULT, NULL );
      initialized = 1;
   }
#ifdef IPHONE
   roadmap_main_show_root(NO);
#endif

   if (with_confirmation)
      ssd_confirm_dialog ("Submit logs", "Sending logs requires large amount of data, continue?", TRUE, roadmap_confirmed_debug_info_submit , NULL);
   else
      roadmap_confirmed_debug_info_submit(dec_yes, NULL);
}

///////////////////////////////////////////////////////
// Submit all debug info
void roadmap_debug_info_submit_confirmed (void)
{
   submit(FALSE);
}



///////////////////////////////////////////////////////
// Submit all debug info
void roadmap_debug_info_submit (void)
{
	submit(TRUE);
}
