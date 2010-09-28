/* privacy_settings.m
 *
 * LICENSE:
 *
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



#include "roadmap_main.h"
#include "roadmap_iphonemain.h"
#include "roadmap_device_events.h"
#include "roadmap_lang.h"
#include "roadmap_res.h"
#include "roadmap_social.h"
#include "roadmap_checklist.h"
#include "Realtime/Realtime.h"
#include "Realtime/RealtimeDefs.h"
#include "Realtime/RealtimePrivacy.h"
#include "widgets/iphoneTableHeader.h"
#include "widgets/iphoneTableFooter.h"
#include "widgets/iphoneCell.h"
#include "widgets/iphoneCellSwitch.h"
#include "privacy_settings.h"


enum IDs {
   ID_WAZE_PRIVACY = 1,
   ID_FACEBOOK_SHOW_NAME,
   ID_FACEBOOK_SHOW_PICTURE,
   ID_FACEBOOK_SHOW_PROFILE,
   ID_TWITTER_SHOW_PROFILE
};


enum types {
   fb_name_privacy,
   fb_picture_privacy,
   fb_profile_privacy,
   tw_profile_privacy
};

#define MAX_IDS 25

extern RoadMapConfigDescriptor RT_CFG_PRM_VISGRP_Var;

#define NUM_PRIVACY  3
static const char *gsPrivacyLabels[NUM_PRIVACY];
#define NUM_PROFILE 2
static const char *gsProfileLabels[NUM_PROFILE];
static const char *gsProfileValues[NUM_PROFILE];
static RoadMapCallback id_callbacks[MAX_IDS];


static void init_labels(void) {
   static BOOL initialized = FALSE;
   
   if (initialized)
      return;
   
   gsPrivacyLabels[0] = strdup(roadmap_lang_get("Don't show"));
   gsPrivacyLabels[1] = strdup(roadmap_lang_get("To friends only"));
   gsPrivacyLabels[2] = strdup(roadmap_lang_get("To everyone"));
   
   gsProfileLabels[0] = strdup(roadmap_lang_get("Nickname"));
   gsProfileLabels[1] = strdup(roadmap_lang_get("Anonymous"));
   gsProfileValues[0] = RT_CFG_PRM_VISGRP_Nickname;
   gsProfileValues[1] = RT_CFG_PRM_VISGRP_Anonymous;
   
   
   initialized = TRUE;
}

static void fb_name_callback (int value, int group) {
   roadmap_facebook_set_show_name(value);
   
   roadmap_main_pop_view(YES);
}

static void fb_picture_callback (int value, int group) {
   roadmap_facebook_set_show_picture(value);
   
   roadmap_main_pop_view(YES);
}

static void fb_profile_callback (int value, int group) {
   roadmap_facebook_set_show_profile(value);
   
   roadmap_main_pop_view(YES);
}

static void tw_profile_callback (int value, int group) {
   roadmap_twitter_set_show_profile(value);
   
   roadmap_main_pop_view(YES);
}

static void show_privacy_options(int type) {
   NSMutableArray *dataArray = [NSMutableArray arrayWithCapacity:1];
	NSMutableArray *groupArray = NULL;
   NSMutableDictionary *dict = NULL;
   NSString *text;
   NSNumber *accessoryType = [NSNumber numberWithInt:UITableViewCellAccessoryCheckmark];
   RoadMapChecklist *privacyView;
   int i;
   int show_mode;
   
   switch (type) {
      case fb_name_privacy:
         show_mode = roadmap_facebook_get_show_name();
         break;
      case fb_picture_privacy:
         show_mode = roadmap_facebook_get_show_picture();
         break;
      case fb_profile_privacy:
         show_mode = roadmap_facebook_get_show_profile();
         break;
      case tw_profile_privacy:
         show_mode = roadmap_twitter_get_show_profile();
         break;
      default:
         break;
   }
      
   groupArray = [NSMutableArray arrayWithCapacity:1];
   for (i = 0; i < NUM_PRIVACY; ++i) {
      dict = [NSMutableDictionary dictionaryWithCapacity:1];
      text = [NSString stringWithUTF8String:roadmap_lang_get(gsPrivacyLabels[i])];
      [dict setValue:text forKey:@"text"];
      
      if (show_mode == i) {
         [dict setObject:accessoryType forKey:@"accessory"];
      }
      [dict setValue:[NSNumber numberWithInt:1] forKey:@"selectable"];
      [groupArray addObject:dict];
   }
   [dataArray addObject:groupArray];
   
   
   switch (type) {
      case fb_name_privacy:
         text = [NSString stringWithUTF8String:roadmap_lang_get ("Show name")];
         privacyView = [[RoadMapChecklist alloc] 
                        activateWithTitle:text andData:dataArray andHeaders:NULL
                        andCallback:fb_name_callback andHeight:60 andFlags:0];
         break;
      case fb_picture_privacy:
         text = [NSString stringWithUTF8String:roadmap_lang_get ("Show picture")];
         privacyView = [[RoadMapChecklist alloc] 
                        activateWithTitle:text andData:dataArray andHeaders:NULL
                        andCallback:fb_picture_callback andHeight:60 andFlags:0];
         break;
      case fb_profile_privacy:
         text = [NSString stringWithUTF8String:roadmap_lang_get ("Show profile")];
         privacyView = [[RoadMapChecklist alloc] 
                        activateWithTitle:text andData:dataArray andHeaders:NULL
                        andCallback:fb_profile_callback andHeight:60 andFlags:0];
         break;
      case tw_profile_privacy:
         text = [NSString stringWithUTF8String:roadmap_lang_get ("Show profile")];
         privacyView = [[RoadMapChecklist alloc] 
                        activateWithTitle:text andData:dataArray andHeaders:NULL
                        andCallback:tw_profile_callback andHeight:60 andFlags:0];
         break;
      default:
         break;
   }  
}

static void show_fb_name (void) {
   show_privacy_options(fb_name_privacy);
}

static void show_fb_picture (void) {
   show_privacy_options(fb_picture_privacy);   
}

static void show_fb_profile (void) {
   show_privacy_options(fb_profile_privacy);   
}

static void show_tw_profile (void) {
   show_privacy_options(tw_profile_privacy);   
}

static void profile_callback (int value, int group) {
   roadmap_config_set (&RT_CFG_PRM_VISGRP_Var, gsProfileValues[value]);
	roadmap_config_save(FALSE);
	
	OnSettingsChanged_VisabilityGroup();
   
   roadmap_main_pop_view(YES);
}

static void show_profile_options(void) {
   NSMutableArray *dataArray = [NSMutableArray arrayWithCapacity:1];
	NSMutableArray *groupArray = NULL;
   NSMutableDictionary *dict = NULL;
   NSString *text;
   NSNumber *accessoryType = [NSNumber numberWithInt:UITableViewCellAccessoryCheckmark];
   RoadMapChecklist *privacyView;
   int i;
   int show_mode;
   
   if (roadmap_config_match(&RT_CFG_PRM_VISGRP_Var, RT_CFG_PRM_VISGRP_Anonymous))
      show_mode = 1;
   else
      show_mode = 0;
   
   groupArray = [NSMutableArray arrayWithCapacity:1];
   for (i = 0; i < NUM_PROFILE; ++i) {
      dict = [NSMutableDictionary dictionaryWithCapacity:1];
      text = [NSString stringWithUTF8String:roadmap_lang_get(gsProfileLabels[i])];
      [dict setValue:text forKey:@"text"];
      
      if (show_mode == i) {
         [dict setObject:accessoryType forKey:@"accessory"];
      }
      [dict setValue:[NSNumber numberWithInt:1] forKey:@"selectable"];
      [groupArray addObject:dict];
   }
   [dataArray addObject:groupArray];
   
   
   text = [NSString stringWithUTF8String:roadmap_lang_get ("Show me as")];
   privacyView = [[RoadMapChecklist alloc] 
                  activateWithTitle:text andData:dataArray andHeaders:NULL
                  andCallback:profile_callback andHeight:60 andFlags:0];
}


void privacy_settings_show() {
   init_labels();
   
   PrivacyDialog *dialog = [[PrivacyDialog alloc] initWithStyle:UITableViewStyleGrouped];
	[dialog show];
}



//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
@implementation PrivacyDialog
@synthesize dataArray;
@synthesize headersArray;
@synthesize footersArray;

- (id)initWithStyle:(UITableViewStyle)style
{	
   int i;
   
	self =  [super initWithStyle:style];
   
	dataArray = [[NSMutableArray arrayWithCapacity:1] retain];
   headersArray = [[NSMutableArray arrayWithCapacity:1] retain];
   footersArray = [[NSMutableArray arrayWithCapacity:1] retain];
   
   isPrivacyModified = FALSE;
   
   for (i=0; i < MAX_IDS; ++i) {
      id_callbacks[i] = NULL;
   }
   
	return self;
}

- (void) refreshCells
{
   UITableView *tableView = [self tableView];
   iphoneCell *cell;
   iphoneTableFooter *footer = NULL;
   BOOL enable = TRUE;
   int i, j;
   NSMutableArray *groupArray;
   
   
   footer = [[iphoneTableFooter alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT)];
   if (roadmap_config_match(&RT_CFG_PRM_VISGRP_Var, RT_CFG_PRM_VISGRP_Anonymous)) {
      [footer setText:"You selected to be anonymous. Note that you can't ping, comment or chit chat while anonymous."];
      [footer layoutIfNeeded];
      enable = FALSE;
   } else {
      [footer setText:""];
   }
   [footersArray replaceObjectAtIndex:0 withObject:footer];
   [footer release];
   
   
   for (i = 0; i < [dataArray count]; i++) {
      groupArray = (NSMutableArray *)[dataArray objectAtIndex:i];
      for (j = 0; j < [groupArray count]; j++) {
         cell = (iphoneCell *)[groupArray objectAtIndex:j];
         switch (cell.tag) {
            case ID_WAZE_PRIVACY:
               if (roadmap_config_match(&RT_CFG_PRM_VISGRP_Var, RT_CFG_PRM_VISGRP_Anonymous)) {
                  cell.rightLabel.text = [NSString stringWithUTF8String:roadmap_lang_get ("Anonymous")];
               } else {
                  cell.rightLabel.text = [NSString stringWithUTF8String:roadmap_lang_get ("Nickname")];
               }
               break;
            case ID_FACEBOOK_SHOW_NAME:
               cell.rightLabel.text = [NSString stringWithUTF8String:gsPrivacyLabels[roadmap_facebook_get_show_name()]];
               [cell setEnableCell:enable];
               break;
            case ID_FACEBOOK_SHOW_PICTURE:
               cell.rightLabel.text = [NSString stringWithUTF8String:gsPrivacyLabels[roadmap_facebook_get_show_picture()]];
               [cell setEnableCell:enable];
               break;
            case ID_FACEBOOK_SHOW_PROFILE:
               cell.rightLabel.text = [NSString stringWithUTF8String:gsPrivacyLabels[roadmap_facebook_get_show_profile()]];
               [cell setEnableCell:enable];
               break;
            case ID_TWITTER_SHOW_PROFILE:
               cell.rightLabel.text = [NSString stringWithUTF8String:gsPrivacyLabels[roadmap_twitter_get_show_profile()]];
               [cell setEnableCell:enable];
               break;
            default:
               break;
         }
         
      }
   }
   
   
   [tableView reloadData];
}


- (void) viewDidLoad
{
   int i;
   iphoneTableHeader *header = NULL;
   iphoneTableFooter *footer = NULL;
	UITableView *tableView = [self tableView];
	
   roadmap_main_set_table_color(tableView);
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


- (void)viewWillAppear:(BOOL)animated
{
   [self refreshCells];
}

- (void)viewDidAppear:(BOOL)animated
{
   //TODO: only call this if not done in willAppear
   [self refreshCells];
}


- (void) onClose
{
   roadmap_main_show_root(0);
}


- (void) populatePrivacyData
{
	NSMutableArray *groupArray = NULL;
   iphoneTableHeader *header = NULL;
   iphoneTableFooter *footer = NULL;
   iphoneCell *callbackCell = NULL;
	
	
   //group #1
	groupArray = [NSMutableArray arrayWithCapacity:1];
   
   header = [[iphoneTableHeader alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT)];
   [header setText:""];
   [headersArray addObject:header];
   [header release];
   
   //waze privacy
   callbackCell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"actionCell"] autorelease];
   [callbackCell setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
   [callbackCell setTag:ID_WAZE_PRIVACY];
   id_callbacks[ID_WAZE_PRIVACY] = show_profile_options;
   callbackCell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get ("Show me as")];
   if (roadmap_config_match(&RT_CFG_PRM_VISGRP_Var, RT_CFG_PRM_VISGRP_Anonymous)) {
      callbackCell.rightLabel.text = [NSString stringWithUTF8String:roadmap_lang_get ("Anonymous")];
   } else {
      callbackCell.rightLabel.text = [NSString stringWithUTF8String:roadmap_lang_get ("Nickname")];
   }
   [groupArray addObject:callbackCell];   
   
   footer = [[iphoneTableFooter alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT)];
   [footer setText:""];
   [footersArray addObject:footer];
   [footer release];
	
	[dataArray addObject:groupArray];
   
   //Facebook details
   
   groupArray = [NSMutableArray arrayWithCapacity:1];
   
   header = [[iphoneTableHeader alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT)];
   [header setText:"Show Facebook details:"];
   [headersArray addObject:header];
   [header release];
   
   //show name
   callbackCell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"actionCell"] autorelease];
   [callbackCell setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
   [callbackCell setTag:ID_FACEBOOK_SHOW_NAME];
   id_callbacks[ID_FACEBOOK_SHOW_NAME] = show_fb_name;
   callbackCell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get ("My FB name")];
   callbackCell.rightLabel.text = [NSString stringWithUTF8String:gsPrivacyLabels[roadmap_facebook_get_show_name()]];
   [groupArray addObject:callbackCell];
   
   //show picture
   callbackCell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"actionCell"] autorelease];
   [callbackCell setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
   [callbackCell setTag:ID_FACEBOOK_SHOW_PICTURE];
   id_callbacks[ID_FACEBOOK_SHOW_PICTURE] = show_fb_picture;
   callbackCell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get ("My FB picture")];
   callbackCell.rightLabel.text = [NSString stringWithUTF8String:gsPrivacyLabels[roadmap_facebook_get_show_picture()]];
   [groupArray addObject:callbackCell];
   
   //show profile link
   callbackCell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"actionCell"] autorelease];
   [callbackCell setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
   [callbackCell setTag:ID_FACEBOOK_SHOW_PROFILE];
   id_callbacks[ID_FACEBOOK_SHOW_PROFILE] = show_fb_profile;
   callbackCell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get ("My profile link")];
   callbackCell.rightLabel.text = [NSString stringWithUTF8String:gsPrivacyLabels[roadmap_facebook_get_show_profile()]];
   [groupArray addObject:callbackCell];
   
   footer = [[iphoneTableFooter alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT)];
   [footer setText:""];
   [footersArray addObject:footer];
   [footer release];
   
   [dataArray addObject:groupArray];
   
   //group #2
   groupArray = [NSMutableArray arrayWithCapacity:1];
   
   //Twitter
   header = [[iphoneTableHeader alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT)];
   [header setText:"Show Twitter details:"];
   [headersArray addObject:header];
   [header release];
   
   //show profile link
   callbackCell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"actionCell"] autorelease];
   [callbackCell setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
   [callbackCell setTag:ID_TWITTER_SHOW_PROFILE];
   id_callbacks[ID_TWITTER_SHOW_PROFILE] = show_tw_profile;
   callbackCell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get ("My profile link")];
   callbackCell.rightLabel.text = [NSString stringWithUTF8String:gsPrivacyLabels[roadmap_twitter_get_show_profile()]];
   [groupArray addObject:callbackCell];
   
   footer = [[iphoneTableFooter alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT)];
   [footer setText:""];
   [footersArray addObject:footer];
   [footer release];
   
   [dataArray addObject:groupArray];
}

- (void) show
{   
	[self populatePrivacyData];
   
	[self setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Privacy")]];
   
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
   roadmap_social_send_permissions();

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
   UIImage *image = NULL;
   UIImageView *accessoryView = NULL;
   
   image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "v");
   if (image) {
      accessoryView = [[UIImageView alloc] initWithImage:image];
   }
   
   if (accessoryView)
      [accessoryView release];
   
	return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	int tag = [[tableView cellForRowAtIndexPath:indexPath] tag];
   
   if (roadmap_config_match(&RT_CFG_PRM_VISGRP_Var, RT_CFG_PRM_VISGRP_Anonymous) &&
       tag != ID_WAZE_PRIVACY)
      return;
   
   if (id_callbacks[tag]) {
      isPrivacyModified = TRUE;
		(*id_callbacks[tag])();
      return;
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

@end

          
          
          
          
          
          
          
          
          


#if 0
#include "roadmap.h"
#include "roadmap_config.h"
#include "roadmap_checklist.h"
#include "roadmap_lang.h"
#include "roadmap_path.h"
#include "privacy_settings.h"
#include "Realtime/RealtimePrivacy.h"
#include "Realtime/RealtimeDefs.h"
#include "Realtime/Realtime.h"
#include "roadmap_bar.h"
#include "widgets/iphoneTableHeader.h"

#define NUM_DRIVING 3
static const char *driving_label[NUM_DRIVING];
static const char *driving_value[NUM_DRIVING];
static const char *driving_icon[NUM_DRIVING];

#define NUM_REPORTING 2
static const char *reporting_label[NUM_REPORTING];
static const char *reporting_value[NUM_REPORTING];
static const char *reporting_icon[NUM_REPORTING];

#define NUM_GROUPS 3
static const char *header_label[NUM_GROUPS];

extern RoadMapConfigDescriptor RT_CFG_PRM_VISGRP_Var;
extern RoadMapConfigDescriptor RT_CFG_PRM_VISREP_Var;


void on_item_selected (int value, int group) {
	if (group == 1) {
		roadmap_config_set (&RT_CFG_PRM_VISGRP_Var, driving_value[value]);
	} else if (group == 2) {
		roadmap_config_set (&RT_CFG_PRM_VISREP_Var, reporting_value[value]);
	}
	
	roadmap_config_save(TRUE);
	
	OnSettingsChanged_VisabilityGroup();
	
	roadmap_bar_draw();
}

UIImage *privacy_load_skin_image (const char *name) {
	const char *cursor;
	UIImage *img;
	for (cursor = roadmap_path_first ("skin");
		 cursor != NULL;
		 cursor = roadmap_path_next ("skin", cursor)){
		
		NSString *fileName = [NSString stringWithFormat:@"%s/%s.png", cursor, name];
		
		img = [[UIImage alloc] initWithContentsOfFile: fileName];
		
		if (img) break; 
	}
	return (img);
}


void privacy_settings_show() {
	NSMutableArray *dataArray = [NSMutableArray arrayWithCapacity:1];
	NSMutableArray *groupArray = NULL;
   NSMutableArray *headersArray = [NSMutableArray arrayWithCapacity:1];
	NSMutableDictionary *dict = NULL;
	NSString *text = NULL;
	NSNumber *accessoryType = [NSNumber numberWithInt:UITableViewCellAccessoryCheckmark];
	UIImage *image = NULL;
   iphoneTableHeader *header;
	RoadMapChecklist *privacyView;
	int i;
	
	//Init
	if (!driving_value[0]) {
		header_label[0] = roadmap_lang_get ("Display my location on waze mobile and web maps as follows:");
 		header_label[1] = roadmap_lang_get ("When driving");
      header_label[2] = roadmap_lang_get ("When reporting");
      
		driving_value[0] = RT_CFG_PRM_VISGRP_Nickname;
		driving_value[1] = RT_CFG_PRM_VISGRP_Anonymous;
		driving_value[2] = RT_CFG_PRM_VISGRP_Invisible;
		driving_label[0] = roadmap_lang_get ("Nickname");
		driving_label[1] = roadmap_lang_get ("Anonymous");
		driving_label[2] = roadmap_lang_get ("Don't show me");
		driving_icon[0] = "privacy_nickname";
		driving_icon[1] = "privacy_anonymous";
		driving_icon[2] = "privacy_invisible";

		reporting_value[0] = RT_CFG_PRM_VISREP_Nickname;
		reporting_value[1] = RT_CFG_PRM_VISREP_Anonymous;
		reporting_label[0] = roadmap_lang_get ("Nickname");
		reporting_label[1] = roadmap_lang_get ("Anonymous");
		reporting_icon[0] = "On_map_nickname";
		reporting_icon[1] = "On_map_anonymous";
	}
	
	//title (group 0)
   text = @"     ";
	text = [text stringByAppendingString: [NSString stringWithUTF8String:header_label[0]]];
   header = [[iphoneTableHeader alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT) ];
   [header setText:header_label[0]];
   [headersArray addObject:header];
   [header release];
   
         //empty group (only header)
  	groupArray = [NSMutableArray arrayWithCapacity:0];
	[dataArray addObject:groupArray];
	
   
	//When driving (group 1)
   text = @"     ";
	text = [text stringByAppendingString: [NSString stringWithUTF8String:header_label[1]]];
   header = [[iphoneTableHeader alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT) ];
   [header setText:header_label[1]];
   [headersArray addObject:header];
   [header release];
   
	groupArray = [NSMutableArray arrayWithCapacity:NUM_DRIVING];
	for (i = 0; i < NUM_DRIVING; ++i) {
		dict = [NSMutableDictionary dictionaryWithCapacity:1];
		text = [NSString stringWithUTF8String:driving_label[i]];
		[dict setValue:text forKey:@"text"];
      if (roadmap_config_match(&RT_CFG_PRM_VISGRP_Var, driving_value[i])) {
         [dict setObject:accessoryType forKey:@"accessory"];
      }
      [dict setValue:[NSNumber numberWithInt:1] forKey:@"selectable"];
      image = privacy_load_skin_image(driving_icon[i]);
      if (image) {
         [dict setObject:image forKey:@"image"];
         [image release];
      }
		[groupArray addObject:dict];
	}
	[dataArray addObject:groupArray];
	
   
	//When reporting (group 2)
   text = @"     ";
	text = [text stringByAppendingString: [NSString stringWithUTF8String:header_label[2]]];
   header = [[iphoneTableHeader alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT) ];
   [header setText:header_label[2]];
   [headersArray addObject:header];
   [header release];
   
	groupArray = [NSMutableArray arrayWithCapacity:NUM_REPORTING];
	for (i = 0; i < NUM_REPORTING; ++i) {
		dict = [NSMutableDictionary dictionaryWithCapacity:1];
		text = [NSString stringWithUTF8String:reporting_label[i]];
		[dict setValue:text forKey:@"text"];
      if (roadmap_config_match(&RT_CFG_PRM_VISREP_Var, reporting_value[i])) {
         [dict setObject:accessoryType forKey:@"accessory"];
      }
      [dict setValue:[NSNumber numberWithInt:1] forKey:@"selectable"];
      image = privacy_load_skin_image(reporting_icon[i]);
      if (image) {
         [dict setObject:image forKey:@"image"];
         [image release];
      }
		[groupArray addObject:dict];
	}
	[dataArray addObject:groupArray];
	
	
	text = [NSString stringWithUTF8String:roadmap_lang_get (PRIVACY_TITLE)];
	
	privacyView = [[RoadMapChecklist alloc] 
                  activateWithTitle:text andData:dataArray andHeaders:headersArray
                  andCallback:on_item_selected andHeight:60 andFlags:0];
}
#endif //0