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
#include "ssd_combobox_dialog.h"


#define  SSD_CBDLG_DIALOG_NAME      ("combobox dialog")


typedef struct tag_ssd_cmb_dlg_ctx
{
   PFN_ON_INPUT_DIALOG_CLOSED on_combobox_closed;
   PFN_ON_INPUT_CHANGED       on_input_changed;
   SsdSoftKeyCallback         left_softkey_callback;
   void*                      context;

}  ssd_cmb_dlg_ctx, *ssd_cmb_dlg_ctx_ptr;

static SsdWidget        s_combo_dialog    = NULL;
static BOOL             dialog_is_active  = FALSE;
static ssd_cmb_dlg_ctx  s_ctx;

static void on_dialog_closed( int exit_code, void* context)
{
   dialog_is_active = FALSE;
   
   if(s_ctx.on_combobox_closed)
      s_ctx.on_combobox_closed( 
               exit_code, 
               ssd_combobox_get_text(s_combo_dialog), 
               s_ctx.context);
}

static int combox_left_softkey_callback (SsdWidget widget, const char *new_value, void *context)
{
   ssd_cmb_dlg_ctx_ptr ctx = (ssd_cmb_dlg_ctx_ptr)ssd_combobox_get_context(widget);
   
   if( ctx->left_softkey_callback)
      return ctx->left_softkey_callback( widget, new_value, ctx->context);
      
   return 0;
}

static void on_input_changed(const char* new_text, void* context){
   ssd_cmb_dlg_ctx_ptr ctx = (ssd_cmb_dlg_ctx_ptr)context;
   
   if(ctx->on_input_changed)
      ctx->on_input_changed( new_text, ctx->context);
}

void ssd_combobox_dialog_show (
            const char*                title, 
            int                        count, 
            const char**               labels,
            const void**               values,
            const char**               icons,            
            PFN_ON_INPUT_CHANGED       on_text_changed,
            PFN_ON_INPUT_DIALOG_CLOSED on_combobox_closed,
            SsdListCallback            on_list_selection,
            unsigned short             input_type,   //   txttyp_<xxx> combination from 'roadmap_text.h'
            void*                      context,
            const char*                left_softkey_text, 
            SsdSoftKeyCallback         left_softkey_callback)
{
   if( dialog_is_active)
      return;   //   Disable recursive instances


   s_ctx.context              = context;
   s_ctx.on_combobox_closed   = on_combobox_closed;
   s_ctx.left_softkey_callback= left_softkey_callback;
   s_ctx.on_input_changed     = on_text_changed;
   if( !s_combo_dialog)
   {
      s_combo_dialog = ssd_dialog_new( SSD_CBDLG_DIALOG_NAME, 
                                       title, 
                                       on_dialog_closed,
                                       SSD_DIALOG_GUI_TAB_ORDER|SSD_CONTAINER_BORDER|SSD_CONTAINER_TITLE);
      
      
      ssd_combobox_new( s_combo_dialog,
                        title, 
                        count, 
                        labels,
                        values,
                        icons,
                        on_input_changed,
                        on_list_selection,
                        NULL,
                        input_type,
                        &s_ctx);
   }
   else
      ssd_combobox_update_list( s_combo_dialog, count, labels, values, icons);

   ssd_widget_set_left_softkey_text(s_combo_dialog, left_softkey_text);
   if( left_softkey_callback)
      ssd_widget_set_left_softkey_callback(s_combo_dialog, combox_left_softkey_callback);

   s_combo_dialog->set_value( s_combo_dialog, title);
      
   ssd_dialog_activate (SSD_CBDLG_DIALOG_NAME, NULL);

   ssd_dialog_draw ();
   ssd_combobox_reset_focus( s_combo_dialog);
   
   dialog_is_active = TRUE;
}

//   Update list-box with new values:
void ssd_combobox_dialog_update_list(  int            count, 
                                       const char**   labels,
                                       const void**    values,
                                       const char**      icons)
{  ssd_combobox_update_list( s_combo_dialog, count, labels, values, icons);}                              

SsdWidget ssd_combobox_dialog_get_textbox(){
   return ssd_combobox_get_textbox(s_combo_dialog);
}

void* ssd_combobox_dialog_get_context(){
   return ssd_combobox_get_context(s_combo_dialog);
}

SsdWidget ssd_combobox_dialog_get_list(){
   return ssd_combobox_get_list(s_combo_dialog);
}

const char* ssd_combobox_dialog_get_text(){
   return ssd_combobox_get_text(s_combo_dialog);
}

void ssd_combobox_dialog_set_text(const char* text){
    ssd_combobox_set_text(s_combo_dialog, text);
}
