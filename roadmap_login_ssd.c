/* roadmap_login_ssd.c - SSD implementation of the Login UI and functionality
 *
 *
 * LICENSE:
 *   Copyright 2009, Waze Ltd
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
 *   See roadmap_login.h, ssd/.
 *
 * TODO:
 *  > Add to/Update lang entries for hebrew labels
 */


#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_button.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_entry.h"
#include "ssd/ssd_entry_label.h"
#include "ssd/ssd_choice.h"
#include "roadmap_keyboard.h"
#include "roadmap_config.h"
#include "roadmap_lang.h"
#include "Realtime/Realtime.h"
#include "Realtime/RealtimeDefs.h"
#include "Realtime/RealtimePrivacy.h"
#include "ssd/ssd_checkbox.h"
#include "roadmap_device.h"
#include "roadmap_sound.h"
#include "roadmap_car.h"
#include "roadmap_mood.h"
#include "roadmap_path.h"
#include "roadmap_social.h"
#include "roadmap_foursquare.h"
#include "roadmap_main.h"
#include "roadmap_messagebox.h"
#include "ssd/ssd_progress_msg_dialog.h"
#include "ssd/ssd_bitmap.h"
#include "ssd/ssd_separator.h"
#include "roadmap_welcome_wizard.h"
#include "roadmap_login.h"
#include "roadmap_device_events.h"
#include "roadmap_screen.h"
#include "roadmap_res.h"
#include "roadmap_analytics.h"

//======== Local Types ========


//======== Defines ========
#define RM_SIGNUP_DLG_NAME			"Create Account Dialog"
#define RM_SIGNUP_DLG_BTN   "SignUp"
#define RM_NEW_EXISTING_NAME  "New Existing"
#define RM_SIGNUP_DLG_TITLE		"Profile"
#define RM_SIGNUP_ENTRY_TITLE	RM_SIGNUP_DLG_TITLE


#define RM_SIGNUP_DLG_LABEL_HOFFSET		5			// Horizontal offset for the label
#if (defined(__SYMBIAN32__) && defined(TOUCH_SCREEN))
#define RM_SIGNUP_DLG_LABEL_CNT_WIDTH  120      // Base width of the label container
#else
#define RM_SIGNUP_DLG_LABEL_CNT_WIDTH  160      // Base width of the label container
#endif

#define RM_SIGNUP_DLG_ENTRY_HEIGHT     30      // Base entry height
#define RM_SIGNUP_DLG_LABEL_FONT		   14	        // Sign in text font size
#define RM_SIGNUP_DLG_LABEL_COLOR      "#202020"
#if (defined(WIN32))
#define RM_SIGNUP_ENTRY_WIDTH       120
#elif (defined(__SYMBIAN32__))
#define RM_SIGNUP_ENTRY_WIDTH       130
#else
#define RM_SIGNUP_ENTRY_WIDTH       190         /* Entry width in pixels. Has to be defined and configured */
                                       /* for each screen size by taken into consideration the    */
                                       /* portrait/landscape issues     *** AGA ***          */
#endif

//======== Globals ========
static SsdWidget sgSignUpDlg = NULL;
static SsdWidget sgLastLoginDlg = NULL;
static int SsdLoginShown = 0;
static const char *yesno_label[2];
static const char *yesno[2];
static BOOL sgSignUpDlgShow = FALSE;
static BOOL sgNewExistingInProcess = FALSE;

extern RoadMapConfigDescriptor RT_CFG_PRM_NAME_Var;
extern RoadMapConfigDescriptor RT_CFG_PRM_NKNM_Var;
extern RoadMapConfigDescriptor RT_CFG_PRM_PASSWORD_Var;


static RoadMapConfigDescriptor RoadMapConfigCarName =
                        ROADMAP_CONFIG_ITEM("Trip", "Car");



//======= Local interface ========
static void create_dialog();
static BOOL next_btn_callback( int         exit_code, const char* value, void*       context );
static BOOL validate_login_data( const char *username, const char* password, const char* confirm_password, const char* email );
static int on_update( SsdWidget this, const char *new_value );
static int on_create( SsdWidget this, const char *new_value );
static void OnDeviceEvent( device_event event, void* context );
static int on_signup_skip( SsdWidget this, const char *new_value);
/***********************************************************
 *  Name        : roadmap_login_update_dlg_show
 *  Purpose     : Shows the update account dialog. Used for the create/update account forms
 *  Params      : [in]  - none
 *              : [out] - none
 *  Returns    :
 *  Notes       :
 */
void roadmap_login_update_dlg_show( void )
{
   static BOOL s_event_registered = FALSE;
   const char* username = NULL;
   SsdCallback callback = NULL;
   SsdWidget signup_btn;

   if ( sgSignUpDlgShow == TRUE )
      return;
   /* Register to receive device event */
   /* TODO :: Add generic mechanism for the dialogs redraw on orientation change *** AGA *** */

   if ( !s_event_registered )
   {
      s_event_registered = TRUE;
      roadmap_device_events_register( OnDeviceEvent, NULL );
   }
   if ( !ssd_dialog_exists( RM_SIGNUP_DLG_NAME ) )
	   create_dialog();

   ssd_dialog_activate( RM_SIGNUP_DLG_NAME, NULL );

   username = RealTime_GetUserName();

   if ( username && *username && Realtime_IsLoggedIn() )
   {	// Update user
	   callback = on_update;
   }
   else
   {	// Create user
	   callback = on_create;
   }
#ifdef TOUCH_SCREEN
   // Button callback
   signup_btn = ssd_widget_get( sgSignUpDlg, RM_SIGNUP_DLG_BTN );
   ssd_widget_set_callback( signup_btn, callback );
#endif
   ssd_dialog_draw();

   sgLastLoginDlg =  sgSignUpDlg;

   sgSignUpDlgShow = TRUE;
}

void on_signup_dlg_close( int exit_code, void* context )
{
	sgSignUpDlgShow = FALSE;
}

#ifndef TOUCH_SCREEN
static int on_register_softkey( SsdWidget this, const char *new_value, void *context )
{
   const char* username = NULL;
   if ( username && *username && Realtime_IsLoggedIn() )
   {  // Update user
      return on_update( NULL, NULL );

   }
   else
   {  // Create user
      return on_create( NULL, NULL );
   }
}

static int on_skip_softkey( SsdWidget this, const char *new_value, void *context )
{
   roadmap_login_on_signup_skip();
   return 1;
}
#endif



/***********************************************************
 *  Name        : get_signup_entry_group
 *  Purpose     : Labeled entry
 *  Params      : [in]  - none
 *              : [out] - none
 *  Returns     :
 *  Notes       :  AGA TODO:: Add general labeled entry widget
 */
static SsdWidget get_signup_entry( const char* label_text, const char* entry_name, const char* edit_box_title, CB_OnKeyboardDone post_done_cb )
{
   SsdWidget entry;

   entry = ssd_entry_label_new( entry_name, roadmap_lang_get ( label_text ), RM_SIGNUP_DLG_LABEL_FONT, RM_SIGNUP_DLG_LABEL_CNT_WIDTH,
                                             RM_SIGNUP_DLG_ENTRY_HEIGHT, SSD_END_ROW | SSD_WIDGET_SPACE | SSD_WS_TABSTOP, "" );
   ssd_entry_label_set_label_offset( entry, RM_SIGNUP_DLG_LABEL_HOFFSET );
   ssd_entry_label_set_label_color( entry, RM_SIGNUP_DLG_LABEL_COLOR );
   ssd_entry_label_set_kb_params( entry, RM_SIGNUP_ENTRY_TITLE, roadmap_lang_get( edit_box_title ), NULL, post_done_cb, SSD_KB_DLG_SHOW_NEXT_BTN|SSD_KB_DLG_INPUT_ENGLISH );
   ssd_entry_label_set_editbox_title( entry, roadmap_lang_get( edit_box_title ) );
   return entry;
}

/***********************************************************
 *  Name        : create_dialog
 *  Purpose     : Creates the create/update account dialog
 *  Params      : [in]  - none
 *              : [out] - none
 *  Returns    :
 *  Notes       :
 */
static void create_dialog()
{
    SsdWidget group;
    SsdWidget box;
    SsdWidget button, text;
    const char* icons[3];
    SsdWidget label_cnt, label;
    SsdWidget group_title;

    sgSignUpDlg = ssd_dialog_new( RM_SIGNUP_DLG_NAME, roadmap_lang_get( RM_SIGNUP_DLG_TITLE ), on_signup_dlg_close, SSD_CONTAINER_TITLE );

    // Space before white container
    ssd_dialog_add_vspace( sgSignUpDlg, ADJ_SCALE( 5 ), 0 );

    box = ssd_container_new ( "Sign Up Box", NULL, SSD_MAX_SIZE,
          SSD_MIN_SIZE, SSD_END_ROW | SSD_ALIGN_CENTER | SSD_CONTAINER_BORDER | SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE );

    // Space before detail title
    ssd_dialog_add_vspace( box, ADJ_SCALE( 16 ), 0 );

    // Your details title
    group = ssd_container_new ( "Details title group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE, SSD_END_ROW );
    ssd_dialog_add_hspace( group, ADJ_SCALE( RM_SIGNUP_DLG_LABEL_HOFFSET ), SSD_ALIGN_VCENTER );
    group_title = ssd_text_new( "Details title", roadmap_lang_get( "Your Details" ), 15, SSD_TEXT_LABEL );
    ssd_text_set_color( group_title, "#202020" );
    ssd_widget_add( group, group_title );
    ssd_widget_add( box, group );

    // Separator
//    ssd_dialog_add_vspace( box, ADJ_SCALE( 4 ), 0 );
//    group = ssd_container_new ( "Separator group", NULL, 0.96*ssd_container_get_width(), SSD_MIN_SIZE,
//                                                                                              SSD_END_ROW );
//    ssd_widget_add( group, ssd_separator_new( "Details title separator", SSD_ALIGN_VCENTER ) );
//    ssd_widget_add( box, group );
    ssd_dialog_add_vspace( box, ADJ_SCALE( 18 ), 0 );

    // Username
    group = get_signup_entry( "Username", "Username", "User name", next_btn_callback );
    ssd_widget_add ( box, group );

    ssd_dialog_add_vspace( box, ADJ_SCALE( 8 ), 0 );

    // Password
    group = get_signup_entry( "Password", "Password", "Password", next_btn_callback );
    ssd_widget_add ( box, group );

    ssd_dialog_add_vspace( box, ADJ_SCALE( 8 ), 0 );

    // Confirm Password
    group = get_signup_entry( "Confirm password", "ConfirmPassword", "Confirm password", next_btn_callback );
    ssd_widget_add ( box, group );

    ssd_dialog_add_vspace( box, ADJ_SCALE( 18 ), 0 );

    // Nickname
    group = get_signup_entry( "Nickname", "Nickname", "Nickname", next_btn_callback );
    ssd_widget_add ( box, group );

    ssd_dialog_add_vspace( box, ADJ_SCALE( 8 ), 0 );

    // Email
    group = get_signup_entry( "Email", "Email", "Email", next_btn_callback );
    ssd_widget_add ( box, group );

    ssd_dialog_add_vspace( box, ADJ_SCALE( 14 ), 0 );

    // Send Update
    group = ssd_container_new ("Send Update group", NULL, SSD_MAX_SIZE,
              SSD_MIN_SIZE, SSD_END_ROW | SSD_WS_TABSTOP );
    ssd_widget_set_color ( group, "#000000", "#ffffff" );
    ssd_dialog_add_hspace( group, ADJ_SCALE( RM_SIGNUP_DLG_LABEL_HOFFSET ), 0 );
    ssd_widget_add( group, ssd_checkbox_new ( "send_updates", TRUE,  0, NULL, NULL, NULL, CHECKBOX_STYLE_DEFAULT ) );
    label_cnt = ssd_container_new ( "Label container", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_ALIGN_VCENTER );
    ssd_widget_set_color ( label_cnt, NULL, NULL );
    ssd_dialog_add_hspace( label_cnt, ADJ_SCALE( 10 ), 0 );
    label = ssd_text_new ("Label", roadmap_lang_get ( "Send me updates"), RM_SIGNUP_DLG_LABEL_FONT, SSD_TEXT_NORMAL_FONT|SSD_TEXT_LABEL|SSD_ALIGN_VCENTER );
    ssd_widget_add ( label_cnt, label );
    ssd_widget_add ( group, label_cnt );
    ssd_widget_add (box, group);

    ssd_dialog_add_vspace( box, ADJ_SCALE( 22 ), 0 );

#ifdef TOUCH_SCREEN

    icons[0] = "welcome_btn";
    icons[1] = "welcome_btn_h";
    icons[2] = NULL;
    // Skip button
    group = ssd_container_new ( "Buttons group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE, SSD_ALIGN_CENTER | SSD_END_ROW );
    button = ssd_button_label_custom( "Skip", roadmap_lang_get ("Skip"), SSD_ALIGN_CENTER | SSD_ALIGN_VCENTER, on_signup_skip,
          icons, 2, "#FFFFFF", "#FFFFFF",14 );
    text = ssd_widget_get( button, "label" );
    text->flags |= SSD_TEXT_NORMAL_FONT;
    ssd_text_set_font_size( text, 15 );
    ssd_widget_add ( group, button );
    ssd_dialog_add_hspace( group, ADJ_SCALE( 10 ), SSD_ALIGN_CENTER | SSD_ALIGN_VCENTER );

    // Signup button
    button = ssd_button_label_custom( "SignUp", roadmap_lang_get ("Next"), SSD_ALIGN_CENTER | SSD_ALIGN_VCENTER, on_update,
          icons, 2, "#FFFFFF", "#FFFFFF",14 );
    text = ssd_widget_get( button, "label" );
    text->flags |= SSD_TEXT_NORMAL_FONT;
    ssd_text_set_font_size( text, 14 );
    ssd_widget_add ( group, button );

    ssd_widget_add ( box, group );

    ssd_dialog_add_vspace( box, ADJ_SCALE( 10 ), 0 );
#endif

    ssd_widget_add ( sgSignUpDlg, box );

    /* Makes it possible to click in the vicinity of the button  */
    ssd_widget_set_click_offsets_ext(  sgSignUpDlg, 0, 0, 0, 20 );

#ifndef TOUCH_SCREEN
    ssd_widget_set_right_softkey_text (  sgSignUpDlg, roadmap_lang_get( "Next" ) );
    ssd_widget_set_right_softkey_callback (  sgSignUpDlg, on_register_softkey );

    ssd_widget_set_left_softkey_text (  sgSignUpDlg, roadmap_lang_get( "Skip" ) );
    ssd_widget_set_left_softkey_callback (  sgSignUpDlg, on_skip_softkey );
#endif
}

static int new_existing_buttons_callback( SsdWidget widget, const char *new_value )
{
   if (( widget != NULL ) && !strcmp( widget->name, "New" ) )
   {
      roadmap_analytics_log_event (ANALYTICS_EVENT_NEW_USER_OPTION, ANALYTICS_EVENT_INFO_ACTION, "New");
	   roadmap_login_update_dlg_show();
	   return 1;
   }
   else if ( ( widget != NULL ) && !strcmp(widget->name, "Existing" ) )
   {
      roadmap_analytics_log_event (ANALYTICS_EVENT_NEW_USER_OPTION, ANALYTICS_EVENT_INFO_ACTION, "Existing");
     roadmap_login_details_dialog_show_un_pw();
     return 1;
   }
   return 0;
}

static SsdWidget space(int height){
   SsdWidget space;

#if (defined(WIN32) && !defined(TOUCH_SCREEN))
   height = height / 2;
#endif
   space = ssd_container_new ("spacer", NULL, SSD_MAX_SIZE, height, SSD_WIDGET_SPACE|SSD_END_ROW);
   ssd_widget_set_color (space, NULL,NULL);
   return space;
}

#ifndef TOUCH_SCREEN
static int on_softkey_existing(SsdWidget widget, const char *new_value, void *context){
   roadmap_login_details_dialog_show_un_pw();
   return 1;
}

static int on_softkey_new(SsdWidget widget, const char *new_value, void *context){
   roadmap_login_update_dlg_show();
   return 1;
}
#else
static int on_right_softkey_new_existing_touch( SsdWidget widget, const char *new_value, void *context )
{
   return 1;
}

#endif

/*
 * TOUCH and OPENGL only dialog implementation.
 */
#if defined(TOUCH_SCREEN) && defined(OPENGL)
void roadmap_login_new_existing_dlg(){

   static SsdWidget sNewExistingDlg = NULL;
   SsdWidget dialog;
   SsdWidget main_container, bubble_container;
   SsdWidget bubble_bmp, group_bg_bmp;
   SsdWidget bubble_title;
   SsdWidget btn_container, new_user_group, existing_user_group;
   const char* icons[3];
   SsdWidget text;
   SsdWidget button;

   /*
    * If currently in process - don't show the dialog
    */
   if ( sgNewExistingInProcess )
      return;

   if ( !ssd_dialog_exists( RM_NEW_EXISTING_NAME ) )
   {
      int bubble_width, bubble_height;
      int group_width, group_height;
      int btn_container_width, btn_container_height;
      dialog = ssd_dialog_new ( RM_NEW_EXISTING_NAME,
                        roadmap_lang_get ("Welcome"),
                        NULL,
                        SSD_CONTAINER_TITLE|SSD_DIALOG_NO_SCROLL|SSD_DIALOG_NO_BACK );

      main_container = ssd_container_new ("Welcome", NULL,
                          SSD_MAX_SIZE,SSD_MAX_SIZE,SSD_WIDGET_SPACE|SSD_END_ROW);
      ssd_widget_set_color ( main_container, NULL,NULL );

      ssd_dialog_add_vspace( main_container, ADJ_SCALE( 2 ), SSD_END_ROW );

      /*
       * Bubble drawing
       */
      bubble_width = roadmap_canvas_width() - 2*ADJ_SCALE( 5 );
      bubble_height = ADJ_SCALE( 280 ) + 10; // Check this in the future

      bubble_container = ssd_container_new ( "Bubble container", NULL, bubble_width, bubble_height, SSD_WIDGET_SPACE|SSD_ALIGN_CENTER|SSD_END_ROW );
      ssd_widget_set_color ( bubble_container, NULL,NULL );
      bubble_bmp = ssd_bitmap_new("Bubble bitmap", "Bubble_02_stretch", SSD_BITMAP_MIDDLE_STRETCH|SSD_ALIGN_CENTER );
      ssd_bitmap_set_middle_stretch( bubble_bmp, bubble_width, bubble_height );
      ssd_widget_add ( bubble_container, bubble_bmp );

      /*
       * Bubble title
       */
      bubble_title = ssd_text_new ( "Bubble title", roadmap_lang_get ("Login"), 16, SSD_TEXT_LABEL|SSD_END_ROW );
      ssd_widget_set_offset( bubble_title, ADJ_SCALE( 14 ), ADJ_SCALE( 14 ) );
      ssd_widget_add ( bubble_bmp, bubble_title );

      ssd_dialog_add_vspace( bubble_bmp, ADJ_SCALE( 24 ), SSD_ALIGN_CENTER|SSD_END_ROW );

      /*
       * New user group definition
       */
      group_width = bubble_width - 2*ADJ_SCALE( 12 );
      group_height = ADJ_SCALE( 100 );
      btn_container_width = ADJ_SCALE( 260 );
      btn_container_height = SSD_MIN_SIZE;
      // Group container
      new_user_group = ssd_container_new ( "Group container", NULL, btn_container_width, btn_container_height, SSD_ALIGN_CENTER|SSD_END_ROW );
      ssd_widget_set_color ( new_user_group, NULL,NULL );
      group_bg_bmp = ssd_bitmap_new( "Group container background", "welcome_gray", SSD_BITMAP_MIDDLE_STRETCH|SSD_ALIGN_CENTER );
      ssd_bitmap_set_middle_stretch( group_bg_bmp, group_width, group_height );
      ssd_widget_add ( new_user_group, group_bg_bmp );
      // Button container
      btn_container = ssd_container_new ( "Button container", NULL, btn_container_width, btn_container_height, SSD_ALIGN_CENTER|SSD_END_ROW );
      ssd_widget_set_color ( btn_container, NULL,NULL );
      ssd_widget_add ( group_bg_bmp, btn_container );
      ssd_dialog_add_vspace( btn_container, ADJ_SCALE( 22 ), SSD_ALIGN_CENTER|SSD_END_ROW );
      // Label
      text = ssd_text_new ( "Button title", roadmap_lang_get ("New user"), 14, SSD_TEXT_LABEL|SSD_TEXT_NORMAL_FONT|SSD_END_ROW );
      ssd_widget_set_color( text, "#545454", "#545454" );
      ssd_widget_set_offset( text, ADJ_SCALE( 4 ), 0 );
      ssd_widget_add ( btn_container, text );

      ssd_dialog_add_vspace( btn_container, ADJ_SCALE( 10 ), SSD_ALIGN_CENTER|SSD_END_ROW );
      icons[0] = "welcome_btn_large";
      icons[1] = "welcome_btn_large_h";
      icons[2] = NULL;
      button = ssd_button_label_custom( "New", roadmap_lang_get ("Sign up"), SSD_ALIGN_CENTER|SSD_END_ROW, new_existing_buttons_callback,
            icons, 2, "#FFFFFF", "#FFFFFF" ,14);
      text = ssd_widget_get( button, "label" );
      text->flags |= SSD_TEXT_NORMAL_FONT;
      ssd_widget_add ( btn_container, button );

      ssd_widget_add ( bubble_bmp, new_user_group );

      ssd_dialog_add_vspace( bubble_bmp, ADJ_SCALE( 5 ), SSD_ALIGN_CENTER|SSD_END_ROW );

      /*
       * Existing user container
       */
      group_width = bubble_width - 2*ADJ_SCALE( 12 );
      group_height = ADJ_SCALE( 100 );
      btn_container_width = ADJ_SCALE( 260 );
      btn_container_height = SSD_MIN_SIZE;
      // Group container
      existing_user_group = ssd_container_new ( "Group container", NULL, btn_container_width, btn_container_height, SSD_ALIGN_CENTER|SSD_END_ROW );
      ssd_widget_set_color ( existing_user_group, NULL,NULL );
      group_bg_bmp = ssd_bitmap_new( "Group container background", "welcome_gray", SSD_BITMAP_MIDDLE_STRETCH|SSD_ALIGN_CENTER );
      ssd_bitmap_set_middle_stretch( group_bg_bmp, group_width, group_height );
      ssd_widget_add ( existing_user_group, group_bg_bmp );
      // Button container
      btn_container = ssd_container_new ( "Button container", NULL, btn_container_width, btn_container_height, SSD_ALIGN_CENTER|SSD_END_ROW );
      ssd_widget_set_color ( btn_container, NULL,NULL );
      ssd_widget_add ( group_bg_bmp, btn_container );
      ssd_dialog_add_vspace( btn_container, ADJ_SCALE( 22 ), SSD_ALIGN_CENTER|SSD_END_ROW );
      // Label
      text = ssd_text_new ( "Button title", roadmap_lang_get ("Existing user"), 14, SSD_TEXT_LABEL|SSD_TEXT_NORMAL_FONT|SSD_END_ROW );
      ssd_widget_set_color( text, "#545454", "#545454" );
      ssd_widget_set_offset( text, ADJ_SCALE( 4 ), 0 );
      ssd_widget_add ( btn_container, text );

      ssd_dialog_add_vspace( btn_container, ADJ_SCALE( 10 ), SSD_ALIGN_CENTER|SSD_END_ROW );
      icons[0] = "welcome_btn_large_g";
      icons[1] = "welcome_btn_large_g_h";
      icons[2] = NULL;
      button = ssd_button_label_custom( "Existing", roadmap_lang_get ( "Get Started" ), SSD_ALIGN_CENTER|SSD_END_ROW, new_existing_buttons_callback,
            icons, 2, "#FFFFFF", "#FFFFFF",14 );
      text = ssd_widget_get( button, "label" );
      text->flags |= SSD_TEXT_NORMAL_FONT;
      ssd_widget_add ( btn_container, button );

      ssd_widget_add ( bubble_bmp, existing_user_group );



      ssd_widget_add ( main_container, bubble_container );
      ssd_widget_add ( dialog, main_container );

      /*
       * Right soft key is defined for the touch devices with the hw back button: this callback makes it
       * impossible to be closed
       */
      ssd_widget_set_right_softkey_callback( dialog, on_right_softkey_new_existing_touch );


      sNewExistingDlg = dialog;
   }
   /*
    * Activate the dialog only if it is not active
    */
   ssd_dialog_activate( RM_NEW_EXISTING_NAME, NULL );

   ssd_dialog_draw();

   sgNewExistingInProcess = TRUE;
}
#else
void roadmap_login_new_existing_dlg(){

   static SsdWidget sNewExistingDlg = NULL;
   SsdWidget dialog;
   SsdWidget group;
   SsdWidget text, group2;
   SsdWidget button;
   int bubble1_flags, bubble2_flags;

   /*
    * If currently in process - don't show the dialog
    */
   if ( sgNewExistingInProcess )
	   return;

   if ( !ssd_dialog_exists( RM_NEW_EXISTING_NAME ) )
   {
	   dialog = ssd_dialog_new ( RM_NEW_EXISTING_NAME,
								roadmap_lang_get ("Welcome"),
								NULL,
								SSD_CONTAINER_TITLE|SSD_DIALOG_NO_SCROLL|SSD_DIALOG_NO_BACK );

	   group = ssd_container_new ("Welcome", NULL,
								  SSD_MAX_SIZE,SSD_MAX_SIZE,SSD_WIDGET_SPACE|SSD_END_ROW);
	   ssd_widget_set_color (group, NULL,NULL);
#ifdef TOUCH_SCREEN
	   ssd_widget_add (group, space(20));
	   group2 = ssd_bitmap_new("group2", "Bubble_01", SSD_END_ROW|SSD_ALIGN_CENTER);

	   text = ssd_text_new ("Label1", roadmap_lang_get ("EXISTING user"), 16, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER);
	   ssd_widget_set_color(text,"#d52c6b", "#d52c6b");
	   ssd_widget_set_offset(text, 10, -7);
	   ssd_widget_add (group2, text);

	   button = ssd_button_label ( "Existing", roadmap_lang_get ("Sign In"),
				SSD_ALIGN_VCENTER|SSD_END_ROW|SSD_ALIGN_RIGHT|SSD_WS_TABSTOP|SSD_WS_DEFWIDGET, new_existing_buttons_callback ) ;
	   if (ssd_widget_rtl(NULL))
		  ssd_widget_set_offset(button, 10, -7);
	   else
		  ssd_widget_set_offset(button, -10, -7);
	   ssd_widget_add (group2, button);

	   ssd_widget_add (group, group2);
	   ssd_widget_add (group, space(20));
	   group2 = ssd_bitmap_new("group2", "Bubble_02", SSD_ALIGN_CENTER);
	   ssd_widget_set_color (group2, NULL,NULL);
	   text = ssd_text_new ("Label1", roadmap_lang_get ("NEW user"), 16, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER);
	   ssd_widget_set_color(text,"#698a21", "#698a21");
	   ssd_widget_set_offset(text, 10, -7);

	   ssd_widget_add (group2, text);
	   button = ssd_button_label ("New", roadmap_lang_get ("Sign Up"),
				SSD_WS_TABSTOP|SSD_ALIGN_VCENTER|SSD_END_ROW|SSD_ALIGN_RIGHT|SSD_WS_TABSTOP, new_existing_buttons_callback );
	   if (ssd_widget_rtl(NULL))
		  ssd_widget_set_offset(button, 10, -7);
	   else
		  ssd_widget_set_offset(button, -10, -7);
	   ssd_widget_add (group2, button);

	   ssd_widget_add (group, group2);

	   ssd_widget_add (dialog, group);
      /*
       * Right soft key is defined for the touch devices with the hw back button: this callback makes it
       * impossible to be closed
       */
      ssd_widget_set_right_softkey_callback( dialog, on_right_softkey_new_existing_touch );
#else
      if (ssd_widget_rtl(NULL)){
         bubble1_flags = SSD_END_ROW|SSD_ALIGN_BOTTOM;
         bubble2_flags = SSD_ALIGN_RIGHT|SSD_ALIGN_BOTTOM;
      }else{
         bubble1_flags = SSD_ALIGN_RIGHT|SSD_ALIGN_BOTTOM;
         bubble2_flags = SSD_END_ROW|SSD_ALIGN_BOTTOM;
      }
      group2 = ssd_bitmap_new("group2", "Bubble_01", bubble1_flags);
      ssd_widget_set_offset(group2, 10, 0);

      text = ssd_text_new ("Label1", roadmap_lang_get ("EXISTING user"), 16, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER);
      ssd_widget_set_color(text,"#d52c6b", "#d52c6b");
      ssd_widget_set_offset(text, 30, -47);
      ssd_widget_add (group2, text);

      ssd_widget_add (group, group2);
      group2 = ssd_bitmap_new("group2", "Bubble_02", bubble2_flags);
      ssd_widget_set_offset(group2, 10, -7);
      ssd_widget_set_color (group2, NULL,NULL);
      text = ssd_text_new ("Label1", roadmap_lang_get ("NEW user"), 16, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER);
      ssd_widget_set_color(text,"#698a21", "#698a21");
      ssd_widget_set_offset(text, 30, -14);

      ssd_widget_add (group2, text);
      ssd_widget_add (group, group2);

      ssd_widget_add (dialog, group);
      ssd_widget_set_right_softkey_text       ( dialog, roadmap_lang_get("Sign In"));
	   ssd_widget_set_right_softkey_callback   ( dialog, on_softkey_existing);

	   ssd_widget_set_left_softkey_text       ( dialog, roadmap_lang_get("Get Started"));
	   ssd_widget_set_left_softkey_callback   ( dialog, on_softkey_new);
#endif

	   sNewExistingDlg = dialog;
   }
   /*
    * Activate the dialog only if it is not active
    */
   ssd_dialog_activate( RM_NEW_EXISTING_NAME, NULL );

   ssd_dialog_draw();

   sgNewExistingInProcess = TRUE;
}
#endif

/*
 * SSD related handling
 */
void roadmap_login_ssd_on_login_cb( BOOL bDetailsVerified, roadmap_result rc )
{
   if( bDetailsVerified )
   {
	  sgNewExistingInProcess = FALSE;
	  ssd_dialog_hide_all( dec_cancel );
	  SsdLoginShown = 0;
   }
}


#ifndef TOUCH_SCREEN
static int on_ok_softkey(SsdWidget this, const char *new_value, void *context){
	roadmap_login_on_ok(this, new_value);
	ssd_dialog_hide_all(dec_ok);
	return 0;
}

static int on_softkey_login(SsdWidget this, const char *new_value, void *context){
   roadmap_login_on_login(this, new_value);
   return 0;
}

static int on_softkey_back(SsdWidget this, const char *new_value, void *context){
   ssd_dialog_hide_current(dec_close);
   return 1;
}
#endif

static void on_car_changed(void){

	char *icon[2];
	char *car_name;
    const char *config_car;

	config_car = roadmap_config_get (&RoadMapConfigCarName);
	car_name = roadmap_path_join("cars", config_car);
	icon[0] = car_name;
	icon[1] = NULL;

	ssd_dialog_change_button(roadmap_lang_get ("Car"), (const char **)&icon[0], 1);
}

static int on_car_select( SsdWidget this, const char *new_value){
	roadmap_car_dialog(on_car_changed);
	return 0;
}

#ifndef TOUCH_SCREEN
static void on_mood_changed(void){

	char *icon[2];
   icon[0] = (char *)roadmap_mood_get();
	icon[1] = NULL;

	ssd_dialog_change_button(roadmap_lang_get ("Mood"), (const char **)&icon[0], 1);
	free(icon[0]);
}

static int on_mood_select( SsdWidget this, const char *new_value){
	roadmap_mood_dialog(on_mood_changed);
	return 0;
}
#endif

static int twitter_button_cb( SsdWidget this, const char *new_value){
	roadmap_twitter_setting_dialog();
	return 0;
}

static int foursquare_button_cb( SsdWidget this, const char *new_value){
   roadmap_foursquare_login_dialog();
   return 0;
}

static int facebook_button_cb( SsdWidget this, const char *new_value){
   roadmap_facebook_setting_dialog();
   return 0;
}


static void close_timeout(void)
{
	roadmap_main_remove_periodic( close_timeout );
	roadmap_login_on_ok(NULL, NULL);
}

static void on_close_dialog (int exit_code, void* context)
{
#ifdef TOUCH_SCREEN
	 if ( exit_code != dec_cancel )
	 {
		SsdLoginShown = 0;

		//roadmap_config_set( &RT_CFG_PRM_NKNM_Var, roadmap_login_dlg_get_nickname() );
	   roadmap_main_set_periodic( 10, close_timeout );
	 }
#endif
}

void roadmap_login_details_dialog_show_un_pw (void) {

   static SsdWidget sUnPwDlg = NULL;
   static const char* sDlgName = "Log In";
   int width;
   int tab_flag = SSD_WS_TABSTOP;


   /*
  	 * Currently shown login function for the
  	 * further processing in callbacks
  	 */

   roadmap_login_set_show_function( roadmap_login_details_dialog_show_un_pw );

   width = roadmap_canvas_width () / 2;


   /*
    * TODO:: Free the dialog after hiding
    */
   if ( !ssd_dialog_exists( sDlgName ) ) {

      SsdWidget dialog;
      SsdWidget entry_label;
      SsdWidget box;
      SsdWidget button;
      SsdWidget space;

      dialog = ssd_dialog_new ( sDlgName, roadmap_lang_get ( sDlgName ), NULL, SSD_CONTAINER_TITLE );
#ifdef TOUCH_SCREEN
      ssd_dialog_add_vspace( dialog, ADJ_SCALE( 10 ), SSD_END_ROW );
#endif

      box = ssd_container_new ("box group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE, SSD_WIDGET_SPACE | SSD_END_ROW | SSD_ROUNDED_CORNERS |
                                                                           SSD_ROUNDED_WHITE | SSD_POINTER_NONE | SSD_CONTAINER_BORDER );

      ssd_dialog_add_vspace( box, ADJ_SCALE( 28 ), SSD_END_ROW );
      // Username
      entry_label = get_signup_entry( "Username", "Username", "Username", NULL );
      ssd_widget_add ( box, entry_label );

      ssd_dialog_add_vspace( box, ADJ_SCALE( 24 ), SSD_END_ROW );
      // Password
      entry_label = get_signup_entry( "Password", "Password", "Password", NULL );
      ssd_widget_add ( box, entry_label );

      ssd_dialog_add_vspace( box, ADJ_SCALE( 24 ), SSD_END_ROW );
      // Password
      entry_label = get_signup_entry( "Nickname", "Nickname", "Nickname", NULL );
      ssd_widget_add ( box, entry_label );

      ssd_dialog_add_vspace( box, ADJ_SCALE( 28 ), SSD_END_ROW );

      ssd_widget_add (dialog, box);

      ssd_dialog_add_vspace( dialog, ADJ_SCALE( 16 ), SSD_END_ROW );


      button = ssd_button_label ("LogIn", roadmap_lang_get ("Log In"),
               SSD_ALIGN_CENTER | SSD_START_NEW_ROW
                        | SSD_WS_TABSTOP, roadmap_login_on_login );
      ssd_widget_add (dialog, button);

      ssd_dialog_add_vspace( dialog, ADJ_SCALE( 20 ), SSD_END_ROW );


      space = ssd_container_new ("spacer", NULL, SSD_MAX_SIZE, 20,
               SSD_WIDGET_SPACE | SSD_END_ROW);
      ssd_widget_set_color (space, NULL, NULL);
      ssd_widget_add (dialog, space);

      /* Makes it possible to click in the vicinity of the button  */
      ssd_widget_set_click_offsets_ext( dialog, 0, 0, 0, ADJ_SCALE( 20 ) );

#ifndef TOUCH_SCREEN
      ssd_widget_set_right_softkey_text ( dialog, roadmap_lang_get("Log In"));
      ssd_widget_set_right_softkey_callback ( dialog, on_softkey_login);

      ssd_widget_set_left_softkey_text ( dialog, roadmap_lang_get("Back"));
      ssd_widget_set_left_softkey_callback ( dialog, on_softkey_back);
#endif

      sUnPwDlg = dialog;
   }
   else
   {
      ssd_dialog_recalculate( sDlgName );
   }

   if (  /* !SsdLoginShown && */ !Realtime_is_random_user() )
   {
      ssd_dialog_set_value ( "Username", roadmap_config_get (
               &RT_CFG_PRM_NAME_Var ) );
      ssd_dialog_set_value ("Password", roadmap_config_get (
               &RT_CFG_PRM_PASSWORD_Var ) );
      ssd_dialog_set_value ("Nickname", roadmap_config_get (
               &RT_CFG_PRM_NKNM_Var ) );

   }

   sgLastLoginDlg = sUnPwDlg;
   ssd_dialog_draw ();
   ssd_dialog_activate( sDlgName, NULL );
}

static SsdWidget create_button_group (const char* group_name,
                                      const char* logo,
                                      const char* text,
                                      SsdCallback callback)
{
   const char *edit_button[] = {"edit_right", "edit_left"};
   const char *icon[2];
   SsdWidget button;
   SsdWidget container;
   const char *buttons[2];
   int tab_flag = SSD_WS_TABSTOP;

   SsdWidget group = ssd_container_new (group_name, NULL,
                                        SSD_MAX_SIZE,ssd_container_get_row_height(),
                                        SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag);
   icon[0] = logo;
   icon[1] = NULL;
   container =  ssd_container_new ("space", NULL, 47,
                                   SSD_MIN_SIZE, SSD_ALIGN_VCENTER);
   ssd_widget_set_color(container, NULL, NULL);
   ssd_widget_add (container,
                   ssd_button_new (text, text, (const char **)&icon[0], 1,
                                   SSD_START_NEW_ROW|SSD_ALIGN_VCENTER,
                                   callback));
   group->callback = callback;
   ssd_widget_add (group, container);
   ssd_widget_add (group,
                   ssd_text_new (text, text, SSD_MAIN_TEXT_SIZE, SSD_TEXT_NORMAL_FONT|SSD_ALIGN_VCENTER));

#ifdef TOUCH_SCREEN
   if (!ssd_widget_rtl(NULL)){
      buttons[0] = edit_button[0];
      buttons[1] = edit_button[0];
   }else{
      buttons[0] = edit_button[1];
      buttons[1] = edit_button[1];
   }
   button = ssd_button_new ("edit_button", "", &buttons[0], 2,
                            SSD_ALIGN_VCENTER|SSD_ALIGN_RIGHT, callback);
   ssd_widget_add(group, button);
#endif

   return group;
}

void roadmap_login_profile_dialog_show( void )
{
   static SsdWidget sProfileDlg = NULL;
   static const char* sDlgName = "Profile";
   char *car_name;
   const char *config_car;
   char *icon[2];
   int row_height = ssd_container_get_row_height();
   int total_width = ssd_container_get_width();
   int width;
   int tab_flag = SSD_WS_TABSTOP;

   /* Current shown login function for the
    * further processing in callbacks
    */
   roadmap_login_set_show_function( (RoadmapLoginDlgShowFn) roadmap_login_profile_dialog_show );

   width = total_width/2;

   if ( !ssd_dialog_exists( sDlgName ) ) {

      SsdWidget dialog;
      SsdWidget group;
      SsdWidget box;
      SsdWidget button;
      SsdWidget space;
      SsdWidget entry_label;
      const char *edit_button[] = {"edit_right", "edit_left"};
      const char *buttons[2];

      dialog = ssd_dialog_new ( sDlgName, roadmap_lang_get(sDlgName), on_close_dialog,
                               SSD_CONTAINER_TITLE);

#ifdef TOUCH_SCREEN
      space = ssd_container_new ("spacer", NULL, SSD_MAX_SIZE, 5, SSD_WIDGET_SPACE|SSD_END_ROW);
      ssd_widget_set_color (space, NULL,NULL);
      ssd_widget_add(dialog, space);
#endif

      box = ssd_container_new ("box group", NULL,
                               total_width,SSD_MIN_SIZE,
                               SSD_WIDGET_SPACE|SSD_END_ROW|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE|SSD_POINTER_NONE|SSD_CONTAINER_BORDER|SSD_ALIGN_CENTER);

      //Username
      group = ssd_container_new ("Name group", NULL,
                                 SSD_MAX_SIZE,row_height,
                                 SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag);
      ssd_widget_set_color (group, "#000000", "#ffffff");
      entry_label = ssd_entry_label_new_confirmed( "Username", roadmap_lang_get("Username"), SSD_MAIN_TEXT_SIZE, RM_SIGNUP_DLG_LABEL_CNT_WIDTH,
                                                   SSD_ROW_HEIGHT/2, SSD_ALIGN_VCENTER, roadmap_lang_get("Username"),
                                                   roadmap_lang_get("Change of username and password should be done on www.waze.com, and only then entered here. Continue?") );
      ssd_entry_label_set_kb_params( entry_label, roadmap_lang_get( "User name" ), NULL, NULL, NULL, SSD_KB_DLG_INPUT_ENGLISH );
      ssd_entry_label_set_editbox_title( entry_label, roadmap_lang_get( "User name" ) );
      ssd_widget_add( group, entry_label );

      ssd_widget_add (box, group);

      ssd_widget_add( box, ssd_separator_new("separator", SSD_END_ROW) );

      //Password
      group = ssd_container_new ("PW group", NULL,
                                 SSD_MAX_SIZE,row_height,
                                 SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag);
      ssd_widget_set_color (group, "#000000", "#ffffff");
      entry_label = ssd_entry_label_new( "Password", roadmap_lang_get("Password"), SSD_MAIN_TEXT_SIZE, RM_SIGNUP_DLG_LABEL_CNT_WIDTH,
                                                   SSD_ROW_HEIGHT/2, SSD_ALIGN_VCENTER, roadmap_lang_get("Password") );
      ssd_entry_label_set_text_flags( entry_label, SSD_TEXT_PASSWORD );
      ssd_entry_label_set_kb_params( entry_label, roadmap_lang_get( "Password" ), NULL, NULL, NULL, SSD_KB_DLG_INPUT_ENGLISH );
      ssd_entry_label_set_editbox_title( entry_label, roadmap_lang_get( "Password" ) );
      ssd_widget_add ( group, entry_label );
      ssd_widget_add (box, group);
      ssd_widget_add(box, ssd_separator_new("separator", SSD_END_ROW));

      //Nickname
      group = ssd_container_new ("Nick group", NULL,
                                 SSD_MAX_SIZE, row_height,
                                 SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag);
      ssd_widget_set_color (group, "#000000", "#ffffff");

      entry_label = ssd_entry_label_new( "Nickname", roadmap_lang_get("Nickname"), SSD_MAIN_TEXT_SIZE, RM_SIGNUP_DLG_LABEL_CNT_WIDTH,
                                                   SSD_ROW_HEIGHT/2, SSD_ALIGN_VCENTER, roadmap_lang_get("Nickname") );
      ssd_entry_label_set_kb_params( entry_label, roadmap_lang_get( "Nickname" ), NULL, NULL, NULL, SSD_KB_DLG_INPUT_ENGLISH );
      ssd_entry_label_set_editbox_title( entry_label, roadmap_lang_get( "Nickname" ) );
      ssd_widget_add (group, entry_label );
      ssd_widget_add (box, group);


      ssd_widget_add (dialog, box);


      box = ssd_container_new ("Privacy section", NULL,
                                 total_width,SSD_MIN_SIZE,
                                 SSD_WIDGET_SPACE|SSD_END_ROW|SSD_ROUNDED_CORNERS|
                                 SSD_ROUNDED_WHITE|SSD_POINTER_NONE|SSD_CONTAINER_BORDER|SSD_ALIGN_CENTER);
      ssd_widget_set_color (box, "#000000", "#ffffff");

      //Privacy
      group = ssd_container_new ("Privacy group", NULL,
                                 SSD_MAX_SIZE, row_height,
                                 SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag);
      ssd_widget_set_color (group, "#000000", "#ffffff");

#ifdef TOUCH_SCREEN
      if (!ssd_widget_rtl(NULL)){
         buttons[0] = edit_button[0];
         buttons[1] = edit_button[0];
      }else{
         buttons[0] = edit_button[1];
         buttons[1] = edit_button[1];
      }
      button = ssd_button_new ("edit_button", "", &buttons[0], 2,
                               SSD_ALIGN_VCENTER|SSD_ALIGN_RIGHT, NULL);
      ssd_widget_add(group, button);
#endif
      icon[0] = "privacy_settings";
      icon[1] = NULL;
      ssd_widget_add (group,
                      ssd_button_new ("Privacy btn", "Privacy btn", (const char **)&icon[0], 1,
                                      0,
                                      NULL));

      ssd_widget_add (group,
                      ssd_text_new ("Privacy Text", roadmap_lang_get("Privacy settings"), SSD_MAIN_TEXT_SIZE,
                            SSD_TEXT_NORMAL_FONT|SSD_TEXT_LABEL|SSD_ALIGN_VCENTER));
      group->callback = RealtimePrivacySettingsWidgetCallBack;


      ssd_widget_add (box, group);
      ssd_widget_add(box, ssd_separator_new("separator", SSD_END_ROW));

      //Allow ping
      group = ssd_checkbox_row_new("AllowPing", roadmap_lang_get ("I agree to be pinged"),
            TRUE, NULL,NULL,NULL,CHECKBOX_STYLE_ON_OFF);
      ssd_widget_add(box,group);

      ssd_widget_add(dialog,box);

#ifndef TOUCH_SCREEN
      group = ssd_container_new ("Car group", NULL,
                                 total_width, row_height,
                                 SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag|SSD_ROUNDED_CORNERS
                                 |SSD_ROUNDED_WHITE|SSD_POINTER_NONE|SSD_CONTAINER_BORDER|SSD_ALIGN_CENTER);
      ssd_widget_set_color (group, "#000000", "#ffffff");
      icon[0] = (char *)roadmap_mood_get();
      icon[1] = NULL;
      ssd_widget_add (group,
                      ssd_text_new ("Mood Text", roadmap_lang_get("Mood"), SSD_MAIN_TEXT_SIZE,
                                    SSD_TEXT_NORMAL_FONT|SSD_TEXT_LABEL|SSD_ALIGN_VCENTER));
      ssd_widget_add (group,
                      ssd_button_new (roadmap_lang_get ("Mood"), roadmap_lang_get(roadmap_mood_get()), (const char **)&icon[0], 1,
                                      SSD_ALIGN_CENTER,
                                      on_mood_select));
      group->callback = on_mood_select;
      ssd_widget_add (dialog, group);

#endif

      //Social networks
      box = ssd_container_new ("social group", NULL,
                               total_width,SSD_MIN_SIZE,
                               SSD_WIDGET_SPACE|SSD_END_ROW|SSD_ROUNDED_CORNERS
                               |SSD_ROUNDED_WHITE|SSD_POINTER_NONE|SSD_CONTAINER_BORDER|SSD_ALIGN_CENTER);

      //Twitter
      group = create_button_group ("Twitter group",
                                   "twitter_logo",
                                   "Twitter",
                                   twitter_button_cb);
      ssd_widget_add (box, group);

#if 1 //enable when facebook is supported
      //Facebook
      ssd_widget_add (box, ssd_separator_new ("separator", 0));
      group = create_button_group ("Facebook group",
                                   "facebook_settings",
                                   "Facebook",
                                   facebook_button_cb);
      ssd_widget_add (box, group);
#endif

      //Foursquare
      if (roadmap_foursquare_is_enabled()) {
         ssd_widget_add (box, ssd_separator_new ("separator", 0));
         group = create_button_group ("Foursquare group",
                                      "foursquare",
                                      "Foursquare",
                                      foursquare_button_cb);
         ssd_widget_add (box, group);
      }

      ssd_widget_add (dialog, box);

      //Avatar
      box = ssd_container_new ("Car group", NULL,
                                 total_width,SSD_MIN_SIZE,
                                 SSD_WIDGET_SPACE|SSD_END_ROW|SSD_ROUNDED_CORNERS|
                                 SSD_ROUNDED_WHITE|SSD_POINTER_NONE|SSD_CONTAINER_BORDER|SSD_ALIGN_CENTER);
      ssd_widget_set_color (box, "#000000", "#ffffff");

      group = ssd_container_new ("Privacy group", NULL,
                                 SSD_MAX_SIZE, row_height,
                                 SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag);
      ssd_widget_set_color (group, "#000000", "#ffffff");

      config_car = roadmap_config_get (&RoadMapConfigCarName);
      car_name = roadmap_path_join("cars", config_car);

      icon[0] = car_name;
      icon[1] = NULL;
      ssd_widget_add (group,
                            ssd_button_new (roadmap_lang_get ("Car"), roadmap_lang_get(config_car), (const char **)&icon[0], 1,
                                            SSD_ALIGN_RIGHT,
                                            on_car_select));

#ifdef TOUCH_SCREEN
      if (!ssd_widget_rtl(NULL)){
         buttons[0] = edit_button[0];
         buttons[1] = edit_button[0];
      }else{
         buttons[0] = edit_button[1];
         buttons[1] = edit_button[1];
      }
      button = ssd_button_new ("edit_button", "", &buttons[0], 2,
                               SSD_ALIGN_VCENTER|SSD_ALIGN_RIGHT, on_car_select);
      ssd_widget_add(group, button);
#endif
      //ssd_dialog_add_hspace(group, 10, 0);
      //ssd_dialog_add_hspace(group, 10, 0);
      ssd_widget_add (group,
                      ssd_text_new ("Car Text", roadmap_lang_get("Car"), SSD_MAIN_TEXT_SIZE,
                                    SSD_TEXT_NORMAL_FONT|SSD_TEXT_LABEL|SSD_ALIGN_VCENTER));
      group->callback = on_car_select;
      ssd_widget_add (box, group);
      ssd_widget_add (dialog, box);


#ifndef TOUCH_SCREEN
      ssd_widget_set_left_softkey_text       ( dialog, roadmap_lang_get("Ok"));
      ssd_widget_set_left_softkey_callback   ( dialog, on_ok_softkey);
#endif

      sProfileDlg = dialog;
      free(car_name);
   }

   if ( !SsdLoginShown) {
      const char *pVal;
      yesno[0] = "Yes";
      yesno[1] = "No";
      // Case insensitive comparison
      ssd_widget_set_value( sProfileDlg, "Username", roadmap_config_get( &RT_CFG_PRM_NAME_Var));
      ssd_widget_set_value( sProfileDlg, "Password", roadmap_config_get( &RT_CFG_PRM_PASSWORD_Var));
      ssd_widget_set_value( sProfileDlg, "Nickname", roadmap_config_get( &RT_CFG_PRM_NKNM_Var));
      if (Realtime_AllowPing()) pVal = yesno[0];
      else pVal = yesno[1];
      ssd_widget_set_data (sProfileDlg, "AllowPing", (void *) pVal);
   }

   SsdLoginShown = 1;

   sgLastLoginDlg = sProfileDlg;

   ssd_dialog_draw ();
   ssd_dialog_activate ( sDlgName, NULL);
}


static int on_create( SsdWidget this, const char *new_value )
{
   const char *username       		= ssd_dialog_get_value( "Username" );
   const char *confirm_password   	= ssd_dialog_get_value( "ConfirmPassword" );
   const char *password       		= ssd_dialog_get_value( "Password" );
   const char *email          		= ssd_dialog_get_value( "Email" );
   const char *send_updates   = ssd_dialog_get_data( "send_updates" );
   BOOL  bSendUpdates = !strcmp( send_updates, "yes" );


   if ( !validate_login_data( username, password, confirm_password, email ) )
   {
	   return 1;
   }

   roadmap_config_set( &RT_CFG_PRM_NKNM_Var, roadmap_login_dlg_get_nickname() );

   roadmap_login_on_create( username, password, email, bSendUpdates ,login_referrer_none);

   return 0;
}

static int on_update( SsdWidget this, const char *new_value )
{
   int res;
   const char *username       		= ssd_dialog_get_value("Username");
   const char *confirm_password   	= ssd_dialog_get_value("ConfirmPassword");
   const char *password       		= ssd_dialog_get_value("Password");
   const char *email          		= ssd_dialog_get_value("Email");
   const char *send_updates   		= ssd_dialog_get_data("send_updates");
   BOOL  bSendUpdates				= !strcmp( send_updates, "yes" );

   if ( !validate_login_data( username, password, confirm_password, email ) )
   {
	   return 1;
   }

   roadmap_config_set( &RT_CFG_PRM_NKNM_Var, roadmap_login_dlg_get_nickname() );

   res = roadmap_login_on_update( username, password, email, bSendUpdates,login_referrer_none );

   return res;
}

/*
 * Checks if all the user entries are ok. Returns TRUE in case of all valid fields, FALSE otherwise
 *
 */
static BOOL validate_login_data( const char *username, const char* password, const char* confirm_password, const char* email )
{
	if ( !roadmap_login_validate_username( username ) )
		return FALSE;

	if ( !roadmap_login_validate_password( password, confirm_password ) )
		return FALSE;

	if ( !roadmap_login_validate_email( email ) )
		return FALSE;

	return TRUE;
}

/***********************************************************
 *  Name        : Callback for the next button
 *  Purpose     :
 *  Params      : [in]
 *              : [out]
 *  Notes       :
 */
static BOOL next_btn_callback( int         exit_code,
							   const char* value,
							   void*       context )
{
	SsdWidget w = (SsdWidget) context;
	SsdWidget next_entry = NULL;
	BOOL retVal = TRUE;
	if ( !strcmp( "Username", w->name ) )
	{
		next_entry = ssd_widget_get( sgSignUpDlg, "Password" );
	}
	if ( !strcmp( "Password", w->name ) )
	{
		next_entry = ssd_widget_get( sgSignUpDlg, "ConfirmPassword" );
	}
	if ( !strcmp( "ConfirmPassword", w->name ) )
	{
		next_entry = ssd_widget_get( sgSignUpDlg, "Email" );
	}
	if ( !strcmp( "Email", w->name ) )
	{
		next_entry = ssd_widget_get( sgSignUpDlg, "Nickname" );
	}

	if ( next_entry )
	{
		SsdWidget text_box = ssd_widget_get( next_entry, "text_box" );
		text_box->callback( next_entry->children, "" );
		retVal = FALSE;
	}

	return retVal;
}

/***********************************************************
 *  Name        : Device event handler
 *  Purpose     :
 *  Params      : [in]
 *              : [out]
 *  Notes       :
 */
static void OnDeviceEvent( device_event event, void* context )
{
   if( device_event_window_orientation_changed == event )
   {
	   /* Add the portrait/landscape orientation handling */
   }
}


/***********************************************************
 *  Name        : roadmap_login_dlg_get_username()
 *  Purpose     : Returns Username taken from the last !!!login dialog!!!
 *  Params      : [in]
 *              : [out]
 *  Notes       :
 */
const char* roadmap_login_dlg_get_username()
{
	return ssd_widget_get_value( sgLastLoginDlg, "Username" );
}

/***********************************************************
 *  Name        : roadmap_login_dlg_get_allowPing()
 *  Purpose     : Returns Username taken from the last !!!login dialog!!!
 *  Params      : [in]
 *              : [out]
 *  Notes       :
 */
const char* roadmap_login_dlg_get_allowPing()
{
   return ssd_widget_get_data( sgLastLoginDlg, "AllowPing" );
}
/***********************************************************
 *  Name        : roadmap_login_dlg_get_password()
 *  Purpose     : Returns Username taken from the last !!!login dialog!!!
 *  Params      : [in]
 *              : [out]
 *  Notes       :
 */
const char* roadmap_login_dlg_get_password()
{
	return ssd_widget_get_value( sgLastLoginDlg, "Password" );
}

/***********************************************************
 *  Name        : roadmap_login_dlg_get_nickname()
 *  Purpose     : Returns nickname taken from the last !!!login dialog!!!
 *  Params      : [in]
 *              : [out]
 *  Notes       :
 */
const char* roadmap_login_dlg_get_nickname()
{
	return ssd_widget_get_value( sgLastLoginDlg, "Nickname" );
}


void roadmap_login_ssd_on_signup_skip( messagebox_closed cb )
{
	ssd_dialog_hide_all( dec_cancel );
	roadmap_messagebox_cb("","You can personalize your account from Settings->profile", cb );
	sgNewExistingInProcess = FALSE;
}




int on_signup_skip( SsdWidget this, const char *new_value)
{
	roadmap_login_on_signup_skip();
	return 1;
}

/*
 * Indicates if new existing process is active
 */
BOOL roadmap_login_ssd_new_existing_in_process()
{
	return sgNewExistingInProcess;
}


/***********************************************************
 *  Name        :
 *  Purpose     :
 *  Params      : [in]
 *              : [out]
 *  Notes       :
 */
