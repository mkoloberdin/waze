/* roadmap_browser.h - Embedded browser
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

#ifndef ROADMAP_BROWSER_H_
#define ROADMAP_BROWSER_H_

#include "roadmap.h"


#define WAZE_CMD_URL_PREFIX        ("waze://")
#define WAZE_EXTERN_URL_PREFIX     ("waze://?open_url=")
#define WEB_VIEW_URL_MAXSIZE       (2048)
#define BROWSER_BAR_NORMAL          0
#define BROWSER_BAR_EXTENDED        1

#if ((defined IPHONE) || (defined ANDROID))
#define BROWSER_WEB_VERSION         "1"
#else
#define BROWSER_WEB_VERSION         "0"
#endif


typedef struct
{
   int height;                            // The height of the browser
   int top_margin;                        // The top margin to show the native web view
   char url[WEB_VIEW_URL_MAXSIZE];        // url to load
} RMBrowserContext;


typedef void (*RMBrowserLauncherCb)( const RMBrowserContext* context );

void roadmap_browser_register_launcher( RMBrowserLauncherCb launcher_cb );

void roadmap_browser_register_close( RoadMapCallback close_cb );

void roadmap_browser_show (const char* title, const char* url, RoadMapCallback callback, int bar_type);
#ifdef IPHONE
BOOL roadmap_browser_show_preloaded (void);
void roadmap_browser_preload (const char* title, const char* url, RoadMapCallback callback, int bar_type);
void roadmap_browser_unload (void);
#endif //IPHONE

#endif /*ROADMAP_BROWSER_H_*/
