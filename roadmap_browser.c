/* roadmap_browser.c - Shows the dialog with the basic web browser source
 *
 * LICENSE:
 *
 *   Copyright 2010 Alex Agranovich (AGA),     Waze Ltd
 *
 *   This file is part of Waze.
 *
 *   Waze is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   Waze is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Waze; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "roadmap.h"
#include "roadmap_browser.h"
#include "roadmap_lang.h"
#include "roadmap_factory.h"
#include <string.h>

#define  SSD_WEB_VIEW_DIALOG_NAME         "WEB_VIEW_DALOG"
#define  SSD_WEB_VIEW_CNT_NAME            "WEB_VIEW_DALOG.MAIN CONTAINER"

static RMBrowserLauncherCb RMBrowserLauncher = NULL;     // Native browser launcher function
static RoadMapCallback     RMBrowserClose = NULL;        // Native browser close function - called to close the native browser control/view
static RoadMapCallback     RMBrowserCallback = NULL;     // Callback function supplied by browse request - called upon browser close

static void roadmap_browser_show_ssd();

/***********************************************************/
/*  Name        : roadmap_browser_register_launcher( RMBrowserLauncherCb launcher )
 *  Purpose     : Stores the web view launcher function
 *
 *  Params     : [in] launcher - the caller function showing the native web browser
 *             :                 with the matching parameters
 *             :
 */
void roadmap_browser_register_launcher( RMBrowserLauncherCb launcher_cb )
{
   RMBrowserLauncher = launcher_cb;
}

/***********************************************************/
/*  Name        : void roadmap_browser_register_close( RoadMapCallback close_cb )
 *  Purpose     : Stores the browser close function
 *
 *  Params     : [in] close_cb - function closing the native web browser
 *             :                 with the matching parameters
 *             :
 */
void roadmap_browser_register_close( RoadMapCallback close_cb )
{
   RMBrowserClose = close_cb;
}


/***********************************************************/
/*  Name        : void roadmap_browser_show()
 *  Purpose     : Shows the webview. Wrapper for the various platform launch
 *
 *  Params     : title - the container title
 *             : url - the url to load
 *  NOTE       : Single brwoser dialog currently supported. There will be no different dialog for another url request.
 */
void roadmap_browser_show (const char* title, const char* url, RoadMapCallback callback)
{
   if ( !RMBrowserLauncher )
   {
      roadmap_log( ROADMAP_ERROR, "Browser launcher is not initialized..." );
      return;
   }

   RMBrowserCallback = callback;
   /*
    * Show the view
    */
#ifdef SSD
   roadmap_browser_show_ssd( title, url );
#endif
}

BOOL roadmap_browser_url_handler( const char* url )
{
   if ( !url || !url[0] )
   {
      roadmap_log( ROADMAP_ERROR, "Url is not valid" );
      return FALSE;
   }

   roadmap_log( ROADMAP_DEBUG, "Processing url: %s", url );

   if ( strstr( url, WAZE_CMD_URL_PREFIX ) == url )
   {
      const char* url_action = url + strlen( WAZE_CMD_URL_PREFIX );
      RoadMapAction* action = roadmap_start_find_action( url_action );
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




#ifdef SSD
#include "ssd/ssd_widget.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_dialog.h"

/***********************************************************/
/*  Name        : void on_dlg_close( int exit_code, void* context )
 *  Purpose     : Close function for the browser dialog
 *
 *  Params     :  not in use
 *             :
 *             :
 */
static void on_dlg_close( int exit_code, void* context )
{
   // Close the native web window
   if ( RMBrowserClose )
      RMBrowserClose();

   // Callback function
   if ( RMBrowserCallback )
      RMBrowserCallback();
}


/***********************************************************/
/*  Name        : void roadmap_browser_show_ssd()
 *  Purpose     : Shows the webview with the ssd container
 *
 *  Params     :  see roadmap_browser_show
 *             :
 *             :
 */
void roadmap_browser_show_ssd( const char* title, const char* url )
{
   static SsdWidget dialog = NULL;
   static SsdWidget container = NULL;
   static RMBrowserContext context;
   SsdSize dlg_size, cnt_size;

   if ( !( dialog = ssd_dialog_activate( SSD_WEB_VIEW_DIALOG_NAME, NULL ) ) )
   {
      dialog = ssd_dialog_new( SSD_WEB_VIEW_DIALOG_NAME, roadmap_lang_get( title ), on_dlg_close, SSD_CONTAINER_TITLE );
      container = ssd_container_new( SSD_WEB_VIEW_CNT_NAME, NULL, SSD_MAX_SIZE, SSD_MAX_SIZE, 0 );
      ssd_widget_add( dialog, container );
      // AGA NOTE: In case of ssd_dialog_new dialog points to the scroll_container.
      // In case of activate dialog points to the container
      dialog = ssd_dialog_activate( SSD_WEB_VIEW_DIALOG_NAME, NULL );
   }

   // Recalculate size for the first time
   ssd_dialog_recalculate( SSD_WEB_VIEW_DIALOG_NAME );

   /*
    * Get the size of the dialog and the container
    */
   container = ssd_widget_get( dialog, SSD_WEB_VIEW_CNT_NAME );
   ssd_widget_get_size( dialog, &dlg_size, NULL );
   ssd_widget_get_size( container, &cnt_size, NULL );
   /*
    * Prepare the context
    */
   context.top_margin = dlg_size.height - cnt_size.height;
   context.height = cnt_size.height;
   strncpy_safe( context.url, url, WEB_VIEW_URL_MAXSIZE );

   ssd_dialog_draw();

   RMBrowserLauncher( &context );
}
#endif   // SSD



