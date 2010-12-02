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
#include "roadmap_gui.h"

#define WAZE_CMD_URL_PREFIX        ("waze://")
#define WAZE_EXTERN_URL_PREFIX     ("waze://?open_url=")
#define WEB_VIEW_URL_MAXSIZE       (2048)
#define BROWSER_BAR_NORMAL          0
#define BROWSER_BAR_EXTENDED        1


/*
 * Note if you use these flags you need to set the title attributes (callbacks and labels for the buttons)
 */
#define BROWSER_FLAG_TITLE_BTN_LEFT1              0x00000001        // Button at the title bar of the browser
#define BROWSER_FLAG_TITLE_BTN_LEFT2              0x00000002        // Button at the title bar of the browser
#define BROWSER_FLAG_TITLE_BTN_RIGHT1             0x00000004        // Button at the title bar of the browser
#define BROWSER_FLAG_TITLE_BTN_RIGHT2             0x00000008        // Button at the title bar of the browser

#define BROWSER_FLAG_BACK_BY_HW_BUTTON            0x00000010        // Use hw back button to perform "back" in browser

#define BROWSER_FLAG_WINDOW_TYPE_EXTENDED         (BROWSER_FLAG_TITLE_BTN_LEFT1| \
                                                   BROWSER_FLAG_TITLE_BTN_LEFT2| \
                                                   BROWSER_FLAG_TITLE_BTN_RIGHT1|\
                                                   BROWSER_FLAG_TITLE_BTN_RIGHT2)

/*
 * Browser window type flags
 */
#define BROWSER_FLAG_WINDOW_TYPE_NORMAL           0x00000000
#define BROWSER_FLAG_WINDOW_TYPE_TRANSPARENT      0x00000020
#define BROWSER_FLAG_WINDOW_TYPE_NO_SCROLL        0x00000040
#define BROWSER_FLAG_WINDOW_TYPE_EMBEDDED         0x00000080
#define BROWSER_FLAG_WINDOW_NO_TITLE_BAR          0x00000100        // Don't show title bar



#if ((defined IPHONE) || (defined ANDROID))
#define BROWSER_WEB_VERSION         "1"
#else
#define BROWSER_WEB_VERSION         "0"
#endif


/*
 * Title bar button attributes
 */
typedef struct
{
   const char* btn_icon_up;
   const char* btn_icon_down;
   const char* btn_label;
   RoadMapCallback btn_cb;
} RMBrTitleBtnAttributes;

/*
 * Title bar attributes
 *                ______  ______               _______   ______
 * Title form :  | Left1 | Left2 | Title text | Right1 | Right2 |
 */
typedef struct
{
   const char *title;
   /*
    * Buttons data { Left1, Left2, Right1, Right2 }
    */
   RMBrTitleBtnAttributes buttons[4];

} RMBrTitleAttributes;

/*
 * Browser attributes
 * Note: No duplicates - references already allocated memory
 */
typedef struct
{
   /*
    * Title attributes
    */
   RMBrTitleAttributes title_attrs;

   RoadMapCallback on_close_cb;  // To be called when the browser is closed


} RMBrowserAttributes;

/*
 * Browser context to be passed to the native control
 */
typedef struct
{
   RoadMapGuiRect rect;                   // The rectangle defining the dimensions of the native browser control
   char url[WEB_VIEW_URL_MAXSIZE];        // url to load
   int flags;
   RMBrowserAttributes attrs;
} RMBrowserContext;



/*
 * Simple browser view - only title text is shown
 */
void roadmap_browser_show (const char* title, const char* url, RoadMapCallback callback, int browser_flags );

void roadmap_browser_show_embeded( RMBrowserContext* context );
void roadmap_browser_hide(void);
/*
 * Customized browser view - buttons are shown at the title bar
 * (See RMTitleAttributes for definitions)
 */
void roadmap_browser_show_extended ( const char* url, int browser_flags, const RMBrowserAttributes* attrs );

void roadmap_browser_set_button_attrs( RMBrTitleAttributes* attrs, int flag, const char* label, RoadMapCallback cb,
      const char* icon_up, const char* icon_down );

void roadmap_browser_reset_attributes( RMBrowserAttributes* attrs );

typedef void (*RMBrowserLauncherCb)( const RMBrowserContext* context );

void roadmap_browser_register_launcher( RMBrowserLauncherCb launcher_cb );

void roadmap_browser_register_close( RoadMapCallback close_cb );

BOOL roadmap_browser_url_handler( const char* url );


void roadmap_browser_set_show_external (void);

#ifdef IPHONE
BOOL roadmap_browser_show_preloaded (void);
void roadmap_browser_preload (const char* title, const char* url, RoadMapCallback callback, int bar_type);
void roadmap_browser_unload (void);
#endif //IPHONE

#endif /*ROADMAP_BROWSER_H_*/
