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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../roadmap_lang.h"
#include "../roadmap_screen.h"
#include "ssd_dialog.h"
#include "ssd_container.h"
#include "roadmap_pointer.h"
#include "ssd_separator.h"
#include "ssd_text.h"
#include "ssd_bitmap.h"
#include "ssd_button.h"
#include "roadmap_keyboard.h"
#include "roadmap_bar.h"
#include "roadmap_utf8.h"
#include "roadmap_device_events.h"

#include "ssd_contextmenu.h"


#define  SSD_CM_POPUP_CONTAINER_NAME_PREFIX     ("ssd_contextmenu_popup_")
#ifdef IPHONE
   #define  CONTEXT_MENU_FONT_SIZE              (18)
#else
   #define  CONTEXT_MENU_FONT_SIZE              (20)
#endif

#define  CONTEXT_MENU_SIMPLE_LIST_FONT_SIZE     (18)

typedef struct tag_cm_context
{
   SsdOnContextMenu     callback;
   ssd_contextmenu_ptr  menu;
   SsdDrawCallback      org_draw;
   BOOL                 recalc_pos;
   SsdWidget            open_popup;
   void*                context;
   int                  last_height;
   BOOL 				close_on_selection; //specifies if the context menu will close after a selection
}     cm_context, *cm_context_ptr;
void  cm_context_reset     ( cm_context_ptr this);
BOOL  cm_context_is_active ( cm_context_ptr this);

static   int         s_text_height        = 0;
static   cm_context  s_ctx                = {NULL/*CB*/,   NULL/*MENU*/,NULL/*DRAW*/,
                                             FALSE/*CALC*/,NULL/*PUP*/, NULL/*CTX*/, -1, TRUE};
static   BOOL        s_open_to_the_right  = TRUE;
static   BOOL        s_registered         = FALSE;
static   SsdSize     s_canvas_size        = {0,0};
static   SsdWidget   s_dialog             = NULL;
static   int         s_requested_size     = -1;
static   char        s_dialog_name[112];
static int on_softkey_right(SsdWidget widget, const char *new_value, void *context);
static int on_softkey_left(SsdWidget widget, const char *new_value, void *context);
static BOOL close_popup_menu( SsdWidget container);

void cm_context_reset( cm_context_ptr this)
{
   SsdDrawCallback   org_draw    = this->org_draw;
   BOOL              recalc_pos  = this->recalc_pos;

   memset( this, 0, sizeof(cm_context));

   this->org_draw    = org_draw;
   this->recalc_pos  = recalc_pos;
}

BOOL cm_context_is_active( cm_context_ptr this)
{ return this->callback || this->menu;}

void ssd_cm_item_show( ssd_cm_item_ptr this)
{ this->flags &= ~CONTEXT_MENU_FLAG_HIDDEN;}

void ssd_cm_item_hide( ssd_cm_item_ptr this)
{ this->flags |= CONTEXT_MENU_FLAG_HIDDEN;}

void ssd_context_menu_set_size( int size)
{ s_requested_size = size;}

static BOOL verify_items_count( ssd_contextmenu_ptr menu)
{
   int   i;
   int   size = 0;

   for( i=0; i<menu->item_count; i++)
      if( !(CONTEXT_MENU_FLAG_HIDDEN & menu->item[i].flags))
         size++;

   if((size < CONTEXT_MENU_MIN_ITEMS_COUNT) || (CONTEXT_MENU_MAX_ITEMS_COUNT < size))
   {
      roadmap_log(ROADMAP_ERROR, "verify_items_count() - Menu (enabled) menu count (%d) is invalid", size);
      return FALSE;
   }

   for( i=0; i<menu->item_count; i++)
      if((CONTEXT_MENU_FLAG_POPUP & menu->item[i].flags))
         if( !verify_items_count( menu->item[i].popup))  // Recursive call
            return FALSE;

   return TRUE;
}

void ssd_contextmenu_show_item(  ssd_contextmenu_ptr  this,
                                 int                  item_id,
                                 BOOL                 show,
                                 BOOL                 recursive)
{
   int i;

   for( i=0; i<this->item_count; i++)
   {
      ssd_cm_item_ptr item = this->item + i;

      if( CONTEXT_MENU_FLAG_POPUP & item->flags)
      {
         if( recursive)
            ssd_contextmenu_show_item( item->popup, item_id, show, TRUE);
      }
      else
      {
         if( item->id == item_id)
         {
            if( show)
               ssd_cm_item_show( item);
            else
               ssd_cm_item_hide( item);
         }
      }
   }
}

void ssd_contextmenu_show_item__by_action_name(
                                 ssd_contextmenu_ptr  this,
                                 const char*          name,       // NOTE: this is not the label, it is the name from the x.menu file
                                 BOOL                 show,
                                 BOOL                 recursive)
{
   int i;

   for( i=0; i<this->item_count; i++)
   {
      ssd_cm_item_ptr item = this->item + i;

      if( CONTEXT_MENU_FLAG_POPUP & item->flags)
      {
         if( recursive)
            ssd_contextmenu_show_item__by_action_name( item->popup, name, show, TRUE);
      }
      else
      {
         if( item->action && !strcmp( item->action->name, name))
         {
            if( show)
               ssd_cm_item_show( item);
            else
               ssd_cm_item_hide( item);
         }
      }
   }
}

void ssd_contextmenu_delete( ssd_contextmenu_ptr this, BOOL delete_labels)
{
   int i;

   if( !this)
      return;

   for( i=0; i<this->item_count; i++)
   {
      ssd_cm_item_ptr item = this->item + i;

      if( CONTEXT_MENU_FLAG_POPUP & item->flags)
      {
         ssd_contextmenu_delete( item->popup, delete_labels);
         free(item->popup);
      }

      if( delete_labels)
      {
         assert(0);  // Do we have a use for this?...
         free((char*)item->label);
      }
   }

   ///[BOOKMARK]:[NOTE]:[PAZ] - Not deleting parent SsdWidget
   this->container = NULL;

   free(this->item);
   free(this);
}

void close_all_popup_menus( ssd_contextmenu_ptr menu)
{
   int i;

   if( !menu)
      return;

   for( i=0; i<menu->item_count; i++)
      if( CONTEXT_MENU_FLAG_POPUP & menu->item[i].flags)
      {
         close_all_popup_menus( menu->item[i].popup);
         ssd_widget_hide( menu->item[i].popup->container);
      }
}

// Called here, to terminate menu session:
static void exit_context_menu( BOOL made_selection, ssd_cm_item_ptr item)
{
   ssd_contextmenu_ptr  menu     = s_ctx.menu;
   SsdOnContextMenu     on_menu  = s_ctx.callback;
   void*                context  = s_ctx.context;
   /* exit menu if 
    * 1. no selection was made - this is a normal exit command (initiated for instance when the back button is pressed)
    * 2. a selection was made, and a close on selection flag exists
   */
   if( !made_selection ||(made_selection&& s_ctx.close_on_selection)){
  		 ssd_dialog_hide( s_dialog_name, dec_ok);   // Will call 'on_dialog_closed()' below...
  		 close_all_popup_menus( menu);
   }
   on_menu( made_selection, item, context);
}

static void on_dialog_closed( int exit_code, void* context)
{
   if(s_ctx.callback && (dec_ok != exit_code))
   {
      close_all_popup_menus( s_ctx.menu);
      s_ctx.callback( FALSE, NULL, s_ctx.context);
   }

   cm_context_reset( &s_ctx);

   s_dialog = NULL;
}


// LeftSoftKey  handlers:
static int on_softkey_left( SsdWidget widget, const char *new_value, void *context)
{
   // Exit context menu with no selection (CANCEL):
   exit_context_menu( FALSE, NULL);
   return 0;
}

//and RightSoftKey handler:
static int on_softkey_right(SsdWidget widget, const char *new_value, void *context)
{
   if (s_ctx.open_popup)
   		close_popup_menu((s_ctx.open_popup));
   else
   		exit_context_menu( FALSE, NULL);
   return 0;
}

static BOOL scroll_to_first_item( ssd_contextmenu_ptr menu)
{
   int i;

   for( i=0; i<menu->item_count; i++)
      if( !(CONTEXT_MENU_FLAG_HIDDEN & menu->item[i].flags))
      {
         menu->item_selected = i;
         return TRUE;
      }

   return FALSE;
}

static BOOL scroll_to_last_item( ssd_contextmenu_ptr menu)
{
   int i;

   for( i=(menu->item_count - 1); 0 <= i; i--)
      if( !(CONTEXT_MENU_FLAG_HIDDEN & menu->item[i].flags))
      {
         menu->item_selected = i;
         return TRUE;
      }

   return FALSE;
}

static BOOL scroll_down( ssd_contextmenu_ptr menu)
{
   int i;

   // NOTE: In the GUI 'DOWN' is i++ (go up with index)

   for( i=(menu->item_selected + 1); i<menu->item_count; i++)
      if( !(CONTEXT_MENU_FLAG_HIDDEN & menu->item[i].flags))
      {
         menu->item_selected = i;
         return TRUE;
      }

   return scroll_to_first_item( menu);
}

static BOOL scroll_up( ssd_contextmenu_ptr menu)
{
   int i;

   // NOTE: In the GUI 'UP' is i-- (go down with index)

   for( i=(menu->item_selected - 1); 0 <= i; i--)
      if( !(CONTEXT_MENU_FLAG_HIDDEN & menu->item[i].flags))
      {
         menu->item_selected = i;
         return TRUE;
      }

   return scroll_to_last_item( menu);
}

static void set_focus_on_first_item( ssd_contextmenu_ptr this)
{
#ifndef TOUCH_SCREEN
   scroll_to_first_item( this);
   ssd_dialog_set_focus( this->item[this->item_selected].row);
#endif
}

static int get_new_container_x_offset__menu_opens_to_the_right__rtl(
               int               canvas_width,
               int               canvas_height,
               int               zero_offset_x,
               int               zero_offset_y,
               RoadMapGuiPoint*  cur_pos,
               SsdSize*          cur_size,
               SsdSize*          new_size)
{
   int   far_x    = cur_pos->x + cur_size->width + new_size->width;
   int   new_x_offset;

   new_x_offset = 2 - (zero_offset_x + new_size->width);
   if( canvas_width < far_x)
      new_x_offset += (far_x-canvas_width);

   return new_x_offset;
}

static int get_new_container_x_offset__menu_opens_to_the_left__rtl(
               int               canvas_width,
               int               canvas_height,
               int               zero_offset_x,
               int               zero_offset_y,
               RoadMapGuiPoint*  cur_pos,
               SsdSize*          cur_size,
               SsdSize*          new_size)
{
   int   new_x;
   int   new_x_offset;

   new_x       = cur_pos->x - new_size->width;
   new_x_offset= cur_size->width - zero_offset_x - 2;

   if( new_x < 0)
      new_x_offset = cur_pos->x + cur_size->width - (zero_offset_x + new_size->width);

   return new_x_offset;
}

static int get_new_container_x_offset__menu_opens_to_the_left(
               int               canvas_width,
               int               canvas_height,
               int               zero_offset_x,
               int               zero_offset_y,
               RoadMapGuiPoint*  cur_pos,
               SsdSize*          cur_size,
               SsdSize*          new_size)
{
   int   new_x    = cur_pos->x - new_size->width;
   int   new_x_offset;

   new_x_offset = 2-(zero_offset_x + new_size->width);
   if( new_x < 0)
      new_x_offset = -(zero_offset_x + cur_pos->x);

   return new_x_offset;
}

static int get_new_container_x_offset__menu_opens_to_the_right(
               int               canvas_width,
               int               canvas_height,
               int               zero_offset_x,
               int               zero_offset_y,
               RoadMapGuiPoint*  cur_pos,
               SsdSize*          cur_size,
               SsdSize*          new_size)
{
   int   far_x;
   int   new_x_offset;

   far_x       = cur_pos->x + cur_size->width + new_size->width;
   new_x_offset= cur_size->width - (zero_offset_y + 2);
   if( canvas_width < far_x)
      new_x_offset -= (far_x-canvas_width);

   return new_x_offset;
}

static int get_new_container_y_offset(
                           int               canvas_width,
                           int               canvas_height,
                           int               zero_offset_x,
                           int               zero_offset_y,
                           RoadMapGuiPoint*  cur_pos,
                           SsdSize*          cur_size,
                           SsdSize*          new_size,
                           int               selected_item)
{
   int   new_cnt_position_y;
   int   activator_offset;
   int   zero_y;
   int   new_offset_y;

   zero_y            = -cur_size->height;
   activator_offset  = (int)(((double)s_text_height * (selected_item + 1))+0.5);
   new_cnt_position_y= cur_pos->y - (new_size->height/2) + activator_offset;
   new_offset_y      = zero_y + activator_offset - (new_size->height/2);

   if( new_cnt_position_y < TITLE_BAR_HEIGHT)
      new_offset_y = zero_y - cur_pos->y + TITLE_BAR_HEIGHT;
   else
      if( canvas_height < (new_cnt_position_y + new_size->height))
      {
         //int cur_y_end = cur_pos->y + cur_size->height + single_corner_height;
         int cur_far_from_edge = canvas_height - cur_pos->y;
         new_offset_y = zero_y + cur_far_from_edge - new_size->height;
      }

   return new_offset_y;
}

static BOOL open_popup_menu( SsdWidget cur_cnt, ssd_cm_item_ptr item)
{
   BOOL                 RTL;
   RoadMapGuiPoint      cur_pos;
   RoadMapGuiPoint      new_pos;
   SsdSize              cur_size;
   SsdSize              new_size;
   SsdWidget            row;
   ssd_contextmenu_ptr  cur_menu;
   SsdWidget            new_cnt;
   ssd_contextmenu_ptr  new_menu;
   int                  canvas_width;
   int                  canvas_height;
   int                  right_size_available;
   int                  left_size_available;
   BOOL                 open_to_the_right = s_open_to_the_right;
   int                  new_x_offset;
   int                  zero_offset_x;
   int                  zero_offset_y;

   if( !(CONTEXT_MENU_FLAG_POPUP & item->flags))
      return FALSE;

   RTL            = roadmap_lang_rtl();
   row            = item->row;
   cur_menu       = (ssd_contextmenu_ptr)row->data;
   new_cnt        = item->popup->container;
   new_menu       = item->popup;
   canvas_width   = roadmap_canvas_width();
   canvas_height  = roadmap_canvas_height() - SOFT_MENU_BAR_HEIGHT;

   ssd_container_get_visible_dimentions( cur_cnt, &cur_pos, &cur_size);
   ssd_container_get_visible_dimentions( new_cnt, &new_pos, &new_size);
   ssd_container_get_zero_offset       ( cur_cnt, &zero_offset_x, &zero_offset_y);

#ifdef   TESTING_BUILD
   temp_code__draw_visible_rect( cur_cnt);
#endif   // TESTING_BUILD

   right_size_available = canvas_width - (cur_cnt->position.x + cur_cnt->size.width);
   left_size_available  = cur_cnt->position.x;

   if( open_to_the_right)
   {
      if((right_size_available < new_cnt->size.width)   &&
         (right_size_available < left_size_available))
         open_to_the_right = FALSE;
   }
   else
   {
      if((left_size_available < new_cnt->size.width)   &&
         (left_size_available < right_size_available))
         open_to_the_right = TRUE;
   }

   if( open_to_the_right)
   {
      if( RTL)
         new_x_offset = get_new_container_x_offset__menu_opens_to_the_right__rtl(
               canvas_width,
               canvas_height,
               zero_offset_x,
               zero_offset_y,
               &cur_pos,
               &cur_size,
               &new_size);
      else
         new_x_offset = get_new_container_x_offset__menu_opens_to_the_right(
               canvas_width,
               canvas_height,
               zero_offset_x,
               zero_offset_y,
               &cur_pos,
               &cur_size,
               &new_size);
   }
   else
   {
      if( RTL)
         new_x_offset = get_new_container_x_offset__menu_opens_to_the_left__rtl(
               canvas_width,
               canvas_height,
               zero_offset_x,
               zero_offset_y,
               &cur_pos,
               &cur_size,
               &new_size);
      else
         new_x_offset = get_new_container_x_offset__menu_opens_to_the_left(
               canvas_width,
               canvas_height,
               zero_offset_x,
               zero_offset_y,
               &cur_pos,
               &cur_size,
               &new_size);
   }

   new_cnt->offset_x = new_x_offset;
   new_cnt->offset_y = get_new_container_y_offset(
                           canvas_width,
                           canvas_height,
                           zero_offset_x,
                           zero_offset_y,
                           &cur_pos,
                           &cur_size,
                           &new_size,
                           cur_menu->item_selected);

   ssd_widget_show( new_cnt);
   set_focus_on_first_item( new_menu);
   ssd_widget_set_backgroundfocus( row, TRUE);

   s_ctx.open_popup = new_cnt;

   return TRUE;
}

static BOOL close_popup_menu( SsdWidget container)
{
   SsdWidget   previous_focus = container->context;

   if( !previous_focus)
      return FALSE;

   if( strncmp( SSD_CM_POPUP_CONTAINER_NAME_PREFIX, container->name, strlen(SSD_CM_POPUP_CONTAINER_NAME_PREFIX)))
      return FALSE;

#ifdef   TESTING_BUILD
   temp_code__draw_visible_rect( container);
#endif   // TESTING_BUILD

   ssd_widget_hide      ( container);
   ssd_dialog_set_focus ( previous_focus);
   ssd_dialog_redraw_screen();

   if( s_dialog == previous_focus->parent)
      s_ctx.open_popup = NULL;
   else
      s_ctx.open_popup = previous_focus->parent;

   return TRUE;
}

static BOOL ListItem_OnKeyPressed( SsdWidget this, const char* utf8char, uint32_t flags)
{
   BOOL                 scrolled = FALSE;
   ssd_contextmenu_ptr  menu;
   ssd_cm_item_ptr      item;

   //   Valid input?
   if( !this || !this->data)
      return FALSE;

   menu = this->data;
   item = menu->item + menu->item_selected;

   //   Our task?
   if( !(flags & KEYBOARD_VIRTUAL_KEY))
   {
      assert(utf8char);

      //   Is this the 'Activate' ('enter' / 'select')
      if( KEY_IS_ENTER)
      {
         if( CONTEXT_MENU_FLAG_POPUP & item->flags)
            open_popup_menu( this->parent, item);
         else
            exit_context_menu( TRUE, (menu->item + menu->item_selected));

         return TRUE;
      }

      if( KEY_IS_ESCAPE)
      {
         exit_context_menu( FALSE /* made_selection */, NULL /* item */);
         return TRUE;
      }

      return FALSE;
   }

   switch(*utf8char)
   {
      case VK_Arrow_up:
         scrolled = scroll_up( menu);
         break;

      case VK_Arrow_down:
         scrolled = scroll_down( menu);
         break;

      case VK_Arrow_right:
         if( s_open_to_the_right)
            open_popup_menu( this->parent, item);
         else
            close_popup_menu( this->parent);
         return TRUE;

      case VK_Arrow_left:
         if( s_open_to_the_right)
            close_popup_menu( this->parent);
         else
            open_popup_menu( this->parent, item);
         return TRUE;
   }

   if( !scrolled)
      return FALSE;

   ssd_dialog_set_focus( menu->item[ menu->item_selected].row);
   return TRUE;
}

static int on_row_selected( SsdWidget this, const char *new_value)
{
   SsdWidget            row   = ssd_widget_get( this, "label");
   ssd_contextmenu_ptr  menu  = this->data;
   ssd_cm_item_ptr      item;
   int                  i;

   menu->item_selected    = -1;

   for( i=0; i<menu->item_count; i++)
   {
      if( !strcmp( menu->item[i].label, row->value))
      {
         menu->item_selected = i;
         break;
      }
   }

   if( -1 == menu->item_selected)
   {
      assert(0);
      return 0;
   }

   item = (menu->item + menu->item_selected);

   if( CONTEXT_MENU_FLAG_POPUP & item->flags)
   {
      if( !s_ctx.open_popup || (s_ctx.open_popup == item->row->parent))
         open_popup_menu( item->row->parent, item);
      else
      {
         if( s_ctx.open_popup == item->popup->container)
            close_popup_menu( item->popup->container);
      }
   }
   else
      exit_context_menu( TRUE, item);

   return 1;
}

static void alloc_rows( SsdWidget            menu_cnt,
                        ssd_contextmenu_ptr  menu,
                        unsigned short       flags)
{
   int i;
   int style;
   assert( menu->item_count >= CONTEXT_MENU_MIN_ITEMS_COUNT);
   assert( menu->item_count <= CONTEXT_MENU_MAX_ITEMS_COUNT);

#ifdef TOUCH_SCREEN
   style = SSD_ROUNDED_BLACK;
#else
   style = SSD_ROUNDED_WHITE;
#endif
   menu->item_selected = 0;

   // First - add all items:
   for( i=0; i<menu->item_count; i++)
   {
      SsdWidget         bitmap= NULL;
      SsdWidget         label = NULL;
      SsdWidget         row   = NULL;
      ssd_cm_item_ptr   item  = menu->item + i;
      int               bmp_flags = SSD_END_ROW|SSD_ALIGN_RIGHT;

      if( NULL != item->row)
         return;

      item->label = roadmap_lang_get( item->label);

      row  = ssd_container_new( "rowx",
                                 NULL,
                                 SSD_MIN_SIZE,
                                 SSD_MIN_SIZE,
                                 SSD_WS_TABSTOP|SSD_END_ROW);
      ssd_widget_set_pointer_force_click( row );

      if( SSD_CONTEXTMENU_SIMPLE_LIST & flags)
      {
         label= ssd_text_new( "label",
                              "",
                              CONTEXT_MENU_SIMPLE_LIST_FONT_SIZE,
                              0);

         ssd_widget_add( row, label);

         ssd_widget_set_color( row, "#000000", "#efefef");
      }
      else
      {
#ifdef TOUCH_SCREEN
        if( item->icon)
        {
            SsdWidget     button;
            const char*   small_row_bitmap[2];
            SsdWidget     image_container;

            image_container = ssd_container_new("image_conatiner",
                                                NULL,
                                                40,
                                                SSD_MIN_SIZE,
                                                0);
            ssd_widget_set_color( image_container, "#000000", NULL);

            small_row_bitmap[0] = item->icon;
            small_row_bitmap[1] = NULL;
            button = ssd_button_new("row_bitmap","", &small_row_bitmap[0], 1,SSD_ALIGN_VCENTER,NULL);
            ssd_widget_add(image_container,button);
            ssd_widget_add(row,image_container);
        }

         label= ssd_text_new( "label",
                              "",
                              CONTEXT_MENU_FONT_SIZE,
                              SSD_ALIGN_VCENTER|SSD_END_ROW);
         ssd_widget_set_color(label, "#ffffff", "#000000");
         ssd_widget_add( row, label);
         if (i != menu->item_count-1){
            SsdWidget sep;
            sep = ssd_separator_new("sep", SSD_ALIGN_BOTTOM);
            if (!ssd_widget_rtl(NULL))
               ssd_widget_set_offset(sep, -10, 0);
            ssd_widget_add(row, sep);
         }
#else
        if( item->icon)
        {
            SsdWidget     button;
            const char*   small_row_bitmap[2];
            SsdWidget     image_container;

            image_container = ssd_container_new("image_conatiner",
                                                NULL,
                                                30,
                                                SSD_MIN_SIZE,
                                                0);
            ssd_widget_set_color( image_container, "#000000", NULL);

            small_row_bitmap[0] = item->icon;
            small_row_bitmap[1] = NULL;
            button = ssd_button_new("row_bitmap","", &small_row_bitmap[0], 1,SSD_ALIGN_VCENTER,NULL);
            ssd_widget_add(image_container,button);
            ssd_widget_add(row,image_container);
        }

         label= ssd_text_new( "label",
                              "",
                              CONTEXT_MENU_FONT_SIZE,
                              SSD_ALIGN_VCENTER);
         ssd_widget_add( row, label);
#endif

         bmp_flags |= SSD_ALIGN_VCENTER;

         ssd_widget_set_color( row, "#000000", "#ff000000");
      }

      ssd_widget_set_callback ( row, on_row_selected);

      if( CONTEXT_MENU_FLAG_POPUP & item->flags)
      {
         const char* image_name = "context_menu_popup_left.png";

         if( s_open_to_the_right)
            image_name = "context_menu_popup_right.png";

         bitmap = ssd_bitmap_new("popup-bitmap", image_name, bmp_flags);
         ssd_widget_add( row, bitmap);
      }

      ssd_widget_add( menu_cnt, row);
      row->key_pressed  = ListItem_OnKeyPressed;
      row->data         = menu;
      item->row         = row;
   }

   // Second - add popups
   for( i=0; i<menu->item_count; i++)
   {
      ssd_cm_item_ptr   item = menu->item + i;

      if( CONTEXT_MENU_FLAG_POPUP & item->flags)
      {
         char        popup_name[112];
         SsdWidget   popup_cnt = NULL;
         int         popup_flg = SSD_START_NEW_ROW|SSD_CONTAINER_BORDER;

         assert( NULL == item->popup->container);

         if( !(SSD_CONTEXTMENU_SIMPLE_LIST & flags))
            popup_flg |= (SSD_ROUNDED_CORNERS|SSD_POINTER_NONE|style);

         sprintf(popup_name, "%s%d", SSD_CM_POPUP_CONTAINER_NAME_PREFIX, i);
         popup_cnt = ssd_container_new(popup_name,
                                             NULL,
                                             SSD_MIN_SIZE,
                                             SSD_MIN_SIZE,
                                             popup_flg);

         popup_cnt->context = item->row;

         if( SSD_CONTEXTMENU_SIMPLE_LIST & flags)
            ssd_widget_set_color( popup_cnt, "#000000", "#000000");
         else
            ssd_widget_set_color( popup_cnt, "#000000", "#ff0000000");

         ssd_widget_hide( popup_cnt);
         alloc_rows     ( popup_cnt, item->popup, flags); // Recursive call
         ssd_widget_add ( menu_cnt, popup_cnt);

         item->popup->container = popup_cnt;
      }
   }
}

static void initialize_rows(  SsdWidget            menu_cnt,
                              ssd_contextmenu_ptr  menu,
                              unsigned short       flags)
{
   int         i;
   int         used_rows_count= 0;
   const char* longest_string = NULL;
   int         text_width;
   int         text_ascent;
   int         text_descent;
   int         container_width;
   int         container_height;
   int         font_size = CONTEXT_MENU_FONT_SIZE;

   if( SSD_CONTEXTMENU_SIMPLE_LIST & flags)
      font_size = CONTEXT_MENU_SIMPLE_LIST_FONT_SIZE;


   // Get text height
   if( !s_text_height)
   {
      roadmap_canvas_get_text_extents( "aAbB19Xx", font_size, &text_width, &text_ascent, &text_descent, NULL);
      s_text_height = 5 + (text_ascent + text_descent);
   }

   if( -1 == s_requested_size)
   {
      // Find longest string:
      for( i=0; i<menu->item_count; i++)
      {
         ssd_cm_item_ptr   item  = menu->item + i;
         const char*       label = item->label;

         if( label && (!longest_string || (utf8_strlen(longest_string) < utf8_strlen(label))))
            longest_string = label;
      }

      // Calc space needed for longest string:
      roadmap_canvas_get_text_extents( longest_string, font_size, &text_width, &text_ascent, &text_descent, NULL);
      text_width = (int)((double)text_width * 1.2F) + 20;
   }
   else
      text_width = s_requested_size;

   // Default selected item:
   menu->item_selected = 0;

   // Setup rows:
   // a. Set each row size(width,height)
   // b. Set row label
   // c. Hide un-used rows
   for( i=0; i<menu->item_count; i++)
   {
      ssd_cm_item_ptr   item = menu->item + i;

      if( !(CONTEXT_MENU_FLAG_HIDDEN & menu->item[i].flags))
      {
#ifdef TOUCH_SCREEN
         if( SSD_CONTEXTMENU_SIMPLE_LIST & flags)
            ssd_widget_set_size  ( item->row, text_width, s_text_height);
         else
         {
            if ( roadmap_screen_is_hd_screen() )
            {
               ssd_widget_set_size  ( item->row, text_width+40, 60);
            }
            else
            {
               ssd_widget_set_size  ( item->row, text_width+10, 40);
            }
         }

#else
         ssd_widget_set_size  ( item->row, text_width , s_text_height);
#endif
         ssd_widget_set_value ( item->row, "label", item->label);
         ssd_widget_show      ( item->row);
         used_rows_count++;
      }
      else
      {
         ssd_widget_set_size  ( item->row, 0, 0);
         ssd_widget_set_value ( item->row, "label", "");
         ssd_widget_hide      ( item->row);
      }

      item->row->background_focus = FALSE;
   }

   // Setup frame size:
   // a. Make the frame bigger then all rows together:

   if( SSD_CONTEXTMENU_SIMPLE_LIST & flags)
   {
      container_width   = 4 + text_width;
      container_height  = 4 + (used_rows_count * s_text_height);
   }
   else
   {
#ifdef TOUCH_SCREEN
      container_width   = 220;
      if ( roadmap_screen_is_hd_screen() )
      {
         container_width   = 320;
         container_height  = (used_rows_count) * 62;
      }
      else
      {
         container_height  = (used_rows_count) * 42;
      }
#else
      container_width   = 8 + text_width;
      if (menu_cnt->flags & SSD_POINTER_MENU)
         container_height  =  30 + (used_rows_count * s_text_height);
      else
         container_height  =  10 + (used_rows_count * s_text_height);
#endif
   }

   // b. Set size
   ssd_widget_set_size( menu_cnt, container_width, container_height);

   // Recursion:  Do the same for all nested popup menus:
   for( i=0; i<menu->item_count; i++)
      if( (CONTEXT_MENU_FLAG_POPUP  & menu->item[i].flags) &&
         !(CONTEXT_MENU_FLAG_HIDDEN   & menu->item[i].flags))
         initialize_rows( menu->item[i].popup->container, menu->item[i].popup, flags); // Recursive call
}


static void populate(SsdWidget            menu_cnt,
                     ssd_contextmenu_ptr  menu,
                     unsigned short       flags)
{
   assert(menu_cnt);
   assert(menu);

   if( !verify_items_count( menu))
   {
      roadmap_log(ROADMAP_ERROR, "cm_populate() - Invalid menu count");
      assert(0);
      return;
   }

   if( SSD_CONTEXTMENU_SIMPLE_LIST & flags)
      ssd_widget_set_color( menu_cnt, "#000000", "#000000");
   else
      ssd_widget_set_color( menu_cnt, "#000000", "#ff0000000");

   alloc_rows       ( menu_cnt, menu, flags);
   initialize_rows  ( menu_cnt, menu, flags);
}

static void set_menu_offsets( int            x,
                              int            y,
                              unsigned short flags,
                              BOOL           valid_input)
{
   static int  s_x;
   int           x_offset;
   static int  s_y;
   int           y_offset;
   static int  s_flags;

   static SsdSize contextmenu_size;

   if( valid_input)
   {
      s_x      = x;
      s_y      = y;
      s_flags  = flags;

      ssd_widget_get_size( s_dialog, &contextmenu_size, NULL);
   }
   else
      flags = s_flags;

   if( SSD_X_SCREEN_LEFT == s_x)
      x_offset = 4;
   else if( SSD_X_SCREEN_RIGHT == s_x)
      x_offset = s_canvas_size.width - contextmenu_size.width - 4;
   else
      x_offset = s_x;

   if( SSD_Y_SCREEN_TOP == s_y)
      y_offset = 12;
   else if( SSD_Y_SCREEN_BOTTOM == s_y)
      y_offset = s_canvas_size.height;
   else
      y_offset = s_y;

   s_dialog->offset_x = x_offset;
   s_dialog->offset_y = y_offset;

   if( !(SSD_CONTEXTMENU_DROP_DOWN & flags))
      s_dialog->offset_y -= contextmenu_size.height;
}

static void on_device_event( device_event event, void* context)
{
   if( device_event_window_orientation_changed == event){
      s_ctx.recalc_pos = TRUE;

      if (!roadmap_screen_refresh())
         roadmap_screen_redraw();

   }
}


// returns the widget that should be focused - If there are open pop-ups,
// it will first search in them for focused items, and if none are open,
// will return the selected item in the main context menu
static void setNewFocus(){
	SsdWidget focusedWidget = ssd_dialog_get_focus();
	if ( !focusedWidget ){
		if ( s_ctx.open_popup )
			close_popup_menu(s_ctx.open_popup);
		if ( s_ctx.menu)
   			set_focus_on_first_item(s_ctx.menu);
	}

}


static void draw(SsdWidget widget, RoadMapGuiRect *rect, int flags)
{
   s_ctx.recalc_pos = TRUE;
   //if ( ssd_dialog_get_focus()!=s_ctx.menu->item[s_ctx.menu->item_selected].row)
   		//ssd_dialog_set_focus(s_ctx.menu->item[s_ctx.menu->item_selected].row);
   setNewFocus();

   if( s_ctx.recalc_pos)
   {

      s_canvas_size.width   = roadmap_canvas_width();
#ifdef TOUCH_SCREEN
      s_canvas_size.height  = roadmap_canvas_height() - roadmap_bar_top_height() -5;
#else
     if (is_screen_wide())
        s_canvas_size.height  = roadmap_canvas_height() - roadmap_bar_bottom_height();
     else
        s_canvas_size.height  = roadmap_canvas_height() - roadmap_bar_top_height() -roadmap_bar_bottom_height();

#endif
     set_menu_offsets( 0, 0, 0, FALSE /* Input is valid? */);
     s_ctx.recalc_pos = FALSE;
   }

   roadmap_pointer_cancel_dragging();
   s_ctx.org_draw( widget, rect, flags);
}

void ssd_context_menu_show(int                  x,
                           int                  y,
                           ssd_contextmenu_ptr  menu,
                           SsdOnContextMenu     on_menu_closed,
                           void*                context,
                           menu_open_direction  dir,
                           unsigned short       flags,
                           BOOL 				close_on_selection)
{
   int style;

#ifdef TOUCH_SCREEN
   style = SSD_ROUNDED_BLACK;
#else
   style = SSD_ROUNDED_WHITE;
#endif

   assert(x);
   assert(y);
   assert(menu);
   assert(on_menu_closed);

   if( cm_context_is_active( &s_ctx))
   {
      exit_context_menu(FALSE,NULL); // we only support one context menu open at a time.
   }

   sprintf( s_dialog_name, "%s_0x%08X", SSD_CMDLG_DIALOG_NAME, (unsigned int) menu );

   /*
    * Check the existence of the dialog. If the dialog doesn't exist (deallocated)
    * create the new one
    */
   if ( ssd_dialog_exists( s_dialog_name ) )
   {
	   s_dialog = menu->container;
   }
   else
   {
	   s_dialog = NULL;
   }

   switch( dir)
   {
      case dir_right:
         s_open_to_the_right = TRUE;
         break;
      case dir_left:
         s_open_to_the_right = FALSE;
         break;
      default:
         s_open_to_the_right = !roadmap_lang_rtl();
   }

   if( !s_canvas_size.height || !s_canvas_size.width)
   {
       s_canvas_size.width   = roadmap_canvas_width();
#ifdef TOUCH_SCREEN
       s_canvas_size.height  = roadmap_canvas_height() ;
#else
       if (is_screen_wide())
          s_canvas_size.height  = roadmap_canvas_height() - roadmap_bar_bottom_height();
       else
          s_canvas_size.height  = roadmap_canvas_height() - roadmap_bar_top_height() -roadmap_bar_bottom_height();
#endif
   }

   s_ctx.menu     = menu;
   s_ctx.callback = on_menu_closed;
   s_ctx.context  = context;
   s_ctx.close_on_selection  = close_on_selection;
   if( !s_dialog)
   {
      int   popup_flg = SSD_DIALOG_FLOAT|SSD_CONTAINER_BORDER|SSD_DIALOG_NO_SCROLL|SSD_PERSISTENT;

      if( !(SSD_CONTEXTMENU_SIMPLE_LIST & flags))
         popup_flg |= (SSD_ROUNDED_CORNERS|style|SSD_POINTER_MENU);

      s_dialog = ssd_dialog_new( s_dialog_name,
                                 NULL,
                                 on_dialog_closed,
                                 popup_flg);

      s_ctx.org_draw = s_dialog->draw;
      s_dialog->draw = draw;

      menu->container= s_dialog;
   }

   if( !(SSD_CONTEXTMENU_USE_SIZE & flags))
      s_requested_size = -1;

   populate( s_dialog, menu, flags);

   ssd_widget_set_right_softkey_callback( s_dialog, on_softkey_right);
   ssd_widget_set_left_softkey_callback ( s_dialog, on_softkey_left);

   set_menu_offsets( x, y, flags, TRUE /* Input is valid! */);

   ssd_dialog_activate( s_dialog_name, NULL);
   ssd_dialog_invalidate_tab_order ();
   set_focus_on_first_item( menu);

   if( !s_registered)
   {
      roadmap_device_events_register( on_device_event, NULL);
      s_registered = TRUE;
   }
   close_all_popup_menus(menu); // when we show a menu, we never want to start showing it with popups - D.F.
   ssd_dialog_draw ();
}
