/* roadmap_list_menu.m - Handle iphone menus as lists
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi R
 *   Copyright 2009, Waze Ltd
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

#include "roadmap_main.h"
#include "roadmap_iphonemain.h"
#include "roadmap_list_menu.h"
#include "roadmap_iphonelist_menu.h"
#include "roadmap_path.h"
#include "roadmap_lang.h"
#include "roadmap_iphoneimage.h"
#include "roadmap_device_events.h"
#include "widgets/iphoneCell.h"
#include "widgets/iphoneCellML.h"



void roadmap_list_menu_simple (const char             *name,
                               const char             *items_file,
                               const char             *items[],
                               const char             *additional_item,
                               const char             *custom_labels[],
                               const char             *custom_icons[],
                               PFN_ON_DIALOG_CLOSED   on_dialog_closed,
                               const RoadMapAction    *actions,
                               int                    flags) {
	
	RoadMapListMenu *RoadMapListViewCont = [[RoadMapListMenu alloc] initWithStyle:UITableViewStyleGrouped];
	
	[RoadMapListViewCont activateSimpleWithName:name 
                                  andItemsFile:items_file
                                      andItems:items
                             andAdditionalItem:additional_item
                               andCustomLabels:custom_labels
                                andCustomIcons:custom_icons
                             andOnDialogClosed:on_dialog_closed
                                    andActions:actions];
}


void roadmap_list_menu_generic_refresh (void*                  list,
                                        const char*            title,
                                        int                    count,
                                        const char**           labels,
                                        const void**           values,
                                        const char**           icons,
                                        const char**           right_labels,
                                        list_menu_selector*    selector,
                                        PFN_ON_ITEM_SELECTED   on_item_selected,
                                        PFN_ON_ITEM_SELECTED   on_item_deleted,
                                        void*                  context,
                                        SsdSoftKeyCallback     detail_button_callback,
                                        int                    list_height,
                                        int                    add_next_button,
                                        list_menu_empty_message* empty_message) {
	
   if (!list)
      return;
   
	RoadMapListMenu *RoadMapListViewCont = (RoadMapListMenu *)list;
	
	[RoadMapListViewCont activateGenericWithTitle:title
                                        andCount:count
                                       andLabels:labels
                                       andValues:values
                                        andIcons:icons
                                  andRightLabels:right_labels
                                     andSelector:selector
                               andOnItemSelected:on_item_selected
                                andOnItemDeleted:on_item_deleted
                                      andContext:context
                         andDetailButtonCallback:detail_button_callback
                                   andListHeight:list_height
                              andAddDetailButton:add_next_button
                                      andRefresh:TRUE
                                        andEmpty:empty_message];
}

void* roadmap_list_menu_generic (const char*          title,
                                 int                  count,
                                 const char**         labels,
                                 const void**         values,
                                 const char** 		   icons,
                                 const char** 		   right_labels,
                                 list_menu_selector   *selector,
                                 PFN_ON_ITEM_SELECTED on_item_selected,
                                 PFN_ON_ITEM_SELECTED on_item_deleted,
                                 void*                context,
                                 SsdSoftKeyCallback   detail_button_callback,
                                 int                  list_height,
                                 int                  add_next_button,
                                 list_menu_empty_message* empty_message) {
	
	RoadMapListMenu *RoadMapListViewCont = [[RoadMapListMenu alloc] initWithStyle:UITableViewStyleGrouped];
	
	[RoadMapListViewCont activateGenericWithTitle:title
                                        andCount:count
                                       andLabels:labels
                                       andValues:values
                                        andIcons:icons
                                  andRightLabels:right_labels
                                     andSelector:selector
                               andOnItemSelected:on_item_selected
                                andOnItemDeleted:on_item_deleted
                                      andContext:context
                         andDetailButtonCallback:detail_button_callback
                                   andListHeight:list_height
                              andAddDetailButton:add_next_button
                                      andRefresh:FALSE
                                        andEmpty:empty_message];
   return (void*) RoadMapListViewCont;
}


void roadmap_list_menu_custom_refresh (void*                  list,
                                       const char*            title,
                                       int                    count,
                                       const void**           values,
                                       list_menu_selector     *selector,
                                       PFN_ON_ITEM_SELECTED	on_item_selected,
                                       PFN_ON_ITEM_SELECTED   on_item_deleted,
                                       SsdSoftKeyCallback     left_softkey_callback,
                                       ViewForCellCallback    custom_view_callback,
                                       void*                  context, 
                                       int                    list_height,
                                       BOOL                   add_next_button,
                                       list_menu_empty_message* empty_message) {
	
   if (!list)
      return;
   
	RoadMapListMenu *RoadMapListViewCont = (RoadMapListMenu *)list;
	
	[RoadMapListViewCont activateCustomWithTitle:title
                                       andCount:count
                                      andValues:values
                                    andSelector:selector
                              andOnItemSelected:on_item_selected
                               andOnItemDeleted:on_item_deleted
                        andDetailButtonCallback:left_softkey_callback
                            andCellViewCallback:custom_view_callback
                                     andContext:context
                                  andListHeight:list_height
                             andAddDetailButton:add_next_button
                                     andRefresh:TRUE
                                       andEmpty:empty_message];
}

void* roadmap_list_menu_custom (const char*           title,
                                int                   count,
                                const void**          values,
                                list_menu_selector   *selector,
                                PFN_ON_ITEM_SELECTED	on_item_selected,
                                PFN_ON_ITEM_SELECTED  on_item_deleted,
                                SsdSoftKeyCallback    left_softkey_callback,
                                ViewForCellCallback   custom_view_callback,
                                void*                 context, 
                                int                   list_height,
                                BOOL                  add_next_button,
                                list_menu_empty_message* empty_message) {
	
	RoadMapListMenu *RoadMapListViewCont = [[RoadMapListMenu alloc] initWithStyle:UITableViewStyleGrouped];
	
	[RoadMapListViewCont activateCustomWithTitle:title
                                       andCount:count
                                      andValues:values
                                    andSelector:selector
                              andOnItemSelected:on_item_selected
                               andOnItemDeleted:on_item_deleted
                        andDetailButtonCallback:left_softkey_callback
                            andCellViewCallback:custom_view_callback
                                     andContext:context
                                  andListHeight:list_height
                             andAddDetailButton:add_next_button
                                     andRefresh:FALSE
                                       andEmpty:empty_message];
   return (void*) RoadMapListViewCont;
}



@implementation RoadMapListMenu
@synthesize dismissListCallback;
@synthesize dataArray;
@synthesize emptyListView;

- (id)initWithStyle:(UITableViewStyle)style
{	
	self = [super initWithStyle:style];
	
	dataArray = [[NSMutableArray arrayWithCapacity:1] retain];
	dismissListCallback = NULL;
   exit_code = dec_close;
   
   emptyListView = NULL;   
	
	return self;
}

- (NSMutableArray *) createSelectorToolbar
{
   list_menu_selector selector = listData.selector;
   NSMutableArray *array = [NSMutableArray arrayWithCapacity:selector.count];
   UISegmentedControl *seg;
   NSMutableArray *barArray = [NSMutableArray arrayWithCapacity:1];
   UIBarButtonItem *barItem;
   UIBarButtonItem *flexSpacer = [[UIBarButtonItem alloc] initWithBarButtonSystemItem: UIBarButtonSystemItemFlexibleSpace
                                                                               target:nil action:nil]; 
   int i;
   
   for (i = 0; i < selector.count; i++) {
      [array addObject:[NSString stringWithUTF8String:roadmap_lang_get(selector.items[i])]];
   }
   
   seg = [[UISegmentedControl alloc] initWithItems:array];
   seg.selectedSegmentIndex = selector.default_index;
   seg.segmentedControlStyle = UISegmentedControlStyleBar;
   [seg sizeToFit];
   CGRect rect = seg.bounds;
   rect.size.width = 300;
   seg.bounds = rect;
   [seg addTarget:self action:@selector(onSegmentSelector:) forControlEvents:UIControlEventValueChanged];
   barItem = [[UIBarButtonItem alloc] initWithCustomView:seg];
   [seg release];
   [barArray addObject:flexSpacer];
   [barArray addObject:barItem];
   [barArray addObject:flexSpacer];
   [flexSpacer release];
   [barItem release];
   
   return barArray;
}


- (void) viewDidLoad
{
	UITableView *tableView = [self tableView];
	
   [tableView setBackgroundColor:roadmap_main_table_color()];
   if ([UITableView instancesRespondToSelector:@selector(setBackgroundView:)])
      [(id)(self.tableView) setBackgroundView:nil];
   
   if (tableHeight > 0) {
		[tableView setRowHeight:tableHeight];
	}
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
   return roadmap_main_should_rotate (interfaceOrientation);
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
   roadmap_device_event_notification( device_event_window_orientation_changed);
}

- (void)viewWillDisappear:(BOOL)animated
{
   [self.navigationController setToolbarHidden:YES];
}

- (void)viewWillAppear:(BOOL)animated
{
   if (listData.selector.count > 0) {
      self.toolbarItems = [self createSelectorToolbar];
      [self.navigationController setToolbarHidden:NO];
   } else {
      self.toolbarItems = NULL;
      [self.navigationController setToolbarHidden:YES];
   }
}


- (void) onClose
{
   [self.navigationController setToolbarHidden:YES];
   roadmap_main_show_root(NO);
}

- (const RoadMapAction *) findAction: (const RoadMapAction *) actions
										withItem: (const char *)item
{	
	while (actions->name != NULL) {
		if (strcmp (actions->name, item) == 0) return actions;
		++actions;
	}
	
	
	while (actions->label_long != NULL) {
		if (strcmp (actions->label_long, item) == 0) return actions;
		++actions;
	}
	
	return NULL;
}

- (void) populateListDataWithItemsFile :	(const char           *) items_file
                               andItems:	(const char           **)items
                             andActions:	(const RoadMapAction  *) actions
                      andAdditionalItem: (const char           *) additional_item
{
	int i;
	const char **menu_items = NULL;
	NSString *str = NULL;
	UIImage *img = NULL;
	NSMutableDictionary *dict = NULL;
	NSMutableArray *groupArray = NULL;
	NSData	*callback = NULL;
	NSNumber *accessoryType = NULL;
	
	if( items_file)
		menu_items = roadmap_factory_user_config (items_file, "menu", actions);
	
	if (!menu_items) menu_items = items;
	
	
	groupArray = [NSMutableArray arrayWithCapacity:1];
	
	for (i = 0; menu_items[i] != NULL; ++i) {
		
		const char *item = menu_items[i];
		
		if (item == RoadMapFactorySeparator) {
			[dataArray addObject:groupArray];
			
			groupArray = [NSMutableArray arrayWithCapacity:1];
			
			continue;
		} else {
			const RoadMapAction *this_action = [self findAction: actions withItem: item];
			dict = [NSMutableDictionary dictionaryWithCapacity:1];
			
			if (this_action)
			{
				if (this_action->label_long) {
					str = [NSString stringWithUTF8String:roadmap_lang_get(this_action->label_long)];
					[dict setObject:str forKey:@"text"];
				}
				
				img = roadmap_iphoneimage_load(item);
				
				if (img) {
					[dict setObject:img forKey:@"image"];
					[img release];
				}
				
				if (this_action->callback) {
					callback = [NSData dataWithBytes:&(this_action->callback) length:sizeof(RoadMapCallback)];
					[dict setObject:callback forKey:@"callback"];
				}
				
				accessoryType = [NSNumber numberWithInt:UITableViewCellAccessoryDisclosureIndicator];
				
				[dict setObject:accessoryType forKey:@"accessory"];
				
				[groupArray addObject:dict];
			}
		}
	}
   
   if (additional_item != NULL) {
      [dataArray addObject:groupArray];
      
      groupArray = [NSMutableArray arrayWithCapacity:1];
      
      const RoadMapAction *this_action = [self findAction: actions withItem: additional_item];
      dict = [NSMutableDictionary dictionaryWithCapacity:1];
      
      if (this_action)
      {
         if (this_action->label_long) {
            str = [NSString stringWithUTF8String:roadmap_lang_get(this_action->label_long)];
            [dict setObject:str forKey:@"text"];
         }
         
         img = roadmap_iphoneimage_load(additional_item);
         
         if (img) {
            [dict setObject:img forKey:@"image"];
            [img release];
         }
         
         if (this_action->callback) {
            callback = [NSData dataWithBytes:&(this_action->callback) length:sizeof(RoadMapCallback)];
            [dict setObject:callback forKey:@"callback"];
         }
         
         accessoryType = [NSNumber numberWithInt:UITableViewCellAccessoryDisclosureIndicator];
         
         [dict setObject:accessoryType forKey:@"accessory"];
         
         [groupArray addObject:dict];
      }
   }
   
   [dataArray addObject:groupArray];
}

- (void) populateListDataWithCount:(int)count
                         andLabels:(const char**)labels
                         andValues:(const void**)values
                          andIcons:(const char**)icons
                    andRightLabels:(const char**)right_labels
                andAddDetailButton:(int)add_next_button
{
	int i;
	NSString *str = NULL;
	UIImage *img = NULL;
	NSMutableDictionary *dict = NULL;
	NSMutableArray *groupArray = NULL;
	NSData	*value = NULL;
	NSNumber *accessoryType = NULL;
   
   [dataArray removeAllObjects];
	
	groupArray = [NSMutableArray arrayWithCapacity:1];
	
	for (i = 0; i < count; ++i) {
		dict = [NSMutableDictionary dictionaryWithCapacity:1];
		
		if (labels) {
			if (labels[i]){
				str = [NSString stringWithUTF8String:roadmap_lang_get(labels[i])];
				[dict setObject:str forKey:@"text"];
			}
		}
		
		if (icons) {
			if (icons[i]) {
				img = roadmap_iphoneimage_load(icons[i]);
				
				if (img) {
					[dict setObject:img forKey:@"image"];
					[img release];
				}
			}
		}
		
		if ((int) values[i] != LIST_MENU_NO_ACTION) {
			value = [NSData dataWithBytes:&values[i] length:sizeof(values[i])];
			[dict setObject:value forKey:@"value"];
		}
      
      if (right_labels) {
			if (right_labels[i]){
				str = [NSString stringWithUTF8String:roadmap_lang_get(right_labels[i])];
				[dict setObject:str forKey:@"text_right"];
			}
		}
		
      if (((int) values[i] != LIST_MENU_NO_ACTION)) {
         if (add_next_button == 1)
            accessoryType = [NSNumber numberWithInt:UITableViewCellAccessoryDetailDisclosureButton];
         else if (add_next_button == 2)
            accessoryType = [NSNumber numberWithInt:UITableViewCellAccessoryDisclosureIndicator];
         else
            accessoryType = [NSNumber numberWithInt:UITableViewCellAccessoryNone];
      } else {
         accessoryType = [NSNumber numberWithInt:UITableViewCellAccessoryNone];
      }
      
		[dict setObject:accessoryType forKey:@"accessory"];
				
		[groupArray addObject:dict];
	}

	[dataArray addObject:groupArray];
}

- (void) applyCustomLabels:(const char**)labels
                  andIcons:(const char**)icons
{
   int i;
   int j;
   int index = 0;
   NSMutableArray *groupArray;
   NSMutableDictionary *dict;
   NSString *str;
   UIImage *img;
   
   for (i = 0; i < [dataArray count]; ++i) {
      groupArray = [dataArray objectAtIndex:i];
      for (j = 0; j < [groupArray count]; ++j) {
         dict = [groupArray objectAtIndex:j];
         
         //set custom label
         if (labels && labels[index]) {
            str = [NSString stringWithUTF8String:roadmap_lang_get(labels[index])];
            [dict setObject:str forKey:@"text"];
         }
         
         //set custom icon
         if (icons && icons[index]) {
            img = roadmap_iphoneimage_load(icons[index]);
            
            if (img) {
               [dict setObject:img forKey:@"image"];
               [img release];
            }
         }
         index++;
      }
   }   
}

- (void) activateSimpleWithName:(const char           *)name
                   andItemsFile:(const char           *) items_file
                       andItems:(const char           **)items
              andAdditionalItem:(const char           *) additional_item
                andCustomLabels:(const char           **)custom_labels
                 andCustomIcons:(const char           **)custom_icons
              andOnDialogClosed:(PFN_ON_DIALOG_CLOSED) on_dialog_closed
                     andActions:(const RoadMapAction  *) actions
{
   tableHeight = 60;
	
	[self setTitle:[NSString stringWithUTF8String:roadmap_lang_get(name)]];
   
   //set right button
	UINavigationItem *navItem = [self navigationItem];
   UIBarButtonItem *barButton = [[UIBarButtonItem alloc] initWithTitle:[NSString stringWithUTF8String:roadmap_lang_get("Close")]
                                                                 style:UIBarButtonItemStyleDone target:self action:@selector(onClose)];
   [navItem setRightBarButtonItem:barButton];
	
	dismissListCallback = on_dialog_closed;
	listData.on_item_selected = NULL;
	listData.on_item_deleted = NULL;
	
	
	
	[self populateListDataWithItemsFile:items_file andItems:items andActions:actions andAdditionalItem:additional_item];
   
   if (custom_labels || custom_icons)
      [self applyCustomLabels:custom_labels andIcons:custom_icons];
	
	roadmap_main_push_view (self);
}

- (void) onSegmentSelector: (id)selector
{
   UISegmentedControl *seg = (UISegmentedControl*) selector;
   if (listData.selector.on_selector) {
      listData.selector.on_selector(NULL, NULL, (void*)seg.selectedSegmentIndex, listData.context);
   }
}

- (void) onEmptyMsgButton
{
   emptyListCb();
}

- (void) showEmptyMessage:(list_menu_empty_message*)empty_message
{
   CGRect rect;
   CGRect viewRect = self.view.bounds;
   iphoneLabel *label;
   UIImageView *imageView;
   UIButton *button;
   UIImage *image;
   
   enum tags {
      TAG_TITLE =1,
      TAG_TEXT,
      TAG_TOP_IMAGE,
      TAG_BOTTOM_IMAGE,
      TAG_BUTTON
   };
   
   if (!emptyListView) {
      emptyListView = [[UIScrollView alloc] initWithFrame:viewRect];
      emptyListView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
      emptyListView.autoresizesSubviews = YES;
      [self.view addSubview:emptyListView];
      
      //create title
      rect = CGRectMake(20, 60, viewRect.size.width - 40, 30);
      label = [[iphoneLabel alloc] initWithFrame:rect];
      label.tag = TAG_TITLE;
      label.autoresizesSubviews = YES;
      label.autoresizingMask = UIViewAutoresizingFlexibleWidth;
      label.numberOfLines = 0;
      label.font = [UIFont boldSystemFontOfSize:24];
      label.textAlignment = UITextAlignmentCenter;
      label.textColor = [UIColor darkGrayColor];
      label.backgroundColor = [UIColor clearColor];
      [emptyListView addSubview:label];
      [label release];
      
      //create text
      rect = CGRectMake(20, 110, viewRect.size.width - 40, 90);
      label = [[iphoneLabel alloc] initWithFrame:rect];
      label.tag = TAG_TEXT;
      label.autoresizesSubviews = YES;
      label.autoresizingMask = UIViewAutoresizingFlexibleWidth;
      label.numberOfLines = 0;
      label.font = [UIFont systemFontOfSize:18];
      label.textAlignment = UITextAlignmentCenter;
      label.textColor = [UIColor darkGrayColor];
      label.backgroundColor = [UIColor clearColor];
      [emptyListView addSubview:label];
      [label release];
      
      
      //create top image
      imageView = [[UIImageView alloc] init];
      imageView.tag = TAG_TOP_IMAGE;
      imageView.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin;
      imageView.autoresizesSubviews = YES;
      [emptyListView addSubview:imageView];
      [imageView release];
      
      //create bottom image
      imageView = [[UIImageView alloc] init];
      imageView.tag = TAG_BOTTOM_IMAGE;
      imageView.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin;
      imageView.autoresizesSubviews = YES;
      [emptyListView addSubview:imageView];
      [imageView release];
      
      //create button
      button = [UIButton buttonWithType:UIButtonTypeCustom];
      button.tag = TAG_BUTTON;
      button.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin;
      button.autoresizesSubviews = YES;
      image = roadmap_iphoneimage_load("button_up");
      [button setBackgroundImage:image forState:UIControlStateNormal];
      [button addTarget:self action:@selector(onEmptyMsgButton) forControlEvents:UIControlEventTouchUpInside];
      [emptyListView addSubview:button];
   }
   
   int yPos = 5;
   if (empty_message == NULL) {
      //title
      label = (iphoneLabel *)[emptyListView viewWithTag:TAG_TITLE];
      label.text = [NSString stringWithUTF8String:roadmap_lang_get("No items")];
      rect = CGRectMake(20, 60, viewRect.size.width - 40, 30);
      label.frame = rect;
      //text
      label = (iphoneLabel *)[emptyListView viewWithTag:TAG_TEXT];
      label.text = NULL;
      
      //top image
      imageView = (UIImageView *)[emptyListView viewWithTag:TAG_TOP_IMAGE];
      imageView.image = NULL;
      //bottom image
      imageView = (UIImageView *)[emptyListView viewWithTag:TAG_BOTTOM_IMAGE];
      imageView.image =  NULL;
      
      //button
      button = (UIButton *)[emptyListView viewWithTag:TAG_BUTTON];
      button.hidden = YES;
   } else {
      //top image
      imageView = (UIImageView *)[emptyListView viewWithTag:TAG_TOP_IMAGE];
      imageView.image = roadmap_iphoneimage_load(empty_message->top_image);
      [imageView sizeToFit];
      rect = imageView.frame;
      rect.origin.x = (viewRect.size.width - imageView.bounds.size.width )/2;
      rect.origin.y = yPos;
      imageView.frame = rect;
      if (empty_message->top_image[0] != 0) {
         yPos += rect.size.height;
      } else {
         yPos += 40;
      }
      
      //title
      label = (iphoneLabel *)[emptyListView viewWithTag:TAG_TITLE];
      label.text = [NSString stringWithUTF8String:roadmap_lang_get(empty_message->title)];
      rect = label.frame;
      rect.origin.y = yPos;
      label.frame = rect;
      yPos += rect.size.height;
      
      //text
      label = (iphoneLabel *)[emptyListView viewWithTag:TAG_TEXT];
      label.text = [NSString stringWithUTF8String:roadmap_lang_get(empty_message->text)];
      rect = label.frame;
      rect.origin.y = yPos;
      label.frame = rect;
      yPos += rect.size.height;

      //bottom image
      imageView = (UIImageView *)[emptyListView viewWithTag:TAG_BOTTOM_IMAGE];
      imageView.image = roadmap_iphoneimage_load(empty_message->bottom_image);
      [imageView sizeToFit];
      
      rect = imageView.frame;
      rect.origin.x = (viewRect.size.width - imageView.bounds.size.width )/2;
      rect.origin.y = yPos;
      imageView.frame = rect;
      yPos += rect.size.height +5;
      
      button = (UIButton *)[emptyListView viewWithTag:TAG_BUTTON];
      if (!empty_message->button_cb || empty_message->button_text[0] == 0) {
         button.hidden = YES;
      } else {
         button.hidden = NO;
         [button setTitle:[NSString stringWithUTF8String:roadmap_lang_get(empty_message->button_text)] forState:UIControlStateNormal];
         [button sizeToFit];
         rect = button.frame;
         rect.origin.y = yPos;
         rect.origin.x = (viewRect.size.width - button.bounds.size.width) /2;
         button.frame = rect;
         yPos += rect.size.height + 10;
         emptyListCb = empty_message->button_cb;
      }

   }
   
   emptyListView.contentSize = CGSizeMake(viewRect.size.width, yPos);
   
   emptyListView.hidden = NO;
}


- (void) activateGenericWithTitle:(const char*)title
                         andCount:(int)count
                        andLabels:(const char**)labels
                        andValues:(const void**)values
                         andIcons:(const char**)icons
                   andRightLabels:(const char**)right_labels
                      andSelector:(list_menu_selector*)selector
                andOnItemSelected:(PFN_ON_ITEM_SELECTED)on_item_selected
                 andOnItemDeleted:(PFN_ON_ITEM_SELECTED)on_item_deleted
                       andContext:(void*)context
          andDetailButtonCallback:(SsdSoftKeyCallback)detail_button_callback
                    andListHeight:(int)list_height
               andAddDetailButton:(int)add_next_button
                       andRefresh:(BOOL)refresh
                         andEmpty:(list_menu_empty_message*)empty_message
{
   tableHeight = list_height;
	
	[self setTitle:[NSString stringWithUTF8String:roadmap_lang_get(title)]];
   
   //set right button
	UINavigationItem *navItem = [self navigationItem];
   UIBarButtonItem *barButton = [[UIBarButtonItem alloc] initWithTitle:[NSString stringWithUTF8String:roadmap_lang_get("Close")]
                                                                 style:UIBarButtonItemStyleDone target:self action:@selector(onClose)];
   [navItem setRightBarButtonItem:barButton];
   [barButton release];
	
	listData.on_item_selected = on_item_selected;
	listData.on_item_deleted = on_item_deleted;
   if (selector) {
      listData.selector = *selector;
   } else {
      listData.selector.count = 0;
      listData.selector.on_selector = NULL;
   }
	listData.context = context;
	listData.detail_button_callback = detail_button_callback;
   
	[self populateListDataWithCount:count andLabels:labels andValues:values andIcons:icons andRightLabels:right_labels andAddDetailButton:add_next_button];
   
   if (refresh)
      [self.tableView reloadData];
   else
      roadmap_main_push_view (self);
   
   if (count == 0) {
      [self showEmptyMessage:empty_message];
   } else {
      if (emptyListView)
         emptyListView.hidden = YES;
   }
}

- (void) activateCustomWithTitle:(const char*)title
                        andCount:(int)count
                       andValues:(const void**)values
                     andSelector:(list_menu_selector*)selector
               andOnItemSelected:(PFN_ON_ITEM_SELECTED)on_item_selected
                andOnItemDeleted:(PFN_ON_ITEM_SELECTED)on_item_deleted
         andDetailButtonCallback:(SsdSoftKeyCallback)left_softkey_callback
             andCellViewCallback:(ViewForCellCallback) custom_view_callback
                      andContext:(void*)context
                   andListHeight:(int)list_height
              andAddDetailButton:(BOOL)add_next_button
                      andRefresh:(BOOL)refresh
                        andEmpty:(list_menu_empty_message*)empty_message
{
   tableHeight = list_height;
	
	[self setTitle:[NSString stringWithUTF8String:roadmap_lang_get(title)]];
   
   //set right button
	UINavigationItem *navItem = [self navigationItem];
   UIBarButtonItem *barButton = [[UIBarButtonItem alloc] initWithTitle:[NSString stringWithUTF8String:roadmap_lang_get("Close")]
                                                                 style:UIBarButtonItemStyleDone target:self action:@selector(onClose)];
   [navItem setRightBarButtonItem:barButton];
	
	listData.on_item_selected = on_item_selected;
	listData.on_item_deleted = on_item_deleted;
   if (selector) {
      listData.selector = *selector;
   } else {
      listData.selector.count = 0;
      listData.selector.on_selector = NULL;
   }
	listData.context = context;
	listData.detail_button_callback = left_softkey_callback;
	listData.custom_view_callback = custom_view_callback;
	
	
	[self populateListDataWithCount:count andLabels:NULL andValues:values andIcons:NULL andRightLabels:NULL andAddDetailButton:add_next_button];
	
   if (refresh)
      [self.tableView reloadData];
   else
      roadmap_main_push_view (self);
   
   if (count == 0) {
      [self showEmptyMessage:empty_message];
   } else {
      if (emptyListView)
         emptyListView.hidden = YES;
   }
}

- (void)dealloc
{
	if (dismissListCallback) {
		dismissListCallback(exit_code, NULL);
      dismissListCallback = NULL;
   }
   
	[dataArray release];
	dataArray = NULL;
   
   if (emptyListView != NULL)
      [emptyListView release];
   
   UITableView *tableView = (UITableView *)self.view;
   tableView.delegate = nil;
   tableView.dataSource = nil;

	[super dealloc];
}


//////////////////////
//TableView delegate
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	
    return [dataArray count];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {

	return [[dataArray objectAtIndex:section] count];
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
   
   if (!dataArray)
      return NULL; //This should not happen
	
	NSMutableArray *groupArray = [dataArray objectAtIndex:indexPath.section];
	NSDictionary *dict = (NSDictionary *)[groupArray objectAtIndex:indexPath.row];
	
	if (![dict objectForKey:@"text"])
		cellId = @"customCell";
	else if ([tableView rowHeight] < 60)
		cellId = @"simpleCell";
	else //Tables with high cells get multiline labels
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
         
	} else {
		NSData *data = (NSData *)[dict objectForKey:@"value"];
		void** value = (void**) [data bytes];
      listData.custom_view_callback (*value, cell.bounds, cell.contentView);
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
	
	NSMutableArray *groupArray = [dataArray objectAtIndex:indexPath.section];
	NSDictionary *dict = (NSDictionary *)[groupArray objectAtIndex:indexPath.row];
	
	if (listData.on_item_selected && [dict objectForKey:@"value"]){
		NSString *name = (NSString *)[dict objectForKey:@"text"];
		NSData *data = (NSData *)[dict objectForKey:@"value"];
		const void** value = (const void**) [data bytes];
      if ((int)value != LIST_MENU_NO_ACTION)
         listData.on_item_selected(1, [name UTF8String], *value, listData.context);
	} else {
		if ([dict objectForKey:@"callback"]) {
			NSData *data = (NSData *)[dict objectForKey:@"callback"];
			RoadMapCallback *callback = (RoadMapCallback*) [data bytes];
			(*callback)();
		}
	}
    
   exit_code = dec_yes;
}

- (void)tableView:(UITableView *)tableView accessoryButtonTappedForRowWithIndexPath:(NSIndexPath *)indexPath
{
	NSMutableArray *groupArray = [dataArray objectAtIndex:indexPath.section];
	NSDictionary *dict = (NSDictionary *)[groupArray objectAtIndex:indexPath.row];
	
	if (!listData.detail_button_callback || ![dict objectForKey:@"value"])
		return;

	NSData *data = (NSData *)[dict objectForKey:@"value"];
	const void** value = (const void**) [data bytes];
	
	listData.detail_button_callback (NULL, *value, listData.context);
}

- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
	if (listData.on_item_deleted)
		return YES;
	else
		return NO;
}

- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
{	
	NSMutableArray *groupArray = [dataArray objectAtIndex:indexPath.section];
	NSDictionary *dict = (NSDictionary *)[groupArray objectAtIndex:indexPath.row];
	
	if ([dict objectForKey:@"value"]){
		NSString *name = (NSString *)[dict objectForKey:@"text"];
		NSData *data = (NSData *)[dict objectForKey:@"value"];
		const void** value = (const void**) [data bytes];
		listData.on_item_deleted(1, [name UTF8String], *value, listData.context);
	}
}


@end
