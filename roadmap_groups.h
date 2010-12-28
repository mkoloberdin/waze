/*
 * LICENSE:
 *
 *   Copyright 2010 Avi Ben-Shoshan
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

#ifndef ROADMAP_GROUPS_H_
#define ROADMAP_GROUPS_H_

#define GROUP_RELEVANCE_NONE      0
#define GROUP_RELEVANCE_FOLLOWING 1
#define GROUP_RELEVANCE_ACTIVE    2

#define POPUP_REPORT_NONE             0
#define POPUP_REPORT_FOLLOWING_GROUPS 1
#define POPUP_REPORT_ONLY_MAIN_GROUP  2

#define POPUP_REPORT_VAL_NONE             "none"
#define POPUP_REPORT_VAL_FOLLOWING_GROUPS "following"
#define POPUP_REPORT_VAL_ONLY_MAIN_GROUP  "main"

#define SHOW_WAZER_GROUP_ALL          0
#define SHOW_WAZER_GROUP_FOLLOWING    1
#define SHOW_WAZER_GROUP_MAIN         2

#define SHOW_WAZER_GROUP_VAL_ALL          "All"
#define SHOW_WAZER_GROUP_VAL_FOLLOWING    "following"
#define SHOW_WAZER_GROUP_VAL_MAIN         "main"

#define GROUP_NAME_MAX_SIZE 250
#define GROUP_ICON_MAX_SIZE 100

void  roadmap_groups_init(void);
void  roadmap_groups_term(void);

BOOL roadmap_groups_display_groups_supported (void);

int   roadmap_groups_get_popup_config(void);
void  roadmap_groups_set_popup_config( const char * popup_val);

int   roadmap_groups_get_show_wazer_config(void);
void  roadmap_groups_set_show_wazer_config( const char * show_wazer_val);

void  roadmap_groups_set_num_following(int num_groups);
int   roadmap_groups_get_num_following(void);

void roadmap_groups_set_active_group_name(char *name);
const char *roadmap_groups_get_active_group_name(void);

void roadmap_groups_set_active_group_icon(char *name);
const char *roadmap_groups_get_active_group_icon(void);
const char *roadmap_groups_get_active_group_wazer_icon(void);

void roadmap_groups_add_following_group_icon(int index, char *name);
void roadmap_groups_add_following_group_name(int index, char *name);

const char *roadmap_groups_get_following_group_icon(int index);
const char *roadmap_groups_get_following_group_name(int index);

const char * roadmap_groups_get_selected_group_name(void);
const char * roadmap_groups_get_selected_group_icon(void);
void roadmap_groups_set_selected_group_name(const char *name);
void roadmap_groups_set_selected_group_icon(const char *icon);

void roadmap_groups_show(void);
void roadmap_groups_show_group(const char *group_name);

void roadmap_groups_popup(void);

void roadmap_groups_alerts_action(void);

void roadmap_groups_dialog (RoadMapCallback callback);
/*
 * Browser callbacks - platform depended implementation
 */
void roadmap_groups_browser_btn_close_cb( void );
void roadmap_groups_browser_btn_back_cb( void );
void roadmap_groups_browser_btn_home_cb( void );

#endif /* ROADMAP_GROUPS_H_ */
