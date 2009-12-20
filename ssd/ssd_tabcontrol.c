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
#include "roadmap_res.h"
#include "roadmap_canvas.h"
#include "ssd_container.h"
#include "ssd_text.h"
#include "roadmap_keyboard.h"
#include "roadmap_lang.h"
#include "roadmap_utf8.h"
#include "roadmap_device_events.h"

#include "ssd_icon.h"
#include "ssd_bitmap.h"
#include "ssd_tabcontrol.h"
#include "ssd_tabcontrol_defs.h"

static int  s_orientation_change_id = 0;
static BOOL RTL                     = FALSE;
static BOOL s_registered            = FALSE;

// Images:

static ssd_icon_wimage  back_tab = {
   "tab_not_selected_left.png",
   "tab_not_selected_middle.png",
   "tab_not_selected_right.png"
};

static ssd_icon_wimage  front_tab = {
   "tab_selected_left.png",
   "tab_selected_middle.png",
   "tab_selected_right.png"
};

static ssd_icon_image_set  left_arrow_images[] = 
{
   {"tab_left.png", NULL},
   {"tab_left_disabled.png", NULL}
};

static ssd_icon_image_set  right_arrow_images[] = 
{
   {"tab_right.png", NULL},
   {"tab_right_disabled.png", NULL}
};

// Image set:  Array
static ssd_icon_wimage_set tab_images[] = 
{
   { &back_tab,  NULL},
   { &front_tab, NULL}
};

void tabcontrol_info_init( tabcontrol_info_ptr this)
{
   memset( this, 0, sizeof(tabcontrol_info));
   this->active_tab           = INVALID_TAB_INDEX;
   this->tli.left_tab_selected= TRUE;
}

void ssd_tabcontrol_free( SsdTcCtx tabcontrol)
{
   tabcontrol_info_ptr  tc = tabcontrol;
   tabcontrol_info_init(tc);
   free( tc);
}

void ssd_tabcontrol_set_title( SsdTcCtx tcx, const char* title)
{
   tabcontrol_info_ptr tci = tcx;
   tci->dialog->set_value( tci->dialog, title);
}

SsdWidget ssd_tabcontrol_get_tab( SsdTcCtx tcx, int tab)
{
   tabcontrol_info_ptr tci = tcx;

   if( (tab<0) || (tci->tabs_count<=tab))
   {
      roadmap_log(ROADMAP_ERROR, 
                  "ssd_tabcontrol_get_tab() - Invalid tab index (%d), when tab-count is %d",
                  tab, tci->tabs_count);
      return NULL;
   }
   
   return tci->tabs[tab].tab;
}

SsdWidget ssd_tabcontrol_get_active_tab( SsdTcCtx tcx)
{ 
   tabcontrol_info_ptr tci = tcx;
   assert( INVALID_TAB_INDEX != tci->active_tab);
   return ssd_tabcontrol_get_tab( tcx, tci->active_tab);
}

static SsdWidget tabline_get_element( tabsline_info_ptr tli, const char* name)
{  return ssd_widget_get( tli->tabs_container, name);}

static SsdWidget tabline_get_left_tab(tabsline_info_ptr tli)
{  return tabline_get_element( tli, TABS_LINE__LEFT_TAB);}

static SsdWidget tabline_get_left_tab_text(tabsline_info_ptr tli)
{  return tabline_get_element( tli, TABS_LINE__LEFT_TAB_TEXT);}

static SsdWidget tabline_get_left_arrow(tabsline_info_ptr tli)
{  return tabline_get_element( tli, TABS_LINE__LEFT_ARROW);}

static SsdWidget tabline_get_right_tab(tabsline_info_ptr tli)
{  return tabline_get_element( tli, TABS_LINE__RIGHT_TAB);}

static SsdWidget tabline_get_right_tab_text(tabsline_info_ptr tli)
{  return tabline_get_element( tli, TABS_LINE__RIGHT_TAB_TEXT);}

static SsdWidget tabline_get_right_arrow(tabsline_info_ptr tli)
{  return tabline_get_element( tli, TABS_LINE__RIGHT_ARROW);}

static void tabsline_enable_arrow( tabcontrol_info_ptr tci, tabsline_side side, BOOL enable)
{
   SsdWidget arrow;
   
   if( left == side)
      arrow = tabline_get_left_arrow( &(tci->tli));
   else
      arrow = tabline_get_right_arrow( &(tci->tli));
   
   if( enable)
      ssd_icon_set_state( arrow, ARROW_ENABLED);
   else
      ssd_icon_set_state( arrow, ARROW_DISABLED);
}

static void tabsline_set_text( tabcontrol_info_ptr tci, tabsline_side side, const char* text)
{
   SsdWidget textbox;
   
   if( left == side)
      textbox = tabline_get_left_tab_text( &(tci->tli));
   else
      textbox = tabline_get_right_tab_text( &(tci->tli));
   
   ssd_text_set_text( textbox, text);
}

static void tabsline_select_tab( tabcontrol_info_ptr tci, tabsline_side side, BOOL select)
{
   SsdWidget   tab;
   SsdWidget   text;
   
   if( left == side)
   {
      tab   = tabline_get_left_tab( &(tci->tli));
      text  = tabline_get_left_tab_text( &(tci->tli));
   }
   else
   {
      tab   = tabline_get_right_tab( &(tci->tli));
      text  = tabline_get_right_tab_text( &(tci->tli));
   }

   if( select)
   {
      ssd_icon_set_state( tab, TAB_SELECTED);
      
      if( text)
         ssd_text_set_color( text,SELECTED_TEXT_COLOR);
   }
   else
   {
      ssd_icon_set_state( tab, TAB_NOT_SELECTED);
      
      if( text)
         ssd_text_set_color( text,UNSELECTED_TEXT_COLOR);
   }
}

static void initialize_tabsline( tabcontrol_info_ptr tci)
{
   BOOL have_more_then_one_tabs = (1 < tci->tabs_count);

   tabsline_select_tab( tci, left,  TRUE);
   tabsline_select_tab( tci, right, FALSE);
   
   tabsline_set_text    ( tci, left, tci->tabs[0].title);
   tabsline_enable_arrow( tci, left, FALSE);

   if( have_more_then_one_tabs)
      tabsline_set_text( tci, right, tci->tabs[1].title);

   tabsline_enable_arrow(tci, right, have_more_then_one_tabs);
   
   tci->tli.left_tab_selected = TRUE;
}

static void update_tabsline( tabcontrol_info_ptr tci, int last_tab, int new_tab)
{
   BOOL moving_up = (last_tab < new_tab);
   
   if( 0 == new_tab)
   {
      initialize_tabsline( tci);
      return;
   }
   
   if( last_tab == new_tab)
      return;
   
   if( tci->tli.left_tab_selected)
   {
      if( moving_up)
      {
         tabsline_select_tab( tci, right,TRUE);
         tabsline_select_tab( tci, left, FALSE);

         tci->tli.left_tab_selected = FALSE;
      }
      else
      {
         // Change text:
         const char* last_text = tci->tabs[last_tab].title;
         const char* new_text  = tci->tabs[new_tab].title;
         
         tabsline_set_text( tci, left, new_text);
         tabsline_set_text( tci, right,last_text);
      }
   }
   else
   {
      if( !moving_up)
      {
         tabsline_select_tab( tci, left, TRUE);
         tabsline_select_tab( tci, right,FALSE);

         tci->tli.left_tab_selected = TRUE;
      }
      else
      {
         // Change text:
         // Change text:
         const char* last_text      = tci->tabs[last_tab].title;
         const char* new_text       = tci->tabs[new_tab].title;
         
         tabsline_set_text( tci, left, last_text);
         tabsline_set_text( tci, right,new_text);
      }
   }

   // Do we need to disable arrow buttons?
   tabsline_enable_arrow(tci, left, (0 != new_tab));
   tabsline_enable_arrow(tci, right,((tci->tabs_count-1) != new_tab));
}

void ssd_tabcontrol_set_active_tab( SsdTcCtx tcx, int tab)
{
   tabcontrol_info_ptr  tci         = tcx;
   SsdWidget            cur_tab     = NULL;
   SsdWidget            new_tab     = NULL;
   int                  cur_tab_id  = tci->active_tab;
   int                  new_tab_id  = tab;
   
   if(!tcx           || (INVALID_TAB_INDEX == tci->active_tab) || 
      !tci->dialog   || !tci->container)
   {
      roadmap_log( ROADMAP_ERROR, "ssd_tabcontrol_set_active_tab() - NO DIALOG IS ACTIVE");
      return;
   }

   if( (tab<0) || (tci->tabs_count<=tab))
   {
      roadmap_log(ROADMAP_ERROR, 
                  "ssd_tabcontrol_set_active_tab() - Trying to set invalid tab (%d), when tab-count is %d",
                  tab, tci->tabs_count);
      return;
   }
   
   if( tab != tci->active_tab)
   {
      if( tci->on_tab_loose_focus && !tci->on_tab_loose_focus( tci->active_tab))
      {
         roadmap_log( ROADMAP_DEBUG, "ssd_tabcontrol_set_active_tab() - Was asked not to loose focus on tab %d", tci->active_tab);
         return;
      }
      
      cur_tab = ssd_tabcontrol_get_active_tab(tcx);
      assert(cur_tab);
      
      tci->active_tab=  tab;
      new_tab        = ssd_tabcontrol_get_active_tab(tcx);
      assert(new_tab);

      ssd_widget_replace( tci->container, cur_tab, new_tab);
      if(tci->on_tab_gain_focus)
         tci->on_tab_gain_focus( tci->active_tab);
   }
   
   ssd_widget_set_offset(tci->dialog,0,0);
   ssd_widget_reset_position(tci->dialog);
   ssd_dialog_resort_tab_order ();
   update_tabsline( tci, cur_tab_id, new_tab_id);
}

static void move_one_tab_down( SsdTcCtx tcx)
{
   tabcontrol_info_ptr  tci      = tcx;
   int                  new_tab  = tci->active_tab - 1;     
   
   if( new_tab < 0)
#ifdef   ENABLE_LOOPING
      new_tab = (tci->tabs_count - 1);
#else
      return;
#endif   // ENABLE_LOOPING
   
   ssd_tabcontrol_set_active_tab( tcx, new_tab);
}

static void move_one_tab_up( SsdTcCtx tcx)
{
   tabcontrol_info_ptr  tci      = tcx;
   int                  new_tab  = tci->active_tab + 1;
   
   if( tci->tabs_count <= new_tab)
#ifdef   ENABLE_LOOPING
      new_tab = 0;
#else
      return;
#endif   // ENABLE_LOOPING
   
   ssd_tabcontrol_set_active_tab( tcx, new_tab);
}

static void move_one_tab_left( SsdTcCtx tcx)
{
   if( RTL)
      move_one_tab_up( tcx);
   else
      move_one_tab_down( tcx);
}

static void move_one_tab_right( SsdTcCtx tcx)
{
   if( RTL)
      move_one_tab_down( tcx);
   else
      move_one_tab_up( tcx);
}

void on_dialog_closed( int exit_code, void* context)
{
   tabcontrol_info_ptr  tci               = context;
   PFN_ON_DIALOG_CLOSED on_tbctrl_closed  = tci->on_tbctrl_closed;

   if(on_tbctrl_closed)
      on_tbctrl_closed( exit_code, context);
}

void ssd_tabcontrol_move_tab_left( SsdWidget dialog)
{
   SsdWidget cnt;
   tabcontrol_info_ptr  tci;
   ssd_widget_set_offset(dialog,0,0);
   cnt = ssd_widget_get( dialog, CONTAINER_NAME);
   tci = cnt->context;
   move_one_tab_left(   tci);
}

void ssd_tabcontrol_move_tab_right( SsdWidget dialog)
{
   SsdWidget cnt;
   tabcontrol_info_ptr  tci;
   ssd_widget_set_offset(dialog,0,0);
   cnt = ssd_widget_get( dialog, CONTAINER_NAME);
   tci = cnt->context;
   move_one_tab_right(  tci);
}

static BOOL OnKeyPressed( SsdWidget this, const char* utf8char, uint32_t flags)
{
   tabcontrol_info_ptr  tci = this->context;
   
   if( KEYBOARD_VIRTUAL_KEY & flags)
   {
      switch( *utf8char)
      {
         case VK_Arrow_left:
            move_one_tab_left(tci);
            return TRUE;
      
         case VK_Arrow_right:
            move_one_tab_right(tci);
            return TRUE;
         
         case VK_Arrow_up:
            ssd_dialog_move_focus( FOCUS_UP);
            return TRUE;

         case VK_Arrow_down:
            ssd_dialog_move_focus( FOCUS_DOWN);
            return TRUE;
            
         default:
            break;
      }
   }
      
   if( !tci->on_unhandled_key_pressed)
      return FALSE;
      
   return tci->on_unhandled_key_pressed( 
            tci->active_tab, utf8char, flags);
}

static BOOL OnTabLineKeyPressed( SsdWidget this, const char* utf8char, uint32_t flags)
{ return OnKeyPressed( this->parent->parent, utf8char, flags);}

SsdWidget ssd_tabcontrol( SsdTcCtx tabcontrol)
{
   tabcontrol_info_ptr  tc = tabcontrol;
   if( !tc)
      return NULL;
   
   return tc->dialog;
}

static void on_device_event( device_event event, void* context)
{
   if( device_event_window_orientation_changed == event)
      s_orientation_change_id++;
}

void ssd_tabcontrol_show( SsdTcCtx tcx)
{
   tabcontrol_info_ptr tci = tcx;

   ssd_widget_set_offset(tci->dialog,0,0);
   ssd_tabcontrol_set_active_tab ( tcx, tci->active_tab);
   ssd_dialog_activate( tci->dialog_name, tcx);
   ssd_dialog_draw();
}

SsdWidget ssd_tabcontrol_get_dialog( SsdTcCtx tcx)
{
   tabcontrol_info_ptr tci = tcx;
   return tci->dialog;
}

static SsdSize* get_arrow_size()
{
   static SsdSize s_size = {0,0};
   
   if( !s_size.width && !s_size.height)
   {
      char*          bitmap= RTL? "tab_right": "tab_left";
      RoadMapImage   arrow = roadmap_res_get( RES_BITMAP, RES_SKIN, bitmap);
      
      s_size.width   = roadmap_canvas_image_width ( arrow);
      s_size.height  = roadmap_canvas_image_height( arrow);
   }
   
   return &s_size;
}

static void calc_tabs_width( SsdWidget main_cont, tabsline_info_ptr tli, int* tabA, int* tabB)
{
   static int  s_width_taken = 0;
   static int  s_height;
   
   int   size_for_the_two;
   
   if( !s_width_taken)
   {
      SsdSize* arrow_size = get_arrow_size();
      
      s_height       = arrow_size->height;
      s_width_taken  = 2 * arrow_size->width;
   }

   tli->tabs_height  = s_height;   
   size_for_the_two  = (main_cont->cached_size.width - s_width_taken);
   (*tabA)           = size_for_the_two/2;
   (*tabB)           = size_for_the_two - (*tabA);
}

static void tabs_draw( SsdWidget this, RoadMapGuiRect *rect, int flags)
{
   tabcontrol_info_ptr  tci = this->parent->context;
   SsdWidget            left_tab;
   SsdWidget            left_text;
   SsdWidget            right_tab;
   SsdWidget            right_text;
   int                  left_width;
   int                  right_width;
   
   if( SSD_GET_SIZE & flags)
      return;
   
   if( tci->orientation_change_id != s_orientation_change_id)
   {
      tci->orientation_change_id    = s_orientation_change_id;
      tci->tli.drawing_initialized  = FALSE;
   }

   if( tci->tli.drawing_initialized)
      return;

   tci->tli.drawing_initialized = TRUE;

   calc_tabs_width( this, &(tci->tli), &left_width, &right_width);
   
   left_tab = ssd_widget_get  ( this, TABS_LINE__LEFT_TAB);
   right_tab= ssd_widget_get  ( this, TABS_LINE__RIGHT_TAB);

   // Set dynamic width:
   ssd_icon_set_width   ( left_tab , left_width);
   ssd_widget_set_size  ( left_tab , left_width, tci->tli.tabs_height);
   ssd_icon_set_width   ( right_tab, right_width);
   ssd_widget_set_size  ( right_tab, right_width,tci->tli.tabs_height);
   
   left_text = ssd_widget_get( left_tab, TABS_LINE__LEFT_TAB_TEXT);
   if( NULL == left_text)
   {
      // Create editbox from here (ssd_widget drawing issue)
      left_text = ssd_text_new( TABS_LINE__LEFT_TAB_TEXT, "", 5, SSD_TEXT_READONLY);
      right_text= ssd_text_new( TABS_LINE__RIGHT_TAB_TEXT,"", 5, SSD_TEXT_READONLY);
      
      // Default settings for text-boxes:
      ssd_text_set_auto_size  ( left_text);
      ssd_text_set_auto_size  ( right_text);
      ssd_text_set_auto_trim  ( left_text);
      ssd_text_set_auto_trim  ( right_text);
      ssd_text_set_color      ( left_text, UNSELECTED_TEXT_COLOR);
      ssd_text_set_color      ( right_text,UNSELECTED_TEXT_COLOR);
      
      ssd_widget_add( left_tab,  left_text);
      ssd_widget_add( right_tab, right_text);

      ssd_text_set_text( left_text, tci->tabs[0].title);
      if( 1 < tci->tabs_count)
         ssd_text_set_text( right_text, tci->tabs[1].title);
      
      initialize_tabsline( tci);
   }
}

static int on_left_arrow( SsdWidget widget, const char* value)
{
   tabcontrol_info_ptr  tci = widget->parent->parent->context;
   
   if( 0 != tci->active_tab)
      move_one_tab_down( tci);

   return 0;
}
static int on_left_arrow_p( SsdWidget widget, const RoadMapGuiPoint *point)
{ return on_left_arrow( widget, NULL);}

static int on_tab( SsdWidget widget, const char* value)
{
   tabcontrol_info_ptr tci = widget->parent->parent->context;

   if( 0 == widget->value)
   {
      // Left tab
      
      if( !tci->tli.left_tab_selected)
         move_one_tab_down( tci);
   }
   else
   {
      // Right tab

      if( tci->tli.left_tab_selected)
         move_one_tab_up( tci);
   }

   return 0;
}
static int on_tab_p( SsdWidget widget, const RoadMapGuiPoint *point)
{ return on_tab( widget, NULL);}

static int on_right_arrow( SsdWidget widget, const char* value)
{
   tabcontrol_info_ptr  tci = widget->parent->parent->context;
   
   if( (tci->tabs_count-1) != tci->active_tab)
      move_one_tab_up( tci);

   return 0;
}
static int on_right_arrow_p( SsdWidget widget, const RoadMapGuiPoint *point)
{ return on_right_arrow( widget, NULL);}

static void create_tabs( tabcontrol_info_ptr tci)
{
   SsdWidget            left;
   SsdWidget            left_tab;
   SsdWidget            right_tab;
   SsdWidget            right;

   ssd_icon_image_set*  left_arrow;
   ssd_icon_image_set*  right_arrow;
   int                  arrow_images_count = LEFT_ARROW_IMAGES_COUNT;
   
   assert( NULL == tci->tli.tabs_container);

   RTL         = roadmap_lang_rtl();
   left_arrow  = RTL? right_arrow_images: left_arrow_images;
   right_arrow = RTL? left_arrow_images: right_arrow_images;      
   
   assert(LEFT_ARROW_IMAGES_COUNT==RIGHT_ARROW_IMAGES_COUNT);
   
   tci->tli.tabs_container = ssd_container_new(TABS_LINE__CONTAINER, "", SSD_MAX_SIZE,SSD_MIN_SIZE, SSD_END_ROW|SSD_CONTAINER_BORDER);
   ssd_widget_set_color(tci->tli.tabs_container, NULL, NULL);
   tci->tli.tabs_container->draw= tabs_draw;

   left     = ssd_icon_create(TABS_LINE__LEFT_ARROW,  FALSE,0);
   left_tab = ssd_icon_create(TABS_LINE__LEFT_TAB,    TRUE, 0);
   right_tab= ssd_icon_create(TABS_LINE__RIGHT_TAB,   TRUE, 0);
   right    = ssd_icon_create(TABS_LINE__RIGHT_ARROW, FALSE,0);
   
   ssd_icon_set_images ( left,      left_arrow,   arrow_images_count);
   ssd_icon_set_wimages( left_tab,  ICON_IMAGES,  ICON_IMAGES_COUNT);
   ssd_icon_set_wimages( right_tab, ICON_IMAGES,  ICON_IMAGES_COUNT);
   ssd_icon_set_images ( right,     right_arrow,  arrow_images_count);
   
   left     ->callback     = on_left_arrow;
   left     ->pointer_down = on_left_arrow_p;   
   left_tab ->callback     = on_tab;
   left_tab ->pointer_down = on_tab_p;
   left_tab ->value        = 0;  // Left tab - index 0
   right_tab->callback     = on_tab;
   right_tab->pointer_down = on_tab_p;
   right_tab->value        = (void*)1;  // Right tab- index 1
   right    ->callback     = on_right_arrow;
   right    ->pointer_down = on_right_arrow_p;
   
   ssd_icon_set_unhandled_key_press( left     , OnTabLineKeyPressed);
   ssd_icon_set_unhandled_key_press( left_tab , OnTabLineKeyPressed);
   ssd_icon_set_unhandled_key_press( right_tab, OnTabLineKeyPressed);
   ssd_icon_set_unhandled_key_press( right    , OnTabLineKeyPressed);
   
   ssd_widget_add( tci->tli.tabs_container, left);
   ssd_widget_add( tci->tli.tabs_container, left_tab);
   ssd_widget_add( tci->tli.tabs_container, right_tab);
   ssd_widget_add( tci->tli.tabs_container, right);

   ssd_widget_add( tci->container, tci->tli.tabs_container);
}

SsdTcCtx ssd_tabcontrol_new(  const char*             name,
                              const char*             title,
                              PFN_ON_DIALOG_CLOSED    on_tbctrl_closed,
                              PFN_ON_TAB_LOOSE_FOCUS  on_tab_loose_focus,
                              PFN_ON_TAB_GAIN_FOCUS   on_tab_gain_focus,
                              PFN_ON_KEY_PRESSED      on_unhandled_key_pressed,
                              const char*             tabs_titles[],
                              SsdWidget               tabs[],
                              int                     tabs_count,
                              int                     active_tab)
{
   tabcontrol_info_ptr  tc_ptr = NULL;
   tabcontrol_info      tc;
   int                  i;
   
   assert(title);
   assert(tabs_titles);
   assert(tabs);
   assert(tabs_count);
   
   tabcontrol_info_init( &tc);
   
   if( (tabs_count<2) || (MAX_TAB_COUNT<tabs_count))
   {
      roadmap_log( ROADMAP_ERROR, "ssd_tabcontrol_new(%s) - Invalid tab count (%d)", title, tabs_count);
      return NULL;
   }
      
   if( (active_tab<0) || (tabs_count<=active_tab))
   {
      roadmap_log(ROADMAP_ERROR, 
                  "ssd_tabcontrol_new(%s) - Invalid 'active_tab' (%d) when tab count is %d", 
                  title, active_tab, tabs_count);
      return NULL;
   }
   
   for( i=0; i<tabs_count; i++)
   {
      const char* tab_title= tabs_titles[i];
      SsdWidget   tab      = tabs[i];

      if( !tab_title || !(*tab_title) || (TAB_TITLE_MAX_SIZE < utf8_strlen( tab_title)))
      {
         roadmap_log(ROADMAP_ERROR,
                     "ssd_tabcontrol_new(%s) - Tab[%d].title == '%s' (max size for tab titles is %d)",
                     title, i, tab_title, TAB_TITLE_MAX_SIZE);
                        
         return NULL;
      }
      
      if( !tab)
      {
         roadmap_log(ROADMAP_ERROR,
                     "ssd_tabcontrol_new(%s) - Tab[%] is NULL",
                     title, i);
                        
         return NULL;
      }
      
      tc.tabs[i].title  = tab_title;
      tc.tabs[i].tab    = tab;
      
      assert( NULL == tab->parent);
   }
   
   tc.orientation_change_id   
                        = s_orientation_change_id;
   tc.tabs_count        = tabs_count;
   tc.active_tab        = active_tab;
   tc.on_tbctrl_closed  = on_tbctrl_closed;
   tc.on_tab_loose_focus= on_tab_loose_focus;
   tc.on_tab_gain_focus = on_tab_gain_focus;
   tc.on_unhandled_key_pressed 
                        = on_unhandled_key_pressed;
   tc.dialog_name       = name;
   tc.dialog            = ssd_dialog_new( 
                                    name,
                                    title,
                                    on_dialog_closed,
                                    SSD_TAB_CONTROL   |  SSD_CONTAINER_TITLE  |
                                    SSD_ALIGN_CENTER  |  SSD_ALIGN_VCENTER);
   tc.container         = ssd_container_new(
                                    CONTAINER_NAME,
                                    NULL, 
                                    SSD_MAX_SIZE,
                                    SSD_MAX_SIZE,
                                    SSD_CONTAINER_BORDER);
   ssd_widget_set_color(tc.container, NULL, NULL);

   tc.container->key_pressed  = OnKeyPressed;

   create_tabs    ( &tc);
   ssd_widget_add ( tc.container, ssd_tabcontrol_get_active_tab( &tc));
   ssd_widget_add ( tc.dialog,    tc.container);
   
   for( i=0; i<tabs_count; i++)
      tc.tabs[i].tab->parent = tc.container;

   tc_ptr               = malloc( sizeof(tabcontrol_info));
   (*tc_ptr)            = tc;
   tc.container->context= tc_ptr;
   
   tabcontrol_info_init( &tc);
   
   if( !s_registered)
   {
      roadmap_device_events_register( on_device_event, NULL);
      s_registered = TRUE;
   }

   return tc_ptr;
}

int  ssd_tabcontrol_get_height(){
   SsdSize *size = get_arrow_size();
   return size->height;
}
