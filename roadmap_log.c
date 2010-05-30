/* roadmap_log.c - a module for managing uniform error & info messages.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
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
 *   #include "roadmap.h"
 *
 *   void roadmap_log (int level, char *format, ...);
 *
 * This module is used to control and manage the appearance of messages
 * printed by the roadmap program. The goals are (1) to produce a uniform
 * look, (2) have a central point of control for error management and
 * (3) have a centralized control for routing messages.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <assert.h>

#include "roadmap.h"
#include "roadmap_path.h"
#include "roadmap_dbread.h"
#include "roadmap_file.h"
#include "roadmap_messagebox.h"
#include "Realtime/Realtime.h"
static FILE *sgLogFile = NULL;

#if defined(IPHONE) || defined(unix) && !defined(J2ME)
#include <sys/timeb.h>
#endif

#ifdef   FREEZE_ON_FATAL_ERROR
   #pragma message("    In case of fatal error process will freeze and wait for debugger")
#endif   // FREEZE_ON_FATAL_ERROR

#define ROADMAP_LOG_STACK_SIZE 256
#define MAX_SIZE_LOG_FILE 10000 // 10 megabytes for now
#define TO_KEEP_LOG_SIZE 1000 // keep the last megabyte

static const char *RoadMapLogStack[ROADMAP_LOG_STACK_SIZE];
static int         RoadMapLogStackCursor = 0;

static struct roadmap_message_descriptor {
   int   level;
   int   show_stack;
   int   save_to_file;
   int   do_exit;
   char *prefix;
} RoadMapMessageHead [] = {
   {ROADMAP_MESSAGE_DEBUG,   0, 1, 0, "..debug.."},
   {ROADMAP_MESSAGE_INFO,    0, 1, 0, "Info --->"},
   {ROADMAP_MESSAGE_WARNING, 0, 1, 0, ":WARNING:"},
   {ROADMAP_MESSAGE_ERROR,   1, 1, 0, "ERROR !!!"},
   {ROADMAP_MESSAGE_FATAL,   1, 1, 1, "[???????]"},
   {0,                       1, 1, 1, "??"}
};


static roadmap_log_msgbox_handler RoadmapLogMsgBox;

void roadmap_log_register_msgbox (roadmap_log_msgbox_handler handler) {
   RoadmapLogMsgBox = handler;
}


#ifndef J2ME
void roadmap_log_push (const char *description) {

   if (RoadMapLogStackCursor < ROADMAP_LOG_STACK_SIZE) {
      RoadMapLogStack[RoadMapLogStackCursor++] = description;
   }
}

void roadmap_log_pop (void) {

   if (RoadMapLogStackCursor > 0) {
      RoadMapLogStackCursor -= 1;
   }
}
#endif

void roadmap_log_reset_stack (void) {

   RoadMapLogStackCursor = 0;
}


void roadmap_log_save_all (void) {

    int i;

    for (i = 0; RoadMapMessageHead[i].level > 0; ++i) {
        RoadMapMessageHead[i].save_to_file = 1;
    }
}


void roadmap_log_save_none (void) {

    int i;

    for (i = 0; RoadMapMessageHead[i].level > 0; ++i) {
        RoadMapMessageHead[i].save_to_file = 0;
    }
    roadmap_log_purge();
}


int  roadmap_log_enabled (int level, char *source, int line) {
   return (level >= roadmap_verbosity());
}

#if(defined WIN32PC && defined _DEBUG)
#define ODS_BUFFER_SIZE    (300)
static void show_logs_in_debugger( struct roadmap_message_descriptor *category, const char *format, va_list ap)
{
   char message   [ODS_BUFFER_SIZE + 1];
   char formatted [ODS_BUFFER_SIZE + 100];

   message[ODS_BUFFER_SIZE] = '\0';
   vsnprintf( message, ODS_BUFFER_SIZE, format, ap);

   sprintf( formatted, "[LOG] %s\t%s\r\n", category->prefix, message);
   OutputDebugStringA( formatted);
}
#endif   // WIN32PC Debug

const char *roadmap_log_access_mode()
{
#if defined (__SYMBIAN32__)
   return ("a+");
#elif defined(ANDROID)
   return ("a+");
#elif !defined (J2ME)
   return ("sa");
#else
   //return ("w");
#endif

}

const char *roadmap_log_path()
{
#if defined (__SYMBIAN32__)
#if defined (WINSCW)
   return ("C:\\");
#else
   return ("C:\\Data\\");
//   return (roadmap_db_map_path());
#endif
#elif defined(ANDROID)
   // Only sdcard can be accessed for reading
   // For the debug purposes the log is appended
   return ( roadmap_path_sdcard());
#elif !defined (J2ME)
   return (roadmap_path_user());
#else
   //return ("file:///e:/FreeMap");
#endif
}

const char *roadmap_log_filename()
{
#if defined (__SYMBIAN32__)
   return ("waze_log.txt");
#elif defined(ANDROID)
   return ("waze_log.txt");
#elif !defined (J2ME)
   return ("postmortem");
#else
   //return ("logger.txt");
#endif
}

static void roadmap_log_one (struct roadmap_message_descriptor *category,
                             FILE *file,
                             char  saved,
                             const char *source,
                             int line,
                             const char *format,
                             va_list ap) {


#if (defined (_WIN32) && !defined (__SYMBIAN32__))
SYSTEMTIME st;
#endif
int i;
struct tm *tms;
time_t now;
char year[5], month[5], day[5];
time( &now );
tms = localtime( &now );
GET_2_DIGIT_STRING( tms->tm_mday, day );
GET_2_DIGIT_STRING( tms->tm_mon+1, month );	// Zero based from January
GET_2_DIGIT_STRING( tms->tm_year-100, year ); // Year from 1900

#ifdef J2ME
   fprintf (file, "%d %c%s %s, line %d ",
         time(NULL), saved, category->prefix, source, line);
#elif defined (__SYMBIAN32__)



   time (&now);
   tms = localtime (&now);

   fprintf (file, "%02d:%02d:%02d %c%s %s, line %d ",
         tms->tm_hour, tms->tm_min, tms->tm_sec,
         saved, category->prefix, source, line);
#elif defined(ANDROID)


   char date_buf[128];

   time (&now);
   tms = localtime (&now);
   strftime( date_buf, sizeof( date_buf ), "%d/%m/%y", tms );

   fprintf (file, "%s %02d:%02d:%02d %c%s %s, line %d ",
		   date_buf, tms->tm_hour, tms->tm_min, tms->tm_sec,
         saved, category->prefix, source, line);

#elif defined (_WIN32)
   GetLocalTime(&st);

   fprintf (file, "%02d/%02d %02d:%02d:%02d %s\t",
         st.wDay, st.wMonth, st.wHour, st.wMinute, st.wSecond,
         category->prefix);

#else
   //struct tm *tms;
   struct timeb tp;

   ftime(&tp);
   tms = localtime (&tp.time);

   fprintf (file, "%02d:%02d:%02d.%03d %c%s %s, line %d ",
         tms->tm_hour, tms->tm_min, tms->tm_sec, tp.millitm,
         saved, category->prefix, source, line);
#endif

   if (!category->show_stack && (RoadMapLogStackCursor > 0)) {
      fprintf (file, "(%s): ", RoadMapLogStack[RoadMapLogStackCursor-1]);
   }

#ifdef _DEBUG
  // if( format && (*format) && ('\n' == format[strlen(format)-1]))
#endif   // _DEBUG

   vfprintf(file, format, ap);
   fprintf( file, " \t[File: '%s'; Line: %d]\n", source, line);

   if (category->show_stack && RoadMapLogStackCursor > 0) {

      int indent = 8;

      fprintf (file, "   Call stack:\n");

      for (i = 0; i < RoadMapLogStackCursor; ++i) {
          fprintf (file, "%*.*s %s\n", indent, indent, "", RoadMapLogStack[i]);
          indent += 3;
      }
   }

   if (RoadmapLogMsgBox && category->do_exit) {
      char str[256];
      char msg[256];
#ifdef   FREEZE_ON_FATAL_ERROR
      const char* title = "Fatal Error - Process awaits debugger";

#else
      const char* title = "Fatal Error";

#endif   // FREEZE_ON_FATAL_ERROR

      vsprintf(msg, format, ap);
      sprintf (str, "%c%s %s, line %d %s",
         saved, category->prefix, source, line, msg);
      RoadmapLogMsgBox(title, str);
   }
}

void roadmap_log (int level, const char *source, int line, const char *format, ...) {

   va_list ap;
   char saved = ' ';
   struct roadmap_message_descriptor *category;
   char *debug;

   if (level < roadmap_verbosity()) return;

#if(defined DEBUG && defined SKIP_DEBUG_LOGS)
   return;
#endif   // SKIP_DEBUG_LOGS

   debug = roadmap_debug();

   if ((debug[0] != 0) && (strcmp (debug, source) != 0)) return;

   for (category = RoadMapMessageHead; category->level != 0; ++category) {
      if (category->level == level) break;
   }

   va_start(ap, format);

   if (category->save_to_file) {
      static int open_file_attemped = 0;

      if ((sgLogFile == NULL) && (!open_file_attemped)) {
         open_file_attemped = 1;
         
         sgLogFile = roadmap_file_fopen (roadmap_log_path(),
                                         roadmap_log_filename(),
                                         roadmap_log_access_mode());

         if (sgLogFile) fprintf (sgLogFile, "*** Starting log file %d ***\n", (int)time(NULL));
      }

      if (sgLogFile != NULL) {

         roadmap_log_one (category, sgLogFile, ' ', source, line, format, ap);
         fflush (sgLogFile);
         //fclose (file);

         va_end(ap);
         va_start(ap, format);

         saved = 's';
      }
   }

#ifdef __SYMBIAN32__
   //roadmap_log_one (category, __stderr(), saved, source, line, format, ap);
#else

#if(defined WIN32PC && defined _DEBUG)
   show_logs_in_debugger( category, format, ap);
#endif   // WIN32PC Debug

   roadmap_log_one (category, stderr, saved, source, line, format, ap);
#endif

   va_end(ap);

   if( category->do_exit)
#ifdef FREEZE_ON_FATAL_ERROR
   {
      int beep_times =   20;
      int sleep_time = 1000;

      do
      {
         Sleep( sleep_time);

         if( beep_times)
         {
            fprintf( sgLogFile, ">>> FATAL ERROR - WAITING FOR PROCESS TO BE ATTACHED BY A DEBUGGER...\r\n");
            MessageBeep(MB_OK);
            beep_times--;

            if(!beep_times)
               sleep_time = 5000;
         }

      }  while(1);
   }

#else
      exit(1);

#endif   // FREEZE_ON_FATAL_ERROR
}


void roadmap_log_purge (void) {

    roadmap_file_remove (roadmap_path_user(), "postmortem");
}


void roadmap_check_allocated_with_source_line
                (const char *source, int line, const void *allocated) {

    if (allocated == NULL) {
        roadmap_log (ROADMAP_MESSAGE_FATAL, source, line, "no more memory");
    }
}

/*
 * Logging of the raw data without any formatting and additional information
 * The logger file has to be opened before !
 */
BOOL roadmap_log_raw_data ( const char* data )
{
	BOOL ret_val = FALSE;
	if ( sgLogFile && data )
	{
		ret_val = roadmap_log_raw_data_fmt( "%s", data );
	}
	return FALSE;
}




/*
 * Logging of the formatted raw data without any additional information
 * The logger file has to be opened before !
 */
BOOL roadmap_log_raw_data_fmt( const char *format, ... )
{
	va_list ap;
	BOOL ret_val = FALSE;

	if ( sgLogFile && format )
	{
		va_start( ap, format );
		vfprintf( sgLogFile, format, ap );
		ret_val = TRUE;
		va_end( ap );
	}
	return FALSE;
}

#if 0
void roadmap_log_init(){
	long fileSize;
	const char *log_path;
	const char *log_path_temp;
	const char * path;
	const char * name;
	char lineFromLog[300];
#if defined (__SYMBIAN32__)
	#if defined (WINSCW)
      path = "C:\\";
      name = "waze_log.txt";
	#else
	  path = roadmap_db_map_path();
      name = "waze_log.txt";
	#endif
#elif defined(ANDROID)
      roadmap_path_sdcard();
      name = "waze_log.txt";
#elif !defined (J2ME)
      path = roadmap_path_user();
      name = "postmortem";
#endif

	fileSize = roadmap_file_length(path,name);
	if (fileSize > 0 ){ // file exists
		if(fileSize>MAX_SIZE_LOG_FILE){
		   FILE * LogFile = roadmap_file_fopen(path,name,"sa+");
		   FILE * tempLogFile = roadmap_file_fopen(path,"temp_log_file.txt","sa+");
		   fseek(LogFile, 0, SEEK_END-TO_KEEP_LOG_SIZE);
		   fgets (lineFromLog,300, LogFile );
		   while (1){
		   	    fgets (lineFromLog,300, LogFile );
		   	    if(feof(LogFile))
		   	    	break;
		   		fputs (lineFromLog,tempLogFile );
		   }
		   fclose(LogFile);
		   fclose(tempLogFile);
		   log_path = roadmap_path_join (path, name);
		   log_path_temp = roadmap_path_join (path, "temp_log_file.txt");
	  	   roadmap_file_remove (path, name);
	  	   rename(log_path_temp,log_path);
	  	   roadmap_path_free (log_path);
	  	   roadmap_path_free (log_path_temp);
	    }
	}
}
#endif

FILE * roadmap_log_get_log_file(){
	if (sgLogFile){
		fseek(sgLogFile,0,SEEK_SET);
		return sgLogFile;
	}
	return NULL;
}
