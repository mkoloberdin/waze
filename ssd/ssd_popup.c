/* ssd_popup.h - PopUp widget
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi Ben-Shoshan
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
 *   See ssd_popup.h.
 */

#include <string.h>
#include <stdlib.h>
#include "roadmap.h"
#include "roadmap_screen.h"
#include "roadmap_math.h"
#include "roadmap_bar.h"
#include "ssd_dialog.h"
#include "ssd_container.h"
#include "ssd_button.h"
#include "ssd_text.h"
#include "ssd_popup.h"
#include "ssd_bitmap.h"

#define TITLE_TEXT_SIZE 16
#define POPUP_TEXT_SIZE 14
#define POPUP_HEIGHT        40
#define ICON_CONTAINER_SIZE 40
#define NEXT_CONTAINER_SIZE 40
#define DEAFULT_POPUP_TEXT_COLOR "#ffffff"
static int initial_x = 0;

#define POPUP_CLOSE_CLICK_OFFSETS_DEFAULT	{-20, -20, 20, 20 };

static SsdClickOffsets sgPopUpCloseOffsets = POPUP_CLOSE_CLICK_OFFSETS_DEFAULT;

struct ssd_popup_data {
   SsdCallback callback;
   void (*draw) (SsdWidget widget, RoadMapGuiRect *rect, int flags);
   RoadMapPosition position;
};

static void draw (SsdWidget widget, RoadMapGuiRect *rect, int flags){
	struct ssd_popup_data *data = (struct ssd_popup_data *)widget->data;
	widget->parent->context = (void *)&data->position;


	data->draw(widget,rect,flags);
}


SsdWidget ssd_popup_new (const char *name,
								 const char *title,
								 PFN_ON_DIALOG_CLOSED on_popup_closed,
								 int width,
								 int height,
								 const RoadMapPosition *position,
                         int flags,
                         int animation) {

   SsdWidget dialog, popup, header, text;
   int text_size = 20;
   int header_size = ADJ_SCALE(30);

   struct ssd_popup_data *data =
      (struct ssd_popup_data *)calloc (1, sizeof(*data));

   dialog = ssd_dialog_new(name,
            title,
            on_popup_closed,
            SSD_DIALOG_FLOAT|SSD_ALIGN_CENTER|SSD_CONTAINER_BORDER|SSD_ROUNDED_CORNERS|flags);
   ssd_dialog_set_animation(name, animation);
    popup =
      ssd_container_new (name, title, width, height, flags);
    ssd_widget_set_color(popup, NULL, NULL);
    if ((position != NULL) && ((position->latitude != 0) && (position->longitude != 0) )){
   	data->position.latitude = position->latitude;
   	data->position.longitude = position->longitude;
   }
   data->draw = popup->draw;
   popup->draw = draw;
   popup->data = data;
   popup->bg_color = NULL;

   if (title && *title){

    header = ssd_container_new ("header_conatiner", "", SSD_MIN_SIZE, header_size, SSD_END_ROW);
    ssd_widget_set_color(header, NULL, NULL);
    ssd_widget_set_click_offsets( header, &sgPopUpCloseOffsets );

	text = ssd_text_new("popuup_text", title, text_size, SSD_END_ROW|SSD_ALIGN_VCENTER);
	ssd_widget_set_color(text,"#f6a201", NULL);

#ifdef TOUCH_SCREEN

   ssd_widget_add(header, text);
#else
      ssd_widget_add(header, text);
#endif


   ssd_widget_add(popup, header);

   }
   ssd_widget_add(dialog, popup);
   return popup;
}

void ssd_popup_update_location(SsdWidget popup, const RoadMapPosition *position){
   struct ssd_popup_data *data =
      (struct ssd_popup_data *)popup->data;
   data->position.latitude = position->latitude;
   data->position.longitude = position->longitude;
}

static void draw_floating (SsdWidget widget, RoadMapGuiRect *rect, int flags){
   RoadMapGuiPoint point;
   int bar_height;
   int height = rect->maxy - rect->miny;

   struct ssd_popup_data *data = (struct ssd_popup_data *)widget->data;
   if (widget->parent)
      widget->parent->context = (void *)&data->position;

   roadmap_math_coordinate (&data->position, &point);
   roadmap_math_rotate_project_coordinate ( &point);
#ifdef TOUCH_SCREEN
   bar_height = roadmap_bar_top_height();
#else
   if (!is_screen_wide())
      bar_height = roadmap_bar_top_height();
   else
      bar_height = 0;
#endif
   rect->miny += (point.y - bar_height)-height -    ADJ_SCALE(30);
   rect->maxy += (point.y - bar_height)- height-   ADJ_SCALE(30);

   rect->minx += (point.x-initial_x);
   rect->maxx += (point.x-initial_x);

   widget->position.x = rect->minx;
   widget->position.y = rect->miny;
   data->draw(widget,rect,flags);
}

static void draw_floating_container (SsdWidget widget, RoadMapGuiRect *rect, int flags){

   struct ssd_popup_data *data = (struct ssd_popup_data *)widget->data;
   if (widget->parent)
      widget->parent->context = (void *)&data->position;

   data->draw(widget,rect,flags);
}

SsdWidget ssd_popup_new_float (const char *name,
                         const char *title,
                         PFN_ON_DIALOG_CLOSED on_popup_closed,
                         int width,
                         int height,
                         const RoadMapPosition *position,
                         int flags) {

   SsdWidget dialog, popup, header, text;
   int text_size = 20;
   int header_size = 30;
   int w_height = height;
   int w_width ;
   struct ssd_popup_data *data =
      (struct ssd_popup_data *)calloc (1, sizeof(*data));

   dialog = ssd_dialog_new(name,
            title,
            on_popup_closed,
            SSD_DIALOG_FLOAT|SSD_CONTAINER_BORDER|SSD_ROUNDED_CORNERS|flags);
   ssd_widget_set_offset(dialog, 0, ADJ_SCALE(-15));
   ssd_dialog_set_animation(name, DIALOG_ANIMATION_FROM_BOTTOM);
   ssd_widget_set_size(dialog, width+SSD_ROUNDED_CORNER_WIDTH*2, height+SSD_ROUNDED_CORNER_HEIGHT*2);

   if (height != SSD_MIN_SIZE){
      w_height = height+SSD_ROUNDED_CORNER_HEIGHT*2;
   }

   w_width = width+SSD_ROUNDED_CORNER_WIDTH*2;
   ssd_widget_set_size(dialog, w_width, w_height);

    popup =
      ssd_container_new (name, title, width, height, flags);
    ssd_widget_set_color(popup, NULL, NULL);
    if ((position != NULL) && ((position->latitude != 0) && (position->longitude != 0) )){
      data->position.latitude = position->latitude;
      data->position.longitude = position->longitude;
      dialog->context = (void *)&data->position;
   }

   data->draw = popup->draw;
   dialog->draw = draw_floating;
   dialog->data = data;

   popup->draw = draw_floating_container;
   popup->data = data;
   popup->bg_color = NULL;

   if (title && *title){

    header = ssd_container_new ("header_conatiner", "", SSD_MIN_SIZE, ADJ_SCALE(header_size), SSD_END_ROW);
    ssd_widget_set_color(header, NULL, NULL);
    ssd_widget_set_click_offsets( header, &sgPopUpCloseOffsets );

#ifdef IPHONE
      text_size = 18;
#endif
   text = ssd_text_new("popuup_text", title, text_size, SSD_END_ROW|SSD_ALIGN_VCENTER);
   ssd_widget_set_color(text,"#f6a201", NULL);

#ifdef TOUCH_SCREEN

   ssd_widget_add(header, text);
#else
      ssd_widget_add(header, text);
#endif


   ssd_widget_add(popup, header);

   }
   ssd_widget_add(dialog, popup);
   return popup;
}

int on_close (SsdWidget widget, const char *new_value){
   ssd_dialog_hide_current(dec_close);
   return 1;
}

void ssd_popup_show_float(const char *name,
                          const char *title,
                          const char *title_color,
                          const char *additional_text,
                          const char *additional_text_color,
                          const char *image,
                          const RoadMapPosition *position,
                          int position_offset_y,
                          PFN_ON_DIALOG_CLOSED on_popup_closed,
                          SsdCallback on_next,
                          const char *more_label,
                          void *context){

   SsdWidget popup, icon, container,text;
   char *icons[3];
   RoadMapGuiPoint point;
   int width = 160;
   int txt_width;
   SsdWidget wgt_hspace;
   SsdSize size;
   int additional_flags = SSD_ALIGN_CENTER;
   int title_flags = SSD_END_ROW|SSD_ALIGN_CENTER;
   int additional_text_flags = SSD_END_ROW|SSD_ALIGN_CENTER;

   if (position){
      roadmap_math_coordinate (position, &point);
      roadmap_math_rotate_project_coordinate ( &point );
      initial_x = point.x;
  }

   if (image) width += ICON_CONTAINER_SIZE;
#ifdef TOUCH_SCREEN
   if (on_next) width += NEXT_CONTAINER_SIZE;
#else
   width += NEXT_CONTAINER_SIZE;
#endif
   if (ssd_dialog_is_currently_active () && (!strcmp (ssd_dialog_currently_active_name (), name)))
       ssd_dialog_hide_current (dec_cancel);
   if (ssd_widget_rtl(NULL)){
      if (point.x < roadmap_canvas_width()/3)
         additional_flags = SSD_ALIGN_RIGHT;
      else if (point.x > 2*roadmap_canvas_width()/3)
         additional_flags = 0;
   }
   else{
      if (point.x < roadmap_canvas_width()/3)
         additional_flags = 0;
      else if (point.x > 2*roadmap_canvas_width()/3)
         additional_flags = SSD_ALIGN_RIGHT;
   }
   popup = ssd_popup_new_float (name, NULL, on_popup_closed,
                                ADJ_SCALE(width), SSD_MIN_SIZE, position, SSD_POINTER_FIXED_LOCATION | SSD_ROUNDED_BLACK | additional_flags);
   ssd_widget_set_offset(popup->parent, 0, position_offset_y);

   if (image){
      int image_container_flags = SSD_ALIGN_RIGHT;
      if (!ssd_widget_rtl(NULL))
         image_container_flags = 0;

      container = ssd_container_new ("FloatingPopupIconContainer", "", ADJ_SCALE(ICON_CONTAINER_SIZE), ADJ_SCALE(POPUP_HEIGHT), image_container_flags|SSD_ALIGN_VCENTER);
      container->callback = on_close;
      ssd_widget_set_color (container,NULL, NULL);
      icon = ssd_bitmap_new ("FloatingPopupIcon", image, SSD_ALIGN_VCENTER | SSD_ALIGN_CENTER);
      ssd_widget_add (container, icon);
      ssd_widget_add (popup, container);
   }

#ifdef TOUCH_SCREEN
   if (on_next){
      int next_container_flags = 0;
      if (!ssd_widget_rtl(NULL))
         next_container_flags = SSD_ALIGN_RIGHT;

      container = ssd_container_new ("FloatingPopupNextContainer", "", ADJ_SCALE(NEXT_CONTAINER_SIZE), ADJ_SCALE(POPUP_HEIGHT), SSD_ALIGN_VCENTER|next_container_flags);
      ssd_widget_set_color (container,NULL, NULL);
      icons[0] = "popup_right";
      icons[1] = "popup_right_s";
      icons[2] = NULL;
      icon = ssd_button_new( "PopUpNext", "PopUpNext", (const char**) &icons[0], 2, SSD_ALIGN_VCENTER | SSD_ALIGN_CENTER, on_next );
      icon->context = (void *)context;
      ssd_widget_add (container, icon);
      ssd_widget_add (popup, container);
   }
#else
   ssd_widget_set_left_softkey_callback(popup->parent, on_next);
   ssd_widget_set_left_softkey_text(popup->parent, roadmap_lang_get(more_label));
#endif
   txt_width =  ADJ_SCALE(width)-2;
   if (image)
      txt_width -=ADJ_SCALE(ICON_CONTAINER_SIZE);
#ifdef TOUCH_SCREEN
   if (on_next)
      txt_width -=ADJ_SCALE(NEXT_CONTAINER_SIZE);
#endif

   container = ssd_container_new ("FloatingPopupTextContainer", "",txt_width, SSD_MIN_SIZE, SSD_ALIGN_VCENTER);
   container->callback = on_close;
//   ssd_widget_set_offset(container, 0, 2);
   ssd_widget_set_color (container, NULL, NULL);

   if (roadmap_screen_is_hd_screen())
      ssd_widget_set_offset(container, 0, ADJ_SCALE(-3));

   if (title){
      int cont_flags = 0;
      if (!additional_text || additional_text[0] == 0){
         title_flags |= SSD_ALIGN_VCENTER;
         cont_flags = SSD_ALIGN_VCENTER;
      }

      text = ssd_text_new ("TitleTxt", title, TITLE_TEXT_SIZE, title_flags);
      if (title_color)
         ssd_text_set_color (text, title_color);
      else
         ssd_text_set_color (text, DEAFULT_POPUP_TEXT_COLOR);
      ssd_widget_add (container, text);
   }

   if (additional_text){
      if (!title || title[0] == 0){
         additional_text_flags |= SSD_ALIGN_VCENTER;
      }
      else{
         additional_text_flags |= SSD_TEXT_NORMAL_FONT;
         ssd_dialog_add_vspace(container,2,0);
      }
      text = ssd_text_new ("AdditionalTxt", additional_text, POPUP_TEXT_SIZE, additional_text_flags);
//      if (title && title[0] != 0)
//         ssd_widget_set_offset(text, 0, -2);

      if (additional_text_color)
         ssd_text_set_color (text, additional_text_color);
      else
         ssd_text_set_color (text, DEAFULT_POPUP_TEXT_COLOR);
      ssd_widget_add (container, text);
   }
   ssd_widget_add (popup, container);
   ssd_widget_get_size(container, &size, NULL);
   size.height += ADJ_SCALE(2*SSD_ROUNDED_CORNER_HEIGHT+30);
   if ((point.y +position_offset_y)< (roadmap_bar_top_height() + size.height))
      roadmap_screen_move(0, -(roadmap_bar_top_height() + size.height - point.y-position_offset_y));

   ssd_dialog_activate (name, NULL);
   ssd_dialog_set_close_on_any_click();
   if (!roadmap_screen_refresh ()) {
      roadmap_screen_redraw ();
   }
}
