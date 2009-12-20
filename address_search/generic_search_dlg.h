/* generic_search_dlg.h
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi Ben-Shoshan
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


#ifndef GENERIC_SEARCH_DLG_H_
#define GENERIC_SEARCH_DLG_H_

#include "../ssd/ssd_dialog.h"

typedef void(*GenericSearchOnReopen)(PFN_ON_DIALOG_CLOSED cbOnClosed, void* context);

void        generic_search_dlg_show( search_types   type,
                                     const char *dlg_name, 
                                     const char *dlg_title,
                                     SsdSoftKeyCallback left_sk_callback,
                                     SsdSoftKeyCallback right_sk_callback,
                                     SsdWidget rcnt,
                                     PFN_ON_DIALOG_CLOSED cbOnClosed,
                                     RoadMapCallback on_search,
                                     GenericSearchOnReopen on_reopen,
                                     void*           context);

void        generic_search_dlg_switch_gui(void);
SsdWidget   generic_search_dlg_get_search_edit_box(search_types type);
SsdWidget   generic_search_dlg_get_search_dlg(search_types type);
BOOL        generic_search_dlg_is_1st(search_types type);
void        generic_search_dlg_reopen_native_keyboard(void);
#endif /* GENERIC_SEARCH_DLG_H_ */
