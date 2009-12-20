/* RealtimeAlertCommentList.h - Manage real time alert Comments List
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


#ifndef	__REALTIME_ALERT_COMMENT_LIST_H__
#define	__REALTIME_ALERT_COMMENT_LIST_H__

// Context menu:
typedef enum real_time_comments_list_context_menu_items
{
   rtcl_cm_show,
   rtcl_cm_add_comments,
   rtcl_cm_exit,
   
   rtcl_cm__count,
   rtcl_cm__invalid

}  real_time_comments_list_context_menu_items;

int RealtimeAlertCommentsList (int alertId); 

#endif //__REALTIME_ALERT_COMMENT_LIST_H__
