/* roadmap_recorder.c - Recorder voice upload and download
 *
 * LICENSE:
 *   Copyright 2009, Waze Ltd
 *   Alex Agranovich
 *   Avi R.
 *
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
 *    // Close the dialog on error
 * SYNOPSYS:
 *
 *   See roadmap_recorder.c
 */
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_main.h"
#include "roadmap_download.h"
#include "roadmap_config.h"
#include "roadmap_file.h"
#include "editor/export/editor_upload.h"
#include "ssd/ssd_progress_msg_dialog.h"
#include "Realtime/Realtime.h"
#include "roadmap_lang.h"
#include "roadmap_path.h"
#include "roadmap_httpcopy_async.h"
#include "roadmap_recorder.h"

//======== Local Types ========
typedef struct
{
	char* voice_path;
	char* data;
	size_t data_size;
	VoiceDownloadCallback download_cb;
	void* context_cb;
} DownloadContext;

//======== Defines ========
#define VOICE_UPLOAD_RESPONSE_PREFIX      "id="
#define VOICE_UPLOAD_SSD_DLG_NAME         "Voice upload process"
#define VOICE_UPLOAD_PROGRESS_FIELD       "Progress"
#define VOICE_UPLOAD_STATUS_FIELD         "Voice upload status"
#define VOICE_UPLOAD_ERROR                "Voice upload error"

#ifdef IPHONE_NATIVE
#define VOICE_UPLOAD_CONTENT_TYPE            "audio/ima4"
#elif defined (_WIN32)
#define VOICE_UPLOAD_CONTENT_TYPE            "audio/wav"
#elif defined (ANDROID)
#define VOICE_UPLOAD_CONTENT_TYPE            "audio/mp4"
#else
#define VOICE_UPLOAD_CONTENT_TYPE            "audio/mp4" //TODO: verify that
#endif

#define VOICE_DOWNLOAD_CACHE_SIZE			50




//======== Globals ========

static char* gVoiceDownloadCache[VOICE_DOWNLOAD_CACHE_SIZE] = {0};
static int  gVoiceDownloadCacheCurIndex = 0;

////// Configuration section ///////
static RoadMapConfigDescriptor RMCfgRecorderVoiceServer =
                        ROADMAP_CONFIG_ITEM( CFG_CATEGORY_RECORDER_VOICE, CFG_ENTRY_RECORDER_VOICE_SERVER );
static RoadMapConfigDescriptor RMCfgRecorderVoiceUrlPrefix =
                        ROADMAP_CONFIG_ITEM( CFG_CATEGORY_RECORDER_VOICE, CFG_ENTRY_RECORDER_VOICE_URL_PREFIX );






static int  download_size_callback( void *context_cb, size_t size );
static void download_progress_callback( void *context_cb, char *data, size_t size );
static void download_error_callback( void *context_cb, int connection_failure, const char *format, ... );
static void download_done_callback( void *context_cb, char *last_modified, const char *format, ...  );


static BOOL roadmap_recorder_voice_uploader( const char *voice_folder, const char *voice_file, char* voice_id,RecorderVoiceUploadCallback cb, void * context);
static char* get_download_url( const char* voice_id );
static char* get_upload_url( void );

static void download_cache_add( const char* file_path );
static void download_cache_clear( void );


static int upload_file_size_callback( void *context, size_t aSize );
static void upload_progress_callback( void *context, char *data, size_t size);
static void upload_error_callback( void *context, int connection_failure, const char *format, ...);
static void upload_done( void *context, char *last_modified, const char *format, ... );

typedef struct tag_upload_context {
	RecorderVoiceUploadCallback cb;
	void * context;
	char * full_path; // voice name, received from RTAlerts.
	char * voice_id;  // buffer from RTAlerts, will be filled according to the response from server.
} upload_context;

/*
 * These functions will handle the async voice upload.
 */
static RoadMapHttpAsyncCallbacks gUploadCallbackFunctions =
{
	upload_file_size_callback,
	upload_progress_callback,
	upload_error_callback,
	upload_done,
};


static RoadMapHttpAsyncCallbacks gHttpAsyncCallbacks =
{
   download_size_callback,
   download_progress_callback,
   download_error_callback,
   download_done_callback
};


/***********************************************************/
/*  Name        : roadmap_recorder_voice_download()
 *  Purpose     : Constructs the url and downloads the voice.
 *
 *  Params     : (in) voice_id - the voice id to download
 *
 *             : (out) download_cb - the full path to the downloaded voice file - user responsibility to
 *             		deallocate the memory!!!!
 *
 *  Notes      :
 *
 */
BOOL roadmap_recorder_voice_download( const char* voice_id,  void* context_cb, VoiceDownloadCallback download_cb )
{
    BOOL res = FALSE;
    char *url;
    char  *voice_file, *voice_path;
    DownloadContext* context;

    roadmap_log( ROADMAP_DEBUG, "Downloading the voice.  ID: %s", voice_id );

    // Target file name from the voice id
    voice_file = malloc( strlen( voice_id ) + strlen( ROADMAP_VOICE_DOWNLOAD_FILE_SUFFIX ) + 1 );
    strcpy( voice_file, voice_id );
    strcat( voice_file, ROADMAP_VOICE_DOWNLOAD_FILE_SUFFIX );

    // Target full path for the output
    voice_path = roadmap_path_join( roadmap_path_voices(), voice_file );
    if (roadmap_file_exists( NULL, voice_path ) )
    {
    	// Add file to cache
    	download_cache_add( voice_path );
    	// The file exists - no need to download
    	download_cb( context_cb, 0, voice_path );
    	roadmap_path_free( voice_path );
    }
    else
    {
      url = get_download_url( voice_id );
		// Init the download process context
		context = malloc( sizeof( DownloadContext ) );
		context->voice_path = voice_path;
		context->context_cb = context_cb;
		context->download_cb = download_cb;
		context->data = NULL;
		// Show the message
		ssd_progress_msg_dialog_show( roadmap_lang_get( "Downloading voice ..." ) );
		// Start the process
		roadmap_http_async_copy( &gHttpAsyncCallbacks, context, url ,0 );
		free( url );
    }

	 free( voice_file );

    return res;
}

/***********************************************************/
/*  Name        : download_url( const char* voice_id )
 *  Purpose     : Returns the url constructed from the given voice id
 *
 */
static char* get_download_url( const char* voice_id )
{
	char* url;
    const char* url_prefix;

    url_prefix = roadmap_config_get ( &RMCfgRecorderVoiceUrlPrefix );

    // Size of prefix +  size of voice id + '\0'
    url = malloc( strlen( url_prefix ) + strlen( voice_id ) + 1 );

    roadmap_check_allocated( url );

    strcpy( url, url_prefix );
	strcat( url, voice_id );

	return url;
}


/***********************************************************/
/*  Name        : download_size_callback( void *context, size_t size )
 *  Purpose     : Download callback: Size
 *
 */
static int  download_size_callback( void *context_cb, size_t size )
{
   DownloadContext* context = (DownloadContext*) context_cb;

   // Allocate data storage
   if ( size > 0 )
   {
   	context->data = malloc( size );
   	context->data_size = 0;
   }

	return size;
}
/***********************************************************/
/*  Name        : download_progress_callback( void *context, char *data, size_t size )
 *  Purpose     : Download callback: Progress
 *
 */
static void download_progress_callback( void *context_cb, char *data, size_t size )
{
   DownloadContext* context = (DownloadContext*) context_cb;

   // Store data
   if ( context->data )
   {
   	memcpy( context->data + context->data_size, data, size );
   	context->data_size += size;
   }
}

/***********************************************************/
/*  Name        : download_error_callback( void *context, const char *format, ... )
 *  Purpose     : Download callback: Error
 *
 */
static void download_error_callback( void *context_cb, int connection_failure, const char *format, ... )
{
   DownloadContext* context = (DownloadContext*) context_cb;
   va_list ap;
   char err_string[1024];

   // Load the arguments
   va_start( ap, format );
   vsnprintf( err_string, 1024, format, ap );
   va_end( ap );

   // Log it...
   roadmap_log( ROADMAP_WARNING, err_string );

	// Free allocated space
	if ( context->data )
	{
		free( context->data );
		context->data = NULL;
	}

   context->download_cb( context->context_cb, -1, NULL );
}

/***********************************************************/
/*  Name        : download_done_callback( void *context )
 *  Purpose     : Download callback: Done
 *
 */
static void download_done_callback( void *context_cb, char *last_modified, const char *format, ...  )
{
	DownloadContext* context = (DownloadContext*) context_cb;
	const char* path = context->voice_path;
	RoadMapFile file;

	roadmap_log( ROADMAP_INFO, "Download is finished. Writing %d bytes to the file: %s", context->data_size, path );

	// Save the voice to the file
	file = roadmap_file_open( path, "w" );
	if ( !file )
	{
		roadmap_log( ROADMAP_WARNING, "File openning error for file: %s", path );
	}

	roadmap_file_write( file, context->data, context->data_size );

	roadmap_file_close(file);

	ssd_progress_msg_dialog_hide();

	context->download_cb( context->context_cb, 0, context->voice_path );

	// Add file to cache
	download_cache_add( path );

	// Deallocate the download context
	free( context->data );
	roadmap_path_free( context->voice_path );
	free( context );
}


/***********************************************************/
/*  Name        : roadmap_recorder_voice_upload()
 *  Purpose     : Upload the voice to the server.
 *  Params      : (in) voice_folder
 *  			    : (in) voice_file
 *              : (out) voice_id - the string containing the resulting voice ID, received from the server
 *              			the input buffer must be of size ROADMAP_VOICE_ID_BUF_LEN
 */
BOOL roadmap_recorder_voice_upload( const char *voice_folder, const char *voice_file,
		char* voice_id, RecorderVoiceUploadCallback cb, void * context)
{
	BOOL res;

	res = roadmap_recorder_voice_uploader( voice_folder, voice_file, voice_id, cb, context );

	return res;
}



/***********************************************************/
/*  Name        : roadmap_recorder_voice_uploader()
 *  Purpose     : Upload the voice to the server. Auxiliary static function
 *  Params      : (in) voice_folder
 *  			: (in) voice_file
 *              : (out) voice_id - the string containing the resulting voice ID, received from the server
 *						the input buffer must be of size ROADMAP_VOICE_ID_BUF_LEN
 *              : (out) message - parsed response from the server
 */
static BOOL roadmap_recorder_voice_uploader( const char *voice_folder, const char *voice_file,
                                            char* voice_id, RecorderVoiceUploadCallback cb, void * context)
{
   BOOL res = FALSE;
   char* full_path;
   const char* target_url;
   upload_context *  ctx;
   int size;
   const char *header;
   
   if( !voice_id )
   {
      roadmap_log( ROADMAP_ERROR, "File upload error: voice id buffer is not available!!" );
      return FALSE;
   }
   
   // Get the full path to the file
   full_path = roadmap_path_join( voice_folder, voice_file );
   
   // Set the target to upload to
   target_url = get_upload_url();
   
   roadmap_log( ROADMAP_DEBUG, "Uploading file: %s. ", full_path );
   
   ctx = malloc(sizeof(upload_context));
   ctx->context = context;
   ctx->cb = cb;
   ctx->full_path = full_path;
   ctx->voice_id = voice_id;
   
   // Upload and get the response
   size = roadmap_file_length (NULL, full_path);
   header = roadmap_http_async_get_upload_header(VOICE_UPLOAD_CONTENT_TYPE, full_path, size, NULL, NULL);
   if (roadmap_http_async_post_file(&gUploadCallbackFunctions, (void *)ctx, target_url, header, full_path, size))
   {
      //change to debug and comment.
      roadmap_log( ROADMAP_DEBUG, "Started Async connection for file : %s", full_path );
      res = TRUE;
   }
   else
   {
      roadmap_log( ROADMAP_WARNING, "File upload error on socket connect %s", full_path );
      roadmap_path_free( full_path );
      free( ctx );
      res = FALSE;
   }
   
   return res;
}



/***********************************************************/
/*  Name        : roadmap_recorder_voice_inialize()
 *  Purpose     : Initializes the recorder voice related parameters
 *
 */
void roadmap_recorder_voice_initialize( void )
{
    /// Declare the configuration entries
    // Server for voice upload
    roadmap_config_declare( "preferences", &RMCfgRecorderVoiceServer, CFG_RECORDER_VOICE_SERVER_DEFAULT, NULL );
    // Url prefix for the voice download
    roadmap_config_declare( "preferences", &RMCfgRecorderVoiceUrlPrefix, CFG_RECORDER_VOICE_URL_PREFIX_DEFAULT, NULL );
}

/***********************************************************/
/*  Name        : roadmap_recorder_voice_shutdown()
 *  Purpose     : Finalizes the voice work
 *
 */
void roadmap_recorder_voice_shutdown()
{
	/* Remove downloaded voices from the directory */
	download_cache_clear();
}


/***********************************************************/
/*  Name        : download_cache_add( const char* file_path )
 *  Purpose     : Adds the downloaded file to the cache
 *
 */
static void download_cache_add( const char* file_path )
{

	if ( gVoiceDownloadCache[gVoiceDownloadCacheCurIndex] != NULL )
	{
		char *file_path = gVoiceDownloadCache[gVoiceDownloadCacheCurIndex];
		// Remove the file
		if ( roadmap_file_exists( "", file_path ) )
		{
			roadmap_file_remove( "", file_path );
		}
		free( file_path );
	}

	gVoiceDownloadCache[gVoiceDownloadCacheCurIndex] = strdup( file_path );
	gVoiceDownloadCacheCurIndex = ( gVoiceDownloadCacheCurIndex + 1 ) % VOICE_DOWNLOAD_CACHE_SIZE;
}


/***********************************************************/
/*  Name        : upload_error_callback( int size )
 *  Purpose     : Upload callback: Error
 *
 */
static void download_cache_clear( void )
{
	int i;
	for ( i = 0; i < VOICE_DOWNLOAD_CACHE_SIZE; ++i )
	{
		if ( gVoiceDownloadCache[i] == NULL )
			break;
		// Remove the file
		if ( roadmap_file_exists( "", gVoiceDownloadCache[i] ) )
		{
			roadmap_file_remove( "", gVoiceDownloadCache[i] );
		}
		free( gVoiceDownloadCache[i] );
		gVoiceDownloadCache[i] = NULL;
	}

}

///////////////////////////////////////////////////////
static int upload_file_size_callback( void *context, size_t aSize ) {
	return 1; // no voice is too big for sending.
}

///////////////////////////////////////////////////////
static void upload_progress_callback(void *context, char *data, size_t size) {
}

///////////////////////////////////////////////////////
static void upload_error_callback( void *context, int connection_failure, const char *format, ...) {
	upload_context *  ctx = (upload_context *)context;
	roadmap_log(ROADMAP_ERROR,"error in uploading voice : %s",ctx->full_path);
	(*ctx->cb ) (ctx->context);
	roadmap_path_free(ctx->full_path);
   free(ctx);
}

///////////////////////////////////////////////////////
static void upload_done( void *context, char *last_modified, const char *format, ... ) {
	upload_context * ctx = (upload_context *)context;

	char response_message[500];
	va_list ap;
	if(format){
		va_start(ap, format);
		vsnprintf(response_message,sizeof(response_message),format,ap);
		va_end(ap);
		roadmap_log(ROADMAP_DEBUG,"done uploading audio file : %s. Received response : %s",ctx->full_path,response_message);
	}

	// Parse the answer
	if( !strncmp( VOICE_UPLOAD_RESPONSE_PREFIX, response_message, strlen( VOICE_UPLOAD_RESPONSE_PREFIX ) ) )
	{
		roadmap_log( ROADMAP_DEBUG, "File was uploaded successfully! Response message: %s", response_message );
		// Copy the message ID to the buffer
		memcpy( ctx->voice_id, (response_message + strlen( VOICE_UPLOAD_RESPONSE_PREFIX )), ROADMAP_VOICE_ID_LEN );
		ctx->voice_id[ROADMAP_VOICE_ID_LEN] = 0;
		(*ctx->cb ) (ctx->context);
	}
	else
	{
		ctx->voice_id[0] = '\0';
		roadmap_log( ROADMAP_WARNING, "Voice upload done, received response message: %s", response_message );
		(*ctx->cb) (ctx->context);
	}

	roadmap_path_free(ctx->full_path);
	free(ctx);
}


/***********************************************************/
/*  Name        : get_upload_url( void )
 *  Purpose     : Returns the url constructed from the server and session id
 *
 */
static char* get_upload_url( void )
{
	char* url;
   const char* url_prefix;
   int size;

   url_prefix = roadmap_config_get ( &RMCfgRecorderVoiceServer );

   // Size of prefix +  size of session id + cookie '\0'
   size = strlen( url_prefix ) + 50 + strlen (Realtime_GetServerCookie()) + 1;
   url = malloc( size );

   roadmap_check_allocated( url );

   snprintf(url, size, "%s?sessionid=%d&cookie=%s",
            url_prefix,
            Realtime_GetServerId(),
            Realtime_GetServerCookie());

   return url;
}
