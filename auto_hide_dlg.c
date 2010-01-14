
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
#include <stdlib.h>
#include <string.h>
#include "ssd/ssd_container.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_button.h"
#include "ssd/ssd_list.h"
#include "ssd/ssd_icon.h"
#include "ssd/ssd_contextmenu.h"
#include "ssd/ssd_bitmap.h"
#include "ssd/ssd_separator.h"
#include "roadmap_input_type.h"
#include "roadmap_lang.h"
#include "roadmap_main.h"
#include "roadmap_screen.h"

#include "auto_hide_dlg.h"

#define  AH_DIALOG_NAME          "auto-hide-dialog"
#define  AH_DIALOG_TITLE         "Suspend Waze"
#define  AH_CONT_NAME            "auto-hide-dialog.container"


static void on_timer__maximize(void)
{
   roadmap_main_remove_periodic( on_timer__maximize);

   roadmap_gui_maximize();
}

static void hide_now( int wait_time)
{

   roadmap_gui_minimize();

   if( -1 != wait_time)
      roadmap_main_set_periodic( wait_time, on_timer__maximize);

   ssd_dialog_hide_current( dec_close);
}



static int auto_hide_dlg_callback (SsdWidget widget, const char *new_value) {

   if (!strcmp(widget->name,"Exit")){
      roadmap_main_exit();
      return 0;
   }
   else if (!strcmp(widget->name,"Cancel")){
   }
   else if (!strcmp(widget->name,"Resume")){
      hide_now( -1);
   }
   else if (!strcmp(widget->name,"Resume30")){
      hide_now( 30000);
   }

   ssd_dialog_hide (AH_DIALOG_NAME, dec_ok);
   return 0;
}

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

void auto_hide_dlg(PFN_ON_DIALOG_CLOSED cbOnClosed){
   SsdWidget dialog;
   SsdWidget bitmap;
   SsdWidget text;
   SsdWidget space;
   SsdWidget container;
   SsdWidget box;
   int height = 45;

   if ( roadmap_screen_is_hd_screen() )
   {
	   height = 65;
   }

   dialog = ssd_dialog_new (AH_DIALOG_NAME, AH_DIALOG_TITLE, cbOnClosed,
         SSD_CONTAINER_BORDER|SSD_DIALOG_FLOAT|
         SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_BLACK);


   ssd_widget_set_color (dialog, "#000000", "#ff0000000");

   space = ssd_container_new ("spacer2", NULL, SSD_MAX_SIZE, 10, SSD_END_ROW);
   ssd_widget_set_color(space, NULL, NULL);
   ssd_widget_add (dialog, space);

   bitmap = ssd_bitmap_new("waze_log", "waze_logo", SSD_ALIGN_CENTER|SSD_END_ROW);
   ssd_widget_add (dialog,bitmap);

   space = ssd_container_new ("spacer2", NULL, SSD_MAX_SIZE, 10, SSD_END_ROW);
   ssd_widget_set_color(space, NULL, NULL);
   ssd_widget_add (dialog, space);

   container = ssd_container_new(AH_CONT_NAME, NULL, SSD_MIN_SIZE, SSD_MIN_SIZE,0);
   ssd_widget_set_color(container, NULL, NULL);

   box = ssd_container_new ("Resume", NULL, SSD_MAX_SIZE, height, SSD_END_ROW|SSD_WS_TABSTOP);
   ssd_widget_set_color(box, NULL, NULL);
   text = ssd_text_new ("ResumeTxt", roadmap_lang_get("Resume manually"), 16, SSD_END_ROW|SSD_WIDGET_SPACE|SSD_ALIGN_VCENTER|SSD_ALIGN_CENTER);
   ssd_text_set_color(text,"#ffffff");
   ssd_widget_add(box, text);
   ssd_widget_add(box, ssd_separator_new("Separator",SSD_ALIGN_BOTTOM));
   box->callback = auto_hide_dlg_callback ;
   ssd_widget_set_pointer_force_click( box );
   box->pointer_down = on_pointer_down;
   ssd_widget_add(container, box);

   box = ssd_container_new ("Resume30", NULL, SSD_MAX_SIZE, height, SSD_END_ROW|SSD_WS_TABSTOP);
   ssd_widget_set_color(box, NULL, NULL);
   text = ssd_text_new ("Resume30Txt", roadmap_lang_get("Resume in 30 seconds"), 16, SSD_END_ROW|SSD_WIDGET_SPACE|SSD_ALIGN_VCENTER|SSD_ALIGN_CENTER);
   ssd_text_set_color(text,"#ffffff");
   ssd_widget_add(box, text);
   ssd_widget_add(box, ssd_separator_new("Separator",SSD_ALIGN_BOTTOM));
   box->callback = auto_hide_dlg_callback ;
   ssd_widget_set_pointer_force_click( box );
   box->pointer_down = on_pointer_down;
   ssd_widget_add(container, box);

   box = ssd_container_new ("Exit", NULL, SSD_MAX_SIZE, height, SSD_END_ROW|SSD_WS_TABSTOP);
   ssd_widget_set_color(box, NULL, NULL);
   text = ssd_text_new ("ExitTxt", roadmap_lang_get("Quit"), 16, SSD_END_ROW|SSD_WIDGET_SPACE|SSD_ALIGN_VCENTER|SSD_ALIGN_CENTER);
   ssd_text_set_color(text,"#ffffff");
   ssd_widget_add(box, text);
   ssd_widget_add(box, ssd_separator_new("Separator",SSD_ALIGN_BOTTOM));
   box->callback = auto_hide_dlg_callback ;
   ssd_widget_set_pointer_force_click( box );
   box->pointer_down = on_pointer_down;
   ssd_widget_add(container, box);

   box = ssd_container_new ("Cancel", NULL, SSD_MAX_SIZE, height, SSD_END_ROW|SSD_WS_TABSTOP);
   ssd_widget_set_color(box, NULL, NULL);
   text = ssd_text_new ("CancelTxt", roadmap_lang_get("Cancel"), 16, SSD_END_ROW|SSD_WIDGET_SPACE|SSD_ALIGN_VCENTER|SSD_ALIGN_CENTER);
   ssd_text_set_color(text,"#ffffff");
   ssd_widget_add(box, text);
   box->callback = auto_hide_dlg_callback ;
   ssd_widget_set_pointer_force_click( box );
   box->pointer_down = on_pointer_down;
   ssd_widget_add(container, box);


   ssd_widget_add(dialog, container);
   ssd_dialog_activate(AH_DIALOG_NAME, NULL);

}
