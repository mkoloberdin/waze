/* roadmap_recommend_ssd.c - SSD implementation of the recommend dialog
 *
 *
 * LICENSE:
 *   Copyright 2010, Waze Ltd      Alex Agranovich (AGA)
 *
 *
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License V2 as published by
 *   the Free Software Foundation.
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
 *   See roadmap_start.h, ssd/.
 *
 *
 *
 */
#include <string.h>
#include "roadmap_screen.h"
#include "roadmap_recommend.h"
#include "roadmap_lang.h"
#include "roadmap_main.h"
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_button.h"
#include "ssd/ssd_bitmap.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_text.h"

#define RECOMMEND_DLG_NAME                "Recommend dialog"
#define RECOMMEND_DLG_TITLE               "Rate us"
#define RECOMMEND_DLG_TEXT_TITLE_FONT     19
#define RECOMMEND_DLG_TEXT_FONT           15
static RMRateLauncherCb RMRateLauncher = NULL;     // Native rate launcher function
static RoadMapCallback  RMOnCloseCb = NULL;        // The callback to be executed when the dialog is closed
#define RECOMMEND_DLG_TXT_LINE_1         "Can we assume that the feeling's mutual? If you've been enjoyning our app, we'd really appreciate it if you could leave us a nice review in the %s."
#if defined(ANDROID)
#define RECOMMEND_DLG_APP_STORE_NAME     "Market"
#elif defined(__SYMBIAN32__)
#define RECOMMEND_DLG_APP_STORE_NAME     "Ovi Store"
#else
#define RECOMMEND_DLG_APP_STORE_NAME     "Application Store"
#endif
#define RECOMMEND_DLG_TXT_LINE_2         "It'll really help us grow :)"



/*
 * Close request timeout callback
 */
static void close_request_cb( void )
{
   roadmap_main_remove_periodic( close_request_cb );
   if ( RMOnCloseCb )
      RMOnCloseCb();
}

/*
 * Dialog close callback
 */
static void on_recommend_dialog_close( int exit_code, void* context )
{
   // Call the callback out of the close dialog stack
   roadmap_main_set_periodic( 20, close_request_cb );
}

/*
 * Skip button callback
 */
static void on_recommend_dialog_skip( SsdWidget widget, const char *new_value )
{
   ssd_dialog_hide_current( dec_cancel );
}

/*
 * Rate button callback
 */
static void on_recommend_dialog_rate( SsdWidget widget, const char *new_value )
{
   ssd_dialog_hide_current( dec_cancel );
   RMRateLauncher();
   // Should be shown when waze backs to foreground
//   roadmap_messagebox_timeout( "", "Thanks for reviewing waze!", 3 );
}


/***********************************************************/
/*  Name        : void roadmap_recommend_register_launcher( RMRateLauncherCb launcher_cb )
 *  Purpose     : Stores the launcher function for the rating
 *
 *  Params     : [in] launcher - the caller function showing the native rating dialog
 *             :
 *             :
 */
void roadmap_recommend_register_launcher( RMRateLauncherCb launcher_cb )
{
   RMRateLauncher = launcher_cb;
}


/***********************************************************
 *  Name        : void roadmap_recommend_rate_us( RoadMapCallback on_close )
 *  Purpose     : Creates the Rate us dialog and shows it to the user
 *              : Launcher for the native rating application should be registered before calling this
 *  Params      : [in]  - on_close - callback to be called when the dialog is closed
 *              : [out] - none
 *  Returns     : void
 *  Notes       :
 */
void roadmap_recommend_rate_us( RoadMapCallback on_close )
{
   static SsdWidget dialog = NULL;
   SsdWidget group, bitmap;
   SsdWidget button, text;
   const char* icons[3];

   roadmap_log( ROADMAP_WARNING, "roadmap_recommend_rate_us is shown" );

   RMOnCloseCb= on_close;

   if ( !RMRateLauncher )
      return;

   if ( !dialog )
   {
      char text_str[512];
      // Format text string
      snprintf( text_str, 512, roadmap_lang_get( RECOMMEND_DLG_TXT_LINE_1 ), roadmap_lang_get( RECOMMEND_DLG_APP_STORE_NAME ) );
      snprintf( text_str + strlen( text_str ), 512 - strlen( text_str ), "\n%s\n", roadmap_lang_get( RECOMMEND_DLG_TXT_LINE_2 ) );

      // Create dialog
      dialog = ssd_dialog_new( roadmap_lang_get( RECOMMEND_DLG_NAME ), roadmap_lang_get( RECOMMEND_DLG_TITLE ), on_recommend_dialog_close,
            SSD_CONTAINER_TITLE );
      // Image draw
      group = ssd_container_new ( "Image container", NULL, SSD_MAX_SIZE, ADJ_SCALE( 106 ), SSD_END_ROW );
      ssd_widget_set_color ( group, NULL, NULL );
      ssd_dialog_add_vspace( group, ADJ_SCALE( 23 ), 0 );
      bitmap = ssd_bitmap_new( "Image bitmap", "rate_img", SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER );
      ssd_widget_add( group, bitmap );
      ssd_widget_add( dialog, group );
      // Title text
      group = ssd_container_new ( "Title text container", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE, SSD_END_ROW );
      ssd_widget_set_color ( group, NULL, NULL );
      text = ssd_text_new ("Label", roadmap_lang_get ( "we love you!"), RECOMMEND_DLG_TEXT_TITLE_FONT,
            SSD_TEXT_LABEL|SSD_ALIGN_VCENTER|SSD_ALIGN_CENTER );
      ssd_text_set_color( text, "#FFFFFF" );
      ssd_widget_add( group, text );
      ssd_widget_add( dialog, group );

      ssd_dialog_add_vspace( dialog, ADJ_SCALE( 10 ), 0 );

      // Text
      group = ssd_container_new ( "Title text container", NULL, ADJ_SCALE( 260 ), ADJ_SCALE( 150 ), SSD_ALIGN_CENTER|SSD_END_ROW );
      ssd_widget_set_color ( group, NULL, NULL );
      text = ssd_text_new( "Label", text_str, RECOMMEND_DLG_TEXT_FONT,
            SSD_TEXT_LABEL|SSD_ALIGN_VCENTER|SSD_ALIGN_CENTER );
      ssd_text_set_color( text, "#FFFFFF" );
      ssd_widget_add( group, text );
      ssd_widget_add( dialog, group );

      ssd_dialog_add_vspace( dialog, ADJ_SCALE( 35 ), 0 );

#ifdef TOUCH_SCREEN

    icons[0] = "welcome_btn_h";
    icons[1] = "welcome_btn";
    icons[2] = NULL;
    // Preload image to get dimensions
    group = ssd_container_new ( "Buttons group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE, SSD_ALIGN_CENTER | SSD_END_ROW );
    ssd_widget_set_color ( group, NULL, NULL );
    button = ssd_button_label_custom( "Skip", roadmap_lang_get( "Skip" ), SSD_ALIGN_CENTER | SSD_ALIGN_VCENTER,
          (SsdCallback) on_recommend_dialog_skip, icons, "#FFFFFF", "#FFFFFF" );
    text = ssd_widget_get( button, "label" );
    ssd_text_set_font_size( text, 14 );
    ssd_widget_add ( group, button );
    ssd_dialog_add_hspace( group, ADJ_SCALE( 8 ), SSD_ALIGN_CENTER | SSD_ALIGN_VCENTER );

    // Skip button

    button = ssd_button_label_custom( "Rate", roadmap_lang_get ( "Rate" ), SSD_ALIGN_CENTER | SSD_ALIGN_VCENTER,
          (SsdCallback) on_recommend_dialog_rate, icons, "#FFFFFF", "#FFFFFF" );
    text = ssd_widget_get( button, "label" );
    ssd_text_set_font_size( text, 14 );
    ssd_widget_add ( group, button );

    ssd_widget_add ( dialog, group );
#endif
   }

   ssd_dialog_activate( RECOMMEND_DLG_NAME, NULL );
   ssd_dialog_draw();
}

