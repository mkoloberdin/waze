/*
 * LICENSE:
 *
 *   Copyright 2008 PazO
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
 */


#ifndef __SSD_TABCONTROL_DEFS_H__
#define __SSD_TABCONTROL_DEFS_H__
  
#define  MAX_TAB_COUNT                 ( 7)
#define  TAB_TEXT_SIZE                 ( 6)
#define  INVALID_TAB_INDEX             (-1)
#define  TAB_TITLE_MAX_SIZE            (24)
#define  DIALOG_NAME                   ("tabcontrol.dialog")
#define  CONTAINER_NAME                ("tabcontrol.container")
#define  TABS_LINE__CONTAINER          ("tabcontrol.tabs.container")
#define  TABS_LINE__LEFT_ARROW         ("tabcontrol.tabs.left.arrow")
#define  TABS_LINE__LEFT_TAB           ("tabcontrol.tabs.left.tab")
#define  TABS_LINE__LEFT_TAB_TEXT      ("tabcontrol.tabs.left.tab.text")
#define  TABS_LINE__RIGHT_TAB          ("tabcontrol.tabs.right.tab")
#define  TABS_LINE__RIGHT_TAB_TEXT     ("tabcontrol.tabs.right.tab.text")
#define  TABS_LINE__RIGHT_ARROW        ("tabcontrol.tabs.right.arrow")
#define  SELECTED_TEXT_COLOR           ("#000000")
#define  UNSELECTED_TEXT_COLOR         ("#3a3a3a")
#define  ICON_IMAGES                   tab_images
#define  ICON_IMAGES_COUNT             (sizeof(tab_images)/sizeof(ssd_icon_wimage_set))
#define  LEFT_ARROW_IMAGES             left_arrow_images
#define  LEFT_ARROW_IMAGES_COUNT       (sizeof(left_arrow_images)/sizeof(ssd_icon_image_set))
#define  RIGHT_ARROW_IMAGES            right_arrow_images
#define  RIGHT_ARROW_IMAGES_COUNT      (sizeof(right_arrow_images)/sizeof(ssd_icon_image_set))
#define  ARROW_DISABLED                (1)
#define  ARROW_ENABLED                 (0)
#define  TAB_SELECTED                  (1)
#define  TAB_NOT_SELECTED              (0)

typedef enum tag_tabsline_side
{
   left,
   right
   
}  tabsline_side;

// Tabs-line (GUI) info:
typedef struct tag_tabsline_info
{
   SsdWidget   tabs_container;
   int         tabs_height;
   BOOL        drawing_initialized;
   BOOL        left_tab_selected;

}  tabsline_info, *tabsline_info_ptr;

// Client-GUI tab info
typedef struct tag_tab_info
{
   const char*       title;
   SsdWidget         tab;
   SsdDrawCallback   draw_cb;
   
}  tab_info, *tab_info_ptr;

// Tab-control context:
typedef struct tag_tabcontrol_info
{
   tabsline_info           tli;
   const char*             dialog_name;
   SsdWidget               dialog;
   SsdWidget               container;
   PFN_ON_DIALOG_CLOSED    on_tbctrl_closed;
   PFN_ON_TAB_LOOSE_FOCUS  on_tab_loose_focus;
   PFN_ON_TAB_GAIN_FOCUS   on_tab_gain_focus;
   PFN_ON_KEY_PRESSED      on_unhandled_key_pressed;
   tab_info                tabs[MAX_TAB_COUNT];
   int                     tabs_count;
   int                     active_tab;
   int                     orientation_change_id;
   RoadMapGuiRect          draw_rect;

}  tabcontrol_info, *tabcontrol_info_ptr;

void  tabcontrol_info_init( tabcontrol_info_ptr this);

#endif // __SSD_TABCONTROL_DEFS_H__
