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

#include "roadmap.h"
#include "roadmap_start.h"
#include "roadmap_main.h"
#include "roadmap_iphonemain.h"
#include "roadmap_device_events.h"
#include "roadmap_lang.h"
#include "roadmap_messagebox.h"
#include "roadmap_browser.h"
#include "roadmap_iphonebrowser.h"


static BrowserViewController *RoadMapBrowser = NULL;


BOOL roadmap_browser_url_handler( const char* url )
{
   if ( !url || !url[0] )
   {
      roadmap_log( ROADMAP_ERROR, "Url is not valid" );
      return FALSE;
   }
   
   roadmap_log( ROADMAP_DEBUG, "Processing url: %s", url );
   
   if ( strstr( url, WAZE_EXTERN_URL_PREFIX ) == url )
   {
      const char* external_url = url + strlen( WAZE_EXTERN_URL_PREFIX );
      
      roadmap_log( ROADMAP_DEBUG, "Launching external url: %s", external_url );
      
      roadmap_main_open_url(external_url);
      
      return TRUE;
      
   }
   
   if ( strstr( url, WAZE_CMD_URL_PREFIX ) == url )
   {
      const char* url_action = url + strlen( WAZE_CMD_URL_PREFIX );
      const RoadMapAction* action = roadmap_start_find_action( url_action );
      if ( action != NULL )
      {
         roadmap_log( ROADMAP_DEBUG, "Processing action: %s, provided in url: %s", action->name, url );
         action->callback();
         return TRUE;
      }
      else
      {
         roadmap_log( ROADMAP_WARNING, "Cannot find action: %s, provided in url: %s", url_action, url );
      }
   }
   
   return FALSE;
}

/////////////////////////////////////////////////////
void roadmap_browser_unload (void) {
   if (RoadMapBrowser) {
      [RoadMapBrowser release];
      RoadMapBrowser = NULL;
   }
}

/////////////////////////////////////////////////////
void roadmap_browser_preload (const char* title, const char* url, RoadMapCallback callback, int bar_type){
   roadmap_browser_unload();
   
   RoadMapBrowser = [[BrowserViewController alloc] init];
   [RoadMapBrowser preload:title url:url callback:callback bar:bar_type];
}

/////////////////////////////////////////////////////
BOOL roadmap_browser_show_preloaded (void) {
   if (RoadMapBrowser) {
      roadmap_main_push_view (RoadMapBrowser);
      RoadMapBrowser = NULL;
      return TRUE;
   } else {
      return FALSE;
   }

}

/////////////////////////////////////////////////////
void roadmap_browser_show (const char* title, const char* url, RoadMapCallback callback, int bar_type){
   roadmap_browser_preload (title, url, callback, bar_type);
   roadmap_browser_show_preloaded();
}





/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
@implementation BrowserViewController

- (void)viewDidAppear:(BOOL)animated {
   gIsShown = TRUE;
   
   if ([gBrowserView isLoading])
      roadmap_main_set_cursor(ROADMAP_CURSOR_WAIT);
}

- (void)viewWillAppear:(BOOL)animated {
   [gBrowserView removeFromSuperview];
   gBrowserView.userInteractionEnabled = YES;
   gBrowserView.alpha = 1.0;
   [self.view addSubview:gBrowserView];
   gBrowserView.frame = self.view.bounds;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
   return roadmap_main_should_rotate (interfaceOrientation);
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
   roadmap_device_event_notification( device_event_window_orientation_changed);
}

- (void) closeButton
{
   roadmap_main_pop_view(YES);
}

- (void) backButton
{
   [gBrowserView stringByEvaluatingJavaScriptFromString:@"back()"];
}

- (void) homeButton
{
   [gBrowserView stringByEvaluatingJavaScriptFromString:@"home()"];
}

- (void) createView
{
	gBrowserView = [[UIWebView alloc] initWithFrame:CGRectZero];
   gBrowserView.delegate = self;
   
   gBrowserView.userInteractionEnabled = FALSE;
   gBrowserView.alpha = 0.1;
   gBrowserView.frame = CGRectMake(0, 0, 1, 1);
   roadmap_main_show_view(gBrowserView); //workaround for loading the page before showing the view
   UIView *containerView = [[UIView alloc] initWithFrame:CGRectZero];
   containerView.autoresizesSubviews = YES;
   containerView.autoresizingMask = UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth;
   gBrowserView.autoresizesSubviews = YES;
   gBrowserView.autoresizingMask = UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth;
	self.view = containerView;
	[containerView release]; // decrement retain count
}

- (void) preload: (const char *) title 
             url: (const char *) url
        callback: (RoadMapCallback) callback
             bar: (int) bar_type
{
   NSString *str;
   gIsShown = FALSE;
   
   [self createView];
   
   self.title = [NSString stringWithUTF8String:roadmap_lang_get(title)];
   str = [[NSString stringWithUTF8String:url] stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
   [gBrowserView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:str]]];
   
   gCallback = callback;
   
   //Always show back button for browser views
   UIBarButtonItem *button;
   UINavigationItem *navItem = [self navigationItem];
   UIView *leftView;
   UIButton *inner_button;
   UIImage *image;
   CGRect rect;
   if (bar_type == BROWSER_BAR_NORMAL) {
      button = [[UIBarButtonItem alloc] initWithTitle:[NSString stringWithUTF8String:roadmap_lang_get("Back_key")]
                                                style:UIBarButtonItemStylePlain target:self action:@selector(closeButton)];
      [navItem setLeftBarButtonItem: button];
      [button release];
   } else {
      button = [[UIBarButtonItem alloc] initWithTitle:[NSString stringWithUTF8String:roadmap_lang_get("Close")]
                                                style:UIBarButtonItemStylePlain target:self action:@selector(closeButton)];
      [navItem setRightBarButtonItem: button];
      [button release];
      
      leftView = [[UIView alloc] initWithFrame:CGRectZero];
      //home button
      inner_button = [UIButton buttonWithType:UIButtonTypeCustom];
      [inner_button addTarget:self action:@selector(homeButton) forControlEvents:UIControlEventTouchUpInside];
      image = roadmap_iphoneimage_load("browser_home");
      if (image) {
         [inner_button setBackgroundImage:image forState:UIControlStateNormal];
         [image release];
      }
      image = roadmap_iphoneimage_load("browser_home_sel");
      if (image) {
         [inner_button setBackgroundImage:image forState:UIControlStateHighlighted];
         [image release];
      }
      [inner_button sizeToFit];
      rect = inner_button.frame;
      [leftView addSubview:inner_button];
      leftView.bounds = rect;
      //back button
      inner_button = [UIButton buttonWithType:UIButtonTypeCustom];
      [inner_button addTarget:self action:@selector(backButton) forControlEvents:UIControlEventTouchUpInside];
      image = roadmap_iphoneimage_load("browser_back");
      if (image) {
         [inner_button setBackgroundImage:image forState:UIControlStateNormal];
         [image release];
      }
      image = roadmap_iphoneimage_load("browser_back_sel");
      if (image) {
         [inner_button setBackgroundImage:image forState:UIControlStateHighlighted];
         [image release];
      }
      [inner_button sizeToFit];
      rect.origin.x += rect.size.width - 15;
      rect.size = inner_button.bounds.size;
      inner_button.frame = rect;
      [leftView addSubview:inner_button];
      rect = leftView.bounds;
      rect.size.width = inner_button.frame.origin.x + inner_button.frame.size.width;
      leftView.bounds = rect;

      button = [[UIBarButtonItem alloc] initWithCustomView:leftView];
      [leftView release];
      [navItem setLeftBarButtonItem: button];
      [button release];
   }
}


- (void)dealloc
{
   roadmap_main_set_cursor(ROADMAP_CURSOR_NORMAL);
   
   if (gBrowserView) {
      gBrowserView.delegate = nil;
      [gBrowserView release];
   }
   
   if (gCallback)
      gCallback();
   
	[super dealloc];
}


/////////////////////////
// web view delegate

- (BOOL)webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType
{
   char url[WEB_VIEW_URL_MAXSIZE];
   
   if (!request) {
      roadmap_log(ROADMAP_ERROR, "Invalid url request !");
      return NO;
   }
   
   strncpy_safe (url, [[[request URL] absoluteString] UTF8String], WEB_VIEW_URL_MAXSIZE);
   
   return !roadmap_browser_url_handler(url);
}

- (void)webViewDidStartLoad:(UIWebView *)webView
{
   if (gIsShown)
      roadmap_main_set_cursor(ROADMAP_CURSOR_WAIT);
}

- (void)webViewDidFinishLoad:(UIWebView *)webView
{
   if (gIsShown)
      roadmap_main_set_cursor(ROADMAP_CURSOR_NORMAL);
}

- (void)webView:(UIWebView *)webView didFailLoadWithError:(NSError *)error
{
   if (gIsShown)
      roadmap_main_set_cursor(ROADMAP_CURSOR_NORMAL);
   roadmap_log (ROADMAP_ERROR, "roadmap_browser - failed loading page, with error: %d ; page: %s", [error code],
                [[[webView.request URL] absoluteString] UTF8String]);
   //roadmap_messagebox("Error", "Could not download data");
}

@end
