/* ssd_menu.c - Icons menu
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
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
 * SYNOPSYS:
 *
 *   See ssd_menu.h.
 */

#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_lang.h"
#include "roadmap_path.h"
#include "roadmap_res.h"
#include "roadmap_screen.h"
#include "roadmap_keyboard.h"
#include "roadmap_factory.h"
#include "roadmap_bar.h"
#include "ssd_dialog.h"
#include "ssd_container.h"
#include "ssd_button.h"
#include "ssd_text.h"
#include "ssd_menu.h"
#include "ssd_separator.h"
#ifdef ANDROID
#include "roadmap_androidmath.h"	// For the div implementation
#endif


#ifdef   USE_CONTEXTMENU_INSTEAD
#endif   // USE_CONTEXTMENU_INSTEAD

typedef struct list_menu_data{
   SsdWidget  MenuWidgets[20];
   int        CurrentIndex;
   int        num_rows;
   SsdWidget  container ;
   int        num_items ;

}  menu_list_data;


static int button_callback (SsdWidget widget, const char *new_value) {

   RoadMapCallback callback = (RoadMapCallback) widget->context;

   (*callback)();

#ifdef TOUCH_SCREEN
   if (widget->parent)
   	ssd_widget_loose_focus(widget->parent);
#endif

   /* Hide dialog */
   while (widget->parent) widget = widget->parent;

   return 0;
}

#ifndef TOUCH_SCREEN
static void move_to_start(){
	  menu_list_data *data;
	  int i;
	  int num_items;

	  data = (menu_list_data *)	ssd_dialog_get_current_data();

	  for (i=0; i<data->num_items;i++)
         ssd_widget_hide(data->MenuWidgets[i]);


     if (data->num_items < data->num_rows)
     	num_items = data->num_items;
     else
     	num_items = data->num_rows;
     for (i=0; i<num_items;i++)
         		ssd_widget_show(data->MenuWidgets[i]);
	  ssd_dialog_resort_tab_order();
	  ssd_dialog_set_focus(ssd_widget_get(data->MenuWidgets[0],"button"));
	  data->CurrentIndex = 0;
}
#endif

static void move_down(){
   menu_list_data *data;
   int i;
   //div_t n;
   int relative_index;

   data = (menu_list_data *)   ssd_dialog_get_current_data();

   //n = div (data->CurrentIndex, data->num_rows);
   relative_index = data->CurrentIndex % data->num_rows;

      if (((data->CurrentIndex == data->num_rows -1) || (relative_index == data->num_rows -1) || (data->CurrentIndex == data->num_items -1)) && (data->num_items > data->num_rows)){
          int index;

         for (i=0; i<data->num_items;i++)
               ssd_widget_hide(data->MenuWidgets[i]);

         if (data->CurrentIndex == data->num_items -1)
            index = 0;
         else
            index = data->CurrentIndex +1;

         for (i=index; i<index+data->num_rows;i++)
            if (i < data->num_items)
               ssd_widget_show(data->MenuWidgets[i]);
      }

      if (data->CurrentIndex < data->num_items-1)
         data->CurrentIndex++;
      else
         data->CurrentIndex = 0;

      ssd_dialog_resort_tab_order();
      ssd_dialog_set_focus(ssd_widget_get(data->MenuWidgets[data->CurrentIndex],"button"));
}

static void move_up(){
      menu_list_data *data;
      int i;
      int n_quot;
   	int relative_index;

      data = (menu_list_data *)   ssd_dialog_get_current_data();
      n_quot = data->CurrentIndex / data->num_rows;
      relative_index = data->CurrentIndex % data->num_rows;

      if (((data->CurrentIndex == 0) || (relative_index == 0) || (data->CurrentIndex == data->num_rows )) && (data->num_items > data->num_rows)){
          int index;

         for (i=0; i<data->num_items;i++)
            ssd_widget_hide(data->MenuWidgets[i]);

         if (relative_index == 0)
         	if (n_quot == 0){
         		//div_t n2;
         		int n2_rem = data->num_items % data->num_rows;
         		int n2_quot = data->num_items / data->num_rows;
         		if (n2_rem == 0)
         			index = (n2_quot -1) * data->num_rows;
         		else
         			index = n2_quot * data->num_rows ;
         	}
         	else
         		index = (n_quot -1) * data->num_rows;
         else
            index =0;

         for (i=index; i<index+data->num_rows;i++)
            if (i < data->num_items)
               ssd_widget_show(data->MenuWidgets[i]);
      }


      if (data->CurrentIndex > 0)
         data->CurrentIndex--;
      else
         data->CurrentIndex = data->num_items -1;

      ssd_dialog_resort_tab_order();
      ssd_dialog_set_focus(ssd_widget_get(data->MenuWidgets[data->CurrentIndex],"button"));

}

#ifdef TOUCH_SCREEN
static void scroll_page_down(){
	  menu_list_data *data;

      data = (menu_list_data *)   ssd_dialog_get_current_data();
      if( (data->CurrentIndex + data->num_rows) >= data->num_items)
      	return;

	  data->CurrentIndex += data->num_rows-1;

	  move_down();

}

static void scroll_page_up(){
	  menu_list_data *data;

      data = (menu_list_data *)   ssd_dialog_get_current_data();
      if( (data->CurrentIndex - data->num_rows) < 0)
      	return;
	  move_up();
	  data->CurrentIndex -= data->num_rows -2;
	  move_up();
}


static int scroll_buttons_callback (SsdWidget widget, const char *new_value) {

   if (!strcmp(widget->name, "scroll_up")) {
      scroll_page_up();
      return 0;
   }

   if (!strcmp( widget->name, "scroll_down")) {
      scroll_page_down();
      return 0;
   }

   return 1;
}
#endif

static BOOL OnKeyPressed (SsdWidget widget, const char* utf8char, uint32_t flags){
   BOOL        key_handled = TRUE;

   if( KEY_IS_ENTER)
   {
      widget->callback(widget, SSD_BUTTON_SHORT_CLICK);
      return TRUE;
   }

    if( KEYBOARD_VIRTUAL_KEY & flags)
   {
      switch( *utf8char) {

         case VK_Arrow_up:
               move_up();
            break;

         case VK_Arrow_down:
            move_down();
            break;

         default:
            key_handled = FALSE;
      }

   }
   else
   {
      assert(utf8char);
      assert(*utf8char);

      // Other special keys:
      if( KEYBOARD_ASCII & flags)
      {
         switch(*utf8char)
         {
            case TAB_KEY:
                move_down();
               break;

            default:
               key_handled = FALSE;
         }
      }
   }

   if( key_handled)
      roadmap_screen_redraw ();

   return key_handled;
}

static void draw_bg( SsdWidget this, RoadMapGuiRect *rect, int flags){

   static RoadMapImage TopBgImage;
   static RoadMapImage BottomBgImage;
   static RoadMapImage MiddleBgImage;
   RoadMapGuiPoint point;
   int i;
   static int top_height, top_width;
   static int bottom_height, bottom_width;

#ifdef TOUCH_SCREEN
   int height = roadmap_canvas_height() ;
#else
   int height = roadmap_canvas_height() -  roadmap_bar_bottom_height();
#endif
   int width = roadmap_canvas_width();

   /*
    * AGA TODO:: Check images caching
    */
   if (!TopBgImage)
      TopBgImage = (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN|RES_NOCACHE, "menu_bg_top");
   if (TopBgImage == NULL){
      return;
   }

   if (!BottomBgImage)
      BottomBgImage = (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN|RES_NOCACHE, "menu_bg_bottom");
   if (BottomBgImage == NULL){
      return;
   }

   if (!MiddleBgImage)
      MiddleBgImage = (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN|RES_NOCACHE, "menu_bg_middle");
   if (MiddleBgImage == NULL){
         return;
   }

   top_height = roadmap_canvas_image_height(TopBgImage);
   bottom_height = roadmap_canvas_image_height(BottomBgImage);


    if ((flags & SSD_GET_SIZE)){
    	rect->miny += top_height + 4;
    	rect->maxy = height - bottom_height ;
    	return;
    }

   top_width  = roadmap_canvas_image_width(TopBgImage);
    bottom_width  = roadmap_canvas_image_width(BottomBgImage);


	if (!(flags & SSD_GET_SIZE)){
		rect->miny += top_height + 4;
#ifdef TOUCH_SCREEN
        rect->maxy = height - bottom_height  ;
#else
		rect->maxy = height - bottom_height -roadmap_bar_bottom_height() ;
#endif
    	        rect->maxx -= 7;

		point.y = 0;
		point.x = width - top_width;

		roadmap_canvas_draw_image (TopBgImage, &point, 0,IMAGE_NORMAL);

		point.y = height - bottom_height+4;
		point.x = width - bottom_width;

		for (i = top_height; i <  point.y; i++){
			RoadMapGuiPoint pointMiddle;
			pointMiddle.y = i;
			pointMiddle.x = width - top_width;
			roadmap_canvas_draw_image (MiddleBgImage, &pointMiddle, 0,IMAGE_NORMAL);
		}

		roadmap_canvas_draw_image (BottomBgImage, &point, 0,IMAGE_NORMAL);
	}
}

static const RoadMapAction *find_action_by_label
                              (const RoadMapAction *actions, const char *item) {

   while (actions->label_long != NULL) {
      if (strcmp (actions->label_long, item) == 0) return actions;
      ++actions;
   }

   return NULL;
}

static const RoadMapAction *find_action
                              (const RoadMapAction *actions, const char *item) {

   const RoadMapAction* first = actions;

   while (actions->name != NULL) {
      if (strcmp (actions->name, item) == 0) return actions;
      ++actions;
   }

   return find_action_by_label( first, item) ;
}

static SsdWidget delayed_widget;

static int on_pointer_down( SsdWidget this, const RoadMapGuiPoint *point)
{
	ssd_widget_pointer_down_force_click(this, point);
   return 0;
}

static void delayed_short_click (void) {
   RoadMapCallback callback = (RoadMapCallback) delayed_widget->context;

   roadmap_main_remove_periodic (delayed_short_click);

   if (callback) {
      (*callback) ();
   }

   delayed_widget->in_focus = FALSE;

   if (delayed_widget->parent->parent->flags & SSD_DIALOG_FLOAT)
      ssd_dialog_hide(delayed_widget->parent->parent->name, dec_close);

#ifdef TOUCH_SCREEN
   if (delayed_widget)
      ssd_widget_loose_focus(delayed_widget);
#endif

   /* Hide dialog */
   while (delayed_widget->parent) delayed_widget = delayed_widget->parent;


   roadmap_screen_redraw ();
}


static int short_click (SsdWidget widget, const RoadMapGuiPoint *point){

   delayed_widget = widget;

   ssd_dialog_set_focus(widget);

   roadmap_screen_redraw ();

   roadmap_main_set_periodic (100, delayed_short_click);

   return 1;
}

static BOOL on_key_pressed(SsdWidget widget, const char* utf8char, uint32_t flags){
   if( KEY_IS_ENTER)
   {
      short_click(widget, NULL);
      return TRUE;
   }

   return FALSE;
}

SsdWidget g_widget;
static void delayed_cancel(void){
   roadmap_main_remove_periodic (delayed_cancel);
   ssd_dialog_hide_current(dec_cancel);
   if (g_widget)
      ssd_widget_loose_focus(g_widget);
   g_widget = NULL;

   roadmap_screen_redraw ();
}

static int on_cancel (SsdWidget widget, const RoadMapGuiPoint *point){
   ssd_dialog_set_focus(widget);

   roadmap_screen_redraw ();
   g_widget = widget;
   roadmap_main_set_periodic (100, delayed_cancel);

   return 1;
}

static BOOL on_key_pressed_cancel (SsdWidget widget, const char* utf8char, uint32_t flags)
{
   if( KEY_IS_ENTER)
   {
      on_cancel(NULL, NULL);
      return TRUE;
   }

   return FALSE;
}


static int long_click (SsdWidget widget, const RoadMapGuiPoint *point) {

   return short_click(widget, point);
}

SsdWidget ssd_menu_new_grid (const char           *name,
                               SsdWidget          addition_conatiner,
                               const char           *items_file,
                               const char           *items[],
                               const RoadMapAction  *actions,
                               int                   flags) {

    SsdWidget container;
    SsdWidget space;
    int i;
    int next_item_flags = 0;
    int row_height = ssd_container_get_row_height();

    const char **menu_items =
       roadmap_factory_user_config (items_file, "menu", actions);

    SsdWidget dialog = ssd_dialog_new (name, roadmap_lang_get (name), NULL,
                       flags|SSD_ALIGN_CENTER|SSD_DIALOG_GUI_TAB_ORDER);

    int width  = ssd_container_get_width();
    container = ssd_container_new (name, NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
                                   SSD_CONTAINER_FLAGS|SSD_CONTAINER_BORDER|SSD_ALIGN_CENTER);
    //ssd_widget_set_color (dialog, NULL, NULL);
    ssd_widget_set_color (container, NULL, NULL);
   //add additional container
      if (addition_conatiner != NULL){
         space = ssd_container_new ("spacer", NULL, SSD_MAX_SIZE, 5, SSD_WIDGET_SPACE|SSD_END_ROW);
         ssd_widget_set_color (space, NULL,NULL);
         if (flags & SSD_DIALOG_ADDITION_BELOW) {
            ssd_widget_add(dialog, space);
            ssd_widget_add(dialog, container);
            ssd_widget_add(dialog, addition_conatiner);
         } else {
            ssd_widget_add(dialog, space);
            ssd_widget_add(dialog, addition_conatiner);
            ssd_widget_add(dialog, container);
         }
      }
      else{
       space = ssd_container_new ("spacer", NULL, SSD_MAX_SIZE, 5, SSD_WIDGET_SPACE|SSD_END_ROW);
       ssd_widget_set_color (space, NULL,NULL);
       ssd_widget_add(dialog, space);
       ssd_widget_add(dialog, container);
      }


    /* Override long click */
    //container->long_click = long_click;

    if (!menu_items) menu_items = items;

    for (i = 0; menu_items[i] != NULL; ++i) {

       const char *item = menu_items[i];

       if (!strcmp(item,RoadMapFactorySeparator)) {
          next_item_flags = SSD_START_NEW_ROW;

       } else {

          SsdWidget text_box,button_container,button;
          const RoadMapAction *this_action = find_action (actions, item);
          const char *button_icon[2];
          SsdWidget text;
          SsdWidget w;
         // int height = 60;

          //if ( roadmap_screen_is_hd_screen() )
          //{
           //height = 90;
         // }

          w = ssd_container_new (item, NULL,
                            100, 100,
                            SSD_WS_TABSTOP|SSD_ALIGN_CENTER|next_item_flags);
          ssd_widget_set_color(w, "#00ff00", "#00ff00");

          w->long_click = long_click;
          w->key_pressed = on_key_pressed;
          w->short_click = short_click;
          w->pointer_down = on_pointer_down;
          ssd_widget_set_pointer_force_click(w);
          w->key_pressed = on_key_pressed;
          ssd_widget_set_color (w, NULL, NULL);

          button_container = ssd_bitmap_new("button_container", "button_bg",
                                          SSD_ALIGN_VCENTER|SSD_ALIGN_CENTER);
          ssd_dialog_add_vspace(button_container, 10, 0);
          ssd_widget_set_color(button_container, "#ff0000", "#ff0000");
          button_icon[0] = item;
          button_icon[1] = NULL;
          button = ssd_button_new
                               (item, item, button_icon, 1, SSD_ALIGN_CENTER|SSD_END_ROW,
                                NULL);
          ssd_widget_set_context (w, this_action->callback);
          ssd_widget_add(button_container, button);


          text = ssd_text_new ("label_long",
                              roadmap_lang_get (this_action->label_long),
                              SSD_MAIN_TEXT_SIZE, SSD_TEXT_NORMAL_FONT|SSD_ALIGN_CENTER);
          ssd_widget_add (button_container, text);
          ssd_widget_add(w, button_container);
          ssd_widget_add (container, w);

          next_item_flags = 0;
          }



    }

    return dialog;
}

SsdWidget ssd_menu_new (const char           *name,
                               SsdWidget          addition_conatiner,
                               const char           *items_file,
                               const char           *items[],
                               const RoadMapAction  *actions,
                               int                   flags) {

     return ssd_menu_new_cb(name, addition_conatiner, items_file, items, actions, flags, NULL);
}

SsdWidget ssd_menu_new_cb (const char           *name,
							   SsdWidget        	 addition_conatiner,
                               const char           *items_file,
                               const char           *items[],
                               const RoadMapAction  *actions,
                               int                   flags,
                               PFN_ON_DIALOG_CLOSED on_dialog_closed) {

   const char *edit_button_r[] = {"edit_right", "edit_right"};
   const char *edit_button_l[] = {"edit_left", "edit_left"};
   SsdWidget container;
   int i;
   int next_item_flags = 0;
   int row_height = ssd_container_get_row_height();

   const char **menu_items =
      roadmap_factory_user_config (items_file, "menu", actions);

   SsdWidget dialog = ssd_dialog_new (name, roadmap_lang_get (name), on_dialog_closed,
                      flags|SSD_ALIGN_CENTER|SSD_DIALOG_GUI_TAB_ORDER);

   if (flags & SSD_DIALOG_FLOAT) {
      int height = roadmap_canvas_height();
      int width = roadmap_canvas_width();
#ifndef EMBEDDED_CE
      if (height < width)
#ifndef IPHONE_NATIVE
         width = height;
#else
         width = ADJ_SCALE(320);
#endif
#endif

      width -= ADJ_SCALE(40);

      if (!roadmap_screen_is_hd_screen()){
        if (width > 240)
          width = 240;
      }
      else{
        if (width > 480)
          width = 480;
      }

      if (flags & SSD_DIALOG_VERTICAL){
         width = SSD_MIN_SIZE;
      }

      ssd_widget_set_size(dialog, SSD_MIN_SIZE, SSD_MIN_SIZE);

      container = ssd_container_new (name, NULL, width
                     , SSD_MIN_SIZE, 0);
      ssd_widget_set_color (dialog, "#000000", "#ff0000000");
      ssd_widget_set_color (container, "#000000", "#ff0000000");

      ssd_widget_add(dialog, container);
   } else {
   	SsdWidget space;
      int width  = ssd_container_get_width();
      container = ssd_container_new (name, NULL, width, SSD_MIN_SIZE,
                                     SSD_CONTAINER_FLAGS|SSD_CONTAINER_BORDER|SSD_ALIGN_CENTER);
      //ssd_widget_set_color (dialog, NULL, NULL);
      ssd_widget_set_color (container, NULL, NULL);
	  //add additional container
   	  if (addition_conatiner != NULL){
   	     space = ssd_container_new ("spacer", NULL, SSD_MAX_SIZE, 5, SSD_WIDGET_SPACE|SSD_END_ROW);
   	     ssd_widget_set_color (space, NULL,NULL);
   	     if (flags & SSD_DIALOG_ADDITION_BELOW) {
   	        ssd_widget_add(dialog, space);
              ssd_widget_add(dialog, container);
              ssd_widget_add(dialog, addition_conatiner);
           } else {
              ssd_widget_add(dialog, space);
              ssd_widget_add(dialog, addition_conatiner);
              ssd_widget_add(dialog, container);
           }
   	  }
   	  else{
   	  	space = ssd_container_new ("spacer", NULL, SSD_MAX_SIZE, 5, SSD_WIDGET_SPACE|SSD_END_ROW);
   	  	ssd_widget_set_color (space, NULL,NULL);
   	  	ssd_widget_add(dialog, space);
   	  	ssd_widget_add(dialog, container);
   	  }
   }


   /* Override long click */
   //container->long_click = long_click;

   if (!menu_items) menu_items = items;

   for (i = 0; menu_items[i] != NULL; ++i) {

      const char *item = menu_items[i];

      if (!strcmp(item,RoadMapFactorySeparator)) {
         next_item_flags = SSD_START_NEW_ROW;

      } else {
      	 if (flags & SSD_DIALOG_FLOAT){
	      	SsdWidget text_box;
	      	SsdWidget button_cont;
	         SsdSize size;
	         const RoadMapAction *this_action = find_action (actions, item);
	         const char *button_icon[2];
	         int item_offset_y = -2;
	         const double hd_factor = 1.5; // AGA TODO: Replace by the dynamic
	         SsdWidget w = ssd_container_new (item, NULL,
	                           SSD_MAX_SIZE, row_height,
	                           SSD_WS_TABSTOP|next_item_flags);
	         SsdWidget button;
	         SsdWidget text;

	         button_icon[0] = item;
	         button_icon[1] = NULL;

	         ssd_widget_set_color (w, NULL, NULL);

	         button_cont = ssd_container_new ("Button_con", NULL,
                           ADJ_SCALE(60), row_height,
                           0);
	         ssd_widget_set_color(button_cont, NULL, NULL);
	         button = ssd_button_new
	                    (item, item, button_icon, 1,
	                     SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER, NULL);

	         ssd_widget_add(button_cont, button);
	         ssd_widget_set_pointer_force_click( w );
	         w->long_click = short_click;
	         w->short_click = short_click;
	         w->pointer_down = on_pointer_down;
	         ssd_widget_set_pointer_force_click(w);
	         w->key_pressed = on_key_pressed;
	         ssd_widget_get_size (button ,&size, NULL);

	         ssd_widget_set_context (w, this_action->callback);
	         ssd_widget_add (w, button_cont);

            text_box = ssd_container_new ("text_box", NULL,
                                         SSD_MIN_SIZE,
                                         SSD_MIN_SIZE,
                                         SSD_ALIGN_VCENTER|SSD_END_ROW);

            ssd_widget_set_color (text_box, NULL, NULL);

            text = ssd_text_new ("label_long",
                                roadmap_lang_get (this_action->label_long),
                                SSD_MAIN_TEXT_SIZE, SSD_ALIGN_VCENTER|SSD_END_ROW);
            ssd_widget_set_color(text, "#ffffff","#000000");
            if ( roadmap_screen_is_hd_screen() )
               item_offset_y *= hd_factor;
            ssd_widget_set_offset( text, 0, item_offset_y );
            ssd_widget_add (text_box, text);
            ssd_widget_add (w, text_box);

	         ssd_widget_add (container, w);
            if (menu_items[i+1] != NULL)
               ssd_widget_add(container, ssd_separator_new("Separator",0));

#ifndef ANDROID
            if (menu_items[i+1] == NULL){
               SsdWidget w = ssd_container_new ("cancel", NULL,
                                 SSD_MAX_SIZE, row_height,
                                 SSD_WS_TABSTOP|next_item_flags);
               ssd_widget_add(container, ssd_separator_new("Separator",0));
               ssd_widget_set_color (w, NULL, NULL);
               w->short_click = on_cancel;
               w->pointer_down = on_pointer_down;
               w->key_pressed = on_key_pressed_cancel;
               ssd_widget_set_pointer_force_click(w);
               text = ssd_text_new("cancel", roadmap_lang_get("Cancel"), SSD_MAIN_TEXT_SIZE, SSD_ALIGN_VCENTER|SSD_ALIGN_CENTER);
               ssd_widget_set_color(text, "#ffffff","#ffffff");
               ssd_widget_set_pointer_force_click(w);
               ssd_widget_add(w, text);
               ssd_widget_add (container, w);
            }
#endif
      	 }
      	 else{
         SsdWidget text_box,button_container,button;
         const RoadMapAction *this_action = find_action (actions, item);
         const char *button_icon[2];
         SsdWidget text;
         SsdWidget w;
        // int height = 60;

         //if ( roadmap_screen_is_hd_screen() )
         //{
        	 //height = 90;
        // }

         w = ssd_container_new (item, NULL,
                           SSD_MAX_SIZE, row_height,
                           SSD_WS_TABSTOP|SSD_ALIGN_CENTER|next_item_flags|SSD_END_ROW);

         w->long_click = long_click;
         w->key_pressed = on_key_pressed;
         w->short_click = short_click;
         w->pointer_down = on_pointer_down;
         ssd_widget_set_pointer_force_click(w);
         w->key_pressed = on_key_pressed;
         ssd_widget_set_color (w, "#000000", NULL);

         button_container = ssd_container_new ("button_container", NULL,
                                         ADJ_SCALE(80),
                                         SSD_MIN_SIZE,
                                         SSD_ALIGN_VCENTER);
         ssd_widget_set_color(button_container, NULL, NULL);
         button_icon[0] = item;
         button_icon[1] = NULL;
         button = ssd_button_new
                              (item, item, button_icon, 1, SSD_ALIGN_VCENTER|SSD_ALIGN_CENTER,
                               NULL);
         ssd_widget_set_context (w, this_action->callback);
         ssd_widget_add(button_container, button);
         ssd_widget_add(w, button_container);


           text_box = ssd_container_new ("text_box", NULL,
                                         SSD_MIN_SIZE,
                                         SSD_MIN_SIZE,
                                         SSD_ALIGN_VCENTER);
           ssd_widget_set_color (text_box, NULL, NULL);

           text = ssd_text_new ("label_long",
                                roadmap_lang_get (this_action->label_long),
                                SSD_MAIN_TEXT_SIZE, SSD_TEXT_NORMAL_FONT);
           ssd_text_set_color(text, SSD_CONTAINER_TEXT_COLOR);
           ssd_widget_add (text_box, text);
           text = ssd_text_new ("right_text",
                          "",
                          SSD_SECONDARY_TEXT_SIZE, SSD_ALIGN_RIGHT|SSD_ALIGN_VCENTER);
           if (ssd_widget_rtl(NULL))
              ssd_widget_set_offset(text, 15,0);
           else
              ssd_widget_set_offset(text, -15,0);
           ssd_widget_set_color(text, "#206892","#ffffff");
           ssd_widget_add (w, text);
           ssd_widget_add (w, text_box);
           if (!ssd_widget_rtl(NULL))
              button = ssd_button_new ("edit_button", "", &edit_button_r[0], 2,
                       SSD_ALIGN_VCENTER|SSD_ALIGN_RIGHT, NULL);
           else
              button = ssd_button_new ("edit_button", "", &edit_button_l[0], 2,
                       SSD_ALIGN_VCENTER|SSD_ALIGN_RIGHT, NULL);
           if (!ssd_widget_rtl(NULL))
              ssd_widget_set_offset(button, -10, 0);
           else
              ssd_widget_set_offset(button, 11, 0);
           ssd_widget_add(w, button);

         ssd_widget_add (container, w);
         if (menu_items[i+1] != NULL){
            ssd_widget_add(container, ssd_separator_new("Separator",SSD_END_ROW));
         }

      	 }

         next_item_flags = 0;
      }
   }

   return dialog;
}

void ssd_menu_set_right_text(char *name, char *item, char *text){
   SsdWidget widget;
   SsdWidget text_w;
   SsdWidget dialog = ssd_dialog_activate (name, NULL);
   if (!dialog)
      return;
   widget = ssd_widget_get(dialog, item);
   if (!widget)
      return;
   text_w = ssd_widget_get(widget, "right_text");
   ssd_text_set_text(text_w, text);

}

void ssd_menu_set_label_long_text(char *name, char *item, const char *text){
   SsdWidget widget;
   SsdWidget text_w;
   SsdWidget dialog = ssd_dialog_activate (name, NULL);
   if (!dialog)
      return;
   widget = ssd_widget_get(dialog, item);
   if (!widget)
      return;
   text_w = ssd_widget_get(widget, "label_long");
   ssd_text_set_text(text_w, text);

}
static SsdWidget  ssd_menu_list_new(const char           *name,
                               const char           *items_file,
                               const char           *items[],
                               const RoadMapAction  *actions,
                               int                   flags) {
   int i;
   int next_item_flags = 0;
   int width = SSD_MAX_SIZE;
   SsdSize button_size;
   SsdSize size;
   SsdWidget dialog;
   const char **menu_items = NULL;
   const char *button_icon[3];
   int rtl_flag;

#ifdef TOUCH_SCREEN
   SsdWidget scroll_up, scroll_down;
   SsdWidget container;
#endif
   menu_list_data *data =  ( menu_list_data *)calloc (1, sizeof(menu_list_data));


   dialog = ssd_dialog_new (name, roadmap_lang_get (name), NULL,
                      flags|SSD_DIALOG_GUI_TAB_ORDER);

     data->num_items = 0;
     data->CurrentIndex = 0;

   if( items_file)
      menu_items = roadmap_factory_user_config (items_file, "menu", actions);

   if (flags & SSD_DIALOG_FLOAT) {

        if (flags & SSD_DIALOG_VERTICAL){
           width = SSD_MIN_SIZE;


        }
        dialog->draw = draw_bg;
        data->container = ssd_container_new (name, NULL, width, SSD_MIN_SIZE, 0);
        ssd_widget_set_size (dialog, SSD_MIN_SIZE, SSD_MIN_SIZE);
        ssd_widget_set_color (dialog, NULL, NULL);
        ssd_widget_set_color (data->container, NULL, NULL);
        if (!is_screen_wide())
           ssd_widget_set_offset(data->container,0, 0-roadmap_bar_top_height());

     } else {
        data->container = ssd_container_new (name, NULL, SSD_MAX_SIZE, SSD_MAX_SIZE,
                                    SSD_ALIGN_GRID);
        ssd_widget_set_color (dialog, "#000000", "#ffffffee");
        ssd_widget_set_color (data->container, "#000000", NULL);
     }


   /* Override long click */
   data->container->long_click = long_click;

   if (!menu_items) menu_items = items;

   if (ssd_widget_rtl (NULL))
	 rtl_flag = SSD_END_ROW;
   else
	 rtl_flag = SSD_ALIGN_RIGHT;

#ifdef TOUCH_SCREEN
   container = ssd_container_new ("scroll_up_con", NULL,
                           70, SSD_MIN_SIZE,
                           SSD_END_ROW);
   ssd_widget_set_color (container, "#000000", NULL);
   button_icon[0] = "menu_page_up";
   scroll_up = ssd_button_new
                    ("scroll_up", "scroll_up", button_icon, 1,
                     SSD_WS_TABSTOP|SSD_END_ROW|SSD_VAR_SIZE|SSD_ALIGN_CENTER,
                     scroll_buttons_callback);
   ssd_widget_add (container, scroll_up);
   ssd_widget_add (dialog, container);
#endif

   for (i = 0; menu_items[i] != NULL; ++i) {

      const char *item = menu_items[i];

      if (item == RoadMapFactorySeparator) {
         next_item_flags = SSD_START_NEW_ROW;

      } else {
         char focus_name[250];
         SsdWidget button;
         SsdWidget w;
         const RoadMapAction *this_action = find_action (actions, item);


         strcpy(focus_name, item);
         strcat(focus_name,"_focus_");
         strcat(focus_name, roadmap_lang_get_system_lang());
         if ( !roadmap_res_get( RES_BITMAP, RES_SKIN, focus_name ) )
         {
			int offset;
            roadmap_log( ROADMAP_WARNING, "Cannot find resource: %s. Loading english resource instead", focus_name );
            offset = strlen( focus_name ) - strlen( roadmap_lang_get_system_lang() );
            strcpy( focus_name + offset, "eng" );
         }

         button_icon[0] = item;
         button_icon[1] = strdup(focus_name);
         button_icon[2] = NULL;

         w = ssd_container_new (item, NULL,
                           SSD_MIN_SIZE, SSD_MIN_SIZE,
                           next_item_flags|rtl_flag);

         ssd_widget_set_color (w, "#000000", NULL);


        button = ssd_button_new
                    ("button", item, button_icon, 2,
                     SSD_WS_TABSTOP|SSD_VAR_SIZE|SSD_ALIGN_CENTER,
                     button_callback );
         ssd_widget_get_size(button, &button_size, NULL );
         button->long_click = long_click;

         ssd_widget_set_context (button, this_action->callback);
         ssd_widget_add (w, button);
         ssd_widget_hide(w);

         if ( rtl_flag == SSD_ALIGN_RIGHT )
         {
            ssd_widget_set_offset( w, -10, 0 );
         }
         else
         {
            ssd_widget_set_offset( w, 5, 0 );
         }
         data->MenuWidgets[data->num_items] = w;
         data->num_items++;
         ssd_widget_add (data->container, w);

         next_item_flags = 0;
      }
   }


   ssd_widget_container_size (dialog, &size);
   if (data->num_items > 0){
   	ssd_widget_container_size (dialog, &size);
   	data->num_rows = size.height / button_size.height;
   	if  (data->num_items <= data->num_rows){
	   	int i;
   		for (i = 0 ;i < data->num_items; i++){
   			ssd_widget_show (data->MenuWidgets[i]);
   			data->MenuWidgets[i]->children->key_pressed = OnKeyPressed;
   		}
   	}
   	else{
   		for (i = 0 ;i < data->num_rows; i++){
   			ssd_widget_show (data->MenuWidgets[i]);
   		}

   		for (i=0; i<data->num_items;i++){
   			data->MenuWidgets[i]->children->key_pressed = OnKeyPressed;
   		}
   	}
   }
   ssd_widget_add (dialog, data->container);

#if defined (TOUCH_SCREEN) && !defined (IPHONE)
   container = ssd_container_new ("scroll_down_con", NULL,
                           70, SSD_MIN_SIZE,
                           SSD_START_NEW_ROW|SSD_END_ROW);
   ssd_widget_set_color (container, "#000000", NULL);
   button_icon[0] = "menu_page_down";
   scroll_down = ssd_button_new
                    ("scroll_down", "scroll_down", button_icon, 1,
                     SSD_START_NEW_ROW|SSD_WS_TABSTOP|SSD_END_ROW|SSD_VAR_SIZE|SSD_ALIGN_CENTER,
                     scroll_buttons_callback);
   ssd_widget_add (container, scroll_down);
   ssd_widget_add (dialog, container);
#endif


   dialog->data = (void *)data;
   return dialog;
}



void ssd_menu_activate (const char           *name,
                        const char           *items_file,
                        const char           *items[],
                        SsdWidget 			 addition_conatiner,
                        PFN_ON_DIALOG_CLOSED on_dialog_closed,
                        const RoadMapAction  *actions,
                        int                   flags,
                        int                   animation) {
   SsdWidget dialog = ssd_dialog_activate (name, NULL);



   if (dialog) {
      ssd_dialog_set_callback (on_dialog_closed);
      ssd_widget_set_flags (dialog, flags);
      ssd_dialog_draw ();
      return;
   }

   dialog = ssd_menu_new (name, addition_conatiner, items_file, items, actions, flags);
   ssd_dialog_set_animation(name, animation);
   ssd_dialog_activate (name, NULL);
   ssd_dialog_set_callback (on_dialog_closed);
   ssd_dialog_draw ();
}


void ssd_grid_menu_activate (const char           *name,
                        const char           *items_file,
                        const char           *items[],
                        SsdWidget          addition_conatiner,
                        PFN_ON_DIALOG_CLOSED on_dialog_closed,
                        const RoadMapAction  *actions,
                        int                   flags) {
   SsdWidget dialog = ssd_dialog_activate (name, NULL);



   if (dialog) {
      ssd_dialog_set_callback (on_dialog_closed);
      ssd_widget_set_flags (dialog, flags);
      ssd_dialog_draw ();
      return;
   }

   dialog = ssd_menu_new_grid (name, addition_conatiner, items_file, items, actions, flags);

   ssd_dialog_activate (name, NULL);
   ssd_dialog_set_callback (on_dialog_closed);
   ssd_dialog_draw ();
}

void ssd_list_menu_activate (const char           *name,
                        const char           *items_file,
                        const char           *items[],
                        PFN_ON_DIALOG_CLOSED on_dialog_closed,
                        const RoadMapAction  *actions,
                        int                   flags) {

   menu_list_data *data;
   int align = 0;
   SsdWidget dialog = ssd_dialog_activate (name, NULL);

   if (!ssd_widget_rtl (NULL))
		align = SSD_ALIGN_RIGHT;

   if (dialog) {
   	  ssd_dialog_set_callback (on_dialog_closed);
      ssd_widget_set_flags (dialog, flags|align);
      data = (menu_list_data *)dialog->data;

#ifndef TOUCH_SCREEN
      dialog->in_focus = dialog->default_widget;
      move_to_start();
#endif
      ssd_dialog_draw ();
      return;
   }


   dialog = ssd_menu_list_new (name, items_file, items, actions, flags|align);
   data = (menu_list_data *)dialog->data;
   data->CurrentIndex = 0;

   ssd_dialog_activate (name, NULL);
   ssd_dialog_set_callback (on_dialog_closed);
   ssd_dialog_draw ();
}

void ssd_menu_hide (const char *name) {

   ssd_dialog_hide (name, dec_close);
}


void ssd_menu_load_images(const char   *items_file, const RoadMapAction  *actions){

	/*
	 * AGA TODO:: Check image locking here
	 */
#ifdef TOUCH_SCREEN
     const char **menu_items = roadmap_factory_user_config (items_file, "menu", actions);
     int i;

     for (i = 0; menu_items[i] != NULL; ++i) {
         const char *item = menu_items[i];
         if (item != RoadMapFactorySeparator) {
               roadmap_res_get(RES_BITMAP,
                                 RES_SKIN|RES_NOCACHE,
                                item);
         }
   }
#endif
}
/*
 * Sets the button icon of the menu item in runtime
 */
void ssd_menu_set_item_icon( SsdWidget menu, const char* item_name, const char* icon_name )
{
	SsdWidget item;
	SsdWidget btn_cnt, btn;
	const char* icons[2];
	/* Menu item */
	item = ssd_widget_get( menu, item_name );
	if (!item)
	   return;

	/* Button container */
	btn_cnt = ssd_widget_get( item, "button_container" );
	if (!btn_cnt)
	   return;

	/* Button */
	btn = ssd_widget_get( btn_cnt, item_name );
	if (!btn)
	   return;

	icons[0] = icon_name;
	icons[1] = NULL;
	ssd_button_change_icon( btn, icons, 1 );

}
