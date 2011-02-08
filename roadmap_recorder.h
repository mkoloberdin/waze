/* roadmap_recorder.h - Display a recorder dialog
 *
 * LICENSE:
 *
 *   Copyright 2010  Avi R.
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

#ifndef INCLUDE__ROADMAP_RECORDER__H
#define INCLUDE__ROADMAP_RECORDER__H


// Configuration entries
#define CFG_CATEGORY_RECORDER_VOICE          ( "Recorder Voice" )
#define CFG_ENTRY_RECORDER_VOICE_SERVER      ( "Web-Service Address" )
#define CFG_ENTRY_RECORDER_VOICE_URL_PREFIX  ( "Download Prefix" )

// Default configuration values for voice
#define CFG_RECORDER_VOICE_SERVER_DEFAULT       ""
#define CFG_RECORDER_VOICE_URL_PREFIX_DEFAULT   "http://waze.audio.s3.amazonaws.com/"

// Voice ID length (bytes)
#define ROADMAP_VOICE_ID_LEN                 (64)
#define ROADMAP_VOICE_ID_BUF_LEN             ( ROADMAP_VOICE_ID_LEN + 1 )

// Downloaded images file suffix
#define ROADMAP_VOICE_DOWNLOAD_FILE_SUFFIX   (".mp3")

typedef void (*VoiceDownloadCallback) ( void* context, int status, const char* image_path );
typedef void (*RecorderVoiceUploadCallback) (void * context);
typedef void(*recorder_closed_cb)( int exit_code, void *context );

void roadmap_recorder_voice_initialize( void );
BOOL roadmap_recorder_voice_upload( const char *voice_folder, const char *voice_file,
                                   char* voice_id, RecorderVoiceUploadCallback cb, void * context);
BOOL roadmap_recorder_voice_download( const char* voice_id,  void* context_cb, VoiceDownloadCallback download_cb );

void roadmap_recorder (const char *text, int seconds, recorder_closed_cb on_recorder_closed, int auto_start, const char *path, const char* file_name, void *context);



#endif // INCLUDE__ROADMAP_RECORDER__H
