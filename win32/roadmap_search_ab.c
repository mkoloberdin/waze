/* roadmap_search_ab.c
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi B.S
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
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "roadmap.h"

#include "ssd/ssd_widget.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_generic_list_dialog.h"
#include "ssd/ssd_menu.h"
#include "ssd/ssd_list.h"
#include "ssd/ssd_contextmenu.h"
#include "ssd/ssd_confirm_dialog.h"
#include "ssd/ssd_keyboard_dialog.h"
#include "ssd/ssd_separator.h"

static int on_pointer_down( SsdWidget this, const RoadMapGuiPoint *point)
{

   ssd_widget_pointer_down_force_click(this, point );

   if( !this->tab_stop)
      return 0;

   if( !this->in_focus)
      ssd_dialog_set_focus( this);
   roadmap_screen_redraw();
   return 0;
}

static int search_ab_dlg_callback (SsdWidget widget, const char *new_value) {

   if (!strcmp(widget->name,"Cancel")){
   }
   else {
	   char * address = (char *)widget->context;
	   single_search_auto_search(address);
   }

   ssd_dialog_hide ("contact_chooser", dec_ok);
   return 0;
}

void activate_dlg(void){
	roadmap_main_remove_periodic(activate_dlg);
}
void address_book_result_dlg(char *name, char *home, char *work, char *other){
	 int count = 0;
	 if (home && *home)
		 count++;
	 if (work && *work)
		 count++;
	 if (other && *other)
		 count++;

	  if (count == 1){
		if (home && *home)
			single_search_auto_search(home);
		else if (work && *work)
			single_search_auto_search(work);
		else if (other && *other)
			single_search_auto_search(other);
		return;

	  }
	  else{ //Selection dlg
		  SsdWidget dialog;
		  SsdWidget container;
		  SsdWidget box;
		  SsdWidget text;
		  SsdWidget bitmap;
		  SsdWidget bitmap_container;
		  int height = 60;
	      if ( roadmap_screen_is_hd_screen() )
			   height = 90;
		  dialog = ssd_dialog_new ("contact_chooser", "contact chooser", NULL,
			 SSD_CONTAINER_BORDER|SSD_DIALOG_FLOAT|
			 SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_BLACK);
		  container = ssd_container_new("contact chooser.container", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE,0);
		  ssd_widget_set_color(container, NULL, NULL);

		  if (name && *name){
			box = ssd_container_new ("Name", NULL, SSD_MAX_SIZE, (int)height*1.5, SSD_END_ROW|SSD_WS_TABSTOP);
			ssd_widget_set_color(box, NULL, NULL);
			text = ssd_text_new ("NameTxt", name, 16, SSD_END_ROW|SSD_WIDGET_SPACE|SSD_ALIGN_VCENTER|SSD_ALIGN_CENTER);
			ssd_text_set_color(text,"#ffffff");
			ssd_widget_add(box, text);
			ssd_widget_add(container, box);
		  }

		  if (home && *home){
			box = ssd_container_new ("Home", NULL, SSD_MAX_SIZE, height, SSD_END_ROW|SSD_WS_TABSTOP);
			ssd_widget_set_color(box, NULL, NULL);
		    ssd_widget_add(box, ssd_separator_new("Separator",SSD_END_ROW));
		    bitmap_container = ssd_container_new("contact chooser.bitmap_container", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE,0);
	    	ssd_widget_set_color(bitmap_container, NULL, NULL);
			bitmap = ssd_bitmap_new ("bitmap", "home", SSD_ALIGN_VCENTER);
			ssd_widget_add(bitmap_container, bitmap );
			ssd_widget_add(box, bitmap_container );
			text = ssd_text_new ("HomeTxt", home, 16, SSD_END_ROW|SSD_WIDGET_SPACE|SSD_ALIGN_VCENTER);
			ssd_text_set_color(text,"#ffffff");
			ssd_widget_add(box, text);
			box->callback = search_ab_dlg_callback ;
			box->context = (void *)home ;
			ssd_widget_set_pointer_force_click( box );
			box->pointer_down = on_pointer_down;
			ssd_widget_add(container, box);
		  }

		  if (work && *work){
			box = ssd_container_new ("Work", NULL, SSD_MAX_SIZE, height, SSD_END_ROW|SSD_WS_TABSTOP);
			ssd_widget_set_color(box, NULL, NULL);
	        ssd_widget_add(box, ssd_separator_new("Separator",SSD_END_ROW));
		    bitmap_container = ssd_container_new("contact chooser.bitmap_container", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE,0);
	    	ssd_widget_set_color(bitmap_container, NULL, NULL);
			bitmap = ssd_bitmap_new ("bitmap", "work", SSD_ALIGN_VCENTER);
			ssd_widget_add(bitmap_container, bitmap );
			ssd_widget_add(box, bitmap_container );
			text = ssd_text_new ("WorklTxt", work, 16, SSD_END_ROW|SSD_WIDGET_SPACE|SSD_ALIGN_VCENTER);
			ssd_text_set_color(text,"#ffffff");
			ssd_widget_add(box, text);
			box->callback = search_ab_dlg_callback ;
			box->context = (void *)work ;
			ssd_widget_set_pointer_force_click( box );
			box->pointer_down = on_pointer_down;
			ssd_widget_add(container, box);
		  }
		  if (other && *other){
			box = ssd_container_new ("Other", NULL, SSD_MAX_SIZE, height, SSD_END_ROW|SSD_WS_TABSTOP);
			ssd_widget_set_color(box, NULL, NULL);
		    ssd_widget_add(box, ssd_separator_new("Separator",SSD_END_ROW));
		    bitmap_container = ssd_container_new("contact chooser.bitmap_container", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE,0);
	    	ssd_widget_set_color(bitmap_container, NULL, NULL);
			bitmap = ssd_bitmap_new ("bitmap", "search_address", SSD_ALIGN_VCENTER);
			ssd_widget_add(bitmap_container, bitmap );
			ssd_widget_add(box, bitmap_container );
			text = ssd_text_new ("OtherTxt", other, 16, SSD_END_ROW|SSD_WIDGET_SPACE|SSD_ALIGN_VCENTER);
			ssd_text_set_color(text,"#ffffff");
			ssd_widget_add(box, text);
			box->callback = search_ab_dlg_callback ;
			box->context = (void *)other ;
			ssd_widget_set_pointer_force_click( box );
			box->pointer_down = on_pointer_down;
			ssd_widget_add(container, box);
		  }

		  box = ssd_container_new ("Cancel", NULL, SSD_MAX_SIZE, height, SSD_END_ROW|SSD_WS_TABSTOP);
		  ssd_widget_set_color(box, NULL, NULL);
		  ssd_widget_add(box, ssd_separator_new("Separator",SSD_END_ROW));
		  text = ssd_text_new ("CancelTxt", roadmap_lang_get("Cancel"), 16, SSD_END_ROW|SSD_WIDGET_SPACE|SSD_ALIGN_VCENTER|SSD_ALIGN_CENTER);
		  ssd_text_set_color(text,"#ffffff");
		  ssd_widget_add(box, text);
		  box->callback = search_ab_dlg_callback ;
		  ssd_widget_set_pointer_force_click( box );
		  box->pointer_down = on_pointer_down;
		  ssd_widget_add(container, box);
	   	  ssd_widget_add(dialog, container);
  	  	  ssd_dialog_activate("contact_chooser", NULL);
		  roadmap_screen_refresh();

  	  }
 }

