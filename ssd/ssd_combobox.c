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


#include <string.h>
#include <stdlib.h>
#include "roadmap.h"
#include "ssd_container.h"
#include "ssd_text.h"
#include "roadmap_keyboard.h"

#include "ssd_combobox.h"

#define  SSD_COMBOBOX_EDITCNT_NAME     ("combobox edit-container")
#define  SSD_COMBOBOX_EDITBOX_NAME     ("combobox edit-box")
#define  SSD_COMBOBOX_LIST_NAME        ("combobox list")
#define  SSD_COMBOBOX_CTXID            ("combobox context")
#define  SSD_COMBOBOX_CTXID_SIZE       (sizeof(SSD_COMBOBOX_CTXID) + 1)

typedef struct tag_ssd_combobox_context
{
   int                     size;
   char                    typeid[SSD_COMBOBOX_CTXID_SIZE+1];
   PFN_ON_INPUT_CHANGED    on_text_changed;
   SsdListCallback         on_list_selection;
   SsdListDeleteCallback   on_delete_list_item;
   void*                   context;

}  ssd_combobox_context, *ssd_combobox_context_ptr;

void ssd_combobox_context_init( ssd_combobox_context_ptr this)
{
   memset( this, 0, sizeof(ssd_combobox_context));
   this->size = sizeof(ssd_combobox_context);
   strcpy( this->typeid, SSD_COMBOBOX_CTXID);
}

BOOL ssd_combobox_context_verify_pointer( ssd_combobox_context_ptr this)
{
   if( this &&
      (sizeof(ssd_combobox_context) == this->size) &&
      (0 == strncmp( this->typeid, SSD_COMBOBOX_CTXID, SSD_COMBOBOX_CTXID_SIZE)))
      return TRUE;
      
   return FALSE;
}

static int on_list_selection( SsdWidget this, const char* selection, const void* data)
{
   SsdWidget                  main_cont= this->parent;
   ssd_combobox_context_ptr   ctx      = ssd_widget_get_context(main_cont);
   SsdWidget                  edit     = ssd_widget_get( main_cont, SSD_COMBOBOX_EDITBOX_NAME);
   
   assert( this);
   assert( main_cont);
   assert( ssd_combobox_context_verify_pointer( ctx));
   assert( edit);
   
   ssd_text_set_text( edit, selection);

   if(ctx->on_list_selection)
      ctx->on_list_selection( this, selection, data);
      
   return 1;
}

static BOOL on_key_pressed__delegate_to_editbox(   
                     SsdWidget   this, 
                     const char* utf8char, 
                     uint32_t    flags)
{
   SsdWidget                  editbox  = NULL;
   SsdWidget                  main_cont= this->parent;
   ssd_combobox_context_ptr   ctx      = NULL;
   
   assert( this);
   assert( main_cont);
   assert( this->children);
   assert( this->children->key_pressed);
   
   //   Special case:   move focus to the list
   if( KEY_IS_ENTER)
   {
      SsdWidget list                = ssd_combobox_get_list( main_cont);
      SsdWidget first_item_in_list  = ssd_list_get_first_item( list);
      
      if( first_item_in_list)
      {
         ssd_dialog_set_focus( first_item_in_list);
         return TRUE;
      }
      
      return FALSE;
   }
   
   editbox = this->children;
   if( !editbox->key_pressed( editbox, utf8char, flags))
   {
      if( VKEY_IS_UP || VKEY_IS_DOWN)
      {
         SsdWidget list = ssd_combobox_get_list( main_cont);
         return ssd_list_set_focus( list, VKEY_IS_DOWN);
      }
   
      return FALSE;
   }
   
   ctx = ssd_widget_get_context(main_cont);
   assert( ssd_combobox_context_verify_pointer( ctx));
   if( ctx->on_text_changed)
   {
      const char* new_text = ssd_text_get_text( editbox);
      if( new_text)
         ctx->on_text_changed( new_text, ctx->context);
   }
   
   return TRUE;
}

static BOOL on_key_pressed__unhandled_event_from_list(   
                     SsdWidget   listbox,
                     const char* utf8char, 
                     uint32_t      flags)
{
   SsdWidget editbox_container= NULL;
   SsdWidget current_tab      = NULL;
   
   if( !listbox || !listbox->parent || !listbox->parent->parent || !listbox->parent->parent->parent)
   {
      assert(0);
      return FALSE;
   }
   
   current_tab       = listbox->parent->parent->parent;
   editbox_container = ssd_widget_get( current_tab, SSD_COMBOBOX_EDITCNT_NAME);
   
   assert(editbox_container);
   
   if( on_key_pressed__delegate_to_editbox( editbox_container, utf8char, flags))
   {
      ssd_dialog_set_focus( editbox_container);
      return TRUE;
   }
   
   return FALSE;
}                   

void* ssd_combobox_get_context( SsdWidget main_cont)
{
   ssd_combobox_context_ptr ctx = ssd_widget_get_context( main_cont);
   return ctx->context;
}

void ssd_combobox_free( SsdWidget main_cont)
{
   ssd_combobox_context_ptr ctx = ssd_widget_get_context( main_cont);
   
   if( ssd_combobox_context_verify_pointer( ctx))
   {
      ssd_widget_set_context( main_cont, NULL);
      free( ctx);
      ctx = NULL;
   }
}

void ssd_combobox_new(  SsdWidget               main_cont,
                        const char*             title, 
                        int                     count, 
                        const char**            labels,
                        const void**            values,
                        const char**			   icons,
                        PFN_ON_INPUT_CHANGED    on_text_changed,     // User modified edit-box text
                        SsdListCallback         on_list_selection,   // User selected iterm from list
                        SsdListDeleteCallback   on_delete_list_item, // User is trying to delete an item from the list
                        unsigned short          input_type,          // inputtype_<xxx> combination from 'roadmap_input_type.h'
                        void*                   context)
{
   ssd_combobox_context_ptr   ctx  = NULL;
   SsdWidget                  edit = NULL;
   SsdWidget                  ecnt = NULL;
   SsdWidget                  list = NULL;
   
   if( !main_cont)
   {
      assert(0);
      return;
   }
   
   // context:
   ctx = ssd_widget_get_context( main_cont);
   if( !ctx)
      ctx  = malloc( sizeof(ssd_combobox_context));
   else
   {
      if( ssd_combobox_context_verify_pointer( ctx))
      {
         // If parent container (main_cont) was used before, AND IF the context memory
         // pointer was never freed (ssd_combobox_free() was never called), THEN we will 
         // reuse the allocated memory
      }
      else
      {
         // Combobox-parent should not use its context pointer...
         assert(0);
         return;
      }
   }

   ssd_combobox_context_init( ctx);
   ctx->on_text_changed       = on_text_changed;
   ctx->on_list_selection     = on_list_selection;
   ctx->on_delete_list_item   = on_delete_list_item;
   ctx->context               = context;
   ssd_widget_set_context( main_cont, ctx);
   
   ecnt = ssd_container_new( SSD_COMBOBOX_EDITCNT_NAME, NULL, SSD_MAX_SIZE, 30, SSD_WS_TABSTOP|SSD_CONTAINER_BORDER|SSD_END_ROW);
   edit = ssd_text_new     ( SSD_COMBOBOX_EDITBOX_NAME, "", 18, 0);
   list = ssd_list_new( SSD_COMBOBOX_LIST_NAME, SSD_MAX_SIZE, SSD_MAX_SIZE, input_type, 0, on_key_pressed__unhandled_event_from_list);
   
   ssd_text_set_input_type( edit, input_type);
   ssd_text_set_readonly  ( edit, FALSE);

   //   Delegate the 'on key pressed' event to the child edit-box:
   ecnt->key_pressed = on_key_pressed__delegate_to_editbox;

   ssd_widget_add( ecnt, edit);
   ssd_widget_add( main_cont, ecnt);
   ssd_widget_add( main_cont, list);

   //   Tab-Order:   Connect between the list-box and the edit-box
   ssd_list_taborder__set_widget_before_list(list, ecnt /* Edit-box container */);
   ssd_list_taborder__set_widget_after_list( list, ecnt /* Edit-box container */);

   main_cont->set_value( main_cont, title);

   ssd_widget_reset_cache( list->parent);
   ssd_list_resize(list, -1);
   ssd_list_populate( list, count, labels, values, icons, NULL, on_list_selection, on_delete_list_item, FALSE);
}

void ssd_combobox_reset_focus( SsdWidget main_cont)
{
   SsdWidget cont = ssd_widget_get( main_cont, SSD_COMBOBOX_EDITCNT_NAME);

   ssd_dialog_set_focus ( cont);
}


void ssd_combobox_reset_state( SsdWidget main_cont)
{
   SsdWidget list = ssd_widget_get( main_cont, SSD_COMBOBOX_LIST_NAME);
   SsdWidget edit = ssd_widget_get( main_cont, SSD_COMBOBOX_EDITBOX_NAME);

   ssd_widget_reset_cache( list->parent);
   ssd_list_resize  ( list, -1);
   ssd_text_reset_text  ( edit);
}

SsdWidget ssd_combobox_get_textbox( SsdWidget main_cont)
{ return ssd_widget_get( main_cont, SSD_COMBOBOX_EDITBOX_NAME);}

const char* ssd_combobox_get_text( SsdWidget main_cont)
{
   SsdWidget edit = ssd_combobox_get_textbox( main_cont);
   
   return ssd_text_get_text(edit);
}

void ssd_combobox_set_text(SsdWidget   main_cont,
                           const char* text)
{
   SsdWidget edit = ssd_combobox_get_textbox( main_cont);
   
   ssd_text_set_text( edit, text);
}

SsdWidget ssd_combobox_get_list( SsdWidget main_cont)
{ return ssd_widget_get( main_cont, SSD_COMBOBOX_LIST_NAME);}

//   Update list-box with new values:
void ssd_combobox_update_list(SsdWidget      main_cont,
                              int            count, 
                              const char**   labels,
                              const void**   values,
                              const char**   icons)
{ 
   ssd_combobox_context_ptr   ctx   = NULL;
   SsdWidget                  list  = NULL;
   SsdWidget                  focus = NULL;
   
   //   1.   Load new values:
   list = ssd_widget_get( main_cont, SSD_COMBOBOX_LIST_NAME);
   assert(list);

   if( NULL == ssd_list_item_has_focus(list))
      focus = ssd_dialog_get_focus();
   
   ctx = ssd_widget_get_context( main_cont);
   ssd_list_populate( list, count, labels, values, NULL, NULL,on_list_selection, ctx->on_delete_list_item, FALSE);
   
   //   2.   Re-sort tab-order:
   ssd_dialog_resort_tab_order();
   
   if( focus)
      ssd_dialog_set_focus( focus);
}                              
