/* ssd_login_details.m
 *
 * LICENSE:
 *
 *   Copyright 2008 Ehud Shabtai
 *   Copyright 2009 Avi R.
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
 */


#include <assert.h>
#include <string.h>
#include "roadmap.h"
#include "roadmap_keyboard.h"
#include "roadmap_config.h"
#include "roadmap_lang.h"
#include "Realtime/Realtime.h"
#include "Realtime/RealtimeDefs.h"
#include "ssd/ssd_login_details.h"
#include "roadmap_device.h"
#include "roadmap_sound.h"
#include "roadmap_car.h"
#include "roadmap_mood.h"
#include "roadmap_path.h"

#include "iphonessd_login_details.h"
#include "roadmap_main.h"
#include "roadmap_iphonemain.h"
#include "widgets/iphoneCell.h"
#include "widgets/iphoneCellEdit.h"
#include "widgets/iphoneCellSwitch.h"
#include "roadmap_factory.h"
#include "roadmap_start.h"
#include "roadmap_twitter.h"
#include "roadmap_messagebox.h"
#include "ssd_progress_msg_dialog.h" //TODO: iPhone native dialog instead?

static const char*   titleProfile = "Profile";
static const char*   titleTwitter = "Twitter";
static const char*   titleUnPw = "Login";

static int loginIsShown = 0;


extern RoadMapConfigDescriptor RT_CFG_PRM_NAME_Var;
extern RoadMapConfigDescriptor RT_CFG_PRM_PASSWORD_Var;
extern RoadMapConfigDescriptor RT_CFG_PRM_NKNM_Var;

enum IDs {
	ID_USERNAME = 0,
	ID_PASSWORD,
	ID_NICKNAME,
	ID_AVATAR,
	//ID_MOOD,
	ID_TWITTER,
	ID_TWITTER_ENABLED,
	ID_TWITTER_USERNAME,
	ID_TWITTER_PASSWORD
};

#define MAX_IDS 25

static const char *id_actions[MAX_IDS];
static RoadMapCallback id_callbacks[MAX_IDS];
                        

void OnLoginDetailsTestResults( BOOL bDetailsVerified, roadmap_result rc)
{
	ssd_dialog_hide_all(dec_cancel);
	
   if( !bDetailsVerified)
   {
      roadmap_messagebox( "Login Failed: Wrong login details", "Please verify login details are accurate");
	   ssd_login_details_dialog_show_un_pw();
   }
}


void ssd_login_details_on_server_response (int status) {
	
	ssd_progress_msg_dialog_hide ();
	switch (status) {
		case 200: //success
			//ssd_login_details_register_ok_repsonse ();
			break;
		case 901: //invalid user name
			roadmap_messagebox ("Error", "Invalid username");
			break;
		case 902://user already exists
			roadmap_messagebox ("Error", "username already exists");
			break;
		case 903://invalid password
			roadmap_messagebox ("Error", "Invalid password");
			break;
		case 904:// invalid email
			roadmap_messagebox ("Error", "Invalid email address");
			break;
		case 905://Email address already exist
			roadmap_messagebox ("Error", "Email address already exists");
			break;
		case 906://internal server error cannot complete request
			roadmap_messagebox ("Error", "Failed to create account, please try again");
			break;
		default:
			roadmap_log (ROADMAP_ERROR,"ssd_login_details_on_server_response - invalid status code (%d)" , status);
	}
}



void twitter_settings() {
	LoginDialog *dialog = [[LoginDialog alloc] initWithStyle:UITableViewStyleGrouped];
	[dialog showTwitter];
}

void ssd_login_details_dialog_show_un_pw(void) {
	if (loginIsShown)
		return;
	loginIsShown = 1;
	
	LoginDialog *dialog = [[LoginDialog alloc] initWithStyle:UITableViewStyleGrouped];
	[dialog showUnPw];
}

void ssd_login_details_dialog_show(void) {
	if (loginIsShown)
		return;
	loginIsShown = 1;
	
	LoginDialog *dialog = [[LoginDialog alloc] initWithStyle:UITableViewStyleGrouped];
	[dialog showLogin];
}



@implementation LoginDialog
@synthesize dataArray;

- (id)initWithStyle:(UITableViewStyle)style
{
	int i;
	static int initialized = 0;
	
	dataArray = [[NSMutableArray arrayWithCapacity:1] retain];
	
	if (!initialized) {
		for (i=0; i < MAX_IDS; ++i) {
			id_callbacks[i] = NULL;
			id_actions[i] = NULL;
		}
		initialized = 1;
	}
	
	return [super initWithStyle:style];
}

- (void) populateUnPwData
{
	static int initialized = 0;
	NSMutableArray *groupArray = NULL;
	iphoneCellEdit *editCell = NULL;
	
	if (!initialized) {
		initialized = 1;
		
		//first group: username, password
		roadmap_config_declare
		("user", &RT_CFG_PRM_NAME_Var, "", NULL);
		roadmap_config_declare_password
		("user", &RT_CFG_PRM_PASSWORD_Var, "");
		roadmap_config_declare
		("user", &RT_CFG_PRM_NKNM_Var, "", NULL);
	}
	
	//first group
	groupArray = [NSMutableArray arrayWithCapacity:1];
	
	editCell = [[[iphoneCellEdit alloc] initWithFrame:CGRectZero reuseIdentifier:@"editCell"] autorelease];
	[editCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get("UserName")]];
	[editCell setText:[NSString stringWithUTF8String:roadmap_config_get( &RT_CFG_PRM_NAME_Var)]];
	[editCell setTag:ID_USERNAME];
	[editCell setDelegate:self];
	[groupArray addObject:editCell];
	
	editCell = [[[iphoneCellEdit alloc] initWithFrame:CGRectZero reuseIdentifier:@"editCell"] autorelease];
	[editCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get("Password")]];
	[editCell setText:[NSString stringWithUTF8String:roadmap_config_get( &RT_CFG_PRM_PASSWORD_Var)]];
	[editCell setTag:ID_PASSWORD];
	[editCell setDelegate:self];
	[editCell setPassword: YES];
	[groupArray addObject:editCell];
	
	[dataArray addObject:groupArray];
	
}

- (void) populateTwitterData
{
	NSMutableArray *groupArray = NULL;
	iphoneCellEdit *editCell = NULL;
	iphoneCellSwitch *swCell = NULL;
	
	
	groupArray = [NSMutableArray arrayWithCapacity:1];
	
	swCell = [[[iphoneCellSwitch alloc] initWithFrame:CGRectZero reuseIdentifier:@"switchCell"] autorelease];
	[swCell setTag:ID_TWITTER_ENABLED];
	[swCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get 
					  ("Twit my reports to my followers")]];
	[swCell setDelegate:self];
	[swCell setState:roadmap_twitter_is_sending_enabled()];
	[groupArray addObject:swCell];
	
	editCell = [[[iphoneCellEdit alloc] initWithFrame:CGRectZero reuseIdentifier:@"editCell"] autorelease];
	[editCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get("UserName")]];
	[editCell setText:[NSString stringWithUTF8String:roadmap_twitter_get_username()]];
	[editCell setTag:ID_TWITTER_USERNAME];
	[editCell setDelegate:self];
	[editCell setEnableCell:roadmap_twitter_is_sending_enabled()];
	[groupArray addObject:editCell];
	
	editCell = [[[iphoneCellEdit alloc] initWithFrame:CGRectZero reuseIdentifier:@"editCell"] autorelease];
	[editCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get("Password")]];
	[editCell setText:[NSString stringWithUTF8String:roadmap_twitter_get_password()]];
	[editCell setTag:ID_TWITTER_PASSWORD];
	[editCell setDelegate:self];
	[editCell setPassword: YES];
	[editCell setEnableCell:roadmap_twitter_is_sending_enabled()];
	[groupArray addObject:editCell];
	
	[dataArray addObject:groupArray];
}

- (void) populateLoginData
{
	static int initialized = 0;
	NSMutableArray *groupArray = NULL;
	iphoneCellEdit *editCell = NULL;
	iphoneCell *actionCell = NULL;
	iphoneCell *callbackCell = NULL;
	UIImage *img = NULL;
	char *icon_name;
	const char *cursor;
	const RoadMapAction *this_action;
	
	if (!initialized) {
		initialized = 1;
		
		//first group: username, password, nickname
		roadmap_config_declare
		("user", &RT_CFG_PRM_NAME_Var, "", NULL);
		roadmap_config_declare_password
		("user", &RT_CFG_PRM_PASSWORD_Var, "");
		roadmap_config_declare
		("user", &RT_CFG_PRM_NKNM_Var, "", NULL);
	}
	
	//first group
	groupArray = [NSMutableArray arrayWithCapacity:1];
	
	editCell = [[[iphoneCellEdit alloc] initWithFrame:CGRectZero reuseIdentifier:@"editCell"] autorelease];
	[editCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get("UserName")]];
	[editCell setText:[NSString stringWithUTF8String:roadmap_config_get( &RT_CFG_PRM_NAME_Var)]];
	[editCell setTag:ID_USERNAME];
	[editCell setDelegate:self];
	[groupArray addObject:editCell];
	
	editCell = [[[iphoneCellEdit alloc] initWithFrame:CGRectZero reuseIdentifier:@"editCell"] autorelease];
	[editCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get("Password")]];
	[editCell setText:[NSString stringWithUTF8String:roadmap_config_get( &RT_CFG_PRM_PASSWORD_Var)]];
	[editCell setTag:ID_PASSWORD];
	[editCell setDelegate:self];
	[editCell setPassword: YES];
	[groupArray addObject:editCell];
	
	editCell = [[[iphoneCellEdit alloc] initWithFrame:CGRectZero reuseIdentifier:@"editCell"] autorelease];
	[editCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get("Nickname")]];
	[editCell setText:[NSString stringWithUTF8String:roadmap_config_get( &RT_CFG_PRM_NKNM_Var)]];
	[editCell setTag:ID_NICKNAME];
	[editCell setDelegate:self];
	[groupArray addObject:editCell];
	
	[dataArray addObject:groupArray];
	
	
	//second group
	groupArray = [NSMutableArray arrayWithCapacity:1];
	//Avatar
	actionCell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"actionCell"] autorelease];
	icon_name = "select_car";
	for (cursor = roadmap_path_first ("skin");
		 cursor != NULL;
		 cursor = roadmap_path_next ("skin", cursor)){
		
		NSString *fileName = [NSString stringWithFormat:@"%s/%s.png", cursor, icon_name];
		img = [[UIImage alloc] initWithContentsOfFile: fileName];
		
		if (img) break; 
	}
	if (img) {
		[actionCell setImage:img];
		[img release];
	}
	[actionCell setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
	[actionCell setTag:ID_AVATAR];
	id_actions[ID_AVATAR] = "select_car";
	this_action =  roadmap_start_find_action (id_actions[ID_AVATAR]);
	[actionCell setText:[NSString stringWithUTF8String:roadmap_lang_get 
						 (this_action->label_long)]];
	[groupArray addObject:actionCell];
	
	[dataArray addObject:groupArray];
	
	//third group
	groupArray = [NSMutableArray arrayWithCapacity:1];
	//Twitter
	callbackCell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"actionCell"] autorelease];
	icon_name = "twitter_settings";
	for (cursor = roadmap_path_first ("skin");
		 cursor != NULL;
		 cursor = roadmap_path_next ("skin", cursor)){
		
		NSString *fileName = [NSString stringWithFormat:@"%s/%s.png", cursor, icon_name];
		img = [[UIImage alloc] initWithContentsOfFile: fileName];
		
		if (img) break; 
	}
	if (img) {
		[callbackCell setImage:img];
		[img release];
	}
	[callbackCell setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
	[callbackCell setTag:ID_TWITTER];
	id_callbacks[ID_TWITTER] = twitter_settings;
	[callbackCell setText:[NSString stringWithUTF8String:roadmap_lang_get 
						 ("Twitter")]];
	[groupArray addObject:callbackCell];
	
	[dataArray addObject:groupArray];
}


- (void) showLogin
{
	UITableView *tableView = [self tableView];
	
	[self populateLoginData];
	
	[tableView setBackgroundColor:[UIColor clearColor]];
	CGRect rect =  [[tableView window] frame];
	rect.size.height = 300.0f;
	[[tableView window] setFrame:rect];

	[self setTitle:[NSString stringWithUTF8String:roadmap_lang_get(titleProfile)]];
	
	roadmap_main_push_view (self);
}

- (void) showTwitter
{
	UITableView *tableView = [self tableView];
	
	[self populateTwitterData];
	
	[tableView setBackgroundColor:[UIColor clearColor]];
	CGRect rect =  [[tableView window] frame];
	rect.size.height = 300.0f;
	[[tableView window] setFrame:rect];
	
	[self setTitle:[NSString stringWithUTF8String:roadmap_lang_get(titleTwitter)]];
	
	roadmap_main_push_view (self);
}

- (void) showUnPw
{
	UITableView *tableView = [self tableView];
	
	[self populateUnPwData];
	
	[tableView setBackgroundColor:[UIColor clearColor]];
	CGRect rect =  [[tableView window] frame];
	rect.size.height = 300.0f;
	[[tableView window] setFrame:rect];
	
	[self setTitle:[NSString stringWithUTF8String:roadmap_lang_get(titleUnPw)]];
	
	roadmap_main_push_view (self);
}

- (void)dealloc
{
	int success;
	
	if ([[self title] compare:[NSString stringWithUTF8String:roadmap_lang_get(titleTwitter)]] == NSOrderedSame) {
		
		success = Realtime_TwitterConnect(roadmap_twitter_get_username(), roadmap_twitter_get_password());
		loginIsShown = 0;
	}

	if (([[self title] compare:[NSString stringWithUTF8String:roadmap_lang_get(titleProfile)]] == NSOrderedSame) ||
		([[self title] compare:[NSString stringWithUTF8String:roadmap_lang_get(titleUnPw)]] == NSOrderedSame)) {
		
		ssd_progress_msg_dialog_show( roadmap_lang_get( "Signing in . . . " ) );
		success = Realtime_VerifyLoginDetails( OnLoginDetailsTestResults );
		
		loginIsShown = 0;
	}
	
	
	[dataArray release];
	
	[super dealloc];
}



//////////////////////////////////////////////////////////
//Table view delegate
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return [dataArray count];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	return [(NSArray *)[dataArray objectAtIndex:section] count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {

	iphoneCell *cell = (iphoneCell *)[(NSArray *)[dataArray objectAtIndex:indexPath.section] objectAtIndex:indexPath.row];
	
	if ([cell tag] == ID_TWITTER_USERNAME ||
		[cell tag] == ID_TWITTER_PASSWORD)
		[(iphoneCellEdit *)cell setEnableCell:roadmap_twitter_is_sending_enabled()];

	return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	int tag = [[tableView cellForRowAtIndexPath:indexPath] tag];
	
	if (id_actions[tag]) {
		const RoadMapAction *this_action =  roadmap_start_find_action (id_actions[tag]);
		(*this_action->callback)();
	}
	
	if (id_callbacks[tag]) {
		(*id_callbacks[tag])();
	}
}


//////////////////////////////////////////////////////////
//Text field delegate
- (void)textFieldDidEndEditing:(UITextField *)textField {
	UIView *view = [[textField superview] superview];
	int tag = [view tag];

	switch (tag) {
		case ID_USERNAME:
			roadmap_config_set(&RT_CFG_PRM_NAME_Var, [[textField text] UTF8String]);
			break;
		case ID_PASSWORD:
			roadmap_config_set(&RT_CFG_PRM_PASSWORD_Var, [[textField text] UTF8String]);
			break;
		case ID_NICKNAME:
			roadmap_config_set(&RT_CFG_PRM_NKNM_Var, [[textField text] UTF8String]);
			break;
		case ID_TWITTER_USERNAME:
			roadmap_twitter_set_username([[textField text] UTF8String]);
			break;
		case ID_TWITTER_PASSWORD:
			roadmap_twitter_set_password([[textField text] UTF8String]);
			break;
		default:
			break;
	}
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
	[textField resignFirstResponder];
	return NO;
}


//////////////////////////////////////////////////////////
//Switch delegate
- (void) switchToggle:(id)switchView {
	static const char *enabled[2];
	UITableView *tableView = [self tableView];
	
	if (!enabled[0]) {
		enabled[0] = "Enabled";
		enabled[1] = "Disabled";
	}
	
	iphoneCellSwitch *view = (iphoneCellSwitch*)[[switchView superview] superview];
	int tag = [view tag];
	
	switch (tag) {
		case ID_TWITTER_ENABLED:
			if ([view getState])
				roadmap_twitter_enable_sending();
			else
				roadmap_twitter_diable_sending(); //disable editbox and label when switch is off
			
			[tableView reloadData];
			break;
		default:
			break;
	}
	
}


@end

