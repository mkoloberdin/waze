/* roadmap_iphonecamera.h - iPhone camera view
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi R.
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

#ifndef __ROADMAP_IPHONECAMERA__H
#define __ROADMAP_IPHONECAMERA__H

#include "roadmap_camera_defs.h"

void roadmap_camera_take_alert_picture(  CGSize image_size, CameraImageQuality image_quality,
                                       const char* image_folder, const char* image_file );

@interface RoadMapCameraView : UIImagePickerController <UIImagePickerControllerDelegate ,UINavigationControllerDelegate> {
	CGSize gImageSize;
	CameraImageQuality gImageQuality;
	const char* gImageFolder;
	const char* gImageFileName;
}


- (void) takePictureWithSize: (CGSize) image_size
                  andQuality: (CameraImageQuality) image_quality
                   andFolder: (const char*) image_folder
                 andFileName: (const char*) image_file;

@end


#endif // __ROADMAP_IPHONECAMERA__H