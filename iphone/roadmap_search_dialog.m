/* roadmap_search_dialog.m
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

#include "roadmap.h"
#include "roadmap_main.h"
#include "roadmap_iphonemain.h"
#include "roadmap_device_events.h"
#include "roadmap_config.h"
#include "roadmap_lang.h"
#include "roadmap_start.h"
#include "widgets/iphoneCell.h"
#include "widgets/iphoneWidgets.h"
#include "widgets/iphoneTableHeader.h"
#include "roadmap_res.h"
#include "roadmap_list_menu.h"
#include "local_search.h"
#include "roadmap_analytics.h"
#include "roadmap_iphonesearch_dialog.h"
#include "roadmap_search.h"


//////////////////////////////////////////////////////////////////////////////////////////////////
static const char*   search_title = "Drive to";

//////////////////////////////////////////////////////////////////////////////////////////////////

enum IDs {
   ID_SEARCH_FAVORITES = 1,
	ID_SEARCH_ADDRESSBOOK,
   ID_SEARCH_HISTORY,
   ID_SEARCH_MARKED_LOCATIONS,
   ID_SEARCH_BAR,
   ID_TAG_IMAGE,
   ID_TAG_LABEL1,
   ID_TAG_LABEL2,
   ID_TAG_LABEL3,
   ID_TAG_BACKGROUND
};

#define MAX_IDS 25

static const char *id_actions[MAX_IDS];

void single_search_resolved_dlg_show (const char** results, void** indexes,
                                      int count_adr, int count_ls, int count_ab,
                                      PFN_ON_ITEM_SELECTED on_item_selected, 
                                      SsdSoftKeyCallback detail_button_callback,
                                      const char* fav_name) {
   
   SingleSearchResultsDialog *dialog = [[SingleSearchResultsDialog alloc] initWithStyle:UITableViewStyleGrouped];
   [dialog showResults:results
               indexes:indexes
              countAdr:count_adr
               countLs:count_ls
               countAb:count_ab
        onItemSelected:on_item_selected
        onDetailButton:detail_button_callback
               favName:fav_name];
   
}

void roadmap_search_menu (void) {
   roadmap_analytics_log_event(ANALYTICS_EVENT_SEARCHMENU, NULL, NULL);
   
   SearchDialog *dialog = [[SearchDialog alloc] init];
   [dialog show];
}


void add_home_work_dlg(int iType){
   AddHomeWorkDialog *dialog = [[AddHomeWorkDialog alloc] init];
   [dialog showWithType: iType];
}



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
@implementation SingleSearchResultsDialog
@synthesize dataArray;
@synthesize headersArray;

- (id)initWithStyle:(UITableViewStyle)style
{	
	self = [super initWithStyle:style];
	
	dataArray = [[NSMutableArray arrayWithCapacity:1] retain];
   headersArray = [[NSMutableArray arrayWithCapacity:1] retain];

	return self;
}

- (void) viewDidLoad
{
	UITableView *tableView = [self tableView];
	
   roadmap_main_set_table_color(tableView);
   tableView.rowHeight = 60;
}


- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
   return roadmap_main_should_rotate (interfaceOrientation);
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
   roadmap_device_event_notification( device_event_window_orientation_changed);
}

- (void) onClose
{
   roadmap_main_show_root(NO);
}

- (void) showResults:(const char**)results
             indexes:(void**)indexes
            countAdr:(int)count_adr
             countLs:(int)count_ls
             countAb:(int)count_ab
      onItemSelected:(PFN_ON_ITEM_SELECTED)on_item_selected
      onDetailButton:(SsdSoftKeyCallback)detail_button_callback
             favName:(const char*)fav_name
{
   NSMutableArray *groupArray = NULL;
   NSString *str = NULL;
	UIImage *img = NULL;
	NSMutableDictionary *dict = NULL;
   NSNumber *accessoryType = NULL;
   NSData	*value = NULL;
   iphoneTableHeader *header;
   int i;
   int count = 0;
   
   if (!fav_name || !fav_name[0]) {
      [self setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Drive to")]];
      accessoryType = [NSNumber numberWithInt:UITableViewCellAccessoryDetailDisclosureButton];
   } else {
      [self setTitle:[NSString stringWithUTF8String:fav_name]];
      accessoryType = [NSNumber numberWithInt:UITableViewCellAccessoryNone];
   }
   
   //set right button
	UINavigationItem *navItem = [self navigationItem];
   UIBarButtonItem *barButton = [[UIBarButtonItem alloc] initWithTitle:[NSString stringWithUTF8String:roadmap_lang_get("Close")]
                                                                 style:UIBarButtonItemStyleDone target:self action:@selector(onClose)];
   [navItem setRightBarButtonItem:barButton];
   
   
   g_on_item_selected = on_item_selected;
   g_detail_button_callback = detail_button_callback;
   g_fav_name = fav_name;
   
   //populate list
   //Group #1 - addresses
   if (count_adr > 0) {
      if (count_adr > 3)
         minimized[0] = TRUE;
      else
         minimized[0] = FALSE;
      
      header = [[iphoneTableHeader alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT) ];
      if (!fav_name || !fav_name[0]) {
         [header setText:roadmap_lang_get("New address")];
      } else {
         [header setText:roadmap_lang_get("Tap the correct address below")];
      }

      [headersArray addObject:header];
      [header release];
      
      groupArray = [NSMutableArray arrayWithCapacity:1];
      for (i = 0; i < count_adr; i++) {
         dict = [NSMutableDictionary dictionaryWithCapacity:1];
         
         str = [NSString stringWithUTF8String:roadmap_lang_get(results[count])];
         [dict setObject:str forKey:@"text"];
         
         value = [NSData dataWithBytes:&indexes[count] length:sizeof(indexes[count])];
			[dict setObject:value forKey:@"value"];
         
         img = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "search_address");
         if (img) {
            [dict setObject:img forKey:@"image"];
         }
         
         [dict setObject:accessoryType forKey:@"accessory"];
         
         [groupArray addObject:dict];
         count++;
      }
      [dataArray addObject:groupArray];
   }
   
   //Group #2 - local search
   if (count_ls > 0) {
      if (count_adr > 3 && count_ls > 3)
         minimized[1] = TRUE;
      else
         minimized[1] = FALSE;
      
      header = [[iphoneTableHeader alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT) ];
      [header setText:roadmap_lang_get(local_search_get_provider_label())];
      [headersArray addObject:header];
      [header release];
      
      groupArray = [NSMutableArray arrayWithCapacity:1];
      for (i = 0; i < count_ls; i++) {
         dict = [NSMutableDictionary dictionaryWithCapacity:1];
         
         str = [NSString stringWithUTF8String:roadmap_lang_get(results[count])];
         if (!str) {
            //TEMP Avi R - we need to handle UTF8 strings correctly, cut full character (variable bytes size)
            char fixed_str[512];
            strncpy_safe(fixed_str, results[count], strlen(results[count]) -2);
            str = [NSString stringWithUTF8String:roadmap_lang_get(fixed_str)];
         }
         [dict setObject:str forKey:@"text"];
         
         value = [NSData dataWithBytes:&indexes[count] length:sizeof(indexes[count])];
			[dict setObject:value forKey:@"value"];
         
         img = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, local_search_get_icon_name());
         if (img) {
            [dict setObject:img forKey:@"image"];
         }
         
         [dict setObject:accessoryType forKey:@"accessory"];
         
         [groupArray addObject:dict];
         count++;
      }
      [dataArray addObject:groupArray];
   }
   
   
   //Group #3 - address book
   if (count_ab > 0) {
      if (count_ab > 3)
         minimized[2] = TRUE;
      else
         minimized[2] = FALSE;
      
      header = [[iphoneTableHeader alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT) ];
      [header setText:roadmap_lang_get("Contacts")];
      [headersArray addObject:header];
      [header release];
      
      groupArray = [NSMutableArray arrayWithCapacity:1];
      for (i = 0; i < count_ab; i++) {
         dict = [NSMutableDictionary dictionaryWithCapacity:1];
         
         str = [NSString stringWithUTF8String:roadmap_lang_get(results[count])];
         [dict setObject:str forKey:@"text"];
         
         value = [NSData dataWithBytes:&indexes[count] length:sizeof(indexes[count])];
			[dict setObject:value forKey:@"value"];
         
         img = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "");
         if (img) {
            [dict setObject:img forKey:@"image"];
         }
         
         [dict setObject:accessoryType forKey:@"accessory"];
         
         [groupArray addObject:dict];
         count++;
      }
      [dataArray addObject:groupArray];
   }
   
   roadmap_main_push_view (self);
}

- (void)dealloc
{
	[dataArray release];
	dataArray = NULL;
   [headersArray release];
   headersArray = NULL;
	
	[super dealloc];
}


//////////////////////
//TableView delegate
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	
   return [dataArray count];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
   int count = [[dataArray objectAtIndex:section] count];
   
   if (minimized[section])
      return 3;
   else
      return count;
}

- (NSIndexPath *)tableView:(UITableView *)tableView willSelectRowAtIndexPath:(NSIndexPath *)indexPath {
   NSMutableArray *groupArray = [dataArray objectAtIndex:indexPath.section];
	NSDictionary *dict = (NSDictionary *)[groupArray objectAtIndex:indexPath.row];
   
   if (![dict objectForKey:@"value"] && ![dict objectForKey:@"callback"])
      return NULL;
   else
      return indexPath;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	iphoneCell *cell = NULL;
	NSString *cellId = NULL;
	UIView *view = NULL;
   char label[50];
   NSMutableArray *groupArray = [dataArray objectAtIndex:indexPath.section];
   
   if (minimized[indexPath.section] && indexPath.row == 2){
      cellId = @"moreCell";
      cell = (iphoneCell *)[tableView dequeueReusableCellWithIdentifier:cellId];
      
      if (cell == nil) {
         cell = [[[iphoneCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:cellId] autorelease];
      }

      if (roadmap_lang_rtl())
         snprintf(label, sizeof(label), "%s %d...", roadmap_lang_get("more"), [groupArray count] - 2);
      else
         snprintf(label, sizeof(label), "%d %s...", [groupArray count] - 2, roadmap_lang_get("more"));
      
      cell.textLabel.text = [NSString stringWithUTF8String:label];
      cell.textLabel.textAlignment = UITextAlignmentCenter;

      return cell;
   }
	
	NSDictionary *dict = (NSDictionary *)[groupArray objectAtIndex:indexPath.row];
	
	cellId = @"simpleCellML";
	
   cell = (iphoneCell *)[tableView dequeueReusableCellWithIdentifier:cellId];
   
   if (cell == nil) {
      cell = [[[iphoneCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:cellId] autorelease];
      
      if (![dict objectForKey:@"text"]) {
			while ([cell.contentView.subviews count] > 0) {
				view =  [cell.contentView.subviews objectAtIndex:0];
				[view removeFromSuperview];
			}
		}
   }
   
   if ([cellId compare:@"simpleCellML"] == 0)
      cell.textLabel.numberOfLines = 0;
   
	if ([dict objectForKey:@"text"]){
		cell.textLabel.text = (NSString *)[dict objectForKey:@"text"];
      
		if ([dict objectForKey:@"image"]){
			cell.imageView.image = (UIImage *)[dict objectForKey:@"image"];
		}
      
      if ([dict objectForKey:@"text_right"]) {
         cell.rightLabel.text = (NSString *)[dict objectForKey:@"text_right"];
      }
      
	}
	
	if ([dict objectForKey:@"accessory"]){
		NSNumber *accessoryType = (NSNumber *)[dict objectForKey:@"accessory"];
		cell.accessoryType = [accessoryType integerValue];
	}
   
   if (![dict objectForKey:@"value"] && ![dict objectForKey:@"callback"])
      cell.selectionStyle = UITableViewCellSelectionStyleNone;
	
	return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	
   [tableView deselectRowAtIndexPath:[tableView indexPathForSelectedRow] animated:NO];
   
   if (minimized[indexPath.section] && indexPath.row == 2) {
      NSIndexSet *sections = [NSIndexSet indexSetWithIndex:indexPath.section];
      minimized[indexPath.section] = FALSE;
      [tableView reloadSections:sections withRowAnimation:NO];
   } else {
      NSMutableArray *groupArray = [dataArray objectAtIndex:indexPath.section];
      NSDictionary *dict = (NSDictionary *)[groupArray objectAtIndex:indexPath.row];
      
      if (g_on_item_selected && [dict objectForKey:@"value"]){
         NSString *name = (NSString *)[dict objectForKey:@"text"];
         NSData *data = (NSData *)[dict objectForKey:@"value"];
         const void** value = (const void**) [data bytes];
         if ((int)value != LIST_MENU_NO_ACTION)
            g_on_item_selected(NULL, [name UTF8String], *value, NULL);
      }
   }
}

- (void)tableView:(UITableView *)tableView accessoryButtonTappedForRowWithIndexPath:(NSIndexPath *)indexPath
{
	NSMutableArray *groupArray = [dataArray objectAtIndex:indexPath.section];
	NSDictionary *dict = (NSDictionary *)[groupArray objectAtIndex:indexPath.row];
	
	if (!g_detail_button_callback || ![dict objectForKey:@"value"])
		return;
   
	NSData *data = (NSData *)[dict objectForKey:@"value"];
	const void** value = (const void**) [data bytes];
	
	g_detail_button_callback (NULL, *value, NULL);
}



- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {
   if (headersArray)
      return [headersArray objectAtIndex:section];
   else
      return NULL;
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

@end


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
@implementation CoverView
@synthesize delegate;

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
   //SearchDialog *view = (SearchDialog *)self.delegate;
   //[view cancelSearch];
}
@end


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
@implementation SearchDialog
@synthesize dataArray;
@synthesize headersArray;
@synthesize gSearchView;
@synthesize gTableView;

- (id)init
{
   int i;
	static int initialized = 0;
   
	self = [super init];
	
	dataArray = [[NSMutableArray arrayWithCapacity:1] retain];
   headersArray = [[NSMutableArray arrayWithCapacity:1] retain];
   
   if (!initialized) {
		for (i=0; i < MAX_IDS; ++i) {
			id_actions[i] = NULL;
		}
		initialized = 1;
	}
   
   UIScrollView *view = [[UIScrollView alloc] init];
   view.alwaysBounceVertical = YES;
   self.view = view;
   [view release];
	
	return self;
}

- (void) viewWillAppear:(BOOL)animated
{
   roadmap_main_set_table_color(gTableView);
   
   [self hideCoverView];
   [self refreshFavorites];
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

- (void) refreshFavorites
{
   NSMutableArray *groupArray = NULL;
   UIImage *image;
   iphoneCell *actionCell = NULL;
   char **labels;
   char **icons;
   int i;
   roadmap_search_top_three_fav(&labels, &fav_values, &icons, &fav_count, &fav_callback);
   
   if (fav_count > 0) {
      groupArray = [NSMutableArray arrayWithCapacity:1];
      
      for (i = 0; i < fav_count; i++) {
         actionCell = [[[iphoneCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"test"] autorelease];
         actionCell.textLabel.text = [NSString stringWithUTF8String:roadmap_lang_get(labels[i])];
         image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, icons[i]);
         if (image) {
            actionCell.imageView.image = image;
         }
         [groupArray addObject:actionCell];
      }
      
      if ([dataArray count] == 2)
         [dataArray replaceObjectAtIndex:0 withObject:groupArray];
      else
         [dataArray insertObject:groupArray atIndex:0];

      //NSIndexSet *sections = [NSIndexSet indexSetWithIndex:0];
      //[gTableView reloadSections:sections withRowAnimation:UITableViewRowAnimationNone];
      [gTableView reloadData];
   }
}

- (void) populateData
{
	NSMutableArray *groupArray = NULL;
   iphoneCell *actionCell = NULL;
   
   
   //Group #1
   //refreshed later
   
   //Group #2
   groupArray = [NSMutableArray arrayWithCapacity:1];
   
   //Favorites
   actionCell = createActionCell ("search_favorites", ID_SEARCH_FAVORITES);
   id_actions[ID_SEARCH_FAVORITES] = "search_favorites";
   [groupArray addObject:actionCell];
   
   //History
   actionCell = createActionCell ("search_history", ID_SEARCH_HISTORY);
   id_actions[ID_SEARCH_HISTORY] = "search_history";
	[groupArray addObject:actionCell];
   
   //Marked locations
   actionCell = createActionCell ("search_marked_locations", ID_SEARCH_MARKED_LOCATIONS);
   id_actions[ID_SEARCH_MARKED_LOCATIONS] = "search_marked_locations";
	[groupArray addObject:actionCell];
   
   //Address Book
   actionCell = createActionCell ("search_ab", ID_SEARCH_ADDRESSBOOK);
   id_actions[ID_SEARCH_ADDRESSBOOK] = "search_ab";
	[groupArray addObject:actionCell];
      
   [dataArray addObject:groupArray];
   
   [self refreshFavorites];
}


- (void) show
{
   CGRect rect;
   UIScrollView *view = (UIScrollView *)self.view;
   UISearchBar *searchBar;
   
   rect = self.view.bounds;
	rect.size.height = 45;
   searchBar = [[UISearchBar alloc] initWithFrame:rect];
   [searchBar setAutoresizesSubviews: YES];
   [searchBar setAutoresizingMask: UIViewAutoresizingFlexibleWidth];
   [searchBar setShowsCancelButton:NO animated:NO];
   searchBar.showsCancelButton = NO;
   searchBar.placeholder = [NSString stringWithUTF8String:roadmap_lang_get("Search address or Place")];
   searchBar.delegate = self;
   searchBar.tag = ID_SEARCH_BAR;
   rect.size.height = 50;
   gSearchView = [[UIView alloc] initWithFrame:rect];
	[gSearchView addSubview:searchBar];
   [searchBar release];
   
   rect = self.view.bounds;
   gTableView = [[UITableView alloc] initWithFrame:rect style:UITableViewStyleGrouped];
   [gTableView setAutoresizingMask: UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight];
   gTableView.dataSource = self;
   gTableView.delegate = self;
   gTableView.rowHeight = 60;
   [view addSubview:gTableView];
   
   gCoverView = NULL;

	[self populateData];

	[self setTitle:[NSString stringWithUTF8String:roadmap_lang_get(search_title)]];
   /*
   //set right button
	UINavigationItem *navItem = [self navigationItem];
   UIBarButtonItem *barButton = [[UIBarButtonItem alloc] initWithTitle:[NSString stringWithUTF8String:roadmap_lang_get("Close")]
                                                                 style:UIBarButtonItemStyleDone target:self action:@selector(onClose)];
   [navItem setRightBarButtonItem:barButton];
	*/
	roadmap_main_push_view (self);
}

- (void)showCoverView
{
   if (!gCoverView) {
      gCoverView = [[CoverView alloc] initWithFrame:CGRectZero];
      gCoverView.backgroundColor = [UIColor colorWithWhite:0.2f alpha:0.7f];
      gCoverView.autoresizesSubviews = YES;
      gCoverView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
      gCoverView.delegate = self;
   } 
   
   CGRect rect = gTableView.frame;
   int barHeight = [gSearchView viewWithTag:ID_SEARCH_BAR].bounds.size.height;
   rect.origin.y += barHeight;
   rect.size.height -= barHeight;
   gCoverView.frame = rect;
   [self.view addSubview:gCoverView];
}

- (void)hideCoverView
{
   if (!gCoverView) {
      return;
   } 
   
   [gCoverView removeFromSuperview];
}

- (void) cancelSearch
{
   UISearchBar *searchBar = (UISearchBar *)[gSearchView viewWithTag:ID_SEARCH_BAR];
   [searchBar resignFirstResponder];
   [searchBar setShowsCancelButton:NO animated:YES];
   [self hideCoverView];
}

- (void)dealloc
{
	[dataArray release];
   dataArray = NULL;
   [headersArray release];
   headersArray = NULL;
   
   if (gSearchView)
      [gSearchView release];
   if (gTableView)
      [gTableView release];
   if (gCoverView)
      [gCoverView release];
	
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
	} else {
      if (indexPath.section == 0 && fav_callback) {
         fav_callback(NULL, NULL, fav_values[indexPath.row]);
      }
   }
}

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {
   if (section == 0)
      return gSearchView;
   else
      return NULL;
}

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section {
   if (section == 0)
      return gSearchView.bounds.size.height;
   else
      return 0;
}


//////////////////////////////////////////////////////////
//UISearchBar view delegate
- (void)searchBarSearchButtonClicked:(UISearchBar *)searchBar
{
	[searchBar resignFirstResponder];
   [searchBar setShowsCancelButton:NO animated:NO];
   single_search_auto_search ([[searchBar text] UTF8String]);
}

- (BOOL)searchBarShouldBeginEditing:(UISearchBar *)searchBar
{
   if (roadmap_keyboard_typing_locked(TRUE)){
      return NO;
   } else {
      [searchBar setShowsCancelButton:YES animated:YES];
      [self showCoverView];
      return YES;
   }
}

- (void)searchBarCancelButtonClicked:(UISearchBar *)searchBar
{
   [self cancelSearch];
}
@end



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
@implementation AddHomeWorkDialog
@synthesize gSearchView;

- (id)init
{
   CGRect rect = [[UIScreen mainScreen] applicationFrame];
	rect.origin.x = 0;
	rect.origin.y = 0;
	rect.size.height = roadmap_main_get_mainbox_height();
   
	self = [super init];
   
   UIScrollView *scrollView = [[UIScrollView alloc] initWithFrame:rect];
   
   scrollView.alwaysBounceVertical = YES;
//   [view setAutoresizesSubviews: YES];
//   [view setAutoresizingMask: UIViewAutoresizingFlexibleWidth || UIViewAutoresizingFlexibleHeight];
   self.view = scrollView;
   [scrollView release];
	
	return self;
}

- (void) resizeViews
{
   int viewPosY = gSearchView.frame.size.height + 10;
   CGRect rect;
   UIImageView *imageView;
   UILabel *label;
   UIScrollView *scrollView = (UIScrollView *)self.view;
   
   //Set image pos
   imageView = (UIImageView *)[scrollView viewWithTag:ID_TAG_IMAGE];
   if (!imageView)
      return;
   rect = imageView.frame;
   rect.origin.x = (scrollView.bounds.size.width - imageView.bounds.size.width) /2;
   rect.origin.y = viewPosY;
   imageView.frame = rect;
   viewPosY += imageView.frame.size.height + 5;
   
   //Set label1 size/pos
   label = (UILabel *)[scrollView viewWithTag:ID_TAG_LABEL1];
   if (!label)
      return;
   rect = label.frame;
   rect.size.width = scrollView.bounds.size.width - 20;
   label.frame = rect;
   [label sizeToFit];
   rect = label.frame;
   rect.origin.x = (scrollView.bounds.size.width - label.bounds.size.width)/2;
   rect.origin.y = viewPosY;
   label.frame = rect;
   viewPosY += label.bounds.size.height + 5;
   
   //Set label2 size/pos
   label = (UILabel *)[scrollView viewWithTag:ID_TAG_LABEL2];
   if (!label)
      return;
   rect = label.frame;
   rect.size.width = scrollView.bounds.size.width - 20;
   label.frame = rect;
   [label sizeToFit];
   rect = label.frame;
   rect.origin.x = (scrollView.bounds.size.width - label.bounds.size.width)/2;
   rect.origin.y = viewPosY;
   label.frame = rect;
   viewPosY += label.bounds.size.height + 5;
   
   //Set label3 size/pos
   label = (UILabel *)[scrollView viewWithTag:ID_TAG_LABEL3];
   if (!label)
      return;
   rect = label.frame;
   rect.size.width = scrollView.bounds.size.width - 20;
   label.frame = rect;
   [label sizeToFit];
   rect = label.frame;
   rect.origin.x = (scrollView.bounds.size.width - label.bounds.size.width)/2;
   rect.origin.y = viewPosY;
   label.frame = rect;
   viewPosY += label.bounds.size.height + 5;
   
   //Set frame
   imageView = (UIImageView *)[scrollView viewWithTag:ID_TAG_BACKGROUND];
   if (!imageView)
      return;
   rect = CGRectMake(5, gSearchView.frame.size.height + 5, scrollView.bounds.size.width - 10, viewPosY - gSearchView.frame.size.height);
   imageView.frame = rect;
   viewPosY += 5;
   
   [scrollView setContentSize:CGSizeMake(scrollView.frame.size.width, viewPosY)];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
   return roadmap_main_should_rotate (interfaceOrientation);
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
   roadmap_device_event_notification( device_event_window_orientation_changed);
}

- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation duration:(NSTimeInterval)duration {
   [self resizeViews];
}

- (void)viewDidAppear:(BOOL)animated {
   [self resizeViews];
}

- (void) showWithType: (int)iType
{
   CGRect rect;
   UIScrollView *scrollView = (UIScrollView *)self.view;
   UISearchBar *searchBar;
   UIImage *image;
   UIImageView *imageView;
   UILabel *label;
   const char *title;
   float posY = 0.0f;
   
   
   if (iType == 0){
      title = roadmap_lang_get("My Home");
      //g_favorite_name = roadmap_lang_get("Home");
   }
   else{
      title = roadmap_lang_get("My Work");
      //g_favorite_name = roadmap_lang_get("Work");
   }
   
   g_iType = iType;
   
   //Search bar
   rect = scrollView.bounds;
	rect.size.height = 45;
   searchBar = [[UISearchBar alloc] initWithFrame:rect];
   [searchBar setAutoresizesSubviews: YES];
   [searchBar setAutoresizingMask: UIViewAutoresizingFlexibleWidth];
   [searchBar setShowsCancelButton:NO animated:NO];
   searchBar.showsCancelButton = NO;
   searchBar.placeholder = [NSString stringWithUTF8String:roadmap_lang_get("Search address or Place")];
   searchBar.delegate = self;
   searchBar.tag = ID_SEARCH_BAR;
   rect.size.height = 50;
   gSearchView = [[UIView alloc] initWithFrame:rect];
   [gSearchView setAutoresizesSubviews: YES];
   [gSearchView setAutoresizingMask: UIViewAutoresizingFlexibleWidth];
	[gSearchView addSubview:searchBar];
   [searchBar release];
   
   [scrollView addSubview:gSearchView];
   
   // Background image
   image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "comments_alert");
	if (image) {
		UIImage *strechedImage = [image stretchableImageWithLeftCapWidth:20 topCapHeight:20];
		imageView = [[UIImageView alloc] initWithImage:strechedImage];
		imageView.tag = ID_TAG_BACKGROUND;
		[scrollView addSubview:imageView];
		[imageView release];
	}
   
   // HW route image
   image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "HW_route");
   
   if (image) {
      imageView = [[UIImageView alloc] initWithImage:image];
      imageView.tag = ID_TAG_IMAGE;
      [scrollView addSubview:imageView];
      [imageView release];
   }
   
   // label 1
   label = [[UILabel alloc] initWithFrame:CGRectZero];
	[label setText:[NSString stringWithUTF8String:roadmap_lang_get("Waze is best used for commuting.")]];
	[label setTextAlignment:UITextAlignmentCenter];
	[label setFont:[UIFont systemFontOfSize:16]];
   [label setAutoresizingMask:UIViewAutoresizingFlexibleHeight || UIViewAutoresizingFlexibleRightMargin || UIViewAutoresizingFlexibleLeftMargin];
   [label setBackgroundColor:[UIColor clearColor]];
   label.tag = ID_TAG_LABEL1;
	[scrollView addSubview:label];
	[label release];
   
   // label 2
   label = [[UILabel alloc] initWithFrame:CGRectZero];
	[label setText:[NSString stringWithUTF8String:roadmap_lang_get("Once you add 'Home' and 'Work' to your favorites, waze'll learn your preferred routes and departure times to these destinations.")]];
	[label setTextAlignment:UITextAlignmentCenter];
	[label setFont:[UIFont systemFontOfSize:16]];
   [label setAutoresizingMask:UIViewAutoresizingFlexibleHeight || UIViewAutoresizingFlexibleRightMargin || UIViewAutoresizingFlexibleLeftMargin];
   [label setBackgroundColor:[UIColor clearColor]];
   [label setNumberOfLines:0];
   label.tag = ID_TAG_LABEL2;
	[scrollView addSubview:label];
	[label release];
   
   // label 3
   label = [[UILabel alloc] initWithFrame:CGRectZero];
	[label setText:[NSString stringWithUTF8String:roadmap_lang_get("Enter your address in the search box above.")]];
	[label setTextAlignment:UITextAlignmentCenter];
	[label setFont:[UIFont systemFontOfSize:16]];
   [label setAutoresizingMask:UIViewAutoresizingFlexibleHeight || UIViewAutoresizingFlexibleRightMargin || UIViewAutoresizingFlexibleLeftMargin];
   [label setBackgroundColor:[UIColor clearColor]];
   [label setNumberOfLines:0];
   label.tag = ID_TAG_LABEL3;
	[scrollView addSubview:label];
	[label release];
   
	[self setTitle:[NSString stringWithUTF8String:roadmap_lang_get(title)]];
   
   [self resizeViews];
   
	roadmap_main_push_view (self);
}

- (void) cancelSearch
{
   UISearchBar *searchBar = (UISearchBar *)[gSearchView viewWithTag:ID_SEARCH_BAR];
   [searchBar resignFirstResponder];
   [searchBar setShowsCancelButton:NO animated:YES];
}

- (void)dealloc
{
   
   if (gSearchView)
      [gSearchView release];
	
	[super dealloc];
}


//////////////////////////////////////////////////////////
//UISearchBar view delegate
- (void)searchBarSearchButtonClicked:(UISearchBar *)searchBar
{
	[searchBar resignFirstResponder];
   [searchBar setShowsCancelButton:NO animated:NO];
   
   if (g_iType == 0){
      single_search_auto_search_fav ([[searchBar text] UTF8String], roadmap_lang_get("Home"));
   }
   else{
      single_search_auto_search_fav ([[searchBar text] UTF8String], roadmap_lang_get("Work"));
   }
}

- (BOOL)searchBarShouldBeginEditing:(UISearchBar *)searchBar
{
   if (roadmap_keyboard_typing_locked(TRUE)){
      return NO;
   } else {
      [searchBar setShowsCancelButton:YES animated:YES];
      return YES;
   }
}

- (void)searchBarCancelButtonClicked:(UISearchBar *)searchBar
{
   [self cancelSearch];
}

@end
