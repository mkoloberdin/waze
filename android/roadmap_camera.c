/* roadmap_camera.c - Android camera API implementation
 *
 * LICENSE:
 *   Copyright 2009, Waze Ltd
 *   Alex Agranovich
 *   Ehud Shabtai
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
 * SYNOPSYS:
 *
 *   See roadmap_camera.h
 */

#include <stdlib.h>
#include <string.h>
#include "roadmap_camera.h"
#include "roadmap_path.h"
#include "roadmap_camera_image.h"
#include "JNI/FreeMapJNI.h"
#include "roadmap_path.h"

// Temporary file for the image capture
static const char* gCaptureFileName = "capture_temp.jpg";

/*
 * JNI Callback context
 */
typedef struct
{
   CameraImageCaptureCallback capture_callback;
   CameraImageCaptureContext* context;
} AndrCameraCbContext;

/*
 * Only one camera capture can be done at a time. So can be assume singelton
 */
static AndrCameraCbContext sgCameraContext;

/***********************************************************
 *  Name        : roadmap_camera_take_picture
 *  Purpose     : Shows the camera preview, passes target image attributes
 *                   and defines the target capture file location
 *
 */
BOOL roadmap_camera_take_picture( CameraImageFile* image_file, CameraImageBuf* image_thumbnail )
{
    int image_quality_percent;  // Image quality in percents
    int retVal, retValThumb = -1;
    int byte_per_pix = roadmap_camera_image_bytes_pp( image_thumbnail->pixfmt );
    // Transform to percent low - 34, medium - 67, high - 100
    image_quality_percent = 34 + 33 * (int) image_file->quality;

    // Set the target path
    CAMERA_IMG_FILE_SET_PATH( *image_file, roadmap_path_user(), gCaptureFileName );

    // Call the JNI for the file
    retVal = FreeMapNativeManager_TakePicture( image_file->width, image_file->height, image_quality_percent,
            image_file->folder, image_file->file );

    // Call the JNI for the thumbnail if requested and the picture was taken successfully
    if ( ( retVal == 0 ) && image_thumbnail )
    {
        // Assume 4 byte per pixel
        // Only int array can be accepted for the Android
        // Malloc SHOULD preserve 4-byte alignment so further casting is possible !!!!!!!
        image_thumbnail->buf = malloc( image_thumbnail->width * image_thumbnail->height * byte_per_pix );
        retValThumb = FreeMapNativeManager_GetThumbnail( image_thumbnail->width, image_thumbnail->height,
                                                                    byte_per_pix, (int*) image_thumbnail->buf );
        if ( retValThumb != 0 )
        {
            roadmap_log( ROADMAP_WARNING, "Thumbnail request to Android is failed" );
        }
    }
    return ( retVal == 0 );
}



/***********************************************************
 *  Name        : roadmap_camera_take_picture_async
 *  Purpose     : Asynchronous function. Main thread still running.
 *                Shows the camera preview, passes target image attributes
 *                   and defines the target capture file location
 *
 */
BOOL roadmap_camera_take_picture_async( CameraImageCaptureCallback callback, CameraImageCaptureContext* context )
{
    int image_quality_percent;  // Image quality in percents
    int retVal, retValThumb = -1;
    CameraImageBuf* image_thumbnail = &context->image_thumbnail;
    CameraImageFile* image_file = &context->image_file;
    // Transform to percent low - 34, medium - 67, high - 100
    image_quality_percent = 34 + 33 * (int) image_file->quality;

    // Set the target path
    CAMERA_IMG_FILE_SET_PATH( *image_file, roadmap_path_user(), gCaptureFileName );

    // Call the JNI for the file
    retVal = FreeMapNativeManager_TakePictureAsync( image_file->width, image_file->height, image_quality_percent,
            image_file->folder, image_file->file );

    sgCameraContext.context = context;
    sgCameraContext.capture_callback = callback;

    return TRUE;
}


void roadmap_camera_take_picture_async_cb( int res )
{
   int retValThumb = -1;

   // Call the JNI for the thumbnail if requested and the picture was taken successfully
   if ( res )
   {
      CameraImageCaptureContext* context = sgCameraContext.context;
      CameraImageBuf* image_thumbnail = &context->image_thumbnail;
      int byte_per_pix = roadmap_camera_image_bytes_pp( image_thumbnail->pixfmt );
     // Assume 4 byte per pixel
     // Only int array can be accepted for the Android
     // Malloc SHOULD preserve 4-byte alignment so further casting is possible !!!!!!!
     image_thumbnail->buf = malloc( image_thumbnail->width * image_thumbnail->height * byte_per_pix );
     retValThumb = FreeMapNativeManager_GetThumbnail( image_thumbnail->width, image_thumbnail->height,
                                                                 byte_per_pix, (int*) image_thumbnail->buf );
     if ( retValThumb != 0 )
     {
         roadmap_log( ROADMAP_WARNING, "Thumbnail request to Android is failed" );
     }
   }
   sgCameraContext.capture_callback( sgCameraContext.context, res );
}

