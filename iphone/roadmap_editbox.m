/* roadmap_editbox.m - Handle iphone editbox views
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
#include "roadmap_device_events.h"
#include "roadmap_editbox.h"
#include "roadmap_lang.h"
#include "roadmap_keyboard.h"
#include "roadmap_iphoneeditbox.h"



void ShowEditbox(const char* aTitleUtf8, 
                 const char* aTextUtf8, 
                 SsdKeyboardCallback callback, 
                 void *context, 
                 TEditBoxType aBoxType ) {
   
	RoadMapEditBox *RoadMapEditViewCont;
	
	RoadMapEditViewCont = [[RoadMapEditBox alloc] initWithStyle:UITableViewStyleGrouped];
	
	[RoadMapEditViewCont showWithTitle:aTitleUtf8
							   andText:aTextUtf8 
						   andCallback:callback 
							andContext:context
							   andType:aBoxType];
}


@implementation EditBoxCell

- (void) layoutSubviews {
   CGRect rect = self.bounds;
   rect.origin.x += 20;
   rect.origin.y += 10;
   rect.size.width -= 40;
   rect.size.height -= 20;
   
   [self.contentView viewWithTag:1].frame = rect;
}

- (void)setText: (UITextView *) textView {
   textView.tag = 1;
	[[self contentView] addSubview: textView];

	[textView becomeFirstResponder];
}

- (void)setSelected:(BOOL)selected animated:(BOOL)animated {
	//[self bringSubviewToFront:textView];
	[[self.contentView viewWithTag:1] becomeFirstResponder];
}

@end



@implementation RoadMapEditBox
@synthesize gTextView;
@synthesize gContext;
@synthesize gCallback;
@synthesize gBoxType;
@synthesize gDoneButton;
@synthesize gCell;

- (void) viewDidLoad
{
	UITableView *tableView = [self tableView];
	[tableView setRowHeight:120.0f];
   roadmap_main_set_table_color(tableView);
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
   return roadmap_main_should_rotate (interfaceOrientation);
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
   roadmap_device_event_notification( device_event_window_orientation_changed);
}


- (void) onDone
{
	if ((gBoxType != EEditBoxEmptyForbidden) || ([[gTextView text] length] > 0)) {
		[gTextView resignFirstResponder];
		gCallback (1, [[gTextView text] UTF8String] , gContext);
	}
}

- (void) showWithTitle: (const char*) aTitleUtf8
			   andText: (const char*) aTextUtf8
		   andCallback: (SsdKeyboardCallback) callback
			andContext: (void *)context
			   andType: (TEditBoxType) aBoxType
{
	[self setTitle:[NSString stringWithUTF8String:aTitleUtf8]];
	
	UINavigationItem *navItem = [self navigationItem];
   gDoneButton = [[UIBarButtonItem alloc] initWithTitle:[NSString stringWithUTF8String:roadmap_lang_get("Done")]
                                                  style:UIBarButtonItemStyleDone target:self action:@selector(onDone)];
	[navItem setRightBarButtonItem:gDoneButton];
	[gDoneButton release];
	if (gBoxType == EEditBoxEmptyForbidden)
			[gDoneButton setEnabled:NO];
	
	gTextView = [[UITextView alloc] init];
	[gTextView setDelegate:self];
	[gTextView setFont: [UIFont systemFontOfSize:24.0f]];
	if (aTextUtf8)
		[gTextView setText:[NSString stringWithUTF8String:aTextUtf8]];
	
	UITableView *tableView = [self tableView];
	[tableView setRowHeight:120.0f];

	
	if (aBoxType == EEditBoxNumeric)
		[gTextView setKeyboardType:UIKeyboardTypeNumberPad];
   
   gCell = [[EditBoxCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"simpleCell"];
	[gCell setText: gTextView];
   [gCell setBackgroundColor:[UIColor whiteColor]];
	
	gContext = context;
	gCallback = callback;
	gBoxType = aBoxType;
	
	roadmap_main_push_view(self);
}


//Table view delegate
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	return 1;
}
	
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	return gCell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	[gTextView becomeFirstResponder];
}

- (void)dealloc
{
	//Add all items to release here
	[gTextView release];
   [gCell release];
	
	[super dealloc];
}


//Text view delegate

- (void)textViewDidChange:(UITextView *)textView
{
   if (gBoxType == EEditBoxEmptyForbidden) {
		if ([[textView text] length] > 0)
			[gDoneButton setEnabled:YES];
		else
			[gDoneButton setEnabled:NO];
	}
}

- (BOOL)textView:(UITextView *)textView shouldChangeTextInRange:(NSRange)range replacementText:(NSString *)text
{	
	if (roadmap_keyboard_typing_locked(TRUE)) {
      [textView resignFirstResponder];
      return NO;
   } else
      return YES;
}

@end
