/* roadmap_geo_location_iphone.m - iPhone geo location dialog
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi R.
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
 *   See roadmap_geo_location_info.h
 */


#include "roadmap_main.h"
#include "roadmap_iphonemain.h"
#include "roadmap_lang.h"
#include "roadmap_iphoneimage.h"
#include "roadmap_device_events.h"

#include "roadmap_geo_location_info.h"
#include "roadmap_geo_location_iphone.h"



#define FONT_SIZE_NORM 18
#define FONT_SIZE_METRO 24
#define LABEL_HEIGHT 30
#define METRO_HEIGHT 45
#define X_MARGIN 40
#define VALUE_WIDTH 80
#define BUTTON_WIDTH 120
#define BUTTON_HEIGHT 30

void roadmap_geo_location_iphone(const char *metro,
                                 const char *state,
                                 const char *map_score,
                                 const char * traffic_score,
                                 const char * usage_score,
                                 int overall_score)
{
	GeoInfoView *view = [[GeoInfoView alloc] init];
	[view showWithMetro: metro
                 State:state
              MapScore:map_score
          TrafficScore:traffic_score
            UsageScore:usage_score
          OverallScore:overall_score];
}




@implementation GeoInfoView

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
   return roadmap_main_should_rotate (interfaceOrientation);
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
   roadmap_device_event_notification( device_event_window_orientation_changed);
}


- (void) onLetMeIn {
   roadmap_main_pop_view(YES);
}

- (void)viewWillAppear:(BOOL)animated
{
	UIScrollView *scrollView = (UIScrollView *) self.view;
	CGFloat viewPosY = 5.0f;
	
	CGFloat progressBarWidth;
	CGFloat progressBarPosX;
	
	UIImage *image = NULL;
   UIImageView *bgView = NULL;
	NSString *text;
	UILabel *label;
	CGRect rect;
	UISlider *sliderView = NULL;
	UIButton *button = NULL;
	
	//background frame
	CGFloat navHeight = self.navigationController.navigationBar.frame.size.height;
	rect =  CGRectMake (5, 10, 310 /*scrollView.bounds.size.width - 10*/, 
						roadmap_main_get_mainbox_height()/* - navHeight - 6*/);
	image = roadmap_iphoneimage_load("comments_alert");
	if (image) {
		UIImage *strechedImage = [image stretchableImageWithLeftCapWidth:20 topCapHeight:20];
		bgView = [[UIImageView alloc] initWithImage:strechedImage];
		[image release];
		[bgView setFrame:rect];
		[scrollView addSubview:bgView];
		[bgView release];
	}
	
	//metro title
	text = [NSString stringWithUTF8String:g_geo_info.metro];
	text = [text stringByAppendingString:[NSString stringWithUTF8String:roadmap_lang_get(" Metro")]];
	rect = CGRectMake(10, viewPosY, [bgView frame].size.width - 20, METRO_HEIGHT);
	label = [[UILabel alloc] initWithFrame:rect];
	[label setText:text];
	[label setFont:[UIFont boldSystemFontOfSize:FONT_SIZE_METRO]];
	[label setAdjustsFontSizeToFitWidth:YES];
	label.textAlignment = UITextAlignmentCenter;
	[scrollView addSubview:label];
	[label release];
	viewPosY += METRO_HEIGHT;
	
	//status title
	text = [NSString stringWithUTF8String:roadmap_lang_get("Status:")];
	rect = CGRectMake(X_MARGIN, viewPosY, [bgView frame].size.width/2 - X_MARGIN, LABEL_HEIGHT);
	label = [[UILabel alloc] initWithFrame:rect];
	[label setText:text];
	[label setFont:[UIFont systemFontOfSize:FONT_SIZE_NORM]];
	[label setAdjustsFontSizeToFitWidth:YES];
	label.textAlignment = UITextAlignmentRight;
	[scrollView addSubview:label];
	[label release];
	//status value
	text = [NSString stringWithUTF8String:g_geo_info.state];
	rect.origin.x +=  [bgView frame].size.width/2 - X_MARGIN;
	label = [[UILabel alloc] initWithFrame:rect];
	[label setText:text];
	[label setFont:[UIFont systemFontOfSize:FONT_SIZE_NORM]];
	[label setTextColor:[UIColor colorWithRed:0.102f green:0.408f blue:0.647f alpha:1.0f]];
	[label setAdjustsFontSizeToFitWidth:YES];
	label.textAlignment = UITextAlignmentLeft;
	[scrollView addSubview:label];
	[label release];
	viewPosY += LABEL_HEIGHT;
	
	//Spacer - should be added here
	viewPosY += 10;
	
	//what you get title
	text = [NSString stringWithUTF8String:roadmap_lang_get("What you get:")];
	rect = CGRectMake(X_MARGIN, viewPosY, [bgView frame].size.width - 2*X_MARGIN, LABEL_HEIGHT);
	label = [[UILabel alloc] initWithFrame:rect];
	[label setText:text];
	[label setFont:[UIFont boldSystemFontOfSize:FONT_SIZE_NORM]];
	[label setAdjustsFontSizeToFitWidth:YES];
	label.textAlignment = UITextAlignmentLeft;
	[scrollView addSubview:label];
	[label release];
	viewPosY += LABEL_HEIGHT;
	
	//Navigable title
	text = [NSString stringWithUTF8String:roadmap_lang_get("Navigable map:")];
	rect = CGRectMake(X_MARGIN, viewPosY, [bgView frame].size.width - 2*X_MARGIN -  VALUE_WIDTH, LABEL_HEIGHT);
	label = [[UILabel alloc] initWithFrame:rect];
	[label setText:text];
	[label setFont:[UIFont systemFontOfSize:FONT_SIZE_NORM]];
	//[label setBackgroundColor:[UIColor grayColor]];
	[label setAdjustsFontSizeToFitWidth:YES];
	label.textAlignment = UITextAlignmentLeft;
	[scrollView addSubview:label];
	[label release];
	//Navigable value
	text = [NSString stringWithUTF8String:g_geo_info.map_score];
	rect.origin.x +=  rect.size.width;
	rect.size.width = VALUE_WIDTH;
	label = [[UILabel alloc] initWithFrame:rect];
	[label setText:text];
	[label setFont:[UIFont systemFontOfSize:FONT_SIZE_NORM]];
	//[label setBackgroundColor:[UIColor redColor]];
	[label setTextColor:[UIColor colorWithRed:0.102f green:0.408f blue:0.647f alpha:1.0f]];
	[label setAdjustsFontSizeToFitWidth:YES];
	label.textAlignment = UITextAlignmentRight;
	[scrollView addSubview:label];
	[label release];
	viewPosY += LABEL_HEIGHT;
	
	//Traffic title
	text = [NSString stringWithUTF8String:roadmap_lang_get("Real-Time traffic:")];
	rect = CGRectMake(X_MARGIN, viewPosY, [bgView frame].size.width - 2*X_MARGIN -  VALUE_WIDTH, LABEL_HEIGHT);
	label = [[UILabel alloc] initWithFrame:rect];
	[label setText:text];
	[label setFont:[UIFont systemFontOfSize:FONT_SIZE_NORM]];
	[label setAdjustsFontSizeToFitWidth:YES];
	label.textAlignment = UITextAlignmentLeft;
	[scrollView addSubview:label];
	[label release];
	//Traffic value
	text = [NSString stringWithUTF8String:g_geo_info.traffic_score];
	rect.origin.x +=  rect.size.width;
	rect.size.width = VALUE_WIDTH;
	label = [[UILabel alloc] initWithFrame:rect];
	[label setText:text];
	[label setFont:[UIFont systemFontOfSize:FONT_SIZE_NORM]];
	[label setTextColor:[UIColor colorWithRed:0.102f green:0.408f blue:0.647f alpha:1.0f]];
	[label setAdjustsFontSizeToFitWidth:YES];
	label.textAlignment = UITextAlignmentRight;
	[scrollView addSubview:label];
	[label release];
	viewPosY += LABEL_HEIGHT;
	
	//Usage title
	text = [NSString stringWithUTF8String:roadmap_lang_get("Community alerts:")];
	rect = CGRectMake(X_MARGIN, viewPosY, [bgView frame].size.width - 2*X_MARGIN -  VALUE_WIDTH, LABEL_HEIGHT);
	label = [[UILabel alloc] initWithFrame:rect];
	[label setText:text];
	[label setFont:[UIFont systemFontOfSize:FONT_SIZE_NORM]];
	[label setAdjustsFontSizeToFitWidth:YES];
	label.textAlignment = UITextAlignmentLeft;
	[scrollView addSubview:label];
	[label release];
	//Usage value
	text = [NSString stringWithUTF8String:g_geo_info.usage_score];
	rect.origin.x +=  rect.size.width;
	rect.size.width = VALUE_WIDTH;
	label = [[UILabel alloc] initWithFrame:rect];
	[label setText:text];
	[label setFont:[UIFont systemFontOfSize:FONT_SIZE_NORM]];
	[label setTextColor:[UIColor colorWithRed:0.102f green:0.408f blue:0.647f alpha:1.0f]];
	[label setAdjustsFontSizeToFitWidth:YES];
	label.textAlignment = UITextAlignmentRight;
	[scrollView addSubview:label];
	[label release];
	viewPosY += LABEL_HEIGHT;
	
	//Spacer - should be added here
	viewPosY += 10;
	
	progressBarWidth = bgView.frame.size.width - 2*20;
	// 0% title
	text = [NSString stringWithUTF8String:roadmap_lang_get("0%")];
	label = [[UILabel alloc] init];
	[label setText:text];
	[label setFont:[UIFont systemFontOfSize:FONT_SIZE_NORM]];
	[label sizeToFit];
	rect = CGRectMake(20, viewPosY, label.frame.size.width, LABEL_HEIGHT);
	label.frame = rect;
	[scrollView addSubview:label];
	[label release];
	progressBarPosX = 20.0f + rect.size.width;
	progressBarWidth -= rect.size.width;
	
	// 100% title
	text = [NSString stringWithUTF8String:roadmap_lang_get("100%")];
	label = [[UILabel alloc] init];
	[label setText:text];
	[label setFont:[UIFont systemFontOfSize:FONT_SIZE_NORM]];
	[label sizeToFit];
	rect = CGRectMake(bgView.frame.size.width - 20 - label.frame.size.width, viewPosY, label.frame.size.width, LABEL_HEIGHT);
	label.frame = rect;
	[scrollView addSubview:label];
	[label release];
	progressBarWidth -= rect.size.width;
	
	//progress bar
	rect = CGRectMake(progressBarPosX, viewPosY, progressBarWidth, LABEL_HEIGHT);
	sliderView = [[UISlider alloc] initWithFrame:rect];
	sliderView.maximumValue = 100;
	image = roadmap_iphoneimage_load("progress_wazzy");
	if (image)
		[sliderView setThumbImage:image forState:UIControlStateNormal];
	[image release];
	image = roadmap_iphoneimage_load("progress_left_fill");
	if (image) {
		[sliderView setMinimumTrackImage:[image stretchableImageWithLeftCapWidth:image.size.width-1 topCapHeight:0] forState:UIControlStateNormal];
		[image release];
	}
	image = roadmap_iphoneimage_load("progress_right");
	if (image) {
		[sliderView setMaximumTrackImage:[image stretchableImageWithLeftCapWidth:1 topCapHeight:0] forState:UIControlStateNormal];
		[image release];
	}
	[sliderView setValue:g_geo_info.overall_score animated:YES]; //TODO: move this to did appear
	sliderView.userInteractionEnabled = NO;
	[scrollView addSubview:sliderView];
	[sliderView release];
	viewPosY += LABEL_HEIGHT;
	
	//Early Building
	text = [NSString stringWithUTF8String:roadmap_lang_get("Early Building")];
	rect = CGRectMake(10, viewPosY, VALUE_WIDTH, LABEL_HEIGHT * 2);
	label = [[UILabel alloc] initWithFrame:rect];
	[label setText:text];
	[label setFont:[UIFont systemFontOfSize:FONT_SIZE_NORM]];
	//[label setBackgroundColor:[UIColor grayColor]];
	[label setAdjustsFontSizeToFitWidth:YES];
	label.textAlignment = UITextAlignmentCenter;
	label.numberOfLines = 2;
	[scrollView addSubview:label];
	[label release];
	
	//Partial Value
	text = [NSString stringWithUTF8String:roadmap_lang_get("Partial Value")];
	rect = CGRectMake((bgView.frame.size.width - VALUE_WIDTH)/2, viewPosY, VALUE_WIDTH, LABEL_HEIGHT * 2);
	label = [[UILabel alloc] initWithFrame:rect];
	[label setText:text];
	[label setFont:[UIFont systemFontOfSize:FONT_SIZE_NORM]];
	[label setAdjustsFontSizeToFitWidth:YES];
	label.textAlignment = UITextAlignmentCenter;
	label.numberOfLines = 2;
	[scrollView addSubview:label];
	[label release];
	
	//Full Value
	text = [NSString stringWithUTF8String:roadmap_lang_get("Full Value")];
	rect = CGRectMake([bgView frame].size.width - VALUE_WIDTH - 10, viewPosY, VALUE_WIDTH, LABEL_HEIGHT * 2);
	label = [[UILabel alloc] initWithFrame:rect];
	[label setText:text];
	[label setFont:[UIFont systemFontOfSize:FONT_SIZE_NORM]];
	[label setAdjustsFontSizeToFitWidth:YES];
	label.textAlignment = UITextAlignmentCenter;
	label.numberOfLines = 2;
	[scrollView addSubview:label];
	[label release];
	viewPosY += LABEL_HEIGHT * 2;
	
	//Spacer - should be added here
	viewPosY += 10;
	
	//let me in button
	button = [UIButton buttonWithType:UIButtonTypeCustom];
	[button setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Let me in")] forState:UIControlStateNormal];
	image = roadmap_iphoneimage_load("button_up");
	if (image) {
		[button setBackgroundImage:[image stretchableImageWithLeftCapWidth:20 topCapHeight:10] forState:UIControlStateNormal];
		[image release];
	}
	image = roadmap_iphoneimage_load("button_down");
	if (image) {
		[button setBackgroundImage:[image stretchableImageWithLeftCapWidth:20 topCapHeight:10] forState:UIControlStateHighlighted];
		[image release];
	}
	rect = CGRectMake((bgView.frame.size.width - BUTTON_WIDTH)/2, viewPosY, BUTTON_WIDTH, BUTTON_HEIGHT);
	[button setFrame:rect];
	[button addTarget:self action:@selector(onLetMeIn) forControlEvents:UIControlEventTouchUpInside];
	[scrollView addSubview:button];
	
	
   viewPosY += button.frame.size.height + 40;
   rect = bgView.bounds;
   rect.size.height = viewPosY;
   bgView.bounds = rect;  
	[scrollView setContentSize:CGSizeMake(bgView.frame.size.width, viewPosY)];
	

	[super viewWillAppear:animated];
}

- (void) showWithMetro: (const char *)metro
                 State: (const char *)state
              MapScore: (const char *)map_score
          TrafficScore: (const char *)traffic_score
            UsageScore: (const char *)usage_score
          OverallScore: (int)overall_score
{		
	UIScrollView *scrollView = [[UIScrollView alloc] initWithFrame:CGRectZero];
	self.view = scrollView;
	[scrollView release]; // decrement retain count
	[scrollView setDelegate:self];
	[self setTitle:[NSString stringWithUTF8String:roadmap_lang_get ("waze status in your area")]];
	UINavigationItem *navItem = [self navigationItem];
	[navItem setHidesBackButton:YES];
	
	strcpy (g_geo_info.metro, metro);
	strcpy (g_geo_info.state, state);
	strcpy (g_geo_info.map_score, map_score);
	strcpy (g_geo_info.traffic_score, traffic_score);
	strcpy (g_geo_info.usage_score, usage_score);
	g_geo_info.overall_score = overall_score;
	
	roadmap_main_push_view(self);
}


- (void)dealloc
{
	[super dealloc];
}


@end


