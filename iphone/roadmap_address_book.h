/* roadmap_address_book.h - RoadMap address book
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi B.
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

#ifndef __ROADMAP_ADDRESS_BOOK__H
#define __ROADMAP_ADDRESS_BOOK__H

#include "roadmap_factory.h"
#include "roadmap_editbox.h"

#import <UIKit/UIKit.h>

#import <AddressBookUI/AddressBookUI.h>

@interface RoadmapAddressBook : UIViewController <ABPeoplePickerNavigationControllerDelegate> {
	SsdKeyboardCallback gCallback;
	void * gContext;
}
@property (nonatomic) SsdKeyboardCallback gCallback;
@property (nonatomic) void * gContext;
- (void)pickname:(SsdKeyboardCallback) callback
	  andContext: (void *)context;

@end
#endif //ROADMAP_ADDRESS_BOOK