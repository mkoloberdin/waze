/* roadmap_androidbrowser.c - Android Browser functionality
 *
 * LICENSE:
 *
  *   Copyright 2010 Alex Agranovich (AGA), Waze Ltd
 *
 *   This file is part of RoadMap.
 *
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

#include "roadmap_androidbrowser.h"
#include "roadmap_browser.h"
#include "JNI/FreeMapJNI.h"
#include "ssd/ssd_dialog.h"

static void roadmap_androidbrowser_launcher( RMBrowserContext* context );
static void _resize( const RoadMapGuiRect* rect );

/***********************************************************/
/*  Name        : void roadmap_androidbrowser_init()
 *  Purpose     : Initializes the browser
 *  Params      : void
 */
void roadmap_androidbrowser_init( void )
{
   /*
    * Launcher and closer registration.
    */
   roadmap_browser_register_launcher( (RMBrowserLauncherCb) roadmap_androidbrowser_launcher );
   roadmap_browser_register_close( FreeMapNativeManager_HideWebView );
   roadmap_browser_register_resize( _resize );
}

/*************************************************************************************************
 * void roadmap_main_browser_launcher( RMBrowserContext* context )
 * Shows the android browser view
 *
 */
static void roadmap_androidbrowser_launcher( RMBrowserContext* context )
{
   FreeMapNativeManager_ShowWebView( context->url, context->rect.minx, context->rect.miny,
                                       context->rect.maxx, context->rect.maxy, context->flags );
}


/*************************************************************************************************
 * void roadmap_groups_browser_btn_home_cb( void )
 * Custom button callback - Groups browser
 *
 */
void roadmap_groups_browser_btn_home_cb( void )
{
   FreeMapNativeManager_LoadUrl( "javascript:home();" );
}
/*************************************************************************************************
 * void roadmap_groups_browser_btn_back_cb( void )
 * Custom button callback - Groups browser
 *
 */
void roadmap_groups_browser_btn_back_cb( void )
{
   FreeMapNativeManager_LoadUrl( "javascript:back();" );
}
/*************************************************************************************************
 * void _resize
 * Browser resize wrapper
 *
 */
static void _resize( const RoadMapGuiRect* rect )
{
   FreeMapNativeManager_ResizeWebView( rect->minx, rect->miny, rect->maxx, rect->maxy );
}
