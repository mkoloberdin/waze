/* iphoneCellEdit.m - Create edit box cell for iPhone table
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

#include "iphoneCellEdit.h"

#define X_MARGIN 10.0f
#define Y_MARGIN 5.0f


@implementation iphoneCellEdit
@synthesize labelView;
@synthesize textView;
@synthesize enableCell;

- (id)initWithFrame:(CGRect)frame reuseIdentifier:(NSString *)reuseIdentifier {
	self = [super initWithFrame:frame reuseIdentifier:reuseIdentifier];
	
	labelView = [[iphoneLabel alloc] initWithFrame:frame];
	[labelView setFont:[UIFont boldSystemFontOfSize:17]];
	[labelView setNumberOfLines:2];
	[[self contentView] addSubview:labelView];
	
	textView = [[UITextField alloc] initWithFrame:frame];
	[textView setBorderStyle:UITextBorderStyleRoundedRect];
	[textView setClearButtonMode:UITextFieldViewModeWhileEditing];
	[textView setAutocorrectionType:UITextAutocorrectionTypeNo];
	[textView setAutocapitalizationType:UITextAutocapitalizationTypeNone];
	[textView setReturnKeyType:UIReturnKeyDone];
	[[self contentView] addSubview:textView];
   
   [self setSelectionStyle:UITableViewCellSelectionStyleNone];
	[self setBackgroundColor:[UIColor whiteColor]];
   
	return self;
}

- (void) layoutSubviews {
	[super layoutSubviews];
	
	if (!labelView || !textView)
		return;
	
	CGRect frame = [[self contentView] bounds];
	CGRect rect;
	
	rect = CGRectMake(X_MARGIN, Y_MARGIN, (frame.size.width/2) - X_MARGIN*1.5, frame.size.height - Y_MARGIN*2);
	[labelView setFrame:rect];
	
	if ([[labelView text] length] == 0)
		rect.size.width = frame.size.width - X_MARGIN*2;
	else
		rect.origin.x += rect.size.width + X_MARGIN;
	
	[textView setFrame:rect];
}

- (void) setText:(NSString *) text {
	[textView setText:text];
}

- (void) setLabel:(NSString *) text {
	[labelView setText:text];
}

- (void) setPassword:(BOOL) set {
	[textView setSecureTextEntry:set];
}

- (void) setEnableCell:(BOOL) set {
	[textView setEnabled:set];
	[labelView setEnabled:set];
}

- (NSString *) getText {
	return [textView text];
}

- (void) setPlaceholder:(NSString *) text {
	[textView setPlaceholder:text];
}

- (void) setDelegate:(id) delegate {
	[textView setDelegate:delegate];
}

- (void)setSelected:(BOOL)selected animated:(BOOL)animated {
	[[[self subviews] objectAtIndex:0] becomeFirstResponder];
}

- (void) hideKeyboard {
	[textView resignFirstResponder];
}

- (void)dealloc
{
	[labelView release];
	[textView release];
	
	[super dealloc];
}

@end
