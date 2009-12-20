/* roadmap_image_viewer.m - RoadMap image viewer for iPhone
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi R
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

#include "roadmap_main.h"
#include "roadmap_iphonemain.h"
#include "roadmap_iphoneimage.h"
#include "roadmap_lang.h"
#include "roadmap_image_viewer.h"
#include "roadmap_iphoneimage_viewer.h"

void roadmap_image_viewer_show(const char** images, int count) {
   ImageViewerView *viewerView = [[ImageViewerView alloc] init];
   [viewerView show:images count:count];
}



@implementation ImageViewerView

- (void) viewDidLoad
{
   roadmap_main_reload_bg_image();
}

- (void) nextStage
{
   UIBarButtonItem *button;
   UINavigationItem *navItem = [self navigationItem];
   
   UINavigationBar *navBar = self.navigationController.navigationBar;
   navBar.barStyle = UIBarStyleBlackOpaque;
   navBar.hidden = NO; //workwround for auto showing nutshell after intro
   
   if (gIndex  == 0) {
      button = NULL;
      navItem.hidesBackButton = YES;
   }
   else {
      button = [[UIBarButtonItem alloc] initWithTitle:[NSString stringWithUTF8String:roadmap_lang_get("Back")]
                                                style:UIBarButtonItemStylePlain target:self action:@selector(backButton)];
      navItem.hidesBackButton = NO;
   }
   [navItem setLeftBarButtonItem: button];
	[button release];
   
   if (gIndex < gCount - 1)
      button = [[UIBarButtonItem alloc] initWithTitle:[NSString stringWithUTF8String:roadmap_lang_get("Next")]
                                                                 style:UIBarButtonItemStylePlain target:self action:@selector(nextButton)];
   else
      button = [[UIBarButtonItem alloc] initWithTitle:[NSString stringWithUTF8String:roadmap_lang_get("Done")]
                                                                 style:UIBarButtonItemStyleDone target:self action:@selector(doneButton)];
   
   [navItem setRightBarButtonItem:button];
	[button release];
   
   [navItem setTitle:[NSString stringWithFormat:@"%d of %d", gIndex + 1, gCount]];

   [self showImage];
}

- (void) doneButton
{
   UINavigationBar *navBar = self.navigationController.navigationBar;
   navBar.barStyle = UIBarStyleDefault;
   
   roadmap_main_pop_view(YES);
}


- (void) backButton
{
   gIndex--;
   [self nextStage];
}

- (void) nextButton
{
   gIndex++;
   [self nextStage];
}

- (void) showImage
{
   static int lastIndex = -1;
   UIView *containerView = self.view;

   UIImageView *newImageView = NULL;
   
   UIImage *image = roadmap_iphoneimage_load(gImages[gIndex]);
   
   if (image) {
      newImageView = [[UIImageView alloc] initWithImage:image];
      [image release];
   }
   
   if ([[containerView subviews] count] > 0) {
      [UIView beginAnimations:NULL context:NULL];
		[UIView setAnimationDuration:1.0f];
		[UIView setAnimationCurve:UIViewAnimationCurveEaseInOut];
		
      if (lastIndex < gIndex)
         [UIView setAnimationTransition: UIViewAnimationTransitionCurlUp forView:containerView cache:YES];
      else
         [UIView setAnimationTransition: UIViewAnimationTransitionCurlDown forView:containerView cache:YES];
      
		[[[containerView subviews] objectAtIndex:0] removeFromSuperview];
		
		[containerView addSubview:newImageView];
		
		[UIView commitAnimations];
	} else
		[containerView addSubview:newImageView];
   
   [newImageView release];
   lastIndex = gIndex;
}

- (void) createView
{
   CGRect rect;
   
   rect = [[UIScreen mainScreen] applicationFrame];
	rect.origin.x = 0;
	rect.origin.y = 0;
	rect.size.height = roadmap_main_get_mainbox_height();
	UIView *containerView = [[UIView alloc] initWithFrame:rect];
	self.view = containerView;
	[containerView release]; // decrement retain count
}

- (void) show: (const char **) images
        count: (int) count
{
   gCount = count;
   gImages = images;
   gIndex = -1;
   
   [self createView];
   
   roadmap_main_push_view(self);
   [self nextButton];
}


- (void)dealloc
{

	[super dealloc];
}


@end

