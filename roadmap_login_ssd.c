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

//======== Local Types ========


//======== Defines ========
#define RM_SIGNUP_DLG_NAME			"Create Account Dialog"
#define RM_SIGNUP_DLG_BTN   "SignUp"
#define RM_NEW_EXISTING_NAME  "New Existing"
#define RM_SIGNUP_DLG_TITLE		"Create your account"
#define RM_SIGNUP_ENTRY_TITLE	RM_SIGNUP_DLG_TITLE


#define RM_SIGNUP_DLG_LABEL_HOFFSET		5			// Horizontal offset for the label
#define RM_SIGNUP_DLG_SIGNIN_FONT		18			// Sign in text font size
#define RM_SIGNUP_DLG_LABEL_FONT		16			// Sign in text font size

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
    SsdWidget button;
    SsdWidget asteric;
    SsdWidget entry;
    SsdWidget label_cnt;
    // int box_width =  ( roadmap_canvas_width()*95 )/100;
    int tab_flag = SSD_WS_TABSTOP;
    int entry_width = RM_SIGNUP_ENTRY_WIDTH;

    if ( roadmap_screen_is_hd_screen() )
    {
    	entry_width = roadmap_screen_adjust_width( entry_width );
    }

    sgSignUpDlg = ssd_dialog_new( RM_SIGNUP_DLG_NAME, roadmap_lang_get( RM_SIGNUP_DLG_TITLE ), on_signup_dlg_close, SSD_CONTAINER_TITLE );

    box = ssd_container_new ( "Sign Up Box", NULL, SSD_MAX_SIZE,
             SSD_MIN_SIZE, SSD_END_ROW | SSD_ALIGN_CENTER | SSD_CONTAINER_BORDER | SSD_WIDGET_SPACE |SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE );
    ssd_widget_set_offset( box, 0, -1 );

    // Space before next entry
    ssd_dialog_add_vspace( box, 3, 0 );

    // Username
    group = ssd_container_new ( "Username group", NULL, SSD_MAX_SIZE,
             SSD_MIN_SIZE, SSD_WIDGET_SPACE | SSD_END_ROW | tab_flag );
    ssd_dialog_add_hspace( group, RM_SIGNUP_DLG_LABEL_HOFFSET, 0 );
    ssd_widget_set_color ( group, "#000000", "#ffffff" );

    label_cnt = ssd_container_new ( "Label container", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_ALIGN_VCENTER );
    ssd_widget_set_color ( label_cnt, NULL,NULL );
    ssd_widget_add( label_cnt, ssd_text_new( "Label", roadmap_lang_get ("Username"),
         RM_SIGNUP_DLG_LABEL_FONT, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER ) );
    asteric = ssd_text_new( "Asteric", "*", SSD_MIN_SIZE, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER );
    ssd_text_set_color( asteric, "#ff0000");
    ssd_widget_add ( label_cnt, asteric );
    ssd_widget_add ( group, label_cnt);
    entry = ssd_entry_new ( "Username", "", SSD_ALIGN_VCENTER|SSD_ALIGN_RIGHT, 0, entry_width, SSD_MIN_SIZE, roadmap_lang_get ("Desired Username") );
    ssd_entry_set_kb_params( entry, RM_SIGNUP_ENTRY_TITLE, roadmap_lang_get( "User name" ), NULL, next_btn_callback, SSD_KB_DLG_SHOW_NEXT_BTN|SSD_KB_DLG_INPUT_ENGLISH );
    ssd_widget_add ( group, entry);
    ssd_dialog_add_hspace( group, RM_SIGNUP_DLG_LABEL_HOFFSET, SSD_ALIGN_RIGHT );
    ssd_widget_add (box, group);

    // Password
    group = ssd_container_new ( "PW group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
             SSD_WIDGET_SPACE | SSD_END_ROW | tab_flag );
    ssd_dialog_add_hspace( group, RM_SIGNUP_DLG_LABEL_HOFFSET, 0 );
    ssd_widget_set_color ( group, "#000000", "#ffffff" );

    label_cnt = ssd_container_new ( "Label container", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_ALIGN_VCENTER );
    ssd_widget_set_color ( label_cnt, NULL,NULL );
    ssd_widget_add ( label_cnt, ssd_text_new ("Label", roadmap_lang_get ( "Password"),
         RM_SIGNUP_DLG_LABEL_FONT, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER ) );
    asteric = ssd_text_new( "Asteric", "*", SSD_MIN_SIZE, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER );
    ssd_text_set_color( asteric, "#ff0000");
    ssd_widget_add ( label_cnt, asteric );
    ssd_widget_add ( group, label_cnt );
    entry = ssd_entry_new ("Password", "", SSD_ALIGN_VCENTER|SSD_ALIGN_RIGHT, SSD_TEXT_PASSWORD, entry_width,
         SSD_MIN_SIZE, roadmap_lang_get ("Desired password") );
    ssd_entry_set_kb_params( entry, RM_SIGNUP_ENTRY_TITLE, roadmap_lang_get( "Password" ), NULL, next_btn_callback, SSD_KB_DLG_SHOW_NEXT_BTN|SSD_KB_DLG_LAST_KB_STATE );
    ssd_widget_add (group, entry );
    ssd_dialog_add_hspace( group, RM_SIGNUP_DLG_LABEL_HOFFSET, SSD_ALIGN_RIGHT );
    ssd_widget_add (box, group);

    // Confirm PW
    group = ssd_container_new ("Repeat PW group", NULL, SSD_MAX_SIZE,
             SSD_MIN_SIZE, SSD_WIDGET_SPACE | SSD_END_ROW | tab_flag );
    ssd_dialog_add_hspace( group, RM_SIGNUP_DLG_LABEL_HOFFSET, 0 );
    ssd_widget_set_color (group, "#000000", "#ffffff");

    label_cnt = ssd_container_new ( "Label container", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_ALIGN_VCENTER );
    ssd_widget_set_color ( label_cnt, NULL,NULL );
    ssd_widget_add ( label_cnt, ssd_text_new ("Label", roadmap_lang_get ( "Confirm" ), RM_SIGNUP_DLG_LABEL_FONT, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER));
    asteric = ssd_text_new( "Asteric", "*", SSD_MIN_SIZE, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER );
    ssd_text_set_color( asteric, "#ff0000");
    ssd_widget_add ( label_cnt, asteric );
    ssd_widget_add ( group, label_cnt );
    entry = ssd_entry_new ("ConfirmPassword", "", SSD_ALIGN_VCENTER | SSD_ALIGN_RIGHT, SSD_TEXT_PASSWORD, entry_width,
            SSD_MIN_SIZE, roadmap_lang_get ("Confirm password") );
    ssd_entry_set_kb_params( entry, RM_SIGNUP_ENTRY_TITLE, roadmap_lang_get( "Confirm password" ), NULL, next_btn_callback, SSD_KB_DLG_SHOW_NEXT_BTN|SSD_KB_DLG_LAST_KB_STATE );
    ssd_widget_add (group, entry );
    ssd_dialog_add_hspace( group, RM_SIGNUP_DLG_LABEL_HOFFSET, SSD_ALIGN_RIGHT );
    ssd_widget_add (box, group);

    // Email
    group = ssd_container_new ("email group", NULL, SSD_MAX_SIZE,
             SSD_MIN_SIZE, SSD_WIDGET_SPACE | SSD_END_ROW | tab_flag);
    ssd_widget_set_color (group, "#000000", "#ffffff");
    ssd_dialog_add_hspace( group, RM_SIGNUP_DLG_LABEL_HOFFSET, 0 );

    label_cnt = ssd_container_new ( "Label container", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_ALIGN_VCENTER );
    ssd_widget_set_color ( label_cnt, NULL,NULL );
    ssd_widget_add ( label_cnt, ssd_text_new ("Label", roadmap_lang_get( "Email" ),
                  RM_SIGNUP_DLG_LABEL_FONT, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER) );
    asteric = ssd_text_new( "Asteric", "*", SSD_MIN_SIZE, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER );
    ssd_text_set_color( asteric, "#ff0000");
    ssd_widget_add ( label_cnt, asteric );
    ssd_widget_add ( group, label_cnt );

    entry = ssd_entry_new( "Email", "", SSD_ALIGN_VCENTER|SSD_ALIGN_RIGHT, 0,
         entry_width, SSD_MIN_SIZE, roadmap_lang_get ( "Email" ) );
    ssd_entry_set_kb_params( entry, RM_SIGNUP_ENTRY_TITLE, roadmap_lang_get( "Email" ), NULL, next_btn_callback, SSD_KB_DLG_SHOW_NEXT_BTN|SSD_KB_DLG_LAST_KB_STATE );
    ssd_widget_add (group, entry );
    ssd_dialog_add_hspace( group, RM_SIGNUP_DLG_LABEL_HOFFSET, SSD_ALIGN_RIGHT );
    ssd_widget_add (box, group);

    // Nickname
    group = ssd_container_new ("Nickname group", NULL, SSD_MAX_SIZE,
             SSD_MIN_SIZE, SSD_WIDGET_SPACE | SSD_END_ROW | tab_flag);
    ssd_widget_set_color ( group, "#000000", "#ffffff" );
    ssd_dialog_add_hspace( group, RM_SIGNUP_DLG_LABEL_HOFFSET, 0 );

    label_cnt = ssd_container_new ( "Label container", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_ALIGN_VCENTER );
    ssd_widget_set_color ( label_cnt, NULL,NULL );
    ssd_widget_add ( label_cnt, ssd_text_new ("Label", roadmap_lang_get ( "Nickname"), RM_SIGNUP_DLG_LABEL_FONT, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER ) );
    ssd_widget_add ( group, label_cnt );
    entry = ssd_entry_new ("Nickname", "", SSD_ALIGN_VCENTER |SSD_ALIGN_RIGHT, 0,
         entry_width, SSD_MIN_SIZE, roadmap_lang_get( "Nickname" ) );
    ssd_entry_set_kb_params( entry, RM_SIGNUP_ENTRY_TITLE, roadmap_lang_get( "Nickname" ), NULL, next_btn_callback, SSD_KB_DLG_LAST_KB_STATE );
    ssd_widget_add ( group, entry );

    ssd_dialog_add_hspace( group, RM_SIGNUP_DLG_LABEL_HOFFSET, SSD_ALIGN_RIGHT );
    ssd_widget_add (box, group);

    // Agree to receive
    group = ssd_container_new ("Send Update group", NULL, SSD_MAX_SIZE,
              SSD_MIN_SIZE, SSD_WIDGET_SPACE | SSD_END_ROW | tab_flag);
    ssd_widget_set_color ( group, "#000000", "#ffffff" );
    ssd_dialog_add_hspace( group, RM_SIGNUP_DLG_LABEL_HOFFSET, 0 );
    ssd_widget_add (group,
          ssd_checkbox_new ("send_updates", TRUE,  0, NULL,NULL,NULL,CHECKBOX_STYLE_DEFAULT));
    label_cnt = ssd_container_new ( "Label container", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_ALIGN_VCENTER );
    ssd_widget_set_color ( label_cnt, NULL,NULL );
    ssd_dialog_add_hspace( label_cnt, RM_SIGNUP_DLG_LABEL_HOFFSET, 0 );
    ssd_widget_add ( label_cnt, ssd_text_new ("Label", roadmap_lang_get ( "Send me updates"), RM_SIGNUP_DLG_LABEL_FONT, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER ) );
    ssd_widget_add ( group, label_cnt );
    ssd_widget_add (box, group);


    // Space under button
    ssd_dialog_add_vspace( box, 10, 0 );

#ifdef TOUCH_SCREEN
    // Create button
    button = ssd_button_label ( "SignUp", roadmap_lang_get ("Next"),
             SSD_ALIGN_RIGHT | SSD_START_NEW_ROW | SSD_WS_TABSTOP, on_update );
    ssd_widget_add ( box, button );
    // Skip button
    button = ssd_button_label ( "Skip", roadmap_lang_get ("Skip"),
             SSD_ALIGN_CENTER | SSD_WS_TABSTOP, on_signup_skip );
    ssd_widget_add ( box, button );

    ssd_dialog_add_vspace( box, 30, 0 );
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
	   roadmap_login_update_dlg_show();
	   return 1;
   }
   else if ( ( widget != NULL ) && !strcmp(widget->name, "Existing" ) )
   {
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

void roadmap_login_new_existing_dlg(){

   static SsdWidget sNewExistingDlg = NULL;
   SsdWidget dialog;
   SsdWidget group;
   SsdWidget text, group2;
   SsdWidget button;
   int bubble1_flags, bubble2_flags;

   /*
    * If currently in process - don't show the dilog
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

   sgNewExistingInProcess = TRUE;
}

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
   static const char* sDlgName = "Login";
   int width;
#ifdef TOUCH_SCREEN
   int tab_flag = SSD_WS_TABSTOP;
#else
   int tab_flag = SSD_WS_TABSTOP;
#endif
  	/*
  	 * Currently shown login function for the
  	 * further processing in callbacks
  	 */

   roadmap_login_set_show_function( roadmap_login_details_dialog_show_un_pw );

   width = roadmap_canvas_width () / 2;

   if ( !ssd_dialog_exists( sDlgName ) ) {

      SsdWidget dialog;
      SsdWidget group;
      SsdWidget box;
      SsdWidget button;
      SsdWidget username, pwd;
      SsdWidget space;

      // Define the labels and values
      yesno_label[0] = roadmap_lang_get ("Yes");
      yesno_label[1] = roadmap_lang_get ("No");
      yesno[0] = "Yes";
      yesno[1] = "No";

      dialog = ssd_dialog_new ( sDlgName, roadmap_lang_get ( sDlgName ),
               NULL, SSD_CONTAINER_TITLE );

#ifdef TOUCH_SCREEN
      space = ssd_container_new ("spacer", NULL, SSD_MAX_SIZE, 10,
               SSD_WIDGET_SPACE | SSD_END_ROW);
      ssd_widget_set_color (space, NULL, NULL);
      ssd_widget_add (dialog, space);
#endif

      box = ssd_container_new ("box group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
               SSD_WIDGET_SPACE | SSD_END_ROW | SSD_ROUNDED_CORNERS
                        | SSD_ROUNDED_WHITE | SSD_POINTER_NONE
                        | SSD_CONTAINER_BORDER);

      group = ssd_container_new ("Name group", NULL, SSD_MAX_SIZE,
               SSD_MIN_SIZE, SSD_WIDGET_SPACE | SSD_END_ROW | tab_flag |SSD_WS_DEFWIDGET);

      ssd_widget_set_color (group, "#000000", "#ffffff");
      ssd_widget_add (group, ssd_text_new ("Label", roadmap_lang_get (
               "Username"), -1, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER));
      username = ssd_entry_new ("Username", "", SSD_ALIGN_VCENTER|SSD_ALIGN_RIGHT, 0, width, SSD_MIN_SIZE, roadmap_lang_get("Username"));
      ssd_entry_set_kb_params( username, roadmap_lang_get( "User name" ), NULL, NULL, NULL, SSD_KB_DLG_INPUT_ENGLISH );
      ssd_widget_add (group, username);

      ssd_widget_add (box, group);

      group = ssd_container_new ("PW group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
               SSD_WIDGET_SPACE | SSD_END_ROW | tab_flag);
      ssd_widget_set_color (group, "#000000", "#ffffff");

      ssd_widget_add (group, ssd_text_new ("Label", roadmap_lang_get (
               "Password"), -1, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER));
      pwd = ssd_entry_new ("Password", "", SSD_ALIGN_VCENTER| SSD_ALIGN_RIGHT,
					  SSD_TEXT_PASSWORD, width, SSD_MIN_SIZE, roadmap_lang_get ("Password") );
      ssd_entry_set_kb_params( pwd, roadmap_lang_get( "Password" ), NULL, NULL, NULL, SSD_KB_DLG_INPUT_ENGLISH );
      ssd_widget_add ( group, pwd );
      ssd_widget_add ( box, group );

      ssd_widget_add (dialog, box);

      space = ssd_container_new ("spacer", NULL, SSD_MAX_SIZE, 10,
               SSD_WIDGET_SPACE | SSD_END_ROW);
      ssd_widget_set_color (space, NULL, NULL);
      ssd_widget_add (dialog, space);

      button = ssd_button_label ("LogIn", roadmap_lang_get ("Log In"),
               SSD_ALIGN_CENTER | SSD_START_NEW_ROW
                        | SSD_WS_TABSTOP, roadmap_login_on_login );
      ssd_widget_add (dialog, button);

      space = ssd_container_new ("spacer", NULL, SSD_MAX_SIZE, 20,
               SSD_WIDGET_SPACE | SSD_END_ROW);
      ssd_widget_set_color (space, NULL, NULL);
      ssd_widget_add (dialog, space);

      /* Makes it possible to click in the vicinity of the button  */
      ssd_widget_set_click_offsets_ext( dialog, 0, 0, 0, 20 );

#ifndef TOUCH_SCREEN
      ssd_widget_set_right_softkey_text ( dialog, roadmap_lang_get("Log In"));
      ssd_widget_set_right_softkey_callback ( dialog, on_softkey_login);

      ssd_widget_set_left_softkey_text ( dialog, roadmap_lang_get("Back"));
      ssd_widget_set_left_softkey_callback ( dialog, on_softkey_back);
#endif

      sUnPwDlg = dialog;
   }

   if (  /* !SsdLoginShown && */ !Realtime_is_random_user() )
   {
      ssd_dialog_set_value ( "Username", roadmap_config_get (
               &RT_CFG_PRM_NAME_Var ) );
      ssd_dialog_set_value ("Password", roadmap_config_get (
               &RT_CFG_PRM_PASSWORD_Var ) );
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
#ifdef TOUCH_SCREEN
   int tab_flag = SSD_WS_TABSTOP;
#else
   int tab_flag = SSD_WS_TABSTOP;
#endif
   SsdWidget group = ssd_container_new (group_name, NULL,
                                        SSD_MAX_SIZE,SSD_MIN_SIZE,
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
                   ssd_text_new (text, text, -1, SSD_ALIGN_VCENTER));

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
   int width;

#ifdef TOUCH_SCREEN
   int tab_flag = SSD_WS_TABSTOP;
#else
   int tab_flag = SSD_WS_TABSTOP;
#endif

   /* Current shown login function for the
    * further processing in callbacks
    */
   roadmap_login_set_show_function( (RoadmapLoginDlgShowFn) roadmap_login_profile_dialog_show );

   width = roadmap_canvas_width()/2;

   if ( !ssd_dialog_exists( sDlgName ) ) {

      SsdWidget dialog;
      SsdWidget group, group2;
      SsdWidget box;
      SsdWidget button;
      SsdWidget username, pwd;
      SsdWidget space;
      int height = 45;
      const char *edit_button[] = {"edit_right", "edit_left"};
      const char *buttons[2];

      if ( roadmap_screen_is_hd_screen() )
      {
         height = 65;
      }

      dialog = ssd_dialog_new ( sDlgName, roadmap_lang_get(sDlgName), on_close_dialog,
                               SSD_CONTAINER_TITLE);

#ifdef TOUCH_SCREEN
      space = ssd_container_new ("spacer", NULL, SSD_MAX_SIZE, 10, SSD_WIDGET_SPACE|SSD_END_ROW);
      ssd_widget_set_color (space, NULL,NULL);
      ssd_widget_add(dialog, space);
#endif

      box = ssd_container_new ("box group", NULL,
                               SSD_MAX_SIZE,SSD_MIN_SIZE,
                               SSD_WIDGET_SPACE|SSD_END_ROW|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE|SSD_POINTER_NONE|SSD_CONTAINER_BORDER);

      group = ssd_container_new ("Name group", NULL,
                                 SSD_MAX_SIZE,SSD_MIN_SIZE,
                                 SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag);

      ssd_widget_set_color (group, "#000000", "#ffffff");
      ssd_widget_add (group,
                      ssd_text_new ("Label", roadmap_lang_get("Username"), -1,
                                    SSD_TEXT_LABEL|SSD_ALIGN_VCENTER));
      username = ssd_confirmed_entry_new ("Username", "", SSD_ALIGN_VCENTER|SSD_ALIGN_RIGHT, 0,
                                          width, SSD_MIN_SIZE,
                                          roadmap_lang_get("Change of username and password should be done on www.waze.com, and only then entered here. Continue?"),
                                          roadmap_lang_get("Username"));
      ssd_entry_set_kb_params( username, roadmap_lang_get( "User name" ), NULL, NULL, NULL, SSD_KB_DLG_INPUT_ENGLISH );

      ssd_widget_add (group, username);

      ssd_widget_add (box, group);

      group = ssd_container_new ("PW group", NULL,
                                 SSD_MAX_SIZE,SSD_MIN_SIZE,
                                 SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag);
      ssd_widget_set_color (group, "#000000", "#ffffff");

      ssd_widget_add (group,
                      ssd_text_new ("Label", roadmap_lang_get("Password"), -1,
                                    SSD_TEXT_LABEL|SSD_ALIGN_VCENTER));
      pwd = ssd_entry_new ("Password", "", SSD_ALIGN_VCENTER|SSD_ALIGN_RIGHT, SSD_TEXT_PASSWORD,
                           width, SSD_MIN_SIZE,roadmap_lang_get("Password") );
      ssd_entry_set_kb_params( pwd, roadmap_lang_get( "Password" ), NULL, NULL, NULL, SSD_KB_DLG_INPUT_ENGLISH );
      ssd_widget_add (group, pwd );
      ssd_widget_add (box, group);

      group = ssd_container_new ("Nick group", NULL,
                                 SSD_MAX_SIZE, SSD_MIN_SIZE,
                                 SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag);
      ssd_widget_set_color (group, "#000000", "#ffffff");

      ssd_widget_add (group,
                      ssd_text_new ("Label", roadmap_lang_get("Nickname"), -1,
                                    SSD_TEXT_LABEL|SSD_ALIGN_VCENTER));

      ssd_widget_add (group,
                      ssd_entry_new ("Nickname", "", SSD_ALIGN_VCENTER|SSD_ALIGN_RIGHT,
                                     0, width, SSD_MIN_SIZE ,roadmap_lang_get("Nickname")));
      ssd_widget_add (box, group);


      ssd_widget_add (dialog, box);
      group = ssd_container_new ("Car group", NULL,
                                 SSD_MAX_SIZE,SSD_MIN_SIZE,
                                 SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag|SSD_ROUNDED_CORNERS|
                                 SSD_ROUNDED_WHITE|SSD_POINTER_NONE|SSD_CONTAINER_BORDER);
      ssd_widget_set_color (group, "#000000", "#ffffff");

      config_car = roadmap_config_get (&RoadMapConfigCarName);
      car_name = roadmap_path_join("cars", config_car);
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
      icon[0] = car_name;
      icon[1] = NULL;
      ssd_dialog_add_hspace(group, 10, 0);
      ssd_widget_add (group,
                      ssd_button_new (roadmap_lang_get ("Car"), roadmap_lang_get(config_car), (const char **)&icon[0], 1,
                                      0,
                                      on_car_select));
      ssd_dialog_add_hspace(group, 10, 0);
      ssd_widget_add (group,
                      ssd_text_new ("Car Text", roadmap_lang_get("Car"), -1,
                                    SSD_TEXT_LABEL|SSD_ALIGN_VCENTER));
      group->callback = on_car_select;
      ssd_widget_add (dialog, group);

      //Privacy
      group = ssd_container_new ("Privacy group", NULL,
                                 SSD_MAX_SIZE,SSD_MIN_SIZE,
                                 SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag|SSD_ROUNDED_CORNERS|
                                 SSD_ROUNDED_WHITE|SSD_POINTER_NONE|SSD_CONTAINER_BORDER);
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
                      ssd_button_new (roadmap_lang_get ("Car"), roadmap_lang_get(config_car), (const char **)&icon[0], 1,
                                      0,
                                      NULL));

      ssd_widget_add (group,
                      ssd_text_new ("Privacy Text", roadmap_lang_get("Privacy settings"), -1,
                                    SSD_TEXT_LABEL|SSD_ALIGN_VCENTER));
      group->callback = RealtimePrivacySettingsWidgetCallBack;


      ssd_widget_add (dialog, group);

      group = ssd_container_new("AllowPing group",NULL,SSD_MAX_SIZE, SSD_MIN_SIZE, SSD_WIDGET_SPACE
                                |SSD_END_ROW|tab_flag|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE|SSD_POINTER_NONE|SSD_CONTAINER_BORDER);
      ssd_widget_set_color (group, NULL, NULL);
      group2 = ssd_container_new ("group2", NULL, 2*roadmap_canvas_width()/3, height,
                                  SSD_ALIGN_VCENTER);
      ssd_widget_set_color (group2, NULL, NULL);
      ssd_widget_add(group2,
                     ssd_text_new("Allow Pinvg TXT",
                                  roadmap_lang_get("I agree to be pinged"),
                                  -1, SSD_TEXT_LABEL|SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE));
      ssd_widget_add(group,group2);
      ssd_widget_add (group,
                      ssd_checkbox_new ("AllowPing", TRUE,  SSD_ALIGN_RIGHT|SSD_ALIGN_VCENTER, NULL,NULL,NULL,CHECKBOX_STYLE_ON_OFF));

      ssd_widget_add(dialog,group);

#ifndef TOUCH_SCREEN
      group = ssd_container_new ("Car group", NULL,
                                 SSD_MAX_SIZE,SSD_MIN_SIZE,
                                 SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag|SSD_ROUNDED_CORNERS
                                 |SSD_ROUNDED_WHITE|SSD_POINTER_NONE|SSD_CONTAINER_BORDER);
      ssd_widget_set_color (group, "#000000", "#ffffff");
      icon[0] = (char *)roadmap_mood_get();
      icon[1] = NULL;
      ssd_widget_add (group,
                      ssd_text_new ("Mood Text", roadmap_lang_get("Mood"), -1,
                                    SSD_TEXT_LABEL|SSD_ALIGN_VCENTER));
      ssd_widget_add (group,
                      ssd_button_new (roadmap_lang_get ("Mood"), roadmap_lang_get(roadmap_mood_get()), (const char **)&icon[0], 1,
                                      SSD_ALIGN_CENTER,
                                      on_mood_select));
      group->callback = on_mood_select;
      ssd_widget_add (dialog, group);

#endif

      //Social networks
      box = ssd_container_new ("social group", NULL,
                               SSD_MAX_SIZE,SSD_MIN_SIZE,
                               SSD_WIDGET_SPACE|SSD_END_ROW|SSD_ROUNDED_CORNERS
                               |SSD_ROUNDED_WHITE|SSD_POINTER_NONE|SSD_CONTAINER_BORDER);

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

   roadmap_login_on_create( username, password, email, bSendUpdates );

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

   res = roadmap_login_on_update( username, password, email, bSendUpdates );

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


void roadmap_login_ssd_on_signup_skip( void )
{
	ssd_dialog_hide_all( dec_cancel );
	roadmap_messagebox("","You can personalize your account from Settings->profile");
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
