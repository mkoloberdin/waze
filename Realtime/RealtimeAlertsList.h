/* RealtimeAlertsList.h - Manage real time alerts List
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi B.S
 *   Copyright 2008 Ehud Shabtai
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

#include "ssd/ssd_widget.h"


#ifndef	__REALTIME_ALERTS_LIST_H__
#define	__REALTIME_ALERTS_LIST_H__

#define MAX_ALERTS_ENTRIES 100
#define ALERTS_LIST_TITLE "Real Time Alerts"

// Context menu:
typedef enum real_time_list_context_menu_items
{
   rtl_cm_show,
   rtl_cm_delete,
   rtl_cm_view_comments,
   rtl_cm_add_comments,
   rtl_cm_sort_proximity,
   rtl_cm_sort_recency,
   rtl_cm_exit,
   rtl_cm_cancel,
   rtl_cm_view_image,
   rtl_cm_report_abuse,
   rtl_cm_on_route,
   rtl_cm_filter_group,
   rtl_cm_show_group,
   rtl_cm_show_all,
   rtl_cm_send_thumbs_up,
   rtl_cm__count,
   rtl_cm__invalid

}  real_time_list_context_menu_items;

// Tabs
typedef enum tabs_real_time_list
{
#ifdef STREAM_TEST
   tab_stream,
#endif //STREAM_TEST
   tab_all,
   tab_police,
   tab_traffic_jam,
   tab_accidents,
   tab_chit_chat,
   tab_others,

   tab__count,
   tab_invalid__invalid

}  real_time_tabs;

#ifndef IPHONE
typedef struct AlertList_s{
   char *labels[MAX_ALERTS_ENTRIES];
   char *values[MAX_ALERTS_ENTRIES];
   char *icons[MAX_ALERTS_ENTRIES];
   SsdWidget icons_widget[MAX_ALERTS_ENTRIES];
   SsdWidget labels_widget[MAX_ALERTS_ENTRIES];
	int type[MAX_ALERTS_ENTRIES];
	int iDistnace[MAX_ALERTS_ENTRIES];
	int iCount;
}AlertList;
#endif //IPHONE


void RealtimeAlertsList (void);
void RealtimeAlertsListGroup(void);

#endif //__REALTIME_ALERTS_LIST_H__
