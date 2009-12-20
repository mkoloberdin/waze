/* roadmap_camera_defs.h - The camera management related definitions
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

#ifndef INCLUDE__ROADMAP_CAMERA_DEFS__H
#define INCLUDE__ROADMAP_CAMERA_DEFS__H
#ifdef __cplusplus
extern "C" {
#endif

// This type represents the quality level for the compressed picture
// low quality = maximum compression
typedef enum
{
    quality_low = 0x0,
    quality_medium,
    quality_high
} CameraImageQuality;

// Camera image file format
typedef enum
{
    file_fmt_JPEG = 0x0,
    file_fmt_PNG,
    file_fmt_BMP,
    file_fmt_RAW
} CameraImageFileFmt;


// Pixel format for camera image buffers
typedef enum
{
	pixel_fmt_bgra_8888 = 0x0,
    pixel_fmt_rgba_8888,
    pixel_fmt_rgb_565,
    pixel_fmt_rgb_888,
    pixel_fmt_jpeg,
    pixel_fmt_png
} CameraImagePixelFmt;

// Camera image file attributes
typedef struct
{
    int width;
    int height;
    CameraImageQuality quality;
    CameraImageFileFmt file_format;
    char* folder;
    char* file;
} CameraImageFile;

// Camera image buffer attributes and pointer to the image data
typedef struct
{
    int width;
    int height;
    CameraImagePixelFmt pixfmt;   // Pixel format
    unsigned char* buf;         // The buffer pointer. Pay attention to the 1-byte alignment
} CameraImageBuf;

// Configuration entries
#define CFG_CATEGORY_CAMERA_IMG         ( "Camera Image" )
#define CFG_ENTRY_CAMERA_IMG_WIDTH      ( "Width" )
#define CFG_ENTRY_CAMERA_IMG_HEIGHT     ( "Height" )
#define CFG_ENTRY_CAMERA_IMG_QUALITY    ( "Quality" )
#define CFG_ENTRY_CAMERA_IMG_SERVER     ( "Web-Service Address" )
#define CFG_ENTRY_CAMERA_IMG_URL_PREFIX ( "Download Prefix" )

// Default configuration values for image ( Portrait 3:4 aspect ratio )
#define CFG_CAMERA_IMG_WIDTH_DEFAULT    320
#define CFG_CAMERA_IMG_HEIGHT_DEFAULT   240
#define CFG_CAMERA_IMG_QUALITY_DEFAULT  1	//quality_medium
#define CFG_CAMERA_IMG_SERVER_DEFAULT   ""
#define CFG_CAMERA_IMG_URL_PREFIX_DEFAULT "http://waze.photos.s3.amazonaws.com/thumbs/thumb347_"

// Camera image thumbnail default values
#define CFG_CAMERA_THMB_WIDTH_DEFAULT    (140)
#define CFG_CAMERA_THMB_HEIGHT_DEFAULT   (105)

// Image ID length (bytes)
#define ROADMAP_IMAGE_ID_LEN              (36)

// Downloaded images file suffix
#define ROADMAP_IMG_DOWNLOAD_FILE_SUFFIX  (".jpg")

// Initializers
#define CAMERA_IMG_FILE_INIT( _this, _width, _height, _quality, _fmt, _folder, _file ) \
{   \
    (_this).width = _width;      \
    (_this).height = _height;    \
    (_this).quality = _quality;  \
    (_this).file_format = _fmt; \
    (_this).folder = _folder;   \
    (_this).file = _file;       \
}

#define CAMERA_IMG_BUF_INIT( _this, _width, _height, _fmt, _buf ) \
{   \
    (_this).width = _width;      \
    (_this).height = _height;    \
    (_this).pixfmt = _fmt;  \
    (_this).buf = _buf; \
}

#define CAMERA_IMG_FILE_SET_PATH( _this, _folder, _file ) \
{   \
    (_this).folder = strdup( _folder );   \
    (_this).file = strdup( _file );       \
}

#define CAMERA_IMG_BUF_SET( _this, _buf ) \
{   \
    (_this).buf = _buf; \
}


#define CAMERA_IMG_FILE_DFLT { CFG_CAMERA_IMG_WIDTH_DEFAULT, CFG_CAMERA_IMG_HEIGHT_DEFAULT, \
                                                        quality_medium, file_fmt_JPEG, "", "" }
#define CAMERA_THMB_BUF_DFLT { CFG_CAMERA_THMB_WIDTH_DEFAULT, CFG_CAMERA_THMB_WIDTH_DEFAULT, \
                                                        pixel_fmt_rgba_8888, NULL }

#ifdef __cplusplus
}
#endif
#endif /* INCLUDE__ROADMAP_CAMERA_DEFS__H */

