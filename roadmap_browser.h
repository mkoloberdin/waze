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

#define WEB_VIEW_URL_MAXSIZE       (512)

typedef struct
{
   int height;                            // The height of the browser
   int top_margin;                        // The top margin to show the native web view
   char url[WEB_VIEW_URL_MAXSIZE];        // url to load
} RMBrowserContext;


typedef void (*RMBrowserLauncherCb)( const RMBrowserContext* context );

void roadmap_browser_register_launcher( RMBrowserLauncherCb launcher_cb );

void roadmap_browser_register_close( RoadMapCallback close_cb );


void roadmap_browser_show (const char* title, const char* url, RoadMapCallback callback);

#endif /*ROADMAP_BROWSER_H_*/
