/* ssd_generic_list_dialog.h
 *
 * LICENSE:
 *
 *   Copyright 2008 Ehud Shabtai
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


#ifndef _SSD_GENERIC_LIST_DIALOG_H__
#define _SSD_GENERIC_LIST_DIALOG_H__

#ifdef ANDROID
	#define SSD_GEN_LIST_ENTRY_HEIGHT 60
#else
	#define SSD_GEN_LIST_ENTRY_HEIGHT 40
#endif


typedef int(*PFN_ON_ITEM_SELECTED)(SsdWidget widget, const char* selection,const void *value, void* context);

void ssd_generic_list_dialog_show(	const char*				title,
												int						count,
												const char**			labels,
												const void**			values,
												PFN_ON_ITEM_SELECTED	on_item_selected,
												PFN_ON_ITEM_SELECTED	on_item_deleted,
												void*						context,
												int                     list_height );

void ssd_generic_icon_list_dialog_show(
                                  const char*			  title,
                                  int					  count,
                                  const char**			  labels,
                                  const void**			  values,
                                  const char**            icons,
                                  const int*			  flags,
                                  PFN_ON_ITEM_SELECTED	  on_item_selected,
                                  PFN_ON_ITEM_SELECTED    on_item_deleted,
                                  void*					  context,
                                  const char*             left_softkey_text,
                                  SsdSoftKeyCallback	  left_softkey_callback,
                                  int                     list_height,
                                  int                     dialog_flags,
                                  BOOL                    add_next_button);


void ssd_generic_list_dialog_hide (void);

void ssd_generic_list_dialog_hide_all (void);


void*       ssd_generic_list_dialog_get_context();
const char* ssd_generic_list_dialog_selected_string (void);
const void* ssd_generic_list_dialog_selected_value  (void);

#endif // _SSD_GENERIC_LIST_DIALOG_H__
