/* roadmap_login.c - Login UI and functionality
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
#include "roadmap_config.h"
#include "roadmap_lang.h"
#include "Realtime/Realtime.h"
#include "Realtime/RealtimeDefs.h"
#include "ssd/ssd_checkbox.h"
#include "roadmap_device.h"
#include "roadmap_sound.h"
#include "roadmap_car.h"
#include "roadmap_path.h"
#include "roadmap_twitter.h"
#include "roadmap_main.h"
#include "roadmap_messagebox.h"
#include "ssd/ssd_progress_msg_dialog.h"
#include "roadmap_welcome_wizard.h"
#include "roadmap_login.h"
#include "roadmap_device_events.h"

//======== Local Types ========

//======== Defines ========

//======== Globals ========

static RoadmapLoginDlgShowFn sgLoginDlgShowFn = NULL;

//   User name
RoadMapConfigDescriptor RT_CFG_PRM_NAME_Var =
                           ROADMAP_CONFIG_ITEM(
                                    RT_CFG_TAB,
                                    RT_CFG_PRM_NAME_Name);

//   Nickname
RoadMapConfigDescriptor RT_CFG_PRM_NKNM_Var =
                           ROADMAP_CONFIG_ITEM(
                                    RT_CFG_TAB,
                                    RT_CFG_PRM_NKNM_Name);

//   Password
RoadMapConfigDescriptor RT_CFG_PRM_PASSWORD_Var =
                           ROADMAP_CONFIG_ITEM(
                                    RT_CFG_TAB,
                                    RT_CFG_PRM_PASSWORD_Name);

//======= Local interface ========
extern void roadmap_login_ssd_on_signup_skip( void );
extern BOOL roadmap_login_ssd_new_existing_in_process();

/***********************************************************
 *  Name        : roadmap_login_initialize
 *  Purpose     : Initialization of the login engine: Configuration, etc...
 *  Params      : [in]  - none
 *              : [out] - none
 *  Returns    :
 *  Notes       :
 */
void roadmap_login_initialize()
{
    roadmap_config_declare ("user", &RT_CFG_PRM_NAME_Var, "", NULL);
    roadmap_config_declare_password ("user", &RT_CFG_PRM_PASSWORD_Var, "");
    roadmap_config_declare ("user", &RT_CFG_PRM_NKNM_Var, "", NULL);
}

void roadmap_login_set_show_function (RoadmapLoginDlgShowFn callback) {
   sgLoginDlgShowFn = callback;
}

void roadmap_login_on_login_cb( BOOL bDetailsVerified, roadmap_result rc )
{

	/* Close the progress message */
   ssd_progress_msg_dialog_hide();

#ifdef IPHONE
   roadmap_main_show_root(0);
#else
   roadmap_login_ssd_on_login_cb( bDetailsVerified, rc );
#endif //IPHONE

	/*
	 * General flow related post-processing
	 */
   if( bDetailsVerified )
   {
	   Realtime_set_random_user(0);
   }
   else
   {
	   /*
	    * If no network - just let the dialog to be closed,
	    * if wrong details - stay in the dialog
	    */
	   if ( rc == err_failed )
	   {
		  roadmap_messagebox( "Login Failed", "No Network connection" );
	   }
	   else
	   {
		   if ( sgLoginDlgShowFn )
		   {
			   sgLoginDlgShowFn();
		   }
		   roadmap_messagebox( "Login Failed: Wrong login details", "Please verify login details are accurate" );
	   }
   }
}


void roadmap_login_update_login_cb( BOOL bDetailsVerified, roadmap_result rc )
{
	/* Close the progress message */
    ssd_progress_msg_dialog_hide();

#ifdef IPHONE

#else
   roadmap_login_ssd_on_login_cb( bDetailsVerified, rc );
#endif //IPHONE


   if( bDetailsVerified)
   {
      Realtime_set_random_user(0);
      welcome_wizard_twitter_dialog();
   }
   else
   {
	 if ( rc == err_failed )
	 {
		roadmap_messagebox( "Sign up failed", "No Network connection" );
	 }
	 else
	 {
		roadmap_login_update_dlg_show();
	 }
   }
}


int roadmap_login_on_login( SsdWidget this, const char *new_value )
{
   const char *username = NULL;
   const char *password = NULL;

   username = roadmap_login_dlg_get_username();
   password = roadmap_login_dlg_get_password();

   if (!*username || !*password )
   {
      roadmap_messagebox( "Login details are missing", "You must have a valid username and password.");
      return 0;
   }

   // ssd_dialog_hide_current(dec_cancel);
   ssd_progress_msg_dialog_show( roadmap_lang_get( "Signing in . . . " ) );

   Realtime_SetLoginUsername( username );
   Realtime_SetLoginPassword( password );
   Realtime_VerifyLoginDetails( roadmap_login_on_login_cb );

   return 0;
}


int roadmap_login_on_ok( SsdWidget this, const char *new_value)
{
   const char *username = NULL;
   const char *password = NULL;
   const char *nickname = NULL;

   username = roadmap_login_dlg_get_username();
   password = roadmap_login_dlg_get_password();
   nickname = roadmap_login_dlg_get_nickname();
   if ( strcmp( roadmap_config_get( &RT_CFG_PRM_NAME_Var), username ) ||
        strcmp( roadmap_config_get( &RT_CFG_PRM_PASSWORD_Var), password ) ||
    		   !Realtime_IsLoggedIn() )
   {
      roadmap_config_set( &RT_CFG_PRM_NKNM_Var, nickname );
      roadmap_login_on_login( NULL, NULL );
   }
   else{
      if (strcmp( roadmap_config_get( &RT_CFG_PRM_NKNM_Var), nickname )){
         roadmap_config_set( &RT_CFG_PRM_NKNM_Var, nickname );
         roadmap_config_save(TRUE);
         Realtime_Relogin();
      }
   }
   return 0;
}

void roadmap_login_details_dialog_show(void)
{

    if ( Realtime_is_random_user() )
    {
    	roadmap_welcome_personalize_dialog();
    	return;
    }

    if ( roadmap_login_empty() )
    {
    	roadmap_login_new_existing_dlg();
    	return;
    }

    /*
     * Manual user - show personilize
     */
    roadmap_login_profile_dialog_show();

}

BOOL check_alphanumeric(const char *str){
   int i =0;
   int len = strlen(str);

   for (i = 0; i< len;i++){
      if (!isalnum(str[i])){
         if (!((str[i] == '-') || (str[i] == '_')))
            return FALSE;
      }
   }
   return TRUE;
}

void roadmap_login_on_signup_skip( void )
{
	/*
	 * Create the random account just in case that there was no successful login before
	 */
	if ( !Realtime_IsLoggedIn() )
	{
	   Realtime_RandomUserRegister();
	}

#ifdef IPHONE
   roadmap_main_show_root(0);
#else
   roadmap_login_ssd_on_signup_skip();
#endif //IPHONE

}

/***********************************************************
 *  Name        : roadmap_login_on_create()
 *  Purpose     : Creating the new account
 *  Params      : [in] username -
 *  			  [in] password -
 *    			  [in] email -
 *    			  [in] send_updates -
 *              : [out]
 *  Notes       :
 */
int roadmap_login_on_create( const char *username, const char* password, const char* email, BOOL send_updates )
{

   ssd_progress_msg_dialog_show( roadmap_lang_get( "Creating new account . . . " ) );

   if ( !Realtime_CreateAccount( username, password, email, send_updates ) )
   {
	  ssd_progress_msg_dialog_hide();
	  roadmap_messagebox("Error", "Network connection problems, please try again later.");
      return FALSE;
   }

   return TRUE;
}


/***********************************************************
 *  Name        : roadmap_login_on_update()
 *  Purpose     : Creating the new account
 *  Params      : [in] username -
 *  			  [in] password -
 *    			  [in] email -
 *    			  [in] send_updates -
 *              : [out]
 *  Notes       :
 */
int roadmap_login_on_update( const char *username, const char* password, const char* email, BOOL send_updates )
{

   ssd_progress_msg_dialog_show( roadmap_lang_get( "Updating account . . . " ) );

   if (!Realtime_UpdateProfile( username, password, email, send_updates ) )
   {
	  ssd_progress_msg_dialog_hide();
      roadmap_messagebox("Error", "Network connection problems, please try again later.");
      return FALSE;
   }
   return TRUE;
}


BOOL roadmap_login_validate_username( const char* username )
{
	if ( strlen( username ) == 0 )
	{
	  roadmap_messagebox("Error", "Username is empty");
	  return FALSE;
	}

	if (!isalpha( username[0]) ){
	  roadmap_messagebox("Error", "Username must start with alpha characters and may contain only alpha characters, digits and '-'");
	  return FALSE;
	}

	if ( !check_alphanumeric( username ) )
	{
	  roadmap_messagebox("Error", "Username must start with alpha characters and may contain only alpha characters, digits and '-'");
	  return FALSE;
	}

	return TRUE;
}

BOOL roadmap_login_validate_password( const char* password, const char* confirm_password )
{
	if ( ( strlen( password ) < 6 ) || ( strlen( password ) > 16 ) ){
	  roadmap_messagebox("Error", "Password must be between 6 and 16 characters and may contain only alpha characters and digits");
	  return FALSE;
	}

	if ( !check_alphanumeric( password ) )
	{
	  roadmap_messagebox("Error", "Password must be between 6 and 16 characters and may contain only alpha characters and digits");
	  return FALSE;
	}

	if ( strlen( confirm_password ) == 0 )
	{
	  roadmap_messagebox("Error", "Two passwords are not identical");
	  return FALSE;
	}

	if ( strcmp( password, confirm_password ) )
	{
	  roadmap_messagebox("Error", "Two passwords are not identical");
	  return FALSE;
	}
	return TRUE;
}

BOOL roadmap_login_validate_email( const char* email )
{
	if ( !strchr(email,'@') || !strchr(email,'.') )
	{
		roadmap_messagebox("Error", "Invalid email address");
		return FALSE;
	}
	return TRUE;
}

void roadmap_login_details_update_profile_ok_repsonse()
{
   ssd_progress_msg_dialog_show( roadmap_lang_get( "Signing in . . . " ) );

   Realtime_SetLoginUsername( roadmap_login_dlg_get_username() );
   Realtime_SetLoginPassword( roadmap_login_dlg_get_password() );
   Realtime_SetLoginNickname( roadmap_login_dlg_get_nickname() );

   Realtime_VerifyLoginDetails( roadmap_login_update_login_cb );
}

void roadmap_login_update_details_on_response( roadmap_result rc )
{
   ssd_progress_msg_dialog_hide ();

   if ( rc != succeeded )
   {
	   roadmap_log( ROADMAP_ERROR, "Update/Create Account had failed with code roadmap error code rc = %d", rc );
   }

   switch ( rc )
   {
      case succeeded: //success
      {
         roadmap_login_details_update_profile_ok_repsonse ();
         break;
      }
      case err_upd_account_invalid_user_name: //invalid user name
      {
         roadmap_messagebox ("Error", "Invalid username");
         break;
      }
      case err_upd_account_name_already_exists://user already exists
      {
         roadmap_messagebox ("Error", "Username already exists");
         break;
      }
      case err_upd_account_invalid_password://invalid password
      {
         roadmap_messagebox ("Error", "Invalid password");
         break;
      }
      case err_upd_account_invalid_email:// invalid email
      {
         roadmap_messagebox ("Error", "Invalid email address");
         break;
      }
      case err_upd_account_email_exists://Email address already exist
      {
         roadmap_messagebox ("Error", "Email address already exist");
         break;
      }
      case err_upd_account_cannot_complete_request://internal server error cannot complete request
      {
         roadmap_messagebox ("Error", "Failed to update account, please try again");
         break;
      }
      default:
      {
    	 roadmap_messagebox ("Error", "Failed to update account, please try again or press 'Skip' to use random account" );
         roadmap_log (ROADMAP_ERROR,"roadmap_login_update_details_on_response - invalid status code (%d)" , rc );
      }
  }
}

void roadmap_login_set_username( LoginDetails* login_details, const char* username )
{
	strncpy( login_details->username, username, RM_LOGIN_MAX_USERNAME_LENGTH );
}

void roadmap_login_set_pwd( LoginDetails* login_details, const char* pwd )
{
	strncpy( login_details->pwd, pwd, RM_LOGIN_MAX_PWD_LENGTH );
}

void roadmap_login_set_nickname(  LoginDetails* login_details, const char* nickname )
{
	strncpy( login_details->nickname, nickname, RM_LOGIN_MAX_NICKNAME_LENGTH );
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
 *  Name        : Returns true if user&pwd fields are empty.
 *                     Indicates if user had ever successful login, becouse these fields can not be empty
 *  Purpose     :
 *  Params      : [in]
 *              : [out]
 *  Notes       :
 */
BOOL roadmap_login_empty()
{	BOOL bRes;
    const char *username = roadmap_config_get( &RT_CFG_PRM_NAME_Var);
    const char *password = roadmap_config_get( &RT_CFG_PRM_PASSWORD_Var);
	bRes = ( strlen( username ) == 0 ) || ( strlen( password ) == 0 );
	return bRes;
}




/***********************************************************
 *  Name        :
 *  Purpose     :
 *  Params      : [in]
 *              : [out]
 *  Notes       :
 */
