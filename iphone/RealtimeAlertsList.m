/* RealtimeAlertsList.c - manage the Real Time Alerts list display
 *
 * LICENSE:
 *
 *   Copyright 2008 Ehud Shabtai
 *   Copyright 2009 Avi R.
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
 * SYNOPSYS:
 *
 *   See RealtimeAlertsList.h
 */

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_history.h"
#include "roadmap_locator.h"
#include "roadmap_street.h"
#include "roadmap_lang.h"
#include "roadmap_messagebox.h"
#include "roadmap_geocode.h"
#include "roadmap_trip.h"
#include "roadmap_county.h"
#include "roadmap_display.h"
#include "roadmap_math.h"
#include "roadmap_navigate.h"
#include "ssd/ssd_keyboard.h"
#include "ssd/ssd_generic_list_dialog.h"
#include "ssd/ssd_confirm_dialog.h"
#include "ssd/ssd_tabcontrol.h"
#include "ssd/ssd_list.h"
#include "ssd/ssd_contextmenu.h"
#include "ssd/ssd_dialog.h"
#include "roadmap_groups.h"

#include "Realtime.h"
#include "Realtime/RealtimeAlerts.h"
#include "Realtime/RealtimeAlertsList.h"
#include "Realtime/RealtimeAlertCommentsList.h"
#include "Realtime/RealtimeTrafficInfo.h"
#include "navigate/navigate_main.h"
#include "iphone/roadmap_list_menu.h"
#include "iphone/roadmap_iphonelist_menu.h"
#include "iphone/widgets/iphoneLabel.h"
#include "roadmap_res.h"
#include "roadmap_path.h"
#include "roadmap_main.h"

#ifdef STREAM_TEST
#include "roadmap_sound_stream.h"
#endif //STREAM_TEST


// Context menu:
static ssd_cm_item      context_menu_items[] = 
{
   SSD_CM_INIT_ITEM  ( "Delete",             rtl_cm_delete),
   SSD_CM_INIT_ITEM  ( "Report abuse",       rtl_cm_report_abuse),
};
static ssd_contextmenu  context_menu = SSD_CM_INIT_MENU( context_menu_items);

static void populate_list();
static void free_list();

static SsdWidget   tabs[tab__count];

#define MAX_ALERT_ADD_ONS  10

typedef struct AlertList_s{
   char *type_str[MAX_ALERTS_ENTRIES];
   char *location_str[MAX_ALERTS_ENTRIES];
   char *detail[MAX_ALERTS_ENTRIES];
   char *distance[MAX_ALERTS_ENTRIES];
   char *unit[MAX_ALERTS_ENTRIES];
   char *more_info[MAX_ALERTS_ENTRIES];
   char *value[MAX_ALERTS_ENTRIES];
   char *icon[MAX_ALERTS_ENTRIES];
   char *group_icon[MAX_ALERTS_ENTRIES];
   char *group_name[MAX_ALERTS_ENTRIES];
   char *attachment[MAX_ALERTS_ENTRIES];
   char *addons[MAX_ALERTS_ENTRIES][MAX_ALERT_ADD_ONS];
   int iAddonsCount[MAX_ALERTS_ENTRIES];
   int type[MAX_ALERTS_ENTRIES];
   int iDistnace[MAX_ALERTS_ENTRIES];
   UIView *view[MAX_ALERTS_ENTRIES];
   BOOL bOldAlert[MAX_ALERTS_ENTRIES];
   int iCount;
   int iOldAlertsCount;
}AlertList;

static AlertList gAlertListTable;
static AlertList gTabAlertList; 
 
static   real_time_tabs		g_active_tab = tab_all;
static   alert_filter g_list_filter = filter_none;
static   void *g_list_menu = NULL;
static   void *g_alerts_list = NULL;

static int g_alert_type = -1;

static UIColor *gColorType = NULL;
static UIColor *gColorUnit = NULL;

static CGRect gRectIcon          = {10.0f, 10.0f, 40.0f, 40.0f};
static CGRect gRectType          = {65.0f, 2.0f, 220.0f, 20.0f};
static CGRect gRectLocation      = {65.0f, 22.0f, 220.0f, 40.0f};
static CGRect gRectDetail        = {65.0f, 62.0f, 220.0f, 45.0f};
static CGRect gRectMoreInfo      = {65.0f, 110.0f, 220.0f, 45.0f};
static CGRect gRectAttachment    = {35.0f, 10.0f, 40.0f, 50.0f};
static CGRect gRectDistance      = {15.0f, 50.0f, 35.0f, 15.0f};
static CGRect gRectUnit          = {15.0f, 65.0f, 35.0f, 20.0f};
static CGRect gRectGroupIcon     = {10.0f, 115.0f, 40.0f, 40.0f};
static CGRect gRectGroupButton   = {10.0f, 110.0f, 300.0f, 45.0f};

#define ALERT_LIST_HEIGHT 160

static int initialized = 0;
//static int fresh_list = 0;


#define MAX_IMG_ITEMS      100

static const char* selector_options[3] = {"Around me", "On route", "My Groups"};
static list_menu_selector gFilterSelector;

static void	show_types_list(BOOL refresh, BOOL no_animation);
static void on_tab_gain_focus(int type);
#define TYPES_LIST 1
#define ALERTS_LIST 2


///////////////////////////////////////////////////////////////////////////////////////////
static int RealtimeAlertsListCallBack(SsdWidget widget, const char *new_value,
        const void *value, void *context)
{
   int alertId;
   
   alertId = atoi((const char*)value);
   if (RTAlerts_Get_Number_of_Comments(alertId) == 0 &&
       !RTAlerts_isAlertArchive(alertId))
   {
		roadmap_main_show_root(0);
      RTAlerts_Popup_By_Id(alertId,FALSE);
   }
   else if (RTAlerts_Get_Number_of_Comments(alertId) > 0)
   {
      RealtimeAlertCommentsList(alertId);
   }
	
	//free_list();
	
   return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////
static void delete_callback(int exit_code, void *context){
   int alertId;
   BOOL success;
   
   if (exit_code != dec_yes)
      return;
   
   alertId = atoi((const char*)context);
   success = real_time_remove_alert(alertId);
  
}

static void on_delete (void *data) {
   char message[200];
   
	int i;
	for (i = 0; i < gAlertListTable.iCount; ++i) {
		if (gAlertListTable.value[i] == data) break;
	}
   
   delete_callback(dec_yes, data);
}

///////////////////////////////////////////////////////////////////////////////////////////
static void report_abuse_confirm_dlg_callback(int exit_code, void *context){
   if (exit_code == dec_yes){
      int alertId = atoi((const char*)context);
      Realtime_ReportAbuse(alertId, -1);      
   }
}

///////////////////////////////////////////////////////////////////////////////////////////
static void on_report_abuse (void *data) {
   ssd_confirm_dialog_custom (roadmap_lang_get("Report abuse"), roadmap_lang_get("Reports by several users will disable this user from reporting"), FALSE,report_abuse_confirm_dlg_callback, data, roadmap_lang_get("Report"), roadmap_lang_get("Cancel")) ;
}

static int on_option_selected (SsdWidget widget, const char *new_value, const void *value,
                               void *context) {
   
   real_time_list_context_menu_items   selection;
   
   ssd_cm_item_ptr item = (ssd_cm_item_ptr)value;
   
   selection = (real_time_list_context_menu_items)item->id;
   
   roadmap_main_pop_view(TRUE);
   
   switch( selection)
   {
      case rtl_cm_delete:
         on_delete(context);
         break;
         
      case rtl_cm_report_abuse:
         on_report_abuse(context);
         break;

      default:
         break;
   }

   
	return TRUE;
}

static void get_context_item (real_time_list_context_menu_items id, ssd_cm_item_ptr *item){
	int i;
	
	for( i=0; i<context_menu.item_count; i++)
	{
		*item = context_menu.item + i;
		if ((*item)->id == id) {		
			return;
		}
	}
	
   roadmap_log(ROADMAP_ERROR, "RealtimeAlertList - get_context_item() bad item id: %d count is: %d", id, context_menu.item_count);
	*item = context_menu.item;
}

///////////////////////////////////////////////////////////////////////////////////////////
static int RealtimeAlertsDeleteCallBack(SsdWidget widget, const char *new_value,
                                        const void *value, void *context)
{
	static char *labels[2];
	static void *values[2];
	static char *icons[2];
	ssd_cm_item_ptr item;
	int alertId = atoi((const char*)value);
   
	int count = 0;	
	
	
	get_context_item (rtl_cm_delete, &item);
	values[count] = item;
	labels[count] = strdup(item->label);
	icons[count] = NULL;
	count++;
	
	if (!RTAlerts_isByMe(alertId) && RTAlerts_Get_Type_By_Id(alertId) != RT_ALERT_TYPE_TRAFFIC_INFO ) {
      get_context_item (rtl_cm_report_abuse, &item);
      values[count] = item;
      labels[count] = strdup(item->label);
      icons[count] = NULL;
      count++;
   }
	   
	
	roadmap_list_menu_generic ("Options",
                              count,
                              (const char **)labels,
                              (const void **)values,
                              (const char **)icons,
                              NULL,
                              NULL,
                              on_option_selected,
                              NULL,
                              (void *)value,            
                              NULL, NULL,
                              60, 0, NULL);
   
   return FALSE;
}


@interface AlertCellView : UIView {
}
- (void) onGroupButton;
@end

@implementation AlertCellView
- (void) onGroupButton {
   UIView *view = (UIView *)[[self subviews] objectAtIndex:0];
   if (!view)
      return;
   int index = view.tag;
   if (index < 0 || index >= gAlertListTable.iCount)
		return;
   
   roadmap_groups_show_group(gAlertListTable.group_name[index]);
}
@end

///////////////////////////////////////////////////////////////////////////////////////////
void RealtimeAlertsListViewForCell (void *value, CGRect viewRect, UIView *parentView) {
	int i;
	int index;
   int j;
	UIImage *img;
	CGRect rect;
	UIImageView *imgView = NULL;
	iphoneLabel *label = NULL;
   UIButton *button;
	UIView *view = NULL;
	AlertCellView *cellView;
	
	enum tags {
		ICON_TAG = 1000,
		TYPE_TAG,
      LOCATION_TAG,
		DETAIL_TAG,
      MORE_INFO_TAG,
		DISTANCE_TAG,
		UM_TAG,
		ATTCH_ICON_TAG,
      ADDON_TAG,
      GROUP_ICON_TAG,
      GROUP_BUTTON_TAG,
      CELL_VIEW_TAG
	};
	
	for (i = 0; i < gAlertListTable.iCount; ++i) {
		if (!strcmp(gAlertListTable.value[i], (char *)value)) break;
	}
	
	index = i;
	
	if (index == gAlertListTable.iCount) {
      roadmap_log(ROADMAP_ERROR, "Requested alert cell could not be found");
		return;
   }
	
   cellView = (AlertCellView *)[parentView viewWithTag:CELL_VIEW_TAG];
	
	if (cellView == NULL) {
		cellView = [[AlertCellView alloc] initWithFrame:viewRect];
      [cellView setAutoresizesSubviews:YES];
      [cellView setAutoresizingMask:UIViewAutoresizingFlexibleWidth];
      cellView.tag = CELL_VIEW_TAG;
      [parentView addSubview:cellView];
      [cellView release];
      
      //view to hold the index
      view = [[UIView alloc] initWithFrame:CGRectZero];
      [cellView addSubview:view];
      [view release];
		
		// Create alert icon
		imgView = [[UIImageView alloc] initWithFrame:gRectIcon];
		imgView.tag = ICON_TAG;
		[cellView addSubview:imgView];
		[imgView release];
		
		// Create alert type
      rect = gRectType;
      rect.size.width = viewRect.size.width - gRectType.origin.x - 20;
		label = [[iphoneLabel alloc] initWithFrame:rect];
      [label setAutoresizesSubviews:YES];
      [label setAutoresizingMask:UIViewAutoresizingFlexibleWidth];
		[label setNumberOfLines:1];
		[label setLineBreakMode:UILineBreakModeWordWrap];
		[label setTextColor:gColorType];
		[label setFont:[UIFont boldSystemFontOfSize:18]];
		[label setAdjustsFontSizeToFitWidth:YES];
      [label setBackgroundColor:[UIColor clearColor]];
		label.tag = TYPE_TAG;
		[cellView addSubview:label];
		[label release];
      
      // Create alert location
      rect = gRectLocation;
      rect.size.width = viewRect.size.width - gRectLocation.origin.x - 20;
		label = [[iphoneLabel alloc] initWithFrame:rect];
      [label setAutoresizesSubviews:YES];
      [label setAutoresizingMask:UIViewAutoresizingFlexibleWidth];
		[label setNumberOfLines:2];
		[label setLineBreakMode:UILineBreakModeWordWrap];
		[label setFont:[UIFont boldSystemFontOfSize:18]];
		[label setAdjustsFontSizeToFitWidth:YES];
      [label setBackgroundColor:[UIColor clearColor]];
		label.tag = LOCATION_TAG;
		[cellView addSubview:label];
		[label release];
		
		// Create alert details
      rect = gRectDetail;
      rect.size.width = viewRect.size.width - gRectDetail.origin.x - 20;
		label = [[iphoneLabel alloc] initWithFrame:rect];
      [label setAutoresizesSubviews:YES];
      [label setAutoresizingMask:UIViewAutoresizingFlexibleWidth];
		[label setNumberOfLines:3];
		[label setLineBreakMode:UILineBreakModeWordWrap];
		[label setFont:[UIFont systemFontOfSize:16]];
      [label setBackgroundColor:[UIColor clearColor]];
		label.tag = DETAIL_TAG;
		[cellView addSubview:label];
		[label release];
		
		// Create alert distance
		label = [[iphoneLabel alloc] initWithFrame:gRectDistance];
		[label setTextAlignment:UITextAlignmentCenter];
		[label setFont:[UIFont boldSystemFontOfSize:16]];
		[label setAdjustsFontSizeToFitWidth:YES];
      [label setBackgroundColor:[UIColor clearColor]];
		label.tag = DISTANCE_TAG;
		[cellView addSubview:label];
		[label release];
		
		// Create alert um
		label = [[iphoneLabel alloc] initWithFrame:gRectUnit];
		[label setTextAlignment:UITextAlignmentCenter];
		[label setTextColor:gColorUnit];
		[label setFont:[UIFont systemFontOfSize:14]];
      [label setBackgroundColor:[UIColor clearColor]];
		label.tag = UM_TAG;
		[cellView addSubview:label];
		[label release];
		
		// Creqte attachment icon
		imgView = [[UIImageView alloc] initWithFrame:gRectAttachment];
		imgView.tag = ATTCH_ICON_TAG;
		[cellView addSubview:imgView];      
      
      // Create group button
      rect = gRectGroupButton;
      rect.size.width = viewRect.size.width - gRectGroupButton.origin.x - 10;
      button = [UIButton buttonWithType:UIButtonTypeCustom];
      button.frame = rect;
      button.tag = GROUP_BUTTON_TAG;
      [button addTarget:cellView action:@selector(onGroupButton) forControlEvents:UIControlEventTouchUpInside];
      [button setAutoresizesSubviews:YES];
      [button setAutoresizingMask:UIViewAutoresizingFlexibleWidth];
		[parentView addSubview:button];
      
      // Create group icon
		imgView = [[UIImageView alloc] initWithFrame:gRectGroupIcon];
		imgView.tag = GROUP_ICON_TAG;
		[parentView addSubview:imgView];
		[imgView release];
      
      // Create alert more info
		label = [[iphoneLabel alloc] initWithFrame:gRectMoreInfo];
      [label setAutoresizesSubviews:YES];
      [label setAutoresizingMask:UIViewAutoresizingFlexibleWidth];
		[label setNumberOfLines:3];
		[label setLineBreakMode:UILineBreakModeWordWrap];
      label.adjustsFontSizeToFitWidth = YES;
		[label setFont:[UIFont systemFontOfSize:13]];
      [label setBackgroundColor:[UIColor clearColor]];
		label.tag = MORE_INFO_TAG;
		[parentView addSubview:label];
		[label release];
	}
   
   while ([cellView viewWithTag:ADDON_TAG] != NULL) {
      [[cellView viewWithTag:ADDON_TAG] removeFromSuperview];
   }
   
   view = (UIView *)[[cellView subviews] objectAtIndex:0];
   view.tag = index;
	
	for (i = 0; i < [[cellView subviews] count]; i++) {
		view = (UIView *) [[cellView subviews] objectAtIndex:i];
		
		switch (view.tag) {
			case ICON_TAG:
				//Set alert icon
				img = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, gAlertListTable.icon[index]);
				[(UIImageView *)view setImage:img];
				if (img) {
					rect = gRectIcon;
					rect.size = [img size];
					rect.origin.x = gRectIcon.origin.x + (gRectIcon.size.width - rect.size.width)/2;
					[view setFrame:rect];
				}
            
            //Set addons
            for (j = 0 ; j < gAlertListTable.iAddonsCount[index]; j++){
               char *AddOn = gAlertListTable.addons[index][j];
               if (AddOn) {
                  img = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, AddOn);
                  imgView = [[UIImageView alloc] initWithImage:img];
                  imgView.tag = ADDON_TAG;
                  rect = gRectIcon;
                  rect.size = [img size];
                  rect.origin.x = gRectIcon.origin.x + (gRectIcon.size.width - rect.size.width)/2;
                  [imgView setFrame:rect];
                  [cellView addSubview:imgView];
                  [imgView release];
               }
            }

				break;
         case GROUP_ICON_TAG:
				//Set group icon
            if (gAlertListTable.group_icon[index] && gAlertListTable.group_icon[index][0]) {
               img = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, gAlertListTable.group_icon[index]);
               [(UIImageView *)view setImage:img];
               if (img) {
                  rect = gRectGroupIcon;
                  rect.size = [img size];
                  rect.origin.x = gRectGroupIcon.origin.x + (gRectGroupIcon.size.width - rect.size.width)/2;
                  [view setFrame:rect];
               }
            } else {
					[(UIImageView *)view setImage:NULL];
				}
            break;
			case TYPE_TAG:
				//set alert description
				[(iphoneLabel *)view setText:[NSString stringWithUTF8String:gAlertListTable.type_str[index]]];
				break;
         case LOCATION_TAG:
				//set alert description
				[(iphoneLabel *)view setText:[NSString stringWithUTF8String:gAlertListTable.location_str[index]]];
				break;
			case DETAIL_TAG:
				//set alert details
				[(iphoneLabel *)view setText:[NSString stringWithUTF8String:gAlertListTable.detail[index]]];
				break;
			case DISTANCE_TAG:
				//set alert distance
				[(iphoneLabel *)view setText:[NSString stringWithUTF8String:gAlertListTable.distance[index]]];
				break;
			case UM_TAG:
				//set alert um
				[(iphoneLabel *)view setText:[NSString stringWithUTF8String:gAlertListTable.unit[index]]];
				break;
			case ATTCH_ICON_TAG:
				//set attachment icon
				if (gAlertListTable.attachment[index]) {
					img = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, gAlertListTable.attachment[index]);
					[(UIImageView *)view setImage:img];
					if (img) {
						rect = gRectAttachment;
						rect.size = [img size];
						rect.origin.x = gRectAttachment.origin.x + (gRectAttachment.size.width - rect.size.width)/2;
						[view setFrame:rect];
					}
				} else {
					[(UIImageView *)view setImage:NULL];
				}
				break;
			default:
				break;
		}
	}
   
   for (i = 0; i < [[parentView subviews] count]; i++) {
		view = (UIView *) [[parentView subviews] objectAtIndex:i];
      switch (view.tag) {
         case GROUP_ICON_TAG:
				//Set group icon
            if (gAlertListTable.group_icon[index] && gAlertListTable.group_icon[index][0]) {
               img = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, gAlertListTable.group_icon[index]);
               [(UIImageView *)view setImage:img];
               if (img) {
                  rect = gRectGroupIcon;
                  rect.size = [img size];
                  rect.origin.x = gRectGroupIcon.origin.x + (gRectGroupIcon.size.width - rect.size.width)/2;
                  [view setFrame:rect];
               }
            } else {
					[(UIImageView *)view setImage:NULL];
				}
            break;
         case GROUP_BUTTON_TAG:
				//set group button
            if (gAlertListTable.group_icon[index] && gAlertListTable.group_icon[index][0]) {
               img = roadmap_res_get(RES_NATIVE_IMAGE, RES_SKIN, "group_in_events");
               if (img) {
                  [(UIButton*)view setBackgroundImage:[img stretchableImageWithLeftCapWidth:10 topCapHeight:0]
                                             forState:UIControlStateNormal];
               }
               view.hidden = NO;
            } else {
					[(UIButton *)view setImage:NULL
                                 forState:UIControlStateNormal];
               view.hidden = YES;
				}
				break;
         case MORE_INFO_TAG:
				//set alert more info
				[(iphoneLabel *)view setText:[NSString stringWithUTF8String:gAlertListTable.more_info[index]]];
            break;
			default:
				break;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
static void* get_selector(void) {
   switch (g_list_filter) {
      case filter_none:
         gFilterSelector.default_index = 0;
         break;
      case filter_on_route:
         gFilterSelector.default_index = 1;
         break;
      case filter_group:
         gFilterSelector.default_index = 2;
         break;
      default:
         gFilterSelector.default_index = 0;
         break;
   }
   
   return &gFilterSelector;
}

///////////////////////////////////////////////////////////////////////////////////////////
static void message_init(list_menu_empty_message *message) {
   message->title[0] = 0;
   message->text[0] = 0;
   message->top_image[0] = 0;
   message->bottom_image[0] = 0;
   message->button_text[0] = 0;
   message->button_cb = NULL;   
}

///////////////////////////////////////////////////////////////////////////////////////////
static list_menu_empty_message create_no_route_message(void) {
   list_menu_empty_message empty_message;
   
   message_init(&empty_message);
   
   strncpy_safe(empty_message.title, "No route", sizeof(empty_message.title));
   strncpy_safe(empty_message.text, "You should be in navigation mode to view events on your route", sizeof(empty_message.text));
   
   return empty_message;
}

///////////////////////////////////////////////////////////////////////////////////////////
static void show_more_bar(void) {
   roadmap_main_show_root(NO);
   roadmap_more_bar_show();
}

///////////////////////////////////////////////////////////////////////////////////////////
static list_menu_empty_message create_no_group_message(void) {
   list_menu_empty_message empty_message;
   
   message_init(&empty_message);
   
   strncpy_safe(empty_message.title, "No Groups events :(", sizeof(empty_message.title));
   snprintf(empty_message.text, sizeof(empty_message.text), "%s\n%s",
            roadmap_lang_get("You're not following any groups - but you should!"),
            roadmap_lang_get("Go to 'Groups' in main menu to find or create a group"));
   strncpy_safe(empty_message.top_image, "groups_none_top", sizeof(empty_message.top_image));
   strncpy_safe(empty_message.bottom_image, "groups_none_bottom", sizeof(empty_message.bottom_image));
   strncpy_safe(empty_message.button_text, "Ok", sizeof(empty_message.button_text));
   empty_message.button_cb = show_more_bar;
   
   return empty_message;
}

///////////////////////////////////////////////////////////////////////////////////////////
static void RealtimeAlertsListShow(BOOL refresh)
{
	const char *title;
   list_menu_empty_message empty_message, *p_empty_message = NULL;
   list_menu_sections sections, *p_sections = NULL;
   int count;
   
	
	switch (g_alert_type) {
		case -1:
			title = "All";
			break;
		case RT_ALERT_TYPE_POLICE:
			title = "Police";
			break;
		case RT_ALERT_TYPE_TRAFFIC_JAM:
			title = "Traffic";
			break;
		case RT_ALERT_TYPE_ACCIDENT:
			title = "Accidents";
			break;
		case RT_ALERT_TYPE_OTHER:
			title = "Other";
			break;
      case RT_ALERT_TYPE_CHIT_CHAT:
         title = "Chit Chats";
         break;
		default:
			title = "Events";
			break;
	}
   
   if (g_list_filter == filter_on_route &&
       (navigate_main_state() == -1) &&
       RealTimeLoginState()) {
      count = 0;
      empty_message = create_no_route_message();
      p_empty_message = &empty_message;
   } else  if (g_list_filter == filter_group &&
               (roadmap_groups_get_num_following() == 0) &&
               RealTimeLoginState()) {
      count = 0;
      empty_message = create_no_group_message();
      p_empty_message = &empty_message;
   } else {
      if (refresh && g_alerts_list)
         on_tab_gain_focus(g_active_tab);
      count = gTabAlertList.iCount;
   }
   
   if (g_list_filter == filter_group &&
       gTabAlertList.iOldAlertsCount > 0) {
      sections.count = 2;
      sections.items[0] = gTabAlertList.iCount - gTabAlertList.iOldAlertsCount;
      sections.items[1] = gTabAlertList.iOldAlertsCount;
      strncpy_safe (sections.titles[0], "Live events", sizeof(sections.titles[0]));
      if (gTabAlertList.iCount - gTabAlertList.iOldAlertsCount == 0) {
         snprintf (sections.footers[0], sizeof(sections.footers[0]), "\n\n(%s)\n\n\n\n\n", roadmap_lang_get("no relevant events"));
      } else {
         strncpy_safe (sections.footers[0], "", sizeof(sections.footers[0]));
      }
      strncpy_safe (sections.titles[1], "Previous events", sizeof(sections.titles[0]));
      strncpy_safe (sections.footers[1], "", sizeof(sections.footers[0]));
      p_sections = &sections;
   }
   
   if (!refresh || !g_alerts_list) {
      g_alerts_list = roadmap_list_menu_custom (roadmap_lang_get(title), count,
                                                (const void **)&gTabAlertList.value[0], get_selector(),
                                                RealtimeAlertsListCallBack, RealtimeAlertsDeleteCallBack,
                                                NULL, RealtimeAlertsListViewForCell, (void *)ALERTS_LIST,
                                                ALERT_LIST_HEIGHT, FALSE, p_empty_message, p_sections);
   } else {
      roadmap_list_menu_custom_refresh (g_alerts_list, roadmap_lang_get(title), count,
                                        (const void **)&gTabAlertList.value[0], get_selector(),
                                        RealtimeAlertsListCallBack, RealtimeAlertsDeleteCallBack,
                                        NULL, RealtimeAlertsListViewForCell, (void *)ALERTS_LIST,
                                        ALERT_LIST_HEIGHT, FALSE, p_empty_message, p_sections);
   }
}

///////////////////////////////////////////////////////////////////////////////////////////
static int RealtimeAlertsListCallBackTabs(SsdWidget widget, const char *new_value,
        const void *value)
{

    int alertId;
	
    if (!new_value[0] || !strcmp(new_value,
            roadmap_lang_get("Scroll Through Alerts on the map")))
    {
        RTAlerts_Scroll_All();
        ssd_generic_list_dialog_hide_all();
        return TRUE;
    }

    alertId = atoi((const char*)value);
    if (RTAlerts_Get_Number_of_Comments(alertId) == 0)
    {
        ssd_generic_list_dialog_hide_all();
        RTAlerts_Popup_By_Id(alertId,FALSE);
    }
    else
    {
        ssd_generic_list_dialog_hide();
        RealtimeAlertCommentsList(alertId);
    }
    return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////
static int RealtimeAlertsDeleteCallBackTabs(SsdWidget widget, const char *new_value,
        const void *value)
{
   char message[200];
    

    if (!new_value[0] || !strcmp(new_value,
            roadmap_lang_get("Scroll Through Alerts on the map")))
    {
         return TRUE;
    }

	delete_callback(dec_yes, (void *)value);

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////
static int count_tab(int tab, int num, int types,...){
	int i;
	int count = 0;
	va_list ap;
	int type;
   
	for ( i=0; i< gAlertListTable.iCount; i++){
		int j;
		va_start(ap, types);
		type = types;
		for (j = 0; j < num; j++ ) {
			if (gAlertListTable.type[i] == type || tab == tab_all) {
            count++;
			}
			type = va_arg(ap, int);
		}
		va_end(ap);
   }
   
   return count;
}

///////////////////////////////////////////////////////////////////////////////////////////
static void populate_tab(int tab, int num, int types,...){
	int i;
	int count = 0;
   int old_count = 0;
	va_list ap;
	int type;
		
	for ( i=0; i< gAlertListTable.iCount; i++){
		int j;
		va_start(ap, types);
		type = types;
		for (j = 0; j < num; j++ ) {
			if (gAlertListTable.type[i] == type || tab == tab_all) {
            int k;
				gTabAlertList.type_str[count] = gAlertListTable.type_str[i];
            gTabAlertList.location_str[count] = gAlertListTable.location_str[i];
				gTabAlertList.detail[count] = gAlertListTable.detail[i];
            gTabAlertList.more_info[count] = gAlertListTable.more_info[i];
				gTabAlertList.distance[count] = gAlertListTable.distance[i];
				gTabAlertList.unit[count] = gAlertListTable.unit[i];
				gTabAlertList.value[count] = gAlertListTable.value[i];
				gTabAlertList.icon[count] = gAlertListTable.icon[i];
            gTabAlertList.group_icon[count] = gAlertListTable.group_icon[i];
            gTabAlertList.group_name[count] = gAlertListTable.group_name[i];
				gTabAlertList.type[count] = gAlertListTable.type[i];
				gTabAlertList.iDistnace[count] = gAlertListTable.iDistnace[i];
            gTabAlertList.iAddonsCount[count] = gAlertListTable.iAddonsCount[i];
            gTabAlertList.bOldAlert[count] = gAlertListTable.bOldAlert[i];
            for (k = 0; k < gTabAlertList.iAddonsCount[count]; k++)
               gTabAlertList.addons[count][k] = gAlertListTable.addons[i][k];
            
            if (gAlertListTable.bOldAlert[i])
               old_count++;
            
				count++;
			}
			type = va_arg(ap, int);
		}
		va_end(ap);
	 }
	 gTabAlertList.iCount = count;
   gTabAlertList.iOldAlertsCount = old_count;
}


extern BOOL get_alert_location_info(const RoadMapPosition *alert_position,roadmap_alerter_location_info *pNew_Info,
                                    roadmap_alerter_location_info *alert_location_info);

///////////////////////////////////////////////////////////////////////////////////////////
static void populate_list(){
   int iCount = -1;
   int index;
   int iAlertCount;
   int iOldAlertsCount = 0;
   RTAlert *alert;
   char AlertStr[700];
   char str[100];
   int distance = -1;
   RoadMapGpsPosition CurrentPosition;
   RoadMapPosition position, current_pos;
   const RoadMapPosition *gps_pos;
   PluginLine line;
   int Direction;
   int distance_far;
   char location_str[100];
   char dist_str[100];
   char unit_str[20];
   char detail_str[RT_ALERT_DESCRIPTION_MAXSIZE + 50 + 1]; // 50 for additional text
   int i, j;
   int num_sections = 1;
   RoadMapPosition context_save_pos;
   zoom_t context_save_zoom;
   time_t now;
   int timeDiff;
   const char *title;
   
   //UIView *cellView = NULL;
   
   
   AlertStr[0] = 0;
   str[0] = 0;
   detail_str[0] = 0;
   
   for (i=0; i<MAX_ALERTS_ENTRIES; i++)
   {
      gAlertListTable.type_str[i] = NULL;
      gAlertListTable.location_str[i] = NULL;
      gAlertListTable.detail[i] = NULL;
      gAlertListTable.distance[i] = NULL;
      gAlertListTable.unit[i] = NULL;
      gAlertListTable.more_info[i] = NULL;
      gAlertListTable.value[i] = NULL;
      gAlertListTable.icon[i] = NULL;
      gAlertListTable.group_icon[i] = NULL;
      gAlertListTable.group_name[i] = NULL;
      gAlertListTable.attachment[i] = NULL;
      gAlertListTable.bOldAlert[i] = FALSE;
   }
   
   iCount = 0;
   
   roadmap_math_get_context(&context_save_pos, &context_save_zoom);
   
   if (g_list_filter == filter_group)
      num_sections = 2;
   
   for (i = 0; i < num_sections; i++) {
      if (i == 1) {
         RTAlerts_Sort_List(sort_recency);
      }
      iAlertCount = RTAlerts_Count();
      index = 0;
      while ((iAlertCount > 0) && (iCount < MAX_ALERTS_ENTRIES))    {
         
         AlertStr[0] = 0;
         
         alert = RTAlerts_Get(index);
         
         if (i == 0) {
            if (alert->bArchive) {
               iAlertCount--;
               index++;
               continue;
            }
            if (g_list_filter == filter_on_route){
               if (!RTAlerts_isAlertOnRoute (alert->iID)){
                  iAlertCount--;
                  index++;
                  continue;
               }
            }
            
            if (g_list_filter == filter_group){
               if (alert->iGroupRelevance == GROUP_RELEVANCE_NONE){
                  iAlertCount--;
                  index++;
                  continue;
               }
            }
         } else {
            if (alert->bArchive) {
               gAlertListTable.bOldAlert[iCount] = TRUE;
            } else {
               iAlertCount--;
               index++;
               continue;
            }

         }
         
         
         position.longitude = alert->iLongitude;
         position.latitude = alert->iLatitude;
         
         roadmap_math_set_context((RoadMapPosition *)&position, 20);
         title = RTAlerts_get_title(alert, alert->iType, alert->iSubType);
         if (title)
            snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr) - strlen(AlertStr), "%s", title);
         
         
         if (roadmap_navigate_get_current(&CurrentPosition, &line, &Direction)
             == -1)
         {
            // check the distance to the alert
            gps_pos = roadmap_trip_get_position("Location");
            if (gps_pos != NULL)
            {
               current_pos.latitude = gps_pos->latitude;
               current_pos.longitude = gps_pos->longitude;
            }
            else
            {
               current_pos.latitude = -1;
               current_pos.longitude = -1;
            }
         }
         else
         {
            current_pos.latitude = CurrentPosition.latitude;
            current_pos.longitude = CurrentPosition.longitude;
         }
         
         if ((current_pos.latitude != -1) && (current_pos.longitude != -1))
         {
            distance = roadmap_math_distance(&current_pos, &position);
            distance_far = roadmap_math_to_trip_distance(distance);
            
            if (distance_far > 0)
            {
               int tenths = roadmap_math_to_trip_distance_tenths(distance);
               snprintf(dist_str, sizeof(dist_str), "%d.%d", distance_far, tenths
                        % 10);
               snprintf(unit_str, sizeof(unit_str), "%s",
                        roadmap_lang_get(roadmap_math_trip_unit()));
            }
            else
            {
               snprintf(dist_str, sizeof(dist_str), "%d", distance);
               snprintf(unit_str, sizeof(unit_str), "%s",
                        roadmap_lang_get(roadmap_math_distance_unit()));
            }
         }
         
         RTAlerts_update_location_str(alert);
         snprintf(location_str, sizeof(location_str), "%s", alert->sLocationStr);
         
         
         snprintf(str, sizeof(str), "%d", alert->iID);
         
         snprintf(detail_str, sizeof(detail_str), "%s", alert->sDescription);
         gAlertListTable.detail[iCount] = strdup(detail_str);
         
         // Display when the alert was generated
         now = time(NULL);
         timeDiff = (int)difftime(now, (time_t)alert->iReportTime);
         if (timeDiff <0)
            timeDiff = 0;
         
         if (alert->iType == RT_ALERT_TYPE_TRAFFIC_INFO)
            snprintf(detail_str, sizeof(detail_str),
                     "%s", roadmap_lang_get("Updated "));
         else
            snprintf(detail_str, sizeof(detail_str),
                     "%s",/*roadmap_lang_get("Reported ")*/"");
         
         
         if (timeDiff < 60)
            snprintf(detail_str + strlen(detail_str), sizeof(detail_str)
                     - strlen(detail_str),
                     roadmap_lang_get("%d seconds ago"), timeDiff);
         else if ((timeDiff > 60) && (timeDiff < 3600))
            snprintf(detail_str + strlen(detail_str), sizeof(detail_str)
                     - strlen(detail_str),
                     roadmap_lang_get("%d minutes ago"), timeDiff/60);
         else
            snprintf(detail_str + strlen(detail_str), sizeof(detail_str)
                     - strlen(detail_str),
                     roadmap_lang_get("%2.1f hours ago"), (float)timeDiff
                     /3600);
         
         
         if (alert->sReportedBy[0] != 0)
            snprintf(detail_str + strlen(detail_str), sizeof(detail_str)
                     - strlen(detail_str),
                     " %s %s",roadmap_lang_get("by"), alert->sReportedBy);
         if (alert->sGroup[0] != 0)
            snprintf(detail_str + strlen(detail_str), sizeof(detail_str)
                     - strlen(detail_str),
                     "\n%s: %s",roadmap_lang_get("group"), alert->sGroup);
         gAlertListTable.more_info[iCount] = strdup(detail_str);
         
         gAlertListTable.type_str[iCount] = strdup(AlertStr);
         gAlertListTable.location_str[iCount] = strdup(location_str);
         gAlertListTable.distance[iCount] = strdup(dist_str);
         gAlertListTable.unit[iCount] = strdup(unit_str);
         gAlertListTable.value[iCount] = strdup(str);
         gAlertListTable.icon[iCount] = strdup(RTAlerts_Get_Icon(alert->iID));
         if (alert->sGroupIcon && alert->sGroupIcon[0])
            gAlertListTable.group_icon[iCount] = strdup(alert->sGroupIcon);
         else if (alert->sGroup && alert->sGroup[0])
            gAlertListTable.group_icon[iCount] = strdup("groups_default_icons");
         gAlertListTable.group_name[iCount] = strdup(alert->sGroup);
         if (RTAlerts_Has_Image(alert->iID) || RTAlerts_Has_Voice(alert->iID))
            gAlertListTable.attachment[iCount] = strdup ("attachment");
         gAlertListTable.type[iCount] = alert->iType;
         gAlertListTable.iDistnace[iCount] = distance;
         
         //Set addons
         gAlertListTable.iAddonsCount[iCount] = RTAlerts_Get_Number_Of_AddOns(alert->iID);
         for (j = 0 ; j < gAlertListTable.iAddonsCount[iCount]; j++){
            gAlertListTable.addons[iCount][j] = strdup(RTAlerts_Get_AddOn(alert->iID, j));
         }
         
         
         
            iCount++;
         if (i == 1)
            iOldAlertsCount++;
         iAlertCount--;
         index++;
      }
   }
   
   gAlertListTable.iCount = iCount;
   gAlertListTable.iOldAlertsCount = iOldAlertsCount;
   
   roadmap_math_set_context(&context_save_pos, context_save_zoom);
}

static void free_list() {
   int i, j;
   
   for (i = 0; i < gAlertListTable.iCount; ++i) {
      if (gAlertListTable.type_str[i]) {
         free (gAlertListTable.type_str[i]);
         gAlertListTable.type_str[i] = NULL;
      }
      if (gAlertListTable.location_str[i]) {
         free (gAlertListTable.location_str[i]);
         gAlertListTable.location_str[i] = NULL;
      }
      if (gAlertListTable.detail[i]) {
         free (gAlertListTable.detail[i]);
         gAlertListTable.detail[i] = NULL;
      }
      if (gAlertListTable.distance[i]) {
         free (gAlertListTable.distance[i]);
         gAlertListTable.distance[i] = NULL;
      }
      if (gAlertListTable.unit[i]) {
         free (gAlertListTable.unit[i]);
         gAlertListTable.unit[i] = NULL;
      }
      if (gAlertListTable.more_info[i]) {
         free (gAlertListTable.more_info[i]);
         gAlertListTable.more_info[i] = NULL;
      }
      if (gAlertListTable.value[i]) {
         free (gAlertListTable.value[i]);
         gAlertListTable.value[i] = NULL;
      }
      if (gAlertListTable.icon[i]) {
         free (gAlertListTable.icon[i]);
         gAlertListTable.icon[i] = NULL;
      }
      if (gAlertListTable.group_icon[i]) {
         free (gAlertListTable.group_icon[i]);
         gAlertListTable.group_icon[i] = NULL;
      }
      if (gAlertListTable.attachment[i]) {
         free (gAlertListTable.attachment[i]);
         gAlertListTable.attachment[i] = NULL;
      }
      
      for (j = 0 ; j < gAlertListTable.iAddonsCount[i]; j++){
         if (gAlertListTable.addons[i][j]) {
            free(gAlertListTable.addons[i][j]);
            gAlertListTable.addons[i][j] = NULL;
         }
      }
   }
   
   gAlertListTable.iCount = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////
static SsdWidget create_list(int tab_id){
   SsdWidget list;
   char tab_name[20];
   sprintf(tab_name, "list %d", tab_id);
   list = ssd_list_new (tab_name, SSD_MAX_SIZE, SSD_MAX_SIZE, inputtype_none, 0, NULL);
#ifdef IPHONE
   ssd_list_resize (list, 73);
#elif defined (TOUCH_SCREEN)	
   ssd_list_resize (list, 66);
#else
   ssd_list_resize (list, 63);
#endif
   
   return list;
}

///////////////////////////////////////////////////////////////////////////////////////////
static void on_tab_gain_focus(int type)
{
	switch (type){
		case tab_all:
         //populate_first_tab();
		populate_tab(tab_all,1,0);
			g_alert_type = -1;
			break;
		case tab_police:
			populate_tab(tab_police,1,RT_ALERT_TYPE_POLICE);
			g_alert_type = RT_ALERT_TYPE_POLICE;
			break;
		case tab_traffic_jam:
         populate_tab(tab_traffic_jam,2, RT_ALERT_TYPE_TRAFFIC_INFO, RT_ALERT_TYPE_TRAFFIC_JAM);
			g_alert_type = RT_ALERT_TYPE_TRAFFIC_JAM;
         break;
      case tab_accidents:
         populate_tab(tab_accidents,1, RT_ALERT_TYPE_ACCIDENT);
			g_alert_type = RT_ALERT_TYPE_ACCIDENT;
         break;
      case tab_chit_chat:
         populate_tab(tab_chit_chat,1, RT_ALERT_TYPE_CHIT_CHAT);
			g_alert_type = RT_ALERT_TYPE_CHIT_CHAT;
         break;
      case tab_others:
         populate_tab(tab_others,5, RT_ALERT_TYPE_HAZARD,RT_ALERT_TYPE_OTHER, RT_ALERT_TYPE_CONSTRUCTION, RT_ALERT_TYPE_PARKING, RT_ALERT_TYPE_DYNAMIC);
			g_alert_type = RT_ALERT_TYPE_OTHER;
         break;
#ifdef STREAM_TEST
      case tab_stream:
         roadmap_radio();
         roadmap_main_show_root(0);
         return 1;
         
         break;
#endif //STREAM_TEST
         
		default:
			break;
	} 
   
	g_active_tab = type;
}

///////////////////////////////////////////////////////////////////////////////////////////
static void clear_lists(){
	int i;
      for (i=0; i< tab__count;i++){
	   	ssd_list_populate (tabs[i], 0, NULL, NULL,NULL, NULL,RealtimeAlertsListCallBackTabs, RealtimeAlertsDeleteCallBackTabs, TRUE);
      }
}

///////////////////////////////////////////////////////////////////////////////////////////
static int on_type_selected (SsdWidget widget, const char* selection,const void *value, void* context) {
	int type = (int) value;
	
	on_tab_gain_focus(type);
   
	RealtimeAlertsListShow(FALSE);
	
	return 1;
}


///////////////////////////////////////////////////////////////////////////////////////////
static int on_selector (SsdWidget widget, const char* selection,const void *value, void* context) {
	int selector_index = (int) value;
   
   if (!g_list_menu)
      return 0;
   
   switch (selector_index) {
      case 0:
         g_list_filter = filter_none;
         break;
      case 1:
         g_list_filter = filter_on_route;
         break;
      case 2:
         g_list_filter = filter_group;
         break;
      default:
         g_list_filter = filter_none;
         break;
   }
   
   if ((int)context == TYPES_LIST) {
      free_list();
      show_types_list(TRUE, FALSE);
   } else if ((int)context == ALERTS_LIST) {
      free_list();
      show_types_list(TRUE, FALSE);
      RealtimeAlertsListShow(TRUE);
   }
   
   return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////
static void	show_types_list(BOOL refresh, BOOL no_animation){
	const char* tabs_titles[tab__count]; //for iPhone, these are not actually tabs...
	const char* tabs_icons[tab__count];
	const char* tabs_values[tab__count];
   const char* tabs_item_count[tab__count];
   char str_count[20];
   list_menu_empty_message empty_message, *p_empty_message = NULL;
   int count;
   int flags = LIST_MENU_ADD_NEXT_BUTTON;
   
   message_init(&empty_message);
   
   RTAlerts_Sort_List(sort_proximity);
   
   RTAlerts_Stop_Scrolling();
   
   populate_list();
	
	//careful with this list, must fill exactly tab__count number of items
	tabs_titles[tab_all] = roadmap_lang_get("All");
	tabs_icons[tab_all] = "report_list_all";
	tabs_values[tab_all] = (void *)(tab_all);
   sprintf (str_count,"%d",count_tab(tab_all,1,0));
   tabs_item_count[tab_all] = strdup(str_count);
   
	tabs_titles[tab_police] = roadmap_lang_get("Police");
	tabs_icons[tab_police] = "report_list_police";
	tabs_values[tab_police] = (void *)(tab_police);
   sprintf (str_count,"%d",count_tab(tab_police,1,RT_ALERT_TYPE_POLICE));
   tabs_item_count[tab_police] = strdup(str_count);
   
	tabs_titles[tab_traffic_jam] = roadmap_lang_get("Traffic");
	tabs_icons[tab_traffic_jam] = "report_list_loads";
	tabs_values[tab_traffic_jam] = (void *)(tab_traffic_jam);
   sprintf (str_count,"%d",count_tab(tab_traffic_jam,2, RT_ALERT_TYPE_TRAFFIC_INFO, RT_ALERT_TYPE_TRAFFIC_JAM));
   tabs_item_count[tab_traffic_jam] = strdup(str_count);
   
	tabs_titles[tab_accidents] = roadmap_lang_get("Accidents");
	tabs_icons[tab_accidents] = "report_list_accidents";
	tabs_values[tab_accidents] = (void *)(tab_accidents);
   sprintf (str_count,"%d",count_tab(tab_accidents,1, RT_ALERT_TYPE_ACCIDENT));
   tabs_item_count[tab_accidents] = strdup(str_count);
   
   tabs_titles[tab_chit_chat] = roadmap_lang_get("Chit Chats");
	tabs_icons[tab_chit_chat] = "report_list_chit_chats";
	tabs_values[tab_chit_chat] = (void *)(tab_chit_chat);
   sprintf (str_count,"%d",count_tab(tab_chit_chat,1, RT_ALERT_TYPE_CHIT_CHAT));
   tabs_item_count[tab_chit_chat] = strdup(str_count);
   
	tabs_titles[tab_others] = roadmap_lang_get("Other");
	tabs_icons[tab_others] = "report_list_other";
	tabs_values[tab_others] = (void *)(tab_others);
   sprintf (str_count,"%d",count_tab(tab_others,5, RT_ALERT_TYPE_HAZARD,RT_ALERT_TYPE_OTHER, RT_ALERT_TYPE_CONSTRUCTION, RT_ALERT_TYPE_PARKING, RT_ALERT_TYPE_DYNAMIC));
   tabs_item_count[tab_others] = strdup(str_count);

#ifdef STREAM_TEST
   tabs_titles[tab_stream] = roadmap_lang_get("Radio road reports");
	tabs_icons[tab_stream] = "radio";
	tabs_values[tab_stream] = (void *)(tab_stream);
   tabs_item_count[tab_stream] = "";
#endif //STREAM_TEST
   
   if (g_list_filter == filter_on_route &&
       (navigate_main_state() == -1) &&
       RealTimeLoginState()) {
      count = 0;
      empty_message = create_no_route_message();
      p_empty_message = &empty_message;
   } else  if (g_list_filter == filter_group &&
               (roadmap_groups_get_num_following() == 0) &&
               RealTimeLoginState()) {
      count = 0;
      empty_message = create_no_group_message();
      p_empty_message = &empty_message;
   } else {
      count = tab__count;
   }
   
   if (no_animation)
      flags |= LIST_MENU_NO_ANIMATION;
   
   if (!refresh || !g_list_menu)
      g_list_menu = roadmap_list_menu_generic("Events", count, tabs_titles, 
                                              (const void**)tabs_values, tabs_icons, tabs_item_count, 
                                              get_selector(), on_type_selected, NULL,
                                              (void *)TYPES_LIST, NULL, NULL, 60, flags, p_empty_message);
   else
      roadmap_list_menu_generic_refresh(g_list_menu, "Events", count, tabs_titles, 
                                        (const void**)tabs_values, tabs_icons, tabs_item_count, 
                                        get_selector(), on_type_selected, NULL,
                                        (void *)TYPES_LIST, NULL, NULL, 60, flags, p_empty_message);
   
   
	
   free ((char*)tabs_item_count[tab_all]);
   free ((char*)tabs_item_count[tab_police]);
   free ((char*)tabs_item_count[tab_traffic_jam]);
   free ((char*)tabs_item_count[tab_accidents]);
   free ((char*)tabs_item_count[tab_chit_chat]);
   free ((char*)tabs_item_count[tab_others]);
#ifdef STREAM_TEST
   //free ((char*)tabs_item_count[tab_stream]);
#endif //STREAM_TEST
}

///////////////////////////////////////////////////////////////////////////////////////////
static void init(){   
   if (!initialized) {
		gColorType = [[UIColor alloc] initWithRed:1.0f green:0.45 blue:0.0f alpha:1.0f]; //orange
      gColorUnit = [UIColor lightGrayColor];
		initialized = 1;
      
      gFilterSelector.items = selector_options;
      gFilterSelector.count = 3;
      gFilterSelector.on_selector = on_selector;
	} else {
		free_list(); //TODO: we better release the list once we are done with it...
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
void RealtimeAlertsList(){
	init();
   g_list_filter = filter_none;
	show_types_list(FALSE, FALSE);
}

///////////////////////////////////////////////////////////////////////////////////////////
void RealtimeAlertsListGroup(){
	init();
   g_list_filter = filter_group;
	show_types_list(FALSE, TRUE);
   on_tab_gain_focus(tab_all);
   RealtimeAlertsListShow(FALSE);
}


