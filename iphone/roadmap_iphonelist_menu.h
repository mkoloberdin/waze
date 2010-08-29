/* roadmap_iphonelist_menu.h - iPhone menu view
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi R.
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

#ifndef __ROADMAP_IPHONELIST_MENU__H
#define __ROADMAP_IPHONELIST_MENU__H

typedef void (*ViewForCellCallback) (void *value, CGRect viewRect, UIView *parentView);

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
                                       list_menu_empty_message* empty_message);

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
                                list_menu_empty_message* empty_message);



@interface RoadMapListMenu : UITableViewController {
	PFN_ON_DIALOG_CLOSED		dismissListCallback;
   int                     exit_code;
   int                     tableHeight;
	NSMutableArray				*dataArray;
   UIScrollView            *emptyListView;
   RoadMapCallback         emptyListCb;
	struct {
		PFN_ON_ITEM_SELECTED on_item_selected;
		PFN_ON_ITEM_SELECTED on_item_deleted;
      list_menu_selector   selector;
		void* context;
		SsdSoftKeyCallback detail_button_callback;
		ViewForCellCallback custom_view_callback;
	} listData;
}

@property (nonatomic) PFN_ON_DIALOG_CLOSED		dismissListCallback;
@property (nonatomic, retain) NSMutableArray    *dataArray;
@property (nonatomic, retain) UIView            *emptyListView;

- (void) activateSimpleWithName:(const char           *)name
                   andItemsFile:(const char           *) items_file
                       andItems:(const char           **)items
              andAdditionalItem:(const char           *) additional_item
                andCustomLabels:(const char           **)custom_labels
                 andCustomIcons:(const char           **)custom_icons
              andOnDialogClosed:(PFN_ON_DIALOG_CLOSED) on_dialog_closed
                     andActions:(const RoadMapAction  *) actions;

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
                         andEmpty:(list_menu_empty_message*)empty_message;

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
                        andEmpty:(list_menu_empty_message*)empty_message;

@end


#endif // __ROADMAP_IPHONELIST_MENU__H