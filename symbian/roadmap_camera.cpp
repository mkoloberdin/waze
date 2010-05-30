/* roadmap_camera.cpp - Symbian camera API implementation
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

#include "FreeMapAppUi.h"
#include "WazeCameraEngine.h"
#include "GSConvert.h"
#include <string.h>
#include <stdlib.h>
#include <eikenv.h>
extern "C" {
#include "roadmap_camera.h"
#include "roadmap_path.h"
#include "roadmap_camera_image.h"
}

// Temporary file for the image capture
static const char* gCaptureFileName = "capture_temp.jpg";


/***********************************************************
 *  Name        : roadmap_camera_take_picture
 *  Purpose     : Shows the camera preview, passes target image attributes
 *                   and defines the target capture file location
 *
 */
EXTERN_C BOOL roadmap_camera_take_picture( CameraImageFile* image_file, CameraImageBuf* image_thumbnail )
{
#ifdef __CAMERA_ENABLED__
	TFileName filePath;
	TBool retValThumb = ETrue;
	// Image Size
	TSize capture_size( image_file->width, image_file->height );
	// App UI framework
	CFreeMapAppUi* pAppUi = static_cast<CFreeMapAppUi*> (  CEikonEnv::Static()->EikAppUi() );
	// Camera engine
	CWazeCameraEngine &engine = pAppUi->GetCameraEngine();	
	
	// Bytes per pixel allocation
	int byte_per_pix = roadmap_camera_image_bytes_pp( image_thumbnail->pixfmt );	
	
	char* full_path;
	
	// AGA NOTE. Only PORTRAIT for now
	if ( image_file->width > image_file->height )
	{
		capture_size.SetSize( image_file->height, image_file->width );
	}
	
	// Set the target path
	CAMERA_IMG_FILE_SET_PATH( *image_file, roadmap_path_first( "maps" ), gCaptureFileName );
	full_path = roadmap_path_join( roadmap_path_first( "maps" ), gCaptureFileName );
	GSConvert::CharPtrToTDes16( full_path, filePath );
	free( full_path );
	
	// Start the engine
	pAppUi->TakePicture( JpegQualityMedium, filePath, capture_size );

	if ( image_thumbnail )
    {
        // Assume 4 byte per pixel
        // Only int array can be accepted for the Android
        // Malloc SHOULD preserve 4-byte alignment so further casting is possible !!!!!!!
		int size = image_thumbnail->width * image_thumbnail->height * byte_per_pix;
        image_thumbnail->buf = static_cast<TUint8*>(  User::AllocL( size ) );	// Will be released later
        TPtr8 memPtr( image_thumbnail->buf, size, size );

    	// Get the thumbnail from the capture
    	retValThumb = engine.GetThumbnail( &memPtr, filePath, TSize( image_thumbnail->width, image_thumbnail->height ) );    

    	if ( !retValThumb )
        {
            roadmap_log( ROADMAP_WARNING, "Thumbnail request to Camera engine is failed" );
        }
    }
#endif //__CAMERA_ENABLED__
    return TRUE;
}

