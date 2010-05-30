/* roadmap_social_dialog.m - iPhone social networks settings dialog
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
#include "widgets/iphoneTableHeader.h"
#include "widgets/iphoneTableFooter.h"
#include "roadmap_factory.h"
#include "roadmap_start.h"
#include "roadmap_social.h"
#include "roadmap_messagebox.h"
#include "roadmap_iphoneimage.h"
#include "ssd_progress_msg_dialog.h"
#include "roadmap_device_events.h"
#include "roadmap_browser.h"
#include "roadmap_social_dialog.h"

static const char*   titleTwitter = "Twitter";
static const char*   titleFacebook = "Facebook";


static int isTwitterModified  = 0;


enum IDs {
	ID_TWITTER_USERNAME = 1,
	ID_TWITTER_PASSWORD,
   ID_TWITTER_SEND_REPORTS,
   ID_TWITTER_DESTINATION_ENABLED,
   ID_TWITTER_DESTINATION_CITY,
   ID_TWITTER_DESTINATION_FULL,
   ID_TWITTER_SEND_MUNCHING,
   ID_FACEBOOK_CONNECT,
   ID_FACEBOOK_DISCONNECT
};

#define MAX_IDS 25
#define TEXT_HEIGHT 30.0f

static char gsTwitterUsername[256];
static char gsTwitterPassword[256];

static TwitterDialog *gs_FacebookDialog = NULL;


void roadmap_facebook_refresh_connection (void) {
   if (gs_FacebookDialog)
      [gs_FacebookDialog refreshConnection];
}

static void facebook_dialog_closing (void) {
   gs_FacebookDialog = NULL;
}

void roadmap_facebook_setting_dialog(void) {
   if (gs_FacebookDialog)
      return;
   
	gs_FacebookDialog = [[TwitterDialog alloc] initWithStyle:UITableViewStyleGrouped];
	[gs_FacebookDialog showFacebook];
}

void roadmap_twitter_setting_dialog(void) {
	TwitterDialog *dialog = [[TwitterDialog alloc] initWithStyle:UITableViewStyleGrouped];
	[dialog showTwitter];
}




//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
@implementation TwitterDialog
@synthesize dataArray;
@synthesize headersArray;
@synthesize footersArray;

- (id)initWithStyle:(UITableViewStyle)style
{	
	self =  [super initWithStyle:style];

	dataArray = [[NSMutableArray arrayWithCapacity:1] retain];
   headersArray = [[NSMutableArray arrayWithCapacity:1] retain];
   footersArray = [[NSMutableArray arrayWithCapacity:1] retain];
   
   strncpy_safe(gsTwitterUsername, roadmap_twitter_get_username(), sizeof (gsTwitterUsername));
   strncpy_safe(gsTwitterPassword, roadmap_twitter_get_password(), sizeof (gsTwitterPassword));
	
	return self;
}

- (void) viewDidLoad
{
   int i;
   iphoneTableHeader *header = NULL;
   iphoneTableFooter *footer = NULL;
	UITableView *tableView = [self tableView];
	
   [tableView setBackgroundColor:roadmap_main_table_color()];
   if ([UITableView instancesRespondToSelector:@selector(setBackgroundView:)])
      [self.tableView setBackgroundView:nil];
   tableView.rowHeight = 50;
   
   if (headersArray) {
      for (i = 0; i < [headersArray count]; ++i) {
         header = [headersArray objectAtIndex:i];
         [header layoutIfNeeded];
      }
   }
   
   if (footersArray) {
      for (i = 0; i < [footersArray count]; ++i) {
         footer = [footersArray objectAtIndex:i];
         [footer layoutIfNeeded];
      }
   }
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

- (void) showDestination: (BOOL) reloadTable {
   UITableView *tableView = [self tableView];

   NSMutableArray *groupArray = [dataArray objectAtIndex:2];
   
   if ([groupArray count] > 1)
      return;
   
   iphoneCell *cell = NULL;
   cell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"cell"] autorelease];
   cell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get("City & state only")];
   [cell setTag:ID_TWITTER_DESTINATION_CITY];
   [groupArray addObject:cell];
   
   cell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"cell"] autorelease];
   cell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get("House #, Street, City, State")];
   [cell setTag:ID_TWITTER_DESTINATION_FULL];
   [groupArray addObject:cell];
   
   if (reloadTable) {
      NSArray *indexPaths = [NSArray arrayWithObjects:
                             [NSIndexPath indexPathForRow:1 inSection:2],
                             [NSIndexPath indexPathForRow:2 inSection:2],
                             nil];
      [tableView beginUpdates];
      [tableView insertRowsAtIndexPaths:indexPaths withRowAnimation:UITableViewRowAnimationBottom];
      [tableView endUpdates];
   }
}

- (void) hideDestination: (BOOL) reloadTable {
   UITableView *tableView = [self tableView];
   
   NSMutableArray *groupArray = [dataArray objectAtIndex:2];
   
   if ([groupArray count] < 3)
      return;
   
   [groupArray removeLastObject];
   [groupArray removeLastObject];
   
   if (reloadTable) {
      NSArray *indexPaths = [NSArray arrayWithObjects:
                             [NSIndexPath indexPathForRow:1 inSection:2],
                             [NSIndexPath indexPathForRow:2 inSection:2],
                             nil];
      [tableView beginUpdates];
      [tableView deleteRowsAtIndexPaths:indexPaths withRowAnimation:UITableViewRowAnimationTop];
      [tableView endUpdates];
   }
}

- (void) populateTwitterData
{
	NSMutableArray *groupArray = NULL;
   iphoneTableHeader *header = NULL;
   iphoneTableFooter *footer = NULL;
	iphoneCellEdit *editCell = NULL;
	iphoneCellSwitch *swCell = NULL;
   iphoneCell *cell = NULL;
   UIImage *image = NULL;
	
	
   //group #1
	groupArray = [NSMutableArray arrayWithCapacity:1];
   
   header = [[iphoneTableHeader alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT)];
   [header setText:""];
   [headersArray addObject:header];
   [header release];
   
   //title
   if (isTwitter) {
      cell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"cell_icon"] autorelease];
      
      cell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get("Account details")];
      image = roadmap_iphoneimage_load("Tweeter-logo");
      if (image) {
         cell.imageView.image = image;
         [image release];
      }
      [groupArray addObject:cell];
   }
   
   //status
	cell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"cell_text"] autorelease];
   if (isTwitter && roadmap_twitter_logged_in() ||
       !isTwitter && roadmap_facebook_logged_in())
      cell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get("Status: logged in")];
   else
      cell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get("Status: not logged in")];
   if (!isTwitter) {
      if (roadmap_facebook_logged_in()) {
         image = roadmap_iphoneimage_load("facebook_disconnect");
         [cell setTag:ID_FACEBOOK_DISCONNECT];
      } else {
         image = roadmap_iphoneimage_load("facebook_connect");
         [cell setTag:ID_FACEBOOK_CONNECT];
      }
      if (image) {
         cell.imageView.image = image;
         [image release];
      }
   }
   [groupArray addObject:cell];
   
   if (isTwitter) {
      //username
      editCell = [[[iphoneCellEdit alloc] initWithFrame:CGRectZero reuseIdentifier:@"editCell"] autorelease];
      [editCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get("UserName")]];
      [editCell setText:[NSString stringWithUTF8String:roadmap_twitter_get_username()]];
      [editCell setTag:ID_TWITTER_USERNAME];
      [editCell setDelegate:self];
      [groupArray addObject:editCell];
      
      //password
      editCell = [[[iphoneCellEdit alloc] initWithFrame:CGRectZero reuseIdentifier:@"editCell"] autorelease];
      [editCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get("Password")]];
      [editCell setText:[NSString stringWithUTF8String:roadmap_twitter_get_password()]];
      [editCell setTag:ID_TWITTER_PASSWORD];
      [editCell setDelegate:self];
      [editCell setPassword: YES];
      [groupArray addObject:editCell];
   }
   
   footer = [[iphoneTableFooter alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT)];
   [footer setText:""];
   [footersArray addObject:footer];
   [footer release];
	
	[dataArray addObject:groupArray];
   
   //group #2
   groupArray = [NSMutableArray arrayWithCapacity:1];
   
   header = [[iphoneTableHeader alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT)];
   if (isTwitter)
      [header setText:"Automatically tweet to my followers:"];
   else
      [header setText:"Automatically post to Facebook:"];
   [headersArray addObject:header];
   [header release];
	
   //send reports
	swCell = [[[iphoneCellSwitch alloc] initWithFrame:CGRectZero reuseIdentifier:@"switchCell"] autorelease];
	[swCell setTag:ID_TWITTER_SEND_REPORTS];
	[swCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get 
                     ("My road reports")]];
	[swCell setDelegate:self];
   [groupArray addObject:swCell];
   
   footer = [[iphoneTableFooter alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT)];
   [footer setText:"e.g:  Just reported a traffic jam on Geary St. SF, CA using @waze Social GPS."];
   [footersArray addObject:footer];
   [footer release];
   
   [dataArray addObject:groupArray];
   
   //group #3
   groupArray = [NSMutableArray arrayWithCapacity:1];
   
   header = [[iphoneTableHeader alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT)];
   [header setText:""];
   [headersArray addObject:header];
   [header release];
	
   //send destination
	swCell = [[[iphoneCellSwitch alloc] initWithFrame:CGRectZero reuseIdentifier:@"switchCell"] autorelease];
	[swCell setTag:ID_TWITTER_DESTINATION_ENABLED];
	[swCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get 
                     ("My destination and ETA")]];
	[swCell setDelegate:self];
   [groupArray addObject:swCell];
   
   footer = [[iphoneTableFooter alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT)];
   [footer setText:"e.g:  Driving to Greary St. SF, using @waze social GPS. ETA 2:32pm."];
   [footersArray addObject:footer];
   [footer release];
   
   [dataArray addObject:groupArray];
   
   //send destination
   
   //group #4
   if (roadmap_twitter_is_show_munching()) {
      groupArray = [NSMutableArray arrayWithCapacity:1];
      
      header = [[iphoneTableHeader alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT)];
      [header setText:""];
      [headersArray addObject:header];
      [header release];
      
      //send munching
      swCell = [[[iphoneCellSwitch alloc] initWithFrame:CGRectZero reuseIdentifier:@"switchCell"] autorelease];
      [swCell setTag:ID_TWITTER_SEND_MUNCHING];
      [swCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get 
                        ("My road munching")]];
      [swCell setDelegate:self];
      [groupArray addObject:swCell];
      
      footer = [[iphoneTableFooter alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT)];
      [footer setText:"e.g:  Just munched a 'waze road goodie' worth 200 points on Geary St. SF driving with @waze social GPS"];
      [footersArray addObject:footer];
      [footer release];
      
      [dataArray addObject:groupArray];
   }
   
   if ((isTwitter && roadmap_twitter_destination_mode() > 0) ||
       (!isTwitter && roadmap_facebook_destination_mode() > 0))
      [self showDestination:FALSE];
}

- (void) showTwitter
{
   isTwitter = TRUE;
   
	[self populateTwitterData];

	[self setTitle:[NSString stringWithUTF8String:roadmap_lang_get(titleTwitter)]];
   
   //set right button
	UINavigationItem *navItem = [self navigationItem];
   UIBarButtonItem *barButton = [[UIBarButtonItem alloc] initWithTitle:[NSString stringWithUTF8String:roadmap_lang_get("Close")]
                                                                 style:UIBarButtonItemStyleDone target:self action:@selector(onClose)];
   [navItem setRightBarButtonItem:barButton];
   [barButton release];
   

	roadmap_main_push_view (self);
}

- (void) refreshConnection
{
   if (isTwitter)
      return;
   
   UITableView *tableView = [self tableView];
   iphoneCell *cell = (iphoneCell *)[tableView viewWithTag:ID_FACEBOOK_CONNECT];
   if (!cell)
      cell = (iphoneCell *)[tableView viewWithTag:ID_FACEBOOK_DISCONNECT];
   UIImage *image;
   
   if (roadmap_facebook_logged_in()) {
      image = roadmap_iphoneimage_load("facebook_disconnect");
      [cell setTag:ID_FACEBOOK_DISCONNECT];
      cell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get("Status: logged in")];
   } else {
      image = roadmap_iphoneimage_load("facebook_connect");
      [cell setTag:ID_FACEBOOK_CONNECT];
      cell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get("Status: not logged in")];
   }
   
   cell.imageView.image = image;
   [cell layoutSubviews];
   if (image)
      [image release]; 
}

- (void) showFacebook
{
   isTwitter = FALSE;
   
	[self populateTwitterData];
   
	[self setTitle:[NSString stringWithUTF8String:roadmap_lang_get(titleFacebook)]];
   
   //set right button
	UINavigationItem *navItem = [self navigationItem];
   UIBarButtonItem *barButton = [[UIBarButtonItem alloc] initWithTitle:[NSString stringWithUTF8String:roadmap_lang_get("Close")]
                                                                 style:UIBarButtonItemStyleDone target:self action:@selector(onClose)];
   [navItem setRightBarButtonItem:barButton];
   [barButton release];
   
	
	roadmap_main_push_view (self);
   
   roadmap_facebook_check_login();
}

- (void)dealloc
{
	int success;
	if (isTwitterModified) {
		success = Realtime_TwitterConnect(gsTwitterUsername, gsTwitterPassword);
      if (success) //TODO: add error message if network error
         roadmap_twitter_set_logged_in (TRUE);
		isTwitterModified = 0;
	}
   
   if (!isTwitter)
      facebook_dialog_closing();
      
	
	[dataArray release];
   [headersArray release];
   [footersArray release];
	
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
   iphoneCellSwitch *swCell;
   UIImage *image = NULL;
   UIImageView *accessoryView = NULL;
   
   image = roadmap_iphoneimage_load("v");
   if (image) {
      accessoryView = [[UIImageView alloc] initWithImage:image];
      [image release];
   }

   int tag = [cell tag];
   
   
   BOOL  is_sending_enabled;
   int   destination_mode;
   BOOL  is_munching_enabled;
   
   if (isTwitter) {
      is_sending_enabled = roadmap_twitter_is_sending_enabled();
      destination_mode = roadmap_twitter_destination_mode();
      is_munching_enabled = roadmap_twitter_is_munching_enabled();
   } else {
      is_sending_enabled = roadmap_facebook_is_sending_enabled();
      destination_mode = roadmap_facebook_destination_mode();
      is_munching_enabled = roadmap_facebook_is_munching_enabled();
   }
   
	switch (tag) {
		case ID_TWITTER_SEND_REPORTS:
			swCell = (iphoneCellSwitch *) cell;
         [swCell setState:is_sending_enabled animated:FALSE];
			break;
      case ID_TWITTER_DESTINATION_ENABLED:
         swCell = (iphoneCellSwitch *) cell;
         [swCell setState:(destination_mode != ROADMAP_SOCIAL_DESTINATION_MODE_DISABLED) animated:FALSE];
			break;
      case ID_TWITTER_DESTINATION_CITY:
         if (destination_mode == ROADMAP_SOCIAL_DESTINATION_MODE_CITY) {
            if (accessoryView) {
					cell.accessoryView = accessoryView;
				} else {
               cell.accessoryType = UITableViewCellAccessoryCheckmark;
            }
         } else {
            cell.accessoryType = UITableViewCellAccessoryNone;
				cell.accessoryView = NULL;
         }
         break;
      case ID_TWITTER_DESTINATION_FULL:
         if (destination_mode == ROADMAP_SOCIAL_DESTINATION_MODE_FULL) {
            if (accessoryView) {
					cell.accessoryView = accessoryView;
				} else {
               cell.accessoryType = UITableViewCellAccessoryCheckmark;
            }
         } else {
            cell.accessoryType = UITableViewCellAccessoryNone;
				cell.accessoryView = NULL;
         }
         break;
      case ID_TWITTER_SEND_MUNCHING:
         swCell = (iphoneCellSwitch *) cell;
         [swCell setState:is_munching_enabled animated:FALSE];
		default:
			break;
	}
   
   if (accessoryView)
      [accessoryView release];

	return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	int tag = [[tableView cellForRowAtIndexPath:indexPath] tag];
   
	switch (tag) {
      case ID_TWITTER_DESTINATION_CITY:
         if (isTwitter)
            roadmap_twitter_set_destination_mode(ROADMAP_SOCIAL_DESTINATION_MODE_CITY);
         else
            roadmap_facebook_set_destination_mode(ROADMAP_SOCIAL_DESTINATION_MODE_CITY);
         [tableView reloadData];
         break;
      case ID_TWITTER_DESTINATION_FULL:
         if (isTwitter)
            roadmap_twitter_set_destination_mode(ROADMAP_SOCIAL_DESTINATION_MODE_FULL);
         else
            roadmap_facebook_set_destination_mode(ROADMAP_SOCIAL_DESTINATION_MODE_FULL);
         [tableView reloadData];
         break;
      case ID_FACEBOOK_CONNECT:
         roadmap_facebook_connect();
         break;
      case ID_FACEBOOK_DISCONNECT:
         roadmap_facebook_disconnect();
         break;
	}
}

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {
   return [headersArray objectAtIndex:section];
}

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section {
   if (!headersArray)
      return 0;
   
   iphoneTableHeader *header = [headersArray objectAtIndex:section];
   
   [header layoutIfNeeded];
   
   if ([[header getText] isEqualToString:@""])
      return 0;
   else
      return header.bounds.size.height;      
}

- (UIView *)tableView:(UITableView *)tableView viewForFooterInSection:(NSInteger)section {
   return [footersArray objectAtIndex:section];
}

- (CGFloat)tableView:(UITableView *)tableView heightForFooterInSection:(NSInteger)section {
   if (!footersArray)
      return 0;
   
   iphoneTableFooter *footer = [footersArray objectAtIndex:section];
   
   [footer layoutIfNeeded];
   
   if ([[footer getText] isEqualToString:@""])
      return 0;
   else
      return footer.bounds.size.height;  
}


//////////////////////////////////////////////////////////
//Text field delegate
- (void)textFieldDidEndEditing:(UITextField *)textField {
   UITableView *tableView = [self tableView];
	UIView *view = [[textField superview] superview];
	int tag = [view tag];

	switch (tag) {
		case ID_TWITTER_USERNAME:
			roadmap_twitter_set_username([[textField text] UTF8String]);
         strncpy_safe(gsTwitterUsername, [[textField text] UTF8String], sizeof (gsTwitterUsername));
         roadmap_twitter_enable_sending();
         [tableView reloadData];
			isTwitterModified = 1;
			break;
		case ID_TWITTER_PASSWORD:
			roadmap_twitter_set_password([[textField text] UTF8String]);
         strncpy_safe(gsTwitterPassword, [[textField text] UTF8String], sizeof (gsTwitterPassword));
         roadmap_twitter_enable_sending();
         [tableView reloadData];
			isTwitterModified = 1;
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


//////////////////////////////////////////////////////////
//Switch delegate
- (void) switchToggle:(id)switchView {
	static const char *enabled[2];
	
	if (!enabled[0]) {
		enabled[0] = "Enabled";
		enabled[1] = "Disabled";
	}
	
	iphoneCellSwitch *view = (iphoneCellSwitch*)[[switchView superview] superview];
	int tag = [view tag];
   
   if (isTwitter) {
      switch (tag) {
         case ID_TWITTER_SEND_REPORTS:
            if ([view getState])
               roadmap_twitter_enable_sending();
            else
               roadmap_twitter_disable_sending();
            break;
         case ID_TWITTER_DESTINATION_ENABLED:
            if ([view getState]) {
               roadmap_twitter_set_destination_mode(ROADMAP_SOCIAL_DESTINATION_MODE_CITY);
               [self showDestination:TRUE];
            } else {
               roadmap_twitter_set_destination_mode(ROADMAP_SOCIAL_DESTINATION_MODE_DISABLED);
               [self hideDestination:TRUE];
            }
            break;
         case ID_TWITTER_SEND_MUNCHING:
            if ([view getState])
               roadmap_twitter_enable_munching();
            else
               roadmap_twitter_disable_munching();
            break;
         default:
            break;
      }
   } else { //Facebook
      switch (tag) {
         case ID_TWITTER_SEND_REPORTS:
            if ([view getState])
               roadmap_facebook_enable_sending();
            else
               roadmap_facebook_disable_sending();
            break;
         case ID_TWITTER_DESTINATION_ENABLED:
            if ([view getState]) {
               roadmap_facebook_set_destination_mode(ROADMAP_SOCIAL_DESTINATION_MODE_CITY);
               [self showDestination:TRUE];
            } else {
               roadmap_facebook_set_destination_mode(ROADMAP_SOCIAL_DESTINATION_MODE_DISABLED);
               [self hideDestination:TRUE];
            }
            break;
         case ID_TWITTER_SEND_MUNCHING:
            if ([view getState])
               roadmap_facebook_enable_munching();
            else
               roadmap_facebook_disable_munching();
            break;
         default:
            break;
      }
   }

}


@end
