/* iphoneSelect.m - iPhone select custom control
 *
 * LICENSE:
 *
 *   Copyright 2010 Avi R
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

#include "roadmap.h"
#include "roadmap_res.h"
#include "iphoneSelect.h"

#define LABEL_TAG 100

@implementation iphoneSelect

- (void) layoutSubviews
{

}

- (int) getSelectedSegment
{
   return gIndex;
}

- (int) setSelectedSegment:(int)index
{
   int i;
   for (i = 0; i < [self.subviews count]; i++) {
      UIView *view = [self.subviews objectAtIndex:i];
      if (view.tag < 1000)
         continue;
      
      if (view.tag - 1000 == index) {
         [(UIButton *)view setSelected:YES];
         ((UILabel *)[view viewWithTag:LABEL_TAG]).textColor = [UIColor whiteColor];
      } else {
         [(UIButton *)view setSelected:NO];
         ((UILabel *)[view viewWithTag:LABEL_TAG]).textColor = [UIColor blackColor];
      }
   }
}

- (void) onButtonTouch: (id) sender
{
   int index = ((UIButton *)sender).tag - 1000;
   int i;
   
   for (i = 0; i < [self.subviews count]; i++) {
      UIView *view = [self.subviews objectAtIndex:i];
      if (view.tag < 1000)
         continue;
      
      if (view.tag - 1000 == index) {
         [(UIButton *)view setSelected:YES];
         ((UILabel *)[view viewWithTag:LABEL_TAG]).textColor = [UIColor whiteColor];
      } else {
         [(UIButton *)view setSelected:NO];
         ((UILabel *)[view viewWithTag:LABEL_TAG]).textColor = [UIColor blackColor];
      }
   }
   
   gIndex = index;
   
   if (gDelegate)
      [gDelegate performSelector:@selector(onSegmentChange:) withObject:self];
}

- (void) createWithItems:(NSArray*) segmentsArray selectedSegment:(int)index delegate:(id) delegate type:(int)type{
   int i;
   int count;
   UIImage *bg_left, *bg_mid, *bg_right;
   UIImage *bg_left_s, *bg_mid_s, *bg_right_s;
   UIImage *image;
   UIButton *button;
   CGFloat x_offset = 0.0f;
   CGRect rect;
   NSDictionary *dict;
   UILabel *label;
   
   if (!segmentsArray) {
      roadmap_log (ROADMAP_ERROR, "Bad segments array !");
      return;
   }
   
   count = [segmentsArray count];
   
   if (count <= 1) {
      roadmap_log (ROADMAP_ERROR, "Bad number of items in segments array !");
      return;
   }
   
   if (type == IPHONE_SELECT_TYPE_TEXT_IMAGE) {
      bg_left = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "button_sc_icon_left");
      bg_left_s = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "button_sc_icon_left_s");
      bg_mid = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "button_sc_icon_mid");
      bg_mid_s = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "button_sc_icon_mid_s");
      bg_right = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "button_sc_icon_right");
      bg_right_s = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "button_sc_icon_right_s");
   } else if (count >= 3) {
      bg_left = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "button_sc_3_left");
      bg_left_s = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "button_sc_3_left_s");
      bg_mid = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "button_sc_3_mid");
      bg_mid_s = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "button_sc_3_mid_s");
      bg_right = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "button_sc_3_right");
      bg_right_s = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "button_sc_3_right_s");
   } else {
      bg_left = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "button_sc_2_left");
      bg_left_s = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "button_sc_2_left_s");
      bg_mid = NULL;
      bg_mid_s = NULL;
      bg_right = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "button_sc_2_right");
      bg_right_s = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "button_sc_2_right_s");
   }

   
   for (i = 0; i < count; i++) {
      button = [UIButton buttonWithType:UIButtonTypeCustom];
      if (i == 0) {
         [button setBackgroundImage:bg_left forState:UIControlStateNormal];
         [button setBackgroundImage:bg_left_s forState:UIControlStateSelected];
         [button setBackgroundImage:bg_left_s forState:UIControlStateHighlighted];
      } else if (i == count -1) {
         [button setBackgroundImage:bg_right forState:UIControlStateNormal];
         [button setBackgroundImage:bg_right_s forState:UIControlStateSelected];
         [button setBackgroundImage:bg_right_s forState:UIControlStateHighlighted];
      } else {
         [button setBackgroundImage:bg_mid forState:UIControlStateNormal];
         [button setBackgroundImage:bg_mid_s forState:UIControlStateSelected];
         [button setBackgroundImage:bg_mid_s forState:UIControlStateHighlighted];
      }
      
      [button setTitleColor:[UIColor blackColor] forState:UIControlStateNormal];
      [button setTitleColor:[UIColor whiteColor] forState:UIControlStateSelected];
      //[button setTitleEdgeInsets:UIEdgeInsetsMake(60, -40, 5, 0)];
      [button setImageEdgeInsets:UIEdgeInsetsMake(0, 0, 30, 0)];
      UIImageView *imageView = button.imageView;
      rect = imageView.bounds;
      rect.origin.x = 100;
      rect.origin.y = 20;
      imageView.frame = rect;
      [button addTarget:self action:@selector(onButtonTouch:) forControlEvents:UIControlEventTouchUpInside];
      button.tag = 1000 + i;
      
      [button sizeToFit];
      rect = button.bounds;
      rect.origin.x = x_offset;
      button.frame = rect;
      x_offset += button.bounds.size.width;
      
      dict = [segmentsArray objectAtIndex:i];

      if (type == IPHONE_SELECT_TYPE_TEXT)
         rect = CGRectMake(5, 5, button.bounds.size.width - 10, button.bounds.size.height - 10);
      else
         rect = CGRectMake(5, button.bounds.size.height - 30, button.bounds.size.width - 10, 25);
      label = [[UILabel alloc] initWithFrame:rect];
      label.textAlignment = UITextAlignmentCenter;
      label.adjustsFontSizeToFitWidth = YES;
      label.tag = LABEL_TAG;
      label.backgroundColor = [UIColor clearColor];
      [button addSubview:label];
      [label release];
      if ([dict objectForKey:@"text"]) {
         //[button setTitle:[dict objectForKey:@"text"] forState:UIControlStateNormal];
         label.text = [dict objectForKey:@"text"];
      }
      if ([dict objectForKey:@"icon"]) {
         image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, [(NSString *)[dict objectForKey:@"icon"] UTF8String]);
         [button setImage:image forState:UIControlStateNormal];
      }
      if ([dict objectForKey:@"icon_sel"]) {
         image = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, [(NSString *)[dict objectForKey:@"icon_sel"] UTF8String]);
         [button setImage:image forState:UIControlStateSelected];
         [button setImage:image forState:UIControlStateHighlighted];
      }
      
      if (index == i) {
         button.selected = YES;
         label.textColor = [UIColor whiteColor];
      } else {
         label.textColor = [UIColor blackColor];
      }

      [self addSubview:button];
      self.userInteractionEnabled = YES;
   }
   rect = CGRectZero;
   rect.size.width = x_offset;
   rect.size.height = button.bounds.size.height;
   self.frame = rect;
   
   gDelegate = delegate;
   gIndex = index;
}

- (void)dealloc
{
   [super dealloc];
}


@end
