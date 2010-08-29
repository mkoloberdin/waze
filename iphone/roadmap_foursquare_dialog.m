/* roadmap_twitter_dialog.m - iPhone twitter settings dialog
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
#include "roadmap_foursquare.h"
#include "roadmap_messagebox.h"
#include "roadmap_iphoneimage.h"
#include "roadmap_list_menu.h"
#include "ssd_progress_msg_dialog.h"
#include "roadmap_device_events.h"
#include "roadmap_foursquare_dialog.h"


static int isFoursquareModified  = 0;


enum IDs {
	ID_FOURSQUARE_USERNAME = 1,
	ID_FOURSQUARE_PASSWORD,
   ID_FOURSQUARE_TWEET_LOGIN,
   ID_FOURSQUARE_TWEET_BADGE,
   
   VIEW_TAG_BG_FRAME,
   VIEW_TAG_ICON,
   VIEW_TAG_MESSAGE,
   VIEW_TAG_ADDRESS,
   VIEW_TAG_POINTS,
   VIEW_TAG_MORE_DETAILS  
};

#define MAX_IDS 25
#define TEXT_HEIGHT 30.0f

#define     ROADMAP_FOURSQUARE_MAX_VENUE_COUNT        10

#define FOURSQUARE_ROW_HEIGHT   25
#define FOURSQUARE_X_MARG       10
#define FOURSQUARE_Y_MARG       10


static char gsFoursquareUsername[256];
static char gsFoursquarePassword[256];




/////////////////////////////////////////////////////////////////////////////////////
void roadmap_foursquare_checkedin_dialog(void) {
   roadmap_main_remove_periodic(roadmap_foursquare_checkedin_dialog);
   
   FoursquareCheckedDialog *dialog = [[FoursquareCheckedDialog alloc] init];
	[dialog showFoursquareCheckedin];
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_foursquare_login_dialog(void) {
   FoursquareLoginDialog *dialog = [[FoursquareLoginDialog alloc] initWithStyle:UITableViewStyleGrouped];
	[dialog showFoursquareLogin];
}


//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

@implementation FoursquareCheckedDialog


- (void) resizeViews
{
   UIScrollView *contentView = (UIScrollView *) self.view;
   UIView *view;
   CGRect bounds = contentView.bounds;
   CGRect rect;
   int yPos = FOURSQUARE_Y_MARG;
   int xPos = FOURSQUARE_X_MARG;
   
   //icon
   view = [contentView viewWithTag:VIEW_TAG_ICON];
   rect = view.frame;
   rect.origin = CGPointMake (xPos, yPos);
   view.frame = rect;
   
   yPos += view.bounds.size.height + FOURSQUARE_Y_MARG;
   xPos = FOURSQUARE_X_MARG;
   
   //message
   view = [contentView viewWithTag:VIEW_TAG_MESSAGE];
   rect = view.frame;
   rect.origin = CGPointMake (xPos, yPos);
   rect.size = CGSizeMake (bounds.size.width - xPos - FOURSQUARE_X_MARG, FOURSQUARE_ROW_HEIGHT);
   view.frame = rect;
   [view sizeToFit];
   
   yPos += view.bounds.size.height + FOURSQUARE_Y_MARG;
   xPos = FOURSQUARE_X_MARG;
   
   //address
   view = [contentView viewWithTag:VIEW_TAG_ADDRESS];
   rect = view.frame;
   rect.origin = CGPointMake (xPos, yPos);
   rect.size = CGSizeMake (bounds.size.width - xPos - FOURSQUARE_X_MARG, FOURSQUARE_ROW_HEIGHT);
   view.frame = rect;
   [view sizeToFit];
   
   yPos += view.bounds.size.height + FOURSQUARE_Y_MARG;
   xPos = FOURSQUARE_X_MARG;
   
   //points
   view = [contentView viewWithTag:VIEW_TAG_POINTS];
   rect = view.frame;
   rect.origin = CGPointMake (xPos, yPos);
   rect.size = CGSizeMake (bounds.size.width - xPos - FOURSQUARE_X_MARG, FOURSQUARE_ROW_HEIGHT);
   view.frame = rect;
   [view sizeToFit];
   
   yPos += view.bounds.size.height + FOURSQUARE_Y_MARG;
   xPos = FOURSQUARE_X_MARG;
   
   //more details
   view = [contentView viewWithTag:VIEW_TAG_MORE_DETAILS];;
   rect = view.frame;
   rect.origin = CGPointMake (xPos, yPos);
   rect.size = CGSizeMake (bounds.size.width - xPos - FOURSQUARE_X_MARG, FOURSQUARE_ROW_HEIGHT);
   view.frame = rect;
   [view sizeToFit];
   
   yPos += view.bounds.size.height + FOURSQUARE_Y_MARG;
   xPos = FOURSQUARE_X_MARG;
   
   
   // frame background
   view = [contentView viewWithTag:VIEW_TAG_BG_FRAME];
   rect.origin = CGPointMake (5, 5);
   rect.size = CGSizeMake (bounds.size.width - 10, yPos);
   view.frame = rect;
   
   
   contentView.contentSize = CGSizeMake (bounds.size.width, yPos + 10);
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
   return roadmap_main_should_rotate (interfaceOrientation);
}

- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation duration:(NSTimeInterval)duration {
   [self resizeViews];
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
   roadmap_device_event_notification( device_event_window_orientation_changed);
}

- (void)viewWillAppear:(BOOL)animated {
   [self resizeViews];
}

- (void) onClose
{
   roadmap_main_show_root(NO);
}


- (void) createView
{
   UIImage *image;
   UIImageView *imageView;
   iphoneLabel *label;

   char text[256];
   FoursquareCheckin checkInInfo;
   
	UIScrollView *containerView = [[UIScrollView alloc] initWithFrame:CGRectZero];
   containerView.alwaysBounceVertical = YES;
	self.view = containerView;
	[containerView release]; // decrement retain count
   
   roadmap_foursquare_get_checkin_info(&checkInInfo);
   
   
   // frame image
   image = roadmap_iphoneimage_load("comments_alert");
	if (image) {
		UIImage *strechedImage = [image stretchableImageWithLeftCapWidth:20 topCapHeight:20];
		imageView = [[UIImageView alloc] initWithImage:strechedImage];
		[image release];
      imageView.tag = VIEW_TAG_BG_FRAME;
		[containerView addSubview:imageView];
		[imageView release];
	}
   
   // icon
   image = roadmap_iphoneimage_load("foursquare_logo");
   if (image) {
      imageView = [[UIImageView alloc] initWithImage:image];
      [image release];
      imageView.tag = VIEW_TAG_ICON;
      [containerView addSubview:imageView];
		[imageView release];
   }
   
   // message
   label = [[iphoneLabel alloc] initWithFrame:CGRectZero];
   label.text = [NSString stringWithUTF8String:checkInInfo.sCheckinMessage];
   label.tag = VIEW_TAG_MESSAGE;
   label.numberOfLines = 0;
   label.autoresizesSubviews = YES;
   label.autoresizingMask = UIViewAutoresizingFlexibleBottomMargin;
   [containerView addSubview:label];
   [label release];
   
   // address
   label = [[iphoneLabel alloc] initWithFrame:CGRectZero];
   label.text = [NSString stringWithUTF8String:checkInInfo.sAddress];
   label.tag = VIEW_TAG_ADDRESS;
   label.numberOfLines = 0;
   label.autoresizesSubviews = YES;
   label.autoresizingMask = UIViewAutoresizingFlexibleBottomMargin;
   [containerView addSubview:label];
   [label release];
   
   // points
   snprintf(text, sizeof(text), "%s %s", roadmap_lang_get("Points:"), checkInInfo.sScorePoints);
   label = [[iphoneLabel alloc] initWithFrame:CGRectZero];
   label.text = [NSString stringWithUTF8String:text];
   label.tag = VIEW_TAG_POINTS;
   label.numberOfLines = 0;
   label.autoresizesSubviews = YES;
   label.autoresizingMask = UIViewAutoresizingFlexibleBottomMargin;
   [containerView addSubview:label];
   [label release];
   
   // more details
   label = [[iphoneLabel alloc] initWithFrame:CGRectZero];
   label.text = [NSString stringWithUTF8String:roadmap_lang_get("Full details on this check-in are available for you on foursquare.com.")];
   label.tag = VIEW_TAG_MORE_DETAILS;
   label.numberOfLines = 0;
   label.autoresizesSubviews = YES;
   label.autoresizingMask = UIViewAutoresizingFlexibleBottomMargin;
   [containerView addSubview:label];
   [label release];
}


- (void) showFoursquareCheckedin
{
	[self createView];
   
	[self setTitle:[NSString stringWithUTF8String:roadmap_lang_get(FOURSQUARE_TITLE)]];
   
   //set right button
	UINavigationItem *navItem = [self navigationItem];
   UIBarButtonItem *barButton = [[UIBarButtonItem alloc] initWithTitle:[NSString stringWithUTF8String:roadmap_lang_get("Close")]
                                                                 style:UIBarButtonItemStyleDone target:self action:@selector(onClose)];
   [navItem setRightBarButtonItem:barButton];
   [barButton release];
   
	
	roadmap_main_push_view (self);
}

- (void)dealloc
{
   
   
	[super dealloc];
}





@end


//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
@implementation FoursquareLoginDialog
@synthesize dataArray;
@synthesize headersArray;
@synthesize footersArray;

- (id)initWithStyle:(UITableViewStyle)style
{	
	self =  [super initWithStyle:style];

	dataArray = [[NSMutableArray arrayWithCapacity:1] retain];
   headersArray = [[NSMutableArray arrayWithCapacity:1] retain];
   footersArray = [[NSMutableArray arrayWithCapacity:1] retain];
   
   strncpy_safe(gsFoursquareUsername, roadmap_foursquare_get_username(), sizeof (gsFoursquareUsername));
   strncpy_safe(gsFoursquarePassword, roadmap_foursquare_get_password(), sizeof (gsFoursquarePassword));
	
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
      [(id)(self.tableView) setBackgroundView:nil];
   tableView.rowHeight = 54;
   
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


- (void) populateFoursquareLoginData
{
	NSMutableArray *groupArray = NULL;
   iphoneTableHeader *header = NULL;
   iphoneTableFooter *footer = NULL;
	iphoneCellEdit *editCell = NULL;
   iphoneCellSwitch *swCell = NULL;
   iphoneCell *cell = NULL;
   UIImage *image = NULL;
   char footer_txt[512];
	
   //group #1
	groupArray = [NSMutableArray arrayWithCapacity:1];
   
   header = [[iphoneTableHeader alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT)];
   [header setText:""];
   [headersArray addObject:header];
   [header release];
   
   //title
	cell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"cell_icon"] autorelease];
   cell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get("Account details")];
   image = roadmap_iphoneimage_load("foursquare_logo");
   if (image) {
      cell.imageView.image = image;
      [image release];
   }
   [groupArray addObject:cell];
   
   //status
	cell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"cell_text"] autorelease];
   if (roadmap_foursquare_logged_in())
      cell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get("Status: logged in")];
   else
      cell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get("Status: not logged in")];
   [groupArray addObject:cell];
   
   //username
	editCell = [[[iphoneCellEdit alloc] initWithFrame:CGRectZero reuseIdentifier:@"editCell"] autorelease];
	[editCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get("Email/phone")]];
	[editCell setText:[NSString stringWithUTF8String:roadmap_foursquare_get_username()]];
	[editCell setTag:ID_FOURSQUARE_USERNAME];
	[editCell setDelegate:self];
	[groupArray addObject:editCell];
	
   //password
	editCell = [[[iphoneCellEdit alloc] initWithFrame:CGRectZero reuseIdentifier:@"editCell"] autorelease];
	[editCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get("Password")]];
	[editCell setText:[NSString stringWithUTF8String:roadmap_foursquare_get_password()]];
	[editCell setTag:ID_FOURSQUARE_PASSWORD];
	[editCell setDelegate:self];
	[editCell setPassword: YES];
	[groupArray addObject:editCell];
   
   footer = [[iphoneTableFooter alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT)];
   [footer setText:""];
   [footersArray addObject:footer];
   [footer release];
   
   [dataArray addObject:groupArray];
   
   //group #2 - Twitter toggles
	groupArray = [NSMutableArray arrayWithCapacity:1];
   
   header = [[iphoneTableHeader alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT)];
   [header setText:"Automatically tweet to my followers that I:"];
   [headersArray addObject:header];
   [header release];
   
   //enable tweet login
   //switch
	swCell = [[[iphoneCellSwitch alloc] initWithFrame:CGRectZero reuseIdentifier:@"switchCell"] autorelease];
	[swCell setTag:ID_FOURSQUARE_TWEET_LOGIN];
	[swCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get 
                     ("Am checking out this integration")]];
	[swCell setDelegate:self];
   [groupArray addObject:swCell];
   
   //enable tweet badge unlock
   //switch
	swCell = [[[iphoneCellSwitch alloc] initWithFrame:CGRectZero reuseIdentifier:@"switchCell"] autorelease];
	[swCell setTag:ID_FOURSQUARE_TWEET_BADGE];
	[swCell setLabel:[NSString stringWithUTF8String:roadmap_lang_get 
                     ("Have unlocked the Roadwarrior Badge")]];
	[swCell setDelegate:self];
   [groupArray addObject:swCell];
   
   footer = [[iphoneTableFooter alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT)];
   [footer setText:""];
   [footersArray addObject:footer];
   [footer release];
   
   [dataArray addObject:groupArray];
   
   //group #3 - header only
	groupArray = [NSMutableArray arrayWithCapacity:1];
   
   header = [[iphoneTableHeader alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT)];
   footer_txt[0]=0;
   snprintf(footer_txt, sizeof(footer_txt),
            "%s\n\n%s\n%s\n%s %s",
            roadmap_lang_get("We've partnered with Foursquare to give you quick access to check-in to nearby venues."),
            roadmap_lang_get("What is Foursquare?"),
            roadmap_lang_get("It's a cool way to discover and promote cool places in your city and be rewarded for doing so."),
            roadmap_lang_get("Don't have an account? Sign up on:"),
            roadmap_lang_get("www.foursquare.com"));
   [header setText:footer_txt];
   [headersArray addObject:header];
   [header release];
   
   [footersArray addObject:footer];
	
	[dataArray addObject:groupArray];
}

- (void) showFoursquareLogin
{
	[self populateFoursquareLoginData];

	[self setTitle:[NSString stringWithUTF8String:roadmap_lang_get(FOURSQUARE_TITLE)]];
   
   //set right button
	UINavigationItem *navItem = [self navigationItem];
   UIBarButtonItem *barButton = [[UIBarButtonItem alloc] initWithTitle:[NSString stringWithUTF8String:roadmap_lang_get("Close")]
                                                                 style:UIBarButtonItemStyleDone target:self action:@selector(onClose)];
   [navItem setRightBarButtonItem:barButton];
   [barButton release];
   
	
	roadmap_main_push_view (self);
}

- (void)dealloc
{
	//int success;
	if (isFoursquareModified) {
      roadmap_foursquare_login(gsFoursquareUsername, gsFoursquarePassword);
		
		isFoursquareModified = 0;
	}
      
	
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
   
   int tag = [cell tag];
   
	switch (tag) {
		case ID_FOURSQUARE_TWEET_LOGIN:
			swCell = (iphoneCellSwitch *) cell;
         [swCell setState:roadmap_foursquare_is_tweet_login_enabled() animated:FALSE];
			break;
      case ID_FOURSQUARE_TWEET_BADGE:
         swCell = (iphoneCellSwitch *) cell;
         [swCell setState:roadmap_foursquare_is_tweet_badge_enabled() animated:FALSE];
			break;
      default:
			break;
	}

	return cell;
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
	UIView *view = [[textField superview] superview];
	int tag = [view tag];

	switch (tag) {
		case ID_FOURSQUARE_USERNAME:
			roadmap_foursquare_set_username([[textField text] UTF8String]);
         strncpy_safe(gsFoursquareUsername, [[textField text] UTF8String], sizeof (gsFoursquareUsername));
			isFoursquareModified = 1;
			break;
		case ID_FOURSQUARE_PASSWORD:
			roadmap_foursquare_set_password([[textField text] UTF8String]);
         strncpy_safe(gsFoursquarePassword, [[textField text] UTF8String], sizeof (gsFoursquarePassword));
			isFoursquareModified = 1;
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
   
	switch (tag) {
		case ID_FOURSQUARE_TWEET_LOGIN:
			if ([view getState])
				roadmap_foursquare_enable_tweet_login();
			else
				roadmap_foursquare_disable_tweet_login();
			break;
      case ID_FOURSQUARE_TWEET_BADGE:
			if ([view getState])
				roadmap_foursquare_enable_tweet_badge();
			else
				roadmap_foursquare_disable_tweet_badge();
			break;
		default:
			break;
	}
   
}



@end
