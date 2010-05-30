/* roadmap_camera_image.c - Camera image functionality implementation: Configuration, capture, upload and
 *                      file management
 *
 * LICENSE:
 *   Copyright 2009, Waze Ltd
 *   Alex Agranovich
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
 *   See roadmap_camera_image.c
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
#include "roadmap_camera_image.h"
#include "roadmap_camera_defs.h"
#include "roadmap_camera.h"
#include "roadmap_lang.h"
#include "roadmap_path.h"
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_bitmap.h"
#include "ssd/ssd_progress_msg_dialog.h"
#include "roadmap_messagebox.h"
#include "roadmap_canvas.h"
#include "roadmap_httpcopy_async.h"

//======== Local Types ========
typedef struct
{
	char* image_path;
	char* data;
	size_t data_size;
	ImageDownloadCallback download_cb;
	void* context_cb;
} DownloadContext;

//======== Defines ========
#define IMG_UPLOAD_RESPONSE_PREFIX      "image_id="
#define IMG_UPLOAD_SSD_DLG_NAME         "Image upload process"
#define IMG_UPLOAD_PROGRESS_FIELD       "Progress"
#define IMG_UPLOAD_STATUS_FIELD         "Image upload status"
#define IMG_UPLOAD_ERROR                "Image upload error"

#define IMG_UPLOAD_CONTENT_TYPE         "image/jpeg"

#define IMG_DOWNLOAD_CACHE_SIZE			50




//======== Globals ========
// Image width
static int gCameraImageWidth = CFG_CAMERA_IMG_WIDTH_DEFAULT;
// Image height
static int gCameraImageHeight = CFG_CAMERA_IMG_HEIGHT_DEFAULT;
// Image quality
static CameraImageQuality gCameraImageQuality = CFG_CAMERA_IMG_QUALITY_DEFAULT;
// Upload process state
static int gFileSize = 0;
static int gUploadedSize = 0;

static char* gImageDownloadCache[IMG_DOWNLOAD_CACHE_SIZE] = {0};
static int  gImageDownloadCacheCurIndex = 0;

////// Configuration section ///////
static RoadMapConfigDescriptor RMCfgCameraImageWidth =
                        ROADMAP_CONFIG_ITEM( CFG_CATEGORY_CAMERA_IMG, CFG_ENTRY_CAMERA_IMG_WIDTH );
static RoadMapConfigDescriptor RMCfgCameraImageHeight =
                        ROADMAP_CONFIG_ITEM( CFG_CATEGORY_CAMERA_IMG, CFG_ENTRY_CAMERA_IMG_HEIGHT );
static RoadMapConfigDescriptor RMCfgCameraImageQuality =
                        ROADMAP_CONFIG_ITEM( CFG_CATEGORY_CAMERA_IMG, CFG_ENTRY_CAMERA_IMG_QUALITY );
static RoadMapConfigDescriptor RMCfgCameraImageServer =
                        ROADMAP_CONFIG_ITEM( CFG_CATEGORY_CAMERA_IMG, CFG_ENTRY_CAMERA_IMG_SERVER );
static RoadMapConfigDescriptor RMCfgCameraImageUrlPrefix =
                        ROADMAP_CONFIG_ITEM( CFG_CATEGORY_CAMERA_IMG, CFG_ENTRY_CAMERA_IMG_URL_PREFIX );






static int  download_size_callback( void *context_cb, size_t size );
static void download_progress_callback( void *context_cb, char *data, size_t size );
static void download_error_callback( void *context_cb, int connection_failure, const char *format, ... );
static void download_done_callback( void *context_cb,char *last_modified );


static BOOL roadmap_camera_image_uploader( RoadMapDownloadCallbacks *callbacks, const char *image_folder, const char *image_file, char* image_id,CameraImageUploadCallback cb, void * context);
static char* get_download_url( const char* image_id );

static void download_cache_add( const char* file_path );
static void download_cache_clear( void );


static int upload_file_size_callback( void *context, int aSize );
static void upload_progress_callback( void *context, int aLoaded );
static void upload_error_callback( void *context );
static void upload_done( void *context, const char *format, ...);

typedef struct tag_upload_context {
	CameraImageUploadCallback cb;
	void * context;
	char * full_path; // image name, received from RTAlerts.
	char * image_id;  // buffer from RTAlerts, will be filled according to the response from server.
} upload_context;

#if 0
static EditorUploadCallbacks gUploadCallbackFunctionsSSD =
{
	upload_file_size_callback_ssd,
	upload_progress_callback_ssd,
	upload_error_callback_ssd,
	upload_done_ssd
};
#endif

/*
 * These functions will handle the async Image upload.
 */
static EditorUploadCallbacks gUploadCallbackFunctions =
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
/*  Name        : roadmap_camera_image_alert()
 *  Purpose     : Constructs the url and downloads the image. Leaks the memory in the image_file
 *
 *  Params     : (in) image_id - the image id to download
 *
 *             : (out) download_cb - the full path to the downloaded image file - user responsibility to
 *             		deallocate the memory!!!!
 *
 *  Notes      :
 *
 */
BOOL roadmap_camera_image_download( const char* image_id,  void* context_cb, ImageDownloadCallback download_cb )
{
    BOOL res = FALSE;
    char *url;
    char  *image_file, *image_path;
    DownloadContext* context;

    roadmap_log( ROADMAP_DEBUG, "Downloading the image.  ID: %s", image_id );
    
    // Target file name from the image id
    image_file = malloc( strlen( image_id ) + strlen( ROADMAP_IMG_DOWNLOAD_FILE_SUFFIX ) + 1 );
    strcpy( image_file, image_id );
    strcat( image_file, ROADMAP_IMG_DOWNLOAD_FILE_SUFFIX );

    // Target full path for the output
    image_path = roadmap_path_join( roadmap_path_images(), image_file );
    if (roadmap_file_exists( NULL, image_path ) )
    {
    	// Add file to cache
    	download_cache_add( image_path );
    	// The file exists - no need to download
    	download_cb( context_cb, 0, image_path );
    	free( image_path );
    }
    else
    {
      url = get_download_url( image_id );
		// Init the download process context
		context = malloc( sizeof( DownloadContext ) );
		context->image_path = image_path;
		context->context_cb = context_cb;
		context->download_cb = download_cb;
		context->data = NULL;
		// Show the message
		ssd_progress_msg_dialog_show( roadmap_lang_get( "Downloading . . . " ) );
		// Start the process
		roadmap_http_async_copy( &gHttpAsyncCallbacks, context, url ,0 );
		free( url );
    }

	 free( image_file );

    return res;
}

/***********************************************************/
/*  Name        : download_url( const char* image_id )
 *  Purpose     : Returns the url constructed from the given image id
 *
 */
static char* get_download_url( const char* image_id )
{
	char* url;
    const char* url_prefix;

    url_prefix = roadmap_config_get ( &RMCfgCameraImageUrlPrefix );

    // Size of prefix +  size of image id + '\0'
    url = malloc( strlen( url_prefix ) + strlen( image_id ) + 1 );

    roadmap_check_allocated( url );

    strcpy( url, url_prefix );
	strcat( url, image_id );

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
static void download_done_callback( void *context_cb, char *last_modified )
{
	DownloadContext* context = (DownloadContext*) context_cb;
	const char* path = context->image_path;
	RoadMapFile file;

	roadmap_log( ROADMAP_INFO, "Download is finished. Writing %d bytes to the file: %s", context->data_size, path );

	// Save the image to the file
	file = roadmap_file_open( path, "w" );
	if ( !file )
	{
		roadmap_log( ROADMAP_WARNING, "File openning error for file: %s", path );
	}
  
	roadmap_file_write( file, context->data, context->data_size );

	roadmap_file_close(file);

	ssd_progress_msg_dialog_hide();

	context->download_cb( context->context_cb, 0, context->image_path );

	// Add file to cache
	download_cache_add( path );

	// Deallocate the download context
	free( context->data );
	free( context->image_path );
	free( context );
}

/***********************************************************/
/*  Name        : roadmap_camera_image_alert()
 *  Purpose     : Performs the entire process of the taking image for the alert
 *                  1. Captures the image according to the configuration parameters
 *                      Saves the image to the file and returns the buffer containing the thumbnail
 *                  2. Uploads the image to the server
 *  Params     : (out) image_path - the path to the image
 *  					users responsibility to release the path memory
 *             : (out) image_thumbnail - the thumbnail of the taken image
 *
 *  Notes      : The function caller is responsible for the memory deallocation of
 *                  the image id and the image_thumbnail
 */
BOOL roadmap_camera_image_alert( char** image_path,  RoadMapImage *image_thumbnail )
{
    BOOL res = FALSE;
    CameraImageFile image_file;
    CameraImageBuf image_buf_thumbnail;

    static BOOL capture_config_initialized = FALSE;

    // Initialize the configuration
    if ( !capture_config_initialized )
    {
        capture_config_initialized = TRUE;
    }

    if ( !image_path )
    {
    	roadmap_log( ROADMAP_WARNING, "Image path parameter is corrupted!!" );
    	return FALSE;
    }
    if ( !image_thumbnail )
	{
		roadmap_log( ROADMAP_WARNING, "Image thumbnail parameter is corrupted!!" );
		return FALSE;
	}

    *image_path = NULL;
    *image_thumbnail = NULL;

    CAMERA_IMG_BUF_INIT( image_buf_thumbnail, CFG_CAMERA_THMB_WIDTH_DEFAULT, CFG_CAMERA_THMB_HEIGHT_DEFAULT,
							pixel_fmt_bgra_8888, NULL );

    // Init the file attributes
    CAMERA_IMG_FILE_INIT( image_file, gCameraImageWidth, gCameraImageHeight, gCameraImageQuality,
                                file_fmt_JPEG, NULL, NULL );

    if (  roadmap_camera_image_capture( &image_file, &image_buf_thumbnail ) )
    {
        // Make the path
        if ( image_file.file )
        {
        	*image_path = roadmap_path_join( image_file.folder, image_file.file );
        	res = TRUE;
        }
        // Set the thumbnail
        if ( image_buf_thumbnail.buf )
        {
            int image_buf_BPP =  roadmap_camera_image_bytes_pp( image_buf_thumbnail.pixfmt );
            int stride = image_buf_BPP*image_buf_thumbnail.width;
            *image_thumbnail = roadmap_canvas_image_from_buf( image_buf_thumbnail.buf, image_buf_thumbnail.width,
															image_buf_thumbnail.height, stride );
        }

    }
    else
    {
    	roadmap_log( ROADMAP_WARNING, "Camera image was not captured!" );
    }


    // Deallocate if necessary
    if ( image_file.folder )
    	free( image_file.folder );
    if ( image_file.file )
    	free( image_file.file );

    return res;
}


/***********************************************************/
/*  Name        : roadmap_camera_image_capture()
 *  Purpose     : Loads the configuration and takes the picture
 *                  using the device API. The image is saved to the temporary file
 *  Params		: [in/out] image_file - image file attributes. Returns folder and file where the image is stored
 *				: [in/out] image_thumbnail - thumbnail attributes. Returns buffer of the thumbnail in bgra format
 */
BOOL roadmap_camera_image_capture( CameraImageFile *image_file, CameraImageBuf *image_thumbnail )
{
    BOOL res = FALSE;

    roadmap_log( ROADMAP_DEBUG, "Going to take camera image. Width: %d. Height: %d", image_file->width, image_file->height );

    // Capture the image
#ifndef J2ME    
    res = roadmap_camera_take_picture( image_file, image_thumbnail );
#endif    
    if ( !image_thumbnail )
    {
        roadmap_log( ROADMAP_DEBUG, "Image thumbnail structure is not initialized properly" );
    }

    if ( res == FALSE )
    {
        roadmap_log( ROADMAP_WARNING, "Image capture was not taken" );
    }
    return res;
}

/***********************************************************/
/*  Name        : roadmap_camera_image_upload()
 *  Purpose     : Upload the image to the server.
 *  Params      : (in) image_folder
 *  			    : (in) image_file
 *              : (out) image_id - the string containing the resulting image ID, received from the server
 *              			the input buffer must be of size ROADMAP_IMAGE_ID_BUF_LEN
 */
BOOL roadmap_camera_image_upload( const char *image_folder, const char *image_file,
		char* image_id, CameraImageUploadCallback cb, void * context)
{
	BOOL res;

	res = roadmap_camera_image_uploader( &gUploadCallbackFunctions,
											image_folder, image_file, image_id,cb,context  );

	return res;
}





/***********************************************************/
/*  Name        : roadmap_camera_image_uploader()
 *  Purpose     : Upload the image to the server. Auxiliary static function
 *  Params      : (in) image_folder
 *  			: (in) image_file
 *              : (out) image_id - the string containing the resulting image ID, received from the server
 *						the input buffer must be of size ROADMAP_IMAGE_ID_BUF_LEN
 *              : (out) message - parsed response from the server
 */
static BOOL roadmap_camera_image_uploader( RoadMapDownloadCallbacks *callbacks,
		const char *image_folder, const char *image_file, char* image_id, CameraImageUploadCallback cb, void * context)
{
    BOOL res = FALSE;
    char* full_path;
    const char* target_url;
    upload_context *  ctx;

    if( !image_id )
    {
        roadmap_log( ROADMAP_ERROR, "File upload error: image id buffer is not available!!" );
        return FALSE;
    }

    // Get the full path to the file
    full_path = roadmap_path_join( image_folder, image_file );

    // Set the target to upload to
    target_url = roadmap_config_get ( &RMCfgCameraImageServer );

    roadmap_log( ROADMAP_DEBUG, "Uploading file: %s. ", full_path );

    ctx = malloc(sizeof(upload_context));
	 ctx->context = context;
	 ctx->cb = cb;
	 ctx->full_path = full_path;
	 ctx->image_id = image_id;

    // Upload and get the response
    if ( !editor_upload_auto( full_path, &gUploadCallbackFunctions, target_url, IMG_UPLOAD_CONTENT_TYPE, ctx ) )
    {
		  //change to debug and comment.
        roadmap_log( ROADMAP_DEBUG, "Started Async connection for file : %s", full_path );
        res = TRUE;
    }
    else
    {
    	roadmap_log( ROADMAP_WARNING, "File upload error on socket connect %s", full_path );
    	free( full_path );
    	free( ctx );
    	res = FALSE;
    }

    return res;
}



/***********************************************************/
/*  Name        : roadmap_camera_image_inialize()
 *  Purpose     : Initializes the camera image related parameters
 *
 */
void roadmap_camera_image_initialize( void )
{
    /// Declare the configuration entries
    // Image width
    roadmap_config_declare( "preferences", &RMCfgCameraImageWidth, OBJ2STR( CFG_CAMERA_IMG_WIDTH_DEFAULT ), NULL );
    // Image height
    roadmap_config_declare( "preferences", &RMCfgCameraImageHeight, OBJ2STR( CFG_CAMERA_IMG_HEIGHT_DEFAULT ), NULL );
    // Image height
    roadmap_config_declare( "preferences", &RMCfgCameraImageQuality, OBJ2STR( CFG_CAMERA_IMG_QUALITY_DEFAULT ), NULL );
    // Server for image upload
    roadmap_config_declare( "preferences", &RMCfgCameraImageServer, CFG_CAMERA_IMG_SERVER_DEFAULT, NULL );
    // Url prefix for the image download
    roadmap_config_declare( "preferences", &RMCfgCameraImageUrlPrefix, CFG_CAMERA_IMG_URL_PREFIX_DEFAULT, NULL );

    /// Load the values
    gCameraImageWidth = roadmap_config_get_integer( &RMCfgCameraImageWidth );
    gCameraImageHeight = roadmap_config_get_integer( &RMCfgCameraImageHeight );
    gCameraImageQuality = (CameraImageQuality) roadmap_config_get_integer( &RMCfgCameraImageQuality );
}

/***********************************************************/
/*  Name        : roadmap_camera_image_shutdown()
 *  Purpose     : Finalizes the camera image work
 *
 */
void roadmap_camera_image_shutdown()
{
	/* Remove downloaded images from the directory */
	download_cache_clear();
}


int roadmap_camera_image_bytes_pp( CameraImagePixelFmt pix_fmt )
{
    int bytes_pp;
    switch ( pix_fmt )
    {
		case pixel_fmt_bgra_8888:
        case pixel_fmt_rgba_8888:       bytes_pp = 4;   break;
        case pixel_fmt_rgb_565:         bytes_pp = 2;   break;
        case pixel_fmt_rgb_888:         bytes_pp = 3;   break;
        default:    bytes_pp = 1;
    }
    return bytes_pp;
}

/***********************************************************/
/*  Name        : download_cache_add( const char* file_path )
 *  Purpose     : Adds the downloaded file to the cache
 *
 */
static void download_cache_add( const char* file_path )
{

	if ( gImageDownloadCache[gImageDownloadCacheCurIndex] != NULL )
	{
		char *file_path = gImageDownloadCache[gImageDownloadCacheCurIndex];
		// Remove the file
		if ( roadmap_file_exists( "", file_path ) )
		{
			roadmap_file_remove( "", file_path );
		}
		free( file_path );
	}

	gImageDownloadCache[gImageDownloadCacheCurIndex] = strdup( file_path );
	gImageDownloadCacheCurIndex = ( gImageDownloadCacheCurIndex + 1 ) % IMG_DOWNLOAD_CACHE_SIZE;
}


/***********************************************************/
/*  Name        : upload_error_callback( int size )
 *  Purpose     : Upload callback: Error
 *
 */
static void download_cache_clear( void )
{
	int i;
	for ( i = 0; i < IMG_DOWNLOAD_CACHE_SIZE; ++i )
	{
		if ( gImageDownloadCache[i] == NULL )
			break;
		// Remove the file
		if ( roadmap_file_exists( "", gImageDownloadCache[i] ) )
		{
			roadmap_file_remove( "", gImageDownloadCache[i] );
		}
		free( gImageDownloadCache[i] );
		gImageDownloadCache[i] = NULL;
	}

}


static int upload_file_size_callback( void *context, int aSize ){
	return 1; // no image is too big for sending.
}

static void upload_progress_callback( void *context, int aLoaded ){
	//upload_context * ctx = (upload_context *)context;
	//roadmap_log(ROADMAP_ERROR,"sent %d bytes of file %s",aLoaded,ctx->full_path);
}

static void upload_error_callback( void *context ){
	upload_context *  ctx = (upload_context *)context;
	roadmap_log(ROADMAP_ERROR,"error in uploading image : %s",ctx->full_path);
	(*ctx->cb ) (ctx->context);
	free(ctx->full_path);
   free(ctx);
}

static void upload_done( void *context, const char *format, ...){
	upload_context * ctx = (upload_context *)context;

	char response_message[500];
	va_list ap;
	if(format){
		va_start(ap, format);
		vsnprintf(response_message,sizeof(response_message),format,ap);
		va_end(ap);
		roadmap_log(ROADMAP_DEBUG,"done uploading log file : %s. Received response : %s",ctx->full_path,response_message);
	}

	// Parse the answer
	if( !strncmp( IMG_UPLOAD_RESPONSE_PREFIX, response_message, strlen( IMG_UPLOAD_RESPONSE_PREFIX ) ) )
	{
		roadmap_log( ROADMAP_DEBUG, "File was uploaded successfully! Response message: %s", response_message );
		// Copy the message ID to the buffer
		memcpy( ctx->image_id, (response_message + strlen( IMG_UPLOAD_RESPONSE_PREFIX )), ROADMAP_IMAGE_ID_LEN );
		ctx->image_id[ROADMAP_IMAGE_ID_LEN] = 0;
		(*ctx->cb ) (ctx->context);
	}
	else
	{
		ctx->image_id[0] = '\0';
		roadmap_log( ROADMAP_WARNING, "Image upload done, received response message: %s", response_message );
		(*ctx->cb) (ctx->context);
	}

	free(ctx->full_path);
	free(ctx);
}



// **************** Unit test section ******************
// *** Can be removed
#if 0
#include "roadmap_path.h"
void roadmap_camera_image_alert_test()
{
    char *imageId = NULL;
    CameraImageBuf imageBuf;
    RoadMapImage image_bitmap;
    const char* fg_color = NULL;
    const char* bg_color = NULL;
#if (defined(IPHONE) || defined(ANDROID))
    fg_color = "#ffffff";
    bg_color = NULL;
#endif

    int BPP;
    #define TEST_DLG_NAME       "Waze thumbnail test"
    // Take the image capture
    if ( roadmap_camera_image_alert( &imageId, &imageBuf ) )
    {
        // TODO :: Update image id
    }
    BPP =  roadmap_camera_image_bytes_pp( imageBuf.pixfmt );
    image_bitmap = roadmap_canvas_image_from_buf( imageBuf.buf, imageBuf.width,
                                                    imageBuf.height, 4*imageBuf.width );
    //////////////////////////////////////////////////////
    // Show dialog with the image
    {
        SsdWidget group, bitmap;
        SsdWidget dialog = ssd_dialog_new( TEST_DLG_NAME, "Waze thumbnail test", NULL, SSD_CONTAINER_BORDER|SSD_CONTAINER_TITLE|
                                    SSD_DIALOG_FLOAT|SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER|SSD_ROUNDED_CORNERS);
        if ( !dialog )
        {
            roadmap_log( ROADMAP_ERROR, "Error creating upload progress dialog" );
            return;
        }
        group = ssd_container_new( "Image thumbnail container", NULL,
                    SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_WIDGET_SPACE|SSD_END_ROW );
        ssd_widget_set_color( group, NULL, NULL );
        bitmap = ssd_bitmap_image_new( "Image thumbnail bitmap", image_bitmap, SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER );
        ssd_widget_set_color (bitmap, bg_color, bg_color );
        ssd_widget_add( group, bitmap );
        ssd_widget_add( dialog, group);
        // Activate the dialog
        ssd_dialog_activate ( TEST_DLG_NAME, NULL );
        ssd_dialog_draw();
    }
    if ( imageId )
        free( imageId );
}


int editor_upload_auto_test (const char *file_name,
                        RoadMapDownloadCallbacks *callbacks,
                        char **message, const char* custom_target )
{
#define MAX_FILE_CHUNCK 64

    int size;
    int loaded;
    int uploaded;
    char buffer[MAX_FILE_CHUNCK];
    RoadMapFile file;

    file = roadmap_file_open( file_name, "w" );
    roadmap_log( ROADMAP_WARNING, "File name: %s. FD: %d ", file_name, file );

    if ( !ROADMAP_FILE_IS_VALID(file) )
    {
       (*callbacks->error) ("Can't open file: %s\n", file_name);
       return -1;
    }

    size = roadmap_file_length (NULL, file_name);
    roadmap_log( ROADMAP_WARNING, "File length: %d ", size );

    if ( !( *callbacks->size ) ( size ) )
    {
       roadmap_file_close (file);
       return -1;
    }
    roadmap_log( ROADMAP_WARNING, "After size" );

    uploaded = 0;
    (*callbacks->progress) (uploaded);

    roadmap_log( ROADMAP_WARNING, "After progress" );
    loaded = uploaded;

    while ( loaded < size )
    {

       uploaded = roadmap_file_read  (file, buffer, sizeof(buffer));

       if (uploaded <= 0) {
          (*callbacks->error) ("Send error after %d data bytes", loaded);
          goto cancel_upload;
       }
       loaded += uploaded;

       (*callbacks->progress) (loaded);
    }
    roadmap_file_close (file);
    roadmap_log( ROADMAP_WARNING, "After while" );
    *message = "Test message";
    return 0;

 cancel_upload:

    roadmap_file_close (file);
    return -1;
 }
#endif


#if 0
// Image upload with SSD output on progress.
// Has to be implemented again to work with a-sync editor_upload,
// but since there are no calls to this function, removed for now - D.F.


/***********************************************************/
/*  Name        : roadmap_camera_image_upload_ssd()
 *  Purpose     : Upload the image to the server using the ssd based progress
 *  Params      : (in) image_folder
 *  			    : (in) image_file
 *              : (out) image_id - the string containing the resulting image ID, received from the server
 *                  the input buffer must be of size ROADMAP_IMAGE_ID_BUF_LEN
 */
BOOL roadmap_camera_image_upload_ssd( const char *image_folder, const char *image_file, char* image_id, char** message,CameraImageUploadCallback cb, void * context )
{
	BOOL res;

	res = roadmap_camera_image_uploader( &gUploadCallbackFunctionsSSD,
											image_folder, image_file, image_id, message );

	return res;
}


/***********************************************************/
/*  Name        : upload_file_size_callback( int size )
 *  Purpose     : Upload callback: File size
 *
 */
static int upload_file_size_callback( int aSize )
{
    gFileSize = aSize;
    return 1;
}

/***********************************************************/
/*  Name        : upload_progress_callback( int size )
 *  Purpose     : Upload callback: Progress
 *
 */
static void upload_progress_callback ( int aLoaded )
{
    static SsdWidget progressDlg = NULL;
    // Show the dialog
    if ( !progressDlg )
    {
        progressDlg = upload_progress_ssd_dialog();
        if ( !progressDlg )
        {
            roadmap_log( ROADMAP_ERROR, "Error creating upload progress dialog" );
            return;
        }
    }

    // Activate the dialog
    ssd_dialog_activate ( IMG_UPLOAD_SSD_DLG_NAME, NULL);
    ssd_dialog_set_value( IMG_UPLOAD_STATUS_FIELD,
                             roadmap_lang_get ("Uploading data...") );
    ssd_dialog_draw();

    if ( ( gUploadedSize > aLoaded) || !aLoaded )
    {
        ssd_widget_set_value ( progressDlg, IMG_UPLOAD_PROGRESS_FIELD, "0" );
        roadmap_screen_redraw ();
        gUploadedSize = aLoaded;
     }
    else
    {
        char progress_str[10];
        // Nothing to do
        if ( gUploadedSize == aLoaded )
        {
           return;
        }
        // Update the dialog
        gUploadedSize = aLoaded;
        snprintf( progress_str, sizeof(progress_str), "%d", ( 100 * gUploadedSize ) / gFileSize );
        ssd_widget_set_value( progressDlg, IMG_UPLOAD_PROGRESS_FIELD, progress_str );
        ssd_dialog_draw();
    }
    // Close the dialog on finish
    if ( gUploadedSize == gFileSize )
    {
        ssd_dialog_hide( IMG_UPLOAD_SSD_DLG_NAME, 0 );
    }
    roadmap_main_flush ();
}

/***********************************************************/
/*  Name        : upload_progress_dialog( int size )
 *  Purpose     : Creates and initializes the progress dialog for the upload
 *
 */
static SsdWidget upload_progress_ssd_dialog( void )
{
    SsdWidget dialog, group, text;
    // const char* image_upload_icon[] = {"image_upload_icon"};
    const char* fg_color = NULL;
    const char* bg_color = NULL;

#if (defined(IPHONE) || defined(ANDROID))
    fg_color = "#ffffff";
    bg_color = NULL;
#endif

    // Persistent dialog with title Waze
    dialog = ssd_dialog_new( IMG_UPLOAD_SSD_DLG_NAME, "Waze", NULL, SSD_CONTAINER_BORDER|SSD_CONTAINER_TITLE|
                                SSD_DIALOG_FLOAT|SSD_ALIGN_CENTER|SSD_PERSISTENT|
                                SSD_ALIGN_VCENTER|SSD_ROUNDED_CORNERS);

    if ( !dialog )
    {
        roadmap_log( ROADMAP_ERROR, "Error creating upload progress dialog" );
        return NULL;
    }

    // Add the image upload icon
    // TODO:: Ask for the icon !!!!
//    ssd_widget_add  ( dialog, ssd_button_new( "Image upload", "", image_upload_icon, 1,
//                                                                SSD_ALIGN_RIGHT|SSD_END_ROW, NULL) );
    group = ssd_container_new( "Image Upload Container", NULL,
                SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_WIDGET_SPACE|SSD_END_ROW );
    ssd_widget_set_color (group, NULL, NULL);
    // 1st row Label the subject of the dialog
    text =   ssd_text_new( "Label", roadmap_lang_get( "Image upload status" ), -1, SSD_TEXT_LABEL );
    ssd_widget_set_color (text, fg_color, bg_color );
    ssd_widget_add( group, text );
    // Progress status text
    text = ssd_text_new( "Image upload status", "", -1, 0 );
    ssd_widget_set_color (text, fg_color, bg_color );
    ssd_widget_add (group, text);
    ssd_widget_add (dialog, group);

    // 2nd row: progress data
    group = ssd_container_new ( "Progress group", NULL,
                SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_WIDGET_SPACE|SSD_END_ROW);
    ssd_widget_set_color (group, NULL, NULL);
    text = ssd_text_new ("Label", "%", -1, 0);
    ssd_widget_set_color (text, fg_color, bg_color );
    ssd_widget_add (group,text  );

    text = ssd_text_new ("Progress", "", -1, 0);
    ssd_widget_set_color (text, fg_color, bg_color );
    ssd_widget_add (group, text);
    ssd_widget_add (dialog, group);
    return dialog;
}

/***********************************************************/
/*  Name        : upload_error_callback( int size )
 *  Purpose     : Upload callback: Error
 *
 */
static void upload_error_callback ( const char *format, ... )
{
   va_list ap;
   char message[2048];

   va_start( ap, format );
   vsnprintf( message, sizeof(message), format, ap );
   va_end( ap );

   roadmap_messagebox( roadmap_lang_get( IMG_UPLOAD_ERROR ), message );
}
#endif
