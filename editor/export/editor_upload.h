/* editor_upload.c - Upload a gpx file to freemap.co.il
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
 */

#ifndef INCLUDE__EDITOR_UPLOAD__H
#define INCLUDE__EDITOR_UPLOAD__H

#include "roadmap_download.h"



typedef int  (*EditorUploadCallbackSize)     (void *context, int size);
typedef void (*EditorUploadCallbackProgress) (void * context, int loaded);
typedef void (*EditorUploadCallbackError)    (void * context);
typedef void (*EditorUploadCallbackDone)     (void * context, const char *format, ...);

typedef struct {
   EditorUploadCallbackSize     size;
   EditorUploadCallbackProgress progress;
   EditorUploadCallbackError    error;
   EditorUploadCallbackDone     done;
} EditorUploadCallbacks;


typedef struct tag_editor_upload_context
{
	char * file_name;
	EditorUploadCallbacks *callbacks;
	char* custom_content_type;
	void * caller_specific_context;
} editor_upload_context, *editor_upload_context_ptr;


void editor_upload_initialize (void);
void editor_upload_select     (void);
void editor_upload_file       (const char *filename, int remove_file);

int  editor_upload_auto       (const char *filename,
                               EditorUploadCallbacks *callbacks,
                               const char* custom_target, const char* custom_content_type, void * context  );

#endif // INCLUDE__EDITOR_UPLOAD__H

