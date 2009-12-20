/* roadmap_camera_image.h - The interface for the camera images management functionality
 *
 * LICENSE:
 *
 *   Copyright 2009, Waze Ltd
 *                   Alex Agranovich
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

#ifndef INCLUDE__ROADMAP_CAMERA_IMAGE__H
#define INCLUDE__ROADMAP_CAMERA_IMAGE__H

#include "roadmap_camera_defs.h"
#include "roadmap_canvas.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef void (*ImageDownloadCallback) ( void* context, int status, const char* image_path );

// Image ID buf length (bytes)
#define ROADMAP_IMAGE_ID_BUF_LEN	( ROADMAP_IMAGE_ID_LEN + 1 )

BOOL roadmap_camera_image_alert( char** image_path,  RoadMapImage *image_thumbnail );

BOOL roadmap_camera_image_capture( CameraImageFile *image_file, CameraImageBuf *image_thumbnail );

BOOL roadmap_camera_image_upload( const char *image_folder, const char *image_file, char* image_id, char** message );

BOOL roadmap_camera_image_upload_ssd( const char *image_folder, const char *image_file, char* image_id, char** message );

int roadmap_camera_image_bytes_pp( CameraImagePixelFmt pix_fmt );

BOOL roadmap_camera_image_download( const char* image_id,  void* context_cb, ImageDownloadCallback download_cb );

void roadmap_camera_image_initialize( void );

void roadmap_camera_image_shutdown( void );

#ifdef __cplusplus
}
#endif
#endif /* INCLUDE__ROADMAP_CAMERA_IMAGE__H */

