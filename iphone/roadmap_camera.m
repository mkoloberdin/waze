/* rodamap_camera.m - iphone camera implementation.
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi R.
 *   Copyright 2009, Waze Ltd
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
 *
 * SYNOPSYS:
 *
 *   See roadmap_camera.h
 */


#include "roadmap_main.h"
#include "roadmap_messagebox.h"
#include "roadmap_iphonemain.h"
#include "roadmap_iphonecamera.h"

void roadmap_camera_take_alert_picture(  CGSize image_size, CameraImageQuality image_quality,
								 const char* image_folder, const char* image_file )
{
   if (![RoadMapCameraView isSourceTypeAvailable:UIImagePickerControllerSourceTypeCamera])
	{
      roadmap_messagebox("Oops", "Camera is not available");
		return;
	}
   
	RoadMapCameraView *imagePicker = [[RoadMapCameraView alloc] init];
	
	[imagePicker setSourceType:UIImagePickerControllerSourceTypeCamera];
	[imagePicker takePictureWithSize: image_size
                         andQuality: image_quality
                          andFolder: image_folder
                        andFileName: image_file];
	
	[imagePicker release];
}

@implementation RoadMapCameraView

- (void) takePictureWithSize: (CGSize) image_size
					   andQuality: (CameraImageQuality) image_quality
                   andFolder: (const char*) image_folder
					  andFileName: (const char*) image_file
{
	gImageSize = image_size;
	gImageQuality = image_quality;
	gImageFolder = image_folder;
	gImageFileName = image_file;
	
	[self setDelegate:self];
	
	roadmap_main_present_modal(self);
}


//UIImagePickerController delegate
- (void)imagePickerController:(UIImagePickerController *)picker didFinishPickingImage:(UIImage *)image editingInfo:(NSDictionary *)editingInfo
{
   CGSize newSize;
   
	//Scale image
   if (image.size.width < image.size.height) {
      newSize = gImageSize;
   } else {
      newSize.width = gImageSize.height;
      newSize.height = gImageSize.width;
   }

	
	UIGraphicsBeginImageContext( newSize );
	[image drawInRect:CGRectMake(0,0,newSize.width,newSize.height)];
	UIImage* newImage = UIGraphicsGetImageFromCurrentImageContext();
	UIGraphicsEndImageContext();
		
	//Get JPEG
	CGFloat quality;
	switch (gImageQuality) {
		case quality_low:
			quality = 0.1f;
			break;
		case quality_medium:
			quality = 0.5f;
			break;
		case quality_high:
			quality = 1.0f;
			break;
		default:
			quality = 0.5f;
			break;
	}
	
	NSData *jpgData = UIImageJPEGRepresentation(newImage, quality);
	
	//Save image
	NSString *fileName = [NSString stringWithFormat:@"%s%s", gImageFolder, gImageFileName]; //Assuming file name is with extension
	[jpgData writeToFile:fileName atomically:NO];
	
	roadmap_main_dismiss_modal();
}

- (void)imagePickerControllerDidCancel:(UIImagePickerController *)picker
{
	roadmap_main_dismiss_modal();
}

- (void)dealloc
{
	
	[super dealloc];
}

@end
