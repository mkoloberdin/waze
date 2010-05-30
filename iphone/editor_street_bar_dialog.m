/* editor_street_bar_dialog.h - iPhone dialog for editor street bar
 *
 * LICENSE:
 *
 *   Copyright 2010 Avi R.
 *   Copyright 2010, Waze Ltd
 *
 *   This file is part of RoadMap.
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
 *
 *
 */
 

#include "roadmap_main.h"
#include "roadmap_iphonemain.h"
#include "roadmap_lang.h"
#include "roadmap_canvas.h"
#include "roadmap_iphonecanvas.h"
#include "roadmap_iphoneimage.h"
#include "editor/editor_main.h"
#include "editor/editor_screen.h"
#include "editor/db/editor_street.h"
#include "editor/db/editor_line.h"
#include "editor/db/editor_override.h"
#include "editor/export/editor_report.h"
#include "editor/db/editor_marker.h"
#include "editor/static/update_range.h"
#include "editor/track/editor_track_main.h"
#include "roadmap_locator.h"
#include "roadmap_square.h"
#include "roadmap_layer.h"
#include "roadmap_line_route.h"
#include "roadmap_line.h"
#include "roadmap_navigate.h"
#include "roadmap_county.h"
#include "roadmap_messagebox.h"
#include "roadmap_config.h"
#include "roadmap_map_settings.h"
#include "roadmap_sound.h"
#include "roadmap_res.h"
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_confirm_dialog.h"
#include "editor/static/editor_street_bar.h"
#include "editor_street_bar_dialog.h"



#define ROW_H              23
#define HOUSE_BOX_W        50
#define SECONDARY_OFFSET1_Y -4
#define SECONDARY_OFFSET2_Y 33
#define SECONDARY_OFFSET_X 0
#define SECONDARY_H       67
#define CITY_W             35
#define DIR_W              55
#define SAVE_H             20
#define SAVE_W             40
#define BTN_W              65


enum view_tags {
   ID_SECONDARY = 1,
   ID_CITY_TITLE,
   ID_CITY_EDT,
   ID_TYPE_BTN,
   ID_TYPE_PCK,
   ID_DIR_BTN,
   ID_LEFT,
   ID_RIGHT,
   ID_LEFT_ARROW,
   ID_RIGHT_ARROW,
   ID_STREET,
   ID_MORE_BTN,
   ID_SAVE_BTN,
   ID_CANCEL_BTN,
   ID_X_BTN,
   ID_SEC_BG,
   ID_MAIN_BG,
   ID_SAME_STREET
};

enum sec_states {
   STATE_HIDDEN,
   STATE_PARTIAL,
   STATE_FULL,
   STATE_SAME_NAME
};




static EditorStreetBarView *gs_streetBarView = NULL;
static BOOL                gs_isBarShown = FALSE;
static BOOL                gs_isEnabledInSession = TRUE;





extern const char *editor_segments_find_city (int line, int plugin_id, int square);




static BOOL editor_street_bar_enabled (void) {
   return (gs_isEnabledInSession && 
           editor_track_shortcut() == 3 &&
           roadmap_map_settings_isAutoShowStreetBar());
}

static void set_enable_in_session (BOOL value) {
   gs_isEnabledInSession = value;
}


////////////////////////////////////////////////////////////////////////////////
void on_save_confirm (int exit_code, void *context){
   if (exit_code == dec_yes){
      [gs_streetBarView onSaveConfirmed];
   } else {
      [gs_streetBarView onClose];
   }
   
}

////////////////////////////////////////////////////////////////////////////////
void editor_street_bar_display (SelectedLine *lines, int lines_count) {
   if (gs_isBarShown)
      return;
   
   if (gs_streetBarView == NULL)
      gs_streetBarView = [[EditorStreetBarView alloc] initWithFrame:CGRectZero];
   
   
   gs_isBarShown = TRUE;
   [gs_streetBarView showStreet:&lines[0]]; //Currently only single selection supported
}

////////////////////////////////////////////////////////////////////////////////
void editor_street_bar_track (PluginLine *line, const RoadMapGpsPosition *gps_position, BOOL lock) {
   if (!editor_street_bar_enabled() && !lock)
      return;
   
   if (gs_streetBarView == NULL)
      gs_streetBarView = [[EditorStreetBarView alloc] initWithFrame:CGRectZero];
   
   
   gs_isBarShown = TRUE;
   [gs_streetBarView showTracking:line andPos:gps_position andLock:lock];
}

////////////////////////////////////////////////////////////////////////////////
void editor_street_bar_update_properties (void) {
   PluginLine line;
   int direction;
   RoadMapGpsPosition gps_position;
   
   roadmap_main_show_root(0);
   
   if (gs_isBarShown)
      return;
   
   if (roadmap_navigate_get_current
       (&gps_position, &line, &direction) == -1)
      return;
   
   editor_street_bar_track(&line, &gps_position, TRUE);
}

////////////////////////////////////////////////////////////////////////////////
void editor_street_bar_stop (void) {
   [gs_streetBarView stopTracking];
}

////////////////////////////////////////////////////////////////////////////////
BOOL editor_street_bar_active (void) {
   return [gs_streetBarView isActive];
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
@implementation EditorStreetBarView

- (id)initWithFrame:(CGRect)aRect
{
   self = [super initWithFrame:aRect];

   if (self != nil) {
      isCreated = FALSE;
      isEditing = FALSE;
   }

   self.autoresizesSubviews = YES;
   self.autoresizingMask = UIViewAutoresizingFlexibleWidth;
   
   return self;
}

- (BOOL) isActive
{
   return (self.hidden == NO);
}

- (void) resizeViews
{
   
   CGRect rect;
   
   rect = self.frame;
   rect.size.width = 320;
   self.frame = rect;
   
   //picker location / size
   roadmap_main_get_bounds(&rect);
   int main_height = rect.size.height;
   rect = typePicker.frame;
   if (roadmap_canvas_width() == 320)
      rect.size = CGSizeMake(320, 216);
   else
      rect.size = CGSizeMake(480, 162);

   rect.origin.y = main_height - rect.size.height;
   typePicker.frame = rect;
   [typePicker reloadAllComponents];
   [typePicker selectRow:cfcc - ROADMAP_ROAD_FIRST inComponent:0 animated:NO];
}
- (void)layoutSubviews
{
	[super layoutSubviews];
   
   [self resizeViews];
}

- (void) hideKeyboard
{
   UITextField *textField;
   
   textField = (UITextField *)[self viewWithTag:ID_LEFT];
   if (textField)
      [textField resignFirstResponder];
   
   textField = (UITextField *)[self viewWithTag:ID_STREET];
   if (textField)
      [textField resignFirstResponder];
   
   textField = (UITextField *)[self viewWithTag:ID_RIGHT];
   if (textField)
      [textField resignFirstResponder];
   
   textField = (UITextField *)[self viewWithTag:ID_CITY_EDT];
   if (textField)
      [textField resignFirstResponder];
}


- (void) animateMore
{
   UIView *secView = [self viewWithTag:ID_SECONDARY];
   CGRect rect = secView.frame;
   
   [UIView beginAnimations:NULL context:NULL];
   [UIView setAnimationDuration:0.5f];
   [UIView setAnimationCurve:UIViewAnimationCurveEaseInOut];
   
   rect.origin.y = SECONDARY_OFFSET2_Y;
   secView.frame = rect;
   
   rect = self.frame;
   rect.size.height = secView.frame.origin.y + secView.frame.size.height;
   self.frame = rect;
   
   [UIView commitAnimations];
}


- (void) setSecondary: (int) newState
{
   UIView *secView = [self viewWithTag:ID_SECONDARY];
   CGRect rect = secView.frame;
   UIButton *cancelTabBtn = (UIButton *)[self viewWithTag:ID_X_BTN];
   UILabel *cityTitle = (UILabel *)[secView viewWithTag:ID_CITY_TITLE];
   UITextField *cityEdt = (UITextField *)[secView viewWithTag:ID_CITY_EDT];
   UIButton *moreBtn = (UIButton *)[secView viewWithTag:ID_MORE_BTN];
   UIButton *typeBtn = (UIButton *)[secView viewWithTag:ID_TYPE_BTN];
   UIButton *dirBtn = (UIButton *)[secView viewWithTag:ID_DIR_BTN];
   UILabel *sameStreetTitle = (UILabel *)[self viewWithTag:ID_SAME_STREET];
   UITextField *streetEdt = (UITextField *)[self viewWithTag:ID_STREET];
   char str[256];
   
   if (newState == secondaryState)
      return;
   
   switch (newState) {
      case STATE_HIDDEN:
         //hide secondary
         secView.hidden = YES;
         //show X tab
         cancelTabBtn.hidden = NO;
         //hide city
         cityTitle.hidden = YES;
         cityEdt.hidden = YES;
         cityEdt.enabled = NO;
         //show more btn
         moreBtn.hidden = NO;
         moreBtn.enabled = YES;
         //disable street type and direction
         typeBtn.enabled = NO;
         dirBtn.enabled = NO;
         //Reset secondary position
         rect.origin.y = SECONDARY_OFFSET1_Y;
         secView.frame = rect;
         //hide same street label
         sameStreetTitle.hidden = YES;
         
         secondaryState = STATE_HIDDEN;
         break;
      case STATE_PARTIAL:
         //show secondary
         secView.hidden = NO;
         //hide X tab
         cancelTabBtn.hidden = YES;
         
         secondaryState = STATE_PARTIAL;
         break;
      case STATE_SAME_NAME:
         //show secondary
         secView.hidden = NO;
         //hide X tab
         cancelTabBtn.hidden = YES;
         
         //show same street question
         snprintf(str, sizeof(str), "%s %s?",
                  roadmap_lang_get("Is this still"),
                  [streetEdt.text UTF8String]);
         sameStreetTitle.text = [NSString stringWithUTF8String:str];
         sameStreetTitle.hidden = NO;
         //hide more btn
         moreBtn.hidden = YES;
         moreBtn.enabled = NO;
         
         secondaryState = STATE_SAME_NAME;
         break;
      case STATE_FULL:
         //show city
         cityTitle.hidden = NO;
         cityEdt.hidden = NO;
         cityEdt.enabled = YES;
         //hide more btn
         moreBtn.hidden = YES;
         moreBtn.enabled = NO;
         //enable street type and direction
         typeBtn.enabled = YES;
         if (saved_dir != ROUTE_DIRECTION_ANY)
            dirBtn.enabled = YES;
         
         secondaryState = STATE_FULL;
         
         [self animateMore];
         break;
      default:
         break;
   }
}

- (void) setPos
{   
   RoadMapNeighbour result;
   PluginLine line;
   
   //isEditing = TRUE;
   
   
   if (!isTracking)
      return;

   if (roadmap_navigate_get_current
       (&CurrentGpsPoint, &line, &CurrentDirection) == -1)
      return;

   if (!roadmap_plugin_get_distance
       ((RoadMapPosition *)&CurrentGpsPoint, &selectedLine, &result))
      return;
   
   CurrentFixedPosition = result.intersection;
   
   
   
}

- (void) setPosHighlight
{
   [self setPos];
   editor_screen_reset_selected ();
   editor_screen_select_line(&selectedLine);
}

- (BOOL) getRangeLeft: (int *)leftOut right:(int *)rightOut
{
   int fraddl;
   int toaddl;
   int fraddr;
   int toaddr;
   
   
   if (!isTracking)
      return FALSE;
   
   if (selectedLine.plugin_id == EditorPluginID)
      return FALSE;
   
   RoadMapStreetProperties properties;
   
   if (roadmap_locator_activate (selectedLine.fips) < 0)
      return FALSE;
   
   roadmap_street_get_properties (selectedLine.line_id, &properties);
   roadmap_street_get_street_range (&properties, ROADMAP_STREET_LEFT_SIDE, &fraddl, &toaddl);
   roadmap_street_get_street_range (&properties, ROADMAP_STREET_RIGHT_SIDE, &fraddr, &toaddr);
   
   int total_length;
   int point_length;
   double rel;
   int left;
   int right;
   
   point_length = roadmap_plugin_calc_length (&CurrentFixedPosition, &selectedLine, &total_length);
   
   rel = 1.0 * point_length / total_length;
   
   left  = fraddl + (int) ((toaddl - fraddl + 1) / 2 * rel) * 2;
   right = fraddr + (int) ((toaddr - fraddr + 1) / 2 * rel) * 2;
   
   if (CurrentDirection == ROUTE_DIRECTION_AGAINST_LINE) {
      int tmp = left;
      
      left = right;
      right = tmp;
   }
   
   if (right < 1 || left < 1)
      return FALSE;
   
   *rightOut = right;
   *leftOut = left;
   
   return TRUE;
}

- (void) refreshData
{
   const char *this_name = "";
   const char *this_type = "";
   const char *this_city = "";
   const char *this_t2s = "";
   int this_direction;
   cfcc = ROADMAP_ROAD_LAST;
   int left, right;
   char city[512];
   
   UITextField *textField;
   UIButton *typeButton;
   UIButton *dirButton;
   UIButton *moreButton;
   UIImage *image;
   UIImageView *imageV;
   
   if (selectedLine.plugin_id == EditorPluginID) {
      if (editor_db_activate (selectedLine.fips) != -1) {
         
         int street_id = -1;
         
         editor_line_get_street (selectedLine.line_id, &street_id);
         
         this_name = editor_street_get_street_fename (street_id);
         this_type = editor_street_get_street_fetype (street_id);
         this_t2s = editor_street_get_street_t2s (street_id);
         this_city = editor_street_get_street_city (street_id);
         editor_line_get_direction (selectedLine.line_id, &this_direction);
      }
   } else { 
      
      RoadMapStreetProperties properties;
      
      if (roadmap_locator_activate (selectedLine.fips) < 0) {
         return;
      }
      
      roadmap_square_set_current (selectedLine.square);
      roadmap_street_get_properties (selectedLine.line_id, &properties);
      
      this_name = roadmap_street_get_street_name (&properties);
      this_type = roadmap_street_get_street_fetype (&properties);
      this_t2s = roadmap_street_get_street_t2s (&properties);
      this_city = roadmap_street_get_street_city (&properties, ROADMAP_STREET_LEFT_SIDE);
      
      if (editor_override_line_get_direction (selectedLine.line_id, selectedLine.square, &this_direction)) {
         
         this_direction = roadmap_line_route_get_direction (selectedLine.line_id, ROUTE_CAR_ALLOWED);
      }
   }
   
   if (!strlen(this_city)) {
      this_city = editor_segments_find_city
         (selectedLine.line_id, selectedLine.plugin_id, selectedLine.square);
   }
   
   if (!this_t2s)
      this_t2s = "";
   strncpy_safe (street_t2s, this_t2s, sizeof(street_t2s));
   if (!this_type)
      this_type = "";
   strncpy_safe (street_type, this_type, sizeof(street_type));
   
   if (selectedLine.cfcc < cfcc) {
      cfcc = selectedLine.cfcc;
   }
   
   
   textField = (UITextField *)[self viewWithTag:ID_STREET];
   if (!this_name)
      this_name = "";
   textField.text = [NSString stringWithUTF8String:this_name];
   
   moreButton = (UIButton *)[self viewWithTag:ID_MORE_BTN];
   snprintf(city, sizeof(city), "%s: %s", roadmap_lang_get("City"), this_city);
   [moreButton setTitle:[NSString stringWithUTF8String:city] forState:UIControlStateNormal];
   
   textField = (UITextField *)[self viewWithTag:ID_CITY_EDT];
   textField.text = [NSString stringWithUTF8String:this_city];
   
   typeButton = (UIButton *)[self viewWithTag:ID_TYPE_BTN];
   [typeButton setTitle:[NSString stringWithUTF8String:roadmap_lang_get(categories[cfcc - ROADMAP_ROAD_FIRST])]
                                              forState:UIControlStateNormal];
   [typePicker selectRow:cfcc - ROADMAP_ROAD_FIRST inComponent:0 animated:NO];
   
   dirButton = (UIButton *)[self viewWithTag:ID_DIR_BTN];
   if (this_direction == ROUTE_DIRECTION_ANY) {
      image = roadmap_iphoneimage_load("street_bar_two_ways");
      dirButton.enabled = NO;
   } else {
      image = roadmap_iphoneimage_load("street_bar_one_way");
      dirButton.enabled = YES;
   }
   saved_dir = this_direction;
   dir = saved_dir;
   [dirButton setBackgroundImage:image forState:UIControlStateNormal];
   [dirButton sizeToFit];
   [image release];
   
   if (isTracking) {
      textField = (UITextField *)[self viewWithTag:ID_LEFT];
      textField.hidden = NO;
      textField = (UITextField *)[self viewWithTag:ID_RIGHT];
      textField.hidden = NO;
      imageV = (UIImageView *)[self viewWithTag:ID_LEFT_ARROW];
      imageV.hidden = NO;
      imageV = (UIImageView *)[self viewWithTag:ID_RIGHT_ARROW];
      imageV.hidden = NO;
      
      if ([self getRangeLeft:&left right:&right]) {
         textField = (UITextField *)[self viewWithTag:ID_LEFT];
         textField.text = [NSString stringWithFormat:@"%d",left];
         textField = (UITextField *)[self viewWithTag:ID_RIGHT];
         textField.text = [NSString stringWithFormat:@"%d",right];
      } else {
         textField = (UITextField *)[self viewWithTag:ID_LEFT];
         textField.text = @"#";
         textField = (UITextField *)[self viewWithTag:ID_RIGHT];
         textField.text = @"#";
      }
   } else {
      textField = (UITextField *)[self viewWithTag:ID_LEFT];
      textField.hidden = YES;
      textField = (UITextField *)[self viewWithTag:ID_RIGHT];
      textField.hidden = YES;
      imageV = (UIImageView *)[self viewWithTag:ID_LEFT_ARROW];
      imageV.hidden = YES;
      imageV = (UIImageView *)[self viewWithTag:ID_RIGHT_ARROW];
      imageV.hidden = YES;
   }
}

- (void) onHide
{
   [self setSecondary:STATE_HIDDEN];
   
   [self hideKeyboard];
   [typePicker removeFromSuperview];
   [self setSecondary:STATE_HIDDEN];
   
   [self removeFromSuperview];
   gs_isBarShown = FALSE;
   
   if (isTracking)
      show_me_on_map();
}

- (void) onSaveConfirmed
{
   int street_id;
   const char *street_name, *city;
   const char *updated_left, *updated_right;
   UITextField *textField;
   
   textField = (UITextField *)[self viewWithTag:ID_STREET];
   street_name = [textField.text UTF8String];
   if (!street_name) street_name = "";
   
   textField = (UITextField *)[self viewWithTag:ID_CITY_EDT];
   city = [textField.text UTF8String];
   if (!city) city = "";
   
   street_id = editor_street_create (street_name, street_type, "", "", city, street_t2s);
   
   if (selectedLine.plugin_id != EditorPluginID) {
      
      if (roadmap_square_scale (selectedLine.square) == 0 &&
          !roadmap_street_line_has_predecessor (&selectedLine)) {
         
         selectedLine.cfcc = cfcc;
         //TODO: how to set direction ?
         //editor_override_line_set_direction(selectedLine.line_id, selectedLine.square, dir);//TODO: verify that
         editor_line_copy (&selectedLine, street_id);	
      }
      roadmap_square_set_current (selectedLine.square);
      editor_override_line_set_flag (selectedLine.line_id, selectedLine.square,  ED_LINE_DELETED);
   } else {
      editor_line_modify_properties (selectedLine.line_id, cfcc, ED_LINE_DIRTY);
      //editor_line_set_direction(selectedLine.line_id, dir);
      editor_line_set_street (selectedLine.line_id, street_id);	
   }
   
   
   editor_screen_reset_selected ();
   editor_report_segments ();
   
   if (isRangeUpdated) {
      textField = (UITextField *)[self viewWithTag:ID_LEFT];
      updated_left = [textField.text UTF8String];
      textField = (UITextField *)[self viewWithTag:ID_RIGHT];
      updated_right = [textField.text UTF8String];
      
      update_range_with_pos (updated_left, updated_right,
                                  city, street_name,
                                  CurrentGpsPoint,
                             CurrentFixedPosition);
   }
   
   [self onHide];
   
   if (!isLock && isTracking)
      isPreviouseNewName = TRUE;
   else
      isPreviouseNewName = FALSE;
   
   isEditing = FALSE;
   isTracking = FALSE;
}

- (void) onClose
{
   [self hideKeyboard];
   [typePicker removeFromSuperview];

   [self onHide];
   editor_screen_reset_selected();
   
   if (!isEditing && isTracking)
      set_enable_in_session(FALSE);
   
   isEditing = FALSE;
   isPreviouseNewName = FALSE;
}

- (void) onSave
{
   [self hideKeyboard];
   [typePicker removeFromSuperview];
   
   if (!isEditing)
      [self onHide];
   else
      ssd_confirm_dialog("", "Are you sure you want to submit this update?", TRUE, on_save_confirm, NULL);
}

- (void) onMore
{
   [self setSecondary:STATE_FULL];
}

- (void) selectType
{
   [self hideKeyboard];
   
   roadmap_main_show_view(typePicker);
}

- (void) toggleDir
{
   [self hideKeyboard];
   [typePicker removeFromSuperview];
   UIButton *button;
   UIImage *image;
   
   if (dir == ROUTE_DIRECTION_ANY) {
      dir = saved_dir;
      image = roadmap_iphoneimage_load("street_bar_one_way");
   } else {
      dir = ROUTE_DIRECTION_ANY;
      image = roadmap_iphoneimage_load("street_bar_two_ways");
   }
   
   if (image) {
      button = (UIButton *)[self viewWithTag:ID_DIR_BTN];
      [button setBackgroundImage:image forState:UIControlStateNormal];
      [image release];
   }
}

- (void) createBar
{
   UIView *secView;
   UITextField *textField;
   UIImage *image;
   UIImageView *imageView;
   UILabel *label;
   CGRect rect;
   CGSize totalSize;
   int streetBoxW;
   int secondaryW;
   int arrowW = 0;
   int buttonW;
   UIButton *button;
   
   //Secondary view
   ////////////////
   //background
   image = roadmap_iphoneimage_load("street_bar_secondary_bg");
   if (!image)
      roadmap_log (ROADMAP_FATAL, "Could not load secondary bg: 'street_bar_secondary_bg'");

   rect = CGRectMake(SECONDARY_OFFSET_X, SECONDARY_OFFSET1_Y, 
                     320/*roadmap_canvas_width()*/ - SECONDARY_OFFSET_X, SECONDARY_H);
   secondaryW = rect.size.width;
   secView = [[UIView alloc] initWithFrame:rect];
   secView.hidden = YES;
   secView.tag = ID_SECONDARY;
   secondaryState = STATE_HIDDEN;
   [self addSubview:secView];
   [secView release];
   
   rect.origin = CGPointMake(0, 0);
   imageView = [[UIImageView alloc] initWithImage:[image stretchableImageWithLeftCapWidth:1 
                                                                             topCapHeight:5]];
   [image release];
   imageView.frame = rect;
   imageView.tag = ID_SEC_BG;
   [secView addSubview:imageView];
   [imageView release];
   
   //Secondary - city title
   rect = CGRectMake(10, SECONDARY_H - ROW_H - 5, CITY_W, ROW_H);
   label = [[UILabel alloc] initWithFrame:rect];
   label.tag = ID_CITY_TITLE;
   label.text = [NSString stringWithUTF8String:roadmap_lang_get("City")];
   label.adjustsFontSizeToFitWidth = YES;
   label.font = [UIFont systemFontOfSize:14];
   label.backgroundColor = [UIColor clearColor];
   label.hidden = YES;
   [secView addSubview:label];
   [label release];
   
   //Secondary - city edit
   rect = CGRectMake(10 + CITY_W + 5, SECONDARY_H - ROW_H - 5, secView.frame.size.width - 25 - CITY_W - BTN_W*2, ROW_H);
   textField = [[UITextField alloc] initWithFrame:rect];
   textField.returnKeyType = UIReturnKeyDone;
   textField.borderStyle = UITextBorderStyleRoundedRect;
   textField.font = [UIFont systemFontOfSize:14];
   textField.placeholder = @"Enter city name";
   textField.delegate = self;
   textField.tag = ID_CITY_EDT;
   textField.hidden = YES;
   [secView addSubview:textField];
   [textField release];
   
   //Secondary - type  title
   rect = CGRectMake(10, 5, CITY_W, ROW_H);
   label = [[UILabel alloc] initWithFrame:rect];
   label.text = [NSString stringWithUTF8String:roadmap_lang_get("Type")];
   label.backgroundColor = [UIColor clearColor];
   label.adjustsFontSizeToFitWidth = YES;
   label.font = [UIFont systemFontOfSize:14];
   [secView addSubview:label];
   [label release];
   
   //Secondary - type button
   button = [UIButton buttonWithType:UIButtonTypeRoundedRect];
   button.tag = ID_TYPE_BTN;
   [button setTitleColor:[UIColor blackColor] forState:UIControlStateNormal];
   button.titleLabel.font = [UIFont systemFontOfSize:14];
   [button addTarget:self action:@selector(selectType) forControlEvents:UIControlEventTouchUpInside];
   button.frame = CGRectMake(10 + label.bounds.size.width + 5, 5, 
                             secView.bounds.size.width - 10 - CITY_W - 10 - DIR_W - 5, ROW_H);
   [secView addSubview:button];
      
   //Secondary - street type picker
   typePicker = [[UIPickerView alloc] initWithFrame:CGRectZero];
   roadmap_main_get_bounds(&rect);
   int main_height = rect.size.height;
   rect = typePicker.frame;
   rect.origin.y = main_height - rect.size.height;
   typePicker.frame = rect;
   typePicker.delegate = self;
   typePicker.dataSource = self;
   typePicker.showsSelectionIndicator = YES;
   typePicker.tag = ID_TYPE_PCK;
   //typePicker.autoresizesSubviews = YES;
   //typePicker.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
   
   //Secondary dir button
   button = [UIButton buttonWithType:UIButtonTypeCustom];
   button.tag = ID_DIR_BTN;
   [button addTarget:self action:@selector(toggleDir) forControlEvents:UIControlEventTouchUpInside];
   button.frame = button.frame = CGRectMake(secView.bounds.size.width - DIR_W - 5, 5, 
                                            DIR_W, ROW_H);
   button.hidden = YES; //Remove this when direction chage is supported by server. Currenty not supported.
   [secView addSubview:button];
   
   //Secondary - cancel button
   button = [UIButton buttonWithType:UIButtonTypeCustom];
   button.tag = ID_CANCEL_BTN;
   [button setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Cancel")] forState:UIControlStateNormal];
   [button setTitleColor:[UIColor colorWithRed:0.23f green:0.21f blue:0.21f alpha:1.0f] forState:UIControlStateNormal];
   [button setTitleShadowColor:[UIColor colorWithRed:1.0f green:1.0f blue:1.0f alpha:1.0f] forState:UIControlStateNormal];
   button.titleLabel.shadowOffset = CGSizeMake(0, 1.0);
   button.titleLabel.font = [UIFont boldSystemFontOfSize:14];
   image = roadmap_iphoneimage_load("street_bar_button");
   [button setBackgroundImage:image forState:UIControlStateNormal];
   [image release];
   [button sizeToFit];
   buttonW = button.bounds.size.width;
   [button addTarget:self action:@selector(onClose) forControlEvents:UIControlEventTouchUpInside];
   button.frame = CGRectMake(secView.bounds.size.width - 2*buttonW - 10, SECONDARY_H - ROW_H - 5, 
                            button.bounds.size.width, button.bounds.size.height);
   [secView addSubview:button];
   
   //Secondary - save button
   button = [UIButton buttonWithType:UIButtonTypeCustom];
   button.tag = ID_SAVE_BTN;
   [button setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Save")] forState:UIControlStateNormal];
   [button setTitleColor:[UIColor colorWithRed:0.23f green:0.21f blue:0.21f alpha:1.0f] forState:UIControlStateNormal];
   [button setTitleShadowColor:[UIColor colorWithRed:1.0f green:1.0f blue:1.0f alpha:1.0f] forState:UIControlStateNormal];
   button.titleLabel.shadowOffset = CGSizeMake(0, 1.0);
   button.titleLabel.font = [UIFont boldSystemFontOfSize:14];
   image = roadmap_iphoneimage_load("street_bar_button");
   [button setBackgroundImage:image forState:UIControlStateNormal];
   [image release];
   [button sizeToFit];
   buttonW = button.bounds.size.width;
   [button addTarget:self action:@selector(onSave) forControlEvents:UIControlEventTouchUpInside];
   button.frame = CGRectMake(secView.bounds.size.width - buttonW - 5, SECONDARY_H - ROW_H - 5, 
                             button.bounds.size.width, button.bounds.size.height);
   [secView addSubview:button];
   
   //Secondary - more button
   button = [UIButton buttonWithType:UIButtonTypeCustom];
   button.tag = ID_MORE_BTN;
   [button setTitleColor:[UIColor blackColor] forState:UIControlStateNormal];
   button.titleLabel.font = [UIFont systemFontOfSize:14];
   image = roadmap_iphoneimage_load("street_bar_more");
   [button setImage:image forState:UIControlStateNormal];
   [image release];
   [button addTarget:self action:@selector(onMore) forControlEvents:UIControlEventTouchUpInside];
   button.frame = CGRectMake(5, SECONDARY_H - ROW_H - 5, secView.frame.size.width - 20 - buttonW*2, ROW_H);
   button.titleEdgeInsets = UIEdgeInsetsMake(0, 20, 0, 0);
   [secView addSubview:button];
   
   //Secondary - is this same steet (same size as more button)
   textField = [[UITextField alloc] initWithFrame:button.frame];
   textField.hidden = YES;
   textField.tag = ID_SAME_STREET;
   [secView addSubview:textField];
   [textField release];
   
   //Main view
   ////////////////
   //background
   image = roadmap_iphoneimage_load("street_bar_bg");
   if (image) {
      rect = CGRectMake(0, 0, /*roadmap_canvas_width()*/320, image.size.height);
      imageView = [[UIImageView alloc] initWithImage:[image stretchableImageWithLeftCapWidth:image.size.width - 1
                                                                                topCapHeight:0]];
      [image release];
      imageView.frame = rect;
      imageView.tag = ID_MAIN_BG;
      [self addSubview:imageView];
      [imageView release];
      
      totalSize = rect.size;
   }
   
   //X (cancel) button
   button = [UIButton buttonWithType:UIButtonTypeCustom];
   button.tag = ID_X_BTN;
   image = roadmap_iphoneimage_load("street_bar_x_tab");
   [button setBackgroundImage:image forState:UIControlStateNormal];
   [image release];
   [button sizeToFit];
   [button addTarget:self action:@selector(onClose) forControlEvents:UIControlEventTouchUpInside];
   button.frame = CGRectMake(2, totalSize.height - 3,
                             button.bounds.size.width, button.bounds.size.height);
   [self addSubview:button];
   totalSize.height += button.bounds.size.height;
   
   streetBoxW = totalSize.width - 5*6 - HOUSE_BOX_W*2;
   
   //left house number box
   rect = CGRectMake(5, 5, HOUSE_BOX_W, ROW_H);
   textField = [[UITextField alloc] initWithFrame:rect];
   textField.keyboardType = UIKeyboardTypeNumberPad;
   textField.returnKeyType = UIReturnKeyDone;
   textField.borderStyle = UITextBorderStyleRoundedRect;
   textField.font = [UIFont systemFontOfSize:14];
   textField.placeholder = @"#";
   textField.delegate = self;
   textField.tag = ID_LEFT;
   textField.adjustsFontSizeToFitWidth = YES;
   textField.minimumFontSize = 10;
   image = roadmap_iphoneimage_load("street_bar_house");
   if (image) {
      imageView = [[UIImageView alloc] initWithImage:image];
      [image release];
      textField.rightView = imageView;
      [imageView release];
   }
   [self addSubview:textField];
   [textField release];
   
   //right house number box
   rect = CGRectMake(totalSize.width - HOUSE_BOX_W - 5, 5, HOUSE_BOX_W, ROW_H);
   textField = [[UITextField alloc] initWithFrame:rect];
   textField.keyboardType = UIKeyboardTypeNumberPad;
   textField.returnKeyType = UIReturnKeyDone;
   textField.borderStyle = UITextBorderStyleRoundedRect;
   textField.autocorrectionType = UITextAutocorrectionTypeNo;
   textField.font = [UIFont systemFontOfSize:14];
   textField.placeholder = @"#";
   textField.delegate = self;
   textField.tag = ID_RIGHT;
   textField.adjustsFontSizeToFitWidth = YES;
   textField.minimumFontSize = 10;
   image = roadmap_iphoneimage_load("street_bar_house");
   if (image) {
      imageView = [[UIImageView alloc] initWithImage:image];
      [image release];
      textField.leftView = imageView;
      [imageView release];
   }
   [self addSubview:textField];
   [textField release];
   
   //left arrow
   image = roadmap_iphoneimage_load("street_bar_left");
   if (image) {
      arrowW = image.size.width;
      streetBoxW -= arrowW;
      imageView = [[UIImageView alloc] initWithImage:image];
      [image release];
      rect = CGRectMake(HOUSE_BOX_W + 2*5, 5 + (ROW_H - image.size.height)/2, image.size.width, image.size.height);
      imageView.frame = rect;
      imageView.tag = ID_LEFT_ARROW;
      [self addSubview:imageView];
      [imageView release];
   }
   
   //right arrow
   image = roadmap_iphoneimage_load("street_bar_right");
   if (image) {
      arrowW = image.size.width;
      streetBoxW -= arrowW;
      imageView = [[UIImageView alloc] initWithImage:image];
      [image release];
      rect = CGRectMake(HOUSE_BOX_W + 4*5 + arrowW + streetBoxW, 5 + (ROW_H - image.size.height)/2, image.size.width, image.size.height);
      imageView.frame = rect;
      imageView.tag = ID_RIGHT_ARROW;
      [self addSubview:imageView];
      [imageView release];
   }
   
   //street name box
   rect = CGRectMake(HOUSE_BOX_W + 3*5 + arrowW, 5, streetBoxW, ROW_H);
   textField = [[UITextField alloc] initWithFrame:rect];
   textField.returnKeyType = UIReturnKeyDone;
   textField.borderStyle = UITextBorderStyleRoundedRect;
   textField.font = [UIFont systemFontOfSize:14];
   textField.placeholder = [NSString stringWithUTF8String:roadmap_lang_get("Enter road name")];
   textField.delegate = self;
   textField.tag = ID_STREET;
   [self addSubview:textField];
   [textField release];

   
   self.frame = CGRectMake(0, 0, totalSize.width, totalSize.height);
}

- (void) showStreet: (SelectedLine *)line
{
   selectedLine = line->line;
   [self setSecondary:STATE_HIDDEN];
   isTracking = FALSE;
   roadmap_layer_get_categories_names(&categories, &catCount);
   catCount = ROADMAP_ROAD_LAST - ROADMAP_ROAD_FIRST + 1;
   
   if (!isCreated)
      [self createBar];
   
   isCreated = TRUE;
   isChangesMade = FALSE;
   isRangeUpdated = FALSE;
   
   [self refreshData];
   
   roadmap_canvas_show_view(self);
}

- (void) stopTracking
{
   isTracking = FALSE;
   isPreviouseNewName == FALSE;
   if (!isEditing)
      [self onHide];
}

- (void) showTracking:(PluginLine *)line andPos:(const RoadMapGpsPosition *) gps_position andLock:(BOOL) lock
{
   //static RoadMapSoundList list = NULL;
   
   if (isEditing)
      return;
   
   int same_line = (selectedLine.line_id == line->line_id &&
                    selectedLine.square == line->square);
   
   if ( same_line && 
       isPreviouseNewName && !lock)
      return; //We already renamed this segment
   
   
   selectedLine = *line;
   [self setPos];
   
   
   
   isTracking = TRUE;
   isChangesMade = FALSE;
   isRangeUpdated = FALSE;
   roadmap_layer_get_categories_names(&categories, &catCount);
   catCount = ROADMAP_ROAD_LAST - ROADMAP_ROAD_FIRST + 1;
   
   if (lock) {
      isEditing = TRUE;
      [self setPos];
      roadmap_screen_hold();
   } /*else {
      if (!list) {
         list = roadmap_sound_list_create (SOUND_LIST_NO_FREE);
         roadmap_sound_list_add (list, "ping2");
         roadmap_res_get (RES_SOUND, 0, "ping2");
      }
      roadmap_sound_play_list (list);
   }
   */
   if (!isCreated)
      [self createBar];
   
   isCreated = TRUE;
   
   [self setSecondary:STATE_HIDDEN];
   if (!same_line && 
       isPreviouseNewName && !lock) {
      // new segment following a new street name, suggest the same name
      [self setSecondary:STATE_SAME_NAME];
   } else {
      [self refreshData];
   }
   
   isLock = lock;
   
   roadmap_canvas_show_view(self);
}

/*
- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
   NSLog(@"tesT");
}
*/



- (void)dealloc
{
	if (typePicker)
      [typePicker release];
	
	[super dealloc];
}





// UITextField delegate
- (void)textFieldDidBeginEditing:(UITextField *)textField
{
   UIButton *button;
   
   if (!isEditing) {
      isEditing = TRUE;
      [self setPosHighlight];
      roadmap_screen_hold();
   }
   
   [self setSecondary:STATE_PARTIAL];
   
   [typePicker removeFromSuperview];
   
   button = (UIButton *)[self viewWithTag:ID_SAVE_BTN];
   [button setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Save")] forState:UIControlStateNormal];
   [button removeTarget:self action:@selector(onClose) forControlEvents:UIControlEventTouchUpInside];
   [button addTarget:self action:@selector(onSave) forControlEvents:UIControlEventTouchUpInside];
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
   [self hideKeyboard];
   return NO;
}

- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string
{
   isChangesMade = TRUE;
   
   if (textField.tag == ID_LEFT ||
       textField.tag == ID_RIGHT)
      isRangeUpdated = TRUE;
   
   return YES;
}

//UIPickerView delegate & dataSource
- (NSInteger)numberOfComponentsInPickerView:(UIPickerView *)pickerView
{
   return 1;
}

- (NSInteger)pickerView:(UIPickerView *)pickerView numberOfRowsInComponent:(NSInteger)component
{
   return catCount;
}

- (NSString *)pickerView:(UIPickerView *)pickerView titleForRow:(NSInteger)row forComponent:(NSInteger)component
{
   return [NSString stringWithUTF8String:roadmap_lang_get(categories[row])];
}

- (void)pickerView:(UIPickerView *)pickerView didSelectRow:(NSInteger)row inComponent:(NSInteger)component
{
   UIButton *typeButton = (UIButton *)[self viewWithTag:ID_TYPE_BTN];
   [typeButton setTitle:[NSString stringWithUTF8String:roadmap_lang_get(categories[row])]
               forState:UIControlStateNormal];
   
   cfcc = row + ROADMAP_ROAD_FIRST;
}

@end