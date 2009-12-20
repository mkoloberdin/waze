/* roadmap_iphonemessagebox.h - iPhone messagebox view
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

#ifndef __ROADMAP_IPHONEMESSAGEBOX__H
#define __ROADMAP_IPHONEMESSAGEBOX__H


@interface RoadMapMessageBoxView : UIView {
   UIView         *messageView;
   UIImageView    *backgroundImage;
   UILabel        *titleLabel;
   UILabel        *contentLabel;
   UIButton       *okButton;
   int            minHeight;
   int            minWidth;
   NSTimer        *okTimer;
   int            gTimeout;
}

@property (nonatomic, retain) UIView         *messageView;
@property (nonatomic, retain) UIImageView    *backgroundImage;
@property (nonatomic, retain) UILabel        *titleLabel;
@property (nonatomic, retain) UILabel        *contentLabel;
@property (nonatomic, retain) UIButton       *okButton;
@property (nonatomic, retain) NSTimer        *okTimer;

- (void) onOkClicked;
- (void) showWithTitle: (const char*) aTitleUtf8
			   andContent: (const char*) aTextUtf8
			andTimeout: (int) timeout;

@end


#endif // __ROADMAP_IPHONEMESSAGEBOX__H