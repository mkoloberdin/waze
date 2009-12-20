/* roadmap_iphonelogin.m - iPhone dialogs for login
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi R.
 *   Copyright 2009, Waze Ltd
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
#include "roadmap_login.h"
#include "roadmap_device.h"
#include "roadmap_sound.h"
#include "roadmap_car.h"
#include "roadmap_path.h"

#include "roadmap_iphonelogin.h"
#include "roadmap_main.h"
#include "roadmap_iphonemain.h"
#include "widgets/iphoneCell.h"
#include "widgets/iphoneCellEdit.h"
#include "widgets/iphoneCellSwitch.h"
#include "roadmap_factory.h"
#include "roadmap_start.h"
#include "roadmap_twitter.h"
#include "roadmap_foursquare.h"
#include "roadmap_messagebox.h"
#include "roadmap_iphoneimage.h"
#include "ssd_progress_msg_dialog.h"
#include "roadmap_device_events.h"
#include "roadmap_welcome_wizard.h"

static const char*   titleProfile = "Profile";
static const char*   titleUnPw = "Log In";


static int isTwitterModified  = 0;
static int isUnPwShown        = 0;
static int isNewExistingShown = 0;
static int isUpdate           = 0;

static char                    LoginDialogUserName[100];
static char                    LoginDialogPassword[100];
static char                    LoginDialogNickname[100];


extern RoadMapConfigDescriptor RT_CFG_PRM_NAME_Var;
extern RoadMapConfigDescriptor RT_CFG_PRM_PASSWORD_Var;
extern RoadMapConfigDescriptor RT_CFG_PRM_NKNM_Var;

enum IDs {
	ID_USERNAME = 1,
	ID_PASSWORD,
	ID_NICKNAME,
	ID_AVATAR,
	ID_TWITTER,
   ID_FOURSQUARE,
	ID_TWITTER_ENABLED,
	ID_TWITTER_USERNAME,
	ID_TWITTER_PASSWORD,
   ID_CHECKED,
   ID_UNCHECKED,
   ID_CONFIRM_PASSWORD,
   ID_EMAIL
};

#define MAX_IDS 25
#define TEXT_HEIGHT 30.0f

static const char *id_actions[MAX_IDS];
static RoadMapCallback id_callbacks[MAX_IDS];

static CGSize gSizeSigninBtn = {125.0f, 30.0f};
static CGSize gSizeNormalBtn = {100.0f, 30.0f};

static LoginDialog *gRoadmapLoginDialog;


const char *roadmap_login_dlg_get_username(){
   return LoginDialogUserName;
}

const char *roadmap_login_dlg_get_password(){
   return LoginDialogPassword;
}

const char *roadmap_login_dlg_get_nickname(){
   return LoginDialogNickname;
}

void roadmap_login_details_dialog_show_un_pw(void) {
   if (isUnPwShown)
      return;
   
   isUnPwShown = TRUE;
   
   roadmap_login_set_show_function(roadmap_login_details_dialog_show_un_pw);
   
	LoginDialog *gRoadmapLoginDialog = [[LoginDialog alloc] initWithStyle:UITableViewStyleGrouped];
	[gRoadmapLoginDialog showUnPw];
}


void roadmap_login_profile_dialog_show(void) {
   roadmap_login_set_show_function ((RoadmapLoginDlgShowFn) roadmap_login_profile_dialog_show);
   
	gRoadmapLoginDialog = [[LoginDialog alloc] initWithStyle:UITableViewStyleGrouped];
	[gRoadmapLoginDialog showLogin];
}


void roadmap_login_new_existing_dlg(){
   if (isNewExistingShown)
      return;
   
   isNewExistingShown = TRUE;
   
   NewExistingDialog *dialog = [[NewExistingDialog alloc] init];
	[dialog show];
}

void roadmap_login_update_dlg_show( void ) {
   LoginUpdateDialog *dialog = [[LoginUpdateDialog alloc] init];
	[dialog showUpdateLogin];
}



//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
@implementation NewExistingDialog

- (void) onGetStarted
{
   roadmap_login_update_dlg_show();
}


- (void) onSignIn
{
   roadmap_login_details_dialog_show_un_pw();
}

////////////////////
// New Existing Dialog
- (void)show
{
   CGFloat viewPosY = 5.0f;
	CGRect rect;
	UIImage *image;
	UIImageView *imageView;
	UILabel *label;
	UIButton *button;
	rect = [[UIScreen mainScreen] applicationFrame];
	rect.origin.x = 0;
	rect.origin.y = 0;
	rect.size.height = roadmap_main_get_mainbox_height();
	UIScrollView *scrollView = [[UIScrollView alloc] initWithFrame:rect];
	self.view = scrollView;
	[scrollView release]; // decrement retain count
	[scrollView setDelegate:self];
   UIImage *imgButtonUp;
   UIImage *imgButtonDown;
   
   image = roadmap_iphoneimage_load("button_up");
   if (image) {
      imgButtonUp = [image stretchableImageWithLeftCapWidth:10 topCapHeight:10];
      [image release];
   }
   image = roadmap_iphoneimage_load("button_down");
   if (image) {
      imgButtonDown = [image stretchableImageWithLeftCapWidth:10 topCapHeight:10];
      [image release];
   }
	
	[self setTitle:[NSString stringWithUTF8String: roadmap_lang_get ("Welcome")]];
   
   //hide left button
	UINavigationItem *navItem = [self navigationItem];
	[navItem setHidesBackButton:YES];
	
	
	//set UI elements
	
   //Existing user
	//background frame
	image = roadmap_iphoneimage_load("Bubble_01");
	if (image) {
		rect = CGRectMake(([scrollView frame].size.width - [image size].width) /2, viewPosY,
                        [image size].width, [image size].height);
      imageView = [[UIImageView alloc] initWithFrame:rect];
      [imageView setImage:[image stretchableImageWithLeftCapWidth:15 topCapHeight:15]];
		[image release];
		[scrollView addSubview:imageView];
      [imageView release];
	}
	
	//text
	rect = imageView.frame;
	rect.size.width -= 15 + gSizeSigninBtn.width;
	rect.size.height = rect.size.height - 20 -10;
	rect.origin.x += 5;
	rect.origin.y += 5;
	label = [[iphoneLabel alloc] initWithFrame:rect];
	[label setText:[NSString stringWithUTF8String:roadmap_lang_get("EXISTING user")]];
	[label setTextColor:[UIColor colorWithRed:0.941f green:0.0f blue:0.376f alpha:1.0f]];
	[scrollView addSubview:label];
	[label release];
   
   //button
   rect = imageView.frame;
   rect.origin.x += rect.size.width - 5 - gSizeSigninBtn.width;
   rect.origin.y += (rect.size.height - gSizeSigninBtn.height - 20) /2;
   rect.size = gSizeSigninBtn;
   button = [UIButton buttonWithType:UIButtonTypeCustom];
	[button setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Sign In")] forState:UIControlStateNormal];
	if (imgButtonUp)
		[button setBackgroundImage:imgButtonUp forState:UIControlStateNormal];
	if (imgButtonDown)
		[button setBackgroundImage:imgButtonDown forState:UIControlStateHighlighted];
	[button setFrame:rect];
	[button addTarget:self action:@selector(onSignIn) forControlEvents:UIControlEventTouchUpInside];
	[scrollView addSubview:button];
	
	viewPosY += imageView.frame.origin.y + imageView.frame.size.height + 5;
   
   //New user
	//background frame
	image = roadmap_iphoneimage_load("Bubble_02");
	if (image) {
		rect = CGRectMake(([scrollView frame].size.width - [image size].width) /2, viewPosY,
                        [image size].width, [image size].height);
      imageView = [[UIImageView alloc] initWithFrame:rect];
      [imageView setImage:[image stretchableImageWithLeftCapWidth:15 topCapHeight:15]];
		[image release];
		[scrollView addSubview:imageView];
      [imageView release];
	}
	
	//text
	rect = imageView.frame;
	rect.size.width -= 15 + gSizeSigninBtn.width;
	rect.size.height = rect.size.height -20 - 10;
	rect.origin.x += 5;
	rect.origin.y += 5;
	label = [[iphoneLabel alloc] initWithFrame:rect];
	[label setText:[NSString stringWithUTF8String:roadmap_lang_get("NEW user")]];
	[label setTextColor:[UIColor colorWithRed:0.373f green:0.549f blue:0.094f alpha:1.0f]];
	[scrollView addSubview:label];
	[label release];
   
   //button
   rect = imageView.frame;
   rect.origin.x += rect.size.width - 5 - gSizeSigninBtn.width;
   rect.origin.y += (rect.size.height - gSizeSigninBtn.height - 20) /2;
   rect.size = gSizeSigninBtn;
   button = [UIButton buttonWithType:UIButtonTypeCustom];
	[button setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Get Started")] forState:UIControlStateNormal];
	if (imgButtonUp)
		[button setBackgroundImage:imgButtonUp forState:UIControlStateNormal];
	if (imgButtonDown)
		[button setBackgroundImage:imgButtonDown forState:UIControlStateHighlighted];
	[button setFrame:rect];
	[button addTarget:self action:@selector(onGetStarted) forControlEvents:UIControlEventTouchUpInside];
	[scrollView addSubview:button];
	
	viewPosY += imageView.frame.origin.y + imageView.frame.size.height;
   
	
	[scrollView setContentSize:CGSizeMake(scrollView.frame.size.width, viewPosY)];
	
	roadmap_main_push_view(self);
}


- (void)dealloc
{
	isNewExistingShown = FALSE;	
	[super dealloc];
}

@end



//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
@implementation LoginDialog
@synthesize dataArray;

- (id)initWithStyle:(UITableViewStyle)style
{
	int i;
	static int initialized = 0;
	
	self =  [super initWithStyle:style];

	dataArray = [[NSMutableArray arrayWithCapacity:1] retain];
	
	if (!initialized) {
		for (i=0; i < MAX_IDS; ++i) {
			id_callbacks[i] = NULL;
			id_actions[i] = NULL;
		}
		initialized = 1;
	}
	
	return self;
}

- (void) viewDidLoad
{
	UITableView *tableView = [self tableView];
	
	[tableView setBackgroundColor:[UIColor clearColor]];
   
   roadmap_main_reload_bg_image();
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
   return roadmap_main_should_rotate (interfaceOrientation);
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
   roadmap_device_event_notification( device_event_window_orientation_changed);
}


- (void) onClose
{
   roadmap_main_show_root(0);
}

- (void) onUnPwDone
{
   NSMutableArray *groupArray;
   iphoneCellEdit *editCell;
   
   groupArray = (NSMutableArray *) [dataArray objectAtIndex:0];
   editCell = (iphoneCellEdit *) [groupArray objectAtIndex:0];
   [editCell hideKeyboard];
   editCell = (iphoneCellEdit *) [groupArray objectAtIndex:1];
   [editCell hideKeyboard];
   
   roadmap_login_on_login (NULL, NULL);
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
   
   LoginDialogUserName[0] = 0;
   LoginDialogPassword[0] = 0;
   LoginDialogNickname[0] = 0;
   if (roadmap_config_get( &RT_CFG_PRM_NKNM_Var)) {
      strncpy_safe (LoginDialogNickname, roadmap_config_get( &RT_CFG_PRM_NKNM_Var), sizeof (LoginDialogNickname));
   }
	
	//first group
	groupArray = [NSMutableArray arrayWithCapacity:1];
	
	editCell = [[[iphoneCellEdit alloc] initWithFrame:CGRectZero reuseIdentifier:@"editCell"] autorelease];
	[editCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get("User name")]];
   if (!Realtime_is_random_user())
      if (roadmap_config_get( &RT_CFG_PRM_NAME_Var)) {
         strncpy_safe (LoginDialogUserName, roadmap_config_get( &RT_CFG_PRM_NAME_Var), sizeof (LoginDialogUserName));
         [editCell setText:[NSString stringWithUTF8String:LoginDialogUserName]];
      }
	[editCell setTag:ID_USERNAME];
	[editCell setDelegate:self];
	[groupArray addObject:editCell];
	
	editCell = [[[iphoneCellEdit alloc] initWithFrame:CGRectZero reuseIdentifier:@"editCell"] autorelease];
	[editCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get("Password")]];
   if (!Realtime_is_random_user())
      if (roadmap_config_get( &RT_CFG_PRM_PASSWORD_Var)) {
         strncpy_safe (LoginDialogPassword, roadmap_config_get( &RT_CFG_PRM_PASSWORD_Var), sizeof (LoginDialogPassword)); 
         [editCell setText:[NSString stringWithUTF8String:LoginDialogPassword]];
      }
	[editCell setTag:ID_PASSWORD];
	[editCell setDelegate:self];
	[editCell setPassword: YES];
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
   
   LoginDialogUserName[0] = 0;
   LoginDialogPassword[0] = 0;
   LoginDialogNickname[0] = 0;

   
	//first group
	groupArray = [NSMutableArray arrayWithCapacity:1];
	
	editCell = [[[iphoneCellEdit alloc] initWithFrame:CGRectZero reuseIdentifier:@"editCell"] autorelease];
	[editCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get("UserName")]];
   if (roadmap_config_get( &RT_CFG_PRM_NAME_Var)) {
      strncpy_safe (LoginDialogUserName, roadmap_config_get( &RT_CFG_PRM_NAME_Var), sizeof (LoginDialogUserName)); 
      [editCell setText:[NSString stringWithUTF8String:LoginDialogUserName]];
   }
	[editCell setTag:ID_USERNAME];
	[editCell setDelegate:self];
	[groupArray addObject:editCell];
	
	editCell = [[[iphoneCellEdit alloc] initWithFrame:CGRectZero reuseIdentifier:@"editCell"] autorelease];
	[editCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get("Password")]];
   if (roadmap_config_get( &RT_CFG_PRM_PASSWORD_Var)) {
      strncpy_safe (LoginDialogPassword, roadmap_config_get( &RT_CFG_PRM_PASSWORD_Var), sizeof (LoginDialogPassword));
      [editCell setText:[NSString stringWithUTF8String:LoginDialogPassword]];
   }
	[editCell setTag:ID_PASSWORD];
	[editCell setDelegate:self];
	[editCell setPassword: YES];
	[groupArray addObject:editCell];
	
	editCell = [[[iphoneCellEdit alloc] initWithFrame:CGRectZero reuseIdentifier:@"editCell"] autorelease];
	[editCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get("Nickname")]];
   if (roadmap_config_get( &RT_CFG_PRM_NKNM_Var)) {
      strncpy_safe (LoginDialogNickname, roadmap_config_get( &RT_CFG_PRM_NKNM_Var), sizeof (LoginDialogNickname));
      [editCell setText:[NSString stringWithUTF8String:LoginDialogNickname]];
   }
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
		//[actionCell setImage:img];
      actionCell.imageView.image = img;
		[img release];
	}
	[actionCell setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
	[actionCell setTag:ID_AVATAR];
	id_actions[ID_AVATAR] = "select_car";
	this_action =  roadmap_start_find_action (id_actions[ID_AVATAR]);
   actionCell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get 
                                (this_action->label_long)];
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
      callbackCell.imageView.image = img;
		[img release];
	}
	[callbackCell setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
	[callbackCell setTag:ID_TWITTER];
	id_callbacks[ID_TWITTER] = roadmap_twitter_setting_dialog;
   callbackCell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get 
                                  ("Twitter")];
	[groupArray addObject:callbackCell];
   
   //Foursquare]
   if (roadmap_foursquare_is_enabled()) {
      callbackCell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"actionCell"] autorelease];
      icon_name = "foursquare";
      for (cursor = roadmap_path_first ("skin");
           cursor != NULL;
           cursor = roadmap_path_next ("skin", cursor)){
         
         NSString *fileName = [NSString stringWithFormat:@"%s/%s.png", cursor, icon_name];
         img = [[UIImage alloc] initWithContentsOfFile: fileName];
         
         if (img) break; 
      }
      if (img) {
         callbackCell.imageView.image = img;
         [img release];
      }
      [callbackCell setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
      [callbackCell setTag:ID_FOURSQUARE];
      id_callbacks[ID_FOURSQUARE] = roadmap_foursquare_login_dialog;
      callbackCell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get 
                                     ("Foursquare")];
      [groupArray addObject:callbackCell];
   }
	
	[dataArray addObject:groupArray];
}


- (void) showLogin
{
	[self populateLoginData];

	[self setTitle:[NSString stringWithUTF8String:roadmap_lang_get(titleProfile)]];
   
   //set right button
	UINavigationItem *navItem = [self navigationItem];
   UIBarButtonItem *barButton = [[UIBarButtonItem alloc] initWithTitle:[NSString stringWithUTF8String:roadmap_lang_get("Close")]
                                                                 style:UIBarButtonItemStyleDone target:self action:@selector(onClose)];
   [navItem setRightBarButtonItem:barButton];
	
	roadmap_main_push_view (self);
}

- (void) showUnPw
{	
	[self populateUnPwData];

	[self setTitle:[NSString stringWithUTF8String:roadmap_lang_get(titleUnPw)]];
   
   //set right button
	UINavigationItem *navItem = [self navigationItem];
   UIBarButtonItem *barButton = [[UIBarButtonItem alloc] initWithTitle:[NSString stringWithUTF8String:roadmap_lang_get("Done")]
                                                                 style:UIBarButtonItemStyleDone target:self action:@selector(onUnPwDone)];
   [navItem setRightBarButtonItem:barButton];
	
	roadmap_main_push_view (self);
}

- (void)dealloc
{
	int success;
	if (isTwitterModified) {
		success = Realtime_TwitterConnect(roadmap_twitter_get_username(), roadmap_twitter_get_password());
		
		isTwitterModified = 0;
	}
	
	if ([[self title] compare:[NSString stringWithUTF8String:roadmap_lang_get(titleProfile)]] == NSOrderedSame) {
      roadmap_login_on_ok (NULL, NULL);
	}
   
   if ([[self title] compare:[NSString stringWithUTF8String:roadmap_lang_get(titleUnPw)]] == NSOrderedSame) {
      isUnPwShown = FALSE;
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
         if ([textField text])
            strncpy_safe (LoginDialogUserName, [[textField text] UTF8String], sizeof (LoginDialogUserName)); 
			break;
		case ID_PASSWORD:
         if ([textField text])
            strncpy_safe (LoginDialogPassword, [[textField text] UTF8String], sizeof (LoginDialogPassword)); 
			break;
      case ID_NICKNAME:
			if ([textField text])
            strncpy_safe (LoginDialogNickname, [[textField text] UTF8String], sizeof (LoginDialogNickname)); 
			break;
		default:
			break;
	}
   [textField resignFirstResponder];
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
	[textField resignFirstResponder];
	return NO;
}

- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string
{
   if (roadmap_keyboard_typing_locked(TRUE)) {
      [textField resignFirstResponder];
      return NO;
   } else
      return YES;
}


@end



//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

@implementation LoginUpdateDialog

- (void) onToggleUpdates: (id) sender
{
   UIButton *button = (UIButton *) sender;
   UIImage *image;
   
   if (button.tag == ID_CHECKED) {
      button.tag = ID_UNCHECKED;
      image = roadmap_iphoneimage_load("default_checkbox_off");
      if (image) {
         [button setBackgroundImage:image forState:UIControlStateNormal];
         [image release];
      }
      
   } else {
      button.tag = ID_CHECKED;
      image = roadmap_iphoneimage_load("default_checkbox_on");
      if (image) {
         [button setBackgroundImage:image forState:UIControlStateNormal];
         [image release];
      }      
   }
   
}

- (void) onNicknameNext
{
   int i;
   UIView *view;
   UITextField *textField;
   UIScrollView *scrollView = (UIScrollView *) self.view;
   const char *email = "";
   BOOL  bSendUpdates = FALSE;
   
   for (i = 0; i < [[scrollView subviews] count]; ++i) {
      view = [[scrollView subviews] objectAtIndex:i];
      switch (view.tag) {
         case ID_NICKNAME:
            textField = ((UITextField *)view);
            if (textField.text)
               strncpy_safe (LoginDialogNickname, [textField.text UTF8String], sizeof (LoginDialogUserName));
            [textField resignFirstResponder];
            break;
         case ID_EMAIL:
            textField = ((UITextField *)view);
            if (textField.text)
               email = [textField.text UTF8String];
            [textField resignFirstResponder];
            break;
         case ID_CHECKED:
            bSendUpdates = TRUE;
            break;
         case ID_UNCHECKED:
            bSendUpdates = FALSE;
            break;
         default:
            break;
      }
   }
   
   if (!roadmap_login_validate_email (email))
      return;
   
   if (isUpdate) {
      roadmap_login_on_update (LoginDialogUserName, LoginDialogPassword, email, bSendUpdates);
   } else {
      roadmap_login_on_create (LoginDialogUserName, LoginDialogPassword, email, bSendUpdates);
   }
   
   roadmap_welcome_wizard_set_first_time_no();
}

- (void) onLoginSkip
{
   roadmap_welcome_personalize_later_dialog();
}

- (void) onLoginNext
{
   UIView *view;
   UITextField *textField;
   UIScrollView *scrollView = (UIScrollView *) self.view;
   
   
   const char *confirmPassword = "";
   
   view = [scrollView viewWithTag: ID_USERNAME];
   if (view) {
      textField = ((UITextField *)view);
      if (textField.text)
         strncpy_safe (LoginDialogUserName, [textField.text UTF8String], sizeof (LoginDialogUserName));
      [textField resignFirstResponder];
   }
   
   view = [scrollView viewWithTag: ID_PASSWORD];
   if (view) {
      textField = ((UITextField *)view);
      if (textField.text)
         strncpy_safe (LoginDialogPassword, [textField.text UTF8String], sizeof (LoginDialogPassword));
      [textField resignFirstResponder];
   }
   
   view = [scrollView viewWithTag: ID_CONFIRM_PASSWORD];
   if (view) {
      textField = ((UITextField *)view);
      if (textField.text)
         confirmPassword = [textField.text UTF8String];
      [textField resignFirstResponder];
   }
   
   if (!roadmap_login_validate_username (LoginDialogUserName))
      return;
   
   if (!roadmap_login_validate_password (LoginDialogPassword, confirmPassword))
      return;
   
   LoginUpdateDialog *nicknameDialog = [[LoginUpdateDialog alloc] init];
   [nicknameDialog showNickname];
}

- (void)viewDidAppear:(BOOL)animated
{
	UIScrollView *scrollView = (UIScrollView *) self.view;
   UIView *view;
	CGRect rect = [scrollView frame];
	rect.size.height = roadmap_main_get_mainbox_height();
	[scrollView setFrame:rect];
   
   int i;
   for (i = 0; i < [[scrollView subviews] count]; ++i) {
      view = [[scrollView subviews] objectAtIndex:i];
      if ((view.tag == ID_USERNAME) ||
          (view.tag == ID_NICKNAME)/* ||
       (view.tag == ID_TWITTER_USERNAME)*/)
         [(UITextField *)view becomeFirstResponder];
   }
	
	[super viewDidAppear:animated];
}

////////////////////
// End Dialog
- (void)showEnd
{
	CGRect rect;
   CGFloat viewPosY = 5.0f;
	UIImage *image;
   UIImage *strechedImage;
	UIImageView *bgView;
	NSString *text;
	UILabel *label;
	UIButton *button;
	rect = [[UIScreen mainScreen] applicationFrame];
	rect.origin.x = 0;
	rect.origin.y = 0;
	rect.size.height = roadmap_main_get_mainbox_height();
	UIScrollView *scrollView = [[UIScrollView alloc] initWithFrame:rect];
	self.view = scrollView;
	[scrollView release]; // decrement retain count
	[scrollView setDelegate:self];
   UIImage *imgButtonUp;
   UIImage *imgButtonDown;
   
   image = roadmap_iphoneimage_load("button_up");
   if (image) {
      imgButtonUp = [image stretchableImageWithLeftCapWidth:10 topCapHeight:10];
      [image release];
   }
   image = roadmap_iphoneimage_load("button_down");
   if (image) {
      imgButtonDown = [image stretchableImageWithLeftCapWidth:10 topCapHeight:10];
      [image release];
   }
	
	//[self setTitle:[NSString stringWithUTF8String: roadmap_lang_get ("Personalize later")]];
	
	
	//set UI elements
	
	//background frame
	rect = CGRectMake(5, 5, scrollView.bounds.size.width - 10,
                     roadmap_main_get_mainbox_height() - 55);
	image = roadmap_iphoneimage_load("comments_alert");
	if (image) {
		strechedImage = [image stretchableImageWithLeftCapWidth:20 topCapHeight:20];
		bgView = [[UIImageView alloc] initWithImage:strechedImage];
		[image release];
		[bgView setFrame:rect];
		[scrollView addSubview:bgView];
		[bgView release];
	}
   
   
	//Way to go text
	text = [NSString stringWithUTF8String:
           roadmap_lang_get("Way to go!")];
   rect = bgView.frame;
   rect.size.width = bgView.bounds.size.width - 80;
   rect.size.height = TEXT_HEIGHT;
   rect.origin.y += viewPosY;
   rect.origin.x += 40;
	label = [[iphoneLabel alloc] initWithFrame:rect];
	[label setText:text];
	[label setTextAlignment:UITextAlignmentCenter];
   [label setFont:[UIFont boldSystemFontOfSize:18]];
   [label setTextColor:[UIColor colorWithRed:0.941f green:0.0f blue:0.376f alpha:1.0f]];
	[scrollView addSubview:label];
	[label release];
   viewPosY += label.bounds.size.height + 5;
   
   //Accound updated text
   text = [NSString stringWithUTF8String:
           roadmap_lang_get("Your account has been updated")];
   rect = bgView.frame;
   rect.size.width = bgView.bounds.size.width - 120;
   rect.size.height = TEXT_HEIGHT * 2;
   rect.origin.y += viewPosY;
   rect.origin.x += 60;
	label = [[iphoneLabel alloc] initWithFrame:rect];
	[label setText:text];
	[label setTextAlignment:UITextAlignmentCenter];
   [label setFont:[UIFont systemFontOfSize:16]];
   [label setNumberOfLines:2];
	[bgView addSubview:label];
	[label release];
   viewPosY += label.bounds.size.height + 5;
   
	//Close button
	button = [UIButton buttonWithType:UIButtonTypeCustom];
	[button setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Close")] forState:UIControlStateNormal];
	if (imgButtonUp)
		[button setBackgroundImage:imgButtonUp forState:UIControlStateNormal];
	if (imgButtonDown)
		[button setBackgroundImage:imgButtonDown forState:UIControlStateHighlighted];
   rect = bgView.frame;
   rect.origin.y += viewPosY;
   rect.origin.x += (bgView.bounds.size.width - gSizeNormalBtn.width) /2;
   rect.size = gSizeNormalBtn;
	[button setFrame:rect];
	[button addTarget:self action:@selector(onFinish) forControlEvents:UIControlEventTouchUpInside];
	[scrollView addSubview:button];
   
   viewPosY += button.bounds.size.height + 5;
	
   
   rect = bgView.frame;
   rect.size.height = viewPosY;
   bgView.frame = rect;
   
	[scrollView setContentSize:CGSizeMake(scrollView.frame.size.width, viewPosY)];
	
	
	roadmap_main_push_view(self);
}

////////////////////
// Twitter Dialog
- (void)showTwitter
{
	CGRect rect;
	UIImage *image;
	UIImageView *imageView;
   UIImage *strechedImage;
	UIImageView *bgView;
	UILabel *label;
	UITextField *textField;
   CGFloat viewPosY = 5.0f;
	rect = [[UIScreen mainScreen] applicationFrame];
	rect.origin.x = 0;
	rect.origin.y = 0;
	rect.size.height = roadmap_main_get_mainbox_height();
	UIScrollView *scrollView = [[UIScrollView alloc] initWithFrame:rect];
	self.view = scrollView;
	[scrollView release]; // decrement retain count
	[scrollView setDelegate:self];
	
	[self setTitle:[NSString stringWithUTF8String: roadmap_lang_get ("Enter twitter details")]];
   
   //set right button
	UINavigationItem *navItem = [self navigationItem];
   UIBarButtonItem *barButton = [[UIBarButtonItem alloc] initWithTitle:[NSString stringWithUTF8String:roadmap_lang_get("Next")]
                                                                 style:UIBarButtonItemStyleDone target:self action:@selector(onTwitterNext)];
	[navItem setRightBarButtonItem:barButton];
	
   
   //UI elements
	
   //background frame
	rect = CGRectMake(5, 5, scrollView.bounds.size.width - 10,
                     roadmap_main_get_mainbox_height() - 55);
	image = roadmap_iphoneimage_load("comments_alert");
	if (image) {
		strechedImage = [image stretchableImageWithLeftCapWidth:20 topCapHeight:20];
		bgView = [[UIImageView alloc] initWithImage:strechedImage];
		[image release];
		[bgView setFrame:rect];
		[scrollView addSubview:bgView];
		[bgView release];
	}
   
   //Twitter icon
   rect = bgView.frame;
   rect.origin.y += viewPosY;
   image = roadmap_iphoneimage_load("Tweeter-logo");
   if (image) {
      if (!roadmap_lang_rtl())
         rect.origin.x += 5;
      else
         rect.origin.x += bgView.bounds.size.width - 5 - image.size.width;
      rect.size = image.size;
      imageView = [[UIImageView alloc] initWithImage:image];
      [image release];
      [imageView setFrame:rect];
      [scrollView addSubview:imageView];
      [imageView release];
   }
	
	//Note text
   rect = bgView.frame;
   rect.origin.y += viewPosY;
   if (!roadmap_lang_rtl())
      rect.origin.x += 5 + imageView.frame.size.width + 10;
   else
      rect.origin.x += 5;
   rect.size.height = TEXT_HEIGHT;
   rect.size.width = bgView.bounds.size.width - imageView.frame.size.width - 20;
	label = [[iphoneLabel alloc] initWithFrame:rect];
	[label setText:[NSString stringWithUTF8String: roadmap_lang_get("Tweet your road reports")]];
	//[label setBackgroundColor:[UIColor clearColor]];
   [label setAdjustsFontSizeToFitWidth:YES];
	[scrollView addSubview:label];
	[label release];
   
   viewPosY += imageView.bounds.size.height + 5;
   
	
   
   //Username
   rect = bgView.frame;
   rect.origin.y += viewPosY;
   rect.origin.x += 5;
   rect.size.height = TEXT_HEIGHT;
   rect.size.width = (bgView.bounds.size.width - 20)/2;
	label = [[iphoneLabel alloc] initWithFrame:rect];
	[label setText:[NSString stringWithUTF8String: roadmap_lang_get("Username")]];
	[label setBackgroundColor:[UIColor clearColor]];
	[scrollView addSubview:label];
	[label release];
	
	rect.origin.x += rect.size.width + 10;
	rect.size.width = (bgView.bounds.size.width - 20)/2;
	textField = [[UITextField alloc] initWithFrame:rect];
	[textField setTag:ID_TWITTER_USERNAME];
	[textField setBorderStyle:UITextBorderStyleRoundedRect];
	[textField setClearButtonMode:UITextFieldViewModeWhileEditing];
	[textField setAutocorrectionType:UITextAutocorrectionTypeNo];
	[textField setAutocapitalizationType:UITextAutocapitalizationTypeNone];
	//[textField setReturnKeyType:UIReturnKeyDone];
	[textField setDelegate:self];
	[textField setPlaceholder:[NSString stringWithUTF8String:roadmap_lang_get("<skip>")]];
	[scrollView addSubview:textField];
	//[textField becomeFirstResponder];
	[textField release];
   
   viewPosY += TEXT_HEIGHT + 10;
	
	//Password
   rect = bgView.frame;
   rect.origin.y += viewPosY;
   rect.origin.x += 5;
   rect.size.height = TEXT_HEIGHT;
   rect.size.width = (bgView.bounds.size.width - 20)/2;
	label = [[iphoneLabel alloc] initWithFrame:rect];
	[label setText:[NSString stringWithUTF8String: roadmap_lang_get("Password")]];
	[label setBackgroundColor:[UIColor clearColor]];
	[scrollView addSubview:label];
	[label release];
	
	rect.origin.x += rect.size.width + 10;
	rect.size.width = (bgView.bounds.size.width - 20)/2;
	textField = [[UITextField alloc] initWithFrame:rect];
	[textField setTag:ID_TWITTER_PASSWORD];
	[textField setBorderStyle:UITextBorderStyleRoundedRect];
	[textField setClearButtonMode:UITextFieldViewModeWhileEditing];
	[textField setAutocorrectionType:UITextAutocorrectionTypeNo];
	[textField setAutocapitalizationType:UITextAutocapitalizationTypeNone];
	[textField setSecureTextEntry:YES];
	//[textField setReturnKeyType:UIReturnKeyDone];
	[textField setDelegate:self];
	[textField setPlaceholder:[NSString stringWithUTF8String:roadmap_lang_get("<skip>")]];
	[scrollView addSubview:textField];
	[textField release];
   
   viewPosY += TEXT_HEIGHT + 5;
   
   
   rect = bgView.frame;
   rect.size.height = viewPosY;
   bgView.frame = rect;
   
	[scrollView setContentSize:CGSizeMake(scrollView.frame.size.width, viewPosY)];
	
	roadmap_main_push_view(self);
}

////////////////////
// Nickname Dialog
- (void)showNickname
{
	CGRect rect;
	UIImage *image;
	UIImage *strechedImage;
	UIImageView *bgView;
	UILabel *label;
	UITextField *textField;
   UIButton *button;
   CGFloat viewPosY = 5.0f;
	rect = [[UIScreen mainScreen] applicationFrame];
	rect.origin.x = 0;
	rect.origin.y = 0;
	rect.size.height = roadmap_main_get_mainbox_height();
	UIScrollView *scrollView = [[UIScrollView alloc] initWithFrame:rect];
	self.view = scrollView;
	[scrollView release]; // decrement retain count
	[scrollView setDelegate:self];
	
	[self setTitle:[NSString stringWithUTF8String: roadmap_lang_get ("Additional Details")]];
   
   //set right button
	UINavigationItem *navItem = [self navigationItem];
   UIBarButtonItem *barButton = [[UIBarButtonItem alloc] initWithTitle:[NSString stringWithUTF8String:roadmap_lang_get("Next")]
                                                                 style:UIBarButtonItemStyleDone target:self action:@selector(onNicknameNext)];
	[navItem setRightBarButtonItem:barButton];
   
	LoginDialogNickname[0] = 0;
   
   
	//UI elements
	
   //background frame
	rect = CGRectMake(5, 5, scrollView.bounds.size.width - 10,
                     roadmap_main_get_mainbox_height() - 55);
	image = roadmap_iphoneimage_load("comments_alert");
	if (image) {
		strechedImage = [image stretchableImageWithLeftCapWidth:20 topCapHeight:20];
		bgView = [[UIImageView alloc] initWithImage:strechedImage];
		[image release];
		[bgView setFrame:rect];
		[scrollView addSubview:bgView];
		[bgView release];
	}
   
	//Nickname
   rect = bgView.frame;
   rect.origin.y += viewPosY;
   rect.origin.x += 5;
   rect.size.height = TEXT_HEIGHT;
   rect.size.width = (bgView.bounds.size.width - 20)/2;
	label = [[iphoneLabel alloc] initWithFrame:rect];
	[label setText:[NSString stringWithUTF8String: roadmap_lang_get("Nickname")]];
	[label setBackgroundColor:[UIColor clearColor]];
	[scrollView addSubview:label];
	[label release];
	
	rect.origin.x += rect.size.width + 10;
	rect.size.width = (bgView.bounds.size.width - 20)/2;
	textField = [[UITextField alloc] initWithFrame:rect];
	[textField setTag:ID_NICKNAME];
	[textField setBorderStyle:UITextBorderStyleRoundedRect];
	[textField setClearButtonMode:UITextFieldViewModeWhileEditing];
	[textField setAutocorrectionType:UITextAutocorrectionTypeNo];
	[textField setAutocapitalizationType:UITextAutocapitalizationTypeNone];
	[textField setDelegate:self];
	[scrollView addSubview:textField];
	[textField release];
   
   viewPosY += TEXT_HEIGHT + 10;
   
   //Email
   rect = bgView.frame;
   rect.origin.y += viewPosY;
   rect.origin.x += 5;
   rect.size.height = TEXT_HEIGHT;
   rect.size.width = (bgView.bounds.size.width - 20)/2;
	label = [[iphoneLabel alloc] initWithFrame:rect];
	[label setText:[NSString stringWithUTF8String: roadmap_lang_get("Email")]];
	[label setBackgroundColor:[UIColor clearColor]];
	[scrollView addSubview:label];
	[label release];
	
	rect.origin.x += rect.size.width + 10;
	rect.size.width = (bgView.bounds.size.width - 20)/2;
	textField = [[UITextField alloc] initWithFrame:rect];
	[textField setTag:ID_EMAIL];
	[textField setBorderStyle:UITextBorderStyleRoundedRect];
	[textField setClearButtonMode:UITextFieldViewModeWhileEditing];
	[textField setAutocorrectionType:UITextAutocorrectionTypeNo];
	[textField setAutocapitalizationType:UITextAutocapitalizationTypeNone];
	//[textField setReturnKeyType:UIReturnKeyDone];
   [textField setKeyboardType:UIKeyboardTypeEmailAddress];
	[textField setDelegate:self];
	//[textField setPlaceholder:[NSString stringWithUTF8String:roadmap_lang_get("<skip>")]];
	[scrollView addSubview:textField];
	//[textField becomeFirstResponder];
	[textField release];
   
   viewPosY += TEXT_HEIGHT + 10;
   
   //Send me updates
   button = [UIButton buttonWithType:UIButtonTypeCustom];
   image = roadmap_iphoneimage_load("default_checkbox_on");
	if (image) {
		[button setBackgroundImage:image forState:UIControlStateNormal];
      [image release];
   }
   button.tag = ID_CHECKED;
   rect = bgView.frame;
   rect.size = image.size;
   rect.origin.y += viewPosY;
   if (!roadmap_lang_rtl())
      rect.origin.x += 5;
   else
      rect.origin.x += bgView.bounds.size.width - rect.size.width - 5;
   [button setFrame:rect];
	[button addTarget:self action:@selector(onToggleUpdates:) forControlEvents:UIControlEventTouchUpInside];
	[scrollView addSubview:button];
   
   rect = bgView.frame;
   rect.origin.y += viewPosY + (button.frame.size.height - TEXT_HEIGHT)/2;
   if (!roadmap_lang_rtl())
      rect.origin.x += 5 + button.frame.size.width + 10;
   else
      rect.origin.x += 5;
   rect.size.height = TEXT_HEIGHT;
   rect.size.width = bgView.bounds.size.width - button.frame.size.width - 20;
	label = [[iphoneLabel alloc] initWithFrame:rect];
	[label setText:[NSString stringWithUTF8String: roadmap_lang_get("Send me updates")]];
	//[label setBackgroundColor:[UIColor clearColor]];
	[scrollView addSubview:label];
	[label release];
	
   
   viewPosY += TEXT_HEIGHT + 5;
   
   
   rect = bgView.frame;
   rect.size.height = viewPosY;
   bgView.frame = rect;
   
   
   
	[scrollView setContentSize:CGSizeMake(scrollView.frame.size.width, viewPosY)];
	
	roadmap_main_push_view(self);
}

////////////////////
// Login Dialog
- (void)showUpdateLogin
{
   const char* username = NULL;
	CGRect rect;
	UIImage *image;
   UIImage *strechedImage;
	UIImageView *bgView;
	UILabel *label;
	UITextField *textField;
   CGFloat viewPosY = 5.0f;
   UIButton *button;
   UIImage *imgButtonUp;
   UIImage *imgButtonDown;
	rect = [[UIScreen mainScreen] applicationFrame];
	rect.origin.x = 0;
	rect.origin.y = 0;
	rect.size.height = roadmap_main_get_mainbox_height();
	UIScrollView *scrollView = [[UIScrollView alloc] initWithFrame:rect];
	self.view = scrollView;
	[scrollView release]; // decrement retain count
	[scrollView setDelegate:self];
	
	[self setTitle:[NSString stringWithUTF8String: roadmap_lang_get ("Login details")]];
   
   image = roadmap_iphoneimage_load("button_up");
   if (image) {
      imgButtonUp = [image stretchableImageWithLeftCapWidth:10 topCapHeight:10];
      [image release];
   }
   image = roadmap_iphoneimage_load("button_down");
   if (image) {
      imgButtonDown = [image stretchableImageWithLeftCapWidth:10 topCapHeight:10];
      [image release];
   }
	
   
   username = RealTime_GetUserName();
   if ( username && *username && Realtime_IsLoggedIn() )
   {	// Update user
	   isUpdate = 1;
   }
   else
   {	// Create user
	   isUpdate = 0;
   }
   
   LoginDialogUserName[0] = 0;
   LoginDialogPassword[0] = 0;
   
	//set UI elements
	
   //background frame
	rect = CGRectMake(5, 5, scrollView.bounds.size.width - 10,
                     roadmap_main_get_mainbox_height() - 55);
	image = roadmap_iphoneimage_load("comments_alert");
	if (image) {
		strechedImage = [image stretchableImageWithLeftCapWidth:20 topCapHeight:20];
		bgView = [[UIImageView alloc] initWithImage:strechedImage];
		[image release];
		[bgView setFrame:rect];
		[scrollView addSubview:bgView];
		[bgView release];
	}
   
	//Username
   rect = bgView.frame;
   rect.origin.y += viewPosY;
   rect.origin.x += 5;
   rect.size.height = TEXT_HEIGHT;
   rect.size.width = (bgView.bounds.size.width - 20)/2;
	label = [[iphoneLabel alloc] initWithFrame:rect];
	[label setText:[NSString stringWithUTF8String: roadmap_lang_get("Username")]];
	[label setBackgroundColor:[UIColor clearColor]];
	[scrollView addSubview:label];
	[label release];
	
	rect.origin.x += rect.size.width + 10;
	rect.size.width = (bgView.bounds.size.width - 20)/2;
	textField = [[UITextField alloc] initWithFrame:rect];
	[textField setTag:ID_USERNAME];
	[textField setBorderStyle:UITextBorderStyleRoundedRect];
	[textField setClearButtonMode:UITextFieldViewModeWhileEditing];
	[textField setAutocorrectionType:UITextAutocorrectionTypeNo];
	[textField setAutocapitalizationType:UITextAutocapitalizationTypeNone];
	//[textField setReturnKeyType:UIReturnKeyDone];
	[textField setDelegate:self];
	//[textField setPlaceholder:[NSString stringWithUTF8String:roadmap_lang_get("<skip>")]];
	[scrollView addSubview:textField];
	//[textField becomeFirstResponder];
	[textField release];
   
   viewPosY += TEXT_HEIGHT + 10;
	
	//Password
   rect = bgView.frame;
   rect.origin.y += viewPosY;
   rect.origin.x += 5;
   rect.size.height = TEXT_HEIGHT;
   rect.size.width = (bgView.bounds.size.width - 20)/2;
	label = [[iphoneLabel alloc] initWithFrame:rect];
	[label setText:[NSString stringWithUTF8String: roadmap_lang_get("Password")]];
	[label setBackgroundColor:[UIColor clearColor]];
	[scrollView addSubview:label];
	[label release];
	
	rect.origin.x += rect.size.width + 10;
	rect.size.width = (bgView.bounds.size.width - 20)/2;
	textField = [[UITextField alloc] initWithFrame:rect];
	[textField setTag:ID_PASSWORD];
	[textField setBorderStyle:UITextBorderStyleRoundedRect];
	[textField setClearButtonMode:UITextFieldViewModeWhileEditing];
	[textField setAutocorrectionType:UITextAutocorrectionTypeNo];
	[textField setAutocapitalizationType:UITextAutocapitalizationTypeNone];
	[textField setSecureTextEntry:YES];
	//[textField setReturnKeyType:UIReturnKeyDone];
	[textField setDelegate:self];
	//[textField setPlaceholder:[NSString stringWithUTF8String:roadmap_lang_get("<skip>")]];
	[scrollView addSubview:textField];
	[textField release];
   
   viewPosY += TEXT_HEIGHT + 10;
   
   //Confirm password
   rect = bgView.frame;
   rect.origin.y += viewPosY;
   rect.origin.x += 5;
   rect.size.height = TEXT_HEIGHT;
   rect.size.width = (bgView.bounds.size.width - 20)/2;
	label = [[iphoneLabel alloc] initWithFrame:rect];
	[label setText:[NSString stringWithUTF8String: roadmap_lang_get("Confirm")]];
	[label setBackgroundColor:[UIColor clearColor]];
   //[label setNumberOfLines:2];
	[scrollView addSubview:label];
	[label release];
	
	rect.origin.x += rect.size.width + 10;
	rect.size.width = (bgView.bounds.size.width - 20)/2;
	textField = [[UITextField alloc] initWithFrame:rect];
	[textField setTag:ID_CONFIRM_PASSWORD];
	[textField setBorderStyle:UITextBorderStyleRoundedRect];
	[textField setClearButtonMode:UITextFieldViewModeWhileEditing];
	[textField setAutocorrectionType:UITextAutocorrectionTypeNo];
	[textField setAutocapitalizationType:UITextAutocapitalizationTypeNone];
	[textField setSecureTextEntry:YES];
	//[textField setReturnKeyType:UIReturnKeyDone];
	[textField setDelegate:self];
	//[textField setPlaceholder:[NSString stringWithUTF8String:roadmap_lang_get("<skip>")]];
	[scrollView addSubview:textField];
	[textField release];
   
   viewPosY += TEXT_HEIGHT + 10;
   
   //Next button
	button = [UIButton buttonWithType:UIButtonTypeCustom];
	[button setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Next")] forState:UIControlStateNormal];
	if (imgButtonUp)
		[button setBackgroundImage:imgButtonUp forState:UIControlStateNormal];
	if (imgButtonDown)
		[button setBackgroundImage:imgButtonDown forState:UIControlStateHighlighted];
   
   rect.origin.y = viewPosY;
   rect.origin.x = bgView.bounds.size.width/2 + 5;
   rect.size = gSizeNormalBtn;
	[button setFrame:rect];
	[button addTarget:self action:@selector(onLoginNext) forControlEvents:UIControlEventTouchUpInside];
	[scrollView addSubview:button];
	
	//Skip button
	button = [UIButton buttonWithType:UIButtonTypeCustom];
	[button setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Skip")] forState:UIControlStateNormal];
	if (imgButtonUp)
		[button setBackgroundImage:imgButtonUp forState:UIControlStateNormal];
	if (imgButtonDown)
		[button setBackgroundImage:imgButtonDown forState:UIControlStateHighlighted];
   
   rect.origin.y = viewPosY;
   rect.origin.x = bgView.bounds.size.width/2 - gSizeNormalBtn.width - 5;
   rect.size = gSizeNormalBtn;
	[button setFrame:rect];
	[button addTarget:self action:@selector(onLoginSkip) forControlEvents:UIControlEventTouchUpInside];
	[scrollView addSubview:button];
	
   viewPosY += button.bounds.size.height + 5;
   
   
   rect = bgView.frame;
   rect.size.height = viewPosY;
   bgView.frame = rect;
	
	[scrollView setContentSize:CGSizeMake(scrollView.frame.size.width, viewPosY)];
	
	roadmap_main_push_view(self);
}


- (void)dealloc
{
   
	[super dealloc];
}

//Text field delegate
- (void)textFieldDidEndEditing:(UITextField *)textField {
	switch ([textField tag]) {
      case ID_USERNAME:
         if ([textField text])
            sprintf(LoginDialogUserName, [[textField text] UTF8String]);
			break;
      case ID_PASSWORD:
         if ([textField text])
            sprintf(LoginDialogPassword, [[textField text] UTF8String]);
			break;
		case ID_NICKNAME:
         if ([textField text])
            sprintf(LoginDialogNickname, [[textField text] UTF8String]);
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
   
   [textField resignFirstResponder];
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
   [textField resignFirstResponder];
   
	return NO;
}

- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string
{
   if (roadmap_keyboard_typing_locked(TRUE)) {
      [textField resignFirstResponder];
      return NO;
   } else
      return YES;
}

@end

