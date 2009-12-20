/* roadmap_main.c - The main function of the RoadMap application.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
 *   Copyright 2008 Morten Bek Ditlevsen
 *   Copyright 2008 Avi R.
 *   Copyright 2008 Ehud Shabtai
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
 *   int main (int argc, char **argv);
 */

#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "../roadmap.h"
#include "../roadmap_path.h"
#include "../roadmap_file.h"
#include "../roadmap_start.h"
#include "../roadmap_device_events.h"
#include "../roadmap_config.h"
#include "../roadmap_history.h"
#include "roadmap_canvas.h"
#include "roadmap_iphonecanvas.h"
#include "roadmap_iphonemain.h"

#include "../roadmap_main.h"
#include "../roadmap_time.h"

#include "../roadmap_lang.h"
#include "../roadmap_download.h"
#include "../editor/editor_main.h"
#include "../ssd/ssd_progress_msg_dialog.h"
#include "roadmap_square.h"
#include "../Realtime/RealtimeAlerts.h"
#include "roadmap_bar.h"
#include "roadmap_iphonebar.h"
#include "roadmap_skin.h"
#include "roadmap_iphoneimage.h"
#include "widgets/iphoneRoundedView.h"
#include "roadmap_iphoneurlscheme.h"
#include "roadmap_version.h"
#include "roadmap_messagebox.h"
#include "roadmap_map_settings.h"

#import <CFNetwork/CFProxySupport.h>
#import <MediaPlayer/MediaPlayer.h>




int USING_PHONE_KEYPAD = 0;

static RoadMapApp *TheApp;
static int sArgc;
static char ** sArgv;


struct roadmap_main_io {
   NSFileHandle *fh;
   CFRunLoopSourceRef rl;
   RoadMapIO io;
   RoadMapInput callback;
   time_t start_time;
   int type;
};

#define ROADMAP_MAX_IO  16
#define IO_TYPE_OUTPUT  0
#define IO_TYPE_INPUT   1
static struct roadmap_main_io RoadMapMainIo[ROADMAP_MAX_IO];

struct roadmap_main_timer {
   NSTimer *timer;
   RoadMapCallback callback;
};

#define ROADMAP_MAX_TIMER 24
static struct roadmap_main_timer RoadMapMainPeriodicTimer[ROADMAP_MAX_TIMER];

static RoadMapCallback idle_callback = NULL;
static char *RoadMapMainTitle = NULL;

static RoadMapKeyInput        RoadMapMainInput           = NULL;
static UIWindow               *RoadMapMainWindow         = NULL;
static RoadMapViewController  *RoadMapMainViewController = NULL;
static UINavigationController	*RoadMapMainNavCont        = NULL;
static RoadMapView            *RoadMapMainBox            = NULL;
static RoadMapCanvasView      *RoadMapCanvasBox          = NULL;
static UIToolbar              *RoadMapBottomBar          = NULL;
static UIView                 *RoadMapMainToolbar        = NULL;
static UIImageView            *RoadmapBgImageV           = NULL;

static int                 IsGoingToSuspend           = 0; //true if suspending not from home button (phone call etc)
static int                 RoadMapMainPlatform        = ROADMAP_MAIN_PLATFORM_NA;
static int                 RoadMapMainOsVersion       = ROADMAP_MAIN_OS_NA;
static char                RoadMapMainProxy[256];
static int                 gsBacklightOn;
static int                 gsTopBarHeight;


static int iPhoneIconsInitialized = 0;


#define EDGE_MARGINE          0.0f
#define BOTTOM_BAR_HEIGHT     40.0f
#define BOTTOM_BAR_HEIGHT_LS  32.0f
#define VIEW_TAG_TOP_BAR      1
#define VIEW_TAG_BG_IMAGE     2

static UIView *roadmap_main_toolbar_icon (const char *icon) {

   if (icon != NULL) {

      const char *icon_file = roadmap_path_search_icon (icon);

      if (icon_file != NULL) {
         iPhoneIconsInitialized = 1;
         return NULL; // gtk_image_new_from_file (icon_file);
      }
   }
   return NULL;
}



void roadmap_main_toggle_full_screen (void) {
}

void roadmap_main_new (const char *title, int width, int height) {
    [TheApp newWithTitle: title andWidth: width andHeight: height];
	
      editor_main_set (1);
}

void roadmap_main_title(char *fmt, ...) {

   char newtitle[200];
   va_list ap;
   int n;

   n = snprintf(newtitle, 200, "%s", RoadMapMainTitle);
   va_start(ap, fmt);
   vsnprintf(&newtitle[n], 200 - n, fmt, ap);
   va_end(ap);

 //  gtk_window_set_title (GTK_WINDOW(RoadMapMainWindow), newtitle);
}

void roadmap_main_set_keyboard
  (struct RoadMapFactoryKeyMap *bindings, RoadMapKeyInput callback) {
   RoadMapMainInput = callback;
}

RoadMapMenu roadmap_main_new_menu (void) {

   return NULL;
}


void roadmap_main_free_menu (RoadMapMenu menu) {
     //NSLog (@"roadmap_main_free_menu\n");

   
}

void roadmap_main_add_menu (RoadMapMenu menu, const char *label) {
     //NSLog (@"roadmap_main_add_menu label: %s\n", label);
/*
   UIView *menu_item;

   if (RoadMapMainMenuBar == NULL) {

      RoadMapMainMenuBar = gtk_menu_bar_new();

      gtk_box_pack_start
         (GTK_BOX(RoadMapMainBox), RoadMapMainMenuBar, FALSE, TRUE, 0);
   }

   menu_item = gtk_menu_item_new_with_label (label);
   gtk_menu_shell_append (GTK_MENU_SHELL(RoadMapMainMenuBar), menu_item);

   gtk_menu_item_set_submenu (GTK_MENU_ITEM(menu_item), (UIView *) menu);
*/
}


void roadmap_main_popup_menu (RoadMapMenu menu, int x, int y) {
     //NSLog (@"roadmap_main_popup_menu\n");

 /*  if (menu != NULL) {
      gtk_menu_popup (GTK_MENU(menu),
                      NULL,
                      NULL,
                      NULL,
                      NULL,
                      0,
                      gtk_get_current_event_time());
   }
   */
}


void roadmap_main_add_menu_item (RoadMapMenu menu,
                                 const char *label,
                                 const char *tip,
                                 RoadMapCallback callback) {

     //NSLog (@"roadmap_main_add_menu_item label: %s tip: %s\n", label, tip);
/*
   UIView *menu_item;

   if (label != NULL) {

      menu_item = gtk_menu_item_new_with_label (label);
      g_signal_connect (menu_item, "activate",
                        (GCallback)roadmap_main_activate,
                        callback);
   } else {
      menu_item = gtk_menu_item_new ();
   }
   gtk_menu_shell_append (GTK_MENU_SHELL(menu), menu_item);
   gtk_widget_show(menu_item);

   if (tip != NULL) {
      gtk_tooltips_set_tip (gtk_tooltips_new (), menu_item, tip, NULL);
   }
   */
}


void roadmap_main_add_separator (RoadMapMenu menu) {
   //NSLog (@"roadmap_main_add_seperator\n");

   roadmap_main_add_menu_item (menu, NULL, NULL, NULL);
}


void roadmap_main_add_toolbar (const char *orientation) {
     //NSLog (@"roadmap_main_add_toolbar orientation: %s\n", orientation);

    if (RoadMapMainToolbar == NULL) {
        int on_top = 1;

        //   GtkOrientation gtk_orientation = GTK_ORIENTATION_HORIZONTAL;

        // RoadMapMainToolbar = gtk_toolbar_new ();

        switch (orientation[0]) {
            case 't':
            case 'T': on_top = 1; break;

            case 'b':
            case 'B': on_top = 0; break;

            case 'l':
            case 'L': on_top = 1;
                      //            gtk_orientation = GTK_ORIENTATION_VERTICAL;
                      break;

            case 'r':
            case 'R': on_top = 0;
                      //           gtk_orientation = GTK_ORIENTATION_VERTICAL;
                      break;

            default: /*roadmap_log (ROADMAP_FATAL,
                             "Invalid toolbar orientation %s", orientation);
                             */
                             break;
        }
        /*      gtk_toolbar_set_orientation (GTK_TOOLBAR(RoadMapMainToolbar),
                gtk_orientation);

                if (gtk_orientation == GTK_ORIENTATION_VERTICAL) {
         */
        /* In this case we need a new box, since the "normal" box
         * is a vertical one and we need an horizontal one. */

        /*         RoadMapCanvasBox = gtk_hbox_new (FALSE, 0);
                   gtk_container_add (GTK_CONTAINER(RoadMapMainBox), RoadMapCanvasBox);
         */
        // }

        if (on_top) {
            // gtk_box_pack_start (GTK_BOX(RoadMapCanvasBox),
            //                     RoadMapMainToolbar, FALSE, FALSE, 0);
        } else {
            // gtk_box_pack_end   (GTK_BOX(RoadMapCanvasBox),
            //                     RoadMapMainToolbar, FALSE, FALSE, 0);
        }
    }
}

void roadmap_main_add_tool (const char *label,
                            const char *icon,
                            const char *tip,
                            RoadMapCallback callback) {
   //NSLog (@"roadmap_main_add_tool label: %s icon: %s tip: %s\n", label, icon, tip);
   if (RoadMapMainToolbar == NULL) {
      roadmap_main_add_toolbar ("");
   }

/*
   gtk_toolbar_append_item (GTK_TOOLBAR(RoadMapMainToolbar),
                            label, tip, NULL,
                            roadmap_main_toolbar_icon (icon),
                            (GtkSignalFunc) roadmap_main_activate, callback);

   if (gdk_screen_height() < 550)
   {
*/
      /* When using a small screen, we want either the labels or the icons,
       * but not both (small screens are typical with PDAs). */
       
/*      gtk_toolbar_set_style
         (GTK_TOOLBAR(RoadMapMainToolbar),
          GtkIconsInitialized?GTK_TOOLBAR_ICONS:GTK_TOOLBAR_TEXT);
   }
   */
}


void roadmap_main_add_tool_space (void) {
   //NSLog (@"roadmap_main_add_tool_space\n");
   if (RoadMapMainToolbar == NULL) {
   //   roadmap_log (ROADMAP_FATAL, "Invalid toolbar space: no toolbar yet");
   }

/*
   gtk_toolbar_append_space (GTK_TOOLBAR(RoadMapMainToolbar));
   */
}

static unsigned long roadmap_main_busy_start;

void roadmap_main_set_cursor (int cursor) {
	static iphoneRoundedView *spinner;
	static UIActivityIndicatorView *activityIndicator;
	CGRect rect;
	UIColor *color;
	
	if (spinner == NULL) {
		rect = CGRectMake(RoadMapMainWindow.bounds.size.width/2 - 80,
						  RoadMapMainWindow.bounds.size.height/2 - 50, 
						  160,
						  100);
		spinner = [[RoadMapRoundedView alloc] initWithFrame: rect];
		spinner.opaque = NO;
		color = [UIColor colorWithWhite:0 alpha:0.7];
		[spinner setBackgroundColor: [UIColor clearColor]];
		[spinner setRectColor: color];
		activityIndicator = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleWhiteLarge];
		rect = activityIndicator.frame;
		rect.origin.x = spinner.bounds.size.width/2 - activityIndicator.frame.size.width/2;
		rect.origin.y = spinner.bounds.size.height/2 - activityIndicator.frame.size.height/2;
		[activityIndicator setFrame:rect];
		[spinner addSubview:activityIndicator];
	}
   
   switch (cursor) {

      case ROADMAP_CURSOR_NORMAL:
		   [activityIndicator stopAnimating];
		   [spinner removeFromSuperview];
         break;

      case ROADMAP_CURSOR_WAIT:
		   
		   [activityIndicator startAnimating];
		   
		   roadmap_main_show_view(spinner);
         break;
   }
}

void roadmap_main_busy_check(void) {

   if (roadmap_main_busy_start == 0)
      return;

   if (roadmap_time_get_millis() - roadmap_main_busy_start > 1000) {
      roadmap_main_set_cursor (ROADMAP_CURSOR_WAIT);
   }
}

static void roadmap_canvas_rect (CGRect *outRect){
   CGRect rect = [RoadMapMainBox bounds];
   int topBarHeight = gsTopBarHeight;
   if (roadmap_map_settings_isShowTopBarOnTap()) //top bar is floating on canvas
      topBarHeight = 0;
   
   
   //Map view
	rect.origin.x += EDGE_MARGINE;
   rect.origin.y += topBarHeight;
	rect.size.width -= EDGE_MARGINE*2;
   rect.size.height -= EDGE_MARGINE + RoadMapBottomBar.bounds.size.height + topBarHeight;
   
   *outRect = rect;
}

void roadmap_main_add_canvas (void) {
   //    NSLog ("roadmap_main_add_canvas\n");
   
   CGRect rect;
	roadmap_canvas_rect(&rect);
   
   RoadMapCanvasBox = [[RoadMapCanvasView alloc] initWithFrame: rect];
	[RoadMapCanvasBox setClipsToBounds:YES];
   [RoadMapCanvasBox setAutoresizesSubviews: YES];
	[RoadMapCanvasBox setAutoresizingMask: UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth];
   [RoadMapMainBox addSubview: RoadMapCanvasBox];
   
   UIView *topView = [RoadMapMainBox viewWithTag:VIEW_TAG_TOP_BAR];
   if (topView)
      [RoadMapMainBox bringSubviewToFront:topView];
}

void roadmap_main_add_status (void) {
   //NSLog (@"roadmap_main_add_status\n");
}


void roadmap_main_show (void) {
   //NSLog (@"roadmap_main_show\n");
}


void roadmap_main_set_input (RoadMapIO *io, RoadMapInput callback) {
    [TheApp setInput: io andCallback: callback];
}

static void outputCallback (
   CFSocketRef s,
   CFSocketCallBackType callbackType,
   CFDataRef address,
   const void *data,
   void *info
)
{
   int i;
   int fd = CFSocketGetNative(s);

   for (i = 0; i < ROADMAP_MAX_IO; ++i) {
      if (RoadMapMainIo[i].io.os.file == fd) {
         (* RoadMapMainIo[i].callback) (&RoadMapMainIo[i].io);
         break;
      }
   }
}

static void internal_set_output (CFSocketRef s, RoadMapIO *io, RoadMapInput callback) {
   int i;
   
   for (i = 0; i < ROADMAP_MAX_IO; ++i) {
      if (RoadMapMainIo[i].io.subsystem == ROADMAP_IO_INVALID) {
         RoadMapMainIo[i].io = *io;
         RoadMapMainIo[i].callback = callback;
         RoadMapMainIo[i].start_time = time(NULL);
         RoadMapMainIo[i].fh = NULL;
         RoadMapMainIo[i].type = IO_TYPE_OUTPUT;
         RoadMapMainIo[i].rl = CFSocketCreateRunLoopSource (NULL, s, 0);
         
         CFRunLoopAddSource (CFRunLoopGetCurrent( ), RoadMapMainIo[i].rl,
                             kCFRunLoopCommonModes);
         break;
      }
   }
}

void dummy_url_request (void) {
   //dummy http request to init connection (workaround)
   NSURLRequest *request;
   NSURLConnection *connection;
   
   // create the request
   request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"http://www.waze.com/"]
                                             cachePolicy:NSURLRequestUseProtocolCachePolicy
                                         timeoutInterval:60.0];
   // create the connection with the request
   // and start loading the data   
   connection = [[NSURLConnection alloc] initWithRequest:request delegate:TheApp];
}

int roadmap_main_async_connect(RoadMapIO *io, struct sockaddr *addr, RoadMapInput callback) {

   CFSocketRef s = CFSocketCreateWithNative(NULL, io->os.socket, kCFSocketConnectCallBack, outputCallback, NULL);

   CFSocketSetSocketFlags (s, kCFSocketAutomaticallyReenableReadCallBack|
                              kCFSocketAutomaticallyReenableAcceptCallBack|
                              kCFSocketAutomaticallyReenableDataCallBack);

   CFDataRef address = CFDataCreate(NULL, (const UInt8 *)addr, sizeof(*addr));
   
   internal_set_output(s, io, callback);
   
   int res =  CFSocketConnectToAddress (s, address, -1);

   if (res == kCFSocketError) {
      roadmap_log(ROADMAP_WARNING, "Socket connect error, trying to resolve");
      dummy_url_request();
   }
   
     
     
   CFRelease (address);

   if (res < 0) {
      roadmap_main_remove_input (io);
      return -1;
   }
   
   return res;
}


RoadMapIO *roadmap_main_output_timedout(time_t timeout) {
   int i;

   for (i = 0; i < ROADMAP_MAX_IO; ++i) {
      if (RoadMapMainIo[i].io.subsystem != ROADMAP_IO_INVALID &&
          RoadMapMainIo[i].type == IO_TYPE_OUTPUT) {
         if (RoadMapMainIo[i].start_time &&
               (timeout > RoadMapMainIo[i].start_time)) {
            return &RoadMapMainIo[i].io;
         }
      }
   }

   return NULL;
}


void roadmap_main_remove_input (RoadMapIO *io) {
    [TheApp removeIO: io];
}

void roadmap_main_set_periodic (int interval, RoadMapCallback callback) {
    [TheApp setPeriodic: (interval*0.001) andCallback: callback];
}


void roadmap_main_remove_periodic (RoadMapCallback callback) {
   [TheApp removePeriodic: callback];
}


void roadmap_main_set_status (const char *text) {
//   NSLog (@"roadmap_main_set_status text: %s\n", text);
}

void roadmap_main_set_idle_function (RoadMapCallback callback) {
    idle_callback = callback;
    [TheApp setPeriodic: 0.00 andCallback: callback];
}

void roadmap_main_remove_idle_function (void) {
   [TheApp removePeriodic: idle_callback];
   idle_callback = NULL;
}


void roadmap_main_flush (void) {
   
   //NSLog (@"roadmap_main_flush\n");
   
   double resolution = 0.0001;

   NSDate* next = [NSDate dateWithTimeIntervalSinceNow:resolution];

   [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode
                              beforeDate:next];
}


int roadmap_main_flush_synchronous (int deadline) {
   NSLog (@"roadmap_main_flush_synchronous\n");

   long start_time, duration;

   start_time = roadmap_time_get_millis();

  /* while (gtk_events_pending ()) {
      if (gtk_main_iteration ()) {
         exit(0); 
      }
   }
   */
//   gdk_flush();

   duration = roadmap_time_get_millis() - start_time;

   if (duration > deadline) {

      roadmap_log (ROADMAP_DEBUG, "processing flush took %d", duration);

      return 0; /* Busy. */
   }

   return 1; /* Not so busy. */
}

void roadmap_main_exit (void) {
   roadmap_start_exit ();
	exit(0);
}

static void roadmap_start_event (int event) {
   switch (event) {
   case ROADMAP_START_INIT:
      editor_main_check_map (); //TODO: enable this
      break;
   }
}

int roadmap_main_should_mute () {
	int shouldMute =  IsGoingToSuspend;

   return (shouldMute);
}

int roadmap_main_will_suspend () {
   return (IsGoingToSuspend);
}

int roadmap_main_get_platform () {
   return (RoadMapMainPlatform);
}

int roadmap_main_get_os_ver () {
   return (RoadMapMainOsVersion);
}

int roadmap_horizontal_screen_orientation(){
   return (RoadMapMainViewController.interfaceOrientation == UIInterfaceOrientationLandscapeLeft ||
           RoadMapMainViewController.interfaceOrientation == UIInterfaceOrientationLandscapeRight);
}
 
void roadmap_main_minimize (void){

}

void roadmap_main_adjust_skin (void) {
	UIImage *appBgImage = NULL;
   NSString *backgroundStr;
	
   backgroundStr = @"background";
	int state = roadmap_skin_state();
	
	//Set background image
	if (state != 0) {
      backgroundStr = [backgroundStr stringByAppendingString:@"_night"];
      [TheApp setStatusBarStyle:UIStatusBarStyleBlackOpaque];
   } else {
      [TheApp setStatusBarStyle:UIStatusBarStyleDefault];
   }
   
   if (roadmap_horizontal_screen_orientation())
      backgroundStr = [backgroundStr stringByAppendingString:@"_hor"];
      
      
   appBgImage = roadmap_iphoneimage_load([backgroundStr UTF8String]);
	
	if (appBgImage) {
      RoadmapBgImageV.image = appBgImage;
		[appBgImage release];
      [RoadmapBgImageV sizeToFit];
	}
}


void roadmap_main_open_url (const char* url) {
   [[UIApplication sharedApplication] openURL:[NSURL URLWithString:[NSString stringWithUTF8String:url]]];
}

void roadmap_main_play_movie (const char* url) {
   [TheApp playMovie:url];
}

void GetProxy (const char* url) {
	
#if TARGET_IPHONE_SIMULATOR
	return;
#else

   strcpy (RoadMapMainProxy, "");

   CFStringRef URLString = CFStringCreateWithCString (NULL, url, kCFStringEncodingUTF8);
   CFURLRef    CFurl = CFURLCreateWithString(NULL, URLString, NULL);
   CFRelease (URLString);
   
   CFURLRef    CFScriptUrl = NULL;
   CFDataRef   CFScriptData;
   SInt32      errCode;
   CFErrorRef  errCodeScript;
   CFStringRef	CFScriptStr;

	CFDictionaryRef systemProxySettings = CFNetworkCopySystemProxySettings ();
   CFDictionaryRef ProxySettings = CFDictionaryCreateCopy (NULL, systemProxySettings);
	CFRelease(systemProxySettings);
   
   CFArrayRef Proxies = CFNetworkCopyProxiesForURL (CFurl, ProxySettings);
	CFRelease(ProxySettings);

   int index = 0;
   int foundProxy = 0;
   int count = CFArrayGetCount (Proxies);
   CFDictionaryRef TheProxy;
   
   char ProxyURL[256];
   int  ProxyPort = 0;
   
   while (index < count) {
      TheProxy = CFArrayGetValueAtIndex (Proxies, index);
      if (CFEqual(CFDictionaryGetValue (TheProxy, kCFProxyTypeKey), kCFProxyTypeHTTP)) {
         CFStringGetCString (CFDictionaryGetValue (TheProxy, kCFProxyHostNameKey),
                              ProxyURL,
                              256,
                              kCFStringEncodingUTF8);
         CFNumberGetValue (CFDictionaryGetValue (TheProxy, kCFProxyPortNumberKey),
                           CFNumberGetType (CFDictionaryGetValue (TheProxy, kCFProxyPortNumberKey)),
                           &ProxyPort);
         sprintf (RoadMapMainProxy, "%s:%d", ProxyURL, ProxyPort);
         
         break;
      } else if (CFEqual(CFDictionaryGetValue (TheProxy, kCFProxyTypeKey), kCFProxyTypeAutoConfigurationURL)) {
         if (foundProxy) break;
         
         foundProxy = 1;
         
         // found proxy script, extract proxy data
         CFScriptUrl = CFDictionaryGetValue (TheProxy, kCFProxyAutoConfigurationURLKey);
         
         // read script from file
         if (!CFURLCreateDataAndPropertiesFromResource(NULL, CFScriptUrl,
                                                   &CFScriptData, NULL, NULL, &errCode)) {
            break;
         }
         CFScriptStr = CFStringCreateWithBytes(NULL, CFDataGetBytePtr(CFScriptData),
                                             CFDataGetLength(CFScriptData), kCFStringEncodingUTF8, true);
         
         CFRelease (Proxies);
         Proxies = CFNetworkCopyProxiesForAutoConfigurationScript (CFScriptStr, CFurl, &errCodeScript);
         
         // run again this loop with real proxy data
         count = CFArrayGetCount (Proxies);
         index = -1;
      }
      index++;
   }
   
   CFRelease (Proxies);
   CFRelease (CFurl);
   return;
#endif //TARGET_IPHONE_SIMULATOR
}

const char* roadmap_main_get_proxy (const char* url) {
   GetProxy(url);
   
   if (strcmp (RoadMapMainProxy, "") == 0) {
      return (NULL);
   }
   
   return (RoadMapMainProxy);
}

char const *roadmap_main_home (void) {
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	if ([paths count] > 0) {
		NSString *documentsDirectory = [paths objectAtIndex:0];
		return [documentsDirectory UTF8String];
	}
	NSLog(@"not found documents");
	return [[[NSBundle mainBundle]  resourcePath] UTF8String];
}

char const *roadmap_main_bundle (void) {
	return [[NSBundle mainBundle] resourcePath].UTF8String;
}

void roadmap_main_present_modal (UIViewController *modalController) {
	[TheApp presentModalView: modalController];
}

void roadmap_main_dismiss_modal () {
	[TheApp dismissModalView];
   
   roadmap_main_set_backlight(gsBacklightOn); //restore backlight state when back from modal view
}

void roadmap_main_push_view (UIViewController *viewController) {
	[TheApp pushView: viewController];
	[viewController release];
}

void roadmap_main_pop_view (int animated) {
	[TheApp popView:animated];
}

void roadmap_main_show_root (int animated) {
	[TheApp showRoot:animated];
}

int roadmap_main_is_root () {
	return ([[RoadMapMainNavCont viewControllers] count] == 1);
}

void roadmap_main_show_view (UIView	*view) {
   [RoadMapMainNavCont.view addSubview:view];
}

int roadmap_main_get_mainbox_height () {
	CGRect rect = RoadMapMainBox.bounds;
	return rect.size.height - gsTopBarHeight;
}

void roadmap_main_get_bounds (CGRect *bounds) {
   *bounds = RoadMapMainNavCont.view.bounds;
}

void roadmap_main_set_backlight( int isAlwaysOn ) {
   gsBacklightOn = isAlwaysOn;
   [UIApplication sharedApplication].idleTimerDisabled = !gsBacklightOn; //workaround for 3.x bug, always toggle between states
   [UIApplication sharedApplication].idleTimerDisabled = gsBacklightOn;
}

void roadmap_main_refresh_backlight (void) {
   roadmap_main_set_backlight (gsBacklightOn);
}

BOOL roadmap_main_should_rotate(UIInterfaceOrientation interfaceOrientation) {
   //return (interfaceOrientation == UIInterfaceOrientationPortrait);
   
   return (interfaceOrientation != UIInterfaceOrientationPortraitUpsideDown);
}

static void on_device_event(device_event event, void* context) {
   
   if( event == device_event_window_orientation_changed ){
      roadmap_main_adjust_skin();
   }
}

void roadmap_main_layout (void) {
   [RoadMapMainBox layoutSubviews];
}

void roadmap_main_reload_bg_image (void) {
   //workaround due to low memory auto release
   if ([RoadMapMainNavCont.view viewWithTag:VIEW_TAG_BG_IMAGE] == NULL) {
      [RoadMapMainNavCont.view addSubview: RoadmapBgImageV];
      [RoadMapMainNavCont.view sendSubviewToBack:RoadmapBgImageV];
   }
      
}



int main (int argc, char **argv) {
    int i;
    int j = 0;
    sArgc = argc;
    sArgv = (char **)malloc(argc * (sizeof (char*)));
    for (i=0; i<argc; i++)
    {
        if (strcmp(argv[i], "--launchedFromSB") != 0) {
            sArgv[i] = strdup(argv[j]);
            j++;
        }
        else
           sArgc--;
    }

    roadmap_option (sArgc, sArgv, NULL);
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    int retVal = UIApplicationMain(sArgc, sArgv, @"RoadMapApp", @"RoadMapApp");
    [pool release];
    return retVal;
}



@implementation RoadMapView


- (void)layoutSubviews
{
   CGRect rect;
   
   //set bottom bar
   rect = RoadMapBottomBar.frame;
   if (roadmap_horizontal_screen_orientation())
      rect.size.height = BOTTOM_BAR_HEIGHT_LS;
   else
      rect.size.height = BOTTOM_BAR_HEIGHT;
   rect.origin.y = RoadMapMainBox.bounds.size.height - rect.size.height;
   RoadMapBottomBar.frame = rect;
   
   //set canvas view
   roadmap_canvas_rect(&rect);
   RoadMapCanvasBox.frame = rect;
   [RoadMapCanvasBox layoutSubviews];
}

@end



@implementation RoadMapViewController
/*
- (void)loadView
{
   RoadMapView *containerView;
   //Mainbox
   CGRect rect = [RoadMapMainWindow bounds];
   containerView = [[RoadMapView alloc] initWithFrame: rect];
   [containerView setAutoresizesSubviews: YES];
   [containerView setAutoresizingMask: UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth];
   
   // Top bar window
   UIView *topBarView = roadmap_bar_create_top_bar ();
   topBarView.tag = VIEW_TAG_TOP_BAR;
   [containerView addSubview: topBarView];
   gsTopBarHeight = topBarView.bounds.size.height;
   
   //Create bottom bar
   rect = containerView.bounds;
   int pos_y = rect.origin.y + rect.size.height - BOTTOM_BAR_HEIGHT;
   RoadMapBottomBar = roadmap_bar_create_bottom_bar (BOTTOM_BAR_HEIGHT,
                                                     pos_y);
   [containerView addSubview: RoadMapBottomBar];
   
   self.view = containerView;
   
   [containerView release];
}

- (void) viewDidLoad
{
   roadmap_main_adjust_skin();
}

- (void) viewDidUnload
{
   roadmap_main_adjust_skin();
}

*/
- (void)viewWillAppear:(BOOL)animated {
	if (RoadMapMainNavCont) {
		[RoadMapMainNavCont setNavigationBarHidden:YES animated:animated];
	}
   
   roadmap_canvas_should_accept_layout(TRUE);
}

- (void)viewWillDisappear:(BOOL)animated {
   roadmap_canvas_should_accept_layout(FALSE);
   [RoadMapMainNavCont setNavigationBarHidden:NO animated:NO];
}

- (void)viewDidAppear:(BOOL)animated
{
   [RoadMapMainViewController setTitle:[NSString stringWithUTF8String:roadmap_lang_get("back")]];
   if (RoadMapCanvasBox)
      roadmap_screen_redraw();
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
   return roadmap_main_should_rotate (interfaceOrientation);
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
   roadmap_device_event_notification( device_event_window_orientation_changed);
}


- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
}


- (void)dealloc {
    [super dealloc];
}

@end



@implementation RoadMapRoundedView
@synthesize rectColor;

-(void) CGContextMakeRoundedCorners: (CGContextRef) c rect: (CGRect) rect radius: (int) corner_radius
{
   int x_left = rect.origin.x;  
   int x_left_center = rect.origin.x + corner_radius;  
   int x_right_center = rect.origin.x + rect.size.width - corner_radius;  
   int x_right = rect.origin.x + rect.size.width;  
   int y_top = rect.origin.y;  
   int y_top_center = rect.origin.y + corner_radius;  
   int y_bottom_center = rect.origin.y + rect.size.height - corner_radius;  
   int y_bottom = rect.origin.y + rect.size.height;  
	
	
   CGContextBeginPath(c);  
   CGContextMoveToPoint(c, x_left, y_top_center);  
	
   CGContextAddArcToPoint(c, x_left, y_top, x_left_center, y_top, corner_radius);  
   CGContextAddLineToPoint(c, x_right_center, y_top);
	
	CGContextAddArcToPoint(c, x_right, y_top, x_right, y_top_center, corner_radius);  
	
   CGContextAddArcToPoint(c, x_right, y_bottom, x_right_center, y_bottom, corner_radius);  
   CGContextAddLineToPoint(c, x_left_center, y_bottom);  
	
   CGContextAddArcToPoint(c, x_left, y_bottom, x_left, y_bottom_center, corner_radius);  
   CGContextAddLineToPoint(c, x_left, y_top_center);  
	
   CGContextClosePath(c);
	
	CGContextSetFillColorWithColor(c, [self rectColor].CGColor);
	CGContextDrawPath(c, kCGPathFill);
}

-(void) drawRect: (CGRect) rect
{
	CGContextRef c = UIGraphicsGetCurrentContext(); 
	[self CGContextMakeRoundedCorners:c rect:rect radius:10];  
}


@end



@implementation RoadMapApp


-(void) newWithTitle: (const char *)title andWidth: (int) width andHeight: (int) height
{
	struct CGRect rect;
	if (RoadMapMainWindow == NULL) {
		
		// Main window
		rect = [[UIScreen mainScreen] bounds];
      //rect = [[UIScreen mainScreen] applicationFrame];
      RoadMapMainWindow = [[UIWindow alloc] initWithFrame:rect];
      
      //Mainbox
      rect = [RoadMapMainWindow bounds];
      RoadMapMainBox = [[RoadMapView alloc] initWithFrame: rect];
      [RoadMapMainBox setAutoresizesSubviews: YES];
		[RoadMapMainBox setAutoresizingMask: UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth];
      
      // Top bar window
      UIView *topBarView = roadmap_bar_create_top_bar ();
      topBarView.tag = VIEW_TAG_TOP_BAR;
		[RoadMapMainBox addSubview: topBarView];
      gsTopBarHeight = topBarView.bounds.size.height;
		
		//Create bottom bar
		rect = RoadMapMainBox.bounds;
		int pos_y = rect.origin.y + rect.size.height - BOTTOM_BAR_HEIGHT;
      RoadMapBottomBar = roadmap_bar_create_bottom_bar (BOTTOM_BAR_HEIGHT,
                                                        pos_y);
		[RoadMapMainBox addSubview: RoadMapBottomBar];		

		//Create view controller
		RoadMapMainViewController = [[RoadMapViewController alloc] init];
		RoadMapMainViewController.view = RoadMapMainBox;
      //RoadMapMainBox = RoadMapMainViewController.view;
		RoadMapMainNavCont = [[UINavigationController alloc] initWithRootViewController:RoadMapMainViewController];
		
		//[RoadMapMainNavCont setNavigationBarHidden:YES animated:NO];
		[RoadMapMainWindow addSubview: RoadMapMainNavCont.view];
      
      //bg image
      rect.origin.y += self.statusBarFrame.size.height;
      RoadmapBgImageV = [[UIImageView alloc] initWithFrame:rect];
      [RoadmapBgImageV setAutoresizesSubviews: YES];
		[RoadmapBgImageV setAutoresizingMask: UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth];
      RoadmapBgImageV.tag = VIEW_TAG_BG_IMAGE;
      [RoadMapMainNavCont.view addSubview: RoadmapBgImageV];
      [RoadMapMainNavCont.view sendSubviewToBack:RoadmapBgImageV];
      //[RoadmapBgImageV release];
		
		//[RoadMapMainWindow setAutoresizesSubviews:YES];
      
      roadmap_main_adjust_skin();
		
		[RoadMapMainWindow makeKeyAndVisible];
	}


    if (RoadMapMainTitle != NULL) {
        free(RoadMapMainTitle);
    }
    RoadMapMainTitle = strdup (title);
}

- (void) presentModalView: (UIViewController *) modalController
{
	[RoadMapMainNavCont presentModalViewController:modalController animated: YES];
}

- (void) dismissModalView
{
	[RoadMapMainNavCont dismissModalViewControllerAnimated:YES];
}

- (void) pushView: (UIViewController *) viewController
{
	if ([[RoadMapMainNavCont viewControllers] count] == 1) {
      //[RoadMapMainNavCont setNavigationBarHidden:NO animated:NO];
      roadmap_canvas_cancel_touches();
	}
	[RoadMapMainNavCont pushViewController: viewController animated: YES];
}

- (void) popView: (BOOL) animated
{
	[RoadMapMainNavCont popViewControllerAnimated:animated];
}

- (void) showRoot: (BOOL) animated
{
	[RoadMapMainNavCont popToRootViewControllerAnimated:animated];
}

- (void) periodicCallback: (NSTimer *) timer
{
   int i;
   for (i = 0; i < ROADMAP_MAX_TIMER; ++i) {
      if (RoadMapMainPeriodicTimer[i].timer == timer) {
         (* RoadMapMainPeriodicTimer[i].callback) ();
         break;
      }
   }
}

-(void) setPeriodic: (float) interval andCallback: (RoadMapCallback) callback
{
   int index;
   struct roadmap_main_timer *timer = NULL;

   for (index = 0; index < ROADMAP_MAX_TIMER; ++index) {

      if (RoadMapMainPeriodicTimer[index].callback == callback) {
         return;
      }
      if (timer == NULL) {
         if (RoadMapMainPeriodicTimer[index].callback == NULL) {
            timer = RoadMapMainPeriodicTimer + index;
         }
      }
   }

   if (timer == NULL) {
      roadmap_log (ROADMAP_FATAL, "Timer table saturated");
   }

   timer->callback = callback;
   timer->timer = [NSTimer scheduledTimerWithTimeInterval: interval
                     target: self
                     selector: @selector(periodicCallback:)
                     userInfo: nil
                     repeats: YES];
}

-(void) removePeriodic: (RoadMapCallback) callback
{
   int index;

   for (index = 0; index < ROADMAP_MAX_TIMER; ++index) {

      if (RoadMapMainPeriodicTimer[index].callback == callback) {
         if (RoadMapMainPeriodicTimer[index].callback != NULL) {
            RoadMapMainPeriodicTimer[index].callback = NULL;
            [RoadMapMainPeriodicTimer[index].timer invalidate];
         }
         return;
      }
   }
}

-(void) inputCallback: (id) notify
{
   NSFileHandle *fh = [notify object];
   int i;
   int fd = [fh fileDescriptor];

   for (i = 0; i < ROADMAP_MAX_IO; ++i) {
      if (RoadMapMainIo[i].io.os.file == fd) {
         (* RoadMapMainIo[i].callback) (&RoadMapMainIo[i].io);
         [RoadMapMainIo[i].fh waitForDataInBackgroundAndNotify];
         break;
      }
   }
}

-(void) setInput: (RoadMapIO*) io andCallback: (RoadMapInput) callback
{
   int i;
   int fd = io->os.file; /* All the same on UNIX. */
   NSFileHandle *fh = [[NSFileHandle alloc] initWithFileDescriptor: fd];
   
   for (i = 0; i < ROADMAP_MAX_IO; ++i) {
      if (RoadMapMainIo[i].io.subsystem == ROADMAP_IO_INVALID) {
         RoadMapMainIo[i].io = *io;
         RoadMapMainIo[i].callback = callback;
         RoadMapMainIo[i].rl = NULL;
         RoadMapMainIo[i].fh = fh;
         RoadMapMainIo[i].type = IO_TYPE_INPUT;
         [[NSNotificationCenter defaultCenter] addObserver: self
                                                  selector:@selector(inputCallback:)
                                                      name:NSFileHandleDataAvailableNotification
                                                    object:fh];
         [fh waitForDataInBackgroundAndNotify];
         break;
      }
   }
}

-(void) removeIO: (RoadMapIO*) io
{
    int i;
    int fd = io->os.file; /* All the same on UNIX. */

    for (i = 0; i < ROADMAP_MAX_IO; ++i) {
        if (RoadMapMainIo[i].io.os.file == fd) {
            if (RoadMapMainIo[i].fh) {
                [[NSNotificationCenter defaultCenter]
                    removeObserver: self
                    name:NSFileHandleDataAvailableNotification
                    object:RoadMapMainIo[i].fh];

                [RoadMapMainIo[i].fh release];
            }

            if (RoadMapMainIo[i].rl) {
                CFSocketRef s = CFSocketCreateWithNative(NULL, RoadMapMainIo[i].io.os.socket, 0, NULL, NULL);
				CFRelease(s); //the previous call receives the original s but also increments the retain count.
				
                CFRunLoopRemoveSource (CFRunLoopGetCurrent( ),
                                       RoadMapMainIo[i].rl,
                                        kCFRunLoopCommonModes);

                CFRelease(RoadMapMainIo[i].rl);
                CFSocketInvalidate (s);
                CFRelease (s);
            }

            RoadMapMainIo[i].io.os.file = -1;
            RoadMapMainIo[i].io.subsystem = ROADMAP_IO_INVALID;
            break;
        }
    }
}

-(void) moviePreloadDoneCallback:(NSNotification *)notification
{
   MPMoviePlayerController *mp = [notification object];
   NSDictionary *userInfo = [notification userInfo];
   NSError *err = [userInfo objectForKey:@"error"];
   
   ssd_progress_msg_dialog_hide();
   
   if (err) {
      roadmap_log(ROADMAP_WARNING, "Error trying to play video (err code: %d)", [err code]);
      
      [[NSNotificationCenter defaultCenter]
       removeObserver: self
       name:MPMoviePlayerContentPreloadDidFinishNotification
       object:mp];
      
      [mp release];
      
      roadmap_messagebox("Error", "Video could not start");
   }

   [mp play];
}

- (void) playMovie: (const char *)url
{
   NSURL *urlFromString = [NSURL URLWithString:[NSString stringWithUTF8String:url]];
   
   if (urlFromString) {
      MPMoviePlayerController *mp = [[MPMoviePlayerController alloc] initWithContentURL:urlFromString];
      if (mp)
      {
         [[NSNotificationCenter defaultCenter]
          addObserver: self
          selector:@selector(moviePreloadDoneCallback:)
          name:MPMoviePlayerContentPreloadDidFinishNotification
          object:mp];
         
         mp.scalingMode = MPMovieScalingModeFill;
         
         ssd_progress_msg_dialog_show(roadmap_lang_get("Starting video..."));
      } else {
         roadmap_log(ROADMAP_WARNING, "Error trying to play video: %s", url);
         roadmap_messagebox("Error", "Video could not start");
      }
   } else {
      roadmap_log(ROADMAP_ERROR, "Error trying to play video: %s - bad url", url);
   }
}



- (void) newVersionCleanup
{
   NSLog(@"New verison, clrearing files");
   
   char **files;
   char **cursor;
   const char* directory;
   const char* source_directory;
   char file_name[256];
   
   //clean preferences
   if (roadmap_file_exists(roadmap_main_home(), "preferences"))
      roadmap_file_remove(roadmap_main_home(), "preferences");
   
   //clean postmortem
   if (roadmap_file_exists(roadmap_main_home(), "postmortem"))
      roadmap_file_remove(roadmap_main_home(), "postmortem");

   //clean map edt
   directory = roadmap_path_preferred("maps");
   files = roadmap_path_list (directory, ".dat");
   for (cursor = files; *cursor != NULL; ++cursor) {
      roadmap_file_remove(directory, *cursor);
   }
   roadmap_path_list_free (files);
   
   //copy default sound files
   source_directory = roadmap_path_join(roadmap_main_bundle(), "/sound");
   directory = roadmap_path_join(roadmap_main_home(), "/sound");
   NSFileManager *fileManager = [NSFileManager defaultManager];   
   NSString *srcStr = [NSString stringWithUTF8String:source_directory];   
   NSString *destinationStr = [NSString stringWithUTF8String:directory];
   NSError *error = nil;
   
   if (![fileManager copyItemAtPath:srcStr toPath:destinationStr error:&error]) {
      NSLog(@"Error copying sound directory");
   }
   roadmap_path_free(source_directory);
   roadmap_path_free(directory);
   
   //copy default lang files
   source_directory = roadmap_main_bundle();
   directory = roadmap_main_home();
   files = roadmap_path_list (source_directory, NULL);
   for (cursor = files; *cursor != NULL; ++cursor) {
      if (strstr(*cursor, "lang.") != NULL) {
         snprintf(file_name, sizeof(file_name), "%s/%s", source_directory, *cursor);
         srcStr = [NSString stringWithUTF8String:file_name];
         snprintf(file_name, sizeof(file_name), "%s/%s", directory, *cursor);
         destinationStr = [NSString stringWithUTF8String:file_name];
         if (![fileManager copyItemAtPath:srcStr toPath:destinationStr error:&error]) {
            NSLog(@"Error copying lang file %s", *cursor);
         }
      }
      
   }
   roadmap_path_list_free (files);
   
}

void appExceptionHandler(NSException *exception)
{
   //Experimental
   NSLog(@"appExceptionHandler () invoked !");
   roadmap_log (ROADMAP_ERROR, "appExceptionHandler () invoked !");
   
   NSArray *stack = [exception callStackReturnAddresses];
   NSNumber *num;
   char cNum[8];
   char addresses[512];
   int i;
   
   addresses[0] = 0;
   if (stack) {
      NSLog(@"Dumping call stack return addresses");
      roadmap_log (ROADMAP_ERROR, "appExceptionHandler (): Dumping call stack return addresses");
      for (i = 0; i < [stack count]; ++i) {
         num = (NSNumber *)[stack objectAtIndex:0];
         snprintf(cNum, sizeof(cNum), "\n%d",[num intValue]);
         strcat(addresses, cNum); 
      }
      NSLog(@"%s", addresses);
      roadmap_log (ROADMAP_ERROR, addresses);
   } else {
      NSLog(@"No stack trace available.");
      roadmap_log (ROADMAP_ERROR, "No stack trace available.");
   }
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
   NSURL *launchURL = NULL;
   
   if (launchOptions != NULL) {
      launchURL = (NSURL *)[launchOptions objectForKey:UIApplicationLaunchOptionsURLKey];
      if (launchURL)
         if (!roadmap_urlscheme_handle (launchURL))
            return NO;
   }
   
   
   TheApp = self;
   
   NSSetUncaughtExceptionHandler(appExceptionHandler);
   
   
   //Clear old version files
   const char *version = roadmap_start_version();
   const char *file_version = roadmap_version_read();
   int running_new_version = 0;
   if (file_version) {
      if (strcmp(version, file_version))
         running_new_version = 1;
   } else {
      running_new_version = 1;
   }
   
   if (running_new_version) {
      [self newVersionCleanup];
      roadmap_version_write (version);
   }

   //set model
   NSString *model = [[UIDevice currentDevice] model];
   if ([model compare:@"iPod touch"] == 0) {
      RoadMapMainPlatform = ROADMAP_MAIN_PLATFORM_IPOD;
   } else {
      RoadMapMainPlatform = ROADMAP_MAIN_PLATFORM_IPHONE;
   }
   
   //set os version
   NSString *sysVersion = [[UIDevice currentDevice] systemVersion];
   if ([sysVersion compare:@"3.0"] == 0 ||
       [sysVersion compare:@"3.0.1"] == 0) {
      RoadMapMainOsVersion = ROADMAP_MAIN_OS_30;
   } else {
      RoadMapMainOsVersion = ROADMAP_MAIN_OS_31;
   }
   
	//set statusbar style
	//[self setStatusBarStyle:UIStatusBarStyleBlackOpaque];
	
	
   
   
   dummy_url_request();

   
	
   int i;
   for (i = 0; i < ROADMAP_MAX_IO; ++i) {
      RoadMapMainIo[i].io.os.file = -1;
      RoadMapMainIo[i].io.subsystem = ROADMAP_IO_INVALID;
   }
   
   roadmap_keyboard_set_typing_lock_enable(TRUE);
   
   roadmap_start_subscribe (roadmap_start_event);
   
   roadmap_start (sArgc, sArgv);
   
   roadmap_device_events_register( on_device_event, NULL);
   
   roadmap_skin_register (roadmap_main_adjust_skin);
	
   return YES;
}


- (void)applicationWillResignActive:(UIApplication *)application
{
	//NSLog (@"applicationWillResignActive\n");
	IsGoingToSuspend = 1;
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
	//NSLog (@"applicationDidBecomeActive\n");
	IsGoingToSuspend = 0;
}

- (void)applicationWillTerminate:(UIApplication *)application
{
   //NSLog (@"applicationWillTerminate\n");

   static int exit_done;

   if (!exit_done++) {
      roadmap_start_exit ();
   }
   
   [self release];
}


//Memory management warnings

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application
{
	roadmap_log (ROADMAP_WARNING, "Application received memory warning");
   //roadmap_square_unload_all(); //clear tiles cache to release memory
}




/////////////////////////////
//NSURLConnection delegate
- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response
{
   [connection release]; //we have nothing to do with this connection
}

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error
{
   roadmap_log(ROADMAP_WARNING, "NSURLConnection failed");
   [connection release];
}

@end
