/* rodamap_browser.m - iphone embedded browser implementation.
 *
 * LICENSE:
 *
 *   Copyright 2010 Avi R.
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
 *   See roadmap_browser.h
 */


#include "roadmap_main.h"
#include "roadmap_iphonemain.h"
#include "roadmap_device_events.h"
#include "roadmap_lang.h"
#include "roadmap_messagebox.h"
#include "roadmap_browser.h"
#include "roadmap_iphonebrowser.h"




void roadmap_browser_show (const char* title, const char* url, RoadMapCallback callback){
   BrowserView *browser = [[BrowserView alloc] init];
   [browser show:title url:url callback:callback];
}





/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
@implementation BrowserView

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
   return roadmap_main_should_rotate (interfaceOrientation);
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
   roadmap_device_event_notification( device_event_window_orientation_changed);
}

- (void) backButton
{
   roadmap_main_pop_view(YES);
}

- (void) createView
{
	UIWebView *containerView = [[UIWebView alloc] initWithFrame:CGRectZero];
   containerView.delegate = self;
	self.view = containerView;
   //containerView.detectsPhoneNumbers = NO;
	[containerView release]; // decrement retain count
}

- (void) show: (const char *) title 
          url: (const char *) url
     callback: (RoadMapCallback) callback
{   
   [self createView];
   
   self.title = [NSString stringWithUTF8String:roadmap_lang_get(title)];
   [(UIWebView *)self.view loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:[NSString stringWithUTF8String:url]]]];
   
   gCallback = callback;
   
   //Always show back button for browser views
   UIBarButtonItem *button;
   UINavigationItem *navItem = [self navigationItem];
   button = [[UIBarButtonItem alloc] initWithTitle:[NSString stringWithUTF8String:roadmap_lang_get("Back_key")]
                                             style:UIBarButtonItemStylePlain target:self action:@selector(backButton)];
   [navItem setLeftBarButtonItem: button];
   [button release];
   
   roadmap_main_push_view(self);
}


- (void)dealloc
{
   roadmap_main_set_cursor(ROADMAP_CURSOR_NORMAL);
   
   ((UIWebView *)self.view).delegate = nil;
   
   if (gCallback)
      gCallback();
   
	[super dealloc];
}


/////////////////////////
// web view delegate
- (void)webViewDidStartLoad:(UIWebView *)webView
{
   roadmap_main_set_cursor(ROADMAP_CURSOR_WAIT);
   roadmap_log (ROADMAP_DEBUG, "roadmap_browser - loading page: %s", 
                [[[webView.request URL] absoluteString] UTF8String]);
}

- (void)webViewDidFinishLoad:(UIWebView *)webView
{
   roadmap_main_set_cursor(ROADMAP_CURSOR_NORMAL);
}

- (void)webView:(UIWebView *)webView didFailLoadWithError:(NSError *)error
{
   roadmap_main_set_cursor(ROADMAP_CURSOR_NORMAL);
   roadmap_log (ROADMAP_ERROR, "roadmap_browser - failed loading page, with error: %d ; page: %s", [error code],
                [[[webView.request URL] absoluteString] UTF8String]);
   //roadmap_messagebox("Error", "Could not download data");
}

@end
