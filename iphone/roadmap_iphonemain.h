/* roadmap_iphonemain.h
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
 *   Copyright 2008 Morten Bek Ditlevsen
 *   Copyright 2008 Ehud Shabtai
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
 *
 * DESCRIPTION:
 *
 *   This defines a toolkit-independent interface for the canvas used
 *   to draw the map. Even while the canvas implementation is highly
 *   toolkit-dependent, the interface must not.
 */

#ifndef INCLUDE__ROADMAP_IPHONE_MAIN__H
#define INCLUDE__ROADMAP_IPHONE_MAIN__H

#import <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>
#import <UIKit/UIWindow.h>
#import <UIKit/UIView.h>
#import <UIKit/UIKit.h>
#import <UIKit/UIApplication.h>
#import <UIKit/UIImageView.h>
#import <UIKit/UIImage.h>
#import <UIKit/UITextView.h>
#import <UIKit/UIDevice.h>


#include <UIKit/UIViewController.h>

void roadmap_main_present_modal (UIViewController *modalController);
void roadmap_main_dismiss_modal (void);
void roadmap_main_push_view (UIViewController *modalController);
void roadmap_main_show_view (UIView *View);
BOOL roadmap_main_should_rotate(UIInterfaceOrientation interfaceOrientation);
void roadmap_main_get_bounds(CGRect *bounds);
UIImageView *roadmap_main_bg_image (void);
UIColor *roadmap_main_table_color (void);



@interface RoadMapView : UIView {
}


@end


@interface RoadMapViewController : UIViewController {

}

@end


@interface RoadMapApp : UIApplication {

}


-(void) presentModalView: (UIViewController*) modalController;
-(void) dismissModalView;
-(void) pushView: (UIViewController*) modalController;
-(void) popView: (BOOL) animated;
-(void) showRoot: (BOOL) animated;
-(void) newWithTitle: (const char *)title andWidth: (int) width andHeight: (int) height;
-(void) inputCallback: (id) notify;
-(void) setInput: (RoadMapIO*) io andCallback: (RoadMapInput) callback;
-(void) removeIO: (RoadMapIO*) io;
-(void) periodicCallback: (NSTimer *) nstimer;
-(void) setPeriodic: (float) interval andCallback: (RoadMapCallback) callback;
-(void) removePeriodic: (RoadMapCallback) callback;
-(void) playMovie: (const char *)url;


@end

#endif // INCLUDE__ROADMAP_IPHONE_MAIN__H

