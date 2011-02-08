   /* roadmap_welcome_wizard.c - First time settup wizard
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi B.S
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
 *
 */

#include <stdlib.h>
#include <string.h>

#include "roadmap.h"
#include "roadmap_main.h"
#include "roadmap_bar.h"
#include "roadmap_config.h"
#include "roadmap_lang.h"
#include "roadmap_start.h"
#include "roadmap_login.h"
#include "roadmap_welcome_wizard.h"
#include "roadmap_social.h"
#include "roadmap_history.h"
#include "roadmap_screen.h"
#include "ssd/ssd_dialog.h"
#include "roadmap_messagebox.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_bitmap.h"
#include "ssd/ssd_choice.h"
#include "ssd/ssd_button.h"
#include "ssd/ssd_bitmap.h"
#include "roadmap_mood.h"
#include "ssd/ssd_list.h"
#include "ssd/ssd_entry.h"
#include "roadmap_analytics.h"
#include "ssd/ssd_entry_label.h"
#include "ssd/ssd_checkbox.h"
#include "ssd/ssd_contextmenu.h"
#include "Realtime/Realtime.h"
#include "Realtime/RealtimeDefs.h"
#include "Realtime/RealtimeAlerts.h"
#include "roadmap_native_keyboard.h"
#include "roadmap_disclaimer.h"
#include "roadmap_browser.h"
#ifdef   TOUCH_SCREEN
   #include "ssd/ssd_keyboard.h"
#endif //TOUCH_SCREEN
#ifdef ANDROID
   #include "roadmap_androidmsgbox.h"
#endif


/* Twitter dialog definitions */
#define WELCOME_WIZ_DLG_TWITTER_NAME           "Welcome Twitter"       // Twitter dialog name
#define WELCOME_WIZ_DLG_TWITTER_TITLE          "Welcome"               // Twitter dialog title
#define WELCOME_WIZ_DLG_TWITTER_SET_SIGNUP     "Twitter set signup"    // Twitter dialog title
/* Ping dialog definitions */
#define WELCOME_WIZ_DLG_PING_NAME           "Welcome Ping"             // Ping dialog name
#define WELCOME_WIZ_DLG_PING_TITLE          "Welcome"                  // Ping dialog title
#define WELCOME_WIZ_DLG_PING_AGREE          "Ping Agree Checkbox"      // Ping agree checkbox name
#define WELCOME_WIZ_DLG_PING_HOFFSET         13               // Space between the label and entry
#define WELCOME_WIZ_DLG_PING_TEXT_WIDTH      215              // Space between the label and entry

#define WELCOME_WIZ_DLG_LBL_FONT  14               // Twitter dialog title
#define WELCOME_WIZ_DLG_LBL_COLOR "#555555"
#define WELCOME_WIZ_DLG_LBL_CNT_WIDTH  160         // Base width of the label container
#define WELCOME_WIZ_DLG_HOFFSET    5               // Space between the label and entry

#define WELCOME_WIZ_DLG_ENTRY_HEIGHT     30        // Entry height




//disclaimer texts ( see disclaimer.h )
static char * DISCLAIMER_ENG[NUMBER_OF_DISCLAIMER_PARTS][NUMBER_OF_LINES_PER_PART_DISC] =
				{{ DIS_ENG_0_0,DIS_ENG_0_1,DIS_ENG_0_2,"","",""},{DIS_ENG_1_0,DIS_ENG_1_1,DIS_ENG_1_2,DIS_ENG_1_3,
				   DIS_ENG_1_4,DIS_ENG_1_5,DIS_ENG_1_6}};

static char * DISCLAIMER_ESP[NUMBER_OF_DISCLAIMER_PARTS][NUMBER_OF_LINES_PER_PART_DISC] =
				{{ DIS_ESP_0_0,DIS_ESP_0_0,"","","",""},{DIS_ESP_1_0,DIS_ESP_1_1,DIS_ESP_1_2,DIS_ESP_1_3,
				   DIS_ESP_1_4,DIS_ESP_1_5,DIS_ESP_1_6}};

static char * DISCLAIMER_HEB[NUMBER_OF_DISCLAIMER_PARTS][NUMBER_OF_LINES_PER_PART_DISC] =
				{{ DIS_HEB_0_0,DIS_HEB_0_1,"","","",""},{DIS_HEB_1_0,DIS_HEB_1_1,DIS_HEB_1_2,DIS_HEB_1_3,
				   DIS_HEB_1_4,DIS_HEB_1_5,DIS_HEB_1_6}};


//intro texts
static char * INTRO_ENG[NUMBER_OF_LINES_INTRO] = {INTR_ENG_0,INTR_ENG_1,INTR_ENG_2,INTR_ENG_3};
static char * INTRO_ESP[NUMBER_OF_LINES_INTRO] = {INTR_ESP_0,INTR_ESP_1,INTR_ESP_2,INTR_ESP_3};


#if defined(RIMAPI) || defined(ANDROID)
static void terms_of_use_native();
#endif
static void add_intro_texts(SsdWidget group, int tab_flag);

extern RoadMapConfigDescriptor RT_CFG_PRM_NKNM_Var;
////   First time
static RoadMapConfigDescriptor WELCOME_WIZ_FIRST_TIME_Var =
                           ROADMAP_CONFIG_ITEM(
                                    WELCOME_WIZ_TAB,
                                    WELCOME_WIZ_FIRST_TIME_Name);

static RoadMapConfigDescriptor WELCOME_WIZ_ENABLED_Var =
                           ROADMAP_CONFIG_ITEM(
                                    WELCOME_WIZ_TAB,
                                    WELOCME_WIZ_ENABLED_Name);

static RoadMapConfigDescriptor WELCOME_WIZ_TERMS_OF_USE_Var =
                           ROADMAP_CONFIG_ITEM(
                                    WELCOME_WIZ_TAB,
                                    WELCOME_WIZ_TERMS_OF_USE_Name);

static RoadMapConfigDescriptor WELCOME_WIZ_MINUTES_FOR_ACTIVATION_Var =
                           ROADMAP_CONFIG_ITEM(
                                    WELCOME_WIZ_TAB,
                                    WELCOME_WIZ_MINUTES_FOR_ACTIVATION_Name);

static RoadMapConfigDescriptor WELCOME_WIZ_SHOW_INTRO_SCREEN_Var =
                           ROADMAP_CONFIG_ITEM(
                                    WELCOME_WIZ_TAB,
                                    WELCOME_WIZ_SHOW_INTRO_SCREEN_Name);

static RoadMapConfigDescriptor WELOCME_WIZ_GUIDED_TOUR_URL_Var =
                           ROADMAP_CONFIG_ITEM(
                                    WELCOME_WIZ_TAB,
                                    WELOCME_WIZ_GUIDED_TOUR_URL_Name);


static RoadMapCallback gCallback;


/////////////////////////////////////////////////////////////////////
static BOOL is_first_time(void){
   roadmap_config_declare_enumeration( WELCOME_WIZ_CONFIG_TYPE,
                                       &WELCOME_WIZ_FIRST_TIME_Var,
                                       NULL,
                                       WELCOME_WIZ_FIRST_TIME_Yes,
                                       WELCOME_WIZ_FIRST_TIME_No,
                                       NULL);
   if( 0 == strcmp( roadmap_config_get( &WELCOME_WIZ_FIRST_TIME_Var), WELCOME_WIZ_FIRST_TIME_Yes))
      return TRUE;
   return FALSE;
}

static BOOL is_terms_accepted(void){
   roadmap_config_declare_enumeration( WELCOME_WIZ_CONFIG_TYPE,
                                       &WELCOME_WIZ_TERMS_OF_USE_Var,
                                       NULL,
                                       WELCOME_WIZ_FIRST_TIME_No,
                                       WELCOME_WIZ_FIRST_TIME_Yes,
                                       NULL);
   if( 0 == strcmp( roadmap_config_get( &WELCOME_WIZ_TERMS_OF_USE_Var), WELCOME_WIZ_FIRST_TIME_Yes))
      return TRUE;
   return FALSE;
}

static BOOL is_show_intro_screen(void){
   roadmap_config_declare_enumeration( WELCOME_WIZ_SHOW_INTRO_CONFIG_TYPE,
                                       &WELCOME_WIZ_SHOW_INTRO_SCREEN_Var,
                                       NULL,
                                       WELCOME_WIZ_SHOW_INTRO_SCREEN_No,
                                       WELCOME_WIZ_SHOW_INTRO_SCREEN_Yes,
                                       NULL);
   if( 0 == strcmp( roadmap_config_get( &WELCOME_WIZ_SHOW_INTRO_SCREEN_Var), WELCOME_WIZ_SHOW_INTRO_SCREEN_Yes))
      return TRUE;
   return FALSE;
}

static void set_terms_accepted(){
   roadmap_config_set(&WELCOME_WIZ_TERMS_OF_USE_Var, WELCOME_WIZ_FIRST_TIME_Yes);
   roadmap_config_save(TRUE);
}

static BOOL is_wizard_enabled(void){
   roadmap_config_declare_enumeration( WELCOME_WIZ_ENABLE_CONFIG_TYPE,
                                       &WELCOME_WIZ_ENABLED_Var,
                                       NULL,
                                       WELCOME_WIZ_FIRST_TIME_No,
                                       WELCOME_WIZ_FIRST_TIME_Yes,
                                       NULL);
   if( 0 == strcmp( roadmap_config_get( &WELCOME_WIZ_ENABLED_Var), WELCOME_WIZ_FIRST_TIME_Yes))
      return TRUE;
   return FALSE;

}
/////////////////////////////////////////////////////////////////////
static void set_first_time_no(){
   roadmap_config_set(&WELCOME_WIZ_FIRST_TIME_Var, WELCOME_WIZ_FIRST_TIME_No);
   roadmap_config_save(TRUE);
}

/////////////////////////////////////////////////////////////////////
static SsdWidget space(int height){
   SsdWidget space;

#if (defined(_WIN32))
   height = height / 2;
#endif
   space = ssd_container_new ("spacer", NULL, SSD_MAX_SIZE, height, SSD_WIDGET_SPACE|SSD_END_ROW);
   ssd_widget_set_color (space, NULL,NULL);
   return space;
}

#ifndef TOUCH_SCREEN
/////////////////////////////////////////////////////////////////////
static int on_softkey_do_nothing(SsdWidget widget, const char *new_value, void *context){
   return 0;
}

#endif //TOUCH_SCREEN

#ifdef TOUCH_SCREEN
/////////////////////////////////////////////////////////////////////
static int end_button_callback (SsdWidget widget, const char *new_value) {

   set_first_time_no();
   ssd_dialog_hide_all (dec_ok);
   roadmap_analytics_log_event (ANALYTICS_EVENT_NEW_USER_SIGNUP, ANALYTICS_EVENT_INFO_ACTION, "End");
   roadmap_welcome_guided_tour();
   return 1;
}
#else //TOUCH_SCREEN
/////////////////////////////////////////////////////////////////////
static int on_softkey_finish(SsdWidget widget, const char *new_value, void *context){
   set_first_time_no();
   ssd_dialog_hide_all (dec_ok);
   roadmap_welcome_guided_tour();
   return 0;
}
#endif //TOUCH_SCREEN

/////////////////////////////////////////////////////////////////////
// End
/////////////////////////////////////////////////////////////////////
static void end_dialog(){
   SsdWidget dialog;
    SsdWidget group;
   SsdWidget text;

   dialog = ssd_dialog_new ("Wiz_End",
                      roadmap_lang_get ("End"),
                      NULL,
                      SSD_CONTAINER_TITLE|SSD_DIALOG_NO_SCROLL);


   group = ssd_container_new ("End_con", NULL,
                        SSD_MIN_SIZE,SSD_MIN_SIZE,SSD_WIDGET_SPACE|SSD_END_ROW|SSD_ALIGN_VCENTER|SSD_CONTAINER_BORDER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE);
   ssd_widget_set_color (group, NULL,NULL);
   ssd_widget_add (group, space(10));

   text = ssd_text_new ("Label", roadmap_lang_get("Way to go!"), 22,SSD_ALIGN_CENTER|SSD_END_ROW);
   ssd_widget_set_color(text,"#d52c6b", "#d52c6b");
   ssd_widget_add (group, text);
   ssd_widget_add (group, space(10));

   text = ssd_text_new ("Label", roadmap_lang_get("Your account has been updated"), 18,SSD_ALIGN_CENTER|SSD_END_ROW);
   ssd_widget_add (group, text);

   ssd_widget_add (group, space(20));


#ifdef TOUCH_SCREEN
   ssd_widget_add (group,
   ssd_button_label ("Finish", roadmap_lang_get ("Finish"),
                        SSD_WS_TABSTOP|SSD_ALIGN_CENTER|SSD_START_NEW_ROW, end_button_callback));
#else //TOUCH_SCREEN
   ssd_widget_set_left_softkey_text       ( dialog, roadmap_lang_get("Finish"));
   ssd_widget_set_left_softkey_callback   ( dialog, on_softkey_finish);
#endif //TOUCH_SCREEN

   ssd_widget_add (dialog, group);
   ssd_dialog_activate ("Wiz_End", NULL);
}



static void ping_button_handler( void )
{
   const char *data = ssd_dialog_get_data( WELCOME_WIZ_DLG_PING_AGREE );
   BOOL agree_to_be_pinged = !strcmp( data, "yes" );
   Realtime_Set_AllowPing( agree_to_be_pinged );
}

static BOOL twitter_button_handler( void )
{
   const char *username = ssd_dialog_get_value( "TwitterUserName" );
   const char *password = ssd_dialog_get_value( "TwitterPassword" );
   const char *tweet_signup = ssd_dialog_get_data( WELCOME_WIZ_DLG_TWITTER_SET_SIGNUP );
   BOOL is_tweet_signup = !strcmp( tweet_signup, "yes" );
   BOOL res = FALSE;

   if (!username || !*username){
      roadmap_messagebox("Error", "Please enter a user name");
      return FALSE;
   }

   if (!password || !*password){
      roadmap_messagebox("Error", "Please enter password");
      return FALSE;
   }

   roadmap_twitter_set_username( username );
   roadmap_twitter_set_password( password );
   roadmap_twitter_set_signup( is_tweet_signup );

   roadmap_config_save(TRUE);
   res = Realtime_TwitterConnect( username, password, roadmap_twitter_is_signup_enabled() );

   if ( res )
   {
      roadmap_twitter_set_signup( FALSE );
   }
   return TRUE;
}

#ifdef TOUCH_SCREEN
/////////////////////////////////////////////////////////////////////
static int ping_button_callback( SsdWidget widget, const char *new_value ) {

   if ( ( widget != NULL ) && !strcmp(widget->name, "Next") )
   {
      ping_button_handler();
   }

#ifdef ANDROID
   end_button_callback ( NULL, NULL );
#else
   end_dialog();
#endif

   return 1;
}
/////////////////////////////////////////////////////////////////////
static int twitter_button_callback (SsdWidget widget, const char *new_value) {


   if ( ( widget != NULL ) && !strcmp(widget->name, "Next"))
   {
       if ( twitter_button_handler() == FALSE ) // Don't pass to the next screen
          return 1;
   }

   welcome_wizard_ping_dialog();

   return 1;
}

#else //TOUCH_SCREEN
/////////////////////////////////////////////////////////////////////
static int on_softkey_next_twitter(SsdWidget widget, const char *new_value, void *context)
{
   if ( twitter_button_handler() )
      welcome_wizard_ping_dialog();
   return 0;
}

static int on_softkey_next_ping(SsdWidget widget, const char *new_value, void *context){
   ping_button_handler();
   end_dialog();
   return 0;
}


#endif //else TOUCH_SCREEN
/////////////////////////////////////////////////////////////////////





/***********************************************************
 *  Name        : get_welcome_wiz_entry_group
 *  Purpose     : Creates the entry with label
 *  Params      : [in]  - none
 *              : [out] - none
 *  Returns    :
 *  Notes       :
 */
static SsdWidget get_welcome_wiz_entry_group( const char* label_text, const char* entry_name, const char* edit_box_title )
{
   SsdWidget entry;

   entry = ssd_entry_label_new( entry_name, roadmap_lang_get ( label_text ), WELCOME_WIZ_DLG_LBL_FONT, WELCOME_WIZ_DLG_LBL_CNT_WIDTH,
                                                WELCOME_WIZ_DLG_ENTRY_HEIGHT, SSD_END_ROW | SSD_WS_TABSTOP, "" );

   ssd_entry_label_set_label_color( entry, WELCOME_WIZ_DLG_LBL_COLOR );
   ssd_entry_label_set_label_offset( entry, WELCOME_WIZ_DLG_HOFFSET );
   ssd_entry_label_set_kb_params( entry, roadmap_lang_get( edit_box_title ), NULL, NULL, NULL, SSD_KB_DLG_INPUT_ENGLISH );
   ssd_entry_label_set_editbox_title( entry, roadmap_lang_get( edit_box_title ) );

   return entry;
}

/////////////////////////////////////////////////////////////////////
// Ping
/////////////////////////////////////////////////////////////////////
void welcome_wizard_ping_dialog(void){
   SsdWidget dialog;
   SsdWidget group, box, label, label_cnt;
   SsdWidget group_title, text;
   SsdWidget button;
   const char* icons[3];
   char str[512];

   dialog = ssd_dialog_new( WELCOME_WIZ_DLG_PING_NAME, roadmap_lang_get( WELCOME_WIZ_DLG_PING_TITLE ), NULL,
                                                            SSD_CONTAINER_TITLE|SSD_DIALOG_NO_SCROLL );

   box = ssd_container_new ( "Ping Box", NULL, SSD_MAX_SIZE,
         SSD_MIN_SIZE, SSD_END_ROW | SSD_ALIGN_CENTER | SSD_CONTAINER_BORDER | SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE|SSD_WIDGET_SPACE );

   // Space inside white container
   ssd_dialog_add_vspace( box, ADJ_SCALE( 1 ), 0 );

   // Ping title
   group = ssd_container_new ( "Ping title group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE, SSD_END_ROW );
   ssd_dialog_add_hspace( group, ADJ_SCALE( WELCOME_WIZ_DLG_PING_HOFFSET ), SSD_ALIGN_VCENTER );

   group_title = ssd_text_new( "Ping", roadmap_lang_get( "Ping" ), 15, SSD_TEXT_LABEL );
   ssd_text_set_color( group_title, "#202020" );
   ssd_widget_add( group, group_title );
   ssd_widget_add( box, group );

   // Ping explanation text
   ssd_dialog_add_vspace( box, ADJ_SCALE( 16 ), 0 );

   snprintf( str, sizeof(str), "%s\n%s",
             roadmap_lang_get("Allow wazers to ping you."),
             roadmap_lang_get("It's useful, fun and you can always turn it off."));

   group = ssd_container_new ( "Ping text", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE, SSD_END_ROW );

   ssd_dialog_add_hspace( group, ADJ_SCALE( WELCOME_WIZ_DLG_PING_HOFFSET ), SSD_ALIGN_VCENTER );

   label_cnt = ssd_container_new ( "Label container", NULL, WELCOME_WIZ_DLG_PING_TEXT_WIDTH, SSD_MIN_SIZE, SSD_END_ROW );

   label = ssd_text_new( "Label", str, WELCOME_WIZ_DLG_LBL_FONT,
                                                            SSD_TEXT_LABEL | SSD_TEXT_NORMAL_FONT );
   ssd_text_set_color( label, WELCOME_WIZ_DLG_LBL_COLOR );

   ssd_widget_add( label_cnt, label );
   ssd_widget_add( group, label_cnt );

   ssd_widget_add( box, group );

   ssd_dialog_add_vspace( box, ADJ_SCALE( 34 ), 0 );

   // Ping checkbox
   group = ssd_container_new ("Ping agree group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE, SSD_END_ROW | SSD_WS_TABSTOP );
   ssd_widget_set_color ( group, NULL, NULL );
   ssd_dialog_add_hspace( group, ADJ_SCALE( WELCOME_WIZ_DLG_PING_HOFFSET ), 0 );
   ssd_widget_add( group, ssd_checkbox_new ( WELCOME_WIZ_DLG_PING_AGREE, TRUE,  0, NULL, NULL, NULL, CHECKBOX_STYLE_DEFAULT ) );
   label_cnt = ssd_container_new ( "Label container", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_ALIGN_VCENTER );
   ssd_widget_set_color ( label_cnt, NULL, NULL );
   ssd_dialog_add_hspace( label_cnt, ADJ_SCALE( 10 ), 0 );
   label = ssd_text_new ("Label", roadmap_lang_get( "I agree to be pinged" ), WELCOME_WIZ_DLG_LBL_FONT,
                                                               SSD_TEXT_NORMAL_FONT|SSD_TEXT_LABEL|SSD_ALIGN_VCENTER );
   ssd_text_set_color( label, WELCOME_WIZ_DLG_LBL_COLOR );
   ssd_widget_add ( label_cnt, label );
   ssd_widget_add ( group, label_cnt );
   ssd_widget_add ( box, group);


#ifdef TOUCH_SCREEN

    // Next button
    ssd_dialog_add_vspace( box, ADJ_SCALE( 8 ), 0 );

    icons[0] = "welcome_btn";
    icons[1] = "welcome_btn_h";
    icons[2] = NULL;
    group = ssd_container_new ( "Buttons group", NULL, ssd_container_get_width()*0.9, SSD_MIN_SIZE, SSD_ALIGN_CENTER | SSD_END_ROW );

    button = ssd_button_label_custom( "Next", roadmap_lang_get ("Next"), SSD_ALIGN_RIGHT | SSD_ALIGN_VCENTER, ping_button_callback,
          icons, 2, "#FFFFFF", "#FFFFFF",14 );
    text = ssd_widget_get( button, "label" );
    text->flags |= SSD_TEXT_NORMAL_FONT;
    ssd_text_set_font_size( text, 15 );
    ssd_widget_add ( group, button );

    ssd_widget_add ( box, group );

#else
    ssd_widget_set_left_softkey_text       ( dialog, roadmap_lang_get("Next"));
    ssd_widget_set_left_softkey_callback   ( dialog, on_softkey_next_ping );
#endif

    ssd_dialog_add_vspace( box, ADJ_SCALE( 12 ), 0 );

    ssd_widget_add ( dialog, box );

    ssd_dialog_activate( WELCOME_WIZ_DLG_PING_NAME, NULL );
    ssd_dialog_draw();
}
/////////////////////////////////////////////////////////////////////
// Twitter
/////////////////////////////////////////////////////////////////////
void welcome_wizard_twitter_dialog(void){
   SsdWidget dialog;
   SsdWidget group, box, label, label_cnt;
   SsdWidget group_title, text;
   SsdWidget bitmap, button;
   const char* icons[3];

   dialog = ssd_dialog_new( WELCOME_WIZ_DLG_TWITTER_NAME, roadmap_lang_get( WELCOME_WIZ_DLG_TWITTER_TITLE ), NULL, SSD_CONTAINER_TITLE|SSD_DIALOG_NO_SCROLL );

   // Space before white container
   ssd_dialog_add_vspace( dialog, ADJ_SCALE( 3 ), 0 );

   box = ssd_container_new ( "Sign Up Box", NULL, SSD_MAX_SIZE,
         SSD_MIN_SIZE, SSD_END_ROW | SSD_ALIGN_CENTER | SSD_CONTAINER_BORDER | SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE|SSD_WIDGET_SPACE );

   // Space inside white container
   ssd_dialog_add_vspace( box, ADJ_SCALE( 1 ), 0 );

   // Twitter title
   group = ssd_container_new ( "Twitter title group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE, SSD_END_ROW );
   ssd_dialog_add_hspace( group, ADJ_SCALE( WELCOME_WIZ_DLG_HOFFSET ), SSD_ALIGN_VCENTER );
   bitmap = ssd_bitmap_new( "Twitter logo","welcome_twitter", SSD_ALIGN_VCENTER);
   ssd_widget_add( group, bitmap );
   ssd_dialog_add_hspace( group, ADJ_SCALE( 15 ), SSD_ALIGN_VCENTER );
   group_title = ssd_text_new( "Twitter", roadmap_lang_get( "Twitter" ), 15, SSD_TEXT_LABEL );
   ssd_text_set_color( group_title, "#202020" );
   ssd_widget_add( group, group_title );

   ssd_widget_add( box, group );

   // Tweeter explanation text
   ssd_dialog_add_vspace( box, ADJ_SCALE( 16 ), 0 );
   group = ssd_container_new ( "Twitter text", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE, SSD_END_ROW );
   ssd_dialog_add_hspace( group, ADJ_SCALE( 5 ), SSD_ALIGN_VCENTER );
   label = ssd_text_new( "Label", roadmap_lang_get ( "Tweet your road reports" ), WELCOME_WIZ_DLG_LBL_FONT,
                                                            SSD_TEXT_LABEL | SSD_ALIGN_VCENTER | SSD_TEXT_NORMAL_FONT );
   ssd_text_set_color( label, WELCOME_WIZ_DLG_LBL_COLOR );
   ssd_widget_add( group, label );

   ssd_widget_add( box, group );

   // Username
   ssd_dialog_add_vspace( box, ADJ_SCALE( 14 ), 0 );
   group = get_welcome_wiz_entry_group( "Username", "TwitterUserName", "Username" );
   ssd_widget_add( box, group );

   // Password
   ssd_dialog_add_vspace( box, ADJ_SCALE( 10 ), 0 );
   group = get_welcome_wiz_entry_group( "Password", "TwitterPassword", "Password" );
   ssd_widget_add( box, group );

   // Tweet checkout waze
   ssd_dialog_add_vspace( box, ADJ_SCALE( 4 ), 0 );
   group = ssd_container_new ("Tweet checkout group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE, SSD_END_ROW | SSD_WS_TABSTOP );
   ssd_widget_set_color ( group, NULL, NULL );
   ssd_dialog_add_hspace( group, ADJ_SCALE( WELCOME_WIZ_DLG_HOFFSET ), 0 );
   ssd_widget_add( group, ssd_checkbox_new ( WELCOME_WIZ_DLG_TWITTER_SET_SIGNUP, TRUE,  0, NULL, NULL, NULL, CHECKBOX_STYLE_DEFAULT ) );
   label_cnt = ssd_container_new ( "Label container", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_ALIGN_VCENTER );
   ssd_widget_set_color ( label_cnt, NULL, NULL );
   ssd_dialog_add_hspace( label_cnt, ADJ_SCALE( 10 ), 0 );
   label = ssd_text_new ("Label", roadmap_lang_get ( "Tweet that I'm checking-out waze"), WELCOME_WIZ_DLG_LBL_FONT,
                                                               SSD_TEXT_NORMAL_FONT|SSD_TEXT_LABEL|SSD_ALIGN_VCENTER );
   ssd_text_set_color( label, WELCOME_WIZ_DLG_LBL_COLOR );
   ssd_widget_add ( label_cnt, label );
   ssd_widget_add ( group, label_cnt );
   ssd_widget_add ( box, group);

   ssd_dialog_add_vspace( box, ADJ_SCALE( 10 ), 0 );

#ifdef TOUCH_SCREEN

    icons[0] = "welcome_btn";
    icons[1] = "welcome_btn_h";
    icons[2] = NULL;
    // Preload image to get dimensions
    group = ssd_container_new ( "Buttons group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE, SSD_ALIGN_CENTER | SSD_END_ROW );
    button = ssd_button_label_custom( "Skip", roadmap_lang_get ("Skip"), SSD_ALIGN_CENTER | SSD_ALIGN_VCENTER, twitter_button_callback,
          icons, 2, "#FFFFFF", "#FFFFFF",14 );
    text = ssd_widget_get( button, "label" );
    text->flags |= SSD_TEXT_NORMAL_FONT;
    ssd_text_set_font_size( text, 15 );
    ssd_widget_add ( group, button );
    ssd_dialog_add_hspace( group, ADJ_SCALE( 10 ), SSD_ALIGN_CENTER | SSD_ALIGN_VCENTER );

    // Skip button

    button = ssd_button_label_custom( "Next", roadmap_lang_get ("Next"), SSD_ALIGN_CENTER | SSD_ALIGN_VCENTER, twitter_button_callback,
          icons, 2, "#FFFFFF", "#FFFFFF",14 );
    text = ssd_widget_get( button, "label" );
    text->flags |= SSD_TEXT_NORMAL_FONT;
    ssd_text_set_font_size( text, 15 );
    ssd_widget_add ( group, button );

    ssd_widget_add ( box, group );

    ssd_dialog_add_vspace( box, ADJ_SCALE( 6 ), 0 );
#else
    ssd_widget_set_left_softkey_text       ( dialog, roadmap_lang_get("Next"));
    ssd_widget_set_left_softkey_callback   ( dialog, on_softkey_next_twitter);
#endif

   ssd_widget_add ( dialog, box );

   ssd_dialog_activate( WELCOME_WIZ_DLG_TWITTER_NAME, NULL );
   ssd_dialog_draw();

}

#ifndef TOUCH_SCREEN


int intro_screen_left_key_callback (SsdWidget widget, const char *new_value, void *context) {
   return 1;  // do nothing if clicked left key, no back possible.
}

/////////////////////////////////////////////////////////////////////
static int on_skip_welcome(SsdWidget widget, const char *new_value, void *context){
   set_first_time_no();
   ssd_dialog_hide_all(dec_close);
   return 0;
}

/////////////////////////////////////////////////////////////////////
static int on_personalize(SsdWidget widget, const char *new_value, void *context){
   roadmap_login_update_dlg_show();
   return 0;
}
#endif //TOUCH_SCREEN

static void on_dialog_closed( int exit_code, void* context){
#ifdef TOUCH_SCREEN
   roadmap_bottom_bar_show();
#endif
}

static int callGlobalCallback (SsdWidget widget, const char *new_value, void *context) {
	ssd_dialog_hide_all(dec_close);
	if (gCallback)
	   (*gCallback)();

	return 1;
}

// callback for buttons in intro screen
static int intro_screen_buttons_callback(SsdWidget widget, const char *new_value){
	if (!strcmp(widget->name, "Next")){
		callGlobalCallback(NULL, NULL, NULL);
	}
	return 1;
}

// create the intro screen
static void create_intro_screen(){
	SsdWidget dialog;
	SsdWidget group;
	int tab_flag=0;
	int         width = roadmap_canvas_width() - 10;
	const char* system_lang;
#ifndef TOUCH_SCREEN
    tab_flag = SSD_WS_TABSTOP;
#endif
	system_lang = roadmap_lang_get("lang");
	dialog = ssd_dialog_new ("Intro_screen",
                            roadmap_lang_get ("Hi there!"),
                            NULL,
                            SSD_CONTAINER_TITLE|SSD_DIALOG_NO_BACK);
   group = ssd_container_new ("Welcome", NULL,
            width, SSD_MIN_SIZE,SSD_WIDGET_SPACE|SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER|SSD_END_ROW|SSD_CONTAINER_BORDER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE);
   ssd_widget_set_color (group, NULL,NULL);
   ssd_widget_add (group, space(8));
   add_intro_texts(group, tab_flag);
#ifdef TOUCH_SCREEN
   ssd_widget_add (group,
   ssd_button_label ("Next", roadmap_lang_get ("Next"),
                     SSD_WS_TABSTOP|SSD_ALIGN_CENTER|SSD_START_NEW_ROW, intro_screen_buttons_callback));

#else
   ssd_widget_set_right_softkey_text       ( dialog, roadmap_lang_get("Next"));
   ssd_widget_set_right_softkey_callback   ( dialog, callGlobalCallback);

   ssd_widget_set_left_softkey_text       ( dialog, roadmap_lang_get(""));
   ssd_widget_set_left_softkey_callback   ( dialog, intro_screen_left_key_callback);

#endif

   ssd_widget_add (group, space(20));
   ssd_widget_add( dialog, group);
   ssd_dialog_activate ("Intro_screen", NULL);
}




///////////////////////////////////////////////////////////////
// Terms of use
//////////////////////////////////////////////////////////////
static void on_terms_accepted( void )
{
#ifdef _WIN32
         const char *age_restriction_checkbox   = ssd_dialog_get_data( "age_restriction_checkbox" );
         BOOL  bAgeRestrictionAccepted = !strcmp( age_restriction_checkbox, "yes" );
         if (!bAgeRestrictionAccepted){
            return 0;
         }
#endif
         set_terms_accepted();
         if (is_show_intro_screen())
            create_intro_screen();
         else
            callGlobalCallback(NULL, NULL, NULL);
}
///////////////////////////////////////////////////////////////
static void on_terms_declined( void )
{
   roadmap_main_exit();
}
///////////////////////////////////////////////////////////////
static int term_of_use_dialog_buttons_callback (SsdWidget widget, const char *new_value) {

   if (!strcmp(widget->name, "Accept")){
      on_terms_accepted();
   }
   else if (!strcmp(widget->name, "Decline")){
      on_terms_declined();
   }

   return 1;
}

#if defined(ANDROID)
///////////////////////////////////////////////////////////////
static void on_terms_callback( int res_code, void *context )
{
   if ( res_code == dec_ok )
   {
      on_terms_accepted();
   }
   else
   {
      on_terms_declined();
   }
}
#endif

//////////////////////////////////////////////////////////////
static void on_dialog_closed_terms_of_use( int exit_code, void* context){
#ifdef TOUCH_SCREEN
   roadmap_bottom_bar_show();
#endif
}

#ifndef TOUCH_SCREEN
/////////////////////////////////////////////////////////////////////
static int terms_of_use_accepted (SsdWidget widget, const char *new_value, void *context) {
  	set_terms_accepted();
    if (is_show_intro_screen())
         create_intro_screen();
    else
         callGlobalCallback(NULL, NULL, NULL);
    return 1;
}
/////////////////////////////////////////////////////////////////////
static int terms_of_use_declined (SsdWidget widget, const char *new_value, void *context){
   roadmap_main_exit();
   return 1;
}
#else
/*
 * Right soft key is defined for the touch devices with the hw back button: this callback makes it
 * impossible to be closed
 */
static int terms_of_use_right_softkey_touch( SsdWidget widget, const char *new_value, void *context )
{
	roadmap_main_minimize();
	return 1;
}
#endif //TOUCH_SCREEN


static BOOL OnKeyPressed( SsdWidget this, const char* utf8char, uint32_t flags){
   if( VK_Arrow_down == (*utf8char))
      return 1;
   else
      return 0;
}



static SsdWidget add_text_to_group(const char * in_text,SsdWidget group, int tab_flag, int font_size, BOOL first_widget ){
	SsdWidget text;
	if(first_widget)
		text = ssd_text_new( "txt", "", font_size, tab_flag|SSD_WS_DEFWIDGET);
	else
		text = ssd_text_new( "txt", "", font_size, tab_flag);
	ssd_text_set_text_size( text, strlen(in_text));
	ssd_text_set_text( text, in_text);
	ssd_widget_add( group, text);
	ssd_widget_add (group, space(8));
	return text;
}

static void add_intro_texts(SsdWidget group, int tab_flag){
	int i;
	const char* system_lang;
	char ** intro_text ;
	SsdWidget last_widget;
	BOOL first_widget = TRUE;
    system_lang = roadmap_lang_get_system_lang();
    if (!strcmp("esp",system_lang))
    	intro_text = &INTRO_ESP[0];
    else // intro_text is english disclaimer
    	intro_text = &INTRO_ENG[0];

    //English
	for (i=0;i<NUMBER_OF_LINES_INTRO;i++){
		if (!strcmp(intro_text[i],"")){// reached an empty screen - got to the last available text
			break;
		}else{
			last_widget = add_text_to_group(intro_text[i],group,tab_flag,16,first_widget);
			first_widget = FALSE;
		}
	}
	last_widget->key_pressed     = OnKeyPressed; // disables wraparound scrolling, only up and down
}

/**
 * Add the disclaimer texts to the disclaimer screen .
 * In - SsdWidget group - the widget the texts will be added to .
 */
static void add_disclaimer_texts(SsdWidget group, int tab_flag){
	int i;
	int j;
	const char* system_lang;
	char ** disclaimer ;
	SsdWidget last_widget;
	BOOL first_widget = TRUE;
    system_lang = roadmap_lang_get_system_lang();
    if (!strcmp("esp",system_lang))
    	disclaimer = &DISCLAIMER_ESP[0][0];
    else if (!strcmp("heb",system_lang))
    	disclaimer = &DISCLAIMER_HEB[0][0];
    else // default is english disclaimer
    	disclaimer = &DISCLAIMER_ENG[0][0];

   if(is_show_intro_screen())
   		i = 1;
   else
   		i = 0;

   for (;i<NUMBER_OF_DISCLAIMER_PARTS;i++){
   		for (j=0;j<NUMBER_OF_LINES_PER_PART_DISC;j++){
	   		if (!strcmp(disclaimer[j+i*NUMBER_OF_LINES_PER_PART_DISC],"")){// reached an empty screen - got to the last available text
				break;
			}else{
				last_widget = add_text_to_group(disclaimer[j+i*NUMBER_OF_LINES_PER_PART_DISC],group,tab_flag,14, first_widget);
				first_widget = FALSE;
			}
   		}
   }

   last_widget->key_pressed     = OnKeyPressed; // disables wrap around scrolling, only up and down.

}


void term_of_use_dialog(void){
   SsdWidget   dialog;
   SsdWidget   group;
#ifdef _WIN32
   SsdWidget   age_group;
   SsdWidget   label_cnt;
#endif
   int         width = roadmap_canvas_width() - 20;
   int         tab_flag = 0;


#ifndef TOUCH_SCREEN
   tab_flag = SSD_WS_TABSTOP;
#endif



   dialog = ssd_dialog_new ("term_of_use",
                            roadmap_lang_get ("Terms of use"),
                            NULL,
                            SSD_CONTAINER_TITLE|SSD_DIALOG_NO_BACK);
   group = ssd_container_new ("Welcome", NULL,
            width, SSD_MIN_SIZE,SSD_WIDGET_SPACE|SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER|SSD_END_ROW|SSD_CONTAINER_BORDER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE);
   ssd_widget_set_color (group, NULL,NULL);

   ssd_widget_add (group, space(20));


   add_disclaimer_texts(group, tab_flag);

#ifdef _WIN32
   age_group = ssd_container_new ("Age restriction group", NULL, SSD_MAX_SIZE,
             SSD_MIN_SIZE, SSD_WIDGET_SPACE | SSD_END_ROW | tab_flag);
   ssd_widget_set_color ( age_group, NULL, NULL );
   ssd_dialog_add_hspace( age_group, 5, 0 );
   ssd_widget_add (age_group,
         ssd_checkbox_new ("age_restriction_checkbox", FALSE,  0, NULL,NULL,NULL,CHECKBOX_STYLE_DEFAULT));
   label_cnt = ssd_container_new ( "Label container", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_ALIGN_VCENTER );
   ssd_widget_set_color ( label_cnt, NULL,NULL );
   ssd_dialog_add_hspace( label_cnt, 5, 0 );
   ssd_widget_add ( label_cnt, ssd_text_new ("Label", roadmap_lang_get ("I certify that i am over the age of 13"), 14, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER ) );
   ssd_widget_add ( age_group, label_cnt );
   ssd_widget_add (group, age_group);
   ssd_widget_add (group, space(15));
#endif

#ifdef TOUCH_SCREEN
   ssd_widget_add (group,
   ssd_button_label ("Accept", roadmap_lang_get ("Accept"),
                     SSD_WS_TABSTOP|SSD_ALIGN_CENTER|SSD_START_NEW_ROW, term_of_use_dialog_buttons_callback));

   ssd_widget_add (group,
   ssd_button_label ("Decline", roadmap_lang_get ("Decline"),
                     SSD_WS_TABSTOP|SSD_ALIGN_CENTER, term_of_use_dialog_buttons_callback));
   ssd_widget_set_right_softkey_callback ( dialog, terms_of_use_right_softkey_touch );
#else
   ssd_widget_set_right_softkey_text       ( dialog, roadmap_lang_get("Accept"));
   ssd_widget_set_right_softkey_callback   ( dialog, terms_of_use_accepted);

   ssd_widget_set_left_softkey_text       ( dialog, roadmap_lang_get("Decline"));
   ssd_widget_set_left_softkey_callback   ( dialog, terms_of_use_declined);

#endif

   ssd_widget_add (group, space(20));
   ssd_widget_add( dialog, group);
   ssd_dialog_activate ("term_of_use", NULL);
   ssd_dialog_set_callback (on_dialog_closed_terms_of_use);
}


//////////////////////////////////////////////////////////////
void roadmap_term_of_use(RoadMapCallback callback){


   if ( is_terms_accepted()){
      if (callback)
         (*callback)();
         return;
   }else
      gCallback = callback;

#ifdef TOUCH_SCREEN
  roadmap_bottom_bar_hide();
#endif

#if defined(RIMAPI) || defined(ANDROID)
  terms_of_use_native();
#else
  term_of_use_dialog();
#endif
}


/////////////////////////////////////////////////////////////////////
static BOOL is_activation_time_reached(){

   time_t now;
   static BOOL initialized = FALSE;
   int minutes_to_activation;
   int first_time = roadmap_start_get_first_time_use();
   if (!initialized){
      roadmap_config_declare("preferences", &WELCOME_WIZ_MINUTES_FOR_ACTIVATION_Var,
               WELCOME_WIZ_MINUTES_FOR_ACTIVATION_Defaut, NULL);
      initialized = TRUE;
   }
   minutes_to_activation = roadmap_config_get_integer(&WELCOME_WIZ_MINUTES_FOR_ACTIVATION_Var);
   now = time(NULL);
   if ((now - first_time) > (minutes_to_activation * 60))
      return TRUE;
   else
      return FALSE;

}


/////////////////////////////////////////////////////////////////////
static int personalize_buttons_callback (SsdWidget widget, const char *new_value) {

   if (!strcmp(widget->name, "Skip")){
         set_first_time_no();
         ssd_dialog_hide_current(dec_close);
   }
   else if (!strcmp(widget->name, "Personalize")){
      roadmap_login_update_dlg_show();
   }

   return 1;
}

static int personalize_signin_callback( SsdWidget widget, const char *new_value )
{
	roadmap_login_details_dialog_show_un_pw();
	return 1;
}



void roadmap_welcome_personalize_dialog( void ){
   SsdWidget dialog;
   SsdWidget group;
   SsdWidget box, text, bitmap;
   SsdWidget text_group;
   SsdClickOffsets offsets = {-10, -10, 10, 10 };
#ifdef TOUCH_SCREEN
  roadmap_bottom_bar_hide();
#endif

   dialog = ssd_dialog_new ( "Welcome",
                            roadmap_lang_get ("Personalize your account"),
                            NULL,
                            SSD_CONTAINER_TITLE|SSD_DIALOG_NO_SCROLL );

   ssd_widget_add (dialog, space(5));

   group = ssd_container_new ("Welcome_group", NULL,
                              SSD_MAX_SIZE,SSD_MIN_SIZE,SSD_WIDGET_SPACE|SSD_END_ROW|SSD_CONTAINER_BORDER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE);
   ssd_widget_set_color (group, NULL,NULL);

   ssd_widget_add (group, space(10));

   box = ssd_container_new ("spacer1", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_WIDGET_SPACE|SSD_END_ROW);
   ssd_widget_set_color (box, NULL,NULL);

   bitmap = ssd_bitmap_new("info", "Info", 0);
   ssd_widget_add(box, bitmap);
   ssd_dialog_add_hspace(box, 10, 0);
   text = ssd_text_new ("Label", roadmap_lang_get("You are signed-in with a randomly generated login"), 18,SSD_END_ROW);
   ssd_widget_add (box, text);

   ssd_widget_add(group, box);

#ifdef TOUCH_SCREEN
   ssd_widget_add (group, space(10));
#endif


   ssd_dialog_add_vspace( group, 5, 0 );

   // Sign in text
#ifdef TOUCH_SCREEN
   text_group = ssd_container_new ( "Text container", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE, SSD_END_ROW );
   ssd_dialog_add_hspace( text_group, 1, 0 );
   ssd_widget_add( text_group, ssd_text_new("Label", roadmap_lang_get ( "Already have a login?  "), 18, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE ) );
   text = ssd_text_new( "Sign in", roadmap_lang_get ( "Sign in" ), 18, SSD_ALIGN_VCENTER );
   ssd_text_set_color( text, "#0000FF" );
   ssd_widget_add ( text_group, text );
   ssd_widget_add( group, text_group );
   ssd_widget_set_click_offsets( text_group, &offsets );
   ssd_widget_set_callback( text_group, personalize_signin_callback );

   // Space before next entry
   ssd_dialog_add_vspace( group, 3, 0 );
#endif

   ssd_widget_add (group, space(10));

   text = ssd_text_new ("Label", roadmap_lang_get("To easily access your web account, send road tweets and more, personalize your account now"), 14,SSD_END_ROW);
   ssd_widget_add (group, text);

   ssd_widget_add (group, space(10));


#ifdef TOUCH_SCREEN
   ssd_widget_add (group,
   ssd_button_label ("Skip", roadmap_lang_get ("Not now"),
                     SSD_WS_TABSTOP|SSD_ALIGN_CENTER|SSD_START_NEW_ROW, personalize_buttons_callback));

   ssd_widget_add (group,
   ssd_button_label ("Personalize", roadmap_lang_get ("Personalize"),
                     SSD_WS_TABSTOP|SSD_ALIGN_CENTER, personalize_buttons_callback));
#else //TOUCH_SCREEN
   ssd_widget_set_left_softkey_text       ( dialog, roadmap_lang_get("Skip"));
   ssd_widget_set_left_softkey_callback   ( dialog, on_skip_welcome);


   ssd_widget_set_right_softkey_text      ( dialog, roadmap_lang_get("Personalize"));
   ssd_widget_set_right_softkey_callback  ( dialog, on_personalize);
#endif //TOUCH_SCREEN

   ssd_widget_add (group, space(20));
   ssd_widget_add (dialog, group);
   ssd_dialog_activate ("Welcome", NULL);
   ssd_dialog_set_callback ( on_dialog_closed );

   roadmap_screen_refresh();

}

BOOL roadmap_welcome_on_preferences( void )
{
   if ( !Realtime_is_random_user() )
      return FALSE;

/*
   if (!is_wizard_enabled())
       return FALSE;
*/

   roadmap_welcome_personalize_dialog();

   return TRUE;
}



///////////////////////////////////////////////////////////////

/*
 * Creates the url for the web based guided tour with the parameters of device and screen dimenstions
 */
static const char* guided_tour_url(void)
{
   static char s_url[WELCOME_WIZ_URL_MAXLEN] = {0};
   const char* resolution_code = "sd";
   const char* on_skip_action = "dialog_hide_current";
   const char* on_close_action = "dialog_hide_current";
   if ( s_url[0] == 0 )
   {
      roadmap_config_declare( WELCOME_WIZ_ENABLE_CONFIG_TYPE, &WELOCME_WIZ_GUIDED_TOUR_URL_Var, WELOCME_WIZ_GUIDED_TOUR_URL_Default, NULL );
   }

   if ( roadmap_screen_is_ld_screen() )
      resolution_code = "ld";
   if ( roadmap_screen_is_hd_screen() )
      resolution_code = "hd";

   snprintf( s_url, sizeof( s_url ), "%s?deviceid=%d&on_skip=%s&on_close=%s&resolution=%s&width=%d",
         roadmap_config_get( &WELOCME_WIZ_GUIDED_TOUR_URL_Var ), RT_DEVICE_ID, on_skip_action, on_close_action, resolution_code,
         roadmap_canvas_width() );

   return s_url;
}

/*
 * Feature enabled checker
 */
static BOOL is_guided_tour_enabled(void){

#ifdef ANDROID
   return TRUE;
#else
   return FALSE;
#endif


}


/*
 * Shows the guided tour
 */
void roadmap_welcome_guided_tour( void )
{
   const char* url;

   if ( !is_guided_tour_enabled() )
   {
      roadmap_log( ROADMAP_WARNING, "Feature is disabled. Guided tour cannot be shown" );
      return;
   }

   url = guided_tour_url();

   /*
    * Checks if feature enabled
    */
   if ( !url || !url[0] )
   {
      roadmap_log( ROADMAP_ERROR, "Url initialization problem. Guided tour cannot be shown" );
      return;
   }

   roadmap_log( ROADMAP_INFO, "Showing the guided tour from url: %s", url );

   roadmap_browser_show( "Guided tour", url, NULL, NULL, NULL, BROWSER_FLAG_WINDOW_NO_TITLE_BAR|BROWSER_FLAG_WINDOW_TYPE_NO_SCROLL );
}

#if defined(RIMAPI) || defined(ANDROID)

#define MAX_DISC_LEN 6000
static void terms_of_use_native(){
   int i;
   int j;
   const char* system_lang;
   char ** disclaimer ;
   int left_size = MAX_DISC_LEN;
   char text[MAX_DISC_LEN];
   char * cursor = &text[0];
   text[0] = 0;

   system_lang = roadmap_lang_get_system_lang();

   if (strstr(system_lang,"esp"))
      disclaimer = &DISCLAIMER_ESP[0][0];
   else if (!strcmp("heb",system_lang))
      disclaimer = &DISCLAIMER_HEB[0][0];
   else // default is english disclaimer
      disclaimer = &DISCLAIMER_ENG[0][0];
   if(is_show_intro_screen())
         i = 1;
   else
         i = 0;
   for (;i<NUMBER_OF_DISCLAIMER_PARTS;i++){
         for (j=0;j<NUMBER_OF_LINES_PER_PART_DISC;j++){
            if (!strcmp(disclaimer[j+i*NUMBER_OF_LINES_PER_PART_DISC],"")){// reached an empty screen - got to the last available text
               break;
         }else{
            int size_of_string_to_add = strlen(disclaimer[j+i*NUMBER_OF_LINES_PER_PART_DISC]);
            if(size_of_string_to_add >= left_size){
               i = NUMBER_OF_DISCLAIMER_PARTS;
               break; // exit loop, text excedes buffer size.
            }
            left_size -= size_of_string_to_add;
            strcat(cursor, disclaimer[j+i*NUMBER_OF_LINES_PER_PART_DISC]);
            cursor +=  size_of_string_to_add;
            strcat(cursor, "\n\n");
            cursor += 2;
            left_size -=2;
         }
      }
   }
#if defined(RIMAPI)
   NOPH_FreemapMainScreen_popUpYesNoDialog(&text[0], roadmap_lang_get("Accept"), roadmap_lang_get("Decline"),rim_terms_of_use_accepted, rim_terms_of_use_declined,0);
#endif / RIMAPI

#if defined(ANDROID)
   roadmap_androidmsgbox_show( roadmap_lang_get ("Terms of use"), &text[0], roadmap_lang_get("Accept"), roadmap_lang_get("Decline"), NULL, on_terms_callback );
#endif
}

#endif
