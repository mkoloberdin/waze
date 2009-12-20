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


#ifndef __SSD_TABCONTROL_H__
#define __SSD_TABCONTROL_H__
  
#include "ssd/ssd_widget.h"
#include "ssd/ssd_dialog.h"

typedef void* SsdTcCtx;

typedef BOOL(*PFN_ON_TAB_LOOSE_FOCUS)  (  int         tab);
typedef void(*PFN_ON_TAB_GAIN_FOCUS)   (  int         tab);
typedef BOOL(*PFN_ON_KEY_PRESSED)      (  int         tab,
                                          const char* utf8char, 
                                          uint32_t    flags);

SsdTcCtx    ssd_tabcontrol_new(  const char*             name,
                                 const char*             title,
                                 PFN_ON_DIALOG_CLOSED    on_tabcontrol_closed,
                                 PFN_ON_TAB_LOOSE_FOCUS  on_tab_loose_focus,
                                 PFN_ON_TAB_GAIN_FOCUS   on_tab_gain_focus,
                                 PFN_ON_KEY_PRESSED      on_unhandled_key_pressed,
                                 const char*             tabs_titles[],
                                 SsdWidget               tabs[],
                                 int                     tabs_count,
                                 int                     active_tab);

void        ssd_tabcontrol_show           ( SsdTcCtx tabcontrol);                
SsdWidget   ssd_tabcontrol_get_tab        ( SsdTcCtx tabcontrol, int tab);
SsdWidget   ssd_tabcontrol_get_active_tab ( SsdTcCtx tabcontrol);
void        ssd_tabcontrol_set_active_tab ( SsdTcCtx tabcontrol, int tab);
SsdWidget   ssd_tabcontrol_get_dialog     ( SsdTcCtx tabcontrol);
void        ssd_tabcontrol_free           ( SsdTcCtx tabcontrol);
void        ssd_tabcontrol_set_title      ( SsdTcCtx tabcontrol, const char* title);

void        ssd_tabcontrol_move_tab_left  ( SsdWidget dialog);
void        ssd_tabcontrol_move_tab_right ( SsdWidget dialog);
int         ssd_tabcontrol_get_height();
#endif // __SSD_TABCONTROL_H__
