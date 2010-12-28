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
#include "roadmap_screen.h"
#include "roadmap_start.h"
#include "roadmap_config.h"
#include "roadmap_bar.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_button.h"
#include <string.h>

#define  SSD_WEB_VIEW_DIALOG_NAME         "WEB_VIEW_DALOG"
#define  SSD_WEB_VIEW_CNT_NAME            "WEB_VIEW_DALOG.MAIN CONTAINER"

static RMBrowserLauncherCb RMBrowserLauncher = NULL;     // Native browser launcher function
static RoadMapCallback     RMBrowserClose = NULL;        // Native browser close function - called to close the native browser control/view

static RMBrowserAttributes RMBrowserAttrs;               // Contains the attributes of the call to the browser
                                                         // Note has to be updated each call to browser

static const char*  SSD_WEB_VIEW_BTN_NAMES[] = { "WEB_VIEW_DALOG.LEFT1 BUTTON", "WEB_VIEW_DALOG.LEFT2 BUTTON",
                                                      "WEB_VIEW_DALOG.RIGHT1 BUTTON", "WEB_VIEW_DALOG.RIGHT2 BUTTON" };

void roadmap_browser_show_ssd( const char* url, int browser_flags );
static int title_btn_cb ( SsdWidget widget, const char *new_value );
static void roadmap_browser_format_title_bar( SsdWidget title_bar, int browser_flags );
static SsdWidget add_title_button( int btn_id, SsdWidget title_bar, int ssd_flags );
static int button_id( int flag );
#ifdef __SYMBIAN32__
static RoadMapConfigDescriptor RoadMapConfigShowExternal =
      ROADMAP_CONFIG_ITEM("Browser", "Show external browser");

//////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL roadmap_browser_show_external (void) {
   if (0 == strcmp (roadmap_config_get (&RoadMapConfigShowExternal), "yes")){
      return TRUE;
   }
   return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_browser_set_show_external (void) {
   roadmap_config_set (&RoadMapConfigShowExternal, "yes");
   roadmap_config_save(0);
}

static void roadmap_browser_config_init(void){
   static BOOL initialized = FALSE;
   if (initialized)
      return;

   roadmap_config_declare_enumeration ("preferences", &RoadMapConfigShowExternal, NULL, "no",
                  "yes", NULL);
   initialized = TRUE;
}
#endif //__SYMBIAN32__
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
/*  Name        : void roadmap_browser_hide()
 *  Purpose     : Hides the webview.
 *
 *  Params     :
 *  NOTE       :
 */
void roadmap_browser_hide(void){

   if ( RMBrowserClose )
      RMBrowserClose();
}


void roadmap_browser_show_embeded( RMBrowserContext* context ){
   if (!RMBrowserLauncher )
   {
      roadmap_log( ROADMAP_ERROR, "roadmap_browser_show_embeded - Browser launcher is not initialized..." );
      return;
   }

   if (!context)
   {
      roadmap_log( ROADMAP_ERROR, "roadmap_browser_show_embeded - Context is null..." );
      return;
   }

   context->flags |= BROWSER_FLAG_WINDOW_TYPE_EMBEDDED;

   RMBrowserLauncher( context );
}

char bin2hex(int val)
{
    int i;

    i = val & 0x0F;
    if (i >= 0 && i < 10)
        return '0' + i;
    else
        return 'A' + (i - 10);
}


char * encode_url(char *buf, const char *str, size_t buflen)
 {
     int i, j;
     int len;
     unsigned char c;
     int buflenm1;

     len = strlen(str);
     buf[buflen - 1] = '\0';
     buflenm1 = buflen - 1;
     for (i = 0, j = 0; i < len && j < buflenm1; i++) {
         c = (unsigned char) str[i];
         if (c == ' ') {
             buf[j++] = '%';
             if (j < buflenm1)
                 buf[j++] = bin2hex((c >> 4) & 0x0F);
             if (j < buflenm1)
                 buf[j++] = bin2hex(c & 0x0F);
         } else {
             buf[j] = str[i];
             j++;
         }
     }
     buf[j] = '\0';

     return buf;
}

/***********************************************************/
/*  Name        : void roadmap_browser_show()
 *  Purpose     : Shows the webview. Wrapper for the various platform launch
 *
 *  Params     : title - the container title
 *             : url - the url to load
 *  NOTE       : Single brwoser dialog currently supported. There will be no different dialog for another url request.
 */
void roadmap_browser_show (const char* title, const char* url, RoadMapCallback callback, int browser_flags )
{
   char temp[1024];
   RMBrowserAttributes attrs;
   /*
    * Set the attributes
    */

#if defined (ANDROID) || defined (IPHONE)
   roadmap_browser_reset_attributes( &attrs );
   attrs.on_close_cb = callback;
   attrs.title_attrs.title = title;

   roadmap_browser_show_extended( url, browser_flags, &attrs );
#elif defined(__SYMBIAN32__)
   roadmap_browser_show_ssd( url, browser_flags );
#elif defined (_WIN32)
   if (roadmap_spawn ("OperaL.exe", encode_url(temp, url, 1024)) == -1){
      roadmap_messagebox("Oops", "Could not locate Opera broswer. Please install Opera");
      return;
   }
#else
   roadmap_spawn ("firefox", encode_url(temp, url, 1024));
   return;
#endif

}


/***********************************************************/
/*  Name        : void roadmap_browser_show_extended()
 *  Purpose     : Shows the webview. Wrapper for the various platform launch
*                 Allows additional customization of the browser view
 *  Params     : title - the container title
 *             : url - the url to load
 *  NOTE       : Single browser dialog currently supported. There will be no different dialog for another url request.
 */
void roadmap_browser_show_extended ( const char* url, int browser_flags, const RMBrowserAttributes* attrs )
{
   char temp[1024];

   // Set the attributes
   RMBrowserAttrs = *attrs;
   /*
    * Show the view
    */
#if defined (ANDROID) || defined (IPHONE)
#ifdef SSD
   roadmap_browser_show_ssd( url, browser_flags );
#endif
#elif defined (__SYMBIAN32__)
   roadmap_browser_config_init();
   if (roadmap_browser_show_external())
      roadmap_external_browser(url);
   else
      roadmap_browser_show_ssd( url, browser_flags );
#elif defined (_WIN32)
   if (roadmap_spawn ("OperaL.exe", encode_url(temp, url, 1024)) == -1){
      roadmap_messagebox("Oops", "Could not locate Opera broswer. Please install Opera");
      return;
   }

#else
   roadmap_browser_show( attrs->title_attrs.title, url, attrs->on_close_cb, browser_flags );
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

#ifdef IPHONE
   if ( strstr( url, WAZE_EXTERN_URL_PREFIX ) == url )
   {
      const char* external_url = url + strlen( WAZE_EXTERN_URL_PREFIX );

      roadmap_log( ROADMAP_DEBUG, "Launching external url: %s", external_url );

      roadmap_main_open_url(external_url);

      return TRUE;

   }
#endif //IPHONE

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

/***********************************************************/
/*  Name        : void roadmap_browser_set_button_attrs
 *  Purpose     : Assigns the configuration to the supplied attributes structure
 *  Params      : attrs - attributes structure to be updated
 *              :
 */
void roadmap_browser_set_button_attrs( RMBrTitleAttributes* attrs, int flag, const char* label, RoadMapCallback cb,
      const char* icon_up, const char* icon_down )
{
   int btn_id = button_id( flag );
   int max_btns = ( sizeof( attrs->buttons )/ sizeof( attrs->buttons[0] ) );
   if ( ( btn_id >= 0 ) && ( btn_id < max_btns ) )
   {
      RMBrTitleBtnAttributes* btnAttrs = &attrs->buttons[btn_id];
      btnAttrs->btn_label = label ? label : "";
      btnAttrs->btn_cb = cb;
      btnAttrs->btn_icon_up = icon_up;
      btnAttrs->btn_icon_down = icon_down;
   }
}

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
   RoadMapCallback on_close_cb = RMBrowserAttrs.on_close_cb;

   roadmap_log( ROADMAP_INFO, "Callback: 0x%x", on_close_cb );

   // Close the native web window
   if ( RMBrowserClose )
      RMBrowserClose();

   // Callback function
   if ( on_close_cb )
      on_close_cb();
}

#ifdef SSD
#include "ssd/ssd_widget.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_dialog.h"

/***********************************************************/
/*  Name        : void roadmap_browser_show_ssd()
 *  Purpose     : Shows the webview with the ssd container
 *
 *  Params     :  see roadmap_browser_show
 *             :
 *             :
 */
void roadmap_browser_show_ssd( const char* url, int browser_flags )
{
   static SsdWidget dialog = NULL;
   SsdWidget container = NULL;
   SsdWidget title_bar = NULL;
   SsdWidget title_text = NULL;
   RMBrowserContext context;
   SsdSize dlg_size, cnt_size;
   RoadMapGuiRect rect = {0};

   int dialog_flags = 0;
   if (!RMBrowserLauncher )
   {
      roadmap_log( ROADMAP_ERROR, "Browser launcher is not initialized..." );
      return;
   }
#ifndef IPHONE
   /*
    * If it was previos call - release the previous dialog
    * NOTE: (Still there is leak on app exit. Can be moved to the on_close when the self-release mechanism
    * will be available
    */
   if ( dialog != NULL )
   {
      ssd_dialog_free( SSD_WEB_VIEW_DIALOG_NAME, FALSE );
      dialog = NULL;
   }

   if ( !(browser_flags & BROWSER_FLAG_WINDOW_NO_TITLE_BAR) )
      dialog_flags |= SSD_CONTAINER_TITLE;

   dialog = ssd_dialog_new( SSD_WEB_VIEW_DIALOG_NAME, "", on_dlg_close, dialog_flags );
   container = ssd_container_new( SSD_WEB_VIEW_CNT_NAME, NULL, SSD_MAX_SIZE, SSD_MAX_SIZE, 0 );
   ssd_widget_add( dialog, container );
   // AGA NOTE: In case of ssd_dialog_new dialog points to the scroll_container.
   // In case of activate dialog points to the container
   dialog = ssd_dialog_activate( SSD_WEB_VIEW_DIALOG_NAME, NULL );

   title_bar = ssd_widget_get( dialog, "title_bar" );

   roadmap_browser_format_title_bar( title_bar, browser_flags );

   // Recalculate size for the first time
   ssd_dialog_recalculate( SSD_WEB_VIEW_DIALOG_NAME );

   /*
    * Get the size of the dialog and the container
    */
   container = ssd_widget_get( dialog, SSD_WEB_VIEW_CNT_NAME );
   ssd_widget_get_size( dialog, &dlg_size, NULL );
   ssd_widget_get_size( container, &cnt_size, NULL );

   ssd_dialog_draw();
#endif //!IPHONE

   /*
    * Prepare the context
    */
   context.flags = browser_flags;
   context.attrs = RMBrowserAttrs;

   rect.minx = 0;
   rect.maxx = roadmap_canvas_width() - 1;
   rect.miny = dlg_size.height - cnt_size.height;
#ifdef TOUCH_SCREEN
   rect.maxy = roadmap_canvas_height() - 1 ;
#else
   rect.maxy = roadmap_canvas_height() - 1 - roadmap_bar_bottom_height() ;
#endif

   context.rect = rect;

   strncpy_safe( context.url, url, WEB_VIEW_URL_MAXSIZE );

   /*
    * Launch native control
    */
   RMBrowserLauncher( &context );
}

/***********************************************************/
/*  Name        : void roadmap_browser_format_title_bar( SsdWidget title_bar, int flags )
 *  Purpose     : Auxiliary
 *
 *  Params     :  title_bar - container
 *             :  flags - browser flags
 *             :
 */
static void roadmap_browser_format_title_bar( SsdWidget title_bar, int browser_flags )
{
   SsdWidget title_text = NULL;
   SsdWidget back_btn   = NULL;
   SsdWidget btn;
   RMBrTitleAttributes attrs = RMBrowserAttrs.title_attrs;
   const char* title = attrs.title;

#ifdef TOUCH_SCREEN
   if ( ( browser_flags & BROWSER_FLAG_WINDOW_TYPE_EXTENDED ) > 0 )
   {
      /*
       * Remove the title and the back button
       */
      title_text = ssd_widget_get( title_bar, "title_text" );
      back_btn = ssd_widget_get( title_bar, SSD_DIALOG_BUTTON_BACK_NAME );
      ssd_widget_remove( title_bar, title_text );
      ssd_widget_remove( title_bar, back_btn );

      /*
       * Title bar customization
       */
      if ( browser_flags & BROWSER_FLAG_TITLE_BTN_LEFT1 )
      {
         int ssd_flags = SSD_ALIGN_VCENTER;
         SsdWidget btn;
         btn = add_title_button( button_id( BROWSER_FLAG_TITLE_BTN_LEFT1 ), title_bar, ssd_flags );
         if ( btn )
            ssd_widget_set_offset( btn, ADJ_SCALE( -6 ), 0 );
      }
      if ( browser_flags & BROWSER_FLAG_TITLE_BTN_LEFT2 )
      {
         ssd_dialog_add_hspace( title_bar, ADJ_SCALE( 6 ), 0 );
         add_title_button( button_id( BROWSER_FLAG_TITLE_BTN_LEFT2 ), title_bar, SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE );
      }
      if ( ( browser_flags & BROWSER_FLAG_WINDOW_TYPE_EXTENDED ) > 0 )
      {
         // Title text
         SsdWidget text = ssd_text_new ("title_text", "" , 18, SSD_WIDGET_SPACE|SSD_ALIGN_VCENTER|SSD_ALIGN_CENTER);
         ssd_widget_set_color (text, "#ffffff", "#ff0000000");
         ssd_widget_add( title_bar, text );
      }
      if ( browser_flags & BROWSER_FLAG_TITLE_BTN_RIGHT2 )
      {
         add_title_button( button_id( BROWSER_FLAG_TITLE_BTN_RIGHT2 ), title_bar,
               SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE|SSD_ALIGN_RIGHT );
      }
      if ( browser_flags & BROWSER_FLAG_TITLE_BTN_RIGHT1 )
      {
         add_title_button( button_id( BROWSER_FLAG_TITLE_BTN_RIGHT1 ), title_bar,
               SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE|SSD_ALIGN_RIGHT );
      }
   }
#endif
   /*
    * Set the title text
    */
   title_text = ssd_widget_get( title_bar, "title_text" );
   ssd_text_set_text( title_text, roadmap_lang_get( title ) );
}



/***********************************************************/
/*  Name        : add_title_button
 *  Purpose     : Auxiliary
 *
 *  Params     :  void
 *             :
 *             :
 */
static SsdWidget add_title_button( int btn_id, SsdWidget title_bar, int ssd_flags )
{
   RMBrTitleAttributes* title_attrs = &RMBrowserAttrs.title_attrs;
   RMBrTitleBtnAttributes* btn_attrs = &title_attrs->buttons[btn_id];
   const char* icons[2] = { btn_attrs->btn_icon_up, btn_attrs->btn_icon_down };
   // AGA DEBUG
   roadmap_log( ROADMAP_DEBUG, "Adding button: %d. Name: %s, Label: %s, Icons: (%s,%s)",
         btn_id, SSD_WEB_VIEW_BTN_NAMES[btn_id], btn_attrs->btn_label, icons[0], icons[1] );
   if ( btn_attrs->btn_cb )
   {
      SsdClickOffsets offsets = { ADJ_SCALE( -6 ), ADJ_SCALE( -10 ), ADJ_SCALE( 6 ), ADJ_SCALE( 10 ) };
      SsdWidget btn = ssd_button_label_custom( SSD_WEB_VIEW_BTN_NAMES[btn_id], roadmap_lang_get( btn_attrs->btn_label ),
            ssd_flags, title_btn_cb, (const char**) icons, "#ffffff", "#ffffff" );
      ssd_widget_set_context( btn, btn_attrs->btn_cb );
      ssd_widget_set_click_offsets( btn, &offsets );
      ssd_widget_add( title_bar, btn );
      return btn;
   }
   return NULL;
}

/***********************************************************/
/*  Name        : title_btn_cb
 *  Purpose     : SsdCallback wrapper
 *  Params      :
 */
static int title_btn_cb ( SsdWidget widget, const char *new_value ) {
   RoadMapCallback cb = (RoadMapCallback) ssd_widget_get_context( widget );
   if (  cb == NULL )
   {
      roadmap_log( ROADMAP_WARNING, "The callback is undefined!" );
   }
   else
   {
      cb();
   }
   return 1;
}

/***********************************************************/
/*  Name        : reset_attributes
 *  Purpose     : Initializes the attributes
 *  Params      :
 */
void roadmap_browser_reset_attributes( RMBrowserAttributes* attrs ) {
   memset( attrs, 0, sizeof( RMBrowserAttributes ) );
}

/***********************************************************/
/*  Name        : button_id
 *  Purpose     : returns button id from the given flag
 *  Params      :
 */
static int button_id( int flag ) {
   if ( flag & BROWSER_FLAG_TITLE_BTN_LEFT1 ) return 0;
   if ( flag & BROWSER_FLAG_TITLE_BTN_LEFT2 ) return 1;
   if ( flag & BROWSER_FLAG_TITLE_BTN_RIGHT1 ) return 2;
   if ( flag & BROWSER_FLAG_TITLE_BTN_RIGHT2 ) return 3;
   return -1;
}


// CURRENTLY NOT IN USE
#if 0
/***********************************************************/
/*  Name        : void roadmap_browser_ssd_reset()
 *  Purpose     : Auxiliary
 *
 *  Params     :  dialog - dialog to reset
 *             :
 *             :
 */
static void roadmap_browser_ssd_reset( SsdWidget dialog )
{
   SsdWidget btn;
   SsdWidget title_text;
   SsdWidget title_bar = ssd_widget_get( dialog, "title_bar" );
   int flags = ssd_widget_get_flags( dialog );

   if ( !( flags & SSD_DIALOG_NO_BACK ) )
   {
      btn = ssd_widget_get( dialog, SSD_DIALOG_BUTTON_BACK_NAME );
      ssd_widget_show( btn );
   }
   /*
    * Custom buttons
    */
   btn = ssd_widget_get( dialog, SSD_WEB_VIEW_HOME_BTN_NAME );
   if ( btn != NULL )
      ssd_widget_hide( btn );

   btn = ssd_widget_get( dialog, SSD_WEB_VIEW_BACK_BTN_NAME );
   if ( btn != NULL )
      ssd_widget_hide( btn );

   btn = ssd_widget_get( dialog, SSD_WEB_VIEW_CLOSE_BTN_NAME );
   if ( btn != NULL )
      ssd_widget_hide( btn );

   title_text = ssd_widget_get( title_bar, "title_text" );
   ssd_text_set_text( title_text, "" );

}

/***********************************************************/
/*  Name        : BOOL roadmap_browser_ssd_set_button_callback( const char* button_name,  SsdCallback cb )
 *  Purpose     : Sets the callback for the button
 *                Auxiliary
 *  Params     :  cb - custom callback for home button
 *             :  button_name - name of the button to upadte
 *             :
 */
static BOOL roadmap_browser_ssd_set_button_callback( const char* button_name,  SsdCallback cb )
{
   SsdWidget btn = NULL;
   // Get the dialog
   SsdWidget webDlg = roadmap_browser_get_browser_widget();
   // Check if the browser is active
   if ( webDlg == NULL  )
   {
      roadmap_log( ROADMAP_WARNING, "Cannot update not existing dialog!" );
      return FALSE;
   }
   // Get the button
   btn = ssd_widget_get( webDlg, SSD_WEB_VIEW_HOME_BTN_NAME );
   if ( btn == NULL )
   {
      roadmap_log( ROADMAP_WARNING, "Cannot update not existing button: %s!", button_name );
      return FALSE;
   }
   ssd_widget_set_callback( btn, cb );

   return TRUE;
}

/***********************************************************/
/*  Name        : SsdWidget roadmap_browser_get_browser_widget
 *  Purpose     : Auxiliary
 *
 *  Params     :  void
 *             :
 *             :
 */
static SsdWidget roadmap_browser_get_browser_widget( void )
{
   // Remember the state
   BOOL is_active = !strcmp( ssd_dialog_currently_active_name(), SSD_WEB_VIEW_DIALOG_NAME );
   // Get the dialog
   SsdWidget webDlg = ssd_dialog_activate( SSD_WEB_VIEW_DIALOG_NAME, NULL );
   // Hide if it was not active
   if ( !is_active )
      ssd_dialog_hide_current( dec_cancel );

   return webDlg;
}

#endif

#endif   // SSD



