
/* ssd_keyboard.c
 *
 * LICENSE:
 *
 *   Copyright 2009 PazO
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
 */
#include <string.h>

#include "roadmap.h"

#include "keyboards_layout/QWERTY.h"
#include "keyboards_layout/GRID.h"
#include "keyboards_layout/WIDEGRID.h"
#include "ssd_container.h"
#include "roadmap_lang.h"
#include "roadmap_config.h"
#include "roadmap_device_events.h"
#include "roadmap_canvas.h"
#include "roadmap_native_keyboard.h"
#include "ssd_keyboard.h"

extern BOOL roadmap_horizontal_screen_orientation();

extern int  ssd_keyboard_get_value_index( SsdWidget   kb);
extern void ssd_keyboard_set_value_index( SsdWidget   kb,
                                          int         index);
extern void ssd_keyboard_increment_value_index(
                                          SsdWidget   kb);

extern int  ssd_keyboard_get_layout(      SsdWidget   kb);
extern void ssd_keyboard_set_layout(      SsdWidget   kb,
                                          int         layout);

static BOOL          first_time_round  = TRUE;
static BOOL          enable_Hebrew     = TRUE;
static int           orientation_id    = 0;
static const char*   keyboard_name     = "ssd_embedded_keyboard";

typedef struct tag_kb_ctx
{
   SsdWidget                  container;
   SsdWidget                  keyboard;
   CB_OnKeyboardButtonPressed cbOnKey;
   CB_OnKeyboardCommand       cbOnSpecialButton;
   const char*                special_button_name;
   int                        orientation_id;
   void*                      context;

}  kb_ctx;

static   buttons_layout_info* s_keyboards_layout[] =
{
   &QWERTY_BUTTONS_LAYOUT,
   &GRID_BUTTONS_LAYOUT,
   &WIDEGRID_BUTTONS_LAYOUT
};

#define  CONFIG_CLASS         ("Keyboard")
#define  CONFIG_ENTRY_LAYOUT  ("Layout")
#define  CONFIG_ENTRY_INPUT   ("Input")

RoadMapConfigDescriptor last_cfg_layout =
                           ROADMAP_CONFIG_ITEM(
                                    CONFIG_CLASS,
                                    CONFIG_ENTRY_LAYOUT);

RoadMapConfigDescriptor last_cfg_input =
                           ROADMAP_CONFIG_ITEM(
                                    CONFIG_CLASS,
                                    CONFIG_ENTRY_INPUT);

static void update_dynamic_buttons__Shift(SsdWidget   kb,
                                          int         layout_index,
                                          int         value_index)
{
   static
   const char*    shift_image = "kb_shift";
   static
   unsigned long  shift_flags =  BUTTON_FLAG_ENABLED  |
                                 BUTTON_FLAG_VISIBLE  |
                                 BUTTON_FLAG_COMMAND  |
                                 BUTTON_FLAG_BITMAP;

   unsigned long  new_flags   = 0;
   const char*    new_values  = NULL;


   if( value_index < Hebrew)
   {
      new_flags   = shift_flags;
      new_values  = shift_image;
   }
   else
   {
      if( qwerty_kb_layout != layout_index)
      {
         new_flags = BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE;

         if( Hebrew == value_index)
            new_values = "ץ";
         else
         {
            if( grid_kb_layout == layout_index)
               new_values = "+";
            else
               new_values = "0";
         }
      }
   }

   ssd_keyboard_update_button_data( kb, "Shift", new_values, new_flags);
}

static void update_dynamic_buttons__Hebrew(  SsdWidget   kb,
                                             int         layout_index,
                                             int         value_index)
{
   unsigned long  new_flags   = 0;
   const char*    new_values  = NULL;

   if( qwerty_kb_layout != layout_index)
      return;

   if( Hebrew == value_index)
   {
      new_flags   = BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE;
      new_values  = "ץ";
   }
   else
   {
      new_flags   = 0;
      new_values  = NULL;
   }

   ssd_keyboard_update_button_data( kb, "Hebrew", new_values, new_flags);
}

static void update_dynamic_buttons__Done( SsdWidget   kb,
                                          int         layout_index,
                                          int         value_index)
{
   kb_ctx* ctx = ssd_keyboard_get_context( kb);

   if( NULL == ctx->cbOnSpecialButton)
   {
      ssd_keyboard_update_button_data( kb, "Done", NULL, 0);
      return;
   }

   ssd_keyboard_update_button_data( kb,
                                    "Done",
                                    ctx->special_button_name,
                                    (BUTTON_FLAG_ENABLED|
                                    BUTTON_FLAG_VISIBLE|
                                    BUTTON_FLAG_COMMAND|
                                    BUTTON_FLAG_VALUE_IS_LABEL));
}

static void update_dynamic_buttons( SsdWidget kb)
{
   int   layout_index= ssd_keyboard_get_layout( kb);
   int   value_index = ssd_keyboard_get_value_index( kb);

   update_dynamic_buttons__Shift ( kb, layout_index, value_index);
   update_dynamic_buttons__Hebrew( kb, layout_index, value_index);
   update_dynamic_buttons__Done  ( kb, layout_index, value_index);
}

static void on_command( void* context, const char* command)
{
   kb_ctx* ctx = context;

   if( !strcmp( command, "Done"))
   {
      ctx->cbOnSpecialButton( ctx->context, ctx->special_button_name);
      return;
   }

   if( !strcmp( command, "Shift"))
   {
      int index = ssd_keyboard_get_value_index( ctx->keyboard);

      switch( index)
      {
         case English_highcase:
            ssd_keyboard_set_charset( ctx->keyboard, English_lowcase);
            break;

         case English_lowcase:
            ssd_keyboard_set_charset( ctx->keyboard, English_highcase);
            break;
      }

      return;
   }

   if( !strcmp( command, ".123"))
   {
      char value_index_str[12];

      ssd_keyboard_increment_value_index( ctx->keyboard);

      sprintf( value_index_str, "%d", ssd_keyboard_get_value_index( ctx->keyboard));
      roadmap_config_set( &last_cfg_input, value_index_str);
   }

   else if( !strcmp( command, "Grid"))
   {
      int layout_index= ssd_keyboard_get_layout( ctx->keyboard);

      if( qwerty_kb_layout == layout_index)
         ssd_keyboard_set_grid_layout( ctx->keyboard);
      else
         ssd_keyboard_set_qwerty_layout( ctx->keyboard);
   }

   update_dynamic_buttons( ctx->keyboard);
}

SsdWidget ssd_get_keyboard( SsdWidget container)
{ return ssd_widget_get( container, keyboard_name);}

BOOL on_key( void* context, const char* utf8char, uint32_t flags)
{
   kb_ctx* ctx = context;

   return ctx->cbOnKey( ctx->context, utf8char, flags);
}

void  ssd_keyboard_set_grid_layout( SsdWidget kb)
{
   int value_index;

   value_index = ssd_keyboard_get_value_index( kb);
   if( roadmap_horizontal_screen_orientation())
      ssd_keyboard_set_layout( kb, widegrid_kb_layout);
   else
      ssd_keyboard_set_layout( kb, grid_kb_layout);

   roadmap_config_set( &last_cfg_layout, "GRID");

   ssd_keyboard_set_charset( kb, value_index);

   update_dynamic_buttons( kb);
}

void ssd_keyboard_set_qwerty_layout( SsdWidget kb)
{
   int value_index = ssd_keyboard_get_value_index( kb);

   ssd_keyboard_set_layout( kb, qwerty_kb_layout);
   roadmap_config_set( &last_cfg_layout, "QWERTY");
   ssd_keyboard_set_charset( kb, value_index);

   update_dynamic_buttons( kb);
}

void ssd_keyboard_set_charset( SsdWidget kb, kb_characters_set cset)
{
   char value_index_str[12];

   ssd_keyboard_set_value_index( kb, (int)cset);

   sprintf( value_index_str, "%d", (int)cset);
   roadmap_config_set( &last_cfg_input, value_index_str);
}

void  ssd_keyboard_verify_orientation( SsdWidget kb)
{
   int layout_index = ssd_keyboard_get_layout( kb);

   if( qwerty_kb_layout != layout_index)
      ssd_keyboard_set_grid_layout( kb);
}

static void on_draw_begin( void* context, int layout_index)
{
   kb_ctx* ctx = context;

   if( ctx->orientation_id != orientation_id)
   {
      ssd_keyboard_verify_orientation( ctx->keyboard);
      ctx->orientation_id = orientation_id;
   }
}

static void on_draw_button( void* context, int layout_index, int button_index)
{
   // Hide line '1,2,3,..,0' if the NUMBERS-AND-DIGITS line is showing now
   if( (layout_index != widegrid_kb_layout) && (button_index < 10))
   {
      buttons_layout_info* layout= s_keyboards_layout[layout_index];
      button_info*         btn   = layout->buttons + button_index;

      if( Nunbers_and_symbols == layout->value_index)
         btn->flags &= ~BUTTON_FLAG_VISIBLE;
      else
         btn->flags |= BUTTON_FLAG_VISIBLE;
   }
}

static void on_draw_end( void* context, int layout_index)
{
// kb_ctx* ctx = context;
}

static BOOL on_approve_value_type( void* context, int index_candidate)
{
   return (enable_Hebrew || ( Hebrew != index_candidate));
}

static void OnDeviceEvent( device_event event, void* context)
{
   if( device_event_window_orientation_changed == event)
      orientation_id++;
}

static void reload_last_configuration( SsdWidget kb)
{
   const char* last_layout = roadmap_config_get( &last_cfg_layout);
   // const char* last_input  = roadmap_config_get( &last_cfg_input);
   int         input_index = English_lowcase;
   int         cur_input   = -1;
   if( enable_Hebrew)
      input_index = Hebrew;

   if( last_layout && (*last_layout))
   {
      BOOL qwerty_layout = (0 == strcmp( last_layout, "QWERTY"));

      cur_input = ssd_keyboard_get_value_index( kb);

      if( qwerty_layout)
         ssd_keyboard_set_layout( kb, qwerty_kb_layout);
      else
      {
         if( roadmap_horizontal_screen_orientation())
            ssd_keyboard_set_layout( kb, widegrid_kb_layout);
         else
            ssd_keyboard_set_layout( kb, grid_kb_layout);
      }
   }

   ssd_keyboard_set_value_index( kb, input_index);
   /*if( last_input && (*last_input))
   {
      int input_index = atoi( last_input);

      if( cur_input != input_index)
         ssd_keyboard_set_value_index( kb, input_index);
   }
   else
   {
      if( -1 != cur_input)
         ssd_keyboard_set_value_index( kb, cur_input);
   }*/

   update_dynamic_buttons( kb);
}

void  ssd_keyboard_reset_state( SsdWidget kb)
{
   reload_last_configuration( kb);
}

SsdWidget ssd_create_keyboard(SsdWidget                  container,
                              CB_OnKeyboardButtonPressed cbOnKey,
                              CB_OnKeyboardCommand       cbOnSpecialButton,
                              const char*                special_button_name,
                              void*                      context)
{
   SsdWidget   kb;

   kb_ctx*     ctx = malloc( sizeof( kb_ctx));

   memset( ctx, 0, sizeof( kb_ctx));

   ctx->container          = container;
   ctx->cbOnKey            = cbOnKey;
   ctx->cbOnSpecialButton  = cbOnSpecialButton;
   ctx->special_button_name= special_button_name;
   ctx->context            = context;
   ctx->orientation_id     = orientation_id;

   kb = ssd_keyboard_layout_create(
                     // SSD usage:
                     keyboard_name,
                     SSD_ALIGN_CENTER|SSD_END_ROW,
                     // Layout(s) info:
                     s_keyboards_layout,
                     sizeof(s_keyboards_layout)/sizeof(buttons_layout_info*),
                     // This module:
                     on_key,
                     on_command,
                     on_draw_begin,
                     on_draw_button,
                     on_draw_end,
                     on_approve_value_type,
                     ctx);


   ctx->keyboard = kb;

   ssd_widget_add( container, kb);

   if( first_time_round)
   {
      const char* system_lang = roadmap_lang_get( "lang");

      enable_Hebrew = (0 == strcmp( system_lang, "Hebrew"));

      roadmap_config_declare( "preferences",
                              &last_cfg_layout,
                              "",
                              NULL);

      roadmap_config_declare( "preferences",
                              &last_cfg_input,
                              "",
                              NULL);

      roadmap_device_events_register( OnDeviceEvent, NULL);
      first_time_round = FALSE;
   }

   reload_last_configuration( kb);

   return kb;
}

/*************************************************************************************************
 * ssd_keyboard_edit_box_top_offset()
 * Returns the offset from the top border for the edit box placement
 * according to the platform and screen size
 * TODO:: Check if the generic computation is possible
 */
int ssd_keyboard_edit_box_top_offset()
{
	int top_offset = 0;

#ifdef ANDROID
   BOOL is_portrait = ( roadmap_canvas_width() < roadmap_canvas_height() );
	if ( roadmap_native_keyboard_enabled() )
	{
		if ( is_portrait )
			top_offset = 40;
		else
			top_offset = 5;
	}
#endif

	return top_offset;
}
