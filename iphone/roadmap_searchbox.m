/* roadmap_searchbox.m - Handle iphone search box
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi R
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
 */

#include "roadmap_main.h"
#include "roadmap_iphonemain.h"
#include "../roadmap_device_events.h"
#include "roadmap_searchbox.h"
#include "roadmap_iphonesearchbox.h"


void ShowSearchbox (const char* aTitleUtf8, 
				   const char* aTextUtf8, 
				   SsdKeyboardCallback callback, 
				   void *context) {
	
	RoadMapSearchBox *RoadMapSearchViewCont = [[RoadMapSearchBox alloc] init];
	
	[RoadMapSearchViewCont showWithTitle:aTitleUtf8
								 andText:aTextUtf8 
							 andCallback:callback 
							  andContext:context];
	
}



@implementation RoadMapSearchBox

@synthesize gContext;
@synthesize gCallback;
@synthesize gSearchView;


- (void)viewDidAppear:(BOOL)animated
{
   [gSearchView becomeFirstResponder];
   	
	[super viewDidAppear:animated];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
   return roadmap_main_should_rotate (interfaceOrientation);
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
   roadmap_device_event_notification( device_event_window_orientation_changed);
}

- (void) showWithTitle: (const char*) aTitleUtf8
               andText: (const char*) aTextUtf8
           andCallback: (SsdKeyboardCallback) callback
            andContext: (void *)context
{
	[self setTitle:[NSString stringWithUTF8String:aTitleUtf8]];
	
	CGRect rect = [[UIScreen mainScreen] applicationFrame];
	
	UIView *view = [[UIView alloc] initWithFrame:rect];
   [view setAutoresizesSubviews: YES];
   [view setAutoresizingMask: UIViewAutoresizingFlexibleWidth];
   
   rect = view.bounds;
	rect.size.height = 40;
	
	gSearchView = [[UISearchBar alloc] initWithFrame:rect];
   [gSearchView setAutoresizesSubviews: YES];
   [gSearchView setAutoresizingMask: UIViewAutoresizingFlexibleWidth];

	
	[view addSubview:gSearchView];
	self.view = view;
	[view release];
	
	[gSearchView setDelegate:self];
	[gSearchView setPlaceholder:[NSString stringWithUTF8String:aTextUtf8]];
	
	gContext = context;
	gCallback = callback;
	
	roadmap_main_push_view(self);
	
}


- (void)dealloc
{
	//Add all items to release here
	[super dealloc];
}



//UISearchBar view delegate
- (void)searchBarSearchButtonClicked:(UISearchBar *)searchBar
{
	[searchBar resignFirstResponder];
	gCallback (1, [[searchBar text] UTF8String] , gContext);
}
/*
- (void)searchBar:(UISearchBar *)searchBar textDidChange:(NSString *)searchText
{
   int i = roadmap_keyboard_typing_locked(TRUE); //TODO: user will only get message but typing will work.
}
*/


@end
