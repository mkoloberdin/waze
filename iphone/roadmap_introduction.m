/* roadmap_introduction.m - introduction screens
 *
 * LICENSE:
 *
 *   Copyright 2009, Waze Ltd
 *   Avi R.
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
 */

#include "roadmap.h"
#include "roadmap_main.h"
#include "roadmap_iphonemain.h"
#include "roadmap_iphoneimage.h"
#include "roadmap_help.h"
#include "roadmap_geo_location_info.h"
#include "roadmap_lang.h"

#include "roadmap_introduction.h"
#include "roadmap_iphoneintroduction.h"


#define START_WAZE_Y   375
#define FIRST_BUTTON_Y 140


//Is introduction available?
int roadmap_introduction_is_available(void) {
   const char* system_lang = roadmap_lang_get( "lang");
   
   if(0 == strcmp( system_lang, "Hebrew"))
      return 0; //no intro for Hebrew
   else
      return 1;
}

//Show the introduction screen
void roadmap_introduction_show(void) {
   if (roadmap_introduction_is_available()) {
      IntroductionDialog *dialog = [[IntroductionDialog alloc] init];
      [dialog show];
   } else {
      roadmap_main_show_root(NO);
   }
}

//Show the introduction screen and auto start nutshell screen
void roadmap_introduction_show_auto(void) {
   if (roadmap_introduction_is_available()) {
      IntroductionDialog *dialog = [[IntroductionDialog alloc] init];
      [dialog showAuto];
   } else {
      roadmap_main_show_root(NO);
   }
}


@implementation IntroductionDialog

- (void)viewWillAppear:(BOOL)animated
{
   UINavigationController *navCont = self.navigationController;
   navCont.navigationBar.hidden = YES;
   
   [super viewWillAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated
{
   UINavigationController *navCont = self.navigationController;
   navCont.navigationBar.hidden = NO;
   
   [super viewWillDisappear:animated];
}

- (void) onNutshell
{
   roadmap_help_nutshell();
}

- (void) onGuidedTour
{
   roadmap_help_guided_tour();
}

- (void) onGeoInfo
{
   roadmap_geo_location_info_show();
}

- (void) onStartWaze
{
   roadmap_main_show_root(NO);
}

- (void)viewDidAppear:(BOOL)animated
{
	[super viewDidAppear:animated];
   
   if (autoShowNutshell) {
      autoShowNutshell = 0;
      [self onNutshell];
   }
}

- (void) showIntro
{
   UIImage *image;
   UIButton *button;
   UIImageView *imageView;
   UIView *containerView;
   CGRect rect;
   int posY = FIRST_BUTTON_Y;
   
   rect = [[UIScreen mainScreen] applicationFrame];
	containerView = [[UIView alloc] initWithFrame:rect];
	self.view = containerView;
	[containerView release]; // decrement retain count
   
   if (roadmap_main_get_platform() == ROADMAP_MAIN_PLATFORM_IPAD)
      image = roadmap_iphoneimage_load("intro_background-ipad");
   else
      image = roadmap_iphoneimage_load("intro_background");
   if (image) {
      imageView = [[UIImageView alloc] initWithImage:image];
      [image release];
      [containerView addSubview:imageView];
      [imageView release];
   }
   
   //nutshell button
   button = [UIButton buttonWithType:UIButtonTypeCustom];
   image = roadmap_iphoneimage_load("nutshell_btn");
   if (image) {
      [button setImage:image forState:UIControlStateNormal];
      [image release];
   }
   image = roadmap_iphoneimage_load("nutshell_btn_press");
   if (image) {
      [button setImage:image forState:UIControlStateHighlighted];
      [image release];
   }
   [button addTarget:self action:@selector(onNutshell) forControlEvents:UIControlEventTouchUpInside];
   [button sizeToFit];
   rect = button.frame;
   rect.origin.y = posY;
   rect.origin.x = (containerView.bounds.size.width - button.bounds.size.width) /2;
   button.frame = rect;
   [containerView addSubview:button];
   posY += button.frame.size.height;
   
   //guided tour button
   button = [UIButton buttonWithType:UIButtonTypeCustom];
   image = roadmap_iphoneimage_load("guided_tour_btn");
   if (image) {
      [button setImage:image forState:UIControlStateNormal];
      [image release];
   }
   image = roadmap_iphoneimage_load("guided_tour_btn_press");
   if (image) {
      [button setImage:image forState:UIControlStateHighlighted];
      [image release];
   }
   [button addTarget:self action:@selector(onGuidedTour) forControlEvents:UIControlEventTouchUpInside];
   [button sizeToFit];
   rect = button.frame;
   rect.origin.y = posY;
   rect.origin.x = (containerView.bounds.size.width - button.bounds.size.width) /2;
   button.frame = rect;
   [containerView addSubview:button];
   posY += button.frame.size.height;
   
   /*
   //geo info button
   button = [UIButton buttonWithType:UIButtonTypeCustom];
   image = roadmap_iphoneimage_load("geo_info_btn");
   if (image) {
      [button setImage:image forState:UIControlStateNormal];
      [image release];
   }
   image = roadmap_iphoneimage_load("geo_info_btn_press");
   if (image) {
      [button setImage:image forState:UIControlStateHighlighted];
      [image release];
   }
   [button addTarget:self action:@selector(onGeoInfo) forControlEvents:UIControlEventTouchUpInside];
   [button sizeToFit];
   rect = button.frame;
   rect.origin.y = posY;
   rect.origin.x = (containerView.bounds.size.width - button.bounds.size.width) /2;
   button.frame = rect;
   [containerView addSubview:button];
   posY += button.frame.size.height;
   */
   
   //start waze button
   button = [UIButton buttonWithType:UIButtonTypeCustom];
   image = roadmap_iphoneimage_load("start_waze");
   if (image) {
      [button setImage:image forState:UIControlStateNormal];
      [image release];
   }
   image = roadmap_iphoneimage_load("start_waze_press");
   if (image) {
      [button setImage:image forState:UIControlStateHighlighted];
      [image release];
   }
   [button addTarget:self action:@selector(onStartWaze) forControlEvents:UIControlEventTouchUpInside];
   [button sizeToFit];
   rect = button.frame;
   rect.origin.y = START_WAZE_Y;
   rect.origin.x = (containerView.bounds.size.width - button.bounds.size.width) /2;
   button.frame = rect;
   [containerView addSubview:button];
   posY += button.frame.size.height;
   
   
   
   roadmap_main_push_view(self);
}

- (void) show
{
   autoShowNutshell = 0;
   [self showIntro];
}

- (void) showAuto
{
   autoShowNutshell = 1;
   [self showIntro];
}

@end

