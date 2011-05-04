/* roadmap_iphonesearch_dialog.h - iPhone main search dialog
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

#ifndef __ROADMAP_IPHONESEARCH_DIALOG__H
#define __ROADMAP_IPHONESEARCH_DIALOG__H

#include "ssd/ssd_list.h"
#include "ssd/ssd_generic_list_dialog.h"

@interface SingleSearchResultsDialog : UITableViewController {
	NSMutableArray				*dataArray;
   NSMutableArray				*headersArray;
   BOOL                    minimized[3];
   PFN_ON_ITEM_SELECTED    g_on_item_selected;
   SsdSoftKeyCallback      g_detail_button_callback;
   const char              *g_fav_name;
}

@property (nonatomic, retain) NSMutableArray	*dataArray;
@property (nonatomic, retain) NSMutableArray	*headersArray;

- (void) showResults:(const char**)results
             indexes:(void**)indexes
            countAdr:(int)count_adr
             countLs:(int)count_ls
             countAb:(int)count_ab
      onItemSelected:(PFN_ON_ITEM_SELECTED)on_item_selected
      onDetailButton:(SsdSoftKeyCallback)detail_button_callback
             favName:(const char*)fav_name;

@end


@interface CoverView : UIView {
   id    delegate;
}

@property (nonatomic, retain) id    delegate;

@end

@interface SearchDialog : UIViewController <UITableViewDelegate, UITableViewDataSource, UISearchBarDelegate> {
	NSMutableArray				*dataArray;
   NSMutableArray				*headersArray;
   UIView                  *gSearchView;
   UITableView             *gTableView;
   CoverView               *gCoverView;
   void                    **fav_values;
   int                     fav_count;
   SsdListCallback         fav_callback;
}

@property (nonatomic, retain) NSMutableArray	*dataArray;
@property (nonatomic, retain) NSMutableArray	*headersArray;
@property (nonatomic, retain) UIView         *gSearchView;
@property (nonatomic, retain) UITableView    *gTableView;

- (void) show;
- (void) cancelSearch;

@end


@interface AddHomeWorkDialog : UIViewController <UISearchBarDelegate> {
   UIView                  *gSearchView;
   int                     g_iType;
}

@property (nonatomic, retain) UIView         *gSearchView;

- (void) showWithType: (int)iType;
- (void) cancelSearch;

@end

#endif // __ROADMAP_IPHONESEARCH_DIALOG__H