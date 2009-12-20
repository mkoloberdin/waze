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
#include "roadmap_download.h"
#include "roadmap_config.h"
#include "roadmap_warning.h"
#include "roadmap_debug_info.h"
#include "Realtime/Realtime.h"


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


static int upload_file_size_callback_dummy( int aSize ) { return 1; }
static void upload_progress_callback_dummy( int aLoaded ) {}
static void upload_error_callback_dummy( const char *format, ... ) {}

static RoadMapDownloadCallbacks gUploadCallbackFunctionsSilent =
{
upload_file_size_callback_dummy,
upload_progress_callback_dummy,
upload_error_callback_dummy
};


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
	GET_2_DIGIT_STRING( tms->tm_mday, day );
	GET_2_DIGIT_STRING( tms->tm_mon+1, month );	// Zero based from January
	GET_2_DIGIT_STRING( tms->tm_year-100, year ); // Year from 1900
   snprintf(out_filename,256, "%s%s%s__%d_%d__%s_%d_%s__%s.gz", day, month, year,
           tms->tm_hour, tms->tm_min, RealTime_GetUserName(), RT_DEVICE_ID, roadmap_start_version(), roadmap_log_filename());
   
   res = roadmap_zlib_compress(roadmap_log_path(), roadmap_log_filename(), roadmap_path_debug(), out_filename, COMPRESSION_LEVEL,TRUE);
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
      
      res = roadmap_zlib_compress(directory, *cursor, roadmap_path_debug(), out_filename, COMPRESSION_LEVEL,FALSE);
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
   char **files = roadmap_path_list (directory, ".gz");
   const char* target_url;
   char *response_message = NULL;
   int count;
   int total;
   char **cursor;
   char *full_path;
   
   // Set the target to upload to
   target_url = roadmap_config_get ( &RMCfgDebugInfoServer);

   
   sprintf (warning_message,"%s",roadmap_lang_get("Uploading logs..."));
   ssd_progress_msg_dialog_show(warning_message);
   roadmap_main_flush();
   
   count = 0;
   for (cursor = files; *cursor != NULL; ++cursor) {
      count++;
   }
   
   total = count;
   count = 0;
   
   for (cursor = files; *cursor != NULL; ++cursor) {
      count++;
      sprintf (warning_message,"%s\n%d/%d",roadmap_lang_get("Uploading logs..."),count, total);
      ssd_progress_msg_dialog_show(warning_message);
      roadmap_main_flush();
      
      full_path = roadmap_path_join( directory, *cursor );
      if ( editor_upload_auto( full_path, &gUploadCallbackFunctionsSilent, &response_message, target_url, LOG_UPLOAD_CONTENT_TYPE ) )
      {
         if (response_message)
            roadmap_log( ROADMAP_WARNING, "File upload error. Response message: %s", response_message );
         else
            roadmap_log( ROADMAP_WARNING, "File upload error.");
         
         roadmap_path_free(full_path);
         roadmap_path_list_free (files);
         
         ssd_progress_msg_dialog_hide();
         return 0;
      } else {
         roadmap_file_remove(directory, *cursor);
      }
      roadmap_path_free(full_path);
   }
   
   roadmap_path_list_free (files);
   
   ssd_progress_msg_dialog_hide();
   return 1;
}



///////////////////////////////////////////////////////
// Warning message
static BOOL roadmap_debug_info_warning( char* dest_string )
{
	BOOL res = FALSE;
	if ( in_process )
	{
		strncpy( dest_string, warning_message, ROADMAP_WARNING_MAX_LEN );
		res = TRUE;
	}
   
	return res;
}


///////////////////////////////////////////////////////
// Submit all debug info - confirmation callback
static void roadmap_confirmed_debug_info_submit(int exit_code, void *context){
   if (exit_code != dec_yes)
      return;

   in_process = 1;
   if (!prepare_for_upload()) {
      roadmap_messagebox_timeout("Error", "Error sending files",5);
      in_process = 0;
      return;
   }
   
   if (!upload()) {
      roadmap_messagebox_timeout("Error", "Error sending files",5);
      in_process = 0;
      return;
   }
   
   in_process = 0;
   roadmap_messagebox_timeout("Thank you!!!", "Logs submitted successfully to waze",3);
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
