/* roadmap_iphonebrowser.h - iPhone web browser
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
 */

#ifndef __ROADMAP_IPHONEBROWSER__H
#define __ROADMAP_IPHONEBROWSER__H


void roadmap_browser_iphone_show ( CGRect rect, const char *url, int flags, const char* title, RoadMapCallback on_close_cb);
void roadmap_browser_iphone_close (void);


@interface BrowserViewController : UIViewController <UIWebViewDelegate> {
   RoadMapCallback   gCallback;
   BOOL              gIsShown;
   UIWebView         *gBrowserView;
   BOOL              gIsEmbedded;
   CGRect            gFrame;
}


- (void) showPreloaded;

- (void) preload: (const char *) title
             url: (const char *) url
        callback: (RoadMapCallback) callback
             bar: (int) bar_type
           flags: (BOOL) flags
            rect: (CGRect) rect;

@end



#endif // __ROADMAP_IPHONEBROWSER__H