/* roadmap_spawn.c - Process control interface for the RoadMap application.
 *
 * LICENSE:
 *
 *   Copyright 2005 Ehud Shabtai
 *
 *   Based on an implementation by Pascal F. Martin.
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
 *   See roadmap_spawn.h
 */

#include <windows.h>
#include <stdio.h>
#include "../roadmap.h"
#include "../roadmap_list.h"
#include "../roadmap_spawn.h"

typedef void (*text2speech)(const char *text);
static int flite_dll_initialized = 0;
static HINSTANCE flite_dll_inst;
static text2speech flite_t2s;
static HANDLE flite_thread;
static HANDLE flite_inuse_event;
static HANDLE flite_launch_event;
static RoadMapFeedback *flite_feedback;
static const char *flite_text;

static char *RoadMapSpawnPath = NULL;

static RoadMapList RoadMapSpawnActive;

DWORD WINAPI FliteThread(LPVOID lpParam) {

   while (1) {
      char *text;

      if (WaitForSingleObject(flite_launch_event, INFINITE) != WAIT_OBJECT_0) {
         roadmap_log (ROADMAP_ERROR,
            "FliteThread - error waiting for event. No more talking.");
         break;
      }

      text = strdup(flite_text);
      SetEvent(flite_inuse_event);

      flite_t2s(text);
      free(text);

      flite_feedback->handler (flite_feedback->data);
   }

   return 0;
}


#ifdef UNDER_CE
static int exec_flite_dll (const char *command_line,
					   RoadMapFeedback *feedback) {

   if (flite_dll_initialized == -1) return -1;

   if (!flite_dll_initialized) {
	   char full_name[MAX_PATH];
	   LPWSTR full_name_unicode;

      snprintf(full_name, MAX_PATH, "%s\\flite_dll.dll", RoadMapSpawnPath);
      full_name_unicode = ConvertToWideChar(full_name, CP_UTF8);

      flite_dll_inst = LoadLibrary(full_name_unicode);
      free (full_name_unicode);
      if (!flite_dll_inst) {
         flite_dll_initialized = -1;
         return -1;
      }

      flite_t2s = (text2speech)GetProcAddress(flite_dll_inst, L"text2speech");
      if (!flite_t2s) {
         FreeLibrary(flite_dll_inst);
         flite_dll_initialized = -1;
         return -1;
      }

      flite_inuse_event = CreateEvent(NULL, FALSE, FALSE, NULL);
      flite_launch_event = CreateEvent(NULL, FALSE, FALSE, NULL);

		flite_thread = CreateThread(NULL, 0, FliteThread, 0, 0, NULL);

      SetThreadPriority(flite_thread, THREAD_PRIORITY_TIME_CRITICAL);

      flite_dll_initialized = 1;
   }

   do {
      while (*command_line && (*command_line == ' ')) command_line++;
      if(*command_line == '-') {
         while (*command_line && (*command_line != ' ')) command_line++;
      }
   } while ((*command_line == ' ') || (*command_line == '-'));

   flite_text = command_line;
   flite_feedback = feedback;

   SetEvent (flite_launch_event);
   if (WaitForSingleObject (flite_inuse_event, 1000) != WAIT_OBJECT_0) {

      ResetEvent (flite_launch_event);
      feedback->handler (feedback->data);
   }

   return 0;
}
#endif


static int roadmap_spawn_child (const char *name,
								const char *command_line,
								RoadMapPipe pipes[2],
                        int feedback)
{
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	char full_name[MAX_PATH];
	LPWSTR full_path_unicode;
	LPWSTR command_line_unicode;


	memset(&pi, 0, sizeof(pi));
	memset(&si, 0, sizeof(si));

	if (*name != '\\') {
		snprintf(full_name, MAX_PATH, "%s\\%s", RoadMapSpawnPath, name);
		full_path_unicode = ConvertToWideChar(full_name, CP_UTF8);
	} else {
		full_path_unicode = ConvertToWideChar(name, CP_UTF8);
	}

	if (command_line != NULL) {
		command_line_unicode = ConvertToWideChar(command_line, CP_UTF8);
	} else {
			command_line_unicode = NULL;
	}

	si.cb = sizeof(STARTUPINFO);

	if (!CreateProcess(full_path_unicode, command_line_unicode, NULL,
							NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
			free(full_path_unicode);
			free(command_line_unicode);
			roadmap_log (ROADMAP_ERROR, "CreateProcess(\"%s\") failed",
							full_name);
			return -1;
	}

	free(full_path_unicode);

	if (command_line_unicode != NULL) {
		free(command_line_unicode);
	}

   CloseHandle(pi.hThread);
   if (!feedback) {
      CloseHandle(pi.hProcess);
   }


	if (pipes != NULL) {
		pipes[0] = ROADMAP_SPAWN_INVALID_PIPE;
		pipes[1] = ROADMAP_SPAWN_INVALID_PIPE;
	}

	return (int)pi.hProcess;
}


void roadmap_spawn_initialize (const char *argv0)
{
	WCHAR path_unicode[MAX_PATH];
	char *path;
	char *tmp;

	GetModuleFileName(NULL, path_unicode,
		sizeof(path_unicode)/sizeof(path_unicode[0]));
	path = ConvertToMultiByte(path_unicode, CP_UTF8);
	tmp = strrchr (path, '\\');
	if (tmp != NULL) {
		*tmp = '\0';
	}
	RoadMapSpawnPath = path;
	ROADMAP_LIST_INIT(&RoadMapSpawnActive);
}


int roadmap_spawn (const char *name,
				   const char *command_line)
{
	return roadmap_spawn_child (name, command_line, NULL, 0);
}


int  roadmap_spawn_with_feedback (const char *name,
								  const char *command_line,
								  RoadMapFeedback *feedback)
{
#ifdef UNDER_CE
   if (!strcmp (name, "flite") &&
      (exec_flite_dll(command_line, feedback) != -1)) {

      return 0;
   }
#endif

	roadmap_list_append (&RoadMapSpawnActive, &feedback->link);
	feedback->child = roadmap_spawn_child (name, command_line, NULL, 1);

	return feedback->child;
}


int  roadmap_spawn_with_pipe (const char *name,
							  const char *command_line,
							  RoadMapPipe pipes[2],
							  RoadMapFeedback *feedback)
{
	roadmap_list_append (&RoadMapSpawnActive, &feedback->link);
	feedback->child = roadmap_spawn_child (name, command_line, pipes, 0);

	return feedback->child;
}


void roadmap_spawn_check (void)
{
	RoadMapListItem *item, *tmp;
	RoadMapFeedback *feedback;

	ROADMAP_LIST_FOR_EACH (&RoadMapSpawnActive, item, tmp) {

		feedback = (RoadMapFeedback *)item;

		if (WaitForSingleObject((HANDLE)feedback->child, 0) == WAIT_OBJECT_0) {
			CloseHandle((HANDLE)feedback->child);
			roadmap_list_remove (&feedback->link);
			feedback->handler (feedback->data);
		}
	}
}


void roadmap_spawn_command (const char *command)
{
   roadmap_spawn(command, NULL);
}


int roadmap_spawn_write_pipe (RoadMapPipe pipe, const void *data, int length)
{
   return -1;
}


int roadmap_spawn_read_pipe (RoadMapPipe pipe, void *data, int size)
{
   return -1;
}


void roadmap_spawn_close_pipe (RoadMapPipe pipe) {} 
