/* roadmap_login.h - The interface for the signup screen
 *
 * LICENSE:
 *
 *   Copyright 2009, Waze Ltd
 *                   Alex Agranovich
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

#ifndef INCLUDE__ROADMAP_LOGIN__H
#define INCLUDE__ROADMAP_LOGIN__H

#include "ssd/ssd_widget.h"
#include "roadmap_config.h"
#ifdef __cplusplus
//extern "C" {
#endif


#define RM_LOGIN_MAX_USERNAME_LENGTH 	64
#define RM_LOGIN_MAX_PWD_LENGTH 		64
#define RM_LOGIN_MAX_NICKNAME_LENGTH 	64
#define RM_LOGIN_MAX_EMAIL_LENGTH 		320

/*
 * This type represents binding to the function showing the login dialog
 */
typedef void (*RoadmapLoginDlgShowFn)( void );

/*
 * This type represents the login details of the user
 *
 */
typedef struct
{
	char username[RM_LOGIN_MAX_USERNAME_LENGTH];
	char pwd[RM_LOGIN_MAX_PWD_LENGTH];
	char nickname[RM_LOGIN_MAX_PWD_LENGTH];
	char email[RM_LOGIN_MAX_EMAIL_LENGTH];
} LoginDetails;

/*
 * Ssd related declarations
 */
extern void roadmap_login_ssd_on_login_cb( BOOL bDetailsVerified, roadmap_result rc );

void roadmap_login_initialize();

void roadmap_login_signup_dlg_show( void );
void roadmap_login_details_dialog_show( void );
void roadmap_login_details_dialog_show_un_pw(void);
BOOL roadmap_login_details_dialog_active( void );
void roadmap_login_profile_dialog_show( void );

BOOL roadmap_login_validate_email( const char* email );
BOOL roadmap_login_validate_password( const char* password, const char* confirm_password );
BOOL roadmap_login_validate_username( const char* username );

BOOL roadmap_login_empty();

void roadmap_login_details_on_server_response(int status);
int roadmap_login_on_create( const char *username, const char* password, const char* email, BOOL send_updates );
int roadmap_login_on_update( const char *username, const char* password, const char* email, BOOL send_updates );

void roadmap_login_new_existing_dlg();
void roadmap_login_update_dlg_show( void );
void roadmap_login_update_details_on_response( roadmap_result rc );

const char* roadmap_login_dlg_get_username();
const char* roadmap_login_dlg_get_password();
const char* roadmap_login_dlg_get_nickname();

void roadmap_login_set_username( LoginDetails* login_details, const char* username );
void roadmap_login_set_pwd( LoginDetails* login_details, const char* pwd );
void roadmap_login_set_nickname( LoginDetails* login_details, const char* nickname );
void roadmap_login_on_login_cb( BOOL bDetailsVerified, roadmap_result rc );
void roadmap_login_update_login_cb( BOOL bDetailsVerified, roadmap_result rc );
int roadmap_login_on_login( SsdWidget this, const char *new_value );
int roadmap_login_on_ok( SsdWidget this, const char *new_value);
void roadmap_login_set_show_function( RoadmapLoginDlgShowFn callback );
void roadmap_login_on_signup_skip( void );

const char *roadmap_login_dlg_get_username();
const char *roadmap_login_dlg_get_password();

#ifdef __cplusplus
}
#endif
#endif /* INCLUDE__ROADMAP_LOGIN__H */

