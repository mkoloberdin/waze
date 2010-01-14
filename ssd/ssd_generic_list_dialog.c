/* ssd_generic_list_dialog.c
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

#include <stdlib.h>
#include "roadmap_lang.h"
#include "ssd_dialog.h"
#include "ssd_list.h"

#include "ssd_generic_list_dialog.h"


typedef struct ssd_list_context {
   PFN_ON_ITEM_SELECTED   on_item_selected;
   PFN_ON_ITEM_SELECTED   on_item_deleted;
   SsdSoftKeyCallback left_softkey_callback;
   void*   context;

}   SsdListContext;


static int list_callback (SsdWidget widget, const char *new_value, const void *value)
{
   SsdWidget      dialog   = widget->parent;
   SsdListContext*   context   = (SsdListContext *)dialog->context;

   if (context->on_item_selected == NULL)
     return 0;

   return (*context->on_item_selected)(widget, new_value, value,  context->context);
}

static int del_callback (SsdWidget widget, const char *new_value, const void *value)
{
   SsdWidget      dialog   = widget->parent;
   SsdListContext*   context   = (SsdListContext *)dialog->context;

   if (context->on_item_deleted == NULL)
      return 0;

   return (*context->on_item_deleted)(widget, new_value, value,  context->context);
}

static int list_left_softkey_callback (SsdWidget widget, const char *new_value, void *context)
{
   SsdListContext*   list_context   = (SsdListContext *)widget->context;
   if (list_context->left_softkey_callback != NULL)
   return (*list_context->left_softkey_callback)(widget, new_value,  list_context->context);
   else
      return 0;
}

static SsdWidget GenericList = NULL;

static void on_dialog_closed( int type, void *context)
{
	ssd_widget_set_left_softkey_callback(GenericList,NULL);
}




void ssd_generic_list_dialog_show(const char*            title,
                                  int                    count,
                                  const char**           labels,
                                  const void**           values,
                                  PFN_ON_ITEM_SELECTED   on_item_selected,
                                  PFN_ON_ITEM_SELECTED   on_item_deleted,
                                  void*                  context,
                                  int                     list_height )
{
   static SsdListContext list_context;

   SsdWidget list;

   list_context.on_item_selected= on_item_selected;
   list_context.on_item_deleted = on_item_deleted;
   list_context.context         = context;
   list_context.left_softkey_callback = NULL;

   if (!GenericList)
   {
      GenericList = ssd_dialog_new ("generic_list", "", on_dialog_closed,
                                    SSD_CONTAINER_TITLE);
      list = ssd_list_new ("list", SSD_MAX_SIZE, SSD_MAX_SIZE, inputtype_none, 0, NULL);

      ssd_widget_add (GenericList, list);
   }

   list = ssd_widget_get (GenericList, "list");
   ssd_widget_set_offset(GenericList,0,0);
   
   ssd_widget_set_left_softkey_text(GenericList, roadmap_lang_get("Exit_key"));
   ssd_widget_set_left_softkey_callback(GenericList, NULL);

   GenericList->set_value (GenericList->parent, title);
   ssd_widget_set_context (GenericList, &list_context);

   ssd_widget_reset_cache (list->parent);
   ssd_widget_reset_position(GenericList);
   ssd_list_resize ( list, list_height );
   ssd_list_populate (list, count, labels, values, NULL, NULL, list_callback, del_callback, FALSE);
   ssd_dialog_activate ("generic_list", NULL);
   ssd_dialog_draw ();
 }

void ssd_generic_icon_list_dialog_show(
                                  const char*            title,
                                  int                    count,
                                  const char**           labels,
                                  const void**           values,
                                  const char**           icons,
                                  const int*             flags,
                                  PFN_ON_ITEM_SELECTED   on_item_selected,
                                  PFN_ON_ITEM_SELECTED   on_item_deleted,
                                  void*                  context,
                                  const char*            left_softkey_text,
                                  SsdSoftKeyCallback     left_softkey_callback,
                                  int                    list_height,
                                  int                    dialog_flags,
                                  BOOL                   add_next_button)
{
   static SsdListContext list_context;

   SsdWidget list;

   list_context.on_item_selected= on_item_selected;
   list_context.on_item_deleted = on_item_deleted;
   list_context.context         = context;
   list_context.left_softkey_callback = left_softkey_callback;
   if (!GenericList)
   {
      GenericList   = ssd_dialog_new ("generic_list", "", on_dialog_closed, SSD_CONTAINER_TITLE|dialog_flags);
      list         = ssd_list_new ("list", SSD_MAX_SIZE, SSD_MAX_SIZE, inputtype_none, 0, NULL);

      ssd_widget_add (GenericList, list);
   }
   else{
     GenericList->flags &= ~SSD_HEADER_BLACK;
     GenericList->flags |= dialog_flags;
   }
   ssd_widget_set_offset(GenericList,0,0);
   list = ssd_widget_get (GenericList, "list");
   
   GenericList->set_value (GenericList->parent, title);
   ssd_widget_set_context (GenericList, &list_context);


   ssd_widget_reset_cache (list->parent);
   ssd_widget_reset_position(GenericList);
   ssd_list_resize (list, list_height   );
   ssd_list_populate (list, count, labels, values,icons,flags, list_callback, del_callback, add_next_button);
   ssd_widget_set_left_softkey_text(GenericList, left_softkey_text);
   if (left_softkey_callback != NULL)
      ssd_widget_set_left_softkey_callback(GenericList, list_left_softkey_callback);

   
   ssd_dialog_activate ("generic_list", NULL);
   ssd_dialog_draw ();
}

void* ssd_generic_list_dialog_get_context(){
   SsdListContext *ctx = ssd_widget_get_context(GenericList);
   return ctx->context;
}

const char* ssd_generic_list_dialog_selected_string (void){
   return ssd_list_selected_string(ssd_widget_get (GenericList, "list"));
}

const void* ssd_generic_list_dialog_selected_value  (void){
   return ssd_list_selected_value(ssd_widget_get (GenericList, "list"));
}


void ssd_generic_list_dialog_hide (void)
{ ssd_dialog_hide ("generic_list", dec_close);}

void ssd_generic_list_dialog_hide_all (void)
{ ssd_dialog_hide_all (dec_close);}
